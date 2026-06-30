#include "InterSystemBias.h"

#include "StateLayout.h"

using namespace gnsstk;

namespace pod
{

    constexpr std::array<gnsstk::TypeID::ValueType, InterSystemBias::NUM_BIAS> kBiasTypes = {
        {gnsstk::TypeID::recISB_GLN, gnsstk::TypeID::recISB_GAL, gnsstk::TypeID::recISB_BDS}};

    struct BiasEntry
    {
        SatelliteSystem system;
        TypeID::ValueType type;
    };

    inline int getBiasIndex(gnsstk::SatelliteSystem sys)
    {
        switch (sys)
        {
        case gnsstk::SatelliteSystem::Glonass:
            return 0;
        case gnsstk::SatelliteSystem::Galileo:
            return 1;
        case gnsstk::SatelliteSystem::BeiDou:
            return 2;
        default:
            return -1;
        }
    }

    constexpr std::array<gnsstk::TypeID::ValueType, 5> kL1ObsTypes = {TypeID::ValueType::prefitC,
                                                                      TypeID::ValueType::prefitC1,
                                                                      TypeID::ValueType::prefitL1,
                                                                      TypeID::ValueType::prefitPC,
                                                                      TypeID::ValueType::prefitLC};

    constexpr bool isL1ObsType(const gnsstk::TypeID& t)
    {
        for (auto v : kL1ObsTypes)
        {
            if (v == t.type)
                return true;
        }
        return false;
    }

    InterSystemBias::InterSystemBias()
    {
        for (auto& m : stochasticModels_)
            m = std::make_unique<ConstantModel>();
    }

    void InterSystemBias::prepare(IRinex& gData)
    {

        activeMask_.fill(false);

        for (const auto& it : gData.getBody())
        {
            int idx = getBiasIndex(it.first.system);

            if (idx >= 0)
            {
                activeMask_[idx] = true;
                stochasticModels_[idx]->Prepare(SatID::dummy, gData);
            }
        }

        activeCount_ = 0;
        for (bool b : activeMask_)
        {
            if (b)
                ++activeCount_;
        }
    }

    void InterSystemBias::fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const
    {
        if (!isL1ObsType(ctx.type))
            return;

        if (ctx.sat.system == SatelliteSystem::GPS)
            return;

        const int j = getBiasIndex(ctx.sat.system);
        if (j < 0 || !activeMask_[j])
            return;

        const int col = layout.index(FilterParameter(gnsstk::TypeID(kBiasTypes[j])));
        H(ctx.row, col) = 1.0;
    }

    ParametersSet InterSystemBias::getParameters() const
    {
        ParametersSet res;

        for (int i = 0; i < NUM_BIAS; ++i)
        {
            if (!activeMask_[i])
                continue;

            res.insert(FilterParameter(gnsstk::TypeID(kBiasTypes[i])));
        }

        return res;
    }

    InterSystemBias& InterSystemBias::setStochasicModel(const SatelliteSystem& system,
                                                        StochasticModelUniquePtr newModel)
    {
        const int idx = getBiasIndex(system);

        if (idx < 0)
        {
            GNSSTK_ASSERT_MSG(false, "Unsupported system in ISB");
            return *this;
        }

        stochasticModels_[idx] = std::move(newModel);
        return *this;
    }

    void InterSystemBias::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        for (int i = 0; i < NUM_BIAS; ++i)
        {
            if (!activeMask_[i])
                continue;
            int col = layout.index(FilterParameter(gnsstk::TypeID(kBiasTypes[i])));
            Phi(col, col) = stochasticModels_[i]->getPhi();
        }
    }

    void InterSystemBias::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        for (int i = 0; i < NUM_BIAS; ++i)
        {
            if (!activeMask_[i])
                continue;
            const int col = layout.index(FilterParameter(gnsstk::TypeID(kBiasTypes[i])));
            Q(col, col) = stochasticModels_[i]->getQ();
        }
    }

    int InterSystemBias::getNumUnknowns() const
    {
        return activeCount_;
    }

    void InterSystemBias::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                gnsstk::Matrix<double>& P,
                                                const StateLayout& layout) const
    {
        for (int i = 0; i < NUM_BIAS; ++i)
        {
            if (!activeMask_[i])
                continue;
            const int col = layout.index(FilterParameter(gnsstk::TypeID(kBiasTypes[i])));
            x(col) = 0;
            P(col, col) = 1e9;
        }
    }
} // namespace pod