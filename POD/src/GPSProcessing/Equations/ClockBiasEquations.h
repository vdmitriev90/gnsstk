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
        virtual void prepare(gnsstk::IRinex& gData) override;
        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             const StateLayout& layout) override;

        virtual ParametersSet getParameters() const override
        {
            return ParametersSet({type});
        }
        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const override;

        virtual int getNumUnknowns() const override;

      protected:
        FilterParameter type;
        gnsstk::StochasticModel_sptr stochModel;
    };
} // namespace pod
