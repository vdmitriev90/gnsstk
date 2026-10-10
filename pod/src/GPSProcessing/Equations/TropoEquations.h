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

        void prepare(gnsstk::IRinex& gData) override;

        void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const override;

        ParametersSet getParameters() const override
        {
            return ParametersSet{paramType_};
        }

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

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
