#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class TropoEquations : public EquationBase
    {
      public:
        TropoEquations()
            : type(gnsstk::TypeID::wetMap)
            , pStochasticModel(std::make_unique<gnsstk::RandomWalkModel>()) {};
        TropoEquations(double qPrime);
        virtual ~TropoEquations() {};

#pragma region Inherited via EquationBase

        virtual void Prepare(gnsstk::IRinex& gData) override;

        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& col_0) override;

        virtual ParametersSet getParameters() const override
        {
            return ParametersSet{type};
        }

        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual int getNumUnknowns() const override;

#pragma endregion

        TropoEquations& setModel(gnsstk::StochasticModel_uptr model)
        {
            pStochasticModel = std::move(model);
            return *this;
        }

        gnsstk::IStochasticModel* getModel() const
        {
            return pStochasticModel.get();
        }

#pragma region Fields

        gnsstk::StochasticModel_uptr pStochasticModel;

        FilterParameter type;

#pragma endregion
    };
} // namespace pod
