#include "CdDiffSolution.h"

#include "BasicModel.hpp"
#include "ClockBiasEquations.h"
#include "ComputeMOPSWeights.hpp"
#include "ComputeTropModel.hpp"
#include "ComputeWeightSimple.h"
#include "Decimate.hpp"
#include "DeltaOp.hpp"
#include "InterSystemBias.h"
#include "LICSDetector.hpp"
#include "LinearCombinations.hpp"
#include "MWCSDetector.hpp"
#include "NeillTropModel.hpp"
#include "OneFreqCSDetector.hpp"
#include "PositionEquations.h"
#include "PowerSum.hpp"
#include "SimpleFilter.hpp"
#include "SyncObs.h"
#include "WinUtils.h"

#include <memory>

using namespace gnsstk;

namespace pod
{

    CdDiffSolution::CdDiffSolution(GnssDataStorePtr data_ptr) : SingleSolution(data_ptr) {}

    CdDiffSolution::~CdDiffSolution() {}

    ///
    void CdDiffSolution::process()
    {
        updateRequaredObs();
        SimpleFilter CodeFilter(codeL1_);
        if (data_->ionoCorrector.getType() == ComputeIonoModel::DualFreq)
            CodeFilter.addFilteredType(TypeID::P2);

        SimpleFilter SNRFilter(TypeID::S1, confReader().getValueAsInt("SNRmask"), DBL_MAX);

        Triple pos;
        int i = 0;
        for (auto& it : confReader().getValueListAsDouble("nominalPosition", opts().SiteBase))
            pos[i++] = it;
        Position refPos(pos);

        // basic model object for ref. station
        BasicModel modelRef(data_->navLibrary_);
        modelRef.setDefaultObservable(codeL1_);
        modelRef.setMinElev(confReader().getValueAsInt("ElMask"));

        // basic model object for rover has the same settings as BasicModel for ref. station
        BasicModel modelRover(modelRef);

        modelRef.setRxPosition(refPos);

        RinexEpoch rin_epoch, gRef;
        SyncObs sync(data_->getObsFiles(opts().SiteBase), rin_epoch);

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

        // troposhere modeling objects
        // for base
        NeillTropModel tropoBase;

        ComputeTropModel computeTropoBase(tropoBase);

        // for rover
        NeillTropModel tropoRov;
        ComputeTropModel computeTropoRover(tropoRov);

        //
        // data_->ionoCorrector.setNominalPosition(refPos);

        //
        ComputeWeightSimple w;

        // Compute single differenceses opreator
        DeltaOp delta;

        KalmanSolver solver(equations_);
        KalmanSolverFB solverFb(equations_);
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

            // update code smoothers sampling rate, according to current rinex file sampling rate
            codeSmoother_.setInterval(codeSmWindowSize_ / roh.interval);

            // read all epochs
            while (rin >> rin_epoch)
            {
                if (rin_epoch.getBody().size() == 0)
                {
                    printMsg(rin_epoch.getHeader().epoch, "Empty epoch record in Rinex file");
                    continue;
                }

                const auto& t = rin_epoch.getHeader().epoch;

                // keep only satellites from satellites systems selecyted for processing
                rin_epoch.keepOnlySatSystems(opts().systems);

                // keep only types used for processing
                rin_epoch.keepOnlyTypeID(requireObs_.getRequiredType());

                // compute approximate position

                if (apprPos().getPosition(rin_epoch, nominalPos_))
                    continue;

                if (firstTime)
                {
                    std::cout << "Baseline: " << std::setprecision(4)
                              << (nominalPos_ - refPos).mag() / 1000 << " km" << std::endl;
                    firstTime = false;
                }

                tropoRov.setAllParameters(t, nominalPos_);
                tropoBase.setAllParameters(t, refPos);
                modelRover.setRxPosition(nominalPos_);

                rin_epoch >> requireObs_;
                rin_epoch >> CodeFilter;
                rin_epoch >> SNRFilter;
                rin_epoch >> computeLinear_;

                if (opts().isSmoothCode)
                    rin_epoch >> codeSmoother_;

                try
                {
                    gRef >> sync;
                    // keep only satellites from satellites systems selecyted for processing
                    gRef.keepOnlySatSystems(opts().systems);

                    // keep only types used for processing
                    gRef.keepOnlyTypeID(requireObs_.getRequiredType());

                    gRef >> requireObs_;
                    gRef >> CodeFilter;
                    gRef >> SNRFilter;
                    gRef >> computeLinear_;

                    if (opts().isSmoothCode)
                    {
                        // update code smoother interval length
                        codeSmootherRef_.setInterval(codeSmWindowSize_
                                                    / sync.getRefHeader().interval);

                        // let's smooth the code
                        gRef >> codeSmootherRef_;
                    }

                    if (gRef.getBody().size() == 0)
                    {
                        printMsg(gRef.getHeader().epoch,
                                 "Reference receiver: all SV has been rejected.");
                        continue;
                    }

                    if (decimateData.check(gRef))
                        continue;

                    gRef >> modelRef;
                    gRef >> computeTropoBase;

                    data_->ionoCorrector.setNominalPosition(refPos);
                    gRef >> data_->ionoCorrector;

                    gRef >> oMinusC_;

                    delta.setRefData(gRef.getBody());
                }
                catch (SynchronizeException& e)
                {
                    break;
                }

                rin_epoch >> modelRover;
                rin_epoch >> computeTropoRover;

                data_->ionoCorrector.setNominalPosition(nominalPos_);
                rin_epoch >> data_->ionoCorrector;

                rin_epoch >> oMinusC_;
                rin_epoch >> delta;
                rin_epoch >> w;

                if (forwardBackwardCycles_ > 0)
                {
                    solverFb.setMinSatNumber(3 + rin_epoch.getBody().getSatSystems().size());
                    rin_epoch >> solverFb;
                }
                else
                {
                    solver.setMinSatNumber(3 + rin_epoch.getBody().getSatSystems().size());
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
    }
    void CdDiffSolution::configureSolver()
    {
        equations_->clearEquations();
        // White noise stochastic models
        auto coord = std::make_unique<PositionEquations>();

        double sigma = confReader().getValueAsDouble("posSigma");
        if (opts().dynamics == GnssDataStore::Dynamics::Static)
        {
            coord->setStochasicModel(std::make_shared<ConstantModel>());
        }
        else if (opts().dynamics == GnssDataStore::Dynamics::Kinematic)
        {
            coord->setStochasicModel(std::make_shared<WhiteNoiseModel>(sigma));
        }
        else if (opts().dynamics == GnssDataStore::Dynamics::RandomWalk)
        {
            for (const auto& it : coord->getParameters())
            {
                coord->setStochasicModel(it, std::make_shared<RandomWalkModel>(sigma));
            }
        }

        // add position equations
        equations_->addEquation(std::move(coord));

        equations_->addEquation(std::make_unique<ClockBiasEquations>());

        if (opts().systems.size() > 1)
            equations_->addEquation(/*std::move(bias)*/ std::make_unique<InterSystemBias>());

        equations_->residTypes() = TypeIDSet{TypeID::postfitC};
        forwardBackwardCycles_ = confReader().getValueAsInt("forwardBackwardCycles");
    }

    ///
    void CdDiffSolution::updateRequaredObs()
    {
        LinearCombinations comm;
        bool useC1 = confReader().getValueAsBoolean("useC1");
        computeLinear_.setUseC1(useC1);

        configureSolver();

        if (useC1)
        {
            codeL1_ = TypeID::C1;
            oMinusC_.add(std::make_unique<PrefitC1>(false));
            equations_->measTypes() = {TypeID::prefitC};
        }
        else
        {
            codeL1_ = TypeID::P1;
            oMinusC_.add(std::make_unique<PrefitP1>(false));
            equations_->measTypes() = {TypeID::prefitP1};
        }

        requireObs_.addRequiredType(codeL1_);
        requireObs_.addRequiredType(TypeID::C1);
        requireObs_.addRequiredType(TypeID::P2);
        requireObs_.addRequiredType(TypeID::L1);
        requireObs_.addRequiredType(TypeID::L2);
        requireObs_.addRequiredType(TypeID::LLI1);
        requireObs_.addRequiredType(TypeID::LLI2);
        requireObs_.addRequiredType(TypeID::S1);

        if (opts().isSmoothCode)
        {
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(codeL1_));
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(TypeID::P2));

            codeSmootherRef_.addSmoother(std::make_unique<CodeSmoother>(codeL1_));
            codeSmootherRef_.addSmoother(std::make_unique<CodeSmoother>(TypeID::P2));

            // add linear combinations, requared  for CS detections
            computeLinear_.add(std::make_unique<LICombimnation>());
            computeLinear_.add(std::make_unique<MWoubenna>());

            // define and add  CS markers
            codeSmoother_.addScMarker(std::make_unique<LICSDetector>());
            codeSmoother_.addScMarker(std::make_unique<MWCSDetector>());

            codeSmootherRef_.addScMarker(std::make_unique<LICSDetector>());
            codeSmootherRef_.addScMarker(std::make_unique<MWCSDetector>());
        }
    }
} // namespace pod
