#ifndef POD_PR_SOLVER_H
#define POD_PR_SOLVER_H

#include"TropModel.hpp"
#include"CodeSolverBase.h"
#include"IonoModelStore.hpp"
#include"CodeProcSvData.h"

namespace pod
{
    class CodeSolver : public CodeSolverBase
    {
    public:

        CodeSolver(gnsstk::TropModel &tropo, GnssDataStore_sptr data) :CodeSolverBase(data), tropo(&tropo)
        {};
        virtual ~CodeSolver()
        {
        };
        gnsstk::TropModel *tropo;

        std::string virtual getName() override
        {
            return "CodeSolver";
        };
        virtual double getTropoCorrection(
            const gnsstk::Position &rxPos,
            const gnsstk::Position &svPos,
            const gnsstk::CommonTime &t) const override;

        virtual gnsstk::NeillTropModel initTropoModel(const gnsstk::Position &nominalPos, int DoY) override;
    protected:



    };

}

#endif // !POD_PR_SOLVER_H