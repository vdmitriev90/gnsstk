#include "SNRCatcher.h"
using namespace gnsstk;

namespace pod
{

    // Minimum buffer size. It is always set to 5
    const int SNRCatcher::minBufferSize = 5;

    SNRCatcher::SNRCatcher()
        : obsType(TypeID::S1)
        , resultType1(gnsstk::TypeID::CSL1)
        , deltaTMax(901.0)
        , satThreshold(10)
        , maxBufferSize(30) {};

    SNRCatcher::SNRCatcher(const TypeID& obsType, const TypeID& resType, double maxgap, double tresh, int maxCount)
        : obsType(obsType)
        , resultType1(resType)
        , deltaTMax(maxgap)
        , satThreshold(tresh)
        , maxBufferSize(maxCount) {};

    IRinex& SNRCatcher::Process(IRinex& gData)
    {
        try
        {
            Process(gData.getHeader().epoch, gData.getBody());
            return gData;
        }
        catch (Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e(getClassName() + ":" + u.what());

            GNSSTK_THROW(e);
        }

    } // End of method 'SNRCatcher::Process()'

    /* Returns a satTypeValueMap object, adding the new data generated
     *  when calling this object.
     *
     * @param epoch     Time of observations.
     * @param gData     Data object holding the data.
     * @param epochflag Epoch flag.
     */
    SatTypePtrMap& SNRCatcher::Process(const CommonTime& epoch, SatTypePtrMap& gData)
    {
        try
        {
            auto& rejTableItem = rejectedSatsTable[epoch];
            double value1(0.0);

            SatIDSet satRejectedSet;
            SatIDSet affectedSatSet;

            for (auto& it : gData)
            {
                try
                {
                    // Try to extract the values
                    value1 = (*it.second)(obsType);
                }
                catch (...)
                {
                    // If some value is missing, then schedule this satellite
                    // for removal
                    satRejectedSet.insert(it.first);
                    continue;
                }

                if (getDetection(epoch, it.first, (*it.second), value1) > .0)
                    affectedSatSet.insert(it.first);
            }

            // Remove satellites with missing data
            gData.removeSatID(satRejectedSet);

            rejTableItem.insert(satRejectedSet.begin(), satRejectedSet.end());

            // palliative guard in case of sharp drop SNR for huge amount of observable satellites
            //if (affectedSatSet.size() < 3)
            for (auto&& sv : affectedSatSet)
            {
                rejTableItem.insert(sv);
                auto& it = gData[sv];
                (*it)[resultType1] += 1.0;
                if ((*it)[resultType1] > 1.0)
                    (*it)[resultType1] = 1.0;
            }

            return gData;
        }
        catch (Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e(getClassName() + ":" + u.what());

            GNSSTK_THROW(e);
        }

    } // End of method 'SNRCatcher::Process()'

 double SNRCatcher::getDetection(const CommonTime& epoch, const SatID& sat, TypeValueRow& tvMap, double snr)
    {
        auto& currData = data[sat];

        if (!currData.epochs.empty())
        {
            const double dt = epoch - currData.epochs.back();

            if (dt <= 0.0 || dt > deltaTMax)
            {
                currData.epochs.clear();
                currData.buffer.clear();
            }
        }

        bool reportDrop = false;

        if (currData.buffer.size() >= static_cast<size_t>(minBufferSize))
        {
            double avg = 0.0;

            for (double value : currData.buffer)
                avg += value;

            avg /= static_cast<double>(currData.buffer.size());

            reportDrop = ((avg - snr) > satThreshold);
        }

        // Update history buffer
        currData.epochs.push_back(epoch);
        currData.buffer.push_back(snr);

        while (currData.buffer.size() > static_cast<size_t>(maxBufferSize))
        {
            currData.epochs.pop_front();
            currData.buffer.pop_front();
        }

        return reportDrop ? 1.0 : 0.0;
    }

    // Returns a string identifying this object.
    std::string SNRCatcher::getClassName() const
    {
        return "SNRCatcher";
    }
} // namespace pod
