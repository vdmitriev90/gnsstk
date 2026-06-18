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
#include "ObservablesSets.h"
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
        SimpleFilter CodeFilter(data_->getGpsGloL1CodeType());
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
        modelRef.setDefaultObservable(data_->getGpsGloL1CodeType());
        modelRef.setMinElev(confReader().getValueAsInt("ElMask"));

        // basic model object for rover has the same settings as BasicModel for ref. station
        BasicModel modelRover(modelRef);

        modelRef.setRxPosition(refPos);

        RinexEpoch epoch_rover, epoch_base;
        SyncObs sync(data_->getObsFiles(opts().SiteBase), epoch_rover);

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

        // troposhere modeling objects
        // for base
        NeillTropModel tropo_base;

        ComputeTropModel comp_tropo_base(tropo_base);

        // for rover
        NeillTropModel tropo_rov;
        ComputeTropModel compute_tropo_rover(tropo_rov);

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
            Rinex3ObsStream rnx_rover;

            // Open Rinex observations file in read-only mode
            rnx_rover.open(obsFile, std::ios::in);

            rnx_rover.exceptions(std::ios::failbit);
            Rinex3ObsHeader roh;

            // read the header
            rnx_rover >> roh;
            gMap_.header = roh;

            // update code smoothers sampling rate, according to current rinex file sampling rate
            codeSmoother_.setInterval(codeSmWindowSize_ / roh.interval);

            // read all epochs
            while (rnx_rover >> epoch_rover)
            {
                if (epoch_rover.getBody().size() == 0)
                {
                    printMsg(epoch_rover.getHeader().epoch, "Empty epoch record in Rinex file");
                    continue;
                }

                const auto& t = epoch_rover.getHeader().epoch;

                // keep only satellites from satellites systems selecyted for processing
                epoch_rover.keepOnlySatSystems(opts().systems);

                // compute approximate position

                if (apprPos().getPosition(epoch_rover, nominalPos_))
                    continue;

                if (firstTime)
                {
                    std::cout << "Baseline: " << std::setprecision(4)
                              << (nominalPos_ - refPos).mag() / 1000 << " km" << std::endl;
                    firstTime = false;
                }

                tropo_rov.setAllParameters(t, nominalPos_);
                tropo_base.setAllParameters(t, refPos);
                modelRover.setRxPosition(nominalPos_);

                epoch_rover >> requireObs_;
                epoch_rover >> CodeFilter;
                epoch_rover >> SNRFilter;
                epoch_rover >> computeLinear_;

                if (opts().isSmoothCode)
                    epoch_rover >> codeSmoother_;

                try
                {
                    epoch_base >> sync;
                    // keep only satellites from satellites systems selecyted for processing
                    epoch_base.keepOnlySatSystems(opts().systems);

                    epoch_base >> requireObs_;
                    epoch_base >> CodeFilter;
                    epoch_base >> SNRFilter;
                    epoch_base >> computeLinear_;

                    if (opts().isSmoothCode)
                    {
                        // update code smoother interval length
                        codeSmootherRef_.setInterval(codeSmWindowSize_
                                                    / sync.getRefHeader().interval);

                        // let's smooth the code
                        epoch_base >> codeSmootherRef_;
                    }

                    if (epoch_base.getBody().size() == 0)
                    {
                        printMsg(epoch_base.getHeader().epoch,
                                 "Reference receiver: all SV has been rejected.");
                        continue;
                    }

                    if (decimateData.check(epoch_base))
                        continue;

                    epoch_base >> modelRef;
                    epoch_base >> comp_tropo_base;

                    data_->ionoCorrector.setNominalPosition(refPos);
                    epoch_base >> data_->ionoCorrector;

                    epoch_base >> oMinusC_;

                    delta.setRefData(epoch_base.getBody());
                }
                catch (SynchronizeException& e)
                {
                    break;
                }

                epoch_rover >> modelRover;
                epoch_rover >> compute_tropo_rover;

                data_->ionoCorrector.setNominalPosition(nominalPos_);
                epoch_rover >> data_->ionoCorrector;

                epoch_rover >> oMinusC_;
                epoch_rover >> delta;
                epoch_rover >> w;

                if (forwardBackwardCycles_ > 0)
                {
                    solverFb.setMinSatNumber(3 + epoch_rover.getBody().getSatSystems().size());
                    epoch_rover >> solverFb;
                }
                else
                {
                    solver.setMinSatNumber(3 + epoch_rover.getBody().getSatSystems().size());
                    epoch_rover >> solver;
                    auto ep = opts().fullOutput ? GnssEpoch(epoch_rover.getBody()) : GnssEpoch();
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

        computeLinear_.setUseC1(opts().useC1);

        configureSolver();

        if (opts().useC1)
        {
            oMinusC_.add(std::make_unique<PrefitC1>(false));
            equations_->measTypes() = {TypeID::prefitC};
        }
        else
        {
            oMinusC_.add(std::make_unique<PrefitP1>(false));
            equations_->measTypes() = {TypeID::prefitP1};
        }

        requireObs_ = RequireObservablesBuilder(opts().systems, opts().useC1).build();

        if (opts().isSmoothCode)
        {
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(data_->getGpsGloL1CodeType()));
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(TypeID::P2));

            codeSmootherRef_.addSmoother(std::make_unique<CodeSmoother>(data_->getGpsGloL1CodeType()));
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
