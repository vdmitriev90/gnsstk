#include "ComputeStatistic.h"

using namespace gnsstk;

namespace pod
{
    // method to compute correlation matrix from variance-covariance one
    static Matrix<double> corrMatrix(const Matrix<double>& covar)
    {
        Matrix<double> corr(covar.rows(), covar.cols(), .0);

        for (size_t i = 0; i < covar.rows(); i++)
            for (size_t j = 0; j < covar.cols(); j++)
                corr(i, j) = covar(i, j) / sqrt(covar(i, i)) / sqrt(covar(j, j));
        return corr;
    }

    // rotation matrix from ECEF (XYZ) to local North-East-Up at the given position
    static Matrix<double> xyzToNEURotation(const Vector<double>& xyz)
    {
        Position pos(xyz[0], xyz[1], xyz[2]);
        const double phi = pos.getGeodeticLatitude() * DEG_TO_RAD;
        const double lam = pos.getLongitude() * DEG_TO_RAD;

        Matrix<double> rot(3, 3, .0);
        rot(0, 0) = -sin(phi) * cos(lam);
        rot(0, 1) = -sin(phi) * sin(lam);
        rot(0, 2) = cos(phi);
        rot(1, 0) = -sin(lam);
        rot(1, 1) = cos(lam);
        rot(1, 2) = 0.0;
        rot(2, 0) = cos(phi) * cos(lam);
        rot(2, 1) = cos(phi) * sin(lam);
        rot(2, 2) = sin(phi);
        return rot;
    }

    // covariance transformed from XYZ to NEU: C_neu = R * C_xyz * R^T
    static Matrix<double> covarToNEU(const Matrix<double>& covar, const Matrix<double>& rot)
    {
        auto full = [&covar](size_t k, size_t l) { return l <= k ? covar(k, l) : covar(l, k); };

        Matrix<double> neu(3, 3, .0);
        for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 3; j++)
                for (size_t k = 0; k < 3; k++)
                    for (size_t l = 0; l < 3; l++)
                        neu(i, j) += rot(i, k) * full(k, l) * rot(j, l);
        return neu;
    }

    void PositionStatistic::compute(const GnssEpochMap& data,
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
        covarNEU = covarToNEU(covar, xyzToNEURotation(sln));
    }

    double PositionStatistic::getHorStdDev() const
    {
        return sqrt(covarNEU(0, 0) + covarNEU(1, 1));
    }

    double PositionStatistic::getVertStdDev() const
    {
        return sqrt(covarNEU(2, 2));
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

    ElevationResidualStatistic::ElevationResidualStatistic(TypeID type, double minElevation, double maxElevation, double stepDeg)
        : residualType(type), minElev(minElevation), step(stepDeg)
    {
        for (double lo = minElevation; lo < maxElevation; lo += stepDeg)
        {
            Bin b;
            b.minElev = lo;
            b.maxElev = std::min(lo + stepDeg, maxElevation);
            binList.push_back(b);
        }
    }

    void ElevationResidualStatistic::compute(const GnssEpochMap& data)
    {
        struct SysValues
        {
            std::vector<double> gps, glo, gal;
        };
        std::vector<SysValues> values(binList.size());

        for (const auto& epoch : data)
        {
            for (const auto& sat : epoch.second.satData)
            {
                const auto& typeValues = sat.second;
                const auto resIt = typeValues.find(residualType);
                const auto elIt = typeValues.find(TypeID::elevation);
                if (resIt == typeValues.end() || elIt == typeValues.end())
                    continue;

                const double elev = elIt->second;
                if (elev < minElev)
                    continue;

                const size_t idx = static_cast<size_t>((elev - minElev) / step);
                if (idx >= values.size())
                    continue;

                switch (sat.first.system)
                {
                case SatelliteSystem::GPS:
                    values[idx].gps.push_back(resIt->second);
                    break;
                case SatelliteSystem::Glonass:
                    values[idx].glo.push_back(resIt->second);
                    break;
                case SatelliteSystem::Galileo:
                    values[idx].gal.push_back(resIt->second);
                    break;
                default:
                    break;
                }
            }
        }

        for (size_t i = 0; i < binList.size(); i++)
        {
            PostfitResidualStatistic::updateStatistic(values[i].gps, binList[i].gps);
            PostfitResidualStatistic::updateStatistic(values[i].glo, binList[i].glo);
            PostfitResidualStatistic::updateStatistic(values[i].gal, binList[i].gal);
        }
    }

    void ElevationResidualStatistic::print(std::ostream& os, const std::string& title) const
    {
        auto printStat = [&os](const std::string& name, const PostfitResidualStatistic::Statistic& s)
        {
            if (s.count == 0)
                return;

            os << std::left << std::setw(20) << name << " N=" << std::setw(8) << s.count << std::fixed
               << " Mean=" << std::setw(12) << std::setprecision(5) << s.mean
               << " Std=" << std::setw(12) << std::setprecision(4) << s.stddev << std::defaultfloat << '\n';
        };

        for (const auto& b : binList)
        {
            const std::string range =
                std::to_string(static_cast<int>(b.minElev)) + "-" + std::to_string(static_cast<int>(b.maxElev));
            printStat(title + "_GPS_" + range, b.gps);
            printStat(title + "_GLO_" + range, b.glo);
            printStat(title + "_GAL_" + range, b.gal);
        }
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
