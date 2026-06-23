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

    void GnssSolution::computeAndStoreSolution(const KalmanSolver& solver, GnssEpoch& gEpoch) const
    {
        if (!solver.getValid())
        {
            gEpoch.slnData.insert(std::make_pair(TypeID::recSlnType, static_cast<int>(SlnType::None)));
            return;
        }

        Position new_pos = nominalPos_;
        new_pos[0] += solver.getSolution(FilterParameter(TypeID::dx)); // dx
        new_pos[1] += solver.getSolution(FilterParameter(TypeID::dy)); // dy
        new_pos[2] += solver.getSolution(FilterParameter(TypeID::dz)); // dz

        double var_x = solver.getVariance(FilterParameter(TypeID::dx)); // Cov dx
        double var_y = solver.getVariance(FilterParameter(TypeID::dy)); // Cov dy
        double var_z = solver.getVariance(FilterParameter(TypeID::dz)); // Cov dz
        const double st_dev3_d = sqrt(var_x + var_y + var_z);

        gEpoch.slnData.insert(std::make_pair(TypeID::recX, new_pos.X()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recY, new_pos.Y()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recZ, new_pos.Z()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recStDev3D, st_dev3_d));

        // number of used sats = number of residuals/number of measurement types
        int num_used_sats = solver.PostfitResiduals().size() / equations_->getMeasTypes().size();
        gEpoch.slnData.insert(std::make_pair(TypeID::recUsedSV, num_used_sats));

        gEpoch.slnData.insert(std::make_pair(TypeID::recSlnType, static_cast<int>(desiredSlnType())));
        gEpoch.slnData.insert(std::make_pair(TypeID::sigma, solver.getPhaseSigma()));
    }

    void GnssSolution::storeReceiverParams(const KalmanSolver& solver,
                                            const FilterParameter& param,
                                            GnssEpoch& gEpoch) const
    {
        if (param.type == TypeID::cdt)
            gEpoch.slnData[TypeID::recCdt] = solver.getSolution(param);
        else if (param.type == TypeID::wetMap)
            gEpoch.slnData[TypeID::recZTropo] = solver.getSolution(param);
        else if (param.type == TypeID::wetMapNorth)
            gEpoch.slnData[TypeID::recTropoNorth] = solver.getSolution(param);
        else if (param.type == TypeID::wetMapEast)
            gEpoch.slnData[TypeID::recTropoEast] = solver.getSolution(param);
        else
            gEpoch.slnData[param.type] = solver.getSolution(param);
    }

    void GnssSolution::storeSatelliteParams(const KalmanSolver& solver,
                                            const FilterParameter& param,
                                            GnssEpoch& gEpoch) const
    {
        const double amb = solver.getSolution(param);
        if (amb != 0)
            gEpoch.satData[param.sv][param.type] = amb;
    }

    void GnssSolution::printSolution(const KalmanSolver& solver,
                                     const gnsstk::CommonTime& time,
                                     GnssEpoch& gEpoch) const
    {
        const auto& layout = equations_->getLayout();

        for (size_t i = 0; i < layout.size(); ++i)
        {
            const auto& it = layout.param(static_cast<int>(i));

            if (it.type == TypeID::dx || it.type == TypeID::dy || it.type == TypeID::dz)
                continue;

            if (it.sv == SatID::dummy)
                storeReceiverParams(solver, it, gEpoch);
            else
                storeSatelliteParams(solver, it, gEpoch);
        }

        computeAndStoreSolution(solver, gEpoch);
    };

    std::string FileNameBuilder::getFileName() const
    {
        return isDifferential(solution_.desiredSlnType()) ? buildWithBaseAndRover()
                                                          : buildRoverOnly();
    }

    std::string FileNameBuilder::buildWithBaseAndRover() const
    {
        std::string ss_str;
        for (auto&& system : solution_.opts().systems)
            ss_str += '_' + convertSatelliteSystemToCode(system);

        return solution_.opts().SiteBase + "-" + solution_.opts().SiteRover + "_"
               + slnType2Str.at(solution_.desiredSlnType()) + ss_str;
    }

    std::string FileNameBuilder::buildRoverOnly() const
    {
        std::string ss_str;
        for (auto&& system : solution_.opts().systems)
            ss_str += '_' + convertSatelliteSystemToCode(system);

        return solution_.opts().SiteRover + "_" + slnType2Str.at(solution_.desiredSlnType()) + ss_str;
    }
} // namespace pod
