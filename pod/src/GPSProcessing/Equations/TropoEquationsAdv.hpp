#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    class TropoEquationsAdv : public EquationBase
    {
      public:
        TropoEquationsAdv()
            : typeSet{FilterParameter(gnsstk::TypeID::wetMap), FilterParameter(gnsstk::TypeID::wetMapDot)}
            , previousTime(gnsstk::CommonTime::BEGINNING_OF_TIME)
            , currentTime(gnsstk::CommonTime::END_OF_TIME)
            , dt(DBL_MAX)
            , isFirstTime(true)
            , q1(0.0)
            , q2(0.0) {};

        TropoEquationsAdv(double q_1, double q_2)
            : typeSet{FilterParameter(gnsstk::TypeID::wetMap), FilterParameter(gnsstk::TypeID::wetMapDot)}
            , previousTime(gnsstk::CommonTime::BEGINNING_OF_TIME)
            , currentTime(gnsstk::CommonTime::END_OF_TIME)
            , dt(DBL_MAX)
            , isFirstTime(true)
            , q1(q_1)
            , q2(q_2) {};

        virtual ~TropoEquationsAdv() {};

#pragma region Inherited via EquationBase

        void prepare(gnsstk::IRinex& gData) override;

        void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const override;

        ParametersSet getParameters() const override
        {
            return typeSet;
        }

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override
        {
            return typeSet.size();
        }

#pragma endregion

        virtual TropoEquationsAdv& setPreviousTime(const gnsstk::CommonTime& prevTime)
        {
            previousTime = prevTime;
            return (*this);
        }

        /** Set the value of current epoch
         *
         * @param currTime   Value of current epoch
         *
         */
        virtual TropoEquationsAdv& setCurrentTime(const gnsstk::CommonTime& currTime)
        {
            currentTime = currTime;
            return (*this);
        }

#pragma region Fields

        // gnsstk::StochasticModelUniquePtr pStochasticModel_;

        ParametersSet typeSet;

        /// the diffusion coefficients, denoting the impact
        /// of white frequency noise
        double q1;

        /// the diffusion coefficients, denoting the impact
        /// of random walk frequency noise
        double q2;

        /// Epoch of previous measurement
        gnsstk::CommonTime previousTime;

        /// Epoch of current measurement
        gnsstk::CommonTime currentTime;

        double dt;

        gnsstk::TypeID typeId = gnsstk::TypeID::wetMap;

        mutable bool isFirstTime;

#pragma endregion
    };
} // namespace pod