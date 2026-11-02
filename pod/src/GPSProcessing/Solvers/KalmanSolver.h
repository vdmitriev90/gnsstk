#pragma once
#include "EquationComposer.h"
#include "SimpleKalmanFilter.hpp"
#include "SolverBase.hpp"

namespace pod
{
    class KalmanSolver : public gnsstk::SolverBase, public gnsstk::ProcessingClass
    {
      public:
        typedef std::map<gnsstk::CommonTime, EquationComposer::FilterState> FilterHistory;

      protected:
        // set of all possible TypeID for code pseudorange postfit residuals
        static const std::set<gnsstk::TypeID> codeResTypes;

        // set of all possible TypeID for  carrier phase postfit residuals
        static const std::set<gnsstk::TypeID> phaseResTypes;

      public:
        // union of code and phase postfit residual TypeIDs
        static const std::set<gnsstk::TypeID> allResTypes;

        // maximum time interval without data
        static double maxGap;

        KalmanSolver();

        KalmanSolver(EquationComposerPtr eqs);

        virtual ~KalmanSolver() = default;

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData);

        // Returns a string identifying this object.
        virtual std::string getClassName() const;

        virtual EquationComposer& eqComposer();

        // return sqrt(vpv/(n-p)) value
        virtual double getSigma() const;

        virtual double getPhaseSigma() const;

        // return minimum number of satellites requared for state esimation
        virtual double getMinSatNumber() const;

        // return isValid
        virtual bool getValid() const;

        // set minimum number of satellites requared for state esimation
        virtual KalmanSolver& setMinSatNumber(int value);

        // get solution
        virtual double getSolution(const FilterParameter& type) const;

        // get solution variance
        virtual double getVariance(const FilterParameter& type) const;

        virtual const EquationComposer::FilterState& getState() const;

        virtual KalmanSolver& setState(const EquationComposer::FilterState& newState);

        bool getResetState();

        bool ResetIfRequared(const gnsstk::CommonTime& t, const FilterHistory& data);

        // filter states, processed so far will be used in case of filer reset
        FilterHistory FilterData;

      protected:
        double getSigma(const gnsstk::TypeIDSet& types) const;

        int getUnknownIndex(const FilterParameter& parameter) const;

        // resolve carrier  phase ambiguities ot integer values
        virtual void fixAmbiguities(gnsstk::IRinex& gData);

        // measurement update: sets solution, covMatrix and postfitResiduals
        void update(const gnsstk::Vector<double>& xminus,
                    const gnsstk::Matrix<double>& Pminus,
                    const gnsstk::Matrix<double>& hMatrix,
                    const gnsstk::Matrix<double>& weightMatrix,
                    const gnsstk::Vector<double>& measVector);

        // check phase data integrity, re-initialize suspicious ambiguities in the a priori state
        int checkPhase(gnsstk::IRinex& gData,
                       gnsstk::Vector<double>& xminus,
                       gnsstk::Matrix<double>& Pminus,
                       const gnsstk::Matrix<double>& hMatrix,
                       const gnsstk::Matrix<double>& weightMatrix);

        // reject bad observation using residuals value
        virtual gnsstk::IRinex& reject(gnsstk::IRinex& gData, const gnsstk::TypeIDSet& typeOfResid);

        virtual void reset();

        gnsstk::CommonTime lastTime_ = gnsstk::CommonTime::BEGINNING_OF_TIME;

        // Minimum satellites number required for state computation
        size_t minSatNumber_;

        // Weight unit error (sqrt(vpv/(n-p)))
        double sigma_;

        double phaseSigma_;

        // object to prepare h, phi, q  matrices for filter
        EquationComposerPtr equations_;

        // Indicator of current filter state validity
        bool isValid_;

        // Indicate, if reset occurred on current filter step
        bool isReset_;
    };
} // namespace pod
