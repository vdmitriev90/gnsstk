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

        virtual double getCodeSigma() const;

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

        // check phase data integrity
        int checkPhase(gnsstk::IRinex& gData);

        // reject bad observation using residuals value
        virtual gnsstk::IRinex& reject(gnsstk::IRinex& gData, const gnsstk::TypeIDSet& typeOfResid);

        virtual void reset();

        gnsstk::CommonTime t_pre = gnsstk::CommonTime::BEGINNING_OF_TIME;

        bool firstTime;

        // Minimum satellites number required for state computation
        size_t minSatNumber;

        // State transition matrix
        gnsstk::Matrix<double> phiMatrix;

        // Process noise matrix
        gnsstk::Matrix<double> qMatrix;

        // Geometry matrix (derivative of observations wrt state)
        gnsstk::Matrix<double> hMatrix;

        // weights matrix
        gnsstk::Matrix<double> weightMatrix;

        // Measurements vector (prefit-residuals)
        gnsstk::Vector<double> measVector;

        // Weight unit error (sqrt(vpv/(n-p)))
        double sigma;

        double phaseSigma;

        double codeSigma;

        // object to prepare h, phi, q  matrices for filter
        EquationComposerPtr equations;

        // Indicator of current filter state validity
        bool isValid;

        // Indicate, if reset occurred on current filter step
        bool isReset;
    };
} // namespace pod
