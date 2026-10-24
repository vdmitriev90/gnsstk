#include "Solution.h"

#include "ComputeStatistic.h"
#include "ComputeWeightSimple.h"
#include "FsUtils.h"
#include "SolutionDatabase.h"
#include "StatisticsDatabase.h"
#include "TimeUtils.h"

#include <chrono>

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

    void Solution::saveStatistic()
    {
        const auto file_name = solver.getFileName();
        const auto& gnss_data = solver.getData();
        if (gnss_data.empty())
            return;
        fs::path out_path(data->opts.workingDir / file_name / ".txt");

        TypeIDSet typeSet{TypeID::recX, TypeID::recY, TypeID::recZ};
        PositionStatistic rms_st(solver.desiredSlnType(), typeSet);

        PostfitResidualStatistics postfit_stat(false);
        postfit_stat.compute(gnss_data);
        postfit_stat.print(std::cout);

        Vector<double> sln;
        Matrix<double> covar;
        rms_st.compute(gnss_data, sln, covar);
        std::cout << std::setprecision(3) << "SdtDev Hor: " << rms_st.getHorStdDev() << " Vert: "
                  << rms_st.getVertStdDev() << std::endl;

        int summ(0);
        double avgSvInView(0);
        double avgSvInUse(0);

        // calculate number of desired sln types
        for_each(gnss_data.begin(), gnss_data.end(), [&](const auto& ep) {
            if ((SlnType)(int)ep.second.slnData.getValue(TypeID::recSlnType) == solver.desiredSlnType())
            {
                summ++;
                avgSvInView += ep.second.satData.size();
                avgSvInUse += ep.second.slnData.at(TypeID::recUsedSV);
            }
        });

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
        const fs::path solutions_dir = data->opts.workingDir / data->opts.projectName / "solutions";
        fs::create_directories(solutions_dir);
        fs::path db_path(solutions_dir / file_w_ext);

        SolutionDatabase::create(db_path.string(), gnss_data);
    }

    void Solution::saveStatisticsDb()
    {
        auto& gnss_data = solver.getData();
        gnss_data.updateMetadata();

        const fs::path project_dir = data->opts.workingDir / data->opts.projectName;
        fs::create_directories(project_dir);
        fs::path db_path(project_dir / "statistics.db");

        // compute aggregated residual statistics (no console output)
        PostfitResidualStatistics postfit_stat(false);
        postfit_stat.compute(gnss_data);

        ProcessingRunRecord run;
        run.processingStartedUnixSec = processingStartedUnixSec_;
        run.processingDurationMSec = processingDurationMSec_;
        run.solutionType = static_cast<int>(solver.desiredSlnType());
        run.elevationCutoffDeg = data->opts.maskEl;

        StatisticsDatabase db(db_path.string());
        db.addRun(run,
                  gnss_data.svs,
                  postfit_stat.elevationResults(),
                  postfit_stat.systemResults(),
                  postfit_stat.satelliteResults());
    }

    void Solution::chekObs()
    {
        data->checkObservable();
    }
} // namespace pod