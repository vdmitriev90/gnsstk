#include "ObservationResolver.h"

#include "GnssObsMapping.h"

namespace pod
{
    gnsstk::TypeID ObservationResolver::resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const
    {
        using namespace gnsstk;

        switch (slot)
        {
        case ObsSlot::FirstBandCode:
        case ObsSlot::SecondBandCode:
        case ObsSlot::FirstBandPhase:
        case ObsSlot::SecondBandPhase:
            return resolveFromConfig(slot, ss);

        case ObsSlot::CodeIonoFree:
            return TypeID::PC;

        case ObsSlot::PhaseIonoFree:
            return TypeID::LC;

        default:
            return TypeID::Unknown;
        }
    }

    gnsstk::TypeID ObservationResolver::resolvePrefit(ObsSlot slot, gnsstk::SatelliteSystem ss) const
    {
        using namespace gnsstk;
        using namespace pod::obs_mapping;

        const TypeID raw = resolve(slot, ss);
        if (raw == TypeID::Unknown)
            return TypeID::Unknown;

        const TypeID::ValueType prefit = getPrefitObs(raw);
        if (prefit == TypeID::ValueType::Unknown)
            return TypeID::Unknown;

        return TypeID(prefit);
    }

    gnsstk::TypeID ObservationResolver::resolveFromConfig(ObsSlot slot, gnsstk::SatelliteSystem ss) const
    {
        using namespace gnsstk;
        using namespace pod::obs_mapping;

        const ObsConfig* cfg = findConfig(ss);
        if (!cfg)
            return TypeID::Unknown;

        switch (slot)
        {
        case ObsSlot::FirstBandCode:
            return TypeID(cfg->firstCode);

        case ObsSlot::SecondBandCode:
            return TypeID(cfg->secondCode);

        case ObsSlot::FirstBandPhase:
            return TypeID(cfg->firstPhase);

        case ObsSlot::SecondBandPhase:
            return TypeID(cfg->secondPhase);

        default:
            return TypeID::Unknown;
        }
    }

    gnsstk::TypeID ObservationResolver::firstCode(gnsstk::SatelliteSystem ss) const
    {
        return resolve(ObsSlot::FirstBandCode, ss);
    }

    gnsstk::TypeID ObservationResolver::secondCode(gnsstk::SatelliteSystem ss) const
    {
        return resolve(ObsSlot::SecondBandCode, ss);
    }

    gnsstk::TypeID ObservationResolver::firstPhase(gnsstk::SatelliteSystem ss) const
    {
        return resolve(ObsSlot::FirstBandPhase, ss);
    }

    gnsstk::TypeID ObservationResolver::secondPhase(gnsstk::SatelliteSystem ss) const
    {
        return resolve(ObsSlot::SecondBandPhase, ss);
    }

    bool ObservationResolver::isSupported(ObsSlot slot, gnsstk::SatelliteSystem ss) const
    {
        return resolve(slot, ss) != gnsstk::TypeID::Unknown;
    }
} // namespace pod