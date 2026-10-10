#pragma once
#include "ObservationResolver.h"
#include "ProcessingClass.hpp"

#include <set>

namespace pod
{
	/// Logical observation a range check is applied to.
	///
	/// Concrete TypeIDs are resolved per satellite system through ObservationResolver,
	/// so the same filter works across GPS, GLONASS, Galileo, etc.
	/// (e.g. SecondCode resolves to P2 for GPS but C5 for Galileo).
	enum class ObsRangeType
	{
		FirstCode,    ///< first frequency code (system-resolved)
		SecondCode,   ///< second frequency code (P2 for GPS, C5 for Galileo, ...)
		FirstPhase,   ///< first frequency phase
		SecondPhase,  ///< second frequency phase
		Snr,          ///< L1 signal strength (S1)
		IonoFreeCode  ///< iono-free code combination (PC)
	};

	/// Filters out satellites whose observations are grossly out of bounds.
	///
	/// For every configured ObsRangeType the filter resolves the actual TypeID
	/// for the satellite's system and checks that the value lies within the
	/// [min, max] range. Satellites missing the observation or having a value
	/// out of bounds are removed from the data structure and recorded in the
	/// rejected satellites table.
	///
	/// This is a pod-side, multi-GNSS aware replacement for the legacy
	/// gnsstk::SimpleFilter, using ObservationResolver like LinearCombination.
	class ObsRangeFilter : public gnsstk::ProcessingClass
	{
	  public:
		/// Default constructor. By default, filters the first code within
		/// [15e6, 30e6] meters.
		ObsRangeFilter();

		/// @param type Observation to be filtered.
		/// @param min  Minimum limit (in meters).
		/// @param max  Maximum limit (in meters).
		ObsRangeFilter(ObsRangeType type, double min, double max);

		/// @param type Observation to be filtered.
		explicit ObsRangeFilter(ObsRangeType type);

		/// @param types Set of observations to be filtered.
		/// @param min   Minimum limit (in meters).
		/// @param max   Maximum limit (in meters).
		ObsRangeFilter(std::initializer_list<ObsRangeType> types, double min, double max);

		/// @param types Set of observations to be filtered.
		explicit ObsRangeFilter(std::initializer_list<ObsRangeType> types);

		ObsRangeFilter& setMinLimit(double min)
		{
			minLimit_ = min;
			return *this;
		}

		double getMinLimit() const
		{
			return minLimit_;
		}

		ObsRangeFilter& setMaxLimit(double max)
		{
			maxLimit_ = max;
			return *this;
		}

		double getMaxLimit() const
		{
			return maxLimit_;
		}

		/// Add an observation to be filtered.
		ObsRangeFilter& addFilteredType(ObsRangeType type)
		{
			filterTypes_.insert(type);
			return *this;
		}

		/// Set a single observation to be filtered, erasing previous ones.
		ObsRangeFilter& setFilteredType(ObsRangeType type)
		{
			filterTypes_ = {type};
			return *this;
		}

		/// Set the observations to be filtered, erasing previous ones.
		ObsRangeFilter& setFilteredType(const std::set<ObsRangeType>& types)
		{
			filterTypes_ = types;
			return *this;
		}

		std::set<ObsRangeType> getFilteredType() const
		{
			return filterTypes_;
		}

		gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

		std::string getClassName() const override;

	  private:
		/// Checks that the value is within the configured limits.
		bool checkValue(double value) const
		{
			return (value >= minLimit_) && (value <= maxLimit_);
		}

		/// Resolve a logical observation to the actual TypeID for a system.
		/// Returns TypeID::Unknown when the system does not provide it.
		gnsstk::TypeID resolve(ObsRangeType type, gnsstk::SatelliteSystem ss) const;

		ObservationResolver resolver_;

		/// Set of observations to be filtered.
		std::set<ObsRangeType> filterTypes_;

		/// Minimum value allowed for input data (in meters).
		double minLimit_;

		/// Maximum value allowed for input data (in meters).
		double maxLimit_;
	};
} // namespace pod
