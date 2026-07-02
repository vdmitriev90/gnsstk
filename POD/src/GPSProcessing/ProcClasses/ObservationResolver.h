#pragma once

#include "CommonEnums.h"
#include "GnssObsMapping.h"
#include "SatID.hpp"
#include "TypeID.hpp"

namespace pod
{
    class ObservationResolver
    {
      public:
        ObservationResolver() = default;

        /// --- unified API ---

        /// Returns the raw observation TypeID (C1, P2, L1, L2, PC, LC, ...)
        [[nodiscard]] gnsstk::TypeID resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept;

        /// Returns the prefit residual TypeID (prefitC1, prefitP2, prefitL1, prefitPC, ...)
        [[nodiscard]] gnsstk::TypeID resolvePrefit(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept;

        [[nodiscard]] gnsstk::TypeID firstCode(gnsstk::SatelliteSystem ss) const noexcept
        {
            return resolve(ObsSlot::FirstBandCode, ss);
        }

        [[nodiscard]] gnsstk::TypeID secondCode(gnsstk::SatelliteSystem ss) const noexcept
        {
            return resolve(ObsSlot::SecondBandCode, ss);
        }

        [[nodiscard]] gnsstk::TypeID firstPhase(gnsstk::SatelliteSystem ss) const noexcept
        {
            return resolve(ObsSlot::FirstBandPhase, ss);
        }

        [[nodiscard]] gnsstk::TypeID secondPhase(gnsstk::SatelliteSystem ss) const noexcept
        {
            return resolve(ObsSlot::SecondBandPhase, ss);
        }

        [[nodiscard]] gnsstk::TypeID firstLLI(gnsstk::SatelliteSystem ss) const noexcept
        {
            return gnsstk::TypeID(obs_mapping::firstLLI(ss));
        }

        [[nodiscard]] gnsstk::TypeID secondLLI(gnsstk::SatelliteSystem ss) const noexcept
        {
            return gnsstk::TypeID(obs_mapping::secondLLI(ss));
        }

        /// Returns the cycle-slip flag TypeID (CSL1, CSL2, CSL5, ...) for the
        /// given observation slot and satellite system.
        /// Returns TypeID::Unknown for iono-free slots (no single CS flag).
        [[nodiscard]] gnsstk::TypeID resolveCsFlag(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept;

        /// optional helper
        [[nodiscard]] bool isSupported(ObsSlot slot, gnsstk::SatelliteSystem ss) const noexcept;
    };

} // namespace pod
