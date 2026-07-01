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
        gnsstk::TypeID resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const;

        /// Returns the prefit residual TypeID (prefitC1, prefitP2, prefitL1, prefitPC, ...)
        gnsstk::TypeID resolvePrefit(ObsSlot slot, gnsstk::SatelliteSystem ss) const;

        gnsstk::TypeID firstCode(gnsstk::SatelliteSystem ss) const;
        gnsstk::TypeID secondCode(gnsstk::SatelliteSystem ss) const;
        gnsstk::TypeID firstPhase(gnsstk::SatelliteSystem ss) const;
        gnsstk::TypeID secondPhase(gnsstk::SatelliteSystem ss) const;

        /// optional helper
        bool isSupported(ObsSlot slot, gnsstk::SatelliteSystem ss) const;

      private:
        gnsstk::TypeID resolveFromConfig(ObsSlot slot, gnsstk::SatelliteSystem ss) const;
    };

} // namespace pod
