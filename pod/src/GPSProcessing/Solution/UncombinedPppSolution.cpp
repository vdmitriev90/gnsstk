#include "UncombinedPppSolution.h"

#include "AmbiguitiesEquations.h"
#include "AntexReader.hpp"
#include "BasicModel.hpp"
#include "ClockBiasEquations.h"
#include "ComputeSatPCenter.hpp"
#include "ComputeTropModel.hpp"
#include "ComputeWeightSimple.h"
#include "ComputeWindUp.hpp"
#include "CorrectObservables.hpp"
#include "Decimate.hpp"
#include "EclipsedSatFilter.hpp"
#include "GravitationalDelay.hpp"
#include "InterFrequencyBiases.h"
#include "InterSystemBias.h"
#include "IonexModel.hpp"
#include "IonoEquations.h"
#include "IonoStochasticModel.h"
#include "KalmanSolver.h"
#include "KalmanSolverFB.h"
#include "LICSDetector2.hpp"
#include "LinearCombinations.hpp"
#include "MWCSDetector.hpp"
#include "NeillTropModel.hpp"
#include "NumSatFilter.h"
#include "ObsRangeFilter.h"
#include "ObservablesSets.h"
#include "PositionEquations.h"
#include "PrefitResCatcher.h"
#include "SatArcMarker.hpp"
#include "TidalDisplacement.h"
#include "TropoEquations.h"
#include "UsedInPvtMarker.hpp"

#include <memory>

using namespace gnsstk;

namespace pod
{

    UncombinedPppSolution::UncombinedPppSolution(GnssDataStorePtr data_ptr) : GnssSolution(data_ptr, 50.0) {}
    UncombinedPppSolution::UncombinedPppSolution(GnssDataStorePtr data_ptr, double max_sigma)
        : GnssSolution(data_ptr, max_sigma)
    {
    }

    UncombinedPppSolution::~UncombinedPppSolution() {}

    void UncombinedPppSolution::process()
    {
        updateRequaredObs();

        ObsRangeFilter CodePhaseFilterBase(
            {ObsRangeType::FirstCode, ObsRangeType::SecondCode, ObsRangeType::FirstPhase, ObsRangeType::SecondPhase});
        ObsRangeFilter CodePhaseFilterRover(CodePhaseFilterBase);

        ObsRangeFilter SNRFilterBase(ObsRangeType::Snr, processingConfig().snrMaskDbHz, DBL_MAX);
        ObsRangeFilter SNRFilterRover(SNRFilterBase);

        // Object to remove eclipsed satellites
        EclipsedSatFilter eclipsedSV;
        ComputeWeightSimple computeWeightSimple(1);

        const auto ref_base_pos = data_->getNominalPosition(projectConfig().siteBase);
        // basic model object for ref. station
        BasicModel modelRef(ref_base_pos, data_->navLibrary_, TypeID::C1);
        modelRef.setMinElev(processingConfig().elevationMaskDeg);

        BasicModel modelRover(data_->navLibrary_);
        modelRover.setDefaultObservable(TypeID::C1);
        modelRover.setMinElev(processingConfig().elevationMaskDeg);

        RinexEpoch rin_epoch;

        // Object to decimate data
        Decimate decimateData(processingConfig().decimationIntervalSeconds,
                              processingConfig().decimationToleranceSeconds,
                              data_->navLibrary_.getInitialTime());

#pragma region troposhere modeling objects

        // for base
        NeillTropModel tropoBasePtr;
        ComputeTropModel computeTropoBase(tropoBasePtr);

        // for rover
        NeillTropModel tropoRovPtr;
        ComputeTropModel computeTropoRover(tropoRovPtr);

#pragma endregion

        IonexModel ionoModel(nominalPos_, data_->ionexStore_, TypeID::C1, false);

        // Object to compute gravitational delay effects
        GravitationalDelay grDelayBase(ref_base_pos);
        GravitationalDelay grDelayRover;

        // Objects to mark cycle slips
        // Checks LI cycle slips
        LICSDetector2 markCSLI2Rover;
        markCSLI2Rover.setSatThreshold(processingConfig().lossOfLockThreshold);

        // Checks Merbourne-Wubbena cycle slips
        MWCSDetector markCSMW2Rover;
        markCSMW2Rover.setMaxNumLambdas(processingConfig().MelbourneWubbenaThreshold);

        PrefitResCatcher resCatcher(config_.slots_);
        NumSatFilter minSatFilter(desiredSlnType());

        // Object to keep track of satellite arcs
        SatArcMarker markArcRover(TypeID::CSL1, true, 31.0);

        AntexReader antexReader;
        antexReader.open(projectConfig().genericFiles.antexFilePath());

        CorrectObservables corrRover(data_->navLibrary_);

        // Vector from monument to antenna ARP [UEN], in meters
        // for base
        //const Triple offsetARP = stationConfig(data_->config_.siteBase).markerToArpUenM;
        //corrBase.setMonument(offsetARP);

        // for rover
        //const Triple offsetARP_Rover = stationConfig(data_->config_.siteRover).markerToArpUenM;
        //corrRover.setMonument(offsetARP_Rover);

        // Objects to compute tidal effects
        TidalDisplacement tides(data_->eopStore_);
        tides.setOceanLoadingFile(projectConfig().genericFiles.oceanLoadingFilePath());

        ComputeWindUp windupRover(
            data_->navLibrary_, ref_base_pos, projectConfig().genericFiles.satelliteDataFilePath());

        ComputeSatPCenter svPcenterRover(ref_base_pos, projectConfig().genericFiles.satelliteDataFilePath());
        svPcenterRover.setAntexReader(antexReader);

        ProcessLinear linear_comb;
        linear_comb.add(std::make_unique<PCCombination>());
        linear_comb.add(std::make_unique<LCCombimnation>());

        UsedInPvtMarker useMarker;

        KalmanSolver solver(equations_, processingConfig());
        solver.setMinSatNumber(5);
        KalmanSolverFB solverFb(equations_, processingConfig());
        solverFb.setMinSatNumber(5);
        if (forwardBackwardCycles_ > 0)
        {
            solverFb.setCyclesNumber(forwardBackwardCycles_);
            solverFb.setLimits(processingConfig().codeLimits, processingConfig().phaseLimits);

            solverFb.ReProcList().push_back(markCSLI2Rover);
            solverFb.ReProcList().push_back(markCSMW2Rover);
            solverFb.ReProcList().push_back(markArcRover);
        }

        bool firstTime = true;
        //
        for (auto& obsFile : data_->getObsFiles(projectConfig().siteRover))
        {
            std::cout << obsFile << std::endl;
            // Input observation file stream
            Rinex3ObsStream rin;
            // Open Rinex observations file in read-only mode
            rin.open(obsFile, std::ios::in);
            rin.exceptions(std::ios::failbit);

            // read the header
            Rinex3ObsHeader roh;
            rin >> roh;
            gMap_.header = roh;

            setupAntenna(corrRover, antexReader, projectConfig().siteRover, roh.antType);

            // read all epochs
            while (rin >> rin_epoch)
            {
                if (rin_epoch.getBody().empty())
                {
                    printMsg(rin_epoch.getHeader().epoch, "Empty epoch record in Rinex file");
                    continue;
                }

                const auto& t = rin_epoch.getHeader().epoch;

                // keep only satellites from satellites systems selecyted for processing
                rin_epoch.keepOnlySatSystems(processingConfig().satelliteSystems);

                // get approximate position
                if (apprPos().getPosition(rin_epoch, nominalPos_))
                    continue;

                grDelayRover.setNominalPosition(nominalPos_);

                tropoRovPtr.setAllParameters(t, nominalPos_);
                tropoBasePtr.setAllParameters(t, ref_base_pos);
                ionoModel.setInitialRxPosition(nominalPos_);

                modelRover.setRxPosition(nominalPos_);
                corrRover.setNominalPosition(nominalPos_);
                windupRover.setNominalPosition(nominalPos_);
                svPcenterRover.setNominalPosition(nominalPos_);

                rin_epoch >> requireObs_;
                rin_epoch >> CodePhaseFilterRover;
                rin_epoch >> SNRFilterRover;

                if (rin_epoch.getBody().empty())
                {
                    printMsg(rin_epoch.getHeader().epoch, "Rover receiver: all SV has been rejected.");
                    continue;
                }

                rin_epoch >> computeLinear_;
                rin_epoch >> markCSLI2Rover;
                rin_epoch >> markCSMW2Rover;
                //rin_epoch >> snrCatcherL1Rover;
                rin_epoch >> markArcRover;

                rin_epoch >> modelRover;
                // rin_epoch >> eclipsedSV;
                rin_epoch >> grDelayRover;
                rin_epoch >> svPcenterRover;

                corrRover.setExtraBiases(tides.getDisplacement(t, nominalPos_, projectConfig().siteRover));
                rin_epoch >> corrRover;

                rin_epoch >> windupRover;
                rin_epoch >> computeTropoRover;
                // rin_epoch >> ionoFilterRover;
                rin_epoch >> linear_comb;
                rin_epoch >> oMinusC_;
                rin_epoch >> resCatcher;
                rin_epoch >> computeWeightSimple;

                rin_epoch >> minSatFilter;
                rin_epoch >> useMarker;

                if (forwardBackwardCycles_ > 0)
                {
                    rin_epoch >> solverFb;
                }
                else
                {
                    rin_epoch >> solver;
                    auto ep = makeEpochToOutput(rin_epoch.getBody());
                    // updateNomPos(solverFB);
                    printSolution(solver, t, ep);
                    gMap_.data.insert(std::make_pair(t, ep));
                }
            }
        }
        if (forwardBackwardCycles_ > 0)
        {
            markCSLI2Rover.setIsReprocess(true);
            markCSMW2Rover.setIsReprocess(true);

            std::cout << "Fw-Bw part started" << std::endl;
            solverFb.reProcess();
            RinexEpoch rin_epoch;
            std::cout << "Last process part started" << std::endl;
            while (solverFb.lastProcess(rin_epoch))
            {
                auto ep = makeEpochToOutput(rin_epoch.getBody());
                // updateNomPos(solverFB);
                printSolution(solverFb, rin_epoch.getHeader().epoch, ep);
                gMap_.data.insert(std::make_pair(rin_epoch.getHeader().epoch, ep));
            }
            std::cout << "Measurments rejected: " << solverFb.rejectedMeasurements << std::endl;
        }

        // CatcherStatistic statB(opts().workingDir+"\\"+data_->config_.siteBase+".cst");
        // statB.logStatistic(plBase);
        // CatcherStatistic statR(opts().workingDir+"\\"+data_->config_.siteRover+".cst");
        // statR.logStatistic(plRov);
    }

    void UncombinedPppSolution::updateRequaredObs()
    {
        computeLinear_.add(std::make_unique<PDelta>());
        computeLinear_.add(std::make_unique<MWoubenna>());

        computeLinear_.add(std::make_unique<LDelta>());
        computeLinear_.add(std::make_unique<LICombimnation>());

        configureSolver();

        requireObs_ = RequireObservablesBuilder(processingConfig().satelliteSystems).build();

        if (processingConfig().carrierBands.find(CarrierBand::L1) != processingConfig().carrierBands.end())
        {
            oMinusC_.add(std::make_unique<PrefitC1>(true));
            oMinusC_.add(std::make_unique<PrefitL1>());

            config_.slots_.push_back(ObsSlot::FirstBandCode);
            config_.slots_.push_back(ObsSlot::FirstBandPhase);
        }
        if (processingConfig().carrierBands.find(CarrierBand::L2) != processingConfig().carrierBands.end())
        {
            oMinusC_.add(std::make_unique<PrefitC2>(true));
            oMinusC_.add(std::make_unique<PrefitL2>());

            config_.slots_.push_back(ObsSlot::SecondBandCode);
            config_.slots_.push_back(ObsSlot::SecondBandPhase);
        }
    }

    void UncombinedPppSolution::configureSolver()
    {
        equations_->clearEquations();

        // tropo
        if (processingConfig().computeTroposphere)
        {
            double qPrimeVert = processingConfig().troposphereVerticalProcessNoise;
            equations_->addEquation(std::make_unique<TropoEquations>(qPrimeVert));
        }

        //  Equations->addEquation(std::make_unique<IonoEquations>(processingConfig().ionoQ));
        // White noise stochastic models
        auto coord = std::make_unique<PositionEquations>();

        double posSigma = processingConfig().positionSigmaM;
        if (processingConfig().dynamics == Dynamics::Static)
        {
            coord->setStochasicModel(std::make_shared<ConstantModel>());
        }
        else if (processingConfig().dynamics == Dynamics::Kinematic)
        {
            coord->setStochasicModel(std::make_shared<WhiteNoiseModel>(posSigma));
        }
        else if (processingConfig().dynamics == Dynamics::RandomWalk)
        {
            for (const auto& it : coord->getParameters())
                coord->setStochasicModel(it, std::make_shared<RandomWalkModel>(posSigma));
        }

        // add position equations
        equations_->addEquation(std::move(coord));

        equations_->addEquation(std::make_unique<ClockBiasEquations>(processingConfig().clockSigmaM));

        if (processingConfig().satelliteSystems.size() > 1)
            equations_->addEquation(std::make_unique<InterSystemBias>());

        if (processingConfig().carrierBands.size() > 1)
            equations_->addEquation(std::make_unique<InterFrequencyBiases>());

        if (processingConfig().computeIonosphere)
        {
            auto ionoEq = std::make_unique<IonoEquations>();

            int ionoModelType = processingConfig().ionosphereModelType;

            if (ionoModelType == 0)
                ionoEq->setStocModel<WhiteNoiseModel>();
            else if (ionoModelType == 1)
                ionoEq->setStocModel<IonoStochasticModel>();

            double ionoSigma = processingConfig().ionosphereSigmaM;
            ionoEq->setSigma(ionoSigma);
            equations_->addEquation(std::move(ionoEq));
        }

        if (processingConfig().carrierBands.find(CarrierBand::L1) != processingConfig().carrierBands.end())
            equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BL1));

        if (processingConfig().carrierBands.find(CarrierBand::L2) != processingConfig().carrierBands.end())
            equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BL2));

        forwardBackwardCycles_ = processingConfig().forwardBackwardCycles;
    }
} // namespace pod
