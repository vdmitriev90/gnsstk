#pragma once
#include "ComputeTropModel.hpp"
#include "GnssSolution.h"
#include "NeillTropModel.hpp"

namespace pod
{
    class PppFloatSolution : public GnssSolution
    {
      public:
        PppFloatSolution(GnssDataStorePtr data_ptr);
        PppFloatSolution(GnssDataStorePtr data_ptr, double max_sigma);
        virtual ~PppFloatSolution() {};

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::PppFloat;
        }

        virtual void process() override;

      protected:
        virtual void updateRequaredObs() override;

        void storeReceiverParams(const KalmanSolver& solver,
                                 const FilterParameter& param,
                                 GnssEpoch& ep) const override;

        void configureSolver();

      private:
        gnsstk::NeillTropModel tropoRover_;
        gnsstk::ComputeTropModel computeTropoRover_;
    };
} // namespace pod
