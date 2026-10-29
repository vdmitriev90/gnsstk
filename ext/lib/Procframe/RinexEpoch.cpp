#include "RinexEpoch.h"
#include <typeinfo>
using namespace std;
namespace gnsstk
{
	// Copies make softly removed satellites visible again: this preserves the
	// behaviour of the former currData view rebuilt from the full body.
	RinexEpoch::RinexEpoch(const RinexEpoch& other) : header_(other.header_), body_(other.body_)
	{
		body_.restoreAll();
	}

	RinexEpoch::RinexEpoch(const gnsstk::gnssRinex& gRin) : header_(gRin.header), body_(gRin.body)
	{
	}

	RinexEpoch::RinexEpoch(const sourceEpochRinexHeader& header, SatTable body)
		: header_(header), body_(std::move(body))
	{
	}

	RinexEpoch& RinexEpoch::operator=(const RinexEpoch& other)
	{
		if (this == &other)
		{
			body_.restoreAll();
			return *this;
		}

		header_ = other.header_;
		body_ = other.body_;
		body_.restoreAll();

		return *this;
	}

	RinexEpoch& RinexEpoch::operator=(const IRinex& other)
	{
		if (this == &other)
			return *this;
		const RinexEpoch* ep = dynamic_cast<const RinexEpoch*>(&other);
		if (!ep)
			throw std::bad_cast();

		return operator=(*ep);
	}

	void RinexEpoch::resetCurrData()
	{
		body_.restoreAll();
	}

	gnssRinex RinexEpoch::toGnssRinex() const
	{
		SatTable all(body_);
		all.restoreAll();

		gnssRinex res;
		res.header = header_;
		res.body = all.toSatTypeValueMap();
		return res;
	}

	std::istream& RinexEpoch::read(std::istream& i)
	{
		gnssRinex rinex;
		i >> rinex;
		header_ = rinex.header;
		body_ = SatTable(rinex.body);
		return i;
	}

	std::ostream& RinexEpoch::print(std::ostream& o) const
	{
		return o << toGnssRinex();
	}

	// Hard filters below operate on all stored satellites (including softly
	// removed ones) and leave no hidden rows, as the former implementation
	// that filtered the full body and rebuilt currData.

	RinexEpoch RinexEpoch::extractSatID(const gnsstk::SatID& satellite) const
	{
		return extractSatID(SatIDSet{satellite});
	}

	RinexEpoch RinexEpoch::extractSatID(const int& p, const GpstkSatSystem& s) const
	{
		return extractSatID(SatID(p, s));
	}

	RinexEpoch RinexEpoch::extractSatID(const gnsstk::SatIDSet& satSet) const
	{
		SatTable all(body_);
		all.restoreAll();
		return RinexEpoch(header_, all.extractSatID(satSet));
	}

	RinexEpoch RinexEpoch::extractSatSyst(const gnsstk::SatSystSet& satSet) const
	{
		SatTable all(body_);
		all.restoreAll();
		return RinexEpoch(header_, all.extractSatSyst(satSet));
	}

	RinexEpoch& RinexEpoch::keepOnlySatID(const gnsstk::SatID& sv)
	{
		return keepOnlySatID(SatIDSet{sv});
	}

	RinexEpoch& RinexEpoch::keepOnlySatID(const int& p, const GpstkSatSystem& s)
	{
		return keepOnlySatID(SatID(p, s));
	}

	RinexEpoch& RinexEpoch::keepOnlySatID(const gnsstk::SatIDSet& satSet)
	{
		body_.restoreAll();
		body_.keepOnlySatID(satSet);
		body_.compact();
		return *this;
	}

	RinexEpoch& RinexEpoch::keepOnlySatSystems(GpstkSatSystem satSyst)
	{
		SatSystSet sss{satSyst};
		return keepOnlySatSystems(sss);
	}

	RinexEpoch& RinexEpoch::keepOnlySatSystems(const gnsstk::SatSystSet& satSet)
	{
		body_.restoreAll();
		body_.keepOnlySatSyst(satSet);
		body_.compact();
		return *this;
	}

	RinexEpoch RinexEpoch::extractTypeID(const gnsstk::TypeID& type) const
	{
		return extractTypeID(TypeIDSet{type});
	}

	RinexEpoch RinexEpoch::extractTypeID(const gnsstk::TypeIDSet& typeSet) const
	{
		RinexEpoch res(*this);
		res.body_.keepOnlyTypeID(typeSet);
		// gnssRinex::extractTypeID drops satellites left without data
		for (auto it = res.body_.begin(); it != res.body_.end();)
		{
			if (it->second.empty())
				it = res.body_.erase(it);
			else
				++it;
		}
		res.body_.compact();
		return res;
	}

	RinexEpoch& RinexEpoch::keepOnlyTypeID(const gnsstk::TypeID& type)
	{
		return keepOnlyTypeID(TypeIDSet{type});
	}

	RinexEpoch& RinexEpoch::keepOnlyTypeID(const gnsstk::TypeIDSet& typeSet)
	{
		body_.restoreAll();
		body_.keepOnlyTypeID(typeSet);
		return *this;
	}

	RinexEpoch& RinexEpoch::removeSatID(int id, SatelliteSystem system)
	{
		return removeSatID(SatID(id, system));
	}

	RinexEpoch& RinexEpoch::removeSatID(const SatIDSet& satSet)
	{
		body_.restoreAll();
		body_.removeSatID(satSet);
		body_.compact();
		return (*this);
	}

	RinexEpoch& RinexEpoch::removeSatID(const SatID& sv)
	{
		return removeSatID(SatIDSet{sv});
	}
} // namespace gnsstk
