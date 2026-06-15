#include "AmbiguitiesEquations.h"

#include "GNSSconstants.hpp"
#include "LinearCombination.h"

using namespace gnsstk;

namespace pod
{
    constexpr double kAmbDefSigma = 2e7;

    constexpr gnsstk::TypeID::ValueType mapObsToPrefit(const gnsstk::TypeID& obsType)
    {
        switch (obsType.type)
        {

        case gnsstk::TypeID::BL1:
            return gnsstk::TypeID::prefitL1;
        case gnsstk::TypeID::BL2:
            return gnsstk::TypeID::prefitL2;
        case gnsstk::TypeID::BL5:
            return gnsstk::TypeID::prefitL5;
        case gnsstk::TypeID::BL6:
            return gnsstk::TypeID::prefitL6;
        case gnsstk::TypeID::BL7:
            return gnsstk::TypeID::prefitL7;
        case gnsstk::TypeID::BL8:
            return gnsstk::TypeID::prefitL8;

        case gnsstk::TypeID::BLC:
            return gnsstk::TypeID::prefitLC;

        default:
            GNSSTK_ASSERT_MSG(
                false, "Unknown observation type for ambiguities equations: " + gnsstk::StringUtils::asString(obsType));
            return gnsstk::TypeID::Unknown;
        }
    }

    ParametersSet AmbiguitiesEquations::getParameters() const
    {
        ParametersSet ambSet;
        for (const auto& it : csFlags_)
            ambSet.insert(FilterParameter(ambType_, it.first));
        return ambSet;
    }

    AmbiguitiesEquations::AmbiguitiesEquations(const gnsstk::TypeID& ambType)
        : ambType_(ambType)
        , obsType_(mapObsToPrefit(ambType))
    {
        stochModel_.setSigma(kAmbDefSigma);
    }

    void AmbiguitiesEquations::prepare(gnsstk::IRinex& gData)
    {
        svsInView_ = gData.getBody().getSatID();

        // update satellites set
        satSet_.insert(svsInView_.cbegin(), svsInView_.cend());

        csFlags_.clear();

        for (const auto& it : satSet_)
        {
            stochModel_.Prepare(it, gData);
            csFlags_[it] = stochModel_.getCS();
        }
        satSet_ = svsInView_;
    }

    void AmbiguitiesEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, int& index) const
    {
        for (auto& it : csFlags_)
        {
            stochModel_.setCS(it.second);
            Phi(index, index) = stochModel_.getPhi();
            ++index;
        }
    }

    void AmbiguitiesEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, int& index) const
    {
        for (auto& it : csFlags_)
        {
            stochModel_.setCS(it.second);
            Q(index, index) = stochModel_.getQ();
            ++index;
        }
    }

    void AmbiguitiesEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
                                                     gnsstk::Matrix<double>& P,
                                                     int& index) const
    {
        for (auto& it : csFlags_)
        {
            x(index) = 0;
            P(index, index) = 4e10;
            ++index;
        }
    }

    void AmbiguitiesEquations::contributeDesignMatrix(const gnsstk::IRinex& gData,
                                                      const gnsstk::TypeIDSet& types,
                                                      gnsstk::Matrix<double>& H,
                                                      int& startColumn)
    {
        // total number of  ambiguities
        int numAmbs(csFlags_.size());

        // sv in view
        auto currentSatSet = gData.getBody().getSatID();

        // total number of sv in view
        int numSv(currentSatSet.size());

        int row(0);
        for (const auto& t : types)
        {
            if (obsType_ != t)
            {
                row += numSv;
                continue;
            }

            // Now, fill the coefficients related to phase biases
            // We must be careful because not all processed satellites
            // are currently visible
            for (const auto& itSat : currentSatSet)
            {
                // Find in which position of 'satSet' is the current '(*itSat)'
                // Please note that 'currSatSet' is a subset of 'satSet'

                auto itSat2 = csFlags_.find(itSat);
                if (itSat2 == csFlags_.end())
                    continue; // assert / throw?
                const int j = std::distance(csFlags_.begin(), itSat2);

                double wavelength(0);
                int fcn = itSat.getGloFcn();
                switch (ambType_.type)
                {
                case TypeID::BL1:
                    wavelength = getWavelength(itSat.system, 1, fcn);
                    break;
                case TypeID::BL2:
                    wavelength = getWavelength(itSat.system, 2, fcn);
                    break;
                case TypeID::BLC:
                    wavelength = 1.0; // LinearCombination::getIonoFreeWaveLength(itSat, 1, 2);
                    break;
                default:
                    break;
                }

                // Put coefficient in the right place
                H(row, j + startColumn) = wavelength;

                ++row;
            }

            startColumn += numAmbs;
        }
        satSet_ = currentSatSet;
    }

    int AmbiguitiesEquations::getNumUnknowns() const
    {
        return csFlags_.size();
    }
} // namespace pod