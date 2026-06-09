#include "PdFixedSolution.h"

namespace pod
{
    PdFixedSolution::PdFixedSolution(GnssDataStorePtr data_ptr) : PdFloatSolution(data_ptr, 50.0)
    {
        equations_->setSlnType(SlnType::PdFixed);
    }

    PdFixedSolution::~PdFixedSolution() {}
} // namespace pod
