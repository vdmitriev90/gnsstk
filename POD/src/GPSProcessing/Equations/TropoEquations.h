#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class TropoEquations : public EquationBase
    {
      public:
        TropoEquations();
        TropoEquations(double qPrime);

#pragma region Inherited via EquationBase

        virtual void prepare(gnsstk::IRinex& gData) override;

        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             const StateLayout& layout) override;

        virtual ParametersSet getParameters() const override
        {
            return ParametersSet{paramType_};
        }

        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const override;

        virtual int getNumUnknowns() const override;

#pragma endregion

        TropoEquations& setModel(gnsstk::StochasticModelUniquePtr model)
        {
            pStochasticModel_ = std::move(model);
            return *this;
        }

        gnsstk::IStochasticModel* getModel() const
        {
            return pStochasticModel_.get();
        }

#pragma region Fields

        gnsstk::StochasticModelUniquePtr pStochasticModel_;

        FilterParameter paramType_;

#pragma endregion
    };
} // namespace pod
