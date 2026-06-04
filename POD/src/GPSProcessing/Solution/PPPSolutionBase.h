#ifndef POD_PPP_SOLUTION_BASE_H
#define POD_PPP_SOLUTION_BASE_H

#include "CodeSolverLEO.h"
#include "ComputeDOP.hpp"
#include "ConfDataReader.hpp"
#include "CorrectCodeBiases.hpp"
#include "EOPStore.hpp"
#include "GnssDataStore.hpp"
#include "GnssEpochMap.h"
#include "GnssSolution.h"
#include "SolverPPP.hpp"

#include <memory>

namespace pod
{
    class PPPSolutionBase : public GnssSolution
    {

      public:
        PPPSolutionBase(GnssDataStorePtr data);

        virtual ~PPPSolutionBase();

#pragma region methods

        virtual SlnType desiredSlnType() const override
        {
            return SlnType::PPP_Float;
        }

        virtual std::string fileName() const override
        {
            return opts().SiteRover + "_ppp_float.txt";
        }

      protected:
        virtual void PRProcess();

        virtual bool processCore() = 0;

        virtual void mapSNR(gnsstk::IRinex& value);

        virtual double mapSNR(double value)
        {
            return value;
        };

#pragma endregion

#pragma region Fields

        // pointer to object for code solution
        std::unique_ptr<CodeSolverBase> solverPR_;

#pragma endregion
    };

    // number of decimal places for output
    static constexpr int outputCoordsPrec = 3;
} // namespace pod
#endif // !POD_PPP_SOLUTION_BASE_H