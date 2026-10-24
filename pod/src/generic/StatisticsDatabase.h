#ifndef POD_STATISTICS_DATABASE
#define POD_STATISTICS_DATABASE

#include "ProcessingRecords.h"
#include "SQLiteConnection.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace pod
{
	// Shared statistics database appended across processing runs:
	// ProcessingRuns, ResidualStatistics, ResidualSatelliteStatistics and the SVS lookup table.
	class StatisticsDatabase : public SQLiteConnection
	{
	  public:
		static const unsigned SCHEMA_VERSION;
		static const std::int32_t APPLICATION_ID;

		explicit StatisticsDatabase(const std::string& path);

		// Atomically stores a processing run with all its statistics; returns the new ProcessingRuns.ID.
		// Nothing is written if any part fails.
		std::int64_t addRun(const ProcessingRunRecord& run,
							const std::set<gnsstk::SatID>& svs,
							const std::vector<ResidualStatisticsRecord>& elevationResults,
							const std::vector<ResidualStatisticsRecord>& systemResults,
							const std::vector<ResidualSatelliteStatisticsRecord>& satelliteResults);

	  private:
		void setPragmas();
		void migrate();

		std::int64_t addProcessingRun(const ProcessingRunRecord& record);
		void addSatellites(const std::set<gnsstk::SatID>& svs);
		void addResidualStatistics(std::int64_t processingRunId, const std::vector<ResidualStatisticsRecord>& records);
		void addResidualSatelliteStatistics(std::int64_t processingRunId,
											const std::vector<ResidualSatelliteStatisticsRecord>& records);
		void bindSummary(sqlite3_stmt* stmt, int firstIdx, const ResidualSummary& s);
	};
} // namespace pod

#endif // !POD_STATISTICS_DATABASE
