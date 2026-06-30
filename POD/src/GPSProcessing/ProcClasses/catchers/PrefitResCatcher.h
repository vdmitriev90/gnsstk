#pragma once
#include "ProcessingClass.hpp"
#include "SatObservationBlocks.h"

namespace pod
{
    class PrefitResCatcher : public gnsstk::ProcessingClass
    {
      public:
        PrefitResCatcher()
            : slotProvider_(&PrefitSlotProvider::instance()) {}

        explicit PrefitResCatcher(const PrefitSlotProvider* provider)
            : slotProvider_(provider) {}

        PrefitResCatcher& setSlotProvider(const PrefitSlotProvider* provider)
        {
            slotProvider_ = provider;
            return *this;
        }

        gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        std::string getClassName() const override;

      private:
        const PrefitSlotProvider* slotProvider_;
    };
} // namespace pod
