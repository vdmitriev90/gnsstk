#include "KalmanSolverFB.h"

#include "GnssObsMapping.h"
#include "GnssSolution.h"
#include "PowerSum.hpp"
#include "SatObservationBlocks.h"
#include "StringUtils.h"
#include "WinUtils.h"
using namespace gnsstk;

namespace pod
{

    KalmanSolverFB::KalmanSolverFB()
        : currCycle(0)
        , cyclesNumber(0)
        , processedMeasurements(0)
        , rejectedMeasurements(0)
        , LiCsDetector_(nullptr)
        , MwCsDetector_(nullptr)
    {
    }

    KalmanSolverFB::KalmanSolverFB(EquationComposerPtr eqs)
        : currCycle(0)
        , cyclesNumber(0)
        , processedMeasurements(0)
        , rejectedMeasurements(0)
        , LiCsDetector_(nullptr)
        , MwCsDetector_(nullptr)
    {
        solver = KalmanSolver(eqs);
    }

    KalmanSolverFB::~KalmanSolverFB() {}

    gnsstk::IRinex& KalmanSolverFB::Process(gnsstk::IRinex& rin_epoch)
    {
        // satellites that reached the solver; others were removed upstream only
        // from currData (e.g. by SingleDifferenceOp) and must not be resurrected
        // by resetCurrData() during reprocessing
        const gnsstk::SatIDSet inputSats = rin_epoch.getBody().getSatID();

        solver.Process(rin_epoch);
        if (LiCsDetector_ && MwCsDetector_ && solver.getResetState())
        {
            LIDetMap[rin_epoch.getHeader().epoch] = *LiCsDetector_;
            MWDetMap[rin_epoch.getHeader().epoch] = *MwCsDetector_;
        }

        // Before returning, store the results for a future iteration
        if (currCycle == 0)
        {
            // remember satellites flagged with cycle slip in the forward pass
            // (includes base station slips merged by single differences)
            auto& slips = fwdSlips[rin_epoch.getHeader().epoch];
            for (const auto& [sat, tv] : rin_epoch.getBody())
                for (auto cs : {TypeID::CSL1, TypeID::CSL2, TypeID::CSL5})
                {
                    auto it = tv->find(cs);
                    if (it != tv->end() && it->second > 0.0)
                    {
                        slips.insert(sat);
                        break;
                    }
                }

            // Store observation data
            auto stored = rin_epoch.clone();
            if (auto* re = dynamic_cast<gnsstk::RinexEpoch*>(stored.get()))
                re->keepOnlySatID(inputSats);
            ObsData.push_back(std::move(stored));

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

    void KalmanSolverFB::reProcessBackward()
    {
        // In backward direction an ambiguity jump that happened between t(k-1) and t(k)
        // must be flagged at t(k-1). Detectors re-run on rover only, so slips coming
        // from the base station (merged in SD) must be shifted explicitly.
        const gnsstk::SatIDSet* later = nullptr;
        for (auto rpos = ObsData.rbegin(); rpos != ObsData.rend(); ++rpos)
        {
            ReProcessOneEpoch(**rpos, later);
            later = &fwdSlips[(*rpos)->getHeader().epoch];
        }
    }

    void KalmanSolverFB::reProcess()
    {
        // Backwards iteration. We must do this at least once
        reProcessBackward();

        for (currCycle = 0; currCycle < cyclesNumber - 1; ++currCycle)
        {
            for (auto& it : ObsData)
            {
                ReProcessOneEpoch(*it);
            }

            reProcessBackward();
        }
    }

    gnsstk::IRinex& KalmanSolverFB::ReProcessOneEpoch(gnsstk::IRinex& rin_epoch,
                                                       const gnsstk::SatIDSet* extraSlips)
    {

        if (solver.ResetIfRequared(rin_epoch.getHeader().epoch, solver.FilterData))
        {
            *LiCsDetector_ = LIDetMap[rin_epoch.getHeader().epoch];
            *MwCsDetector_ = MWDetMap[rin_epoch.getHeader().epoch];
            DBOUT_LINE("state update")
            //*satArcMarker = SatArcMap[rin_epoch.getHeader().epoch];
        }

        rin_epoch.resetCurrData();

        usedSvMarker.keepOnlyUsed(rin_epoch.getBody());
        usedSvMarker.CleanSatArcFlags(rin_epoch.getBody());
        usedSvMarker.CleanScFlags(rin_epoch.getBody());
        checkLimits(rin_epoch, currCycle);

        rin_epoch >> reProcList;

        if (extraSlips)
        {
            for (auto& [sat, tv] : rin_epoch.getBody())
            {
                if (!extraSlips->count(sat))
                    continue;
                for (auto cs : {TypeID::CSL1, TypeID::CSL2, TypeID::CSL5})
                {
                    auto it = tv->find(cs);
                    if (it != tv->end())
                        it->second = 1.0;
                }
            }
        }

        solver.Process(rin_epoch);

        if (LiCsDetector_ && MwCsDetector_ 
            && LIDetMap.find(rin_epoch.getHeader().epoch) != LIDetMap.end())
        {
            solver.FilterData[rin_epoch.getHeader().epoch] = solver.getState();
            LIDetMap[rin_epoch.getHeader().epoch] = *LiCsDetector_;
            MWDetMap[rin_epoch.getHeader().epoch] = *MwCsDetector_;
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

        // Build per-satellite measurement blocks (slot-driven).
        SatObservationBlocks blocks;
        buildObservationBlocks(gData, solver.eqComposer().getConfig(), blocks);

        for (const auto& block : blocks)
        {
            auto it = gData.getBody().find(block.sat);
            if (it == gData.getBody().end() || !it->second)
                continue;

            auto& values = *it->second;

            for (const auto& m : block.measurements)
            {
                // measurement (prefit) type -> postfit residual type
                const auto postfit = obs_mapping::prefitToPostfit(m.type);
                if (postfit == TypeID::Unknown)
                    continue;

                const TypeID postfitType(postfit);

                // only code/phase residuals have configured limits
                if (!codeResTypes.count(postfitType) && !phaseResTypes.count(postfitType))
                    continue;

                auto itRes = values.find(postfitType);
                if (itRes == values.end())
                    continue;

                // Check postfit residual against the per-cycle limit
                if (std::abs(itRes->second) > getLimit(postfitType, cycleNumber))
                {
                    values.erase(postfitType);
                    satRejectedSet.insert(block.sat);
                }
            }
        }

        // Update the number of rejected measurements
        rejectedMeasurements += satRejectedSet.size();

        // Remove satellites with missing data
        gData.getBody().removeSatID(satRejectedSet);
    }
} // namespace pod
