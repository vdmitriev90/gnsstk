#include "PrefitResCatcher.h"
#include "ObservationResolver.h"

#include "WinUtils.h"

using namespace gnsstk;
namespace pod
{
    bool getDetection(const Vector<double> sample, int excl, double maxRatio, double& ratio)
    {
        size_t s = sample.size();
        double avg(0);
        for (size_t i = 0; i < s; i++)
            if (i != excl)
                avg += sample[i];
        avg /= s - 1;

        double resExcl = abs(avg - sample[excl]);
        double resAvg(0);

        for (size_t i = 0; i < s; i++)
            if (i != excl)
                resAvg += abs(avg - sample[i]);
        resAvg /= s - 1;

        ratio = resExcl / resAvg;

        return (ratio > maxRatio);
    }

    IRinex& PrefitResCatcher::Process(IRinex& gData)
    {
        auto& rejSatItem = rejectedSatsTable[gData.getHeader().epoch];

        const auto& slots = slots_;
        const ObservationResolver resolver;

        // Collect unique TypeIDs resolved for all slots × all systems in data
        TypeIDSet resolvedTypes;
        for (const auto& [sat, tvPtr] : gData.getBody())
        {
            for (const auto& slot : slots)
            {
                const TypeID tid = resolver.resolvePrefit(slot, sat.system);
                if (tid != TypeID::Unknown)
                    resolvedTypes.insert(tid);
            }
        }

        for (const auto& tid : resolvedTypes)
        {
            auto svs    = gData.getBody().getVectorOfSatID();
            auto values = gData.getBody().getVectorOfTypeID(tid);

            double ratio(0);
            for (size_t i = 0; i < values.size(); i++)
            {
                if (getDetection(values, i, 100, ratio))
                {
                    DBOUT_LINE(getClassName() << " " << svs[i] << " " << tid << " " << ratio)
                    gData.getBody().removeSatID(svs[i]);
                    rejSatItem.insert(svs[i]);
                    break;
                }
            }
        }
        return gData;
    }

    std::string PrefitResCatcher::getClassName() const
    {
        return "pod::PrefitResCatcher";
    }
} // namespace pod
