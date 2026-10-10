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

        void prepare(gnsstk::IRinex& gData) override;

        void fillRow(const RowContext& ctx,
                     const StateLayout& layout,
                     gnsstk::Matrix<double>& H) const override;

        ParametersSet getParameters() const override
        {
            return types;
        }

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

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
