#include "TropoGradEquations.h"
using namespace gnsstk;
namespace pod
{
    TropoGradEquations::TropoGradEquations(double qPrimeZ, double qPrimeN, double qPrimeE)
        : types({FilterParameter(gnsstk::TypeID::wetMap),
                 FilterParameter(gnsstk::TypeID::wetMapNorth),
                 FilterParameter(gnsstk::TypeID::wetMapEast)})
        , pStochModelZ(std::make_unique<gnsstk::RandomWalkModel>(qPrimeZ))
        , pStochModelNorth(std::make_unique<gnsstk::RandomWalkModel>(qPrimeN))
        , pStochModelEast(std::make_unique<gnsstk::RandomWalkModel>(qPrimeE))
    {
    }

    void TropoGradEquations::Prepare(gnsstk::IRinex& gData)
    {
        pStochModelZ->Prepare(SatID::dummy, gData);
        pStochModelNorth->Prepare(SatID::dummy, gData);
        pStochModelEast->Prepare(SatID::dummy, gData);
    }

    void TropoGradEquations::updateH(const gnsstk::IRinex& gData,
                                     const gnsstk::TypeIDSet& obsTypes,
                                     gnsstk::Matrix<double>& H,
                                     int& col_0)
    {

        for (auto&& type : types)
        {
            int row(0);
            for (const auto& t : obsTypes)
            {
                for (const auto& it : gData.getBody())
                    H(row++, col_0) = it.second->get_value().at(type.type);
            }
            col_0++;
        }
    }

    void TropoGradEquations::updatePhi(gnsstk::Matrix<double>& Phi, int& index) const
    {
        Phi(index, index) = pStochModelZ->getPhi();
        ++index;
        Phi(index, index) = pStochModelNorth->getPhi();
        ++index;
        Phi(index, index) = pStochModelEast->getPhi();
        ++index;
    }

    void TropoGradEquations::updateQ(gnsstk::Matrix<double>& Q, int& index) const
    {
        Q(index, index) = pStochModelZ->getQ();
        ++index;
        Q(index, index) = pStochModelNorth->getQ();
        ++index;
        Q(index, index) = pStochModelEast->getQ();
        ++index;
    }

    void TropoGradEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                   gnsstk::Matrix<double>& P,
                                                   int& index) const
    {
        x(index) = 5.000e-02;
        P(index, index) = 5e-5;
        ++index;
        x(index) = 0.000e-03;
        P(index, index) = 1.667e-06;
        ++index;
        x(index) = 0.000e-03;
        P(index, index) = 1.667e-06;
        ++index;
    }

    int TropoGradEquations::getNumUnknowns() const
    {
        return types.size();
    }
} // namespace pod
