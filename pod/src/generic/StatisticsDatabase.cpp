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
			"CREATE TABLE IF NOT EXISTS `ProcessingData` ("
			"`ID` INTEGER NOT NULL,"
			"`RoverSite` TEXT NOT NULL,"
			"`BaseSite` TEXT,"
			"`SolutionType` INTEGER NOT NULL,"
			"`SatelliteSystemsMask` INTEGER NOT NULL,"
			"`ElevationCutoffDeg` REAL NOT NULL,"
			"`ObservationStartUnixSec` INTEGER NOT NULL,"
			"`EpochNumber` INTEGER NOT NULL,"
			"`ObservationDurationSec` INTEGER NOT NULL,"
			"PRIMARY KEY (`ID`)"
			");"
			"CREATE TABLE IF NOT EXISTS `SolutionStatistics` ("
			"`ProcessingDataID` INTEGER NOT NULL,"
			"`AverageX` REAL NOT NULL,`AverageY` REAL NOT NULL,`AverageZ` REAL NOT NULL,"
			"`HorizontalStdDev` REAL NOT NULL,`VerticalStdDev` REAL NOT NULL,"
			"`AverageSvInUse` REAL NOT NULL,"
			"`CovXX` REAL NOT NULL,`CovYY` REAL NOT NULL,`CovZZ` REAL NOT NULL,"
			"`CovXY` REAL NOT NULL,`CovXZ` REAL NOT NULL,`CovYZ` REAL NOT NULL,"
			"PRIMARY KEY (`ProcessingDataID`),"
			"FOREIGN KEY (`ProcessingDataID`) REFERENCES `ProcessingData` (`ID`)"
			");"
			"CREATE TABLE IF NOT EXISTS `ProcessingTechnicalInfo` ("
			"`ProcessingDataID` INTEGER NOT NULL,"
			"`ProcessingStartedUnixSec` INTEGER NOT NULL,"
			"`ProcessingDurationMSec` INTEGER NOT NULL,"
			"`SoftwareVersion` TEXT,"
			"PRIMARY KEY (`ProcessingDataID`),"
			"FOREIGN KEY (`ProcessingDataID`) REFERENCES `ProcessingData` (`ID`)"
			");"
			"CREATE TABLE IF NOT EXISTS `ResidualStatisticsByElevation` ("
			"`ID` INTEGER NOT NULL,"
			"`ProcessingDataID` INTEGER NOT NULL,"
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
			"FOREIGN KEY (`ProcessingDataID`) REFERENCES `ProcessingData` (`ID`)"
			");"
			"CREATE UNIQUE INDEX IF NOT EXISTS `ResidualStatisticsByElevation_Bin_Unique` "
			"ON `ResidualStatisticsByElevation` (`ProcessingDataID`, `SatelliteSystem`, `ObservationClass`, "
			"`ElevationMinDeg`, `ElevationMaxDeg`);"
			"CREATE TABLE IF NOT EXISTS `ResidualStatisticsBySatSystem` ("
			"`ID` INTEGER NOT NULL,"
			"`ProcessingDataID` INTEGER NOT NULL,"
			"`SatelliteSystem` INTEGER NOT NULL,"
			"`ObservationClass` INTEGER NOT NULL,"
			"`Count` INTEGER NOT NULL,"
			"`RejectedCount` INTEGER NOT NULL DEFAULT 0,"
			"`Mean` REAL,"
			"`StandardDeviation` REAL,"
			"`Minimum` REAL,"
			"`Maximum` REAL,"
			"PRIMARY KEY (`ID`),"
			"FOREIGN KEY (`ProcessingDataID`) REFERENCES `ProcessingData` (`ID`)"
			");"
			"CREATE UNIQUE INDEX IF NOT EXISTS `ResidualStatisticsBySatSystem_Unique` "
			"ON `ResidualStatisticsBySatSystem` (`ProcessingDataID`, `SatelliteSystem`, `ObservationClass`);"
			"CREATE TABLE IF NOT EXISTS `ResidualSatelliteStatistics` ("
			"`ProcessingDataID` INTEGER NOT NULL,"
			"`SV` INTEGER NOT NULL,"
			"`ObservationClass` INTEGER NOT NULL,"
			"`Count` INTEGER NOT NULL,"
			"`RejectedCount` INTEGER NOT NULL DEFAULT 0,"
			"`Mean` REAL,"
			"`StandardDeviation` REAL,"
			"`Minimum` REAL,"
			"`Maximum` REAL,"
			"PRIMARY KEY (`ProcessingDataID`, `SV`, `ObservationClass`),"
			"FOREIGN KEY (`ProcessingDataID`) REFERENCES `ProcessingData` (`ID`),"
			"FOREIGN KEY (`SV`) REFERENCES `SVS` (`ID`)"
			");";
	} // namespace

	const unsigned StatisticsDatabase::SCHEMA_VERSION = 4;
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

	std::int64_t StatisticsDatabase::addRun(const ProcessingDataRecord& data,
											const ProcessingTechnicalInfoRecord& info,
											const std::set<gnsstk::SatID>& svs,											const std::vector<ResidualStatisticsRecord>& elevationResults,
											const std::vector<ResidualStatisticsRecord>& systemResults,
											const std::vector<ResidualSatelliteStatisticsRecord>& satelliteResults,
											const std::optional<SolutionStatisticsRecord>& solution)
	{
		std::int64_t dataId = 0;
		inTransaction([&] {
			dataId = addProcessingData(data, info);
			if (solution)
				addSolutionStatistics(dataId, *solution);
			addSatellites(svs);
			addResidualStatisticsByElevation(dataId, elevationResults);
			addResidualStatisticsBySatSystem(dataId, systemResults);
			addResidualSatelliteStatistics(dataId, satelliteResults);
		});
		return dataId;
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

	void StatisticsDatabase::addSolutionStatistics(std::int64_t processingDataId, const SolutionStatisticsRecord& s)
	{
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `SolutionStatistics`(`ProcessingDataID`,`AverageX`,`AverageY`,`AverageZ`,"
				"`HorizontalStdDev`,`VerticalStdDev`,`AverageSvInUse`,"
				"`CovXX`,`CovYY`,`CovZZ`,`CovXY`,`CovXZ`,`CovYZ`) "
				"VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13);");
			bindInt64(stmt, 1, processingDataId);
			bindDouble(stmt, 2, s.average[0]);
			bindDouble(stmt, 3, s.average[1]);
			bindDouble(stmt, 4, s.average[2]);
			bindDouble(stmt, 5, s.horizontalStdDev);
			bindDouble(stmt, 6, s.verticalStdDev);
			bindDouble(stmt, 7, s.averageSvInUse);
			bindDouble(stmt, 8, s.covariance.xx);
			bindDouble(stmt, 9, s.covariance.yy);
			bindDouble(stmt, 10, s.covariance.zz);
			bindDouble(stmt, 11, s.covariance.xy);
			bindDouble(stmt, 12, s.covariance.xz);
			bindDouble(stmt, 13, s.covariance.yz);
			tryExecuteNonQueryAndGetRowId(stmt);
		});
	}

	std::int64_t StatisticsDatabase::addProcessingData(const ProcessingDataRecord& d,
													   const ProcessingTechnicalInfoRecord& t)
	{
		std::int64_t id = 0;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ProcessingData`(`RoverSite`,`BaseSite`,`SolutionType`,`SatelliteSystemsMask`,"
				"`ElevationCutoffDeg`,`ObservationStartUnixSec`,`EpochNumber`,`ObservationDurationSec`) "
				"VALUES (?1,?2,?3,?4,?5,?6,?7,?8);");
			bindText(stmt, 1, d.roverSite);
			if (d.baseSite)
				bindText(stmt, 2, *d.baseSite);
			else
				checkBind(sqlite3_bind_null(stmt, 2), stmt);
			bindInt64(stmt, 3, d.solutionType);
			bindInt64(stmt, 4, d.satelliteSystemsMask);
			bindDouble(stmt, 5, d.elevationCutoffDeg);
			bindInt64(stmt, 6, d.observationStartUnixSec);
			bindInt64(stmt, 7, d.epochNumber);
			bindInt64(stmt, 8, d.observationDurationSec);
			id = tryExecuteNonQueryAndGetRowId(stmt);

			stmt = prepare(
				"INSERT INTO `ProcessingTechnicalInfo`(`ProcessingDataID`,`ProcessingStartedUnixSec`,"
				"`ProcessingDurationMSec`,`SoftwareVersion`) VALUES (?1,?2,?3,?4);");
			bindInt64(stmt, 1, id);
			bindInt64(stmt, 2, t.processingStartedUnixSec);
			bindInt64(stmt, 3, t.processingDurationMSec);
			if (t.softwareVersion)
				bindText(stmt, 4, *t.softwareVersion);
			else
				checkBind(sqlite3_bind_null(stmt, 4), stmt);
			tryExecuteNonQueryAndGetRowId(stmt);
		});
		return id;
	}

	void StatisticsDatabase::addResidualStatisticsBySatSystem(std::int64_t dataId,
															  const std::vector<ResidualStatisticsRecord>& records)
	{
		if (records.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ResidualStatisticsBySatSystem`(`ProcessingDataID`,`SatelliteSystem`,`ObservationClass`,"
				"`Count`,`RejectedCount`,`Mean`,`StandardDeviation`,`Minimum`,`Maximum`) "
				"VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9);");
			for (const auto& r : records)
			{
				bindInt64(stmt, 1, dataId);
				bindInt64(stmt, 2, static_cast<int>(r.satelliteSystem));
				bindInt64(stmt, 3, r.residualType.type);
				bindSummary(stmt, 4, r);
				stepAndReset(stmt);
			}
			sqlite3_finalize(stmt);
		});
	}

	void StatisticsDatabase::addResidualStatisticsByElevation(std::int64_t dataId,
															  const std::vector<ResidualStatisticsRecord>& records)
	{
		if (records.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ResidualStatisticsByElevation`(`ProcessingDataID`,`SatelliteSystem`,`ObservationClass`,"
				"`ElevationMinDeg`,`ElevationMaxDeg`,`Count`,`RejectedCount`,`Mean`,`StandardDeviation`,"
				"`Minimum`,`Maximum`) VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11);");
			for (const auto& r : records)
			{
				bindInt64(stmt, 1, dataId);
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
		std::int64_t dataId, const std::vector<ResidualSatelliteStatisticsRecord>& records)
	{
		if (records.empty())
			return;
		inTransaction([&] {
			sqlite3_stmt* stmt = prepare(
				"INSERT INTO `ResidualSatelliteStatistics`(`ProcessingDataID`,`SV`,`ObservationClass`,`Count`,"
				"`RejectedCount`,`Mean`,`StandardDeviation`,`Minimum`,`Maximum`) VALUES "
				"(?1,(SELECT ID FROM SVS WHERE SVID = ?2 AND SSID = ?3),?4,?5,?6,?7,?8,?9,?10);");
			for (const auto& r : records)
			{
				bindInt64(stmt, 1, dataId);
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
