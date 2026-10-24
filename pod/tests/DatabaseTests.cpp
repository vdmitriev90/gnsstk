// Standalone tests for SolutionDatabase / StatisticsDatabase and residual statistics storage.
// Run: pod_tests (returns non-zero on failure).
#include "ComputeStatistic.h"
#include "SolutionDatabase.h"
#include "StatisticsDatabase.h"

#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <set>
#include <vector>

using namespace pod;
using gnsstk::TypeID;
using gnsstk::SatelliteSystem;
constexpr auto GPS = SatelliteSystem::GPS;
constexpr auto Glonass = SatelliteSystem::Glonass;
namespace fs = std::filesystem;

static int failures = 0;

#define CHECK(cond)                                                                      \
	do                                                                                   \
	{                                                                                    \
		if (!(cond))                                                                     \
		{                                                                                \
			std::cerr << __FILE__ << ":" << __LINE__ << " CHECK failed: " #cond << "\n"; \
			++failures;                                                                  \
		}                                                                                \
	} while (0)

static bool throws(const std::function<void()>& f)
{
	try
	{
		f();
	}
	catch (const std::exception&)
	{
		return true;
	}
	return false;
}

static std::string tempDb(const char* name)
{
	auto p = fs::temp_directory_path() / (std::string("pod_test_") + name + ".db");
	std::error_code ec;
	fs::remove(p, ec);
	return p.string();
}

static long long scalar(sqlite3* db, const std::string& sql)
{
	sqlite3_stmt* st = nullptr;
	long long v = -1;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW)
		v = sqlite3_column_int64(st, 0);
	sqlite3_finalize(st);
	return v;
}

static long long count(sqlite3* db, const char* table)
{
	return scalar(db, std::string("SELECT COUNT(*) FROM ") + table);
}

static bool hasTable(sqlite3* db, const char* t)
{
	return scalar(db, std::string("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='") + t + "'") == 1;
}

static std::int64_t addFile(SolutionDatabase& a)
{
	GnssEpochMap m;
	m.title = "test";
	return a.addSolution(m);
}

using Bins = std::vector<ResidualStatisticsRecord>;
using Sats = std::vector<ResidualSatelliteStatisticsRecord>;

static std::int64_t addRun(StatisticsDatabase& a, const Bins& elev = {}, const Bins& sys = {}, const Sats& sats = {},
						   const std::set<gnsstk::SatID>& svs = {}, const ProcessingRunRecord& run = {})
{
	return a.addRun(run, svs, elev, sys, sats);
}

static ResidualSatelliteStatisticsRecord satRec(int prn, double mean = 0.0)
{
	ResidualSatelliteStatisticsRecord s;
	s.sat = gnsstk::SatID(prn, GPS);
	s.residualType = TypeID::postfitLC;
	s.count_ = 1;
	s.mean_ = mean;
	return s;
}

static ResidualStatisticsRecord bin(gnsstk::SatelliteSystem sys, gnsstk::TypeID t, double lo, double hi)
{
	ResidualStatisticsRecord r;
	r.satelliteSystem = sys;
	r.residualType = t;
	r.elevationMinDeg = lo;
	r.elevationMaxDeg = hi;
	r.count_ = 10;
	r.mean_ = 0.01;
	return r;
}

static void testSolutionDatabase()
{
	auto path = tempDb("sln");
	for (int pass = 0; pass < 2; ++pass) // reopening is idempotent
	{
		SolutionDatabase a(path);
		CHECK(a.userVersion() == SolutionDatabase::SCHEMA_VERSION);
		CHECK(scalar(a.handle(), "PRAGMA foreign_keys") == 1);
		for (const char* t : {"GnssObsFile", "Epochs", "SlnDataItems", "SvDataItems", "SVS", "RinexTypePairs", "TypeIDsByFiles"})
			CHECK(hasTable(a.handle(), t));
		for (const char* t : {"ProcessingRuns", "ResidualStatistics", "ResidualSatelliteStatistics"})
			CHECK(!hasTable(a.handle(), t));
		CHECK(addFile(a) > 0);
		CHECK(count(a.handle(), "GnssObsFile") == pass + 1);
	}
	fs::remove(path);
}

static void testApplicationId()
{
	auto sPath = tempDb("appid_sln");
	auto tPath = tempDb("appid_stat");
	{
		SolutionDatabase s(sPath);
		CHECK(scalar(s.handle(), "PRAGMA application_id") == SolutionDatabase::APPLICATION_ID);
		CHECK(s.userVersion() == SolutionDatabase::SCHEMA_VERSION);
		StatisticsDatabase t(tPath);
		CHECK(scalar(t.handle(), "PRAGMA application_id") == StatisticsDatabase::APPLICATION_ID);
	}
	CHECK(SolutionDatabase::APPLICATION_ID != StatisticsDatabase::APPLICATION_ID);
	CHECK(throws([&] { StatisticsDatabase x(sPath); }));
	CHECK(throws([&] { SolutionDatabase x(tPath); }));
	CHECK(!throws([&] { SolutionDatabase x(sPath); }));
	CHECK(!throws([&] { StatisticsDatabase x(tPath); }));
	fs::remove(sPath);
	fs::remove(tPath);
}

static void testSolutionCreate()
{
	auto path = tempDb("sln_create");
	auto partial = path + ".partial";
	fs::remove(partial);
	GnssEpochMap m;
	m.title = "test";

	// success: final file published, partial gone, exactly one solution
	SolutionDatabase::create(path, m);
	CHECK(fs::exists(path));
	CHECK(!fs::exists(partial));
	{
		SolutionDatabase a(path);
		CHECK(count(a.handle(), "GnssObsFile") == 1);
	}

	// existing final file is rejected and left untouched
	CHECK(throws([&] { SolutionDatabase::create(path, m); }));
	CHECK(!fs::exists(partial));
	{
		SolutionDatabase a(path);
		CHECK(count(a.handle(), "GnssObsFile") == 1);
	}
	fs::remove(path);

	// failure: nothing published
	CHECK(throws([&] {
		SolutionDatabase::create(path, [](SolutionDatabase& db) {
			GnssEpochMap e;
			db.addSolution(e);
			throw std::runtime_error("boom");
		});
	}));
	CHECK(!fs::exists(path));
	CHECK(!fs::exists(partial));
}

static void testSolutionRollback()
{
	auto path = tempDb("sln_rollback");
	{
		SolutionDatabase a(path);
		a.beginTransaction();
		addFile(a);
		a.rollbackTransaction();
		CHECK(count(a.handle(), "GnssObsFile") == 0);
	}
	fs::remove(path);
}

static void testStatisticsDatabase()
{
	auto path = tempDb("stat");
	{
		StatisticsDatabase a(path);
		CHECK(a.userVersion() == StatisticsDatabase::SCHEMA_VERSION);
		CHECK(scalar(a.handle(), "PRAGMA foreign_keys") == 1);
		for (const char* t : {"ProcessingRuns", "ResidualStatistics", "ResidualSatelliteStatistics", "SVS"})
			CHECK(hasTable(a.handle(), t));
		for (const char* t : {"GnssObsFile", "Epochs", "SlnDataItems", "SvDataItems", "RinexTypePairs"})
			CHECK(!hasTable(a.handle(), t));
	}
	fs::remove(path);
}

static void testRunsAndConstraints()
{
	auto path = tempDb("runs");
	{
		StatisticsDatabase a(path);
		const auto runId0 = addRun(a);
		CHECK(runId0 > 0);
		ProcessingRunRecord run;
		run.processingStartedUnixSec = 5000000000LL;  // > 2^32
		run.processingDurationMSec = 30000000000LL;   // > 2^32
		const auto runId = addRun(a, {}, {}, {}, {}, run);
		CHECK(runId > 0);
		CHECK(scalar(a.handle(), "SELECT ProcessingStartedUnixSec FROM ProcessingRuns WHERE ID=" + std::to_string(runId)) == 5000000000LL);
		CHECK(scalar(a.handle(), "SELECT ProcessingDurationMSec FROM ProcessingRuns WHERE ID=" + std::to_string(runId)) == 30000000000LL);
		CHECK(scalar(a.handle(), "SELECT COUNT(*) FROM ProcessingRuns WHERE SoftwareVersion IS NULL AND ElevationCutoffDeg IS NULL") == 2);
	}
	fs::remove(path);
}

static void testStatisticsInsertion()
{
	auto path = tempDb("stats");
	{
		StatisticsDatabase a(path);
		auto b1 = bin(GPS, TypeID::postfitPC, 0, 10);
		auto b2 = bin(GPS, TypeID::postfitPC, 10, 20);
		auto s = satRec(5, std::nan(""));
		s.count_ = 3;
		addRun(a, {b1, b2}, {}, {s}, {gnsstk::SatID(5, GPS)});
		CHECK(count(a.handle(), "ProcessingRuns") == 1);
		CHECK(count(a.handle(), "ResidualStatistics") == 2);
		CHECK(scalar(a.handle(), "SELECT COUNT(*) FROM ResidualStatistics WHERE StandardDeviation IS NULL AND Minimum IS NULL AND Maximum IS NULL") == 2);
		CHECK(count(a.handle(), "ResidualSatelliteStatistics") == 1);
		CHECK(scalar(a.handle(), "SELECT COUNT(*) FROM ResidualSatelliteStatistics WHERE Mean IS NULL") == 1);

		// duplicate bin within a run -> whole run rejected atomically
		CHECK(throws([&] { addRun(a, {b1, b1}); }));
		// duplicate satellite record -> whole run rejected atomically
		CHECK(throws([&] { addRun(a, {b1}, {}, {s, s}, {gnsstk::SatID(5, GPS)}); }));
		// satellite not registered via svs -> FK/NOT NULL failure, whole run rejected
		CHECK(throws([&] { addRun(a, {b1}, {}, {satRec(7)}); }));

		CHECK(count(a.handle(), "ProcessingRuns") == 1);
		CHECK(count(a.handle(), "ResidualStatistics") == 2);
		CHECK(count(a.handle(), "ResidualSatelliteStatistics") == 1);
	}
	fs::remove(path);
}

static void testSystemStatisticsInsertion()
{
	auto path = tempDb("sysstats");
	{
		StatisticsDatabase a(path);
		const double maxDeg = PostfitResidualStatistics::ELEV_BIN_MAX_DEG;
		// elevation bins and the all-elevation (0-90) system record are stored side by side
		addRun(a, {bin(GPS, TypeID::postfitPC, 0, 10), bin(GPS, TypeID::postfitPC, 80, 90)},
			   {bin(GPS, TypeID::postfitPC, 0, maxDeg), bin(Glonass, TypeID::postfitLC, 0, maxDeg)});
		CHECK(count(a.handle(), "ResidualStatistics") == 4);
		CHECK(scalar(a.handle(), "SELECT COUNT(*) FROM ResidualStatistics WHERE ElevationMinDeg=0 AND ElevationMaxDeg=90") == 2);
		CHECK(throws([&] { addRun(a, {}, {bin(GPS, TypeID::postfitPC, 0, maxDeg), bin(GPS, TypeID::postfitPC, 0, maxDeg)}); })); // unique
		CHECK(count(a.handle(), "ProcessingRuns") == 1);
	}
	fs::remove(path);
}

static void testStatisticsRollback()
{
	auto path = tempDb("stat_rollback");
	{
		StatisticsDatabase a(path);
		a.beginTransaction();
		addRun(a, {bin(GPS, TypeID::postfitPC, 0, 10)});
		a.rollbackTransaction();

		CHECK(count(a.handle(), "ProcessingRuns") == 0);
		CHECK(count(a.handle(), "ResidualStatistics") == 0);
	}
	fs::remove(path);
}

static void testStatisticsAppendAcrossRuns()
{
	auto path = tempDb("stat_append");
	for (int pass = 1; pass <= 2; ++pass)
	{
		StatisticsDatabase st(path);
		addRun(st, {bin(GPS, TypeID::postfitPC, 0, 10)}, {}, {satRec(5)}, {gnsstk::SatID(5, GPS)});

		CHECK(count(st.handle(), "ProcessingRuns") == pass);
		CHECK(count(st.handle(), "ResidualStatistics") == pass);
		CHECK(count(st.handle(), "ResidualSatelliteStatistics") == pass);
		CHECK(count(st.handle(), "SVS") == 1);
	}
	fs::remove(path);
}

static void testNewerSchemaRejected()
{
	auto path = tempDb("newer");
	{
		sqlite3* db = nullptr;
		sqlite3_open(path.c_str(), &db);
		sqlite3_exec(db, "PRAGMA user_version = 99;", nullptr, nullptr, nullptr);
		sqlite3_close(db);
	}
	CHECK(throws([&] { SolutionDatabase a(path); }));
	CHECK(throws([&] { StatisticsDatabase a(path); }));
	fs::remove(path);
}

static void testSummaryStatistics()
{
	ResidualSummary s;
	PostfitResidualStatistics::summarize({1.0, 2.0, 3.0, 4.0, 100.0}, s);
	CHECK(s.count_ == 5);
	CHECK(std::fabs(*s.mean_ - 22.0) < 1e-12);
	CHECK(*s.minimum_ == 1.0 && *s.maximum_ == 100.0);
	// sample standard deviation
	CHECK(std::fabs(*s.stdDev_ - std::sqrt((441.0 + 400.0 + 361.0 + 324.0 + 6084.0) / 4.0)) < 1e-9);

	PostfitResidualStatistics::summarize({2.5}, s);
	CHECK(s.count_ == 1 && !s.stdDev_ && *s.minimum_ == 2.5);
	PostfitResidualStatistics::summarize({}, s);
	CHECK(s.count_ == 0 && !s.mean_ && !s.minimum_);
}

int main()
{
	testSolutionDatabase();
	testApplicationId();
	testSolutionCreate();
	testSolutionRollback();
	testStatisticsDatabase();
	testRunsAndConstraints();
	testStatisticsInsertion();
	testSystemStatisticsInsertion();
	testStatisticsRollback();
	testStatisticsAppendAcrossRuns();
	testNewerSchemaRejected();
	testSummaryStatistics();

	if (failures)
		std::cerr << failures << " check(s) failed\n";
	else
		std::cout << "All tests passed\n";
	return failures ? 1 : 0;
}
