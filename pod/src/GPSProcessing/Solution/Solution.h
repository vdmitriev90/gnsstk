#ifndef POD_SOLUTION_H
#define POD_SOLUTION_H

#include "BasicFramework.hpp"
#include "SolutionFacade.h"

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
        GnssEpochMap& getData()
        {
            return solver.getData();
        };

        SolutionFacade& getSolver()
        {
            return solver;
        };

      protected:
        GnssDataStorePtr data;

        gnsstk::CommandOptionWithArg confFile;

        SolutionFacade solver;

        // set by process(): wall-clock start (Unix seconds) and steady-clock duration (milliseconds)
        std::int64_t processingStartedUnixSec_ = 0;
        std::int64_t processingDurationMSec_ = 0;
    };
} // namespace pod

#endif // !POD_SOLUTION_H
