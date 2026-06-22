#include "PositionEquations.h"
#include "StateLayout.h"

using namespace gnsstk;

namespace pod
{
    // PositionEquations::PositionEquations()
    //     :types({ TypeID::dx,TypeID::dy,TypeID::dz })

    //{
    //    for (const auto &it : types)
    //        stochasticModels[it] = std::make_unique<WhiteNoiseModel>(100);
    //}
    PositionEquations::PositionEquations(double sigma)
        : types({FilterParameter(TypeID::dx),
                 FilterParameter(TypeID::dy),
                 FilterParameter(TypeID::dz)})
    {
        for (const auto& it : types)
            stochasticModels[it] = std::make_unique<WhiteNoiseModel>(sigma);
    }

    PositionEquations& PositionEquations::setStochasicModel(StochasticModel_sptr newModel)
    {
        for (auto& it : stochasticModels)
            it.second = newModel;
        return *this;
    }

    PositionEquations& PositionEquations::setStochasicModel(FilterParameter param,
                                                            StochasticModel_sptr newModel)
    {
        stochasticModels[param] = newModel;
        return *this;
    }

    void PositionEquations::prepare(gnsstk::IRinex& gData)
    {
        for (const auto& it : stochasticModels)
            it.second->Prepare(SatID::dummy, gData);
    }

    void PositionEquations::contributeDesignMatrix(const gnsstk::IRinex& gData,
                                    const gnsstk::TypeIDSet& obsTypes,
                                    gnsstk::Matrix<double>& H,
                                    const StateLayout& layout)
    {
        int row(0);

        for (auto&& obs : obsTypes)
            for (auto&& it : gData.getBody())
            {
                for (auto&& t : types)
                {
                    int col = layout.index(t);
                    H(row, col) = it.second->at(t.type);
                }
                row++;
            }
    }

    void PositionEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {

        for (const auto& it : types)
        {
            int col = layout.index(it);
            Phi(col, col) = stochasticModels.at(it)->getPhi();
        }
    }

    void PositionEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {

        for (const auto& it : types)
        {
            int col = layout.index(it);
            Q(col, col) = stochasticModels.at(it)->getQ();
        }
    }

    void PositionEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                  gnsstk::Matrix<double>& P,
                                                  const StateLayout& layout) const
    {
        for (const auto& it : types)
        {
            int col = layout.index(it);
            x(col) = 0;
            P(col, col) = 1e9;
        }
    }

    int PositionEquations::getNumUnknowns() const
    {
        return types.size();
    }
} // namespace pod
