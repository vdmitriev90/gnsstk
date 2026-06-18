#ifndef POD_CDDIFFSOLUTION_H
#define POD_CDDIFFSOLUTION_H

#include "DeltaOp.hpp"
#include "GnssDataStore.hpp"
#include "SingleSolution.h"

namespace pod
{
    class CdDiffSolution : public SingleSolution
    {
      public:
        CdDiffSolution(GnssDataStorePtr prt);

        virtual ~CdDiffSolution();

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::CodeDiff;
        }

        virtual void process() override;

      protected:
        virtual void updateRequaredObs() override;

        void configureSolver() override;

      private:
        // Compute single differenceses opreator
        gnsstk::DeltaOp deltaOp_;
        // code smoothers for Ref. receiver obseravtions
        CodeSmoother2 codeSmootherRef_;
    };
} // namespace pod
#endif // !POD_CDDIFFSOLUTION_H
