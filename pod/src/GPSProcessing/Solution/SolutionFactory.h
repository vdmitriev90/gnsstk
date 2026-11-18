#ifndef POD_SOLUTION_FACTORY_H
#define POD_SOLUTION_FACTORY_H

#include "GnssDataStore.hpp"
#include "IGnssSolution.h"

namespace pod
{
	// Creates the solution matching processing.solutionMode of the given configuration.
	// Throws if the configuration is invalid or the solution mode is undefined.
	IGnssSolutionUPtr makeSolution(GnssDataStorePtr dataStore);
} // namespace pod

#endif // !POD_SOLUTION_FACTORY_H
