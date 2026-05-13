#pragma once
#include "GnssEpochMap.h"
#include "GnssSolution.h"

#include <iostream>

namespace pod
{
    class ComputeStatistic
    {

      public:
        // method to compute correlation matrix from variance-covariance one
        static gnsstk::Matrix<double> corrMatrix(const gnsstk::Matrix<double>& covar);

        ComputeStatistic(SlnType st, gnsstk::TypeIDSet tIDs)
            : slnType(st)
            , types(tIDs)
            , goodEpochs(0) {};
        ~ComputeStatistic() {};

        SlnType getSlnType()
        {
            return slnType;
        }

        gnsstk::TypeIDSet getTypeIDSet()
        {
            return types;
        }

        void compute(const GnssEpochMap& data,
                     gnsstk::Vector<double>& sln,
                     gnsstk::Matrix<double>& cov);

        int goodEpochs;
        int totalEpochs;

      private:
        SlnType slnType;
        gnsstk::TypeIDSet types;
    };
} // namespace pod
