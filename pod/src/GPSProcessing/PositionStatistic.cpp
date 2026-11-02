
#include "PositionStatistic.h"

namespace pod
{
    namespace
    {
        constexpr double sigmaFloor = 0.01; // m

        struct WeightedPosition
        {
            Triple weightedSum{0.0, 0.0, 0.0};
            Triple weightSum{0.0, 0.0, 0.0};
        };

        TypeID getSigmaType(const TypeID& coordinateType)
        {
            switch (coordinateType.type)
            {
            case TypeID::recX:
                return TypeID::recStDevX;

            case TypeID::recY:
                return TypeID::recStDevY;

            case TypeID::recZ:
                return TypeID::recStDevZ;

            default:
                throw Exception("PositionStatistic: unsupported coordinate TypeID");
            }
        }

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

        bool accumulateEpoch(const GnssEpoch& epoch, const TypeIDSet& types, WeightedPosition& accumulator)
        {
            size_t coordinateIndex = 0;

            for (const TypeID& coordinateType : types)
            {
                if (coordinateIndex >= 3)
                    return false;

                try
                {
                    const TypeID sigmaType = getSigmaType(coordinateType);
                    const double coordinate = epoch.slnData.getValue(coordinateType);
                    const double sigma = epoch.slnData.getValue(sigmaType);

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

                ++coordinateIndex;
            }

            return coordinateIndex == 3;
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

        CovarianceMatrix3 computeMeanCovariance(const WeightedPosition& accumulator)
        {
            CovarianceMatrix3 covariance{};

            /*
             * Формальная дисперсия взвешенного среднего:
             *
             * Var(mean) = 1 / sum(weights)
             */
            covariance.xx = 1.0 / accumulator.weightSum[0];
            covariance.yy = 1.0 / accumulator.weightSum[1];
            covariance.zz = 1.0 / accumulator.weightSum[2];

            /*
             * Корреляции между X, Y и Z неизвестны,
             * поскольку для каждой эпохи сохранены только
             * recStDevX, recStDevY и recStDevZ.
             */
            covariance.xy = 0.0;
            covariance.xz = 0.0;
            covariance.yz = 0.0;

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

    SlnType PositionStatistic::getSlnType() const
    {
        return slnType;
    }
    TypeIDSet PositionStatistic::getTypeIDSet() const
    {
        return types;
    }
    void PositionStatistic::compute(const GnssEpochMap& data, Triple& avgPos, CovarianceMatrix3& covar)
    {
        avgPos = Triple(0.0, 0.0, 0.0);
        covar = CovarianceMatrix3{};
        covarNEU = Matrix<double>(3, 3, 0.0);

        goodEpochs = 0;
        totalEpochs = static_cast<int>(data.size());

        if (types.size() != 3)
            return;

        WeightedPosition accumulator;

        for (const auto& ep : data)
        {
            const auto& epoch = ep.second;

            if (!isRequestedSolutionType(epoch, slnType))
            {
                continue;
            }

            /*
             * Сначала накапливаем эпоху во временный объект.
             * Это не позволяет частично добавить X и Y,
             * если чтение Z или sigma Z завершилось ошибкой.
             */
            WeightedPosition epochAccumulator;

            if (!accumulateEpoch(epoch, types, epochAccumulator))
            {
                continue;
            }

            for (size_t i = 0; i < 3; ++i)
            {
                accumulator.weightedSum[i] += epochAccumulator.weightedSum[i];
                accumulator.weightSum[i] += epochAccumulator.weightSum[i];
            }

            ++goodEpochs;
        }

        if (goodEpochs == 0)
            return;

        if (!computeWeightedSolution(accumulator, avgPos))
        {
            goodEpochs = 0;
            return;
        }

        covar = computeMeanCovariance(accumulator);

        covarNEU = covarToNEU(covar, xyzToNEURotation(avgPos));
    }

    double PositionStatistic::getHorStdDev() const
    {
        return sqrt(covarNEU(0, 0) + covarNEU(1, 1));
    }

    double PositionStatistic::getVertStdDev() const
    {
        return sqrt(covarNEU(2, 2));
    }

    PositionStatistic::PositionStatistic(SlnType st, TypeIDSet tIDs) : slnType(st), types(std::move(tIDs)) {}

} // namespace pod