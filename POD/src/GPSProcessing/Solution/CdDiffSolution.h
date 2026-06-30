#ifndef POD_CDDIFFSOLUTION_H
#define POD_CDDIFFSOLUTION_H

#include "SingleDiffOp.h"
#include "GnssDataStore.hpp"
#include "SingleSolution.h"

namespace pod
{
    
    class CdDiffSolution : public SingleSolution
    {
      public:
        CdDiffSolution(GnssDataStorePtr prt);

        SlnType desiredSlnType() const override
        {
            return SlnType::CodeDiff;
        }

        virtual void process() override;

      protected:
        void updateRequaredObs() override;

        void configureSolver() override;

      private:
        // Compute single differenceses opreator
        SingleDifferenceOp deltaOp_;
        // code smoothers for Ref. receiver obseravtions
        CodeSmoother2 codeSmootherRef_;
    };
} // namespace pod
#endif // !POD_CDDIFFSOLUTION_H
