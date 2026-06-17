#pragma once
#include "EquationBase.h"
#include "PhaseAmbiguityModel.hpp"

namespace pod
{
    class AmbiguitiesEquations : public EquationBase
    {

      public:
        static const double sigma;

        AmbiguitiesEquations() : type(gnsstk::TypeID::BL1), obsType(typeMap[gnsstk::TypeID::BL1])
        {
            stochModel.setSigma(sigma);
        };

        AmbiguitiesEquations(const gnsstk::TypeID& obsType)
            : type(obsType)
            , obsType(typeMap[obsType])
        {
            stochModel.setSigma(sigma);
        };

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
        gnsstk::TypeID type;

        /// type ID of carrier phase measurements (or combination of measurements)
        gnsstk::TypeID obsType;

        /// salellites set to be preocessed
        gnsstk::SatIDSet satSet;

        /// current satellites set (subset of 'satSet')
        gnsstk::SatIDSet svsInView;

        /// cycle slip flags for satellites in 'satSet'
        std::map<gnsstk::SatID, bool> csFlags;

        /// phase ambiguity stochasic model
        mutable gnsstk::PhaseAmbiguityModel stochModel;

        static std::map<gnsstk::TypeID, gnsstk::TypeID> typeMap;

        class Initializer
        {
          public:
            Initializer();
        };
        static Initializer initializer;
    };
} // namespace pod
