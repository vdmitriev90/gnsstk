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

      public:
        IonoEquations();
        IonoEquations(double qPrime);
        ~IonoEquations() {};

        /* update state of equations with new observational data */
        void prepare(gnsstk::IRinex& gData) override;

        /// Fill ONE row of the design matrix
        void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const override;

        virtual ParametersSet getParameters() const override
        {
            return currParameters;
        }

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;


        int getNumUnknowns() const override;

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
#pragma endregion
    };
} // namespace pod
