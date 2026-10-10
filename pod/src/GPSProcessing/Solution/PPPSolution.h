#ifndef POD_PPP_SOLUTION_H
#define POD_PPP_SOLUTION_H

#include "ConfDataReader.hpp"
#include "NeillTropModel.hpp"
#include "PPPSolutionBase.h"

namespace pod
{
    class PPPSolution : public PPPSolutionBase
    {
      public:
        PPPSolution(GnssDataStorePtr confReader);
        virtual ~PPPSolution() {};

        void process() override;

      protected:
        virtual bool processCore() override;
        gnsstk::NeillTropModel tropModel;
        virtual void updateRequaredObs() override;

      protected:
        void printSolution(std::ofstream& of,
                           const gnsstk::SolverLMS& solver,
                           const gnsstk::CommonTime& time,
                           GnssEpoch& gEpoch);
    };
} // namespace pod

#endif // !POD_PPP_SOLUTION_H