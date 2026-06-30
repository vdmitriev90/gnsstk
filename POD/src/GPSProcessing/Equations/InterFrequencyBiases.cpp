#include "InterFrequencyBiases.h"

#include "StateLayout.h"

using namespace gnsstk;

namespace pod
{
    std::map<SatelliteSystem, FilterParameter> InterFrequencyBiases::ss2ifb;
    std::map<FilterParameter, SatelliteSystem> InterFrequencyBiases::ifb2ss;

    const TypeIDSet InterFrequencyBiases::l2Types{TypeID::prefitP2, TypeID::prefitL2};

    InterFrequencyBiases::Initilizer InterFrequencyBiases::IfbSingleton;

    InterFrequencyBiases::Initilizer::Initilizer()
    {
        ss2ifb[SatelliteSystem::GPS] = FilterParameter(TypeID::recIFB_GPS_L2);
        ss2ifb[SatelliteSystem::Glonass] = FilterParameter(TypeID::recIFB_GLN_L2);
        ss2ifb[SatelliteSystem::Galileo] = FilterParameter(TypeID::recIFB_GAL_E5);
        ss2ifb[SatelliteSystem::BeiDou] = FilterParameter(TypeID::recIFB_BDS_B2);

        for (const auto& it : ss2ifb)
            ifb2ss.insert(std::make_pair(it.second, it.first));
    }

    InterFrequencyBiases::InterFrequencyBiases()
    {
        for (auto& it : ifb2ss)
            stochasticModels.insert(std::make_pair(it.first, std::make_unique<ConstantModel>()));
    }

    void InterFrequencyBiases::prepare(IRinex& gData)
    {
        types.clear();
        for (const auto& it : gData.getBody())
            types.insert(ss2ifb[it.first.system]);

        for (const auto& ss : types)
            stochasticModels[ss]->Prepare(SatID::dummy, gData);
    }

    void InterFrequencyBiases::fillRow(const RowContext& ctx,
                                                      const StateLayout& layout,
                                                      gnsstk::Matrix<double>& H) const
    {
        /*
		... | cdt(G2) | cdt(R2) |...| cdt(B2) |...
		*/
        if (l2Types.find(ctx.type) == l2Types.end())
            return;

        const auto& ifbParam = ss2ifb.find(ctx.sat.system);
        if (ifbParam == ss2ifb.end())
            return;

        const auto& it = types.find(ifbParam->second);
        if (it == types.end())
            return;

        int col = layout.index(*it);
        H(ctx.row, col) = 1.0;
    }

    InterFrequencyBiases& InterFrequencyBiases::setStochasicModel(const SatelliteSystem& system,
                                                                  StochasticModelUniquePtr newModel)
    {
        stochasticModels[ss2ifb.at(system)] = std::move(newModel);
        return *this;
    }

    void InterFrequencyBiases::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
    {
        for (const auto& ss : types)
        {
            int col = layout.index(ss);
            Phi(col, col) = stochasticModels.at(ss)->getPhi();
        }
    }

    void InterFrequencyBiases::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
    {
        for (const auto& ss : types)
        {
            int col = layout.index(ss);
            Q(col, col) = stochasticModels.at(ss)->getQ();
        }
    }

    int InterFrequencyBiases::getNumUnknowns() const
    {
        return types.size();
    }

    void InterFrequencyBiases::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                     gnsstk::Matrix<double>& P,
                                                     const StateLayout& layout) const
    {
        for (const auto& ss : types)
        {
            int col = layout.index(ss);
            x(col) = 0;
            P(col, col) = 1e9;
        }
    }
} // namespace pod
