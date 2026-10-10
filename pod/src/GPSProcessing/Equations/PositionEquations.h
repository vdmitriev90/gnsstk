#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class PositionEquations : public EquationBase
    {
      public:
        // PositionEquations();
        PositionEquations(double posSigma = 100.0);
        virtual ~PositionEquations() {};

        virtual PositionEquations& setStochasicModel(gnsstk::StochasticModel_sptr newModel);

        virtual PositionEquations& setStochasicModel(FilterParameter,
                                                     gnsstk::StochasticModel_sptr newModel);

        ParametersSet getParameters() const override
        {
            return types;
        }

        virtual void prepare(gnsstk::IRinex& gData);

        void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const override;

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

      protected:
        std::map<FilterParameter, gnsstk::StochasticModel_sptr> stochasticModels;

        ParametersSet types;
    };
} // namespace pod
