#ifndef POD_PROCESSING_RECORDS_H
#define POD_PROCESSING_RECORDS_H

#include "SatID.hpp"
#include "Triple.hpp"
#include "TypeID.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace pod
{
	// The observation class is the pair (gnsstk::SatelliteSystem, post-fit residual gnsstk::TypeID),
	// e.g. (GPS, postfitPC) or (Glonass, postfitLC). The TypeID is stored in the `ObservationClass` column
	// as TypeID::type, the system in the `SatelliteSystem` column as its integer value.

	// Stable application-defined satellite-system bits stored in ProcessingData.SatelliteSystemsMask.
	// Independent of gnsstk::SatelliteSystem enum values; never renumber existing bits.
	enum SatelliteSystemBit : int
	{
		SatSysBitGPS = 1 << 0,
		SatSysBitGlonass = 1 << 1,
		SatSysBitGalileo = 1 << 2,
		SatSysBitBeiDou = 1 << 3,
		SatSysBitQZSS = 1 << 4,
		SatSysBitIRNSS = 1 << 5,
		SatSysBitSBAS = 1 << 6,
	};

	inline int satelliteSystemBit(gnsstk::SatelliteSystem s)
	{
		switch (s)
		{
			case gnsstk::SatelliteSystem::GPS: return SatSysBitGPS;
			case gnsstk::SatelliteSystem::Glonass: return SatSysBitGlonass;
			case gnsstk::SatelliteSystem::Galileo: return SatSysBitGalileo;
			case gnsstk::SatelliteSystem::BeiDou: return SatSysBitBeiDou;
			case gnsstk::SatelliteSystem::QZSS: return SatSysBitQZSS;
			case gnsstk::SatelliteSystem::IRNSS: return SatSysBitIRNSS;
			case gnsstk::SatelliteSystem::Geosync: return SatSysBitSBAS;
			default: return 0;
		}
	}

	template <class SatContainer>
	int satelliteSystemsMask(const SatContainer& sats)
	{
		int mask = 0;
		for (const auto& sv : sats)
			mask |= satelliteSystemBit(sv.system);
		return mask;
	}

	// Describes the processed GNSS dataset (table ProcessingData).
	struct ProcessingDataRecord
	{
		std::string roverSite;
		std::optional<std::string> baseSite; // std::nullopt -> SQL NULL
		int solutionType = 0;
		int satelliteSystemsMask = 0;
		double elevationCutoffDeg = 0.0;
		std::int64_t observationStartUnixSec = 0;
		std::int64_t epochNumber = 0;
		std::int64_t observationDurationSec = 0;
		};

		// Symmetric 3x3 XYZ covariance matrix: only unique elements are stored.
		struct CovarianceMatrix3
		{
			double xx = 0.0, yy = 0.0, zz = 0.0;
			double xy = 0.0, xz = 0.0, yz = 0.0;
		};

		// Solution statistics over epochs of the desired solution type (table SolutionStatistics).
		struct SolutionStatisticsRecord
		{
			gnsstk::Triple average; // mean X, Y, Z
			double horizontalStdDev = 0.0, verticalStdDev = 0.0;
			double averageSvInUse = 0.0;
			CovarianceMatrix3 covariance;
		};

	// Execution metadata (table ProcessingTechnicalInfo).
	struct ProcessingTechnicalInfoRecord
	{
		std::int64_t processingStartedUnixSec = 0; // system_clock, seconds since Unix epoch
		std::int64_t processingDurationMSec = 0;   // steady_clock, milliseconds
		std::optional<std::string> softwareVersion; // std::nullopt -> SQL NULL
	};

	// Common summary fields. Unavailable values are std::nullopt and stored as SQL NULL.
	struct ResidualSummary
	{
		std::int64_t count_ = 0;
		std::int64_t rejectedCount_ = 0;
		std::optional<double> mean_;
		std::optional<double> stdDev_; // sample (n-1) standard deviation
		std::optional<double> minimum_;
		std::optional<double> maximum_;

		// Clears count and statistics; rejectedCount_ is left untouched.
		void reset()
		{
			count_ = 0;
			mean_.reset();
			stdDev_.reset();
			minimum_.reset();
			maximum_.reset();
		}
	};

	struct ResidualStatisticsRecord : ResidualSummary
	{
		gnsstk::SatelliteSystem satelliteSystem;
		gnsstk::TypeID residualType;
		double elevationMinDeg = 0.0;
		double elevationMaxDeg = 0.0;
	};

	struct ResidualSatelliteStatisticsRecord : ResidualSummary
	{
		gnsstk::SatID sat; // resolved to SVS.ID by the adapter
		gnsstk::TypeID residualType;
	};
} // namespace pod

#endif // !POD_PROCESSING_RECORDS_H
