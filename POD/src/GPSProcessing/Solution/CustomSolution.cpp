#include "CustomSolution.h"

#include "CdDiffSolution.h"
#include "PODSolution.h"
#include "PPPSolution.h"
#include "PPPSolutionBase.h"
#include "PdFixedSolution.h"
#include "PdFloatSolution.h"
#include "PppFloatSolution.h"
#include "SingleSolution.h"

namespace pod
{
    GnssSolutionUPtr CustomSolution::Factory(GnssDataStorePtr dataStore)
    {
        switch (dataStore->opts.slnType)
        {
        case pod::Standalone:
            return std::make_unique<SingleSolution>(dataStore);
        case pod::CODE_DIFF:
            return std::make_unique<CdDiffSolution>(dataStore);
        case pod::PD_Float:
            return std::make_unique<PdFloatSolution>(dataStore);
        case pod::PD_Fixed:
            return std::make_unique<PdFixedSolution>(dataStore);
        case pod::PPP_Float:
            if (dataStore->opts.isSpaceborneRcv)
                return std::make_unique<PODSolution>(dataStore);
            else
                // return std::make_unique<PPPSolution>(dataStore);
                return std::make_unique<PppFloatSolution>(dataStore);
            break;
        case pod::PPP_Fixed:
            break;
        default:
            break;
        }
        std::string sExc = "Processing for Solution type " + slnType2Str[dataStore->opts.slnType]
                           + " is not supported.";

        GNSSTK_THROW(InvalidRequest(sExc))
    }

    CustomSolution::CustomSolution() : GnssSolution(nullptr, .0), ptr(nullptr) {}

    CustomSolution::CustomSolution(GnssDataStorePtr dataStore) : GnssSolution(nullptr, .0)
    {
        ptr = Factory(dataStore);
    }

    CustomSolution::~CustomSolution() {}
} // namespace pod
