
#include "PositionStatistic.h"

namespace pod
{
    namespace
    {
        namespace
        {
            struct PositionOutlierThreshold
            {
                double horizontal; // m
                double vertical;   // m
            };

            constexpr PositionOutlierThreshold getPositionOutlierThreshold(const SlnType solutionType)
            {
                switch (solutionType)
                {
                case SlnType::Standalone:
                    return {10.0, 20.0};

                case SlnType::CodeDiff:
                    return {3.0, 4.0};

                case SlnType::PdFloat:
                    return {1.0, 2.0};

                case SlnType::PdFixed:
                    return {0.15, 0.30};

                case SlnType::PppFloat:
                    return {0.20, 0.40};

                case SlnType::PppFixed:
                    return {0.10, 0.20};

                case SlnType::None:
                default:
                    return {0.0, 0.0};
                }
            }
            bool isPositionOutlier(const Triple& position,
                                   const Triple& mean,
                                   const Matrix<double>& xyzToNeu,
                                   const SlnType solutionType)
            {
                const auto threshold = getPositionOutlierThreshold(solutionType);

                const double dx = position[0] - mean[0];
                const double dy = position[1] - mean[1];
                const double dz = position[2] - mean[2];

                const double north = xyzToNeu(0, 0) * dx + xyzToNeu(0, 1) * dy + xyzToNeu(0, 2) * dz;
                const double east = xyzToNeu(1, 0) * dx + xyzToNeu(1, 1) * dy + xyzToNeu(1, 2) * dz;
                const double up = xyzToNeu(2, 0) * dx + xyzToNeu(2, 1) * dy + xyzToNeu(2, 2) * dz;
                const double horizontalError = std::hypot(north, east);

                return horizontalError > threshold.horizontal || std::abs(up) > threshold.vertical;
            }
        } // namespace

        constexpr double sigmaFloor = 0.01; // m

        struct WeightedPosition
        {
            Triple weightedSum{0.0, 0.0, 0.0};
            Triple weightSum{0.0, 0.0, 0.0};
        };

        constexpr TypeID::ValueType coordinateTypes[3] = {TypeID::recX, TypeID::recY, TypeID::recZ};
        constexpr TypeID::ValueType sigmaTypes[3] = {TypeID::recStDevX, TypeID::recStDevY, TypeID::recStDevZ};

        bool isRequestedSolutionType(const GnssEpoch& epoch, const SlnType requestedType)
        {
            try
            {
                const SlnType currentType = static_cast<SlnType>(epoch.slnData.getValue(TypeID::recSlnType));
                return currentType == requestedType;
            }
            catch (...)
            {
                return false;
            }
        }

        bool accumulateEpoch(const GnssEpoch& epoch, WeightedPosition& accumulator)
        {
            for (size_t coordinateIndex = 0; coordinateIndex < 3; ++coordinateIndex)
            {
                try
                {
                    const double coordinate = epoch.slnData.getValue(TypeID(coordinateTypes[coordinateIndex]));
                    const double sigma = epoch.slnData.getValue(TypeID(sigmaTypes[coordinateIndex]));

                    if (!std::isfinite(coordinate) || !std::isfinite(sigma) || sigma <= 0.0)
                    {
                        return false;
                    }

                    const double effectiveSigma = std::max(sigma, sigmaFloor);
                    const double variance = effectiveSigma * effectiveSigma;
                    const double weight = 1.0 / variance;

                    accumulator.weightedSum[coordinateIndex] += weight * coordinate;
                    accumulator.weightSum[coordinateIndex] += weight;
                }
                catch (...)
                {
                    return false;
                }
            }

            return true;
        }

        bool computeWeightedSolution(const WeightedPosition& accumulator, Triple& solution)
        {
            for (size_t i = 0; i < 3; ++i)
            {
                const double weightSum = accumulator.weightSum[i];

                if (!std::isfinite(weightSum) || weightSum <= 0.0)
                {
                    return false;
                }

                solution[i] = accumulator.weightedSum[i] / weightSum;
            }

            return true;
        }

        /*
         * Ковариация по внутренней сходимости: выборочный разброс эпохальных решений
         * относительно среднего. Формальные sigma используются только как веса среднего.
         */
        CovarianceMatrix3 computeScatterCovariance(const std::vector<Triple>& positions, const Triple& mean)
        {
            CovarianceMatrix3 covariance{};

            if (positions.size() < 2)
                return covariance;

            for (const Triple& p : positions)
            {
                const double dx = p[0] - mean[0];
                const double dy = p[1] - mean[1];
                const double dz = p[2] - mean[2];

                covariance.xx += dx * dx;
                covariance.yy += dy * dy;
                covariance.zz += dz * dz;
                covariance.xy += dx * dy;
                covariance.xz += dx * dz;
                covariance.yz += dy * dz;
            }

            const double n = static_cast<double>(positions.size()) - 1.0;
            covariance.xx /= n;
            covariance.yy /= n;
            covariance.zz /= n;
            covariance.xy /= n;
            covariance.xz /= n;
            covariance.yz /= n;

            return covariance;
        }

    } // namespace

    // rotation matrix from ECEF (XYZ) to local North-East-Up at the given position
    static Matrix<double> xyzToNEURotation(const Triple& xyz)
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
    static Matrix<double> covarToNEU(const CovarianceMatrix3& covar, const Matrix<double>& rot)
    {
        const double full[3][3] = {
            {covar.xx, covar.xy, covar.xz}, {covar.xy, covar.yy, covar.yz}, {covar.xz, covar.yz, covar.zz}};

        Matrix<double> neu(3, 3, .0);
        for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 3; j++)
                for (size_t k = 0; k < 3; k++)
                    for (size_t l = 0; l < 3; l++)
                        neu(i, j) += rot(i, k) * full[k][l] * rot(j, l);
        return neu;
    }

    SlnType PositionStatistic::getSlnType() const noexcept
    {
        return solutionType_;
    }

    PositionStatisticResult PositionStatistic::compute(GnssEpochMap& data) const
    {
        PositionStatisticResult result;
        result.totalEpochs = data.size();

        struct EpochSample
        {
            GnssEpoch* epoch = nullptr;
            Triple position;
            Triple sigma;
            Triple weightedSum;
            Triple weightSum;
        };

        std::vector<EpochSample> samples;
        WeightedPosition preliminary;
        double inUse(0.0);

        for (auto& ep : data)
        {
            auto& epoch = ep.second;

            if (!isRequestedSolutionType(epoch, solutionType_))
                continue;

            epoch.isValidSolution_ = false;
            WeightedPosition epochAccumulator;

            ++result.fixEpochs;
            inUse += epoch.slnData.at(TypeID::recUsedSV);

            if (!accumulateEpoch(epoch, epochAccumulator))
                continue;

            EpochSample s;
            s.epoch = &epoch;
            s.weightedSum = epochAccumulator.weightedSum;
            s.weightSum = epochAccumulator.weightSum;
            for (size_t i = 0; i < 3; ++i)
            {
                s.position[i] = s.weightedSum[i] / s.weightSum[i];
                s.sigma[i] = 1.0 / std::sqrt(s.weightSum[i]); // effective sigma
                preliminary.weightedSum[i] += s.weightedSum[i];
                preliminary.weightSum[i] += s.weightSum[i];
            }
            samples.push_back(s);
        }

        if (result.fixEpochs > 0)
        {
            result.avgSvInUse = inUse / static_cast<double>(result.fixEpochs);
        }

        Triple preliminaryMean;
        if (samples.empty() || !computeWeightedSolution(preliminary, preliminaryMean))
            return result;

        const auto xyzToNeu = xyzToNEURotation(preliminaryMean);
        // single-pass 4 sigma rejection relative to the preliminary weighted mean
        WeightedPosition accumulator;
        std::vector<Triple> positions;

        for (const auto& s : samples)
        {
            bool outlier = isPositionOutlier(s.position, preliminaryMean, xyzToNeu, solutionType_);

            if (outlier)
            {
                ++result.outlierEpochs;
                s.epoch->isValidSolution_ = false;
                continue;
            }

            s.epoch->isValidSolution_ = true;
            for (size_t i = 0; i < 3; ++i)
            {
                accumulator.weightedSum[i] += s.weightedSum[i];
                accumulator.weightSum[i] += s.weightSum[i];
            }
            positions.push_back(s.position);
        }

        if (positions.empty() || !computeWeightedSolution(accumulator, result.averagePosition))
            return result;

        result.covarianceEcef = computeScatterCovariance(positions, result.averagePosition);

        const Matrix<double> covarNEU = covarToNEU(result.covarianceEcef, xyzToNEURotation(result.averagePosition));
        result.horizontalStdDev = std::sqrt(covarNEU(0, 0) + covarNEU(1, 1));
        result.verticalStdDev = std::sqrt(covarNEU(2, 2));
        result.valid = true;

        return result;
    }

    PositionStatistic::PositionStatistic(SlnType solutionType) : solutionType_(solutionType) {}

    PositionStatisticResult computeSolutionSummary(GnssEpochMap& gnss_data, SlnType slnType)
    {
        const PositionStatistic rms_st(slnType);
        return rms_st.compute(gnss_data);
    }

} // namespace pod