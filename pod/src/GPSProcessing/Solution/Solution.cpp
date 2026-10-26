#include "Solution.h"

#include "ComputeStatistic.h"
#include "ComputeWeightSimple.h"
#include "FsUtils.h"
#include "SolutionDatabase.h"
#include "StatisticsDatabase.h"
#include "TimeUtils.h"

#include <chrono>
#include <cmath>
#include "UnixTime.hpp"

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
         processingStartedUnixSec_ = std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
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

    namespace
    {
        // Position and satellite-count statistics over epochs having the desired solution type.
        struct SolutionSummary
        {
            Vector<double> sln;
            Matrix<double> covar;
            double horStdDev = 0.0;
            double vertStdDev = 0.0;
            int goodEpochs = 0;
            double avgSvInView = 0.0;
            double avgSvInUse = 0.0;
        };

        SolutionSummary computeSolutionSummary(const GnssEpochMap& gnss_data, SlnType slnType)
        {
            SolutionSummary r;
            TypeIDSet typeSet{TypeID::recX, TypeID::recY, TypeID::recZ};
            PositionStatistic rms_st(slnType, typeSet);
            rms_st.compute(gnss_data, r.sln, r.covar);
            r.horStdDev = rms_st.getHorStdDev();
            r.vertStdDev = rms_st.getVertStdDev();

            double inView(0), inUse(0);
            for (const auto& ep : gnss_data)
            {
                if ((SlnType)(int)ep.second.slnData.getValue(TypeID::recSlnType) == slnType)
                {
                    r.goodEpochs++;
                    inView += ep.second.satData.size();
                    inUse += ep.second.slnData.at(TypeID::recUsedSV);
                }
            }
            if (r.goodEpochs > 0)
            {
                r.avgSvInView = inView / r.goodEpochs;
                r.avgSvInUse = inUse / r.goodEpochs;
            }
            return r;
        }
    } // namespace

    void Solution::saveStatistic()
    {
        const auto file_name = solver.getFileName();
        const auto& gnss_data = solver.getData();
        if (gnss_data.empty())
            return;
        fs::path out_path(data->opts.workingDir / file_name / ".txt");

        PostfitResidualStatistics postfit_stat(false);
        postfit_stat.compute(gnss_data);
        postfit_stat.print(std::cout);

        const auto s = computeSolutionSummary(gnss_data, solver.desiredSlnType());
        const auto& sln = s.sln;
        const auto& covar = s.covar;
        std::cout << std::setprecision(3) << "SdtDev Hor: " << s.horStdDev << " Vert: " << s.vertStdDev << std::endl;

        const int summ = s.goodEpochs;
        const double avgSvInView = s.avgSvInView * summ;
        const double avgSvInUse = s.avgSvInUse * summ;

        std::ofstream wrt(out_path.string(), std::ostream::out | std::ostream::app);
        const std::string sep = ",";

        // print time interval
        const std::string fmt = "%04Y-%02m-%02d %02H:%02M:%02S";
        wrt << CivilTime(gnss_data.getInitialTime()).printf(fmt) << sep
            << CivilTime(gnss_data.getFinalTime()).printf(fmt) << sep;

        // XYZ coordinates
        for (auto x : sln)
            wrt << std::fixed << std::setw(13) << std::setprecision(4) << std::setfill(' ') << x << sep;
        wrt << static_cast<int>(solver.desiredSlnType()) << sep;

        // number of good solutions
        wrt << summ << sep << gnss_data.size() << sep << std::setprecision(1) << avgSvInView / summ << sep
            << avgSvInUse / summ << sep;

        // print rms3d
        const double rms3d = sqrt(covar(0, 0) + covar(1, 1) + covar(2, 2));
        wrt << std::fixed << std::scientific << std::setprecision(3) << std::setfill(' ') << rms3d << std::endl;
    }

    void Solution::saveToDb()
    {
        saveSolutionDb();
        saveStatisticsDb();
    }

    void Solution::saveSolutionDb()
    {
        const auto file_name = solver.getFileName();
        auto& gnss_data = solver.getData();
        gnss_data.title = file_name;
        gnss_data.updateMetadata();

        const std::string file_w_ext = file_name + ".db";
        const fs::path solutions_dir = data->opts.workingDir / "solutions";
        fs::create_directories(solutions_dir);
        fs::path db_path(solutions_dir / file_w_ext);

        SolutionDatabase::create(db_path.string(), gnss_data);
    }

    void Solution::saveStatisticsDb()
    {
        auto& gnss_data = solver.getData();
        gnss_data.updateMetadata();

        fs::path db_path(data->opts.workingDir / "statistics.db");

        // compute aggregated residual statistics (no console output)
        PostfitResidualStatistics postfit_stat(false);
        postfit_stat.compute(gnss_data);

        ProcessingDataRecord data_rec;
        std::optional<SolutionStatisticsRecord> solution_rec;
        data_rec.roverSite = data->opts.SiteRover;
        if (isDifferential(solver.desiredSlnType()) && !data->opts.SiteBase.empty())
            data_rec.baseSite = data->opts.SiteBase;
        data_rec.solutionType = static_cast<int>(solver.desiredSlnType());
        data_rec.satelliteSystemsMask = satelliteSystemsMask(gnss_data.svs);
        data_rec.elevationCutoffDeg = data->opts.maskEl;
        data_rec.epochNumber = static_cast<std::int64_t>(gnss_data.size());
        if (!gnss_data.empty())
        {
            const auto t0 = gnss_data.getInitialTime();
            const auto t1 = gnss_data.getFinalTime();
            data_rec.observationStartUnixSec = static_cast<std::int64_t>(gnsstk::UnixTime(t0).tv.tv_sec);
            data_rec.observationDurationSec = static_cast<std::int64_t>(std::llround(t1 - t0));

            const auto s = computeSolutionSummary(gnss_data, solver.desiredSlnType());
            if (s.goodEpochs > 0 && s.sln.size() >= 3 && s.covar.rows() >= 3 && s.covar.cols() >= 3)
            {
                SolutionStatisticsRecord rec;
                rec.average = gnsstk::Triple(s.sln[0], s.sln[1], s.sln[2]);
                rec.horizontalStdDev = s.horStdDev;
                rec.verticalStdDev = s.vertStdDev;
                rec.averageSvInUse = s.avgSvInUse;
                rec.covariance = CovarianceMatrix3{s.covar(0, 0), s.covar(1, 1), s.covar(2, 2),
                                                   s.covar(0, 1), s.covar(0, 2), s.covar(1, 2)};
                solution_rec = rec;
            }
        }

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
    }

    void Solution::chekObs()
    {
        data->checkObservable();
    }
} // namespace pod