#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class TropoGradEquations : public EquationBase
    {
      public:
        TropoGradEquations()
            : types({FilterParameter(gnsstk::TypeID::wetMap),
                     FilterParameter(gnsstk::TypeID::wetMapNorth),
                     FilterParameter(gnsstk::TypeID::wetMapEast)})
            , pStochModelZ(std::make_unique<gnsstk::RandomWalkModel>())
            , pStochModelNorth(std::make_unique<gnsstk::RandomWalkModel>())
            , pStochModelEast(std::make_unique<gnsstk::RandomWalkModel>()) {};

        TropoGradEquations(double qPrimeZ, double qPrimeN, double qPrimeE);

        virtual ~TropoGradEquations() {};

#pragma region Inherited via EquationBase

        virtual void Prepare(gnsstk::IRinex& gData) override;

        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        virtual ParametersSet getParameters() const override
        {
            return types;
        }

        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual int getNumUnknowns() const override;

#pragma endregion

        TropoGradEquations& setModel(gnsstk::StochasticModelUniquePtr model)
        {
            pStochModelZ = std::move(model);
            return *this;
        }

        gnsstk::IStochasticModel* getModel() const
        {
            return pStochModelZ.get();
        }

#pragma region Fields

        gnsstk::StochasticModelUniquePtr pStochModelZ;
        gnsstk::StochasticModelUniquePtr pStochModelNorth;
        gnsstk::StochasticModelUniquePtr pStochModelEast;

        ParametersSet types;

#pragma endregion
    };
} // namespace pod
