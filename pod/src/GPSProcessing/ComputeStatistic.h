#pragma once
#include "GnssEpochMap.h"
#include "GnssSolution.h"
#include "ProcessingRecords.h"

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace pod
{
	class PositionStatistic
	{
	  public:
		PositionStatistic(SlnType st, gnsstk::TypeIDSet tIDs);

		SlnType getSlnType() const;
		gnsstk::TypeIDSet getTypeIDSet() const;

		void compute(const GnssEpochMap& data, gnsstk::Vector<double>& sln, gnsstk::Matrix<double>& cov);
		double getHorStdDev() const;
		double getVertStdDev() const;

		int goodEpochs = 0;
		int totalEpochs = 0;

	  private:
		SlnType slnType;
		gnsstk::TypeIDSet types;
		gnsstk::Matrix<double> covarNEU;
	};

	// Online accumulator (Welford): count, mean, sample variance, min, max without storing samples.
	// The only implementation of residual statistics calculations.
	class RunningStats
	{
	  public:
		void add(double v);

		std::int64_t count() const;
		double mean() const;
		double min() const;
		double max() const;
		// Sample (n-1) standard deviation, nullopt when fewer than 2 values.
		std::optional<double> sampleStdDev() const;

		// Fills count/mean/std/min/max. Std is the sample (n-1) one, unset when fewer than 2 values.
		void writeTo(ResidualSummary& out) const;

	  private:
		std::int64_t count_ = 0;
		double mean_ = 0.0;
		double m2_ = 0.0;
		double min_ = 0.0;
		double max_ = 0.0;
	};

	// Post-fit residual statistics. All values are aggregated in a single pass into records;
	// the same records are persisted to the database and printed to the console.
	// Observations with satStatus < 0 count as rejected and are excluded from the statistics.
	class PostfitResidualStatistics
	{
	  public:
		static constexpr double ELEV_BIN_STEP_DEG = 10.0;
		static constexpr double ELEV_BIN_MAX_DEG = 90.0;

		explicit PostfitResidualStatistics(bool withElevation = false);

		// Computes statistics only; nothing is printed.
		void compute(const GnssEpochMap& data);

		// Per (system, residual type) statistics over all elevations.
		const std::vector<ResidualStatisticsRecord>& systemResults() const;

		// Elevation-binned statistics per (system, residual type).
		const std::vector<ResidualStatisticsRecord>& elevationResults() const;

		// Per-satellite statistics per residual type (elevation >= 30 deg only).
		const std::vector<ResidualSatelliteStatisticsRecord>& satelliteResults() const;

		// Prints systemResults() and, if enabled, elevationResults().
		void print(std::ostream& os) const;

		// Fills count/mean/std/min/max.
		static void summarize(const std::vector<double>& values, ResidualSummary& out);

	  private:
		struct Item
		{
			std::string name;
			gnsstk::TypeID type;
		};

		void computeAggregates(const GnssEpochMap& data);

		std::vector<Item> printOrder;
		std::vector<ResidualStatisticsRecord> sysResults;
		std::vector<ResidualStatisticsRecord> binResults;
		std::vector<ResidualSatelliteStatisticsRecord> satResults;
		bool elevationEnabled;
	};

} // namespace pod
