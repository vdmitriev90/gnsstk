#include "TropoEquations.h"
#include "StateLayout.h"
using namespace gnsstk;

namespace pod
{
    TropoEquations::TropoEquations()
        : paramType_(gnsstk::TypeID::wetMap)
        , pStochasticModel_(std::make_unique<gnsstk::RandomWalkModel>())
    {
    }

    TropoEquations::TropoEquations(double qPrime)
        : paramType_(gnsstk::TypeID::wetMap)
        , pStochasticModel_(std::make_unique<RandomWalkModel>(qPrime))
    {
    }

    void TropoEquations::prepare(gnsstk::IRinex& gData)
    {

        pStochasticModel_->Prepare(SatID::dummy, gData);
    }

    void TropoEquations::fillRow(const RowContext& ctx,
                                 const StateLayout& layout,
                                 gnsstk::Matrix<double>& H) const
    {
        const int col = layout.index(paramType_);
        H(ctx.row, col) = ctx.data->at(paramType_.type);
    }

    void TropoEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        const int col = layout.index(paramType_);
        Phi(col, col) = pStochasticModel_->getPhi();
    }

    void TropoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        const int col = layout.index(paramType_);
        Q(col, col) = pStochasticModel_->getQ();
    }

    void TropoEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                               gnsstk::Matrix<double>& P,
                                               const StateLayout& layout) const
    {
        const int col = layout.index(paramType_);
        x(col) = 0.0;
        P(col, col) = 0.25;
    }

    int TropoEquations::getNumUnknowns() const
    {
        return 1;
    }
} // namespace pod
