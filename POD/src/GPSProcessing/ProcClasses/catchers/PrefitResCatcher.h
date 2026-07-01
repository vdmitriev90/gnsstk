#pragma once
#include "CommonEnums.h"
#include "ProcessingClass.hpp"
#include "SatObservationBlocks.h"

namespace pod
{
    class PrefitResCatcher : public gnsstk::ProcessingClass
    {
      public:
        explicit PrefitResCatcher(const std::vector<ObsSlot>& slots) : slots_(slots) {}

        gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        std::string getClassName() const override;

      private:
        const std::vector<ObsSlot> slots_;
    };
} // namespace pod
