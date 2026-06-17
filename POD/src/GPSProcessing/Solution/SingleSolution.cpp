#include "SingleSolution.h"

#include "BasicModel.hpp"
#include "ClockBiasEquations.h"
#include "ComputeTropModel.hpp"
#include "ComputeWeightSimple.h"
#include "Decimate.hpp"
#include "InterSystemBias.h"
#include "LICSDetector.hpp"
#include "LinearCombinations.hpp"
#include "MWCSDetector.hpp"
#include "NeillTropModel.hpp"
#include "PositionEquations.h"
#include "PowerSum.hpp"
#include "SimpleFilter.hpp"
#include "ObservablesSets.h"
#include "WinUtils.h"

#include <memory>

using namespace gnsstk;

namespace pod
{

    SingleSolution::SingleSolution(GnssDataStorePtr data_ptr)
        : GnssSolution(data_ptr, 50.0)
        , codeSmWindowSize_(600)
    {
    }

    //
    void SingleSolution::process()
    {
        updateRequaredObs();

        SimpleFilter PRFilter(data_->getGpsGloL1CodeType());
        if (data_->ionoCorrector.getType() == ComputeIonoModel::DualFreq)
            PRFilter.addFilteredType(TypeID::P2);

        SimpleFilter SNRFilter(TypeID::S1, 30, DBL_MAX);

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

        // basic model object
        BasicModel model(data_->navLibrary_);
        model.setDefaultObservable(data_->getGpsGloL1CodeType());
        model.setMinElev(confReader().getValueAsInt("ElMask"));

        // troposhere modeling object
        std::unique_ptr<NeillTropModel> uptrTropModel = std::make_unique<NeillTropModel>();
        ComputeTropModel computeTropo(*uptrTropModel);

        //
        ComputeWeightSimple w(2);
        KalmanSolver solver(equations_);

        KalmanSolverFB solverFb(equations_);

        if (forwardBackwardCycles_ > 0)
        {
            solverFb.setCyclesNumber(forwardBackwardCycles_);
            solverFb.setLimits(confReader().getValueListAsDouble("codeLimList"),
                               confReader().getValueListAsDouble("phaseLimList"));
        }

        bool firstTime = true;

        RinexEpoch rin_epoch;

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
            if (roh.interval > 0.0)
                codeSmoother_.setInterval(codeSmWindowSize_ / roh.interval);

            // read all epochs
            while (rin >> rin_epoch)
            {
                // work around for post header comments
                if (rin_epoch.getBody().size() == 0)
                {
                    printMsg(rin_epoch.getHeader().epoch, "Empty epoch record in Rinex file");
                    continue;
                }

                auto& t = rin_epoch.getHeader().epoch;

                // keep only satellites from satellites systems selected for processing
                rin_epoch.keepOnlySatSystems(opts().systems);

                // filter out satellites with incomplete observables set
                rin_epoch >> requireObs_;

                // compute approximate position

                if (apprPos().getPosition(rin_epoch, nominalPos_))
                    continue;

                if (firstTime)
                {
                    std::cout << std::setprecision(10) << nominalPos_ << std::endl;
                    firstTime = false;
                }

                // update approximate position
                data_->ionoCorrector.setNominalPosition(nominalPos_);
                uptrTropModel->setAllParameters(t, nominalPos_);
                model.setRxPosition(nominalPos_);


                rin_epoch >> PRFilter;
                rin_epoch >> SNRFilter;
                rin_epoch >> computeLinear_;

                // smooth pseudoranges if required
                if (opts().isSmoothCode)
                    rin_epoch >> codeSmoother_;

                if (rin_epoch.getBody().size() == 0)
                {
                    printMsg(rin_epoch.getHeader().epoch, "All SV has been rejected.");
                    continue;
                }

                if (decimateData.check(rin_epoch))
                    continue;

                rin_epoch >> model;
                rin_epoch >> computeTropo;
                rin_epoch >> data_->ionoCorrector;
                rin_epoch >> oMinusC_;
                rin_epoch >> w;
                const size_t min_sv_num = rin_epoch.getBody().getSatSystems().size() + 3;
                if (forwardBackwardCycles_ > 0)
                {
                    solverFb.setMinSatNumber(min_sv_num);
                    rin_epoch >> solverFb;
                }
                else
                {
                    solver.setMinSatNumber(min_sv_num);
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
            std::cout << "measurements rejected: " << solverFb.rejectedMeasurements << std::endl;
        }
    }

    void SingleSolution::updateNomPos(KalmanSolver& solver)
    {
        PowerSum psum;
        for (const auto& it : solver.PostfitResiduals())
            psum.add(it);
        double sigma = sqrt(psum.variance());

        int numSats = solver.PostfitResiduals().size();
        Position newPos;
        if (numSats >= 4 && sigma < getMaxSigma())
        {
            newPos[0] =
                nominalPos_.X() + solver.getSolution(FilterParameter(TypeID::dx)); // dx    - #4
            newPos[1] =
                nominalPos_.Y() + solver.getSolution(FilterParameter(TypeID::dy)); // dy    - #5
            newPos[2] =
                nominalPos_.Z() + solver.getSolution(FilterParameter(TypeID::dz)); // dz    - #6

            nominalPos_ = newPos;
        }
    }

    void SingleSolution::configureSolver()
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

        equations_->getResidTypes() = TypeIDSet{TypeID::postfitC};
        forwardBackwardCycles_ = confReader().getValueAsInt("forwardBackwardCycles");
    }

    void SingleSolution::updateRequaredObs()
    {
        configureSolver();

        if (opts().useC1)
        {
            oMinusC_.add(std::make_unique<PrefitC1>(false));
            equations_->getMeasTypes() = TypeIDSet{TypeID::prefitC};
        }
        else
        {
            equations_->getMeasTypes() = TypeIDSet{TypeID::prefitP1};
        }

        requireObs_ = RequireObservablesBuilder(opts().systems, opts().useC1).build();

        if (opts().isSmoothCode)
        {
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(data_->getGpsGloL1CodeType()));
            codeSmoother_.addSmoother(std::make_unique<CodeSmoother>(TypeID::P2));

            // add linear combinations, requared  for CS detections
            computeLinear_.add(std::make_unique<LICombimnation>());
            computeLinear_.add(std::make_unique<MWoubenna>());

            // define and add  CS markers
            codeSmoother_.addScMarker(std::make_unique<LICSDetector>());
            codeSmoother_.addScMarker(std::make_unique<MWCSDetector>());
        }
    }

} // namespace pod