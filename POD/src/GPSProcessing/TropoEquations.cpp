#include "TropoEquations.h"
using namespace gnsstk;

namespace pod
{
    TropoEquations::TropoEquations(double qPrime)
        :type( gnsstk::TypeID::wetMap ),
        pStochasticModel(std::make_unique<RandomWalkModel>(qPrime))
    {}

    void TropoEquations::Prepare(gnsstk::IRinex & gData)
    {
        
        pStochasticModel->Prepare(SatID::dummy, gData);
    }

    void TropoEquations::updateH(const gnsstk::IRinex& gData, const gnsstk::TypeIDSet & obsTypes, gnsstk::Matrix<double>& H, int & col_0)
    {
        int row(0);
        for (const auto& t : obsTypes)
            for (const auto& it : gData.getBody())
                H(row++, col_0) = it.second->get_value().at(type.type);
        col_0++;
    }
    

    void TropoEquations::updatePhi(gnsstk::Matrix<double>& Phi, int & index) const
    {
        Phi(index, index) = pStochasticModel->getPhi();
        ++index;
    }

    void TropoEquations::updateQ(gnsstk::Matrix<double>& Q, int & index) const
    {
        Q(index, index) = pStochasticModel->getQ();
        ++index;
    }

    void TropoEquations::defStateAndCovariance(gnsstk::Vector<double>& x, gnsstk::Matrix<double>& P, int & index) const
    {
        x(index) = 0.0;
        P(index, index) = 0.25;

        ++index;
    }

    int TropoEquations::getNumUnknowns() const
    {
        return 1;
    }
}
