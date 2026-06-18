#include "UsedInPvtMarker.hpp"

using namespace gnsstk;
namespace pod
{

    gnsstk::IRinex& UsedInPvtMarker::Process(gnsstk::IRinex& rin_epoch)
    {
        markAsUsed(rin_epoch.getBody());
        return rin_epoch;
    }

    // mark all SV in SatTypePtrMap as useable in PVT
    gnsstk::SatTypePtrMap& UsedInPvtMarker::markAsUsed(gnsstk::SatTypePtrMap& satData) const
    {
        for (auto&& it : satData)
            it.second->get_value()[type] = SatUsedStatus::UsedInPVT;

        return satData;
    }

    gnsstk::SatTypePtrMap& UsedInPvtMarker::keepOnlyUsed(gnsstk::SatTypePtrMap& satData) const
    {

        SatIDSet rejectedSats;

        for (auto&& [sat, data] : satData)
        {
            const auto& values = data->get_value();
            auto it = values.find(type);

            if (it == values.end() || it->second == static_cast<double>(SatUsedStatus::NotUsedInPVT))
            {
                rejectedSats.insert(sat);
            }
        }


        satData.removeSatID(rejectedSats);
        return satData;
    }

    gnsstk::SatTypePtrMap& UsedInPvtMarker::CleanScFlags(gnsstk::SatTypePtrMap& satData) const
    {
        for (auto it = satData.begin(); it != satData.end(); ++it)
        {
            auto status = it->second->get_value().find(TypeID::satStatus);
            // if (preEpochSats.find(it->first) == preEpochSats.end())
            //	it->second->get_value()[TypeID::CSL1] = it->second->get_value()[TypeID::CSL2] = 1;

            // reset CS flag, if this sv - epoch already has been rejected by CS catcher
            if (status != it->second->get_value().end()
                && static_cast<SatUsedStatus>(status->second) == SatUsedStatus::NotEnoughData)
            {
                it->second->get_value()[TypeID::CSL1] = it->second->get_value()[TypeID::CSL2] = 0;
                // status->second = UsedInPVT;
            }
        }
        return satData;
    }

    gnsstk::SatTypePtrMap& UsedInPvtMarker::CleanSatArcFlags(gnsstk::SatTypePtrMap& satData) const
    {
        for (auto&& it : satData)
            it.second->get_value()[TypeID::satArc] = 0;

        return satData;
    }

    UsedInPvtMarker& UsedInPvtMarker::updateLastEpoch(const gnsstk::IRinex& rin_epoch)
    {
        preEpoch = rin_epoch.getHeader().epoch;
        preEpochSats = rin_epoch.getBody().getSatID();

        return *this;
    }
} // namespace pod