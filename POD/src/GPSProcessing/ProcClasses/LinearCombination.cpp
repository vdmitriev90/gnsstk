#include "LinearCombination.h"

#include "GNSSconstants.hpp"
#include "GnssObsMapping.h"

#include <array>

namespace pod
{
    ObsTypesProviderPtr ObservationTypesProvider::instance()
    {
        static auto inst = std::make_shared<ObservationTypesProvider>();
        return inst;
    }

    TypeID ObservationTypesProvider::getFirstCodeType(SatelliteSystem ss) const
    {
        const auto* cfg = obs_mapping::findConfig(ss);
        if (!cfg)
        {
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getFirstCodeType");
            return TypeID::Unknown;
        }

        return obs_mapping::getOrAssert(cfg->firstCode, ss, "getFirstCodeType");
    }
    TypeID ObservationTypesProvider::getSecondCodeType(SatelliteSystem ss) const
    {

        const auto* cfg = obs_mapping::findConfig(ss);
        if (!cfg)
        {
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getSecondCodeType");
            return TypeID::Unknown;
        }

        return obs_mapping::getOrAssert(cfg->secondCode, ss, "getSecondCodeType");
    }

    TypeID ObservationTypesProvider::getFirstPhaseType(SatelliteSystem ss) const
    {

        const auto* cfg = obs_mapping::findConfig(ss);
        if (!cfg)
        {
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getFirstPhaseType");
            return TypeID::Unknown;
        }

        return obs_mapping::getOrAssert(cfg->firstPhase, ss, "getFirstPhaseType");
    }
    TypeID ObservationTypesProvider::getSecondPhaseType(SatelliteSystem ss) const
    {

        const auto* cfg = obs_mapping::findConfig(ss);
        if (!cfg)
        {
            GNSSTK_ASSERT_MSG(false, "Unknown satellite system in getSecondPhaseType");
            return TypeID::Unknown;
        }

        return obs_mapping::getOrAssert(cfg->secondPhase, ss, "getSecondPhaseType");
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
        auto type2 = obsTypesProvider_->getSecondPhaseType(sv.system);

        const int band1 = obs_mapping::getBand(sv.system, type1);
        const int band2 = obs_mapping::getBand(sv.system, type2);

        if (band1 < 0 || band2 < 0)
            return std::nullopt;

        return getIonoFreeWaveLength(sv, band1, band2);
    }

    std::optional<double> LinearCombination::getFirstFreqWaveLength(const SatID& sv) const
    {
        auto type = obsTypesProvider_->getFirstPhaseType(sv.system);
        const int band1 = obs_mapping::getBand(sv.system, type);
        if (band1 < 0)
            return std::nullopt;

        const int fcn = sv.getGloFcn();
        return getWavelength(sv.system, band1, fcn);
    }

    std::optional<double> LinearCombination::getSecondFreqWaveLength(const SatID& sv) const
    {
        auto type = obsTypesProvider_->getSecondPhaseType(sv.system);
        const int band = obs_mapping::getBand(sv.system, type);
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type);
        if (band2 < 0)
            return false;

        int fcn = sv.getGloFcn();
        double F1 = C_MPS / getWavelength(sv.system, band1, fcn);
        double F2 = C_MPS / getWavelength(sv.system, band2, fcn);

        value =
            (itL1->second * F1 - itL2->second * F2) / (F1 - F2) - (itC1->second * F1 + itC2->second * F2) / (F1 + F2);

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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type);
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type);
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
    bool PCCombination::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type.type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type.type);
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
    bool LCCombimnation::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type.type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type.type);
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
    bool LICombimnation::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
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
        return obs_mapping::getPrefitObs(firstCodeType);
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
        return obs_mapping::getPrefitObs(secondCodeType);
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
        return obs_mapping::getPrefitObs(type);
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
        return obs_mapping::getPrefitObs(type);
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

    bool CodeIonoDelayL1::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type);
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

    bool PhaseIonoDelayL1::getCombination(const SatID& sv, const typeValueMap& tvMap, double& value) const
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

        const int band1 = obs_mapping::getBand(sv.system, first_phase_type);
        if (band1 < 0)
            return false;
        const int band2 = obs_mapping::getBand(sv.system, second_phase_type);
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
