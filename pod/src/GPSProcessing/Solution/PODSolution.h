#ifndef POD_POD_SOLUTION_H
#define POD_POD_SOLUTION_H

#include "PPPSolutionBase.h"

namespace pod
{
    class PODSolution : public PPPSolutionBase
    {

      public:
        PODSolution(GnssDataStorePtr confData);

        virtual ~PODSolution() {};

        void process() override;

      protected:
        virtual bool processCore() override;

        virtual double mapSNR(double value) override;

        virtual void updateRequaredObs() override;

        void printSolution(std::ofstream& of,
                           const gnsstk::SolverLMS& solver,
                           const gnsstk::CommonTime& time,
                           GnssEpoch& gEpoch);
    };
} // namespace pod

#endif // !POD_POD_SOLUTION_H