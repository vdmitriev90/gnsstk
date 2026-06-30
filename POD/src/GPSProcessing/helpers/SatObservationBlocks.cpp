#include "SatObservationBlocks.h"

#include "RinexEpoch.h"
#include "Vector.hpp"

#include <cmath>

namespace pod
{
    void buildObservationBlocks(const gnsstk::IRinex& gData, SatObservationBlocks& blocks)
    {
        using gnsstk::TypeID;

        blocks.clear();
        blocks.reserve(gData.getBody().size());

        auto& slotProvider = PrefitSlotProvider::instance();
        const auto& slots = slotProvider.getSlots();

        for (const auto& [sat, tvPtr] : gData.getBody())
        {
            if (!tvPtr)
                continue;

            const auto& values = *tvPtr;

            SatObservationBlock block;
            block.sat = sat;
            block.data = tvPtr;

            block.reserve(slots.size());

            // Read the per-satellite weight (TypeID::weight), default to 1.0 if absent
            double satWeight = 1.0;
            {
                auto wIt = values.find(TypeID::weight);
                if (wIt != values.end())
                    satWeight = wIt->second;
            }

            for (const auto& slot : slots)
            {
                const TypeID type = slotProvider.resolve(slot, sat.system);

                if (type == TypeID::Unknown)
                    continue;

                auto it = values.find(type);
                if (it == values.end())
                    continue;

                const double prefit_residual = it->second;

                // Combine per-satellite weight with the type-based weight factor,
                // matching the W matrix: W(i,i) = weights(i) * weightFactor
                const double w = satWeight * pod::weighting::weightOf(type.type);

                block.measurements.push_back({type, prefit_residual, w});
            }

            if (!block.empty())
                blocks.push_back(std::move(block));
        }
    }

    /// Remove all configured slots
    void PrefitSlotProvider::clearSlots()
    {
        slots_.clear();
    }

    /// Set single measurement slot
    void PrefitSlotProvider::setSlots(ObsSlot slot)
    {
        slots_.clear();
        slots_.push_back(slot);
    }

    /// Set multiple measurement slots
    void PrefitSlotProvider::setSlots(const std::vector<ObsSlot>& slots)
    {
        slots_ = slots;
    }

    /// Add single measurement slot
    void PrefitSlotProvider::addSlot(ObsSlot slot)
    {
        slots_.push_back(slot);
    }

    /// Get current slots
    const std::vector<ObsSlot>& PrefitSlotProvider::getSlots() const
    {
        return slots_;
    }

    /// Resolve slot -> actual prefit TypeID for given system
    gnsstk::TypeID PrefitSlotProvider::resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const
    {
        using namespace gnsstk;
        using namespace pod::obs_mapping;

        const ObsConfig* cfg = findConfig(ss);
        if (!cfg)
            return TypeID::Unknown;

        auto toPrefit = [](const TypeID::ValueType t) {
            auto p = getPrefitObs(TypeID(t));
            return (p == TypeID::ValueType::Unknown) ? TypeID::Unknown : TypeID(p);
        };

        switch (slot)
        {
        case ObsSlot::FirstBandCode:
            return toPrefit(cfg->firstCode);

        case ObsSlot::SecondBandCode:
            return toPrefit(cfg->secondCode);

        case ObsSlot::FirstBandPhase:
            return toPrefit(cfg->firstPhase);

        case ObsSlot::SecondBandPhase:
            return toPrefit(cfg->secondPhase);

        case ObsSlot::CodeIonoFree:
            return TypeID::prefitPC;

        case ObsSlot::PhaseIonoFree:
            return TypeID::prefitLC;

        default:
            return TypeID::Unknown;
        }
    }

    /// Number of rows contributed by this satellite
    size_t SatObservationBlock::size() const noexcept
    {
        return measurements.size();
    }

    /// Check if block is empty
    bool SatObservationBlock::empty() const noexcept
    {
        return measurements.empty();
    }

    /// Clear measurements (reuse block)
    void SatObservationBlock::clear()
    {
        measurements.clear();
    }

    /// Reserve space (performance)
    void SatObservationBlock::reserve(size_t n)
    {
        measurements.reserve(n);
    }

    ResidualInfo findMaxResidual(const SatObservationBlocks& blocks,
                                 const gnsstk::Vector<double>& residuals,
                                 const std::set<gnsstk::TypeID>& postfitTypes)
    {
        ResidualInfo worst;

        int row = 0;
        for (const auto& block : blocks)
        {
            for (const auto& m : block.measurements)
            {
                const auto postfit = obs_mapping::prefitToPostfit(m.type);
                if (postfit != gnsstk::TypeID::Unknown && postfitTypes.count(gnsstk::TypeID(postfit)))
                {
                    const double value = std::abs(residuals(row));
                    if (value > worst.value)
                    {
                        worst.sat = block.sat;
                        worst.type = gnsstk::TypeID(postfit);
                        worst.row = row;
                        worst.value = value;
                    }
                }
                ++row;
            }
        }
        return worst;
    }

    std::set<int> getSatRows(const SatObservationBlocks& blocks, const gnsstk::SatID& sat)
    {
        std::set<int> rows;

        int row = 0;
        for (const auto& block : blocks)
        {
            if (block.sat == sat)
                for (size_t k = 0; k < block.size(); ++k)
                    rows.insert(row + static_cast<int>(k));
            row += static_cast<int>(block.size());
        }
        return rows;
    }

    size_t SatObservationBlocks::totalMeasurements() const
    {
        size_t sum = 0;
        for (const auto& b : blocks_)
            sum += b.size();
        return sum;
    }
} // namespace pod