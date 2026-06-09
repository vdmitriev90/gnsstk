#ifndef POD_SINGLE_SOLUTION_H
#define POD_SINGLE_SOLUTION_H

#include "CodeSmoother.hpp"
#include "CodeSmoother2.h"
#include "ComputeIonoModel.hpp"
#include "ComputeLinear.hpp"
#include "GnssSolution.h"
#include "KalmanSolverFB.h"
#include "ProcessLinear.h"

namespace pod
{
    class SingleSolution : public GnssSolution
    {

      public:
        SingleSolution(GnssDataStorePtr data_ptr);
        virtual ~SingleSolution() {};

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::Standalone;
        }

        virtual void process() override;

      protected:
        virtual void updateRequaredObs() override;

        virtual void configureSolver();

        void updateNomPos(KalmanSolver& solver);

        // code smoother
        CodeSmoother2 codeSmoother_;

        // Maximum size of filter window, in seconds.
        int codeSmWindowSize_;
    };
} // namespace pod
#endif //! POD_SINGLE_SOLUTION_H
