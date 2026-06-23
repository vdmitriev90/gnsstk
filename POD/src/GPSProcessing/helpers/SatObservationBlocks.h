#pragma once

#include "SatID.hpp"
#include "TypeID.hpp"

#include "LinearCombination.h"
#include "GnssObsMapping.h"
#include "Weighting.h"
#include "RinexEpoch.h"

#include <vector>
namespace gnsstk
{
    struct typeValueMap;
    class IRinex;
} // namespace gnsstk

namespace pod
{
    /// One measurement associated with a satellite
    struct Measurement
    {
        gnsstk::TypeID type; ///< observation type (e.g. L1, P2, ...)
        double value = 0.0;  ///< observation value
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
        size_t size() const noexcept
        {
            return measurements.size();
        }

        /// Check if block is empty
        bool empty() const noexcept
        {
            return measurements.empty();
        }

        /// Clear measurements (reuse block)
        void clear()
        {
            measurements.clear();
        }

        /// Reserve space (performance)
        void reserve(size_t n)
        {
            measurements.reserve(n);
        }
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

        size_t totalMeasurements() const
        {
            size_t sum = 0;
            for (const auto& b : blocks_)
                sum += b.size();
            return sum;
        }

      private:
        Container blocks_;
    };

    void buildObservationBlocks(const gnsstk::IRinex& gData, const ObservationTypesProvider& provider, SatObservationBlocks& blocks);

} // namespace pod