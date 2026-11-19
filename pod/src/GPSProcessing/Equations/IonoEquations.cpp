#include "IonoEquations.h"

#include "GnssObsMapping.h"
#include "StateLayout.h"
#include "Diagnostics.h"
#include "StringUtils.h"
#include <iostream>
#include <sstream>

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
        resetSats_.clear();
        auto&& currentSatSet = gData.getBody().getSatID();

        for (auto&& it : currentSatSet)
        {
            if (prevSats_.find(it) == prevSats_.end())
                resetSats_.insert(it);

            auto it_model = stochModels.find(it);
            if (it_model == stochModels.end())
            {
                auto&& newModel = (this->*stModelInitializer)(sigma);
                it_model = stochModels.insert(std::make_pair(it, std::move(newModel))).first;
            }

            it_model->second->Prepare(it, gData);
            currParameters.insert(FilterParameter(eqType, it));
        }
        DIAG_LOG_BLOCK(
        {
            if (!resetSats_.empty())
            {
                std::ostringstream oss;
                oss << "[DIAG] " << StringUtils::formatTime(gData.getHeader().epoch) << " IONO NEW/RETURNED:";
                for (const auto& s : resetSats_)
                    oss << " " << s;
                ::pod::diagLog() << oss.str() << std::endl;
            }
            for (const auto& s : prevSats_)
                if (currentSatSet.find(s) == currentSatSet.end())
                    ::pod::diagLog() << "[DIAG] " << StringUtils::formatTime(gData.getHeader().epoch)
                                     << " SAT LOST: " << s << std::endl;
        });
        prevSats_ = currentSatSet;
    }

    void IonoEquations::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        const auto attr = obs_mapping::findObsAttr(ctx.type.type);
        if (!attr.valid())
        {
            GNSSTK_ASSERT_MSG(false, "Unknown observation type in IonoEquations");
            return;
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
            Phi(col, col) = resetSats_.count(it.sv) ? 0.0 : stochModels.at(it.sv)->getPhi();
        }
    }

    void IonoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        for (const auto& it : currParameters)
        {
            int col = layout.index(it);
            Q(col, col) = resetSats_.count(it.sv) ? kIonoInitVariance : stochModels.at(it.sv)->getQ();
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
            P(col, col) = kIonoInitVariance;
        }
    }

    int IonoEquations::getNumUnknowns() const
    {
        return currParameters.size();
    }
} // namespace pod
