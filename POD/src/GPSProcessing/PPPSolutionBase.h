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
        PPPSolutionBase(GnssDataStore_sptr data);

      public:
        virtual ~PPPSolutionBase();

#pragma region methods

      public:
        virtual SlnType desiredSlnType() const override
        {
            return SlnType::PPP_Float;
        }

      public:
        virtual std::string fileName() const override
        {
            return opts().SiteRover + "_ppp_float.txt";
        }

      protected:
        virtual void PRProcess();

      protected:
        virtual bool processCore() = 0;

      protected:
        virtual void mapSNR(gnsstk::IRinex& value);

      protected:
        virtual double mapSNR(double value)
        {
            return value;
        };

#pragma endregion

#pragma region Fields

        // pointer to object for code solution
      protected:
        std::unique_ptr<CodeSolverBase> solverPR;

#pragma endregion
    };
} // namespace pod
#endif // !POD_PPP_SOLUTION_BASE_H