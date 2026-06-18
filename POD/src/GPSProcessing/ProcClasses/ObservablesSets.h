#pragma once
#include "RequireObservables.hpp"
#include "SatID.hpp"

namespace pod
{
    /** Builder for gnsstk::RequireObservables.
     *
     *  Configures per-system TypeID requirements based on the active satellite
     *  systems and whether C1 or P1 is used as the L1 code observable.
     *
     *  Usage:
     *  @code
     *    requireObs_ = RequireObservablesBuilder(opts().systems, useC1).build();
     *  @endcode
     */
    class RequireObservablesBuilder
    {
      public:
        RequireObservablesBuilder(const gnsstk::SatSystSet& systems, bool useC1,
                                   bool keepOnlyRequiredTypes  = true);

        gnsstk::RequireObservables build() const;

      private:
        const gnsstk::SatSystSet& systems_;
        bool useGpsGloC1_;
        bool keepOnlyRequiredTypes_;
    };

} // namespace pod
