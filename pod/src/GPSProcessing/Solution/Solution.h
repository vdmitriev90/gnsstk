#ifndef POD_SOLUTION_H
#define POD_SOLUTION_H

#include "BasicFramework.hpp"
#include "ConfDataReader.hpp"
#include "CustomSolution.h"

#include <cstdint>

namespace pod
{
    struct GnssEpochMap;
    class Solution : public gnsstk::BasicFramework
    {

      public:
        Solution(const char* path);
        virtual ~Solution() {}
        virtual void process();
        void chekObs();
        void saveToDb();
        void saveSolutionDb();
        void saveStatisticsDb();
        void saveStatistic();
        GnssEpochMap& getData()
        {
            return solver.getData();
        };

        CustomSolution& getSolver()
        {
            return solver;
        };

      protected:
        GnssDataStorePtr data;

        gnsstk::CommandOptionWithArg confFile;

        // Configuration file reader
        gnsstk::ConfDataReader confReader;

        CustomSolution solver;

        // set by process(): wall-clock start (Unix seconds) and steady-clock duration (milliseconds)
        std::int64_t processingStartedUnixSec_ = 0;
        std::int64_t processingDurationMSec_ = 0;
    };
} // namespace pod

#endif // !POD_SOLUTION_H
