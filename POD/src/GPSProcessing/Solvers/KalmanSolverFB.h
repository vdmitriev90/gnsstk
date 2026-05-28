#pragma once
#include "KalmanSolver.h"
#include "LICSDetector2.hpp"
#include "MWCSDetector.hpp"
#include "ProcessingList.hpp"
#include "RinexEpoch.h"
#include "SatArcMarker.hpp"
#include "UsedInPvtMarker.hpp"

namespace pod
{
    class KalmanSolverFB : public KalmanSolver
    {

      public:
        KalmanSolverFB();
        KalmanSolverFB(eqComposer_sptr eqs);

        virtual ~KalmanSolverFB();

        virtual std::string getClassName() const override
        {
            return "KalmanSolverFB";
        }

        virtual EquationComposer& eqComposer() override
        {
            return solver.eqComposer();
        }

        // Solution
        virtual const gnsstk::Vector<double>& Solution() const
        {
            return solver.Solution();
        }

        virtual gnsstk::Vector<double>& Solution()
        {
            return solver.Solution();
        }

        // Postfit-residuals.
        virtual const gnsstk::Vector<double>& PostfitResiduals() const override
        {
            return solver.PostfitResiduals();
        }

        // return postfit residuals vector
        virtual gnsstk::Vector<double>& PostfitResiduals() override
        {
            return solver.PostfitResiduals();
        }

        // return current varince - covarince matrix
        virtual const gnsstk::Matrix<double>& CovMatrix() const
        {
            return solver.CovMatrix();
        }

        // return current varince - covarince matrix
        virtual gnsstk::Matrix<double>& CovMatrix()
        {
            return solver.CovMatrix();
        }

        // return sqrt(vpv/(n-p)) value
        virtual double getSigma() const
        {
            return solver.getSigma();
        }

        // return sqrt(vpv/(n-p)) value
        virtual double getPhaseSigma() const
        {
            return solver.getPhaseSigma();
        }

        // return sqrt(vpv/(n-p)) value
        virtual double getCodeSigma() const
        {
            return solver.getCodeSigma();
        }

        // return minimum number of satellites requared for state esimation
        virtual double getMinSatNumber() const
        {
            return solver.getMinSatNumber();
        }

        // return current solver  status
        //  true - solution valid
        //  false - invalid
        virtual bool getValid() const
        {
            return solver.getValid();
        }

        virtual gnsstk::ProcessingList& ReProcList()
        {
            return reProcList;
        }

        virtual const gnsstk::ProcessingList& ReProcList() const
        {
            return reProcList;
        }

        virtual UsedInPvtMarker& UsedSvMarker()
        {
            return usedSvMarker;
        }

        virtual const UsedInPvtMarker& UsedSvMarker() const
        {
            return usedSvMarker;
        }

        // set minimum number of satellites requared for state esimation
        virtual KalmanSolverFB& setMinSatNumber(int value) override
        {
            solver.setMinSatNumber(value);
            return *this;
        }

        // get current value for given filter parameter
        double getSolution(const FilterParameter& type) const override
        {
            return solver.getSolution(type);
        }

        // get current varince value for given filter parameter
        double getVariance(const FilterParameter& type) const override
        {
            return solver.getVariance(type);
        }

        KalmanSolverFB& setLimits(const std::vector<double>& codeLims,
                                  const std::vector<double>& phaseLims);

        KalmanSolverFB& setCyclesNumber(size_t number)
        {
            cyclesNumber = number;
            return *this;
        }

        gnsstk::IRinex& Process(gnsstk::IRinex& rin_epoch);

        // last forward process cycle
        bool lastProcess(gnsstk::IRinex& rin_epoch);

        // Reprocess the data stored during a previous 'Process()' call.
        void reProcess(void);

        void setCSDetRef(gnsstk::LICSDetector2& li, gnsstk::MWCSDetector& mw)
        {
            LiCsDetector_ = &li;
            MwCsDetector_ = &mw;
        }

      private:
        gnsstk::IRinex& ReProcessOneEpoch(gnsstk::IRinex& rin_epoch);

        // This method checks the residuals and modifies 'gData' accordingly.
        void checkLimits(gnsstk::IRinex& gData, size_t cycleNumber);

        double getLimit(const gnsstk::TypeID& type, size_t cycleNumber);

#pragma region Fields

        struct limits
        {
            std::vector<double> codeLimits;
            std::vector<double> phaseLimits;
        } tresholds;

      public:
        // Number of processed measurements.
        int processedMeasurements;

        // Number of measurements rejected because they were off limits.
        int rejectedMeasurements;

      private:
        // observations data to be reprocessed
        std::list<gnsstk::irinex_uptr> ObsData;

        std::map<gnsstk::CommonTime, gnsstk::LICSDetector2> LIDetMap;
        std::map<gnsstk::CommonTime, gnsstk::MWCSDetector> MWDetMap;

        // internal kalman solver object, which do main part of real work
        KalmanSolver solver;

        // number of forward-backward cycles
        size_t cyclesNumber;

        // current  forward-backward cycle
        size_t currCycle;

        //
        gnsstk::ProcessingList reProcList;

        //
        UsedInPvtMarker usedSvMarker;

        gnsstk::LICSDetector2* LiCsDetector_;
        gnsstk::MWCSDetector* MwCsDetector_;

#pragma endregion
    };
} // namespace pod
