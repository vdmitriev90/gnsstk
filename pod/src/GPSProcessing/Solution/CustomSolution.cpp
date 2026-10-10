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
        case pod::SlnType::Standalone:
            return std::make_unique<SingleSolution>(dataStore);
        case pod::SlnType::CodeDiff:
            return std::make_unique<CdDiffSolution>(dataStore);
        case pod::SlnType::PdFloat:
            return std::make_unique<PdFloatSolution>(dataStore);
        case pod::SlnType::PdFixed:
            return std::make_unique<PdFixedSolution>(dataStore);
        case pod::SlnType::PppFloat:
            if (dataStore->opts.isSpaceborneRcv)
                return std::make_unique<PODSolution>(dataStore);
            else
                // return std::make_unique<PPPSolution>(dataStore);
                return std::make_unique<PppFloatSolution>(dataStore);
            break;
        case pod::SlnType::PppFixed:
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
