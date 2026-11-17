#include "BiasSinexCorrector.h"

using namespace gnsstk;

namespace pod
{
	BiasSinexCorrector::BiasSinexCorrector(const BiasSinexStore& store)
		: store_(&store)
	{
		setDefaultCodeMappings();
		setDefaultPhaseMappings();
	}

	BiasSinexCorrector& BiasSinexCorrector::clearMappings()
	{
		codeMappings_.clear();
		phaseMappings_.clear();
		return *this;
	}

	BiasSinexCorrector& BiasSinexCorrector::setDefaultCodeMappings()
	{
		codeMappings_.clear();

		mapCodeObservable(SatelliteSystem::GPS, TypeID(TypeID::C1), "C1C");
		mapCodeObservable(SatelliteSystem::GPS, TypeID(TypeID::P1), "C1W");
		mapCodeObservable(SatelliteSystem::GPS, TypeID(TypeID::P2), "C2W");

		mapCodeObservable(SatelliteSystem::Glonass, TypeID(TypeID::C1), "C1C");
		mapCodeObservable(SatelliteSystem::Glonass, TypeID(TypeID::P1), "C1P");
		mapCodeObservable(SatelliteSystem::Glonass, TypeID(TypeID::P2), "C2P");

		mapCodeObservable(SatelliteSystem::Galileo, TypeID(TypeID::C1), "C1X");
		mapCodeObservable(SatelliteSystem::Galileo, TypeID(TypeID::C5), "C5X");

		mapCodeObservable(SatelliteSystem::BeiDou, TypeID(TypeID::C1), "C1P");
		mapCodeObservable(SatelliteSystem::BeiDou, TypeID(TypeID::C5), "C5P");

		mapCodeObservable(SatelliteSystem::QZSS, TypeID(TypeID::C1), "C1L");
		mapCodeObservable(SatelliteSystem::QZSS, TypeID(TypeID::C5), "C5Q");

		return *this;
	}

	BiasSinexCorrector& BiasSinexCorrector::mapCodeObservable(const SatelliteSystem& system,
															  const TypeID& type,
															  const std::string& obsCode)
	{
		codeMappings_[system][type] = obsCode;
		return *this;
	}

	BiasSinexCorrector& BiasSinexCorrector::setDefaultPhaseMappings()
	{
		phaseMappings_.clear();

		mapPhaseObservable(SatelliteSystem::GPS, TypeID(TypeID::L1), "L1C");
		mapPhaseObservable(SatelliteSystem::GPS, TypeID(TypeID::L2), "L2W");

		mapPhaseObservable(SatelliteSystem::Glonass, TypeID(TypeID::L1), "L1C");
		mapPhaseObservable(SatelliteSystem::Glonass, TypeID(TypeID::L2), "L2P");

		mapPhaseObservable(SatelliteSystem::Galileo, TypeID(TypeID::L1), "L1X");
		mapPhaseObservable(SatelliteSystem::Galileo, TypeID(TypeID::L5), "L5X");

		mapPhaseObservable(SatelliteSystem::BeiDou, TypeID(TypeID::L1), "L1P");
		mapPhaseObservable(SatelliteSystem::BeiDou, TypeID(TypeID::L5), "L5P");

		mapPhaseObservable(SatelliteSystem::QZSS, TypeID(TypeID::L1), "L1L");
		mapPhaseObservable(SatelliteSystem::QZSS, TypeID(TypeID::L5), "L5Q");

		return *this;
	}

	BiasSinexCorrector& BiasSinexCorrector::mapPhaseObservable(const SatelliteSystem& system,
															   const TypeID& type,
															   const std::string& obsCode)
	{
		phaseMappings_[system][type] = obsCode;
		return *this;
	}

	void BiasSinexCorrector::applyOsb(const CommonTime& epoch,
									  const SatID& sat,
									  const std::map<SatelliteSystem, TypeMap>& mappings,
									  TypeValueRow& tvMap) const
	{
		const auto sysIt = mappings.find(sat.system);
		if (sysIt == mappings.end())
			return;

		for (const auto& mapping : sysIt->second)
		{
			const TypeID& type = mapping.first;
			const std::string& obsCode = mapping.second;

			const auto tvIt = tvMap.find(type);
			if (tvIt == tvMap.end())
				continue;

			const auto* bias = store_->findSatelliteOsb(sat, obsCode, epoch);
			if (bias == nullptr)
				continue;
			if (bias->unit != BiasSinexUnit::Ns)
				continue;
			const double bias_m = bias->estimatedValue * gnsstk::C_MPS * 1.0e-9;
			tvIt->second -= bias_m;
		}
	}

	SatTypePtrMap& BiasSinexCorrector::Process(const CommonTime& epoch, SatTypePtrMap& gData)
	{
		for (auto it = gData.begin(); it != gData.end(); ++it)
		{
			applyOsb(epoch, it->first, codeMappings_, it->second);
			applyOsb(epoch, it->first, phaseMappings_, it->second);
		}

		return gData;
	}

	IRinex& BiasSinexCorrector::Process(IRinex& gData)
	{
		Process(gData.getHeader().epoch, gData.getBody());
		return gData;
	}

	std::string BiasSinexCorrector::getClassName() const
	{
		return "BiasSinexCorrector";
	}
} // namespace pod
