#pragma once
#include "GnssEpochMap.h"
#include "GnssSolution.h"
#include "ProcessingRecords.h"
namespace pod
{
    class PositionStatistic
    {
      public:
        PositionStatistic(SlnType st, gnsstk::TypeIDSet tIDs);

        SlnType getSlnType() const;
        gnsstk::TypeIDSet getTypeIDSet() const;

        void compute(const GnssEpochMap& data, gnsstk::Triple& sln, CovarianceMatrix3& cov);
        double getHorStdDev() const;
        double getVertStdDev() const;

        int goodEpochs = 0;
        int totalEpochs = 0;

      private:
        SlnType slnType;
        gnsstk::TypeIDSet types;
        gnsstk::Matrix<double> covarNEU;
    };

} // namespace pod