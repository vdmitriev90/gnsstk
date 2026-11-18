#include "IonoStochasticModel.h"

using namespace gnsstk;

namespace pod
{
    void IonoStochasticModel::Prepare(const gnsstk::SatID& sat, gnsstk::IRinex& gData)
    {
        RandomWalkModel::Prepare(sat, gData);

        try
        {
            csFlag = (gData.getBody().getValue(sat, TypeID::CSL1) > 0.0);
            el = gData.getBody().getValue(sat, TypeID::elevation);
            return;
        }
        catch (SatIDNotFound& e)
        {
        }
        catch (TypeIDNotFound& e)
        {
        }

        el = 0;
        csFlag = true;
    }

    double IonoStochasticModel::getQ() const
    {
        if (csFlag)
        {
            // re-initialization: same variance as initial ionosphere state, m^2
            return kIonoInitVariance;
        }
        else
        {
            // single-layer ionosphere mapping function (H = 450 km)
            constexpr double Re = 6371.0;
            constexpr double Hion = 450.0;
            const double s = Re / (Re + Hion) * cos(el * PI / 180.0);
            const double mf2 = 1.0 / (1.0 - s * s);
            return RandomWalkModel::getQ() * mf2;
        }
    }

    double IonoStochasticModel::getPhi() const
    {
        return csFlag ? 0 : 1;
    }

} // namespace pod