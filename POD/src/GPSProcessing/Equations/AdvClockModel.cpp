#include "AdvClockModel.h"

#include "StateLayout.h"

using namespace gnsstk;

namespace pod
{
    /** Set the value of previous epoch
         *
         * @param prevTime   Value of previous epoch
         *
         */
    AdvClockModel& AdvClockModel::setPreviousTime(const gnsstk::CommonTime& prevTime)
    {
        previousTime = prevTime;
        return (*this);
    }
    /** Set the value of current epoch
         *
         * @param currTime   Value of current epoch
         *
         */
    AdvClockModel& AdvClockModel::setCurrentTime(const gnsstk::CommonTime& currTime)
    {
        currentTime = currTime;
        return (*this);
    }
    AdvClockModel& AdvClockModel::setQ1(double q1_)
    {
        q1 = q1_;
        return (*this);
    }
    AdvClockModel& AdvClockModel::setQ2(double q2_)
    {
        q2 = q2_;
        return (*this);
    }
    void AdvClockModel::prepare(gnsstk::IRinex& gData)
    {
        // Update previous epoch
        setPreviousTime(currentTime);
        setCurrentTime(gData.getHeader().epoch);

        double d = std::abs(currentTime - previousTime);
        dt = isFirstTime || d < DBL_EPSILON ? 30.0 : d;
        isFirstTime = false;
    }

    ParametersSet AdvClockModel::getParameters() const
    {
        return types;
    }

    void AdvClockModel::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        FilterParameter pCdt(gnsstk::TypeID::recCdt);
        FilterParameter pDot(gnsstk::TypeID::recCdtdot);

        int i0 = layout.index(pCdt);
        int i1 = layout.index(pDot);

        Phi(i0, i0) = 1.0;
        Phi(i0, i1) = dt;
        Phi(i1, i0) = 0.0;
        Phi(i1, i1) = 1.0;
    }

    void AdvClockModel::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        FilterParameter pCdt(gnsstk::TypeID::recCdt);
        FilterParameter pDot(gnsstk::TypeID::recCdtdot);

        int i0 = layout.index(pCdt);
        int i1 = layout.index(pDot);

        double dt2 = dt * dt;
        double dt3 = dt2 * dt;

        Q(i0, i0) = q1 * dt + q2 * dt3 / 3.0;
        Q(i0, i1) = Q(i1, i0) = q2 * dt2 / 2.0;
        Q(i1, i1) = q2 * dt;
    }

    void AdvClockModel::defStateAndCovariance(gnsstk::Vector<double>& x,
                                              gnsstk::Matrix<double>& P,
                                              const StateLayout& layout) const
    {
        FilterParameter pCdt(gnsstk::TypeID::recCdt);
        FilterParameter pDot(gnsstk::TypeID::recCdtdot);

        int i0 = layout.index(pCdt);
        x(i0) = 0;
        P(i0, i0) = 1e9;

        int i1 = layout.index(pDot);
        x(i1) = 0;
        P(i1, i1) = 1e9;
    }

    void AdvClockModel::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        FilterParameter pCdt(gnsstk::TypeID::recCdt);
        FilterParameter pDot(gnsstk::TypeID::recCdtdot);

        int colCdt = layout.index(pCdt);
        int colDot = layout.index(pDot);

        H(ctx.row, colCdt) = 1.0;
        H(ctx.row, colDot) = dt;
    }

    int AdvClockModel::getNumUnknowns() const
    {
        return 2;
    }
} // namespace pod
