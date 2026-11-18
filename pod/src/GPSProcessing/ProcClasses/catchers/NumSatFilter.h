#pragma once
#include "GnssDataStore.hpp"
#include "ProcessingClass.hpp"

namespace pod
{
    class NumSatFilter : public gnsstk::ProcessingClass
    {
      public:
        NumSatFilter(bool fixamb) : fixAmbig_(fixamb) {}
        virtual ~NumSatFilter() {};

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        virtual std::string getClassName(void) const override;

      private:
        int getMinSAtNum() const;

        bool fixAmbig_ = false;
    };

} // namespace pod
