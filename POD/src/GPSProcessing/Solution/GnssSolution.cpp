#include "GnssSolution.h"

#include "KalmanSolver.h"
#include "PowerSum.hpp"

using namespace gnsstk;

namespace pod
{
    std::ostream& GnssSolution::printMsg(const gnsstk::CommonTime& time, const char* msg)
    {
        return (std::cout << "Epoch: " << CivilTime(time).asString() << " " << msg << std::endl);
    }

    GnssSolution::GnssSolution(GnssDataStorePtr gnssData, double sigma = 50.0)
        : data_(gnssData)
        , equations_(std::make_shared<EquationComposer>())
        , maxSigma_(sigma)
    {
    }

    GnssSolution::~GnssSolution() {}

    void GnssSolution::printSolution(const KalmanSolver& solver,
                                     const gnsstk::CommonTime& time,
                                     GnssEpoch& gEpoch)
    {

        for (auto&& it : equations_->currentUnknowns())
        {
            if (it.type == TypeID::dx || it.type == TypeID::dy || it.type == TypeID::dz)
                continue;

            if (it.sv == SatID::dummy)
            {
                if (it.type == TypeID::cdt)
                    gEpoch.slnData[TypeID::recCdt] = solver.getSolution(it);
                else if (it.type == TypeID::wetMap)
                    gEpoch.slnData[TypeID::recZTropo] = solver.getSolution(it);
                else if (it.type == TypeID::wetMapNorth)
                    gEpoch.slnData[TypeID::recTropoNorth] = solver.getSolution(it);
                else if (it.type == TypeID::wetMapEast)
                    gEpoch.slnData[TypeID::recTropoEast] = solver.getSolution(it);
                else
                    gEpoch.slnData[it.type] = solver.getSolution(it);
            }
            else
            {
                double amb = solver.getSolution(it);
                if (amb != 0)
                    gEpoch.satData[it.sv][it.type] = amb;
            }
        }
        Position new_pos;
        double st_dev3_d(NAN);

        new_pos[0] = nominalPos_.X() + solver.getSolution(FilterParameter(TypeID::dx)); // dx    - #4
        new_pos[1] = nominalPos_.Y() + solver.getSolution(FilterParameter(TypeID::dy)); // dy    - #5
        new_pos[2] = nominalPos_.Z() + solver.getSolution(FilterParameter(TypeID::dz)); // dz    - #6

        double var_x = solver.getVariance(FilterParameter(TypeID::dx)); // Cov dx    - #8
        double var_y = solver.getVariance(FilterParameter(TypeID::dy)); // Cov dy    - #9
        double var_z = solver.getVariance(FilterParameter(TypeID::dz)); // Cov dz    - #10
        st_dev3_d = sqrt(var_x + var_y + var_z);

        gEpoch.slnData.insert(std::make_pair(TypeID::recX, new_pos.X()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recY, new_pos.Y()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recZ, new_pos.Z()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recStDev3D, st_dev3_d));

        // number of used sats = number of residuals/number of measurement types
        int num_used_sats = solver.PostfitResiduals().size() / equations_->measTypes().size();
        gEpoch.slnData.insert(std::make_pair(TypeID::recUsedSV, num_used_sats));

        SlnType sln_type = solver.getValid() ? desiredSlnType() : SlnType::NONE_SOLUTION;

        gEpoch.slnData.insert(std::make_pair(TypeID::recSlnType, sln_type));
        gEpoch.slnData.insert(std::make_pair(TypeID::sigma, solver.getPhaseSigma()));
    };
} // namespace pod
