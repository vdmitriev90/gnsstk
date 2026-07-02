#include "CycleSlipDetector.hpp"

namespace pod
{
    using namespace gnsstk;

    CycleSlipDetector::CycleSlipDetector(TypeID ot1)
        : obsType(ot1)
        , deltaTMax(61.0)
        , useLLI(true)
        , useEpochFlag(false)
        , isReprocess(false)
    {
    }

    CycleSlipDetector::CycleSlipDetector(TypeID ot1, double dtMax, bool useLli)
        : obsType(ot1)
        , useLLI(useLli)
        , useEpochFlag(false)
        , isReprocess(false)
    {
        setDeltaTMax(dtMax);
    }

    /* Method to set the maximum interval of time allowed between two
	 *  successive epochs.
	 *
	 * @param maxDelta      Maximum interval of time, in seconds
	 */
    CycleSlipDetector& CycleSlipDetector::setDeltaTMax(const double& maxDelta)
    {
        // Don't allow delta times less than or equal to 0
        if (maxDelta > 0.0)
        {
            deltaTMax = maxDelta;
        }
        else
        {
            deltaTMax = 61.0;
        }

        return (*this);

    } // End of method 'LICSDetector2::setDeltaTMax()'

    /* Returns a satTypeValueMap object, adding the new data generated
	 * when calling this object.
	 *
	 * @param epoch     Time of observations.
	 * @param gData     Data object holding the data.
	 * @param epochflag Epoch flag.
	 */
    SatTypePtrMap& CycleSlipDetector::Process(const CommonTime& epoch, SatTypePtrMap& gData, const short& epochflag)
    {
        try
        {
            double value1(0.0);
            double lli1(0.0);
            double lli2(0.0);

            SatIDSet satRejectedSet;
            auto& rejTableItem = rejectedSatsTable[epoch];
            // Loop through all the satellites
            for (auto&& it : gData)
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

                if (useLLI)
                {
                    try
                    {
                        // Try to get the LLI1 index
                        const auto first_lli = resolver_.firstLLI(it.first.system);
                        lli1 = (*it.second)(first_lli);
                    }
                    catch (...)
                    {
                        // If LLI #1 is not found, set it to zero
                        // You REALLY want to have BOTH LLI indexes properly set
                        lli1 = 0.0;
                    }

                    try
                    {
                        // Try to get the LLI2 index
                        const auto second_lli = resolver_.secondLLI(it.first.system);
                        lli2 = (*it.second)(second_lli);
                    }
                    catch (...)
                    {
                        // If LLI #2 is not found, set it to zero
                        // You REALLY want to have BOTH LLI indexes properly set
                        lli2 = 0.0;
                    }
                }

                // If everything is OK, then get the new values inside the
                // structure. This way of computing it allows concatenation of
                // several different cycle slip detectors
                auto res = getDetection(epoch, it.first, (*it.second), epochflag, value1, lli1, lli2);

                if (isReprocess)
                {
                    if (res == DetectionResult::NotEnoughData)
                        continue;
                }

                auto st = SatUsedStatus::Unknown;
                auto it1 = it.second->find(TypeID::satStatus);
                if (it1 != it.second->end())
                    st = static_cast<SatUsedStatus>((int)it1->second);

                if (res == DetectionResult::NotEnoughData)
                    (*it.second)[TypeID::satStatus] = SatUsedStatus::NotEnoughData;
                else if (res == DetectionResult::CsDetected)
                    (*it.second)[TypeID::satStatus] = SatUsedStatus::RejectedByCsDetector;
                else if (res == DetectionResult::CsDetectedByMW)
                {
                    if (st == SatUsedStatus::RejectedByLIDetector)
                        (*it.second)[TypeID::satStatus] = SatUsedStatus::RejectedByCsDetector;
                    else
                        (*it.second)[TypeID::satStatus] = SatUsedStatus::RejectedByMWDetector;
                }
                else if (res == DetectionResult::CsDetectedByLI2)
                {
                    if (st == SatUsedStatus::RejectedByMWDetector)
                        (*it.second)[TypeID::satStatus] = SatUsedStatus::RejectedByCsDetector;
                    else
                        (*it.second)[TypeID::satStatus] = SatUsedStatus::RejectedByLIDetector;
                }

                const TypeID csFlag1 = resolver_.resolveCsFlag(ObsSlot::FirstBandPhase,  it.first.system);
                const TypeID csFlag2 = resolver_.resolveCsFlag(ObsSlot::SecondBandPhase, it.first.system);

                (*it.second)[csFlag1] += res;

                if (res > 0)
                    rejTableItem.insert(it.first);

                if ((*it.second)[csFlag1] > 1.0)
                    (*it.second)[csFlag1] = 1.0;

                // We will mark both cycle slip flags
                (*it.second)[csFlag2] = (*it.second)[csFlag1];
            }

            // Remove satellites with missing data
            gData.removeSatID(satRejectedSet);

            return gData;
        }
        catch (Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e(getClassName() + ":" + u.what());

            GNSSTK_THROW(e);
        }

    } // End of method 'MWCSDetector::Process()'

    /* Returns a gnnsRinex object, adding the new data generated when
	   * calling this object.
	   *
	   * @param gData    Data object holding the data.
	   */
    IRinex& CycleSlipDetector::Process(IRinex& gData)
    {

        try
        {
            auto flag = (useEpochFlag) ? gData.getHeader().epochFlag : 0;
            Process(gData.getHeader().epoch, gData.getBody(), flag);

            return gData;
        }
        catch (Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e(getClassName() + ":" + u.what());

            GNSSTK_THROW(e);
        }

    } // End of method 'MWCSDetector::Process()'

} // namespace pod