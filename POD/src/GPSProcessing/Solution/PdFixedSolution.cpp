#include "PdFixedSolution.h"

namespace pod
{
    PdFixedSolution::PdFixedSolution(GnssDataStore_sptr data_ptr) : PdFloatSolution(data_ptr, 50.0)
    {
        equations_->setSlnType(SlnType::PD_Fixed);
    }

    PdFixedSolution::~PdFixedSolution() {}
} // namespace pod
