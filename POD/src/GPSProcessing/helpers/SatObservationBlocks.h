#pragma once

#include "CommonEnums.h"
#include "GnssObsMapping.h"
#include "SatID.hpp"
#include "TypeID.hpp"
#include "Weighting.h"

#include <set>
#include <vector>
namespace gnsstk
{
    struct typeValueMap;
    class IRinex;
    template <class T>
    class Vector;
} // namespace gnsstk

namespace pod
{

    enum class ObsSlot
    {
        Undefined = 0,
        FirstBandCode,
        FirstBandPhase,
        SecondBandCode,
        SecondBandPhase,
        CodeIonoFree, // iono-free code
        PhaseIonoFree // iono-free phase
    };

    class PrefitSlotProvider
    {
      public:
        static PrefitSlotProvider& instance()
        {
            static PrefitSlotProvider inst;
            return inst;
        }

        /// Remove all configured slots
        void clearSlots();
        /// Set single measurement slot
        void setSlots(ObsSlot slot);
        /// Set multiple measurement slots
        void setSlots(const std::vector<ObsSlot>& slots);
        /// Add single measurement slot
        void addSlot(ObsSlot slot);

        /// Add multiple slots (optional convenience)
        template <typename... Slots>
        inline void addSlots(Slots... s)
        {
            (slots_.push_back(s), ...);
        }

        /// Get current slots
        const std::vector<ObsSlot>& getSlots() const;

        /// Resolve slot -> actual prefit TypeID for given system
        gnsstk::TypeID resolve(ObsSlot slot, gnsstk::SatelliteSystem ss) const;

      private:
        PrefitSlotProvider() = default;

      private:
        std::vector<ObsSlot> slots_;
    };

    /// One measurement associated with a satellite
    struct Measurement
    {
        gnsstk::TypeID type; ///< observation type (e.g. L1, P2, ...)
        double prefitResidual = 0.0; ///< observation value
        double weight = 0.0; ///< measurement weight
    };

    /// Block of observations for a single satellite
    ///
    /// This is the basic unit used to build:
    /// - design matrix (H)
    /// - measurements vector (l)
    /// - weights matrix (W)
    ///
    /// Each block corresponds to ONE satellite
    /// and contains all available measurements for it.
    ///
    struct SatObservationBlock
    {
        /// Satellite identifier
        gnsstk::SatID sat;

        /// Pointer to raw observation storage (from GNSS data)
        const gnsstk::typeValueMap* data = nullptr;

        /// List of actual measurements used in filter
        std::vector<Measurement> measurements;

        /// Number of rows contributed by this satellite
        size_t size() const noexcept;

        /// Check if block is empty
        bool empty() const noexcept;

        /// Clear measurements (reuse block)
        void clear();

        /// Reserve space (performance)
        void reserve(size_t n);
    };

    /// Container for all satellite observation blocks for one epoch
    class SatObservationBlocks
    {
      public:
        using Container = std::vector<SatObservationBlock>;
        using iterator = Container::iterator;
        using const_iterator = Container::const_iterator;

        iterator begin()
        {
            return blocks_.begin();
        }
        const_iterator begin() const
        {
            return blocks_.begin();
        }

        iterator end()
        {
            return blocks_.end();
        }
        const_iterator end() const
        {
            return blocks_.end();
        }

        size_t size() const
        {
            return blocks_.size();
        }
        bool empty() const
        {
            return blocks_.empty();
        }

        void clear()
        {
            blocks_.clear();
        }

        void reserve(size_t n)
        {
            blocks_.reserve(n);
        }

        void push_back(SatObservationBlock&& b)
        {
            blocks_.push_back(std::move(b));
        }

        const SatObservationBlock& operator[](size_t i) const
        {
            return blocks_[i];
        }

        size_t totalMeasurements() const;

      private:
        Container blocks_;
    };

    /// Describes a single residual row within the block (satellite)-major layout
    struct ResidualInfo
    {
        gnsstk::SatID sat; ///< satellite owning the row
        gnsstk::TypeID type; ///< postfit residual type
        int row = -1; ///< row index in the residual vector (-1 if none)
        double value = 0.0; ///< |residual| value
    };

    /// Find the row with the largest |residual| among the given postfit types.
    ///
    /// Walks @blocks in the same block (satellite)-major order used to build the
    /// measurement/residual vectors, converting each measurement's prefit type to
    /// its postfit counterpart before matching against @postfitTypes.
    ResidualInfo findMaxResidual(const SatObservationBlocks& blocks,
                                 const gnsstk::Vector<double>& residuals,
                                 const std::set<gnsstk::TypeID>& postfitTypes);

    /// Collect the contiguous set of residual rows contributed by @sat.
    std::set<int> getSatRows(const SatObservationBlocks& blocks, const gnsstk::SatID& sat);

    void buildObservationBlocks(const gnsstk::IRinex& gData, SatObservationBlocks& blocks);

} // namespace pod