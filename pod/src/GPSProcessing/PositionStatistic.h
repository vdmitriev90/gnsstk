#pragma once
#include "GnssEpochMap.h"
#include "GnssSolution.h"
#include "ProcessingRecords.h"

#include <cstddef>

namespace pod
{
    struct PositionStatisticResult
    {
        gnsstk::Triple averagePosition{0.0, 0.0, 0.0};

        CovarianceMatrix3 covarianceEcef{};

        double horizontalStdDev = 0.0;
        double verticalStdDev = 0.0;

        int totalEpochs = 0;
        int fixEpochs = 0;
        int outlierEpochs = 0;

        // epochs having the requested solution type
        double avgSvInUse = 0.0;

        bool valid = false;
    };

    class PositionStatistic
    {
      public:
        explicit PositionStatistic(SlnType solutionType);

        PositionStatisticResult compute(GnssEpochMap& data) const;

        SlnType getSlnType() const noexcept;

      private:
        SlnType solutionType_;
    };

    PositionStatisticResult computeSolutionSummary(GnssEpochMap& gnss_data, SlnType slnType);
} // namespace pod
