#include "BiasSinexCorrector.h"

using namespace gnsstk;

namespace pod
{
	BiasSinexCorrector::BiasSinexCorrector(const BiasSinexStore& store)
		: store_(&store)
	{
		setDefaultCodeMappings();
	}

	BiasSinexCorrector& BiasSinexCorrector::clearMappings()
	{
		codeMappings_.clear();
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

	SatTypePtrMap& BiasSinexCorrector::Process(const CommonTime& epoch, SatTypePtrMap& gData)
	{
		for (auto it = gData.begin(); it != gData.end(); ++it)
		{
			const SatID& sat = it->first;
			typeValueMap* tvMap = it->second;
			if (tvMap == nullptr)
				continue;

			const auto sysIt = codeMappings_.find(sat.system);
			if (sysIt == codeMappings_.end())
				continue;

			for (const auto& mapping : sysIt->second)
			{
				const TypeID& type = mapping.first;
				const std::string& obsCode = mapping.second;

				const auto tvIt = tvMap->find(type);
				if (tvIt == tvMap->end())
					continue;

				const auto* bias = store_->findSatelliteOsb(sat, obsCode, epoch);
				if (bias == nullptr)
					continue;
				if (bias->unit != BiasSinexUnit::Ns)
					continue;
                const double bias_m = bias->estimatedValue* gnsstk::C_MPS* 1.0e-9;
                tvIt->second -= bias_m;
			}
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
