#include "RinexEpoch.h"
#include <typeinfo>
using namespace std;
namespace gnsstk
{
    RinexEpoch::RinexEpoch(const RinexEpoch& other) : rinex(other.rinex)
    {
        resetCurrData();
    }

    RinexEpoch::RinexEpoch(const gnsstk::gnssRinex& gRin) : rinex(gRin)
    {
        resetCurrData();
    }

    RinexEpoch& RinexEpoch::operator=(const RinexEpoch& other)
    {
        this->rinex = other.rinex;
        resetCurrData();

        return *this;
    }

    RinexEpoch& RinexEpoch::operator=(const IRinex& other)
    {
        if (this == &other)
            return *this;
        const RinexEpoch* ep = dynamic_cast<const RinexEpoch*>(&other);
        if (!ep)
            throw std::bad_cast();

        this->rinex = ep->rinex;
        resetCurrData();

        return *this;
    }

    void RinexEpoch::resetCurrData()
    {
        currData = SatTypePtrMap(rinex.body);
    }

    std::istream& RinexEpoch::read(std::istream& i)
    {
        i >> rinex;
        resetCurrData();
        return i;
    }

    RinexEpoch RinexEpoch::extractSatID(const gnsstk::SatID& satellite) const
    {
        return RinexEpoch(rinex.extractSatID(satellite));
    }

    RinexEpoch RinexEpoch::extractSatID(const int& p, const GpstkSatSystem& s) const
    {
        return RinexEpoch(rinex.extractSatID(p, s));
    }

    RinexEpoch RinexEpoch::extractSatID(const gnsstk::SatIDSet& satSet) const
    {
        return RinexEpoch(rinex.extractSatID(satSet));
    }

    RinexEpoch RinexEpoch::extractSatSyst(const gnsstk::SatSystSet& satSet) const
    {
        return RinexEpoch(rinex.extractSatSyst(satSet));
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
        rinex.keepOnlySatID(satSet);
        resetCurrData();
        return *this;
    }

    RinexEpoch& RinexEpoch::keepOnlySatSystems(GpstkSatSystem satSyst)
    {
        SatSystSet sss{satSyst};
        return keepOnlySatSystems(sss);
    }

    RinexEpoch& RinexEpoch::keepOnlySatSystems(const gnsstk::SatSystSet& satSet)
    {
        rinex.keepOnlySatSystems(satSet);
        resetCurrData();
        return *this;
    }

    RinexEpoch RinexEpoch::extractTypeID(const gnsstk::TypeID& type) const
    {
        return RinexEpoch(rinex.extractTypeID(type));
    }

    RinexEpoch RinexEpoch::extractTypeID(const gnsstk::TypeIDSet& typeSet) const
    {
        return RinexEpoch(rinex.extractTypeID(typeSet));
    }

    RinexEpoch& RinexEpoch::keepOnlyTypeID(const gnsstk::TypeID& type)
    {
        return keepOnlyTypeID(TypeIDSet{type});
    }

    RinexEpoch& RinexEpoch::keepOnlyTypeID(const gnsstk::TypeIDSet& typeSet)
    {
        rinex.keepOnlyTypeID(typeSet);
        resetCurrData();
        return *this;
    }

    RinexEpoch& RinexEpoch::removeSatID(int id, SatelliteSystem system)
    {
        SatID sv(id, system);
        rinex.removeSatID(sv);
        resetCurrData();
        return (*this);
    }

    RinexEpoch& RinexEpoch::removeSatID(const SatIDSet& satSet)
    {
        rinex.removeSatID(satSet);
        resetCurrData();
        return (*this);
    }

    RinexEpoch& RinexEpoch::removeSatID(const SatID& sv)
    {
        rinex.removeSatID(sv);
        resetCurrData();
        return (*this);
    }
} // namespace gnsstk