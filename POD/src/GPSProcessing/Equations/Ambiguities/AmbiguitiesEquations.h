#pragma once
#include "EquationBase.h"
#include "PhaseAmbiguityModel.hpp"

namespace pod
{
    class AmbiguitiesEquations : public EquationBase
    {

      public:
        AmbiguitiesEquations() =delete;

        AmbiguitiesEquations(const gnsstk::TypeID& ambType);

        virtual ~AmbiguitiesEquations() {};

        // Inherited via EquationBase
        virtual void prepare(gnsstk::IRinex& gData) override;

        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual int getNumUnknowns() const override;

        virtual ParametersSet getParameters() const override;

        virtual void contributeDesignMatrix(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        virtual ParametersSet getAmbSet() const override
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
