#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class ClockBiasEquations : public EquationBase
    {
      public:
        ClockBiasEquations();
        ClockBiasEquations(double sigma);
        virtual ~ClockBiasEquations() {};

        virtual ClockBiasEquations& setStochasicModel(gnsstk::StochasticModel_sptr newModel);

        // Inherited via EquationBase
        void prepare(gnsstk::IRinex& gData) override;
        void fillRow(const RowContext& ctx,
                     const StateLayout& layout,
                     gnsstk::Matrix<double>& H) const override;

        ParametersSet getParameters() const override
        {
            return ParametersSet({type});
        }
        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

      protected:
        FilterParameter type;
        gnsstk::StochasticModel_sptr stochModel;
    };
} // namespace pod
