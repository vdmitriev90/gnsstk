#ifndef POD_PR_SOLVER_H
#define POD_PR_SOLVER_H

#include "CodeProcSvData.h"
#include "CodeSolverBase.h"
#include "IonoModelStore.hpp"
#include "TropModel.hpp"

namespace pod
{
    class CodeSolver : public CodeSolverBase
    {
      public:
        CodeSolver(gnsstk::TropModel& tropo, GnssDataStorePtr data)
            : CodeSolverBase(data)
            , tropo(&tropo) {};
        virtual ~CodeSolver() {};
        gnsstk::TropModel* tropo;

        std::string virtual getName() override
        {
            return "CodeSolver";
        };
        virtual double getTropoCorrection(const gnsstk::Position& rxPos,
                                          const gnsstk::Position& svPos,
                                          const gnsstk::CommonTime& t) const override;

        virtual gnsstk::NeillTropModel initTropoModel(const gnsstk::Position& nominalPos,
                                                      int DoY) override;

      protected:
    };

} // namespace pod

#endif // !POD_PR_SOLVER_H