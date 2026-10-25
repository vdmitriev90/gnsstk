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
	// ProcessingData, ProcessingTechnicalInfo, ResidualStatisticsByElevation, ResidualStatisticsBySatSystem,
	// ResidualSatelliteStatistics and the SVS lookup table.
	class StatisticsDatabase : public SQLiteConnection
	{
	  public:
		static const unsigned SCHEMA_VERSION;
		static const std::int32_t APPLICATION_ID;

		explicit StatisticsDatabase(const std::string& path);

		// Atomically stores a processing run with all its statistics; returns the new ProcessingData.ID.
		// Nothing is written if any part fails.
		std::int64_t addRun(const ProcessingDataRecord& data,
							const ProcessingTechnicalInfoRecord& info,
							const std::set<gnsstk::SatID>& svs,
							const std::vector<ResidualStatisticsRecord>& elevationResults,
							const std::vector<ResidualStatisticsRecord>& systemResults,
							const std::vector<ResidualSatelliteStatisticsRecord>& satelliteResults,
														const std::optional<SolutionStatisticsRecord>& solution = std::nullopt);

								  private:
									void addSolutionStatistics(std::int64_t processingDataId, const SolutionStatisticsRecord& s);
		void setPragmas();
		void migrate();

		// Inserts ProcessingData and ProcessingTechnicalInfo atomically; returns ProcessingData.ID.
				std::int64_t addProcessingData(const ProcessingDataRecord& data, const ProcessingTechnicalInfoRecord& info);
		void addSatellites(const std::set<gnsstk::SatID>& svs);
		void addResidualStatisticsByElevation(std::int64_t processingDataId,
											  const std::vector<ResidualStatisticsRecord>& records);
		void addResidualStatisticsBySatSystem(std::int64_t processingDataId,
											  const std::vector<ResidualStatisticsRecord>& records);
		void addResidualSatelliteStatistics(std::int64_t processingDataId,
											const std::vector<ResidualSatelliteStatisticsRecord>& records);
		void bindSummary(sqlite3_stmt* stmt, int firstIdx, const ResidualSummary& s);
	};
} // namespace pod

#endif // !POD_STATISTICS_DATABASE
