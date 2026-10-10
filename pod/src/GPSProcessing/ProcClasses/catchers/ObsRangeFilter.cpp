#include "ObsRangeFilter.h"

using namespace gnsstk;

namespace pod
{
	ObsRangeFilter::ObsRangeFilter()
		: filterTypes_{ObsRangeType::FirstCode}
		, minLimit_(15000000.0)
		, maxLimit_(30000000.0)
	{
	}

	ObsRangeFilter::ObsRangeFilter(ObsRangeType type, double min, double max)
		: filterTypes_{type}
		, minLimit_(min)
		, maxLimit_(max)
	{
	}

	ObsRangeFilter::ObsRangeFilter(ObsRangeType type)
		: filterTypes_{type}
		, minLimit_(15000000.0)
		, maxLimit_(30000000.0)
	{
	}

	ObsRangeFilter::ObsRangeFilter(std::initializer_list<ObsRangeType> types, double min, double max)
		: filterTypes_(types)
		, minLimit_(min)
		, maxLimit_(max)
	{
	}

	ObsRangeFilter::ObsRangeFilter(std::initializer_list<ObsRangeType> types)
		: filterTypes_(types)
		, minLimit_(15000000.0)
		, maxLimit_(30000000.0)
	{
	}

	TypeID ObsRangeFilter::resolve(ObsRangeType type, SatelliteSystem ss) const
	{
		switch (type)
		{
		case ObsRangeType::FirstCode:
			return resolver_.firstCode(ss);
		case ObsRangeType::SecondCode:
			return resolver_.secondCode(ss);
		case ObsRangeType::FirstPhase:
			return resolver_.firstPhase(ss);
		case ObsRangeType::SecondPhase:
			return resolver_.secondPhase(ss);
		case ObsRangeType::Snr:
			return TypeID::S1;
		case ObsRangeType::IonoFreeCode:
			return TypeID::PC;
		}
		return TypeID::Unknown;
	}

	IRinex& ObsRangeFilter::Process(IRinex& gData)
	{
		auto& rejSatItem = rejectedSatsTable[gData.getHeader().epoch];

		SatIDSet satRejectedSet;
		auto& body = gData.getBody();

		for (auto& [sat, tvPtr] : body)
		{
			for (const auto& type : filterTypes_)
			{
				try
				{
					// Resolve the actual observation for this system and check
					// that its value is within bounds.
					if (!checkValue((*tvPtr)(resolve(type, sat.system))))
					{
						satRejectedSet.insert(sat);
						break;
					}
				}
				catch (...)
				{
					// If the observation is missing or cannot be resolved for
					// this system, schedule the satellite for removal.
					satRejectedSet.insert(sat);
					break;
				}
			}
		}

		body.removeSatID(satRejectedSet);
		rejSatItem.insert(satRejectedSet.begin(), satRejectedSet.end());

		return gData;
	}

	std::string ObsRangeFilter::getClassName() const
	{
		return "pod::ObsRangeFilter";
	}
} // namespace pod
