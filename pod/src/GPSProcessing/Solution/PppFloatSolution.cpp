#include "PppFloatSolution.h"

#include "AdvClockModel.h"
#include "AmbiguitiesEquations.h"
#include "AntexReader.hpp"
#include "BasicModel.hpp"
#include "BiasSinexCorrector.h"
#include "ClockBiasEquations.h"
#include "ComputeSatPCenter.hpp"
#include "ComputeTropModel.hpp"
#include "ComputeWeightSimple.h"
#include "ComputeWindUp.hpp"
#include "CorrectObservables.hpp"
#include "Decimate.hpp"
#include "EclipsedSatFilter.hpp"
#include "ElevationMask.hpp"
#include "GravitationalDelay.hpp"
#include "InterFrequencyBiases.h"
#include "InterSystemBias.h"
#include "IonexModel.hpp"
#include "IonoEquations.h"
#include "IonoStochasticModel.h"
#include "KalmanSolverFB.h"
#include "LICSDetector.hpp"
#include "LICSDetector2.hpp"
#include "LinearCombinations.hpp"
#include "MJD.hpp"
#include "MWCSDetector.hpp"
#include "ObsRangeFilter.h"
#include "ObservablesSets.h"
#include "PositionEquations.h"
#include "PowerSum.hpp"
#include "PrefitResCatcher.h"
#include "SNRCatcher.h"
#include "SatArcMarker.hpp"
#include "StringUtils.h"
#include "TidalDisplacement.h"
#include "TropoEquations.h"
#include "TropoEquationsAdv.hpp"
#include "TropoGradEquations.h"
#include "UsedInPvtMarker.hpp"
#include "WinUtils.h"

using namespace gnsstk;

namespace pod
{
    PppFloatSolution::PppFloatSolution(GnssDataStorePtr data_ptr)
        : GnssSolution(data_ptr, 50.0)
        , computeTropoRover_(tropoRover_, false)
    {
    }

    PppFloatSolution::PppFloatSolution(GnssDataStorePtr data_ptr, double max_sigma)
        : GnssSolution(data_ptr, max_sigma)
        , computeTropoRover_(tropoRover_, false)
    {
    }

    void PppFloatSolution::process()
    {
        updateRequaredObs();

        BasicModel model(data_->navLibrary_);
        model.setDefaultObservable(TypeID::C1);
        model.setMinElev(.0);

        ElevationMask elMask(processingConfig().elevationMaskDeg);

        ObsRangeFilter CodeFilter(
            {ObsRangeType::FirstCode, ObsRangeType::SecondCode, ObsRangeType::FirstPhase, ObsRangeType::SecondPhase});
        ObsRangeFilter SNRFilter(ObsRangeType::Snr, processingConfig().snrMaskDbHz, DBL_MAX);
        // Object to remove eclipsed satellites
        EclipsedSatFilter eclipsedSV;

        RinexEpoch rin_epoch;

        // Object to decimate data
        Decimate decimateData(processingConfig().decimationIntervalSeconds,
                              processingConfig().decimationToleranceSeconds,
                              data_->navLibrary_.getInitialTime());

        // Object to compute gravitational delay effects
        GravitationalDelay grDelayRover;

        // for rover
        computeTropoRover_.setTropModel(tropoRover_);

#pragma region CS detectors

        // Objects to mark cycle slips
        // Checks LI cycle slips
        LICSDetector2 markCSLI2Rover;
        markCSLI2Rover.setSatThreshold(processingConfig().lossOfLockThreshold);

        // Checks Merbourne-Wubbena cycle slips
        MWCSDetector markCSMW2Rover;
        markCSMW2Rover.setMaxNumLambdas(processingConfig().MelbourneWubbenaThreshold);

        // check sharp SNR drops
        SNRCatcher snrCatcherL1Rover;
        PrefitResCatcher resCatcher(config_.slots_);

        // Object to keep track of satellite arcs
        SatArcMarker markArcRover(TypeID::CSL1, true, 31.0);

#pragma endregion

        AntexReader antexReader;
        antexReader.open(data_->config_.genericFiles.antexFilePath());

#pragma region correct observable

        CorrectObservables corrRover(data_->navLibrary_);

#pragma endregion

        ComputeWeightSimple computeWeightSimple(2.0);
        // Objects to compute tidal effects
        TidalDisplacement tides(data_->eopStore_);
        tides.setUseOcean(true);
        // Configure ocean loading model
        tides.setOceanLoadingFile(data_->config_.genericFiles.oceanLoadingFilePath());

        ComputeWindUp windupRover(data_->navLibrary_, data_->config_.genericFiles.satelliteDataFilePath());

        ComputeSatPCenter svPcenterRover(data_->config_.genericFiles.satelliteDataFilePath());
        svPcenterRover.setAntexReader(antexReader);

        ProcessLinear linearIonoFree;
        linearIonoFree.add(std::make_unique<PCCombination>());
        linearIonoFree.add(std::make_unique<LCCombimnation>());

        BiasSinexCorrector biasSinexCorrector(data_->biasSinexStore_);

        UsedInPvtMarker useMarker;
        KalmanSolver solver(equations_);
        KalmanSolverFB solverFb(equations_);

        if (forwardBackwardCycles_ > 0)
        {
            solverFb.setCyclesNumber(forwardBackwardCycles_);
            solverFb.setLimits(processingConfig().codeLimits,
                               processingConfig().phaseLimits);
            solverFb.setCSDetRef(markCSLI2Rover, markCSMW2Rover);

            solverFb.ReProcList().push_back(markCSLI2Rover);
            solverFb.ReProcList().push_back(markCSMW2Rover);
            solverFb.ReProcList().push_back(markArcRover);
            solverFb.ReProcList().push_back(elMask);
        }

        bool firstTime = true;
        //
        for (auto& obsFile : data_->getObsFiles(data_->config_.siteRover))
        {
            std::cout << obsFile << std::endl;
            // Input observation file stream
            Rinex3ObsStream rin;

            // Open Rinex observations file in read-only mode
            rin.open(obsFile, std::ios::in);

            rin.exceptions(std::ios::failbit);
            Rinex3ObsHeader roh;

            // read the header
            rin >> roh;
            gMap_.header = roh;

            setupAntenna(corrRover, antexReader, data_->config_.siteRover, roh.antType);

            // read all epochs
            while (rin >> rin_epoch)
            {
                if (decimateData.check(rin_epoch))
                    continue;

                if (rin_epoch.getBody().empty())
                {
                    printMsg(rin_epoch.getHeader().epoch, "Empty epoch record in Rinex file");
                    continue;
                }

                const auto& t = rin_epoch.getHeader().epoch;
#if _DEBUG
                bool b;

                CATCH_TIME(t, 2014, 12, 19, 0, 14, 15, b)
                if (b)
                    DBOUT_LINE("catched")
#endif
                // keep only satellites from satellites systems selected for processing
                rin_epoch.keepOnlySatSystems(processingConfig().satelliteSystems);

                rin_epoch >> requireObs_;

                // get approximate position
                if (apprPos().getPosition(rin_epoch, nominalPos_))
                    continue;
                grDelayRover.setNominalPosition(nominalPos_);

                tropoRover_.setAllParameters(t, nominalPos_);

                corrRover.setNominalPosition(nominalPos_);
                windupRover.setNominalPosition(nominalPos_);
                svPcenterRover.setNominalPosition(nominalPos_);
                model.setRxPosition(nominalPos_);

                rin_epoch >> CodeFilter;
                rin_epoch >> SNRFilter;

                if (rin_epoch.getBody().empty())
                {
                    printMsg(rin_epoch.getHeader().epoch, "Rover receiver: all SV has been rejected.");
                    continue;
                }

                rin_epoch >> biasSinexCorrector;

                rin_epoch >> computeLinear_;

                rin_epoch >> model;
                rin_epoch >> eclipsedSV;
                rin_epoch >> grDelayRover;
                rin_epoch >> svPcenterRover;

                corrRover.setExtraBiases(tides.getDisplacement(t, nominalPos_, data_->config_.siteRover));
                rin_epoch >> corrRover;

                rin_epoch >> windupRover;
                rin_epoch >> computeTropoRover_;

                rin_epoch >> linearIonoFree;
                rin_epoch >> oMinusC_;
                rin_epoch >> resCatcher;
                rin_epoch >> computeWeightSimple;

                rin_epoch >> useMarker;
                rin_epoch >> markCSLI2Rover;
                rin_epoch >> markCSMW2Rover;
                rin_epoch >> markArcRover;
                rin_epoch >> elMask;
                // rin_epoch >> snrCatcherL1Rover;

                // DBOUT_LINE(">>" << CivilTime(rin_epoch.getHeader().epoch).asString());

                if (forwardBackwardCycles_ > 0)
                {
                    solverFb.setMinSatNumber(4 /*+ rin_epoch.getBody().getSatSystems().size()*/);
                    rin_epoch >> solverFb;
                }
                else
                {
                    solver.setMinSatNumber(4 /*+ rin_epoch.getBody().getSatSystems().size()*/);
                    rin_epoch >> solver;
                    auto ep = makeEpochToOutput(rin_epoch.getBody());
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
                // fill GnssEpoch by IRinex object data
                GnssEpoch epoch_to_output = makeEpochToOutput(rin_epoch.getBody());

                // update nominal position
                apprPos().getPosition(rin_epoch, nominalPos_);

                // fill GnssEpoch by filter state data
                printSolution(solverFb, rin_epoch.getHeader().epoch, epoch_to_output);

                // add epoch to map
                gMap_.data.insert(std::make_pair(rin_epoch.getHeader().epoch, epoch_to_output));
            }
            std::cout << "Measurments rejected: " << solverFb.rejectedMeasurements << std::endl;
        }
    }

    void PppFloatSolution::storeReceiverParams(const KalmanSolver& solver,
                                               const FilterParameter& param,
                                               GnssEpoch& ep) const
    {

        if (param.type == TypeID::wetMap)
        {
            const double total_z_delay =
                tropoRover_.dry_zenith_delay() + tropoRover_.wet_zenith_delay() + solver.getSolution(param);

            ep.slnData[TypeID::recZTropo] = total_z_delay;

            return;
        }

        // Fallback to base implementation
        GnssSolution::storeReceiverParams(solver, param, ep);
    }

    void PppFloatSolution::updateRequaredObs()
    {
        computeLinear_.add(std::make_unique<PDelta>());
        computeLinear_.add(std::make_unique<MWoubenna>());

        computeLinear_.add(std::make_unique<LDelta>());
        computeLinear_.add(std::make_unique<LICombimnation>());

        configureSolver();
        requireObs_ = RequireObservablesBuilder(processingConfig().satelliteSystems).build();

        oMinusC_.add(std::make_unique<PrefitPC>(true));
        oMinusC_.add(std::make_unique<PrefitLC>());

        config_.slots_ = {ObsSlot::CodeIonoFree, ObsSlot::PhaseIonoFree};
    }

    void PppFloatSolution::configureSolver()
    {
        equations_->clearEquations();

        double qPrimeVert = processingConfig().troposphereQ1;
        double qPrimeHor = processingConfig().troposphereQ2;

        if (processingConfig().troposphereModelType == TropoModelType::Simple)
        {
            equations_->addEquation(std::make_unique<TropoEquations>(qPrimeVert));
        }
        else if (processingConfig().troposphereModelType == TropoModelType::SimpleWithGradients)
        {
            equations_->addEquation(std::make_unique<TropoGradEquations>(qPrimeVert, qPrimeHor, qPrimeHor));
        }
        else if (processingConfig().troposphereModelType == TropoModelType::Advanced)
        {
            equations_->addEquation(std::make_unique<TropoEquationsAdv>(qPrimeVert, qPrimeHor));
        }

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

        if (processingConfig().useAdvancedClockModel)
            equations_->addEquation(std::make_unique<AdvClockModel>(processingConfig().clockQ1,
                                                                    processingConfig().clockQ2));
        else
            equations_->addEquation(std::make_unique<ClockBiasEquations>());

        if (processingConfig().satelliteSystems.size() > 1)
            equations_->addEquation(std::make_unique<InterSystemBias>());

        equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BLC));

        forwardBackwardCycles_ = processingConfig().forwardBackwardCycles;
    }
} // namespace pod
