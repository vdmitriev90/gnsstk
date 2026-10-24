#include "StatisticsDatabase.h"

namespace pod
{
	namespace
	{
		const std::string createStatisticsSchemaCommand =
			"CREATE TABLE IF NOT EXISTS `SVS` ("
			"\t`ID`\tINTEGER NOT NULL,"
			"\t`SVID`\tINTEGER NOT NULL,"
			"\t`SSID`\tINTEGER NOT NULL,"
			"\tPRIMARY KEY(`ID`)"
			");"
			"CREATE UNIQUE INDEX IF NOT EXISTS `SV_Unique` ON `SVS` ("
			"\t`SVID`,"
			"\t`SSID`"
			");"
			"CREATE TABLE IF NOT EXISTS `ProcessingRuns` ("
			"`ID` INTEGER NOT NULL,"
			"`ProcessingStartedUnixSec` INTEGER NOT NULL,"
			"`ProcessingDurationMSec` INTEGER NOT NULL,"
			"`SoftwareVersion` TEXT,"
			"`SolutionType` INTEGER,"
			"`ElevationCutoffDeg` REAL,"
			"PRIMARY KEY (`ID`)"
			");"
			"CREATE TABLE IF NOT EXISTS `ResidualStatistics` ("
			"`ID` INTEGER NOT NULL,"
			"`ProcessingRunID` INTEGER NOT NULL,"
			"`SatelliteSystem` INTEGER NOT NULL,"
			"`ObservationClass` INTEGER NOT NULL,"
			"`ElevationMinDeg` REAL NOT NULL,"
			"`ElevationMaxDeg` REAL NOT NULL,"
			"`Count` INTEGER NOT NULL,"
			"`RejectedCount` INTEGER NOT NULL DEFAULT 0,"
			"`Mean` REAL,"
			"`StandardDeviation` REAL,"
			"`Minimum` REAL,"
			"`Maximum` REAL,"
			"PRIMARY KEY (`ID`),"
			"FOREIGN KEY (`ProcessingRunID`) REFERENCES `ProcessingRuns` (`ID`)"
			");"
			"CREATE UNIQUE INDEX IF NOT EXISTS `ResidualStatistics_Bin_Unique` "
			"ON `ResidualStatistics` (`ProcessingRunID`, `SatelliteSystem`, `ObservationClass`, "
			"`ElevationMinDeg`, `ElevationMaxDeg`);"
			"CREATE TABLE IF NOT EXISTS `ResidualSatelliteStatistics` ("
			"`ProcessingRunID` INTEGER NOT NULL,"
			"`SV` INTEGER NOT NULL,"
			"`ObservationClass` INTEGER NOT NULL,"
			"`Count` INTEGER NOT NULL,"
			"`RejectedCount` INTEGER NOT NULL DEFAULT 0,"
			"`Mean` REAL,"
			"`StandardDeviation` REAL,"
			"`Minimum` REAL,"
			"`Maximum` REAL,"
			"PRIMARY KEY (`ProcessingRunID`, `SV`, `ObservationClass`),"
			"FOREIGN KEY (`ProcessingRunID`) REFERENCES `ProcessingRuns` (`ID`),"
			"FOREIGN KEY (`SV`) REFERENCES `SVS` (`ID`)"
			");";
	} // namespace

	const unsigned StatisticsDatabase::SCHEMA_VERSION = 1;
	const std::int32_t StatisticsDatabase::APPLICATION_ID = 0x47535441; // 'GSTA'

	StatisticsDatabase::StatisticsDatabase(const std::string& path) : SQLiteConnection(path)
	{
		checkApplicationId(APPLICATION_ID, "statistics");
		setPragmas();
		migrate();
	}

	// The file is shared and appended across runs, so a durable on-disk journal is used.
	void StatisticsDatabase::setPragmas()
	{
		applyPragmas({
			"PRAGMA foreign_keys = ON;",
			"PRAGMA journal_mode = DELETE;",
			"PRAGMA busy_timeout = 5000;",
		});
	}

	void StatisticsDatabase::migrate()
	{
		const unsigned version = userVersion();
		if (version == SCHEMA_VERSION)
			return;
		if (version > SCHEMA_VERSION)
			errorHandler(SQLITE_ERROR, "Statistics database schema is newer than supported.");

		Transaction tx(*this);
		execScript(createStatisticsSchemaCommand);
		setUserVersion(SCHEMA_VERSION);
		tx.commit();
	}

	std::int64_t StatisticsDatabase::addRun(const ProcessingRunRecord& run,
											const std::set<gnsstk::SatID>& svs,
											const std::vector<ResidualStatisticsRecord>& elevationResults,
											const std::vector<ResidualStatisticsRecord>& systemResults,
											const std::vector<ResidualSatelliteStatisticsRecord>& satelliteResults)
	{
		std::int64_t runId = 0;
		inTransaction([&] {
			runId = addProcessingRun(run);
			addSatellites(svs);
			addResidualStatistics(runId, elevationResults);
			addResidualStatistics(runId, systemResults);
			addResidualSatelliteStatistics(runId, satelliteResults);
		});
		return runId;
	}

	void StatisticsDatabase::bindSummary(sqlite3_stmt* stmt, int firstIdx, const ResidualSummary& s)
	{
		bindInt64(stmt, firstIdx, s.count_);
		bindInt64(stmt, firstIdx + 1, s.rejectedCount_);
		bindOptionalDouble(stmt, firstIdx + 2, s.mean_);
		bindOptionalDouble(stmt, firstIdx + 3, s.stdDev_);
		bindOptionalDouble(stmt, firstIdx + 4, s.minimum_);
		bindOptionalDouble(stmt, firstIdx + 5, s.maximum_);
	}

	std::int64_t StatisticsDatabase::addProcessingRun(const ProcessingRunRecord& r)
	{
		std::int64_t id = 0;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ProcessingRuns`(`ProcessingStartedUnixSec`,`ProcessingDurationMSec`,"
				"`SoftwareVersion`,`SolutionType`,`ElevationCutoffDeg`) VALUES (?1,?2,?3,?4,?5);");
			bindInt64(stmt, 1, r.processingStartedUnixSec);
			bindInt64(stmt, 2, r.processingDurationMSec);
			bindOptionalText(stmt, 3, r.softwareVersion);
			bindOptionalInt(stmt, 4, r.solutionType);
			bindOptionalDouble(stmt, 5, r.elevationCutoffDeg);
			id = tryExecuteNonQueryAndGetRowId(stmt);
		});
		return id;
	}

	void StatisticsDatabase::addResidualStatistics(std::int64_t runId,
												   const std::vector<ResidualStatisticsRecord>& records)
	{
		if (records.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ResidualStatistics`(`ProcessingRunID`,`SatelliteSystem`,`ObservationClass`,"
				"`ElevationMinDeg`,`ElevationMaxDeg`,`Count`,`RejectedCount`,`Mean`,`StandardDeviation`,"
				"`Minimum`,`Maximum`) VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11);");
			for (const auto& r : records)
			{
				bindInt64(stmt, 1, runId);
				bindInt64(stmt, 2, static_cast<int>(r.satelliteSystem));
				bindInt64(stmt, 3, r.residualType.type);
				bindDouble(stmt, 4, r.elevationMinDeg);
				bindDouble(stmt, 5, r.elevationMaxDeg);
				bindSummary(stmt, 6, r);
				stepAndReset(stmt);
			}
			sqlite3_finalize(stmt);
		});
	}

	void StatisticsDatabase::addResidualSatelliteStatistics(
		std::int64_t runId, const std::vector<ResidualSatelliteStatisticsRecord>& records)
	{
		if (records.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ResidualSatelliteStatistics`(`ProcessingRunID`,`SV`,`ObservationClass`,`Count`,"
				"`RejectedCount`,`Mean`,`StandardDeviation`,`Minimum`,`Maximum`) VALUES "
				"(?1,(SELECT ID FROM SVS WHERE SVID = ?2 AND SSID = ?3),?4,?5,?6,?7,?8,?9,?10);");
			for (const auto& r : records)
			{
				bindInt64(stmt, 1, runId);
				bindInt64(stmt, 2, r.sat.id);
				bindInt64(stmt, 3, static_cast<int>(r.sat.system));
				bindInt64(stmt, 4, r.residualType.type);
				bindSummary(stmt, 5, r);
				stepAndReset(stmt);
			}
			sqlite3_finalize(stmt);
		});
	}

	void StatisticsDatabase::addSatellites(const std::set<gnsstk::SatID>& svs)
	{
		if (svs.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare("INSERT OR IGNORE INTO `SVS`(`SVID`,`SSID`) VALUES (?1, ?2);");
			for (const auto& sv : svs)
			{
				bindInt64(stmt, 1, sv.id);
				bindInt64(stmt, 2, static_cast<int>(sv.system));
				stepAndReset(stmt);
			}
			sqlite3_finalize(stmt);
		});
	}
} // namespace pod
