#pragma once
#include "GnssEpochMap.h"
#include "GnssSolution.h"

#include <iostream>

namespace pod
{
    class ComputeStatistic
    {

      public:
        // method to compute correlation matrix from variance-covariance one
        static gnsstk::Matrix<double> corrMatrix(const gnsstk::Matrix<double>& covar);

        ComputeStatistic(SlnType st, gnsstk::TypeIDSet tIDs) : slnType(st), types(tIDs), goodEpochs(0) {};
        ~ComputeStatistic() {};

        SlnType getSlnType()
        {
            return slnType;
        }

        gnsstk::TypeIDSet getTypeIDSet()
        {
            return types;
        }

        void compute(const GnssEpochMap& data, gnsstk::Vector<double>& sln, gnsstk::Matrix<double>& cov);
        int goodEpochs;
        int totalEpochs;

      private:
        SlnType slnType;
        gnsstk::TypeIDSet types;
    };

    class PostfitResidualStatistic
    {
      public:
        struct Statistic
        {
            int count = 0;

            double mean = 0.0;
            double stddev = 0.0;

            double minVal = 0.0;
            double maxVal = 0.0;
        };

        explicit PostfitResidualStatistic(gnsstk::TypeID type) : residualType(type) {}

        void compute(const GnssEpochMap& data);

        const Statistic& gps() const
        {
            return gpsStat;
        }
        const Statistic& glo() const
        {
            return gloStat;
        }
        const Statistic& gal() const
        {
            return galStat;
        }

      private:
        void updateStatistic(const std::vector<double>& values, Statistic& stat);

        gnsstk::TypeID residualType;

        Statistic gpsStat;
        Statistic gloStat;
        Statistic galStat;
    };

    class PostfitResidualStatistics
    {
        struct Item
        {
            std::string name;
            PostfitResidualStatistic statistic;

            Item(const std::string& n, gnsstk::TypeID type) : name(n), statistic(type) {}
        };

      public:
        PostfitResidualStatistics()
        {
            add("PC", gnsstk::TypeID::postfitPC);

            add("C1", gnsstk::TypeID::postfitC1);
            add("P1", gnsstk::TypeID::postfitP1);
            add("P2", gnsstk::TypeID::postfitP2);
            add("C5", gnsstk::TypeID::postfitC5);
            add("C6", gnsstk::TypeID::postfitC6);
            add("C7", gnsstk::TypeID::postfitC7);
            add("C8", gnsstk::TypeID::postfitC8);

            add("L1", gnsstk::TypeID::postfitL1);
            add("L2", gnsstk::TypeID::postfitL2);
            add("L5", gnsstk::TypeID::postfitL5);
            add("L6", gnsstk::TypeID::postfitL6);
            add("L7", gnsstk::TypeID::postfitL7);
            add("L8", gnsstk::TypeID::postfitL8);

            add("LC", gnsstk::TypeID::postfitLC);
        }

        void compute(const GnssEpochMap& data)
        {
            for (auto& item : stats)
            {
                item.statistic.compute(data);
            }
        }

        void print(std::ostream& os) const
        {
        auto printStat =
            [&os](const std::string& title, const PostfitResidualStatistic::Statistic& s)
        {
                if (s.count == 0)
                    return;

                os << std::left << std::setw(12) << title << " N=" << std::setw(8) << s.count
                   << " Mean=" << std::setw(12) << s.mean 
                   << " Std=" << std::setw(12) << s.stddev << '\n';
        };

        for (const auto& item : stats)
        {
                printStat(item.name + "_GPS", item.statistic.gps());
                printStat(item.name + "_GLO", item.statistic.glo());
                printStat(item.name + "_GAL", item.statistic.gal());
        }
        }

      private:
        void add(const std::string& name, gnsstk::TypeID type)
        {
            stats.emplace_back(name, type);
        }

        std::vector<Item> stats;
    };

} // namespace pod
