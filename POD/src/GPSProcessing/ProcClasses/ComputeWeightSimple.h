#pragma once
#include "ProcessingClass.hpp"
namespace pod
{
    class ComputeWeightSimple : public gnsstk::ProcessingClass
    {
      public:
        static const double elev0;

        static const double sin0;

        ComputeWeightSimple(double glnFactor = 1) : glnSigmaFactor(glnFactor) {}

        virtual ~ComputeWeightSimple();

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        gnsstk::SatTypePtrMap& Process(gnsstk::SatTypePtrMap& gData);

        virtual std::string getClassName(void) const override
        {
            return "ComputeWeightSimple";
        }

      private:
        double glnSigmaFactor;
    };
} // namespace pod
