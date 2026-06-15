#include "LinearCombination.h"

#include "GNSSconstants.hpp"

namespace pod
{
    namespace details
    {
        TypeID::ValueType getPrefitObsTypeByObsType(const TypeID& originalType)
        {
            switch (originalType.type)
            {
            case TypeID::C1:
                return TypeID::prefitC1;
            case TypeID::C2:
                return TypeID::prefitC2;
            case TypeID::C5:
                return TypeID::prefitC5;
            case TypeID::C6:
                return TypeID::prefitC6;
            case TypeID::C7:
                return TypeID::prefitC7;
            case TypeID::C8:
                return TypeID::prefitC8;

            case TypeID::P1:
                return TypeID::prefitP1;
            case TypeID::P2:
                return TypeID::prefitP2;

            case TypeID::PC:
                return TypeID::prefitPC;

            case TypeID::L1:
                return TypeID::prefitL1;
            case TypeID::L2:
                return TypeID::prefitL2;
            case TypeID::L5:
                return TypeID::prefitL5;
            case TypeID::L6:
                return TypeID::prefitL6;
            case TypeID::L7:
                return TypeID::prefitL7;
            case TypeID::L8:
                return TypeID::prefitL8;

            case TypeID::LC:
                return TypeID::prefitLC;

            default:
                return TypeID::Unknown;
            }
        }
        TypeID::ValueType getPostfitObsTypeByObsType(const TypeID& originalType)
        {
            switch (originalType.type)
            {
            case TypeID::C1:
                return TypeID::postfitC1;
            case TypeID::C2:
                return TypeID::postfitC2;
            case TypeID::C5:
                return TypeID::postfitC5;
            case TypeID::C6:
                return TypeID::postfitC6;
            case TypeID::C7:
                return TypeID::postfitC7;
            case TypeID::C8:
                return TypeID::postfitC8;

            case TypeID::P1:
                return TypeID::postfitC1; // нет postfitP1
            case TypeID::P2:
                return TypeID::postfitC2; // нет postfitP2

            case TypeID::PC:
                return TypeID::postfitPC;

            case TypeID::L1:
                return TypeID::postfitL1;
            case TypeID::L2:
                return TypeID::postfitL2;
            case TypeID::L5:
                return TypeID::postfitL5;
            case TypeID::L6:
                return TypeID::postfitL6;
            case TypeID::L7:
                return TypeID::postfitL7;
            case TypeID::L8:
                return TypeID::postfitL8;

            case TypeID::LC:
                return TypeID::postfitLC;

            default:
                return TypeID::Unknown;
            }
        }

        int getRnx3BandIdBySystem(SatelliteSystem ss, TypeID typeId)
        {
            switch (ss)
            {
            case SatelliteSystem::GPS:
            case SatelliteSystem::QZSS:
                // Standard mapping for GPS/QZSS signals
                switch (typeId.type)
                {
                case TypeID::L1:
                    return 1;
                case TypeID::L2:
                    return 2;
                case TypeID::L5:
                    return 5;
                }
                break;

            case SatelliteSystem::Galileo:
                // Galileo uses different band naming (E1, E5a, etc.)
                switch (typeId.type)
                {
                case TypeID::L1:
                    return 1; // corresponds to E1
                case TypeID::L5:
                    return 5; // corresponds to E5a
                case TypeID::L2:
                    return -1; // no direct equivalent
                }
                break;

            case SatelliteSystem::Glonass:
                // GLONASS frequency bands (G1, G2, G3)
                switch (typeId.type)
                {
                case TypeID::L1:
                    return 1; // G1
                case TypeID::L2:
                    return 2; // G2
                case TypeID::L5:
                    return -1; // not used in standard mapping
                }
                break;

            case SatelliteSystem::BeiDou:
                // Approximate mapping for BeiDou signals
                switch (typeId.type)
                {
                case TypeID::L1:
                    return 1; // B1
                case TypeID::L2:
                    return 2; // simplified mapping (B1-2 / legacy)
                case TypeID::L5:
                    return 5; // B2a
                }
                break;

            case SatelliteSystem::IRNSS:
                return -1; // unsupported

            default:
                break;
            }

            // Return -1 if combination is unsupported or unknown
            return -1;
        }
    } // namespace details

    ObsTypesProviderPtr ObservationTypesProvider::instance()
    {
        static auto inst = std::make_shared<ObservationTypesProvider>();
        return inst;
    }

    TypeID ObservationTypesProvider::getFirstCodeType(SatelliteSystem ss) const
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
            return getGpsGloL1CodeType();
        case SatelliteSystem::Galileo:
        case SatelliteSystem::Geosync:
        case SatelliteSystem::BeiDou:
        case SatelliteSystem::QZSS:
            return TypeID::C1;
        case SatelliteSystem::IRNSS:
        case SatelliteSystem::LEO:
        case SatelliteSystem::Transit:
            GNSSTK_ASSERT_MSG(false,
                              "Unsupported \\" + convertSatelliteSystemToCode(ss)
                                  + "\\ satellite system in getFirstCodeType");
            return TypeID::Unknown;
        default:
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getFirstCodeType");
            return TypeID::Unknown;
        }
    }
    TypeID ObservationTypesProvider::getSecondCodeType(SatelliteSystem ss) const
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
            return TypeID::P2;
        case SatelliteSystem::Galileo:
            return TypeID::C5;
        case SatelliteSystem::QZSS:
        case SatelliteSystem::Geosync:
            return TypeID::C2;
        case SatelliteSystem::BeiDou:
        case SatelliteSystem::IRNSS:
        case SatelliteSystem::LEO:
        case SatelliteSystem::Transit:
            GNSSTK_ASSERT_MSG(false,
                              "Unsupported \\" + convertSatelliteSystemToCode(ss)
                                  + "\\ satellite system in getSecondCodeType");
            return TypeID::Unknown;
        default:
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getSecondCodeType");
            return TypeID::Unknown;
        }
    }
    TypeID ObservationTypesProvider::getFirstPhaseType(SatelliteSystem ss) const
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
        case SatelliteSystem::Galileo:
        case SatelliteSystem::Geosync:
        case SatelliteSystem::BeiDou:
        case SatelliteSystem::QZSS:
            return TypeID::L1;
        case SatelliteSystem::IRNSS:
        case SatelliteSystem::LEO:
        case SatelliteSystem::Transit:
            GNSSTK_ASSERT_MSG(false,
                              "Unsupported \\" + convertSatelliteSystemToCode(ss)
                                  + "\\ satellite system in getFirstPhaseType");
            return TypeID::Unknown;
        default:
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getFirstPhaseType");
            return TypeID::Unknown;
        }
    }
    TypeID ObservationTypesProvider::getSecondPhaseType(SatelliteSystem ss) const
    {
        switch (ss)
        {
        case SatelliteSystem::GPS:
        case SatelliteSystem::Glonass:
        case SatelliteSystem::QZSS:
            return TypeID::L2;
        case SatelliteSystem::Galileo:
            return TypeID::L5;
        case SatelliteSystem::Geosync:
        case SatelliteSystem::BeiDou:
        case SatelliteSystem::IRNSS:
        case SatelliteSystem::LEO:
        case SatelliteSystem::Transit:
            GNSSTK_ASSERT_MSG(false,
                              "Unsupported \\" + convertSatelliteSystemToCode(ss)
                                  + "\\ satellite system in getSecondPhaseType");
            return TypeID::Unknown;
        default:
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getSecondPhaseType");
            return TypeID::Unknown;
        }
    }

    void ObservationTypesProvider::setGpsGloL1CodeType(bool useCaForGpsGlo)
    {
        useCaForGpsGlo_ = useCaForGpsGlo;
    }

    inline TypeID ObservationTypesProvider::getGpsGloL1CodeType() const
    {
        return useCaForGpsGlo_ ? TypeID::C1 : TypeID::P1;
    }

    /* Iono-Free wavelength according to equation 20.47 (pg. 591) in
       "Peter J.G. Teunissen, Oliver Montenbruck (Eds.)
       Springer Handbook of Global Navigation Satellite Systems"
     */

    double LinearCombination::getIonoFreeWaveLength(const gnsstk::SatID& sv, int band1, int band2)
    {
        int fcn = sv.getGloFcn();
        double wlL1 = getWavelength(sv.system, band1, fcn);
        double wlL2 = getWavelength(sv.system, band2, fcn);

        return wlL1 * wlL2 / (wlL1 + wlL2);
    }

    std::optional<double> LinearCombination::getIonoFreePhaseWaveLength(const SatID& sv) const
    {
        auto type1 = obsTypesProvider_->getFirstPhaseType(sv.system);
        const int band1 = details::getRnx3BandIdBySystem(sv.system, type1);
        if (band1 < 0)
            return std::nullopt;

        auto type2 = obsTypesProvider_->getSecondPhaseType(sv.system);
        const int band2 = details::getRnx3BandIdBySystem(sv.system, type2);
        if (band2 < 0)
            return std::nullopt;

        return getIonoFreeWaveLength(sv, band1, band2);
    }

    std::optional<double> LinearCombination::getFirstFreqWaveLength(const SatID& sv) const
    {
        auto type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const int band1 = details::getRnx3BandIdBySystem(sv.system, type);
        if (band1 < 0)
            return std::nullopt;

        const int fcn = sv.getGloFcn();
        return getWavelength(sv.system, band1, fcn);
    }

    std::optional<double> LinearCombination::getSecondFreqWaveLength(const SatID& sv) const
    {
        auto type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const int band = details::getRnx3BandIdBySystem(sv.system, type);
        if (band < 0)
            return std::nullopt;

        const int fcn = sv.getGloFcn();
        return getWavelength(sv.system, band, fcn);
    }

#pragma region MWoubenna
    bool MWoubenna::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;
        const TypeID first_code_type = obsTypesProvider_->getFirstCodeType(sv.system);
        const auto& itC1 = tvMap.find(first_code_type);
        if (itC1 == tvMap.end())
            return false;

        const TypeID second_code_type = obsTypesProvider_->getSecondCodeType(sv.system);
        const auto& itC2 = tvMap.find(second_code_type);
        if (itC2 == tvMap.end())
            return false;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const auto& itL1 = tvMap.find(first_phase_type);
        if (itL1 == tvMap.end())
            return false;

        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const auto& itL2 = tvMap.find(second_phase_type);
        if (itL2 == tvMap.end())
            return false;

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        value = (itL1->second * F1 - itL2->second * F2) / (F1 - F2)
                - (itC1->second * F1 + itC2->second * F2) / (F1 + F2);

        return true;
    }

    TypeID MWoubenna::getType(SatelliteSystem ss) const
    {
        return TypeID::MWubbena;
    }
#pragma endregion

#pragma region PDelta
    bool PDelta::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        const TypeID first_code_type = obsTypesProvider_->getFirstCodeType(sv.system);
        const auto& itC1 = tvMap.find(first_code_type);
        if (itC1 == tvMap.end())
            return false;

        const TypeID second_code_type = obsTypesProvider_->getSecondCodeType(sv.system);
        const auto& itC2 = tvMap.find(second_code_type);
        if (itC2 == tvMap.end())
            return false;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        double c = F1 / (F1 + F2);
        double d = F2 / (F1 + F2);

        value = c * itC1->second + d * itC2->second;

        return true;
    }

    TypeID PDelta::getType(SatelliteSystem ss) const
    {
        return TypeID::Pdelta;
    }
#pragma endregion

#pragma region LDelta
    bool LDelta::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const auto& itL1 = tvMap.find(first_phase_type);
        if (itL1 == tvMap.end())
            return false;

        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const auto& itL2 = tvMap.find(second_phase_type);
        if (itL2 == tvMap.end())
            return false;

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        double e = F1 / (F1 - F2);
        double f = F2 / (F1 - F2);

        value = e * itL1->second + f * itL2->second;

        return true;
    }

    TypeID LDelta::getType(SatelliteSystem ss) const
    {
        return TypeID::Ldelta;
    }
#pragma endregion

#pragma region PC
    bool PCCombination::getCombination(const SatID& sv,
                                        const typeValueMap& tvMap,
                                        double& value) const
    {
        value = NAN;

        const TypeID first_code_type = obsTypesProvider_->getFirstCodeType(sv.system);
        const auto& itC1 = tvMap.find(first_code_type);
        if (itC1 == tvMap.end())
            return false;

        const TypeID second_code_type = obsTypesProvider_->getSecondCodeType(sv.system);
        const auto& itC2 = tvMap.find(second_code_type);
        if (itC2 == tvMap.end())
            return false;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        F1 = F1 * F1;
        F2 = F2 * F2;

        value = (F1 * itC1->second - F2 * itC2->second) / (F1 - F2);

        return true;
    }

    TypeID PCCombination::getType(SatelliteSystem ss) const
    {
        return TypeID::PC;
    }
#pragma endregion

#pragma region LC
    bool LCCombimnation::getCombination(const SatID& sv,
                                        const typeValueMap& tvMap,
                                        double& value) const
    {
        value = NAN;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const auto& itL1 = tvMap.find(first_phase_type);
        if (itL1 == tvMap.end())
            return false;

        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const auto& itL2 = tvMap.find(second_phase_type);
        if (itL2 == tvMap.end())
            return false;

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        F1 = F1 * F1;
        F2 = F2 * F2;

        value = (F1 * itL1->second - F2 * itL2->second) / (F1 - F2);

        return true;
    }

    TypeID LCCombimnation::getType(SatelliteSystem ss) const
    {
        return TypeID::LC;
    }
#pragma endregion

#pragma region LI
    bool LICombimnation::getCombination(const SatID& sv,
                                        const typeValueMap& tvMap,
                                        double& value) const
    {
        value = NAN;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const auto& itL1 = tvMap.find(first_phase_type);
        if (itL1 == tvMap.end())
            return false;
        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const auto& itL2 = tvMap.find(second_phase_type);
        if (itL2 == tvMap.end())
            return false;
        value = itL1->second - itL2->second;

        return true;
    }

    TypeID LICombimnation::getType(SatelliteSystem ss) const
    {
        return TypeID::LI;
    }
#pragma endregion

#pragma region PrefitC1
    bool PrefitC1::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // C1 or P1 code pseudorange, depending on ObservationTypesProvider settings
        const TypeID firstCodeType = obsTypesProvider_->getFirstCodeType(sv.system);
        auto it = tvMap.find(firstCodeType);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // ionospheric delay on L1
        it = tvMap.find(TypeID::ionoL1);
        if (it != tvMap.end())
            value -= it->second;

        if (!isPrecise)
            return true;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it != tvMap.end())
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // instrument delays
        it = tvMap.find(TypeID::instC1);
        if (it != tvMap.end())
            value -= it->second;

        // multipath L1
        it = tvMap.find(TypeID::mpC1);
        if (it != tvMap.end())
            value -= it->second;

        return true;
    }

    TypeID PrefitC1::getType(SatelliteSystem ss) const
    {
        const TypeID firstCodeType = obsTypesProvider_->getFirstCodeType(ss);
        return details::getPrefitObsTypeByObsType(firstCodeType);
    }
#pragma endregion

#pragma region PrefitP2

    bool PrefitC2::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // P2 or C2 code pseudorange, depending on ObservationTypesProvider settings
        const TypeID secondCodeType = obsTypesProvider_->getSecondCodeType(sv.system);
        auto it = tvMap.find(secondCodeType);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // ionospheric delay on L2
        it = tvMap.find(TypeID::ionoL2);
        if (it != tvMap.end())
            value -= it->second;

        if (!isPrecise)
            return true;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it != tvMap.end())
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // instrument delays
        it = tvMap.find(TypeID::instC2);
        if (it != tvMap.end())
            value -= it->second;

        // multipath P2/C2
        it = tvMap.find(TypeID::mpC2);
        if (it != tvMap.end())
            value -= it->second;

        return true;
    }

    TypeID PrefitC2::getType(SatelliteSystem ss) const
    {
        const TypeID secondCodeType = obsTypesProvider_->getSecondCodeType(ss);
        return details::getPrefitObsTypeByObsType(secondCodeType);
    }

#pragma endregion

#pragma region PrefitPC
    bool PrefitPC::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // L1 phase pseudorange
        auto it = tvMap.find(TypeID::PC);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        if (!isPrecise)
            return true;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // wind Up
        it = tvMap.find(TypeID::windUp);
        if (it == tvMap.end())
            return false;
        else
        {
            auto wu_wl = getIonoFreePhaseWaveLength(sv);
            if (!wu_wl)
                return false;

            value -= it->second * wu_wl.value() / TWO_PI;
        }
        return true;
    }

    TypeID PrefitPC::getType(SatelliteSystem ss) const
    {
        return TypeID::prefitPC;
    }

#pragma endregion

#pragma region PrefitL1
    bool PrefitL1::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // L1 phase pseudorange
        const TypeID type = obsTypesProvider_->getFirstPhaseType(sv.system);
        auto it = tvMap.find(type);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // ionospheric delay on L1
        it = tvMap.find(TypeID::ionoL1);
        if (it != tvMap.end())
            value += it->second;

        // wind Up
        it = tvMap.find(TypeID::windUp);
        if (it == tvMap.end())
        {
            return false;
        }
        else
        {
            const auto wl = getFirstFreqWaveLength(sv);
            if (!wl)
                return false;
            value -= it->second * wl.value() / TWO_PI;
        }

        // multipath L1
        it = tvMap.find(TypeID::mpL1);
        if (it != tvMap.end())
            value -= it->second;

        return true;
    }

    TypeID PrefitL1::getType(SatelliteSystem ss) const
    {
        const TypeID type = obsTypesProvider_->getFirstPhaseType(ss);
        return details::getPrefitObsTypeByObsType(type);
    }
#pragma endregion

#pragma region PrefitL2
    bool PrefitL2::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // L2 phase pseudorange
        const TypeID type = obsTypesProvider_->getSecondPhaseType(sv.system);
        auto it = tvMap.find(type);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // ionospheric delay on L1
        it = tvMap.find(TypeID::ionoL2);
        if (it != tvMap.end())
            value += it->second;

        // wind Up
        it = tvMap.find(TypeID::windUp);
        if (it == tvMap.end())
        {
            return false;
        }
        else
        {
            const auto wl = getSecondFreqWaveLength(sv);
            if (!wl)
                return false;
            value -= it->second * wl.value() / TWO_PI;
        }

        // multipath L1
        it = tvMap.find(TypeID::mpL2);
        if (it != tvMap.end())
            value -= it->second;

        return true;
    }

    TypeID PrefitL2::getType(SatelliteSystem ss) const
    {
        const TypeID type = obsTypesProvider_->getSecondPhaseType(ss);
        return details::getPrefitObsTypeByObsType(type);
    }

#pragma endregion

#pragma region PrefitLC
    bool PrefitLC::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
    {
        value = NAN;

        // L1 phase pseudorange
        auto it = tvMap.find(TypeID::LC);
        if (it == tvMap.end())
            return false;
        else
            value = it->second;

        // calculated distance
        it = tvMap.find(TypeID::rho);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // SV clock offset
        it = tvMap.find(TypeID::dtSat);
        if (it == tvMap.end())
            return false;
        else
            value += it->second;

        // rel
        it = tvMap.find(TypeID::rel);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // grav Delay
        it = tvMap.find(TypeID::gravDelay);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // sv phase center
        it = tvMap.find(TypeID::satPCenter);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // tropo
        it = tvMap.find(TypeID::tropoSlant);
        if (it == tvMap.end())
            return false;
        else
            value -= it->second;

        // wind Up
        it = tvMap.find(TypeID::windUp);
        if (it == tvMap.end())
        {
            return false;
        }
        else
        {
            auto wu_wl = getIonoFreePhaseWaveLength(sv);
            if (!wu_wl)
                return false;

            value -= it->second * wu_wl.value() / TWO_PI;
        }
        return true;
    }

    TypeID PrefitLC::getType(SatelliteSystem ss) const
    {
        return TypeID::prefitLC;
    }
#pragma endregion

#pragma region CodeIonoDelayL1

    bool CodeIonoDelayL1::getCombination(const SatID& sv,
                                         const typeValueMap& tvMap,
                                         double& value) const
    {
        value = NAN;

        const TypeID first_code_type = obsTypesProvider_->getFirstCodeType(sv.system);
        const auto& itC1 = tvMap.find(first_code_type);
        if (itC1 == tvMap.end())
            return false;

        const TypeID second_code_type = obsTypesProvider_->getSecondCodeType(sv.system);
        const auto& itP2 = tvMap.find(second_code_type);
        if (itP2 == tvMap.end())
            return false;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        F1 *= F1;
        F2 *= F2;

        value = (itC1->second - itP2->second) * F2 / (F2 - F1);

        return true;
    }

    TypeID CodeIonoDelayL1::getType(SatelliteSystem ss) const
    {
        return TypeID::ionoL1;
    }

#pragma endregion

#pragma region PhaseIonoDelayL1

    bool PhaseIonoDelayL1::getCombination(const SatID& sv,
                                          const typeValueMap& tvMap,
                                          double& value) const
    {
        value = NAN;

        const TypeID first_phase_type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const auto& itC1 = tvMap.find(first_phase_type);
        if (itC1 == tvMap.end())
            return false;

        const TypeID second_phase_type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const auto& itP2 = tvMap.find(second_phase_type);
        if (itP2 == tvMap.end())
            return false;

        const int band1 = details::getRnx3BandIdBySystem(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = details::getRnx3BandIdBySystem(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        F1 *= F1;
        F2 *= F2;

        value = (itC1->second - itP2->second) * F2 / (F2 - F1);

        return true;
    }

    TypeID PhaseIonoDelayL1::getType(SatelliteSystem ss) const
    {
        return TypeID::ionoL1;
    }

#pragma endregion
} // namespace pod
