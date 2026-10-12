#include "TidalDisplacement.h"

namespace pod
{
	TidalDisplacement::TidalDisplacement(gnsstk::EOPStore& eopStore)
		: eopStore_(eopStore)
	{
	}

	void TidalDisplacement::setOceanLoadingFile(const std::string& fileName)
	{
		ocean_.setFilename(fileName);
		useOcean_ = true;
	}

	void TidalDisplacement::setUseSolid(bool use)
	{
		useSolid_ = use;
	}

	void TidalDisplacement::setUsePole(bool use)
	{
		usePole_ = use;
	}

	void TidalDisplacement::setUseOcean(bool use)
	{
		useOcean_ = use;
	}

	bool TidalDisplacement::getUseSolid() const
	{
		return useSolid_;
	}

	bool TidalDisplacement::getUsePole() const
	{
		return usePole_;
	}

	bool TidalDisplacement::getUseOcean() const
	{
		return useOcean_;
	}

	gnsstk::Triple TidalDisplacement::getDisplacement(const gnsstk::CommonTime& t,
													  const gnsstk::Position& p,
													  const std::string& site)
	{
		gnsstk::Triple total(0.0, 0.0, 0.0);
		if (useSolid_)
			total = total + solid_.getSolidTide(t, p);
		if (usePole_)
		{
			updatePoleXY(t);
			total = total + pole_.getPoleTide(t, p);
		}
		if (useOcean_)
			total = total + ocean_.getOceanLoading(site, t);
		return total;
	}

	void TidalDisplacement::updatePoleXY(const gnsstk::CommonTime& t)
	{
		if (hasEop_ && t == lastEopTime_)
			return;
		auto eop = eopStore_.getEOP(gnsstk::MJD(t).mjd, gnsstk::IERSConvention::IERS2010);
		pole_.setXY(eop.xp, eop.yp);
		lastEopTime_ = t;
		hasEop_ = true;
	}
}
