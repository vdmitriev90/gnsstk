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

    void TropoEquations::contributeDesignMatrix(const gnsstk::IRinex& gData,
                                 const gnsstk::TypeIDSet& obsTypes,
                                 gnsstk::Matrix<double>& H,
                                 const StateLayout& layout)
    {
        int row(0);
        int col = layout.index(paramType_);
        for (const auto& t : obsTypes)
            for (const auto& it : gData.getBody())
                H(row++, col) = it.second->at(paramType_.type);
    }

    void TropoEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        int col = layout.index(paramType_);
        Phi(col, col) = pStochasticModel_->getPhi();
    }

    void TropoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        int col = layout.index(paramType_);
        Q(col, col) = pStochasticModel_->getQ();
    }

    void TropoEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                               gnsstk::Matrix<double>& P,
                                               const StateLayout& layout) const
    {
        int col = layout.index(paramType_);
        x(col) = 0.0;
        P(col, col) = 0.25;
    }

    int TropoEquations::getNumUnknowns() const
    {
        return 1;
    }
} // namespace pod
