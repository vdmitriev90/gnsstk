#ifndef POD_ADV_CLOCK_MODEL_H
#define POD_ADV_CLOCK_MODEL_H

#include "EquationBase.h"
#include "StochasticModel.hpp"

namespace pod
{
    // linear clock model
    class AdvClockModel : public EquationBase
    {
      public:
        AdvClockModel()
            : q1(1e-29)
            , q2(1e-30)
            , previousTime(gnsstk::CommonTime::BEGINNING_OF_TIME)
            , currentTime(gnsstk::CommonTime::END_OF_TIME)
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt), FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        AdvClockModel(double q1_, double q2_)
            : q1(q1_)
            , q2(q2_)
            , previousTime(gnsstk::CommonTime::BEGINNING_OF_TIME)
            , currentTime(gnsstk::CommonTime::END_OF_TIME)
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt), FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        AdvClockModel(double q1_, double q2_, const gnsstk::CommonTime& t1, const gnsstk::CommonTime& t2)
            : q1(q1_)
            , q2(q2_)
            , previousTime(t1)
            , currentTime(t2)
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt), FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        virtual AdvClockModel& setPreviousTime(const gnsstk::CommonTime& prevTime);

        virtual AdvClockModel& setCurrentTime(const gnsstk::CommonTime& currTime);

        virtual AdvClockModel& setQ1(double q1_);

        virtual AdvClockModel& setQ2(double q2_);

        // Inherited via EquationBase
        void prepare(gnsstk::IRinex& gData) override;

        ParametersSet getParameters() const override;

        void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const override;

        void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const override;

        void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const override;

        void defStateAndCovariance(gnsstk::Vector<double>& x,
                                   gnsstk::Matrix<double>& P,
                                   const StateLayout& layout) const override;

        int getNumUnknowns() const override;

      private:
        ParametersSet types;

        double dt;

        /// the diffusion coefficients, denoting the impact
        ///  of white frequency noise
        double q1;

        /// the diffusion coefficients, denoting the impact
        ///  of random walk frequency noise
        double q2;

        /// Epoch of previous measurement
        gnsstk::CommonTime previousTime;

        /// Epoch of current measurement
        gnsstk::CommonTime currentTime;

        mutable bool isFirstTime;
    };
} // namespace pod
#endif // !POD_ADV_CLOCK_MODEL_H
