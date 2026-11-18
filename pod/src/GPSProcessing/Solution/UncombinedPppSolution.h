#pragma once
#include "GnssSolution.h"
namespace pod
{
    class UncombinedPppSolution : public GnssSolution
    {
      public:
        UncombinedPppSolution(GnssDataStorePtr data_ptr);
        UncombinedPppSolution(GnssDataStorePtr data_ptr, double max_sigma);
        virtual ~UncombinedPppSolution();

        virtual void process() override;

      protected:
        virtual void updateRequaredObs() override;

        void configureSolver();
    };
} // namespace pod
