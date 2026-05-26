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
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt),
                                  FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        AdvClockModel(double q1_, double q2_)
            : q1(q1_)
            , q2(q2_)
            , previousTime(gnsstk::CommonTime::BEGINNING_OF_TIME)
            , currentTime(gnsstk::CommonTime::END_OF_TIME)
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt),
                                  FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        AdvClockModel(double q1_,
                      double q2_,
                      const gnsstk::CommonTime& t1,
                      const gnsstk::CommonTime& t2)
            : q1(q1_)
            , q2(q2_)
            , previousTime(t1)
            , currentTime(t2)
            , types(ParametersSet{FilterParameter(gnsstk::TypeID::recCdt),
                                  FilterParameter(gnsstk::TypeID::recCdtdot)})
            , isFirstTime(true)
            , dt(DBL_MAX) {};

        virtual ~AdvClockModel() {};

        /** Set the value of previous epoch
         *
         * @param prevTime   Value of previous epoch
         *
         */
        virtual AdvClockModel& setPreviousTime(const gnsstk::CommonTime& prevTime)
        {
            previousTime = prevTime;
            return (*this);
        }

        /** Set the value of current epoch
         *
         * @param currTime   Value of current epoch
         *
         */
        virtual AdvClockModel& setCurrentTime(const gnsstk::CommonTime& currTime)
        {
            currentTime = currTime;
            return (*this);
        }

        virtual AdvClockModel& setQ1(double q1_)
        {
            q1 = q1_;
            return (*this);
        }

        virtual AdvClockModel& setQ2(double q2_)
        {
            q2 = q2_;
            return (*this);
        }

        // Inherited via EquationBase
        virtual void Prepare(gnsstk::IRinex& gData) override;

        virtual ParametersSet getParameters() const override;

        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const override;

        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& startColumn) override;

        virtual int getNumUnknowns() const override;

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
