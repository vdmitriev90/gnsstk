#include "NumSatFilter.h"

using namespace gnsstk;

namespace pod
{

    IRinex& pod::NumSatFilter::Process(IRinex& gData)
    {
        auto svs = gData.getBody().getSatID();
        std::map<SatelliteSystem, int> counter;
        auto& rejTableItem = rejectedSatsTable[gData.getHeader().epoch];
        for_each(svs.begin(), svs.end(), [&counter](const SatID& sv) { counter[sv.system]++; });
        SatSystSet ssset;
        for (auto&& it : counter)
        {
            if (it.second >= getMinSAtNum())
                ssset.insert(it.first);
        }

        for (auto&& sv : svs)
            if (ssset.find(sv.system) == ssset.end())
                rejTableItem.insert(sv);

        gData.getBody().keepOnlySatSyst(ssset);
        return gData;
    }

    std::string pod::NumSatFilter::getClassName(void) const
    {
        return "pod::NumSatFilter";
    }
    int NumSatFilter::getMinSAtNum() const
    {
        return fixAmbig_ ? 2 : 1;
    }
} // namespace pod
