#include "PdFloatSolution.h"

#include "AmbiguitiesEquations.h"
#include "AntexReader.hpp"
#include "BasicModel.hpp"
#include "CatcherStatistic.h"
#include "ClockBiasEquations.h"
#include "ComputeSatPCenter.hpp"
#include "ComputeTropModel.hpp"
#include "ComputeWeightSimple.h"
#include "ComputeWindUp.hpp"
#include "CorrectObservables.hpp"
#include "Decimate.hpp"
#include "DeltaOp.hpp"
#include "EclipsedSatFilter.hpp"
#include "GravitationalDelay.hpp"
#include "InterFrequencyBiases.h"
#include "InterSystemBias.h"
#include "IonexModel.hpp"
#include "IonoEquations.h"
#include "IonoStochasticModel.h"
#include "KalmanSolver.h"
#include "KalmanSolverFB.h"
#include "LICSDetector.hpp"
#include "LICSDetector2.hpp"
#include "LinearCombinations.hpp"
#include "MJD.hpp"
#include "MWCSDetector.hpp"
#include "NeillTropModel.hpp"
#include "NumSatFilter.h"
#include "OceanLoading.hpp"
#include "PoleTides.hpp"
#include "PositionEquations.h"
#include "PowerSum.hpp"
#include "PrefitResCatcher.h"
#include "SNRCatcher.h"
#include "SatArcMarker.hpp"
#include "SimpleFilter.hpp"
#include "SolidTides.hpp"
#include "SyncObs.h"
#include "TropoEquations.h"
#include "UsedInPvtMarker.hpp"
#include "ObservablesSets.h"
#include "WinUtils.h"

#include <memory>

using namespace gnsstk;

namespace pod
{

    PdFloatSolution::PdFloatSolution(GnssDataStorePtr data_ptr) : GnssSolution(data_ptr, 50.0) {}
    PdFloatSolution::PdFloatSolution(GnssDataStorePtr data_ptr, double max_sigma)
        : GnssSolution(data_ptr, max_sigma)
    {
    }
    PdFloatSolution::~PdFloatSolution() {}

    void PdFloatSolution::process()
    {
        updateRequaredObs();

        SimpleFilter CodePhaseFilterBase(TypeIDSet{data_->getGpsGloL1CodeType(), TypeID::P2, TypeID::L1, TypeID::L2});
        SimpleFilter CodePhaseFilterRover(CodePhaseFilterBase);

        SimpleFilter SNRFilterBase(TypeID::S1, confReader().getValueAsInt("SNRmask"), DBL_MAX);
        SimpleFilter SNRFilterRover(SNRFilterBase);

        // Object to remove eclipsed satellites
        EclipsedSatFilter eclipsedSV;
        ComputeWeightSimple computeWeightSimple(1);

        const auto ref_base_pos = data_->getNominalPosition(opts().SiteBase);
        // basic model object for ref. station
        BasicModel modelRef(ref_base_pos, data_->navLibrary_, data_->getGpsGloL1CodeType());
        modelRef.setMinElev(opts().maskEl);

        BasicModel modelRover(data_->navLibrary_);
        modelRover.setDefaultObservable(data_->getGpsGloL1CodeType());
        modelRover.setMinElev(opts().maskEl);

        RinexEpoch rin_epoch, gRef;
        SyncObs sync(data_->getObsFiles(opts().SiteBase), rin_epoch);

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

#pragma region troposhere modeling objects

        // for base
        NeillTropModel tropoBasePtr;
        ComputeTropModel computeTropoBase(tropoBasePtr);

        // for rover
        NeillTropModel tropoRovPtr;
        ComputeTropModel computeTropoRover(tropoRovPtr);

#pragma endregion

        IonexModel ionoModel(nominalPos_, data_->ionexStore, TypeID::C1, false);

        // Object to compute gravitational delay effects
        GravitationalDelay grDelayBase(ref_base_pos);
        GravitationalDelay grDelayRover;

#pragma region Catcher objects

        // Objects to mark cycle slips
        // Checks LI cycle slips
        LICSDetector2 markCSLI2Base, markCSLI2Rover;
        markCSLI2Base.setSatThreshold(confReader().getValueAsDouble("LISatThreshold"));
        markCSLI2Rover.setSatThreshold(confReader().getValueAsDouble("LISatThreshold"));

        // Checks Merbourne-Wubbena cycle slips
        MWCSDetector markCSMW2Base, markCSMW2Rover;
        markCSMW2Base.setMaxNumLambdas(confReader().getValueAsDouble("MWNLambdas"));
        markCSMW2Rover.setMaxNumLambdas(confReader().getValueAsDouble("MWNLambdas"));

        // check sharp SNR drops
        SNRCatcher snrCatcherL1Base(TypeID::S1, TypeID::CSL1, 901.0, 5, 30);
        SNRCatcher snrCatcherL1Rover(TypeID::S1, TypeID::CSL1, 901.0, 5, 30);
        PrefitResCatcher resCatcher(equations_->measTypes());
        NumSatFilter minSatFilter(desiredSlnType());

        // Object to keep track of satellite arcs
        SatArcMarker markArcBase(TypeID::CSL1, true, 31.0);
        SatArcMarker markArcRover(TypeID::CSL1, true, 31.0);

#pragma endregion

#pragma region prepare ANTEX reader

        std::string antxfile = opts().genericFilesDirectory;
        antxfile += confReader().getValue("antexFile");

        AntexReader antexReader;
        antexReader.open(antxfile);

#pragma endregion

#pragma region correct observable

        CorrectObservables corrBase(data_->navLibrary_);
        corrBase.setNominalPosition(ref_base_pos);

        CorrectObservables corrRover(data_->navLibrary_);

        // Vector from monument to antenna ARP [UEN], in meters
        // for base
        const Triple offsetARP = confReader().getValueListAsTriple("offsetARP", opts().SiteBase);
        corrBase.setMonument(offsetARP);

        // for rover
        const Triple offsetARP_Rover =
            confReader().getValueListAsTriple("offsetARP", opts().SiteRover);
        corrRover.setMonument(offsetARP_Rover);

        Antenna baseAnt(
            antexReader.getAntenna(confReader().getValue("antennaModel", opts().SiteBase)));
        corrBase.setAntenna(baseAnt);
        corrBase.setUsePcv(confReader().getValueAsBoolean("usePCPatterns", opts().SiteBase));
        corrBase.setUseAzimuth(confReader().getValueAsBoolean("useAzim", opts().SiteBase));

        Antenna roverAnt(
            antexReader.getAntenna(confReader().getValue("antennaModel", opts().SiteRover)));
        corrRover.setAntenna(roverAnt);
        corrRover.setUsePcv(confReader().getValueAsBoolean("usePCPatterns", opts().SiteRover));
        corrRover.setUseAzimuth(confReader().getValueAsBoolean("useAzim", opts().SiteRover));

#pragma endregion

        // Objects to compute tidal effects
        SolidTides solid;
        PoleTides pole;
        // Configure ocean loading model
        OceanLoading ocean;
        ocean.setFilename(opts().genericFilesDirectory + confReader().getValue("oceanLoadingFile"));
        const std::string sat_file =
            opts().genericFilesDirectory + confReader().getValue("satDataFile");

        ComputeWindUp windupBase(data_->navLibrary_, ref_base_pos, sat_file);
        ComputeWindUp windupRover(data_->navLibrary_, ref_base_pos, sat_file);

        ComputeSatPCenter svPcenterBase(ref_base_pos);
        svPcenterBase.setAntexReader(antexReader);

        ComputeSatPCenter svPcenterRover(ref_base_pos);
        svPcenterRover.setAntexReader(antexReader);

        ProcessLinear linearIonoFree;
        linearIonoFree.add(std::make_unique<PCCombination>());
        linearIonoFree.add(std::make_unique<LCCombimnation>());

        // Compute single differences opreator
        DeltaOp delta;
        UsedInPvtMarker useMarker;

        // configure single differences operator with appropriate measurements types
        TypeIDSet diffTypeSet;
        for (auto&& it : equations_->measTypes())
            diffTypeSet.insert(it);
        delta.setDiffTypeSet(diffTypeSet);

        KalmanSolver solver(equations_);
        solver.setMinSatNumber(5);
        KalmanSolverFB solverFb(equations_);
        solverFb.setMinSatNumber(5);
        if (forwardBackwardCycles_ > 0)
        {
            solverFb.setCyclesNumber(forwardBackwardCycles_);
            solverFb.setLimits(confReader().getValueListAsDouble("codeLimList"),
                               confReader().getValueListAsDouble("phaseLimList"));
        }

        bool firstTime = true;
        //
        for (auto& obsFile : data_->getObsFiles(opts().SiteRover))
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

            Antenna ant_rov(antexReader.getAntenna(roh.antType));
            corrRover.setAntenna(ant_rov);

            // read all epochs
            while (rin >> rin_epoch)
            {

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
                // keep only satellites from satellites systems selecyted for processing
                rin_epoch.keepOnlySatSystems(opts().systems);

                // keep only types used for processing
                //  rin_epoch.keepOnlyTypeID(requireObs_.getRequiredType());

                // compute approximate position
                if (firstTime)
                {
                    /* if (computeApprPos(rin_epoch, data_->SP3EphList, nominalPos_))
                         continue;*/
                    nominalPos_ = data_->getNominalPosition(opts().SiteRover);  
                    std::cout << "Baseline: " << std::setprecision(4)
                              << (nominalPos_ - ref_base_pos).mag() / 1000 << " km" << std::endl;
                    firstTime = false;
                }

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
                    printMsg(rin_epoch.getHeader().epoch,
                             "Rover receiver: all SV has been rejected.");
                    continue;
                }

                rin_epoch >> computeLinear_;
                rin_epoch >> markCSLI2Rover;
                rin_epoch >> markCSMW2Rover;
                rin_epoch >> snrCatcherL1Rover;
                rin_epoch >> markArcRover;

                auto eop = data_->eopStore.getEOP(MJD(t).mjd, IERSConvention::IERS2010);
                pole.setXY(eop.xp, eop.yp);

                try
                {
                    gRef >> sync;

                    Antenna ant_ref(antexReader.getAntenna(sync.getRefHeader().antType));
                    corrBase.setAntenna(ant_ref);

                    // keep only satellites from satellites systems selecyted for processing
                    gRef.keepOnlySatSystems(opts().systems);

                    gRef >> requireObs_;
                    gRef >> CodePhaseFilterBase;
                    gRef >> SNRFilterBase;

                    gRef >> computeLinear_;
                    gRef >> markCSLI2Base;
                    gRef >> markCSMW2Base;
                    gRef >> snrCatcherL1Base;
                    gRef >> markArcBase;

                    if (decimateData.check(gRef))
                        continue;

                    gRef >> modelRef;
                    // gRef >> eclipsedSV;
                    gRef >> grDelayBase;
                    gRef >> svPcenterBase;

                    Triple tides(solid.getSolidTide(t, ref_base_pos)
                                 + ocean.getOceanLoading(opts().SiteBase, t)
                                 + pole.getPoleTide(t, ref_base_pos));
                    corrBase.setExtraBiases(tides);

                    gRef >> corrBase;
                    gRef >> windupBase;
                    data_->ionoCorrector.setNominalPosition(ref_base_pos);

                    gRef >> computeTropoBase;
                    // gRef >> ionoFilterBase;
                    gRef >> linearIonoFree;
                    gRef >> oMinusC_;

                    delta.setRefData(gRef.getBody());
                }
                catch (SyncNextRoverEpoch& e)
                {
                    continue;
                }
                catch (SynchronizeException& e)
                {
                    break;
                }

                rin_epoch >> modelRover;
                // rin_epoch >> eclipsedSV;
                rin_epoch >> grDelayRover;
                rin_epoch >> svPcenterRover;

                Triple tides(solid.getSolidTide(t, nominalPos_)
                             + ocean.getOceanLoading(opts().SiteRover, t)
                             + pole.getPoleTide(t, nominalPos_));
                corrRover.setExtraBiases(tides);
                rin_epoch >> corrRover;

                rin_epoch >> windupRover;
                rin_epoch >> computeTropoRover;
                // rin_epoch >> ionoFilterRover;
                rin_epoch >> linearIonoFree;
                rin_epoch >> oMinusC_;
                rin_epoch >> delta;
                rin_epoch >> resCatcher;
                rin_epoch >> computeWeightSimple;

                rin_epoch >> minSatFilter;
                rin_epoch >> useMarker;

                DBOUT_LINE(">>" << CivilTime(rin_epoch.getHeader().epoch).asString());
                if (forwardBackwardCycles_ > 0)
                {
                    rin_epoch >> solverFb;
                }
                else
                {
                    rin_epoch >> solver;
                    auto ep = opts().fullOutput ? GnssEpoch(rin_epoch.getBody()) : GnssEpoch();
                    // updateNomPos(solverFB);
                    printSolution(solver, t, ep);
                    gMap_.data.insert(std::make_pair(t, ep));
                }
            }
        }
        if (forwardBackwardCycles_ > 0)
        {
            std::cout << "Fw-Bw part started" << std::endl;
            solverFb.reProcess();
            RinexEpoch rin_epoch;
            std::cout << "Last process part started" << std::endl;
            while (solverFb.lastProcess(rin_epoch))
            {
                auto ep = opts().fullOutput ? GnssEpoch(rin_epoch.getBody()) : GnssEpoch();
                // updateNomPos(solverFB);
                printSolution(solverFb, rin_epoch.getHeader().epoch, ep);
                gMap_.data.insert(std::make_pair(rin_epoch.getHeader().epoch, ep));
            }
            std::cout << "Measurments rejected: " << solverFb.rejectedMeasurements << std::endl;
        }

        // CatcherStatistic statB(opts().workingDir+"\\"+opts().SiteBase+".cst");
        // statB.logStatistic(plBase);
        // CatcherStatistic statR(opts().workingDir+"\\"+opts().SiteRover+".cst");
        // statR.logStatistic(plRov);
    }

    void PdFloatSolution::updateRequaredObs()
    {
        LinearCombinations comm;

        computeLinear_.add(std::make_unique<PDelta>());
        computeLinear_.add(std::make_unique<MWoubenna>());

        computeLinear_.add(std::make_unique<LDelta>());
        computeLinear_.add(std::make_unique<LICombimnation>());

        configureSolver();

        requireObs_ = RequireObservablesBuilder(opts().systems, opts().useC1).build();

        if (opts().carrierBands.find(CarrierBand::L1) != opts().carrierBands.end())
        {
            oMinusC_.add(std::make_unique<PrefitC1>(true));
            oMinusC_.add(std::make_unique<PrefitL1>());

            equations_->measTypes().insert(TypeID::prefitC);
            equations_->measTypes().insert(TypeID::prefitL1);

            equations_->residTypes().insert(TypeID::postfitC);
            equations_->residTypes().insert(TypeID::postfitL1);
        }
        if (opts().carrierBands.find(CarrierBand::L2) != opts().carrierBands.end())
        {
            oMinusC_.add(std::make_unique<PrefitC2>(true));
            oMinusC_.add(std::make_unique<PrefitL2>());

            equations_->measTypes().insert(TypeID::prefitP2);
            equations_->measTypes().insert(TypeID::prefitL2);
            equations_->residTypes().insert(TypeID::postfitP2);
            equations_->residTypes().insert(TypeID::postfitL2);
        }
    }

    void PdFloatSolution::configureSolver()
    {
        equations_->clearEquations();

        // tropo
        if (opts().computeTropo)
        {
            double qPrimeVert = confReader().getValueAsDouble("tropoQVertical");
            equations_->addEquation(std::make_unique<TropoEquations>(qPrimeVert));
        }

        //  Equations->addEquation(std::make_unique<IonoEquations>(confReader().getValueAsDouble("ionoQ")));
        // White noise stochastic models
        auto coord = std::make_unique<PositionEquations>();

        double posSigma = confReader().getValueAsDouble("posSigma");
        if (opts().dynamics == GnssDataStore::Dynamics::Static)
        {
            coord->setStochasicModel(std::make_shared<ConstantModel>());
        }
        else if (opts().dynamics == GnssDataStore::Dynamics::Kinematic)
        {
            coord->setStochasicModel(std::make_shared<WhiteNoiseModel>(posSigma));
        }
        else if (opts().dynamics == GnssDataStore::Dynamics::RandomWalk)
        {
            for (const auto& it : coord->getParameters())
                coord->setStochasicModel(it, std::make_shared<RandomWalkModel>(posSigma));
        }

        // add position equations
        equations_->addEquation(std::move(coord));

        equations_->addEquation(
            std::make_unique<ClockBiasEquations>(confReader().getValueAsDouble("clkSigma")));

        if (opts().systems.size() > 1)
            equations_->addEquation(std::make_unique<InterSystemBias>());

        if (opts().carrierBands.size() > 1)
            equations_->addEquation(std::make_unique<InterFrequencyBiases>());

        if (confReader().getValueAsBoolean("computeIono"))
        {
            auto ionoEq = std::make_unique<IonoEquations>();

            int ionoModelType = confReader().getValueAsInt("ionoModelType");

            if (ionoModelType == 0)
                ionoEq->setStocModel<WhiteNoiseModel>();
            else if (ionoModelType == 1)
                ionoEq->setStocModel<IonoStochasticModel>();

            double ionoSigma = confReader().getValueAsDouble("ionoSigma");
            ionoEq->setSigma(ionoSigma);
            equations_->addEquation(std::move(ionoEq));
        }

        if (opts().carrierBands.find(CarrierBand::L1) != opts().carrierBands.end())
            equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BL1));

        if (opts().carrierBands.find(CarrierBand::L2) != opts().carrierBands.end())
            equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BL2));

        forwardBackwardCycles_ = confReader().getValueAsInt("forwardBackwardCycles");
    }
} // namespace pod
