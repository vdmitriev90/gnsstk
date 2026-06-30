#include "IonoEquations.h"

#include "GnssObsMapping.h"
#include "StateLayout.h"

using namespace gnsstk;
namespace pod
{
    const double SQR_L1_WL_GPS = L1_WAVELENGTH_GPS * L1_WAVELENGTH_GPS;

#pragma region Stochasic model initializers

    gnsstk::StochasticModelUniquePtr IonoEquations::constantModel(double sigma)
    {
        return std::make_unique<ConstantModel>();
    }

    gnsstk::StochasticModelUniquePtr IonoEquations::rWalkModel(double qPrime)
    {
        return std::make_unique<RandomWalkModel>(qPrime);
    }

    gnsstk::StochasticModelUniquePtr IonoEquations::whiteNoiseModel(double sigma)
    {
        return std::make_unique<WhiteNoiseModel>(sigma);
    }

    gnsstk::StochasticModelUniquePtr IonoEquations::ionoModel(double sigma)
    {
        return std::make_unique<IonoStochasticModel>(sigma);
    }

#pragma endregion

    IonoEquations::IonoEquations()
        : eqType(TypeID::ionoL1)
        , sigma(300000.0)
        , stModelInitializer(&IonoEquations::constantModel) {};

    IonoEquations::IonoEquations(double qPrime)
        : eqType(TypeID::ionoL1)
        , sigma(qPrime)
        , stModelInitializer(&IonoEquations::constantModel) {};

    void IonoEquations::prepare(gnsstk::IRinex& gData)
    {
        currParameters.clear();
        auto&& currentSatSet = gData.getBody().getSatID();

        for (auto&& it : currentSatSet)
        {
            auto it_model = stochModels.find(it);
            if (it_model == stochModels.end())
            {
                auto&& newModel = (this->*stModelInitializer)(sigma);
                it_model = stochModels.insert(std::make_pair(it, std::move(newModel))).first;
            }

            it_model->second->Prepare(it, gData);
            currParameters.insert(FilterParameter(eqType, it));
        }
    }

    void IonoEquations::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        const auto attr = obs_mapping::findObsAttr(ctx.type.type);
        if (!attr.valid())
        {
            GNSSTK_ASSERT_MSG(false, "Unknown observation type in IonoEquations");
        }

        FilterParameter param(eqType, ctx.sat);
        auto it = currParameters.find(param);
        if (it == currParameters.end())
            return;

        const int col = layout.index(param);
        const int fcn = ctx.sat.getGloFcn();
        double wl = getWavelength(ctx.sat.system, attr.band, fcn);
        wl *= wl;

        H(ctx.row, col) = attr.sign * wl / SQR_L1_WL_GPS;
    }

    void IonoEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        for (const auto& it : currParameters)
        {
            int col = layout.index(it);
            Phi(col, col) = stochModels.at(it.sv)->getPhi();
        }
    }

    void IonoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        for (const auto& it : currParameters)
        {
            int col = layout.index(it);
            Q(col, col) = stochModels.at(it.sv)->getQ();
        }
    }

    void IonoEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                              gnsstk::Matrix<double>& P,
                                              const StateLayout& layout) const
    {
        for (const auto& it : currParameters)
        {
            int col = layout.index(it);
            x(col) = 0.0;
            P(col, col) = 1000;
        }
    }

    int IonoEquations::getNumUnknowns() const
    {
        return currParameters.size();
    }
} // namespace pod
