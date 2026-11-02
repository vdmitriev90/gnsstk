#include "ComputeWeightSimple.h"

#include <math.h>

using namespace gnsstk;

namespace pod
{
    const double ComputeWeightSimple::elev0 = 30.0;

    const double ComputeWeightSimple::sin0 = (std::sin(elev0 * DEG_TO_RAD));

    // const double ComputeWeightSimple::glnSigmaFactor = 2;

    ComputeWeightSimple::~ComputeWeightSimple() {}

    IRinex& ComputeWeightSimple::Process(IRinex& gData)
    {
        Process(gData.getBody());
        return gData;
    }
    SatTypePtrMap& ComputeWeightSimple::Process(SatTypePtrMap& gData)
    {
        SatIDSet rejSatSet;

        constexpr double referenceSNR = 45.0;

        // Минимальный вес, но не порог отбраковки.
        constexpr double minimumSNRWeight = 0.01;

        // 20.0 для мягкого взвешивания.
        // 10.0 для более агрессивного.
        constexpr double snrScale = 20.0;

        for (auto& it : gData)
        {
            TypeValueRow& tvMap = *it.second;

            const auto el = tvMap.find(TypeID::elevation);

            if (el == tvMap.end() || !std::isfinite(el->second) || el->second <= 0.0)
            {
                rejSatSet.insert(it.first);
                continue;
            }

            const auto snr = tvMap.find(TypeID::S1);

            if (snr == tvMap.end() || !std::isfinite(snr->second))
            {
                rejSatSet.insert(it.first);
                continue;
            }

            double elevationWeight = 1.0;

            if (el->second < elev0)
            {
                const double normalizedSin = std::sin(el->second * DEG_TO_RAD) / sin0;

                elevationWeight = normalizedSin * normalizedSin;
            }

            const double snrWeight =
                std::clamp(std::pow(10.0, (snr->second - referenceSNR) / snrScale), minimumSNRWeight, 1.0);

            const double systemFactor = (it.first.system == SatelliteSystem::Glonass) ? glnSigmaFactor : 1.0;

            tvMap[TypeID::weight] = elevationWeight * snrWeight / systemFactor;
        }

        // Удаляем спутники без elevation или SNR.
        gData.removeSatID(rejSatSet);

        return gData;
    }

} // namespace pod
