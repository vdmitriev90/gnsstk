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

        virtual void Prepare(gnsstk::IRinex& gData);

        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual int getNumUnknowns() const override;

      protected:
        std::map<FilterParameter, gnsstk::StochasticModel_sptr> stochasticModels;

        ParametersSet types;
    };
} // namespace pod
