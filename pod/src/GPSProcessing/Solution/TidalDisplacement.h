#pragma once

#include "EOPStore.hpp"
#include "MJD.hpp"
#include "OceanLoading.hpp"
#include "PoleTides.hpp"
#include "SolidTides.hpp"

#include <string>

namespace pod
{
    /// Aggregates tidal displacement models (solid Earth tides, pole tides,
    /// ocean loading) and returns the total site displacement [UEN], in meters.
    class TidalDisplacement
    {
    public:
        explicit TidalDisplacement(gnsstk::EOPStore& eopStore);

        /// Enable ocean loading using the given BLQ file.
        void setOceanLoadingFile(const std::string& fileName);

        void setUseSolid(bool use);
        void setUsePole(bool use);
        void setUseOcean(bool use);

        bool getUseSolid() const;
        bool getUsePole() const;
        bool getUseOcean() const;

        /// Total tidal displacement for station at position p.
        /// @param site station name, used by ocean loading model
        gnsstk::Triple getDisplacement(const gnsstk::CommonTime& t,
                                       const gnsstk::Position& p,
                                       const std::string& site);

    private:
        void updatePoleXY(const gnsstk::CommonTime& t);

        gnsstk::EOPStore& eopStore_;
        gnsstk::CommonTime lastEopTime_;
        bool hasEop_ = false;

        gnsstk::SolidTides solid_;
        gnsstk::PoleTides pole_;
        gnsstk::OceanLoading ocean_;

        bool useSolid_ = true;
        bool usePole_ = true;
        bool useOcean_ = true;
    };
}
