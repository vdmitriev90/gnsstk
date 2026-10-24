#ifndef POD_PROCESSING_RECORDS_H
#define POD_PROCESSING_RECORDS_H

#include "SatID.hpp"
#include "TypeID.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace pod
{
	// The observation class is the pair (gnsstk::SatelliteSystem, post-fit residual gnsstk::TypeID),
	// e.g. (GPS, postfitPC) or (Glonass, postfitLC). The TypeID is stored in the `ObservationClass` column
	// as TypeID::type, the system in the `SatelliteSystem` column as its integer value.

	struct ProcessingRunRecord
	{
		std::int64_t processingStartedUnixSec = 0; // system_clock, seconds since Unix epoch
		std::int64_t processingDurationMSec = 0;   // steady_clock, milliseconds
		std::string softwareVersion;
		std::optional<int> solutionType;
		std::optional<double> elevationCutoffDeg;
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
