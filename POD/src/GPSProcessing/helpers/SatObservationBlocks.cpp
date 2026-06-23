#include "SatObservationBlocks.h"

namespace pod
{
    void buildObservationBlocks(const gnsstk::IRinex& gData, const ObservationTypesProvider& provider, SatObservationBlocks& blocks)
    {
        using namespace pod::obs_mapping;

        blocks.clear();
        blocks.reserve(gData.getBody().size());

        for (const auto& [sat, tvPtr] : gData.getBody())
        {
            if (!tvPtr)
                continue;

            const auto& values = *tvPtr;

            SatObservationBlock block;
            block.sat = sat;
            block.data = tvPtr;

            const auto ss = sat.system;

            // raw measurement types
            const std::array<gnsstk::TypeID, 4> rawTypes = {provider.getFirstCodeType(ss),
                                                            provider.getSecondCodeType(ss),
                                                            provider.getFirstPhaseType(ss),
                                                            provider.getSecondPhaseType(ss)};

            for (const auto& raw : rawTypes)
            {
                if (raw == gnsstk::TypeID::Unknown)
                    continue;

                // raw -> prefit
                gnsstk::TypeID prefit(getPrefitObs(raw));

                if (prefit == gnsstk::TypeID::Unknown)
                    continue;

                auto it = values.find(prefit);
                if (it == values.end())
                    continue;

                Measurement m;
                m.type = prefit;
                m.value = it->second;
                m.weight = pod::weighting::weightOf(prefit.type);

                block.measurements.push_back(m);
            }

            if (!block.empty())
                blocks.push_back(std::move(block));
        }
    }
} // namespace pod
