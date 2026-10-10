#include "ObservationResolver.h"

#include "GnssObsMapping.h"

namespace pod
{
    gnsstk::TypeID ObservationResolver::resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept
    {
        using namespace gnsstk;
        using namespace obs_mapping;

        switch (slot)
        {
        case ObsSlot::CodeIonoFree:  return TypeID::PC;
        case ObsSlot::PhaseIonoFree: return TypeID::LC;
        default: break;
        }

        const ObsConfig* cfg = findConfig(ss);
        if (!cfg) return TypeID::Unknown;

        switch (slot)
        {
        case ObsSlot::FirstBandCode:   return TypeID(cfg->firstCode);
        case ObsSlot::SecondBandCode:  return TypeID(cfg->secondCode);
        case ObsSlot::FirstBandPhase:  return TypeID(cfg->firstPhase);
        case ObsSlot::SecondBandPhase: return TypeID(cfg->secondPhase);
        default:                       return TypeID::Unknown;
        }
    }

    gnsstk::TypeID ObservationResolver::resolvePrefit(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept
    {
        using namespace gnsstk;
        using namespace obs_mapping;

        if (slot == ObsSlot::CodeIonoFree)  return TypeID::prefitPC;
        if (slot == ObsSlot::PhaseIonoFree) return TypeID::prefitLC;

        const ObsConfig* cfg = findConfig(ss);
        if (!cfg) return TypeID::Unknown;

        switch (slot)
        {
        case ObsSlot::FirstBandCode:   return TypeID(cfg->firstPrefitCode);
        case ObsSlot::SecondBandCode:  return TypeID(cfg->secondPrefitCode);
        case ObsSlot::FirstBandPhase:  return TypeID(cfg->firstPrefitPhase);
        case ObsSlot::SecondBandPhase: return TypeID(cfg->secondPrefitPhase);
        default:                       return TypeID::Unknown;
        }
    }

    gnsstk::TypeID ObservationResolver::resolveCsFlag(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept
    {
        using namespace gnsstk;
        using namespace obs_mapping;

        const ObsConfig* cfg = findConfig(ss);
        if (!cfg) return TypeID::Unknown;

        switch (slot)
        {
        case ObsSlot::FirstBandCode:
        case ObsSlot::FirstBandPhase:
            return TypeID(cfg->firstCsFlag);
        case ObsSlot::SecondBandCode:
        case ObsSlot::SecondBandPhase:
            return TypeID(cfg->secondCsFlag);
        default:
            return TypeID::Unknown;
        }
    }

    bool ObservationResolver::isSupported(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept
    {
        return resolve(slot, ss) != gnsstk::TypeID::Unknown;
    }
} // namespace pod