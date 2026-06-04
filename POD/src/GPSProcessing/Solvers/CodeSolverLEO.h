#ifndef POD_PR_SOLVER_LEO_H
#define POD_PR_SOLVER_LEO_H

#include "CodeProcSvData.h"
#include "CodeSolverBase.h"
#include "IonoModelStore.hpp"
#include "NeillTropModel.hpp"

namespace pod
{
    class CodeSolverLEO : public CodeSolverBase
    {
      public:
        CodeSolverLEO(GnssDataStorePtr data) : CodeSolverBase(data) {};
        virtual ~CodeSolverLEO() {};

        std::string virtual getName() override
        {
            return "CodeSolverLEO";
        };

        virtual gnsstk::NeillTropModel initTropoModel(const gnsstk::Position& nominalPos,
                                                      int DoY) override
        {
            return NULL;
        };
        virtual double getTropoCorrection(const gnsstk::Position& rxPos,
                                          const gnsstk::Position& svPos,
                                          const gnsstk::CommonTime& t) const override
        {
            return 0;
        };
    };
} // namespace pod
#endif // !POD_PR_SOLVER_LEO_H