#include "PppFloatSolution.h"

#include "AdvClockModel.h"
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
#include "ObservablesSets.h"
#include "OceanLoading.hpp"
#include "PoleTides.hpp"
#include "PositionEquations.h"
#include "PowerSum.hpp"
#include "PrefitResCatcher.h"
#include "SNRCatcher.h"
#include "SatArcMarker.hpp"
#include "SimpleFilter.hpp"
#include "SolidTides.hpp"
#include "StringUtils.h"
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
        model.setDefaultObservable(data_->getGpsGloL1CodeType());
        model.setMinElev(.0);

        ElevationMask elMask(opts().maskEl);

        SimpleFilter CodeFilter(TypeIDSet{data_->getGpsGloL1CodeType(), TypeID::P2, TypeID::L1, TypeID::L2});
        SimpleFilter SNRFilter(TypeID::S1, confReader().getValueAsInt("SNRmask"), DBL_MAX);
        // Object to remove eclipsed satellites
        EclipsedSatFilter eclipsedSV;

        RinexEpoch rin_epoch;

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

        // Object to compute gravitational delay effects
        GravitationalDelay grDelayRover;

        // for rover
        computeTropoRover_.setTropModel(tropoRover_);

#pragma region CS detectors

        // Objects to mark cycle slips
        // Checks LI cycle slips
        LICSDetector2 markCSLI2Rover;
        markCSLI2Rover.setSatThreshold(confReader().getValueAsDouble("LISatThreshold"));

        // Checks Merbourne-Wubbena cycle slips
        MWCSDetector markCSMW2Rover;
        markCSMW2Rover.setMaxNumLambdas(confReader().getValueAsDouble("MWNLambdas"));

        // check sharp SNR drops
        SNRCatcher snrCatcherL1Rover;
        PrefitResCatcher resCatcher(equations_->getMeasTypes());

        // Object to keep track of satellite arcs
        SatArcMarker markArcRover(TypeID::CSL1, true, 31.0);

#pragma endregion

#pragma region prepare ANTEX reader

        std::string antxfile = opts().genericFilesDirectory;
        antxfile += confReader().getValue("antexFile");

        AntexReader antexReader;
        antexReader.open(antxfile);

#pragma endregion

#pragma region correct observable

        CorrectObservables corrRover(data_->navLibrary_);

        // Vector from monument to antenna ARP [UEN], in meters
        const Triple offsetARP = confReader().getValueListAsTriple("offsetARP", opts().SiteRover);
        corrRover.setMonument(offsetARP);

        Antenna roverAnt(antexReader.getAntenna(confReader().getValue("antennaModel", opts().SiteRover)));
        corrRover.setUsePcv(confReader().getValueAsBoolean("usePCPatterns", opts().SiteRover));
        corrRover.setAntenna(roverAnt);
        corrRover.setUseAzimuth(confReader().getValueAsBoolean("useAzim", opts().SiteRover));

#pragma endregion

        ComputeWeightSimple computeWeightSimple(2);
        // Objects to compute tidal effects
        SolidTides solid;
        PoleTides pole;
        // Configure ocean loading model
        // OceanLoading ocean;
        // ocean.setFilename(opts().genericFilesDirectory + confReader().getValue("oceanLoadingFile"));
        const std::string sat_file = opts().genericFilesDirectory + confReader().getValue("satDataFile");

        ComputeWindUp windupRover(data_->navLibrary_, sat_file);

        ComputeSatPCenter svPcenterRover(sat_file);
        svPcenterRover.setAntexReader(antexReader);

        ProcessLinear linearIonoFree;
        linearIonoFree.add(std::make_unique<PCCombination>());
        linearIonoFree.add(std::make_unique<LCCombimnation>());

        UsedInPvtMarker useMarker;
        KalmanSolver solver(equations_);
        KalmanSolverFB solverFb(equations_);

        if (forwardBackwardCycles_ > 0)
        {
            solverFb.setCyclesNumber(forwardBackwardCycles_);
            solverFb.setLimits(confReader().getValueListAsDouble("codeLimList"),
                               confReader().getValueListAsDouble("phaseLimList"));
            solverFb.setCSDetRef(markCSLI2Rover, markCSMW2Rover);

            solverFb.ReProcList().push_back(markCSLI2Rover);
            solverFb.ReProcList().push_back(markCSMW2Rover);
            solverFb.ReProcList().push_back(markArcRover);
            solverFb.ReProcList().push_back(elMask);
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
                rin_epoch.keepOnlySatSystems(opts().systems);

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
                rin_epoch >> computeLinear_;

                auto eop = data_->eopStore.getEOP(MJD(t).mjd, IERSConvention::IERS2010);
                pole.setXY(eop.xp, eop.yp);

                rin_epoch >> model;
                rin_epoch >> eclipsedSV;
                rin_epoch >> grDelayRover;
                rin_epoch >> svPcenterRover;

                Triple tides(solid.getSolidTide(t, nominalPos_) /*+ ocean.getOceanLoading(opts().SiteRover, t)*/
                             + pole.getPoleTide(t, nominalPos_));
                corrRover.setExtraBiases(tides);
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
                    auto ep = opts().fullOutput ? GnssEpoch(rin_epoch.getBody()) : GnssEpoch();
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
                auto ep = opts().fullOutput ? GnssEpoch(rin_epoch.getBody()) : GnssEpoch();

                // update nominal position
                apprPos().getPosition(rin_epoch, nominalPos_);

                // fill GnssEpoch by filter state data
                printSolution(solverFb, rin_epoch.getHeader().epoch, ep);

                // add epoch to map
                gMap_.data.insert(std::make_pair(rin_epoch.getHeader().epoch, ep));
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
        requireObs_ = RequireObservablesBuilder(opts().systems, opts().useC1).build();

        oMinusC_.add(std::make_unique<PrefitPC>(true));
        oMinusC_.add(std::make_unique<PrefitLC>());

        equations_->getMeasTypes().insert(TypeID::prefitPC);
        equations_->getMeasTypes().insert(TypeID::prefitLC);
        equations_->getResidTypes().insert(TypeID::postfitPC);
        equations_->getResidTypes().insert(TypeID::postfitLC);
    }

    void PppFloatSolution::configureSolver()
    {
        equations_->clearEquations();

        double qPrimeVert = confReader().getValueAsDouble("tropoQ1");
        double qPrimeHor = confReader().getValueAsDouble("tropoQ2");

        if (opts().tropoModelType == TropoModelType::Simple)
        {
            equations_->addEquation(std::make_unique<TropoEquations>(qPrimeVert));
        }
        else if (opts().tropoModelType == TropoModelType::SimpleWithGradients)
        {
            equations_->addEquation(std::make_unique<TropoGradEquations>(qPrimeVert, qPrimeHor, qPrimeHor));
        }
        else if (opts().tropoModelType == TropoModelType::Advanced)
        {
            equations_->addEquation(std::make_unique<TropoEquationsAdv>(qPrimeVert, qPrimeHor));
        }

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

        if (confReader().getValueAsBoolean("useAdvClkModel"))
            equations_->addEquation(std::make_unique<AdvClockModel>(confReader().getValueAsDouble("q1Clk"),
                                                                    confReader().getValueAsDouble("q2Clk")));
        else
            equations_->addEquation(std::make_unique<ClockBiasEquations>());

        if (opts().systems.size() > 1)
            equations_->addEquation(std::make_unique<InterSystemBias>());

        equations_->addEquation(std::make_unique<AmbiguitiesEquations>(TypeID::BLC));

        forwardBackwardCycles_ = confReader().getValueAsInt("forwardBackwardCycles");
    }
} // namespace pod
