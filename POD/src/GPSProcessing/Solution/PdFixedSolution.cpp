#include "PdFixedSolution.h"

namespace pod
{
    PdFixedSolution::PdFixedSolution(GnssDataStorePtr data_ptr) : PdFloatSolution(data_ptr, 50.0)
    {
        equations_->setSlnType(SlnType::PD_Fixed);
    }

    PdFixedSolution::~PdFixedSolution() {}
} // namespace pod
