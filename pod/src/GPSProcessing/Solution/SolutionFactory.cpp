#include "SolutionFactory.h"

#include "CdDiffSolution.h"
#include "PdFixedSolution.h"
#include "PdFloatSolution.h"
#include "PppFloatSolution.h"
#include "SingleSolution.h"
#include "UncombinedPppSolution.h"

namespace pod
{
	IGnssSolutionUPtr makeSolution(GnssDataStorePtr dataStore)
	{
		const auto& processing = dataStore->config().processing;
		processing.validate();

		switch (processing.solutionMode)
		{
		case SolutionMode::CodeAbsolute:
			return std::make_unique<SingleSolution>(dataStore);

		case SolutionMode::CodeDifferential:
			return std::make_unique<CdDiffSolution>(dataStore);

		case SolutionMode::PhaseDiff:
			if (processing.integerAmbiguityResolution)
				return std::make_unique<PdFixedSolution>(dataStore);
			return std::make_unique<PdFloatSolution>(dataStore);

		case SolutionMode::IonoFreePpp:
			return std::make_unique<PppFloatSolution>(dataStore);

		case SolutionMode::UncombinedPpp:
			return std::make_unique<UncombinedPppSolution>(dataStore);

		case SolutionMode::Undefined:
		default:
			break;
		}

		GNSSTK_THROW(InvalidRequest("Undefined GNSS solution mode"))
	}
} // namespace pod
