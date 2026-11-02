#include "ResidualsStatistic.h"

#include "KalmanSolver.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <optional>
#include <ostream>
#include <tuple>

using namespace gnsstk;

namespace pod
{
    void RunningStats::add(double v)
    {
        if (count_ == 0)
            min_ = max_ = v;
        else
        {
            min_ = std::min(min_, v);
            max_ = std::max(max_, v);
        }
        ++count_;
        const double delta = v - mean_;
        mean_ += delta / static_cast<double>(count_);
        m2_ += delta * (v - mean_);
    }

    std::int64_t RunningStats::count() const
    {
        return count_;
    }

    double RunningStats::mean() const
    {
        return mean_;
    }

    double RunningStats::min() const
    {
        return min_;
    }

    double RunningStats::max() const
    {
        return max_;
    }

    std::optional<double> RunningStats::sampleStdDev() const
    {
        if (count_ < 2)
            return std::nullopt;
        return std::sqrt(m2_ / static_cast<double>(count_ - 1));
    }

    void RunningStats::writeTo(ResidualSummary& out) const
    {
        out.reset();
        out.count_ = count_;
        if (count_ == 0)
            return;

        out.mean_ = mean_;
        out.minimum_ = min_;
        out.maximum_ = max_;
        if (const auto sd = sampleStdDev())
            out.stdDev_ = *sd;
    }

    void PostfitResidualStatistics::summarize(const std::vector<double>& values, ResidualSummary& out)
    {
        RunningStats stats;
        for (double v : values)
            stats.add(v);
        stats.writeTo(out);
    }

    namespace
    {
        constexpr double SAT_MIN_ELEV_DEG = 30.0;

        struct Acc
        {
            RunningStats values;
            std::int64_t rejected = 0;

            void add(double v, bool isRejected)
            {
                if (isRejected)
                    ++rejected;
                else
                    values.add(v);
            }
        };

        using SysKey = std::tuple<int, int>;      // system, residual TypeID
        using BinKey = std::tuple<int, int, int>; // system, residual TypeID, bin index
        using SatKey = std::tuple<int, int, int>; // system, id, residual TypeID

        bool isSupported(SatelliteSystem sys)
        {
            return sys == SatelliteSystem::GPS || sys == SatelliteSystem::Glonass || sys == SatelliteSystem::Galileo;
        }

        TypeID toTypeID(int v)
        {
            return TypeID(static_cast<TypeID::ValueType>(v));
        }

        std::optional<double> elevationOf(const TypeValueRow& tv)
        {
            const auto it = tv.find(TypeID::elevation);
            if (it == tv.end() || !std::isfinite(it->second))
                return std::nullopt;
            return it->second;
        }

        bool isRejected(const TypeValueRow& tv)
        {
            const auto it = tv.find(TypeID::satStatus);
            return it != tv.end() && it->second < 0;
        }
    } // namespace

    void PostfitResidualStatistics::computeAggregates(const GnssEpochMap& data)
    {
        std::map<SysKey, Acc> systems;
        std::map<BinKey, Acc> bins;
        std::map<SatKey, Acc> sats;

        const int binCount = static_cast<int>(ELEV_BIN_MAX_DEG / ELEV_BIN_STEP_DEG);

        // accumulates all residual types of one satellite at one epoch
        auto accumulate = [&](const SatID& sat, const TypeValueRow& tv) {
            const auto el = elevationOf(tv);
            const bool rejected = isRejected(tv);
            const bool inBins = el && *el >= 0.0 && *el <= ELEV_BIN_MAX_DEG;
            const bool inSats = el && *el >= SAT_MIN_ELEV_DEG;
            const int sys = static_cast<int>(sat.system);

            for (const TypeID& type : KalmanSolver::allResTypes)
            {
                const auto resIt = tv.find(type);
                if (resIt == tv.end() || !std::isfinite(resIt->second))
                    continue;

                systems[SysKey(sys, type.type)].add(resIt->second, rejected);
                if (inBins)
                {
                    const int idx = std::min(static_cast<int>(*el / ELEV_BIN_STEP_DEG), binCount - 1);
                    bins[BinKey(sys, type.type, idx)].add(resIt->second, rejected);
                }
                if (inSats)
                    sats[SatKey(sys, sat.id, type.type)].add(resIt->second, rejected);
            }
        };

        for (const auto& epoch : data)
            for (const auto& sat : epoch.second.satData)
                if (isSupported(sat.first.system))
                    accumulate(sat.first, sat.second);

        sysResults.clear();
        for (auto& [key, acc] : systems)
        {
            ResidualStatisticsRecord rec;
            rec.satelliteSystem = static_cast<SatelliteSystem>(std::get<0>(key));
            rec.residualType = toTypeID(std::get<1>(key));
            rec.elevationMinDeg = 0.0;
            rec.elevationMaxDeg = ELEV_BIN_MAX_DEG;
            acc.values.writeTo(rec);
            rec.rejectedCount_ = acc.rejected;
            sysResults.push_back(std::move(rec));
        }

        binResults.clear();
        for (auto& [key, acc] : bins)
        {
            ResidualStatisticsRecord rec;
            rec.satelliteSystem = static_cast<SatelliteSystem>(std::get<0>(key));
            rec.residualType = toTypeID(std::get<1>(key));
            rec.elevationMinDeg = std::get<2>(key) * ELEV_BIN_STEP_DEG;
            rec.elevationMaxDeg = rec.elevationMinDeg + ELEV_BIN_STEP_DEG;
            acc.values.writeTo(rec);
            rec.rejectedCount_ = acc.rejected;
            binResults.push_back(std::move(rec));
        }

        satResults.clear();
        for (auto& [key, acc] : sats)
        {
            ResidualSatelliteStatisticsRecord rec;
            rec.sat = SatID(std::get<1>(key), static_cast<SatelliteSystem>(std::get<0>(key)));
            rec.residualType = toTypeID(std::get<2>(key));
            acc.values.writeTo(rec);
            rec.rejectedCount_ = acc.rejected;
            satResults.push_back(std::move(rec));
        }
    }

	PostfitResidualStatistics::PostfitResidualStatistics(bool withElevation)
		: printOrder{

			  {"C1", TypeID::postfitC1},
			  {"P1", TypeID::postfitP1},
			  {"P2", TypeID::postfitP2},
			  {"C5", TypeID::postfitC5},
			  {"C6", TypeID::postfitC6},
			  {"C7", TypeID::postfitC7},
			  {"C8", TypeID::postfitC8},
			  {"PC", TypeID::postfitPC},

			  {"L1", TypeID::postfitL1},
			  {"L2", TypeID::postfitL2},
			  {"L5", TypeID::postfitL5},
			  {"L6", TypeID::postfitL6},
			  {"L7", TypeID::postfitL7},
			  {"L8", TypeID::postfitL8},
			  {"LC", TypeID::postfitLC},
		  }
		, elevationEnabled(withElevation)
	{
	}

	void PostfitResidualStatistics::compute(const GnssEpochMap& data)
	{
		computeAggregates(data);
	}

	const std::vector<ResidualStatisticsRecord>& PostfitResidualStatistics::systemResults() const
	{
		return sysResults;
	}

	const std::vector<ResidualStatisticsRecord>& PostfitResidualStatistics::elevationResults() const
	{
		return binResults;
	}

	const std::vector<ResidualSatelliteStatisticsRecord>& PostfitResidualStatistics::satelliteResults() const
	{
		return satResults;
	}

	void PostfitResidualStatistics::print(std::ostream& os) const
	{
		static const std::pair<SatelliteSystem, const char*> systems[] = {
			{SatelliteSystem::GPS, "GPS"}, {SatelliteSystem::Glonass, "GLO"}, {SatelliteSystem::Galileo, "GAL"}};

		auto find = [](const std::vector<ResidualStatisticsRecord>& recs, SatelliteSystem sys, const TypeID& type,
					   double elMin) -> const ResidualStatisticsRecord* {
			const auto it = std::find_if(recs.begin(), recs.end(), [&](const ResidualStatisticsRecord& r) {
				return r.satelliteSystem == sys && r.residualType == type && r.elevationMinDeg == elMin;
			});
			return it != recs.end() && it->count_ > 0 ? &*it : nullptr;
		};

		for (const auto& item : printOrder)
			for (const auto& [sys, sysName] : systems)
				if (const auto* r = find(sysResults, sys, item.type, 0.0))
					os << std::left << std::setw(12) << item.name + "_" + sysName << " N=" << std::setw(8) << r->count_
					   << " Mean=" << std::setw(12) << r->mean_.value_or(0.0) << " Std=" << std::setw(12)
					   << r->stdDev_.value_or(0.0) << '\n';

		if (!elevationEnabled)
			return;

		for (const auto& item : printOrder)
			for (double lo = 0.0; lo < ELEV_BIN_MAX_DEG; lo += ELEV_BIN_STEP_DEG)
				for (const auto& [sys, sysName] : systems)
					if (const auto* r = find(binResults, sys, item.type, lo))
					{
						const std::string range = std::to_string(static_cast<int>(r->elevationMinDeg)) + "-"
												  + std::to_string(static_cast<int>(r->elevationMaxDeg));
						os << std::left << std::setw(20) << item.name + "_" + sysName + "_" + range << " N="
						   << std::setw(8) << r->count_ << std::fixed << " Mean=" << std::setw(12)
						   << std::setprecision(5) << r->mean_.value_or(0.0) << " Std=" << std::setw(12)
						   << std::setprecision(4) << r->stdDev_.value_or(0.0) << std::defaultfloat << '\n';
					}
	}
} // namespace pod
