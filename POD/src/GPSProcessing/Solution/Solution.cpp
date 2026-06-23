#include "Solution.h"

#include "ComputeStatistic.h"
#include "FsUtils.h"

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
        ComputeStatistic st(solver.desiredSlnType(), typeSet);

        Vector<double> sln;
        Matrix<double> covar;
        st.compute(gnss_data, sln, covar);
        int summ(0);
        double avgSvInView(0);
        double avgSvInUse(0);

        // calculate number of desired sln types
        for_each(gnss_data.begin(), gnss_data.end(), [&](const auto& ep) {
            if ((SlnType)(int)ep.second.slnData.getValue(TypeID::recSlnType)
                == solver.desiredSlnType())
            {
                summ++;
                avgSvInView += ep.second.satData.size();
                avgSvInUse += ep.second.slnData.at(TypeID::recUsedSV);
            }
        });

        // calculate 3D RMS
        double rms3d = sqrt(covar(0, 0) + covar(1, 1) + covar(2, 2));

        std::ofstream wrt(out_path.string(), std::ostream::out | std::ostream::app);
        const std::string sep = ",";
        
        // print time interval
        const std::string fmt = "%04Y-%02m-%02d %02H:%02M:%02S";
        wrt << CivilTime(gnss_data.getInitialTime()).printf(fmt) << sep
            << CivilTime(gnss_data.getFinalTime()).printf(fmt) << sep;

        // XYZ coordinates
        for (auto x : sln)
            wrt << std::fixed << std::setw(13) << std::setprecision(4) << std::setfill(' ') << x
                << sep;
        wrt << static_cast<int>(solver.desiredSlnType()) << sep;

        // number of good solutions
        wrt << summ << sep << gnss_data.size() << sep << std::setprecision(1) << avgSvInView / summ
            << sep << avgSvInUse / summ << sep;

        // print rms3d
        wrt << std::fixed << std::scientific << std::setprecision(3) << std::setfill(' ') << rms3d
            << sep;

        // compute corr. matrix
        const auto corr = ComputeStatistic::corrMatrix(covar);
        // print std.dev.
        for (size_t i = 0; i < covar.rows(); i++)
            wrt << std::fixed << std::scientific << std::setprecision(4) << std::setfill(' ')
                << sqrt(covar(i, i)) << sep;

        // print correlation coeff.: xx,xz,yz
        for (size_t i = 0; i < corr.rows(); i++)
            for (size_t j = 0; j < i; j++)
                wrt << std::fixed << std::scientific << std::setprecision(4) << std::setfill(' ')
                    << corr(i, j) << sep;

        wrt << std::endl;
    }

    void Solution::saveToDb()
    {
        const auto file_name = solver.getFileName();
        auto& gnss_data = solver.getData();
        gnss_data.title = file_name;
        gnss_data.updateMetadata();

        const std::string file_w_ext = file_name + ".db";
        fs::path db_path(data->opts.workingDir / file_w_ext);

        // delete curtrent solution database file, if exists
        // string cmd = "del \"" + db_path.string() + "\"";
        // system(cmd.c_str());

        // insert solution data into DB
        SQLiteAdapter db(db_path.string());
        db.addNewFile(gnss_data);
    }

    void Solution::chekObs()
    {
        data->checkObservable();
    }
} // namespace pod