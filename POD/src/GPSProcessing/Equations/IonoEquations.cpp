#include "GnssObsMapping.h"

#include "IonoEquations.h"

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

    void IonoEquations::contributeDesignMatrix(const gnsstk::IRinex& gData,
                                const gnsstk::TypeIDSet& types,
                                gnsstk::Matrix<double>& H,
                                int& startColumn)
    {
        int nSv = currParameters.size();
        Matrix<double> mI = ident<double>(nSv);

        int row(0);
        for (const auto& it : types)
        {
            const auto attr = obs_mapping::findObsAttr(it.type);
                        if (!attr.valid())
            {
                GNSSTK_ASSERT_MSG(false, "Unknown observation type in IonoEquations");
                continue;
            }

            int i(0);
            for (const auto& sv : currParameters)
            {
                int fcn = sv.sv.getGloFcn();
                double wl = getWavelength(sv.sv.system, attr.band, fcn);
                wl *= wl;
                H(row, startColumn + i) = attr.sign * wl / SQR_L1_WL_GPS;
                row++;
                i++;
            }
        }
        startColumn += nSv;
    }

    void IonoEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const
    {
        for (const auto& it : currParameters)
        {
            Phi(index, index) = stochModels.at(it.sv)->getPhi();
            ++index;
        }
    }

    void IonoEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const
    {
        for (const auto& it : currParameters)
        {
            Q(index, index) = stochModels.at(it.sv)->getQ();
            ++index;
        }
    }

    void IonoEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                              gnsstk::Matrix<double>& P,
                                              int& index) const
    {
        for (const auto& it : currParameters)
        {
            x(index) = 0.0;
            P(index, index) = 1000;

            ++index;
        }
    }

    int IonoEquations::getNumUnknowns() const
    {
        return currParameters.size();
    }
} // namespace pod
