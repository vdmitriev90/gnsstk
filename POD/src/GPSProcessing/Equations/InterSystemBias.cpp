#include "InterSystemBias.h"

using namespace gnsstk;

namespace pod
{
    std::map<SatelliteSystem, FilterParameter> InterSystemBias::SatSystemToBiasTypeId;
    std::map<FilterParameter, SatelliteSystem> InterSystemBias::BiasTypeIdToSatSystem;

    //
    const TypeIDSet InterSystemBias::kL1ObsTypes{TypeID::prefitC,
                                                 TypeID::prefitL1,
                                                 TypeID::prefitPC,
                                                 TypeID::prefitLC};

    InterSystemBias::Initilizer InterSystemBias::IsbSingleton;

    InterSystemBias::Initilizer::Initilizer()
    {
        SatSystemToBiasTypeId[SatelliteSystem::Glonass] = FilterParameter(TypeID::recISB_GLN);
        // SatSystemToBiasTypeId[SatelliteSystem::Galileo] = FilterParameter(TypeID::recISB_GAL);
        SatSystemToBiasTypeId[SatelliteSystem::BeiDou] = FilterParameter(TypeID::recISB_BDS);

        for (const auto& it : SatSystemToBiasTypeId)
            BiasTypeIdToSatSystem[it.second] = it.first;
    }

    InterSystemBias::InterSystemBias()
    {
        for (auto& it : BiasTypeIdToSatSystem)
            stochasticModels_[it.first] = std::make_unique<ConstantModel>();
    }

    void InterSystemBias::prepare(IRinex& gData)
    {
        // update current set of Satellite systems
        params_.clear();
        for (const auto& it : gData.getBody())
            if (it.first.system != SatelliteSystem::GPS)
                params_.insert(SatSystemToBiasTypeId[it.first.system]);

        for (const auto& ss : params_)
            stochasticModels_[ss]->Prepare(SatID::dummy, gData);
    }

    void InterSystemBias::contributeDesignMatrix(const gnsstk::IRinex& gData,
                                  const gnsstk::TypeIDSet& obsTypes,
                                  gnsstk::Matrix<double>& H,
                                  int& startColumn)
    {
        auto currentSatSet = gData.getBody().getSatID();
        int row(0);
        for (const auto& obs : obsTypes)
        {
            if (kL1ObsTypes.find(obs) == kL1ObsTypes.end())
            {
                row += currentSatSet.size();
                continue;
            }

            for (const auto& sv : currentSatSet)
            {
                if (sv.system != SatelliteSystem::GPS)
                {
                    auto it = params_.find(SatSystemToBiasTypeId[sv.system]);
                    int j = std::distance(params_.begin(), it);
                    H(row, startColumn + j) = 1;
                }
                row++;
            }
        }
        startColumn += params_.size();
    }

    InterSystemBias& InterSystemBias::setStochasicModel(const SatelliteSystem& system,
                                                        StochasticModelUniquePtr newModel)
    {
        stochasticModels_[SatSystemToBiasTypeId.at(system)] = std::move(newModel);
        return *this;
    }

    void InterSystemBias::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const
    {
        for (const auto& ss : params_)
        {
            Phi(index, index) = stochasticModels_.at(ss)->getPhi();
            ++index;
        }
    }

    void InterSystemBias::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const
    {
        for (const auto& ss : params_)
        {
            Q(index, index) = stochasticModels_.at(ss)->getQ();
            ++index;
        }
    }

    int InterSystemBias::getNumUnknowns() const
    {
        return params_.size();
    }

    void InterSystemBias::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                gnsstk::Matrix<double>& P,
                                                int& index) const
    {
        for (const auto& ss : params_)
        {
            x(index) = 0;
            P(index, index) = 1e9;
            ++index;
        }
    }
} // namespace pod