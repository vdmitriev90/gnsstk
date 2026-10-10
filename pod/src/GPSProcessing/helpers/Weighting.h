#pragma once

#include "TypeID.hpp"

#include <cstdint>

namespace pod::weighting
{
    namespace details
    {

        enum class ObsKind
        {
            Code,
            Phase,
            Other
        };

        constexpr ObsKind classifyPrefit(gnsstk::TypeID::ValueType t)
        {
            using VT = gnsstk::TypeID::ValueType;

            switch (t)
            {
            // --- CODE ---
            case VT::prefitC:
            case VT::prefitC1:
            case VT::prefitP1:
            case VT::prefitC2:
            case VT::prefitP2:
            case VT::prefitC5:
            case VT::prefitC6:
            case VT::prefitC7:
            case VT::prefitC8:
            case VT::prefitPC:
                return ObsKind::Code;

            // --- PHASE ---
            case VT::prefitL:
            case VT::prefitL1:
            case VT::prefitL2:
            case VT::prefitL5:
            case VT::prefitL6:
            case VT::prefitL7:
            case VT::prefitL8:
            case VT::prefitLC:
            case VT::prefitWL:
            case VT::prefitWL2:
            case VT::prefitWL3:
            case VT::prefitWL4:
                return ObsKind::Phase;

            default:
                return ObsKind::Other;
            }
        }
    } // namespace details

    inline constexpr double weightOf(gnsstk::TypeID::ValueType t)
    {
        switch (details::classifyPrefit(t))
        {
        case details::ObsKind::Code:
            return 1.0;
        case details::ObsKind::Phase:
            return 10000.0;
        default:
            GNSSTK_ASSERT_MSG(false,
                              "Unknown prefit residual type: " + gnsstk::StringUtils::asString(t));
            return 1.0;
        }
    }
} // namespace pod::weighting
