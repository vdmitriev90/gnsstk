#pragma once
#include "EquationBase.h"
#include "IonoStochasticModel.h"
#include "StochasticModel.hpp"

#include <memory>
#include <type_traits>

namespace pod
{

    class IonoEquations : public EquationBase
    {
        typedef gnsstk::StochasticModelUniquePtr (IonoEquations::*StochModelInitialazer)(double qprime);
        static const double SQR_L1_WL_GPS;

      public:
        IonoEquations();
        IonoEquations(double qPrime);
        ~IonoEquations() {};

        /* update state of equations with new observational data */
        virtual void prepare(gnsstk::IRinex& gData) override;

        /*Check, if unknown parameters currently observable, if so,
        put the corresponding TypeID into 'TypeIDSet'
        */
        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        /* return set of TypeID, corresponding unknown parameters  for given equations */
        virtual ParametersSet getParameters() const override
        {
            return currParameters;
        }

        /* Put the values in state tarnsition matrix, starting with specific index,
        index will be incremented inside this method
        */
        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const override;

        /*Put process noise components into corresponding matrix,
        starting with specific index, index will be incremented inside this method
        */
        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const override;

        /* Put default values of state vector and it's covariance into corresponding matrices,
        starting with specific index, index will be incremented inside this method
        */
        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        /* return number of unknowns
         */
        virtual int getNumUnknowns() const override;

        template <class T,
                  typename = std::enable_if_t<std::is_base_of<gnsstk::IStochasticModel, T>::value>>
        IonoEquations& setStocModel()
        {
            if (std::is_same<T, gnsstk::ConstantModel>::value)
                stModelInitializer = &IonoEquations::constantModel;
            else if (std::is_same<T, gnsstk::RandomWalkModel>::value)
                stModelInitializer = &IonoEquations::rWalkModel;
            else if (std::is_same<T, gnsstk::WhiteNoiseModel>::value)
                stModelInitializer = &IonoEquations::whiteNoiseModel;
            else if (std::is_same<T, pod::IonoStochasticModel>::value)
                stModelInitializer = &IonoEquations::ionoModel;
            return *this;
        }

        IonoEquations& setSigma(double sigma)
        {
            this->sigma = sigma;
            return *this;
        }

      private:
        gnsstk::StochasticModelUniquePtr constantModel(double sigma);
        gnsstk::StochasticModelUniquePtr rWalkModel(double qPrime);
        gnsstk::StochasticModelUniquePtr whiteNoiseModel(double sigma);
        gnsstk::StochasticModelUniquePtr ionoModel(double sigma);

#pragma region Fields

        gnsstk::TypeID eqType;

        // current parameters - iono delay along lines of sight for each satellites
        ParametersSet currParameters;

        StochModelInitialazer stModelInitializer;

        std::map<gnsstk::SatID, gnsstk::StochasticModelUniquePtr> stochModels;

        double sigma;

        static std::map<gnsstk::TypeID, int> obsType2Band;
        static std::map<gnsstk::TypeID, int> obsType2Sign;

#pragma endregion
    };
} // namespace pod
