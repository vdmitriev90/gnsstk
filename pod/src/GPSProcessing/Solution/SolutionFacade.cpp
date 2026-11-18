#include "SolutionFacade.h"

#include "SolutionFactory.h"

namespace pod
{
	SolutionFacade::SolutionFacade(GnssDataStorePtr dataStore)
	{
		setConfigData(dataStore);
	}

	SolutionFacade& SolutionFacade::setConfigData(GnssDataStorePtr dataStore)
	{
		solution_ = makeSolution(dataStore);
		return *this;
	}

	void SolutionFacade::process()
	{
		impl().process();
	}

	GnssEpochMap& SolutionFacade::getData()
	{
		return impl().getData();
	}

	const GnssEpochMap& SolutionFacade::getData() const
	{
		return impl().getData();
	}

	std::string SolutionFacade::getFileName() const
	{
		return impl().getFileName();
	}

	std::string SolutionFacade::getBaseFileName() const
	{
		return impl().getBaseFileName();
	}

	SlnType SolutionFacade::desiredSlnType() const
	{
		return impl().desiredSlnType();
	}

	IGnssSolution& SolutionFacade::impl()
	{
		if (!solution_)
			GNSSTK_THROW(gnsstk::InvalidRequest("SolutionFacade: solution is not configured, call setConfigData()"))
		return *solution_;
	}

	const IGnssSolution& SolutionFacade::impl() const
	{
		if (!solution_)
			GNSSTK_THROW(gnsstk::InvalidRequest("SolutionFacade: solution is not configured, call setConfigData()"))
		return *solution_;
	}
} // namespace pod
