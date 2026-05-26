#include "KalmanSolverFB.h"

#include "GnssSolution.h"
#include "PowerSum.hpp"
#include "StringUtils.h"
#include "WinUtils.h"
using namespace gnsstk;

namespace pod
{

    KalmanSolverFB::KalmanSolverFB()
        : currCycle(0)
        , processedMeasurements(0)
        , rejectedMeasurements(0)
    {
    }

    KalmanSolverFB::KalmanSolverFB(eqComposer_sptr eqs)
        : currCycle(0)
        , processedMeasurements(0)
        , rejectedMeasurements(0)
    {
        solver = KalmanSolver(eqs);
    }

    KalmanSolverFB::~KalmanSolverFB() {}

    gnsstk::IRinex& KalmanSolverFB::Process(gnsstk::IRinex& rin_epoch)
    {
        solver.Process(rin_epoch);
        if (solver.getResetState())
        {
            LIDetMap[rin_epoch.getHeader().epoch] = *LIDet;
            MWDetMap[rin_epoch.getHeader().epoch] = *MWDet;
        }

        // Before returning, store the results for a future iteration
        if (currCycle == 0)
        {
            // Store observation data
            ObsData.push_back(rin_epoch.clone());

            // Update the number of processed measurements
            processedMeasurements += rin_epoch.getBody().numSats();
        }

        return rin_epoch;
    }

    bool KalmanSolverFB::lastProcess(gnsstk::IRinex& rin_epoch)
    {

        // Keep processing while 'ObsData' is not empty
        if (!ObsData.empty())
        {
            // Get the first data epoch in 'ObsData' and process it.
            // The result will be stored in 'gData'
            rin_epoch = ReProcessOneEpoch(*ObsData.front());

            // gData = ObsData.front();
            // Remove the first data epoch in 'ObsData', freeing some
            // memory and preparing for next epoch
            ObsData.pop_front();
            return true;
        }
        else
        {
            return false;
        }
    }

    void KalmanSolverFB::reProcess()
    {
        // Backwards iteration. We must do this at least once
        for (auto rpos = ObsData.rbegin(); rpos != ObsData.rend(); ++rpos)
            ReProcessOneEpoch(**rpos);

        for (currCycle = 0; currCycle < cyclesNumber - 1; ++currCycle)
        {
            for (auto& it : ObsData)
            {
                ReProcessOneEpoch(*it);
            }

            for (auto rpos = ObsData.rbegin(); rpos != ObsData.rend(); ++rpos)
                ReProcessOneEpoch(**rpos);
        }
    }

    gnsstk::IRinex& KalmanSolverFB::ReProcessOneEpoch(gnsstk::IRinex& rin_epoch)
    {

        if (solver.ResetIfRequared(rin_epoch.getHeader().epoch, solver.FilterData))
        {
            *LIDet = LIDetMap[rin_epoch.getHeader().epoch];
            *MWDet = MWDetMap[rin_epoch.getHeader().epoch];
            DBOUT_LINE("state update")
            //*satArcMarker = SatArcMap[rin_epoch.getHeader().epoch];
        }

        rin_epoch.resetCurrData();

        usedSvMarker.keepOnlyUsed(rin_epoch.getBody());
        usedSvMarker.CleanSatArcFlags(rin_epoch.getBody());
        usedSvMarker.CleanScFlags(rin_epoch.getBody());
        checkLimits(rin_epoch, currCycle);

        rin_epoch >> reProcList;

        solver.Process(rin_epoch);

        if (LIDetMap.find(rin_epoch.getHeader().epoch) != LIDetMap.end())
        {
            solver.FilterData[rin_epoch.getHeader().epoch] = solver.getState();
            LIDetMap[rin_epoch.getHeader().epoch] = *LIDet;
            MWDetMap[rin_epoch.getHeader().epoch] = *MWDet;
            DBOUT_LINE("state stored")
        }
        // SatArcMap[rin_epoch.getHeader().epoch] = *satArcMarker;

        return rin_epoch;
    }

    KalmanSolverFB& KalmanSolverFB::setLimits(const std::vector<double>& codeLims,
                                              const std::vector<double>& phaseLims)
    {
        tresholds.codeLimits = codeLims;
        tresholds.phaseLimits = phaseLims;

        return *this;
    }

    double KalmanSolverFB::getLimit(const gnsstk::TypeID& type, size_t cycleNumber)
    {
        if (codeResTypes.find(type) != codeResTypes.end())
            if (cycleNumber < tresholds.codeLimits.size())
                return tresholds.codeLimits[cycleNumber];
        if (phaseResTypes.find(type) != phaseResTypes.end())
            if (cycleNumber < tresholds.phaseLimits.size())
                return tresholds.phaseLimits[cycleNumber];

        std::string msg = "Can't get observables treshold for type: '" + TypeID::tStrings[type.type]
                          + "' with reprocess cycle number: '"
                          + gnsstk::StringUtils::asString(cycleNumber) + "'.";

        InvalidRequest e(msg);
        GNSSTK_THROW(e);
    }

    void KalmanSolverFB::checkLimits(IRinex& gData, size_t cycleNumber)
    {
        // Set to store rejected satellites
        SatIDSet satRejectedSet;

        // Let's check limits
        for (auto&& type : solver.eqComposer().residTypes())
        {
            double limit = getLimit(type, cycleNumber);
            for (auto&& it : gData.getBody())
            {
                // Check postfit values and mark satellites as rejected
                auto itRes = it.second->get_value().find(type);
                if (itRes != it.second->get_value().end() && std::abs(itRes->second) > limit)
                {
                    it.second->get_value().erase(type);
                    satRejectedSet.insert(it.first);
                }
            }
        }

        // Update the number of rejected measurements
        rejectedMeasurements += satRejectedSet.size();

        // Remove satellites with missing data
        gData.getBody().removeSatID(satRejectedSet);
    }
} // namespace pod
