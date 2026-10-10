#pragma once

#include "CommonTime.hpp"
#include "SatID.hpp"
#include "SatelliteSystem.hpp"
#include "TimeSystem.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pod
{
	enum class BiasSinexEntryType
	{
		Unknown,
		OSB,
		DSB,
	};

	enum class BiasSinexTarget
	{
		Unknown,
		Satellite,
		Station,
	};

	enum class BiasSinexUnit
	{
		Unknown,
		Ns,
		Cycles,
	};

	struct BiasSinexHeader
	{
		double version = 0.0;
		std::string creationAgency;
		std::string creationEpoch;
		std::string dataAgency;
		std::string startEpoch;
		std::string endEpoch;
		char observationCode = ' ';
		std::string serial;
	};

	struct BiasSinexSolutionRecord
	{
		BiasSinexEntryType entryType = BiasSinexEntryType::Unknown;
		BiasSinexTarget target = BiasSinexTarget::Unknown;

		std::string svn;
		std::optional<gnsstk::SatID> sat;
		std::string station;
		std::string obs1;
		std::string obs2;

		gnsstk::CommonTime start = gnsstk::CommonTime::BEGINNING_OF_TIME;
		gnsstk::CommonTime end = gnsstk::CommonTime::BEGINNING_OF_TIME;
		BiasSinexUnit unit = BiasSinexUnit::Unknown;

		double estimatedValue = 0.0;
		double stdDev = 0.0;
		std::optional<double> estimatedSlope;
		std::optional<double> slopeStdDev;

		std::size_t lineNumber = 0;
		std::string rawLine;

		[[nodiscard]] bool isSatelliteOsb() const noexcept;
		[[nodiscard]] bool contains(const gnsstk::CommonTime& epoch) const noexcept;
	};

	struct BiasSinexDescription
	{
		std::map<std::string, std::vector<std::string>> values;
	};

	class BiasSinexStore
	{
	  public:
		void clear();
		void loadFile(const std::string& filePath);

		[[nodiscard]] const BiasSinexHeader& header() const noexcept;
		[[nodiscard]] const BiasSinexDescription& description() const noexcept;
		[[nodiscard]] const std::vector<BiasSinexSolutionRecord>& records() const noexcept;

		[[nodiscard]] const BiasSinexSolutionRecord* findSatelliteOsb(const gnsstk::SatID& sat,
																	  std::string_view observable,
																	  const gnsstk::CommonTime& epoch) const noexcept;

	  private:
		struct SatelliteObservableKey
		{
			gnsstk::SatID sat;
			std::string observable;

			bool operator<(const SatelliteObservableKey& rhs) const noexcept;
		};

		void rebuildIndex();

		BiasSinexHeader header_;
		BiasSinexDescription description_;
		std::vector<BiasSinexSolutionRecord> records_;
		std::map<SatelliteObservableKey, std::vector<std::size_t>> satelliteOsbIndex_;
	};
} // namespace pod
