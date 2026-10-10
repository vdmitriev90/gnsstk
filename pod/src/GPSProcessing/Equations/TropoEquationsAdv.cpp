#include "TropoEquationsAdv.hpp"

#include "StateLayout.h"
#include "WinUtils.h"

using namespace gnsstk;

namespace pod
{

    void TropoEquationsAdv::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        FilterParameter pMap(gnsstk::TypeID::wetMap);
        FilterParameter pDot(gnsstk::TypeID::wetMapDot);

        int colMap = layout.index(pMap);
        int colDot = layout.index(pDot);

        const double mf = ctx.data->at(typeId);
        
        H(ctx.row, colMap) = mf;
        H(ctx.row, colDot) = mf * dt;
    }

    void TropoEquationsAdv::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        FilterParameter pMap(gnsstk::TypeID::wetMap);
        FilterParameter pDot(gnsstk::TypeID::wetMapDot);

        int i0 = layout.index(pMap);
        int i1 = layout.index(pDot);

        Phi(i0, i0) = 1.0;
        Phi(i0, i1) = dt;
        Phi(i1, i0) = 0.0;
        Phi(i1, i1) = 1.0;
    }

    void TropoEquationsAdv::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        FilterParameter pMap(gnsstk::TypeID::wetMap);
        FilterParameter pDot(gnsstk::TypeID::wetMapDot);

        int i0 = layout.index(pMap);
        int i1 = layout.index(pDot);

        double dt2 = dt * dt;
        // double dt3 = dt2 * dt;

        Q(i0, i0) = q1 * dt; //+ q2 * dt3 / 3.0;
        Q(i0, i1) = Q(i1, i0) = q2 * dt2 / 2.0;
        Q(i1, i1) = q2 * dt;
    }

    void TropoEquationsAdv::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                  gnsstk::Matrix<double>& P,
                                                  const StateLayout& layout) const
    {
        FilterParameter pMap(gnsstk::TypeID::wetMap);
        FilterParameter pDot(gnsstk::TypeID::wetMapDot);

        int i0 = layout.index(pMap);
        x(i0) = 5.000e-02;
        P(i0, i0) = 5e-5;

        int i1 = layout.index(pDot);
        x(i1) = 0;
        P(i1, i1) = 1.667e-06;
    }

    void TropoEquationsAdv::prepare(IRinex& gData)
    {
        // Update previous epoch
        setPreviousTime(currentTime);

        setCurrentTime(gData.getHeader().epoch);

        double d = std::abs(currentTime - previousTime);
        dt = isFirstTime || d < DBL_EPSILON ? 30.0 : d;
        isFirstTime = false;
    }
} // namespace pod
