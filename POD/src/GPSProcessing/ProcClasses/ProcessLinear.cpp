#include "ProcessLinear.h"

#include "LinearCombination.h"

#include <map>

using namespace std;
namespace pod
{
    gnsstk::SatTypePtrMap& ProcessLinear::Process(gnsstk::SatTypePtrMap& gData)
    {
        for (auto& sv : gData)
        {
            for (const auto& lc : this->combs)
            {
                double value;
                if (lc->getCombination(sv.first, sv.second->get_value(), value))
                    sv.second->get_value().emplace(lc->getType(sv.first.system), value);
            }
        }
        return gData;
    }

    gnsstk::IRinex& ProcessLinear::Process(gnsstk::IRinex& gData)
    {
        Process(gData.getBody());
        return gData;
    }
} // namespace pod
