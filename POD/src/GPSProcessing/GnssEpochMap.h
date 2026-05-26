#ifndef POD_GNSS_EPOCH_H
#define POD_GNSS_EPOCH_H

#include "CommonTime.hpp"
#include "DataStructures.hpp"
#include "Position.hpp"

namespace pod
{
    struct GnssEpoch
    {

        GnssEpoch();

        GnssEpoch(const gnsstk::satTypeValueMap& sData);
        GnssEpoch(const gnsstk::SatTypePtrMap& stpMap);
        GnssEpoch(const gnsstk::gnssRinex& rin_epoch);
        ~GnssEpoch();

        gnsstk::satTypeValueMap satData;

        gnsstk::typeValueMap slnData;

        std::ostream& dump(std::ostream& s, int precision = 4);
    };

    struct GnssEpochMap
    {
        GnssEpochMap();

        ~GnssEpochMap();

        // dump object to a stream
        std::ostream& dump(std::ostream& s, int precision = 4);

        void updateMetadata();

        std::string title;

        // all sv in data
        std::set<gnsstk::SatID> svs;

        // all solution types
        std::set<int> slnTypes;

        // all typeIDs
        gnsstk::TypeIDSet types;

        gnsstk::Rinex3ObsHeader header;

        std::map<gnsstk::CommonTime, GnssEpoch> data;

        std::map<gnsstk::CommonTime, GnssEpoch>::iterator begin()
        {
            return data.begin();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::iterator end()
        {
            return data.end();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::const_iterator begin() const
        {
            return data.begin();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::const_iterator end() const
        {
            return data.end();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::reverse_iterator rbegin()
        {
            return data.rbegin();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::const_reverse_iterator rbegin() const
        {
            return data.rbegin();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::reverse_iterator rend()
        {
            return data.rend();
        };

        std::map<gnsstk::CommonTime, GnssEpoch>::const_reverse_iterator rend() const
        {
            return data.rend();
        };

        gnsstk::CommonTime getInitialTime() const
        {
            if (data.size() == 0)
                GNSSTK_THROW(gnsstk::InvalidRequest("GnssEpochMap objects contais no elements"));

            return begin()->first;
        }

        gnsstk::CommonTime getFinalTime() const
        {
            if (data.size() == 0)
                GNSSTK_THROW(gnsstk::InvalidRequest("GnssEpochMap objects contais no elements"));

            return rbegin()->first;
        }

        size_t size() const
        {
            return data.size();
        }

      protected:
        void updateTypes(const gnsstk::TypeIDSet& types);
    };

} // namespace pod

#endif // !POD_GNSS_EPOCH_H
