#pragma once
#include "EquationBase.h"
#include "PhaseAmbiguityModel.hpp"

namespace pod
{
    class AmbiguitiesEquations : public EquationBase
    {

      public:
        AmbiguitiesEquations() = delete;

        AmbiguitiesEquations(const gnsstk::TypeID& ambType);

        virtual ~AmbiguitiesEquations() {};

        // Inherited via EquationBase
        void prepare(gnsstk::IRinex& gData) override;

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

        ParametersSet getParameters() const override;

        void fillRow(const RowContext& ctx,
                     const StateLayout& layout,
                     gnsstk::Matrix<double>& H) const override;

        ParametersSet getAmbSet() const override
        {
            return getParameters();
        }

      private:
        /// type ID of unknown values
        gnsstk::TypeID ambType_;

        /// type ID of carrier phase measurements (or combination of measurements)
        gnsstk::TypeID obsType_;

        /// salellites set to be preocessed
        gnsstk::SatIDSet satSet_;

        /// current satellites set (subset of 'satSet')
        gnsstk::SatIDSet svsInView_;

        /// cycle slip flags for satellites in 'satSet'
        std::map<gnsstk::SatID, bool> csFlags_;

        /// phase ambiguity stochasic model
        mutable gnsstk::PhaseAmbiguityModel stochModel_;
    };
} // namespace pod
