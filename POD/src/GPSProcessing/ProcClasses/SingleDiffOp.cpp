#include "SingleDiffOp.h"

#include "RinexEpoch.h"

namespace pod
{
    gnsstk::SatTypePtrMap& SingleDifferenceOp::Process(gnsstk::SatTypePtrMap& gData)
    {
        const auto& slots = slots_;

        gnsstk::SatIDSet satRejectedSet;

        for (auto& [sat, tvPtr] : gData)
        {
            if (!tvPtr)
            {
                satRejectedSet.insert(sat);
                continue;
            }

            auto itRef = refData_.find(sat);
            if (itRef == refData_.end() || !itRef->second)
            {
                satRejectedSet.insert(sat);
                continue;
            }

            bool rejected = false;

            // Resolve slots -> TypeIDs per satellite system and difference them
            for (const auto& slot : slots)
            {
                const gnsstk::TypeID type = resolver_.resolvePrefit(slot, sat.system);
                if (type == gnsstk::TypeID::Unknown)
                    continue;

                auto it1 = tvPtr->find(type);
                auto it2 = itRef->second->find(type);

                if (it1 == tvPtr->end() || it2 == itRef->second->end())
                {
                    rejected = true;
                    break;
                }

                (*tvPtr)[type] = it1->second - it2->second;
            }

            if (rejected)
            {
                satRejectedSet.insert(sat);
                continue;
            }

            // OR cycle slip flags from both stations
            if (updateCSFlag_)
            {
                for (const auto slot : {ObsSlot::FirstBandPhase, ObsSlot::SecondBandPhase})
                {
                    const gnsstk::TypeID csType = resolver_.resolveCsFlag(slot, sat.system);
                    if (csType == gnsstk::TypeID::Unknown)
                        continue;

                    auto c1 = tvPtr->find(csType);
                    auto c2 = itRef->second->find(csType);

                    if (c1 != tvPtr->end() && c2 != itRef->second->end())
                        (*tvPtr)[csType] = (c1->second + c2->second > 0.0) ? 1.0 : 0.0;
                }
            }
        }

        if (deleteMissingSats_)
            gData.removeSatID(satRejectedSet);

        return gData;
    }

} // namespace pod
