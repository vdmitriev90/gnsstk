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

        virtual void prepare(gnsstk::IRinex& gData) override;

        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             const StateLayout& layout) override;

        virtual ParametersSet getParameters() const override
        {
            return types;
        }

        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const override;

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
