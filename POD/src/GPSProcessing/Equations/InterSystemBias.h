#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

#include <memory>

#include <array>

namespace pod
{

    /**This is a class to modify the GNSS data stucture: to
     * add ISB componenst of receiver clock offset
     *
     * The "InterSystemBias" object will visit every satellite in
     * the GNSS data structure and will try to add ISB model terms
     * to it.
     *
     * When used with the ">>" operator, this class returns the same
     * incoming data structure with the ISB data inserted along their
     * corresponding satellites. Be warned that if a GDS contains
     * less then two satellites of some satellite system its will
     * be summarily deleted from the data structure.
     */
    class InterSystemBias : public EquationBase
    {
      public:
        InterSystemBias();
        virtual ~InterSystemBias() {};

        virtual void prepare(gnsstk::IRinex& gData);

        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             const StateLayout& layout) override;

        virtual ParametersSet getParameters() const override;

        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const override;

        virtual int getNumUnknowns() const override;

        virtual InterSystemBias& setStochasicModel(const gnsstk::SatelliteSystem& system,
                                                   gnsstk::StochasticModelUniquePtr newModel);

        static constexpr int NUM_BIAS = 3;
      private:

        std::array<gnsstk::StochasticModelUniquePtr, NUM_BIAS> stochasticModels_;
        std::array<bool, NUM_BIAS> activeMask_{};
        int activeCount_{0};

    };
} // namespace pod
