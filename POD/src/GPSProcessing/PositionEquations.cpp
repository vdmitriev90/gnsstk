#include "PositionEquations.h"

using namespace gnsstk;

namespace pod
{
    //PositionEquations::PositionEquations()
    //    :types({ TypeID::dx,TypeID::dy,TypeID::dz })

    //{
    //    for (const auto &it : types)
    //        stochasticModels[it] = std::make_unique<WhiteNoiseModel>(100);
    //}
    PositionEquations::
        PositionEquations(double sigma)
        :types({ FilterParameter(TypeID::dx), FilterParameter(TypeID::dy), FilterParameter(TypeID::dz) })
    {
        for (const auto &it : types)
            stochasticModels[it] = std::make_unique<WhiteNoiseModel>(sigma);
    }
    
    PositionEquations& PositionEquations::
        setStochasicModel( StochasticModel_sptr newModel)
    {
        for(auto& it: stochasticModels)
            it.second= newModel;
        return *this;
    }

    PositionEquations& PositionEquations::
        setStochasicModel(FilterParameter param, StochasticModel_sptr newModel)
    {
        stochasticModels[param] = newModel;
        return *this;
    }

    void PositionEquations::
        Prepare(gnsstk::IRinex& gData)
    {
        for (const auto& it : stochasticModels)
            it.second->Prepare(SatID::dummy, gData);
    }

    void PositionEquations::
        updateH(const gnsstk::IRinex & gData, const gnsstk::TypeIDSet & obsTypes, gnsstk::Matrix<double>& H, int & col_0)
    {
        int row(0);

        for (auto&& obs : obsTypes)
            for (auto&& it : gData.getBody())
            {
                int j(0);
                for (auto&& t : types)
                    H(row, col_0 + j++) = it.second->get_value().at(t.type);
                row++;
            }
        col_0 += 3;
    }

    void PositionEquations::
        updatePhi(gnsstk::Matrix<double>& Phi, int& index) const
    {

        for (const auto &it : types)
        {
            Phi(index, index) = stochasticModels.at(it)->getPhi();
            ++index;
        }
    }

    void PositionEquations::
        updateQ(gnsstk::Matrix<double>& Q, int& index) const
    {

        for (const auto &it : types)
        {
            Q(index, index) = stochasticModels.at(it)->getQ();
            ++index;
        }
    }

    void PositionEquations::
        defStateAndCovariance(gnsstk::Vector<double>& x, gnsstk::Matrix<double>& P, int& index) const
    {
        for (const auto &it : types)
        {
            x(index) = 0;
            P(index, index) = 1e9;
            ++index;
        }
    }

    int PositionEquations::
        getNumUnknowns() const
    {
        return types.size();
    }
}
