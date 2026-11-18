#ifndef POD_SOLUTION_FACADE_H
#define POD_SOLUTION_FACADE_H

#include "IGnssSolution.h"

namespace pod
{
	// Single entry point for processing: selects the concrete solution by the processing
	// configuration and delegates all calls to it. Holds no processing state of its own.
	class SolutionFacade : public IGnssSolution
	{
	  public:
		SolutionFacade() = default;

		explicit SolutionFacade(GnssDataStorePtr dataStore);

		// Creates the solution matching the configuration of dataStore
		SolutionFacade& setConfigData(GnssDataStorePtr dataStore) override;

		void process() override;

		GnssEpochMap& getData() override;
		const GnssEpochMap& getData() const override;

		std::string getFileName() const override;
		std::string getBaseFileName() const override;

		SlnType desiredSlnType() const override;

		bool isConfigured() const
		{
			return solution_ != nullptr;
		}

	  private:
		// throws if setConfigData() has not been called
		IGnssSolution& impl();
		const IGnssSolution& impl() const;

		IGnssSolutionUPtr solution_;
	};
} // namespace pod

#endif // !POD_SOLUTION_FACADE_H
