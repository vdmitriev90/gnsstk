#pragma once
#include "PdFloatSolution.h"

namespace pod
{
    class PdFixedSolution : public PdFloatSolution
    {
      public:
        PdFixedSolution(GnssDataStorePtr data_ptr);
        virtual ~PdFixedSolution();

    };
} // namespace pod
