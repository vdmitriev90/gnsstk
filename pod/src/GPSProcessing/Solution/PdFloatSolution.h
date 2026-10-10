#pragma once
#include "GnssSolution.h"
namespace pod
{
    class PdFloatSolution : public GnssSolution
    {
      public:
        PdFloatSolution(GnssDataStorePtr data_ptr);
        PdFloatSolution(GnssDataStorePtr data_ptr, double max_sigma);
        virtual ~PdFloatSolution();

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::PdFloat;
        }

        virtual void process() override;

      protected:
        virtual void updateRequaredObs() override;

        void configureSolver();
    };
} // namespace pod
