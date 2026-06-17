#include "TropoEquations.h"
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
                                 int& startColumn)
    {
        int row(0);
        for (const auto& t : obsTypes)
            for (const auto& it : gData.getBody())
                H(row++, startColumn) = it.second->get_value().at(paramType_.type);
        startColumn++;
    }

    void TropoEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const
    {
        Phi(index, index) = pStochasticModel_->getPhi();
        ++index;
    }

    void TropoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const
    {
        Q(index, index) = pStochasticModel_->getQ();
        ++index;
    }

    void TropoEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                               gnsstk::Matrix<double>& P,
                                               int& index) const
    {
        x(index) = 0.0;
        P(index, index) = 0.25;

        ++index;
    }

    int TropoEquations::getNumUnknowns() const
    {
        return 1;
    }
} // namespace pod
