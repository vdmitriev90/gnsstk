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

        virtual ParametersSet getParameters() const override
        {
            return types;
        }

        virtual void prepare(gnsstk::IRinex& gData);

        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             const StateLayout& layout) override;

        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const override;

        virtual int getNumUnknowns() const override;

      protected:
        std::map<FilterParameter, gnsstk::StochasticModel_sptr> stochasticModels;

        ParametersSet types;
    };
} // namespace pod
