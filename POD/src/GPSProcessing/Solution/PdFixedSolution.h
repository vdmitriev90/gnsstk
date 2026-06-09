#pragma once
#include "PdFloatSolution.h"

namespace pod
{
    class PdFixedSolution : public PdFloatSolution
    {
      public:
        PdFixedSolution(GnssDataStorePtr data_ptr);
        virtual ~PdFixedSolution();

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::PdFixed;
        }
    };
} // namespace pod
