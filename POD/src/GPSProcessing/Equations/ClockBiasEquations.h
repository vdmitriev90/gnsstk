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
        virtual void Prepare(gnsstk::IRinex& gData) override;
        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        virtual ParametersSet getParameters() const override
        {
            return ParametersSet({type});
        }
        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual int getNumUnknowns() const override;

      protected:
        FilterParameter type;
        gnsstk::StochasticModel_sptr stochModel;
    };
} // namespace pod
