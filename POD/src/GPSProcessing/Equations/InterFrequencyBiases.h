#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class InterFrequencyBiases : public EquationBase
    {
      private:
        static std::map<gnsstk::SatelliteSystem, FilterParameter> ss2ifb;
        static std::map<FilterParameter, gnsstk::SatelliteSystem> ifb2ss;
        static const gnsstk::TypeIDSet l2Types;

      public:
        InterFrequencyBiases();
        virtual ~InterFrequencyBiases() {};

        virtual void prepare(gnsstk::IRinex& gData);

        void contributeDesignMatrix(const gnsstk::IRinex& gData,
                                    const gnsstk::TypeIDSet& types,
                                    gnsstk::Matrix<double>& H,
                                    const StateLayout& layout);

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

        virtual InterFrequencyBiases& setStochasicModel(const gnsstk::SatelliteSystem& system,
                                                        gnsstk::StochasticModelUniquePtr newModel);

      private:
        std::map<FilterParameter, gnsstk::StochasticModelUniquePtr> stochasticModels;

        // current set of satellite systems
        ParametersSet types;

        class Initilizer
        {
          public:
            Initilizer();
        };
        static Initilizer IfbSingleton;
    };
} // namespace pod
