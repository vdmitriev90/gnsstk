#include "ComputeStatistic.h"

using namespace gnsstk;

namespace pod
{
    // method to compute correlation matrix from variance-covariance one
    Matrix<double> ComputeStatistic::corrMatrix(const Matrix<double>& covar)
    {
        Matrix<double> corr(covar.rows(), covar.cols(), .0);

        for (size_t i = 0; i < covar.rows(); i++)
            for (size_t j = 0; j < covar.cols(); j++)
                corr(i, j) = covar(i, j) / sqrt(covar(i, i)) / sqrt(covar(j, j));
        return corr;
    }

    void ComputeStatistic::compute(const GnssEpochMap& data,
                                   gnsstk::Vector<double>& sln,
                                   gnsstk::Matrix<double>& covar)
    {
        int s = types.size();
        std::list<Vector<double>> res;
        sln.resize(s, .0);
        covar = Matrix<double>(s, s, .0);
        int i(0);

        // for (auto ep = begin(data); ep != end(data); ep++)
        for (auto&& ep : data)
        {
            SlnType curSt = (SlnType)(int)ep.second.slnData.getValue(TypeID::recSlnType);

            if (curSt == slnType)
            {
                i++;
                Vector<double> resi(s, .0);
                int j(0);
                for (auto&& it : types)
                {
                    resi[j] = ep.second.slnData.getValue(it);
                    sln[j] = sln[j] * (i - 1) / i + resi[j] / i;
                    j++;
                }
                res.push_back(resi);
            }
        }
        goodEpochs = i;
        totalEpochs = data.size();

        // calculate sum squares
        for (auto&& it : res)
        {
            for (size_t k = 0; k < s; k++)
                for (size_t l = 0; l <= k; l++)
                {
                    double d1 = it[k] - sln[k];
                    double d2 = it[l] - sln[l];
                    covar(k, l) += d1 * d2;
                }
        }

        for (size_t k = 0; k < s; k++)
            for (size_t l = 0; l <= k; l++)
                covar(k, l) = covar(k, l) / (res.size() - 1);
    }

    void PostfitResidualStatistic::compute(const GnssEpochMap& data)
    {
        std::vector<double> gpsValues;
        std::vector<double> gloValues;
        std::vector<double> galValues;

        for (const auto& epoch : data)
        {
            const auto& satData = epoch.second.satData;

            for (const auto& sat : satData)
            {
                const SatID& satId = sat.first;
                const auto& values = sat.second;
                const auto it = values.find(residualType);
                if (it == values.end())
                {
                    continue;
                }

                double residual = it->second;

                switch (satId.system)
                {
                case SatelliteSystem::GPS:
                    gpsValues.push_back(residual);
                    break;

                case SatelliteSystem::Glonass:
                    gloValues.push_back(residual);
                    break;

                case SatelliteSystem::Galileo:
                    galValues.push_back(residual);
                    break;

                default:
                    break;
                }
            }
        }

        updateStatistic(gpsValues, gpsStat);
        updateStatistic(gloValues, gloStat);
        updateStatistic(galValues, galStat);
    }

    void PostfitResidualStatistic::updateStatistic(const std::vector<double>& values, Statistic& stat)
    {
        if (values.empty())
        {
            stat = Statistic();
            return;
        }

        stat.count = static_cast<int>(values.size());

        double sum = 0.0;
        double sum2 = 0.0;

        stat.minVal = values.front();
        stat.maxVal = values.front();

        for (double v : values)
        {
            sum += v;
            sum2 += v * v;

            stat.minVal = std::min(stat.minVal, v);
            stat.maxVal = std::max(stat.maxVal, v);
        }

        stat.mean = sum / stat.count;

        double var = 0.0;

        for (double v : values)
        {
            double d = v - stat.mean;
            var += d * d;
        }

        stat.stddev = (stat.count > 1) ? std::sqrt(var / (stat.count - 1)) : 0.0;
    }
} // namespace pod
