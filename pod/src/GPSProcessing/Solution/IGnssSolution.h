#ifndef POD_I_GNSS_SOLUTION_H
#define POD_I_GNSS_SOLUTION_H

#include "CommonEnums.h"
#include "GnssDataStore.hpp"
#include "GnssEpochMap.h"

#include <memory>
#include <string>

namespace pod
{
	// Public contract of a GNSS post-processing solution. Contains no state.
	// Implemented by GnssSolution (concrete algorithms) and SolutionFacade (mode-based selection).
	class IGnssSolution
	{
	  public:
		virtual ~IGnssSolution() = default;

		// Binds the solution to the input data and configuration
		virtual IGnssSolution& setConfigData(GnssDataStorePtr dataStore) = 0;

		virtual void process() = 0;

		virtual GnssEpochMap& getData() = 0;
		virtual const GnssEpochMap& getData() const = 0;

		virtual std::string getFileName() const = 0;
		// File name without date suffix
		virtual std::string getBaseFileName() const = 0;

		// Solution type requested by the processing configuration
		virtual SlnType desiredSlnType() const = 0;
	};

	typedef std::unique_ptr<IGnssSolution> IGnssSolutionUPtr;
} // namespace pod

#endif // !POD_I_GNSS_SOLUTION_H
