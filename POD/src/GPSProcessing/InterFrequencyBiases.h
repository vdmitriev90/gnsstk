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

        virtual void Prepare(gnsstk::IRinex& gData);

        void updateH(const gnsstk::IRinex& gData,
                     const gnsstk::TypeIDSet& types,
                     gnsstk::Matrix<double>& H,
                     int& col_0);

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

        virtual InterFrequencyBiases& setStochasicModel(const gnsstk::SatelliteSystem& system,
                                                        gnsstk::StochasticModel_uptr newModel);

      private:
        std::map<FilterParameter, gnsstk::StochasticModel_uptr> stochasticModels;

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
