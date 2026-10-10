#include "ClockBiasEquations.h"

#include "StateLayout.h"

using namespace gnsstk;

namespace pod
{
    ClockBiasEquations::ClockBiasEquations() : type(TypeID::cdt), stochModel(std::make_unique<WhiteNoiseModel>()) {}

    ClockBiasEquations::ClockBiasEquations(double sigma)
        : type(TypeID::cdt)
        , stochModel(std::make_unique<WhiteNoiseModel>(sigma))
    {
    }

    ClockBiasEquations& ClockBiasEquations::setStochasicModel(StochasticModel_sptr newModel)
    {
        stochModel = newModel;
        return *this;
    }

    void ClockBiasEquations::prepare(IRinex& gData)
    {
        stochModel->Prepare(SatID::dummy, gData);
    }

    void ClockBiasEquations::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        const int col = layout.index(type);

        H(ctx.row, col) = 1.0;
    }

    void ClockBiasEquations::contributeTransitionMartix(Matrix<double>& Phi, const StateLayout& layout) const
    {
        int col = layout.index(type);
        Phi(col, col) = stochModel->getPhi();
    }

    void ClockBiasEquations::contributeProcessNoiseMatrix(Matrix<double>& Q, const StateLayout& layout) const
    {
        int col = layout.index(type);
        Q(col, col) = stochModel->getQ();
    }

    void ClockBiasEquations::defStateAndCovariance(Vector<double>& x,
                                                   Matrix<double>& P,
                                                   const StateLayout& layout) const
    {
        int col = layout.index(type);
        x(col) = 0;
        P(col, col) = 1e9;
    }

    int ClockBiasEquations::getNumUnknowns() const
    {
        return 1;
    }
} // namespace pod
