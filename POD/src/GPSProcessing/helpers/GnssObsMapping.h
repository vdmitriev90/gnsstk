#pragma once

#include "SatelliteSystem.hpp"
#include "TypeID.hpp"

#include <array>
#include <string>

namespace pod::obs_mapping
{
    using gnsstk::SatelliteSystem;
    using gnsstk::TypeID;

    //==============================================================
    // Residual type conversion (raw → prefit/postfit)
    //==============================================================

    enum class ObsFlavor
    {
        Prefit,
        Postfit
    };

    struct ObsConversionEntry
    {
        TypeID::ValueType src;
        TypeID::ValueType prefit;
        TypeID::ValueType postfit;
    };

    constexpr std::array<ObsConversionEntry, 16> obsConversionTable = {{
        {TypeID::C1, TypeID::prefitC1, TypeID::postfitC1},
        {TypeID::C2, TypeID::prefitC2, TypeID::postfitC2},
        {TypeID::C5, TypeID::prefitC5, TypeID::postfitC5},
        {TypeID::C6, TypeID::prefitC6, TypeID::postfitC6},
        {TypeID::C7, TypeID::prefitC7, TypeID::postfitC7},
        {TypeID::C8, TypeID::prefitC8, TypeID::postfitC8},

        {TypeID::P1, TypeID::prefitP1, TypeID::postfitP1},
        {TypeID::P2, TypeID::prefitP2, TypeID::postfitP2},

        {TypeID::PC, TypeID::prefitPC, TypeID::postfitPC},

        {TypeID::L1, TypeID::prefitL1, TypeID::postfitL1},
        {TypeID::L2, TypeID::prefitL2, TypeID::postfitL2},
        {TypeID::L5, TypeID::prefitL5, TypeID::postfitL5},
        {TypeID::L6, TypeID::prefitL6, TypeID::postfitL6},
        {TypeID::L7, TypeID::prefitL7, TypeID::postfitL7},
        {TypeID::L8, TypeID::prefitL8, TypeID::postfitL8},

        {TypeID::LC, TypeID::prefitLC, TypeID::postfitLC},
    }};

    constexpr TypeID::ValueType convertObs(const TypeID& in, ObsFlavor flavor)
    {
        for (const auto& e : obsConversionTable)
        {
            if (e.src == in.type)
            {
                return (flavor == ObsFlavor::Prefit) ? e.prefit : e.postfit;
            }
        }
        return TypeID::ValueType::Unknown;
    }

    constexpr TypeID::ValueType getPrefitObs(const TypeID& in)
    {
        return convertObs(in, ObsFlavor::Prefit);
    }

    constexpr TypeID::ValueType getPostfitObs(const TypeID& in)
    {
        return convertObs(in, ObsFlavor::Postfit);
    }

    /// prefit TypeID -> postfit TypeID
    constexpr TypeID::ValueType prefitToPostfit(const TypeID& prefit)
    {
        for (const auto& e : obsConversionTable)
        {
            if (e.prefit == prefit.type)
                return e.postfit;
        }
        return TypeID::ValueType::Unknown;
    }

    //==============================================================
    // Band mapping (RINEX3-like abstraction)
    //==============================================================

    struct BandEntry
    {
        SatelliteSystem sys;
        TypeID::ValueType type;
        int band;
    };

    constexpr std::array<BandEntry, 12> bandTable = {{
        {SatelliteSystem::GPS, TypeID::L1, 1},
        {SatelliteSystem::GPS, TypeID::L2, 2},
        {SatelliteSystem::GPS, TypeID::L5, 5},

        {SatelliteSystem::QZSS, TypeID::L1, 1},
        {SatelliteSystem::QZSS, TypeID::L2, 2},
        {SatelliteSystem::QZSS, TypeID::L5, 5},

        {SatelliteSystem::Galileo, TypeID::L1, 1},
        {SatelliteSystem::Galileo, TypeID::L5, 5},

        {SatelliteSystem::Glonass, TypeID::L1, 1},
        {SatelliteSystem::Glonass, TypeID::L2, 2},
    }};

    constexpr int getBand(SatelliteSystem ss, const TypeID& type)
    {
        for (const auto& e : bandTable)
        {
            if (e.sys == ss && e.type == type.type)
                return e.band;
        }
        return -1;
    }

    //==============================================================
    // Observation attributes (band + sign)
    //==============================================================

    struct ObsSignalTraits
    {
        TypeID::ValueType type;
        int band;
        int sign;

        bool valid() const
        {
            return type != TypeID::ValueType::Unknown;
        }
    };

    constexpr std::array<ObsSignalTraits, 7> obsAttrTable = {{
        {TypeID::prefitL1, 1, -1},
        {TypeID::prefitL2, 2, -1},
        {TypeID::prefitL5, 5, -1},
        {TypeID::prefitP1, 1, 1},
        {TypeID::prefitC, 1, 1},
        {TypeID::prefitC1, 1, 1},
        {TypeID::prefitP2, 2, 1},
    }};

    constexpr ObsSignalTraits findObsAttr(TypeID::ValueType t)
    {
        for (const auto& e : obsAttrTable)
        {
            if (e.type == t)
                return e;
        }
        return {TypeID::ValueType::Unknown, -1, 0};
    }

    //==============================================================
    // System observation configuration
    //==============================================================

    struct ObsConfig
    {
        SatelliteSystem sys;
        TypeID::ValueType firstCode;
        TypeID::ValueType secondCode;
        TypeID::ValueType firstPhase;
        TypeID::ValueType secondPhase;
    };

    constexpr std::array<ObsConfig, 6> obsConfigTable = {{
        {SatelliteSystem::GPS, TypeID::C1, TypeID::P2, TypeID::L1, TypeID::L2},
        {SatelliteSystem::Glonass, TypeID::C1, TypeID::P2, TypeID::L1, TypeID::L2},
        {SatelliteSystem::Galileo, TypeID::C1, TypeID::C5, TypeID::L1, TypeID::L5},
        {SatelliteSystem::QZSS, TypeID::C1, TypeID::C2, TypeID::L1, TypeID::L2},
        {SatelliteSystem::Geosync, TypeID::C1, TypeID::C2, TypeID::L1, TypeID::L5},
        {SatelliteSystem::BeiDou, TypeID::C1, TypeID::Unknown, TypeID::L1, TypeID::Unknown},
    }};

    constexpr const ObsConfig* findConfig(SatelliteSystem ss)
    {
        for (const auto& e : obsConfigTable)
        {
            if (e.sys == ss)
                return &e;
        }
        return nullptr;
    }

    //==============================================================
    // Runtime helper (assert + conversion)
    //==============================================================

    inline TypeID getOrAssert(TypeID::ValueType t, SatelliteSystem ss, const char* msg)
    {
        if (t == TypeID::ValueType::Unknown)
        {
            GNSSTK_ASSERT_MSG(false,
                              std::string("Unsupported \"") + convertSatelliteSystemToCode(ss)
                                  + "\" satellite system in " + msg);
            return TypeID::ValueType::Unknown;
        }

        return TypeID(t);
    }

    //==============================================================
    // LLI mapping
    //==============================================================

    constexpr TypeID::ValueType firstLLI(SatelliteSystem ss)
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
        case SatelliteSystem::Galileo:
        case SatelliteSystem::QZSS:
        case SatelliteSystem::Geosync:
        case SatelliteSystem::BeiDou:
            return TypeID::LLI1;
        default:
            return TypeID::Unknown;
        }
    }

    constexpr TypeID::ValueType secondLLI(SatelliteSystem ss)
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
        case SatelliteSystem::QZSS:
            return TypeID::LLI2;
        case SatelliteSystem::Galileo:
        case SatelliteSystem::Geosync:
            return TypeID::LLI5;
        default:
            return TypeID::Unknown;
        }
    }

} // namespace pod::obs_mapping