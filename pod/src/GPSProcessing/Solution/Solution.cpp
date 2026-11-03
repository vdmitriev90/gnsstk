#include "Solution.h"

#include "ComputeWeightSimple.h"
#include "FsUtils.h"
#include "PositionStatistic.h"
#include "ResidualsStatistic.h"
#include "SolutionDatabase.h"
#include "StatisticsDatabase.h"
#include "TimeUtils.h"
#include "UnixTime.hpp"

#include <chrono>
#include <cmath>

using namespace gnsstk;
namespace pod
{
    namespace fs = std::filesystem;

    Solution::Solution(const char* path)
        : BasicFramework("pod", "discr")
        // Option initialization. "true" means a mandatory option
        , confFile(CommandOption::stdType,
                   'c',
                   "conffile",
                   " [-c|--conffile]    Name of configuration file ('config.txt' by default).",
                   false)
    {
        data.reset(new GnssDataStore(confReader));
        data->LoadData(path);

        solver.setConfigData(data);
    }

    void Solution::process()
    {
        processingStartedUnixSec_ =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        const StopTimer stopTimer(processingDurationMSec_);

        try
        {
            solver.process();
        }
        catch (gnsstk::Exception& e)
        {
            std::cerr << "An exception has occured. Processing stopped." << std::endl;
            std::cerr << e.getLocation() << std::endl;
            std::cerr << e.getText() << std::endl;

            GNSSTK_RETHROW(e);
        }
    }

    void printStatistic(const PositionStatisticResult& slnSumm, const PostfitResidualStatistics& residStat)
    {
        residStat.print(std::cout, false);

        const auto& sln = slnSumm.averagePosition;
        std::cout << std::setprecision(3) << "SdtDev Hor: " << slnSumm.horizontalStdDev
                  << " Vert: " << slnSumm.verticalStdDev << " TotalEpohs " << slnSumm.totalEpochs << " FixEpochs "
                  << slnSumm.fixEpochs << " Outliers " << slnSumm.outlierEpochs << std::endl;
    }

    void Solution::saveToDb()
    {
        saveSolutionDb();
        saveStatisticsDb();
    }

    namespace
    {
        // Builds a unique .db path: <base>_<first epoch>[_001, _002, ...].db
        fs::path makeUniqueDbPath(const fs::path& dir, std::string base, const GnssEpochMap& gnss_data)
        {
            if (!gnss_data.empty())
                base += "_" + CivilTime(gnss_data.getInitialTime()).printf("%02y%02m%02d%02H%02M%02S");

            fs::path db_path = dir / (base + ".db");
            for (int i = 1; fs::exists(db_path); ++i)
            {
                std::ostringstream suffix;
                suffix << "_" << std::setw(3) << std::setfill('0') << i;
                db_path = dir / (base + suffix.str() + ".db");
            }
            return db_path;
        }
    } // namespace

    void Solution::saveSolutionDb()
    {
        auto& gnss_data = solver.getData();

        const fs::path solutions_dir = data->opts.workingDir / "solutions";
        fs::create_directories(solutions_dir);

        const fs::path db_path = makeUniqueDbPath(solutions_dir, solver.getBaseFileName(), gnss_data);

        gnss_data.title = db_path.stem().string();
        gnss_data.updateMetadata();

        SolutionDatabase::create(db_path.string(), gnss_data);
    }

    void Solution::saveStatisticsDb()
    {
        auto& gnss_data = solver.getData();
        gnss_data.updateMetadata();

        if (gnss_data.empty())
            return;

        fs::path db_path(data->opts.workingDir / "statistics.db");

        std::optional<SolutionStatisticsRecord> solution_rec;
        const auto sln_summ = computeSolutionSummary(gnss_data, solver.desiredSlnType());
        if (sln_summ.valid)
        {
            SolutionStatisticsRecord rec;
            rec.average = sln_summ.averagePosition;
            rec.horizontalStdDev = sln_summ.horizontalStdDev;
            rec.verticalStdDev = sln_summ.verticalStdDev;
            rec.averageSvInUse = sln_summ.avgSvInUse;
            rec.covariance = sln_summ.covarianceEcef;
            rec.totalEpochs = sln_summ.totalEpochs;
            rec.fixEpochs = sln_summ.fixEpochs;
            rec.outlierEpochs = sln_summ.outlierEpochs;
            solution_rec = rec;
        }

        // compute aggregated residual statistics
        PostfitResidualStatistics postfit_stat(true);
        postfit_stat.compute(gnss_data);

        ProcessingDataRecord data_rec;
        data_rec.roverSite = data->opts.SiteRover;
        if (isDifferential(solver.desiredSlnType()) && !data->opts.SiteBase.empty())
            data_rec.baseSite = data->opts.SiteBase;
        data_rec.solutionType = static_cast<int>(solver.desiredSlnType());
        data_rec.satelliteSystemsMask = satelliteSystemsMask(gnss_data.svs);
        data_rec.elevationCutoffDeg = data->opts.maskEl;

        const auto t0 = gnss_data.getInitialTime();
        const auto t1 = gnss_data.getFinalTime();
        data_rec.observationStartUnixSec = static_cast<std::int64_t>(gnsstk::UnixTime(t0).tv.tv_sec);
        data_rec.observationDurationSec = static_cast<std::int64_t>(std::llround(t1 - t0));

        ProcessingTechnicalInfoRecord info;
        info.processingStartedUnixSec = processingStartedUnixSec_;
        info.processingDurationMSec = processingDurationMSec_;

        StatisticsDatabase db(db_path.string());
        db.addRun(data_rec,
                  info,
                  gnss_data.svs,
                  postfit_stat.elevationResults(),
                  postfit_stat.systemResults(),
                  postfit_stat.satelliteResults(),
                  solution_rec);

        printStatistic(sln_summ, postfit_stat);
    }

    void Solution::chekObs()
    {
        data->checkObservable();
    }
} // namespace pod