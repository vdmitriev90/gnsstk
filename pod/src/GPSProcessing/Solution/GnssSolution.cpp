#include "GnssSolution.h"

#include "KalmanSolver.h"
#include "PowerSum.hpp"
#include "TimeUtils.h"

using namespace gnsstk;

namespace pod
{
    std::ostream& GnssSolution::printMsg(const gnsstk::CommonTime& time, const char* msg)
    {
        return (std::cout << "Epoch: " << CivilTime(time).asString() << " " << msg << std::endl);
    }

    FileNameBuilder::FileNameBuilder(const GnssSolution& solution) : solution_(solution) {}

    GnssEpochMap& GnssSolution::getData()
    {
        return gMap_;
    }

    const GnssEpochMap& GnssSolution::getData() const
    {
        return gMap_;
    }

    std::string GnssSolution::getFileName() const
    {
        return FileNameBuilder(*this).getFileName();
    }

    std::string GnssSolution::getBaseFileName() const
    {
        return FileNameBuilder(*this).getBaseFileName();
    }

    SlnType GnssSolution::desiredSlnType() const
    {
        const auto& processing = processingConfig();

        switch (processing.solutionMode)
        {
        case SolutionMode::CodeAbsolute:
            return SlnType::Standalone;

        case SolutionMode::CodeDifferential:
            return SlnType::CodeDiff;

        case SolutionMode::PhaseDiff:
            return processing.integerAmbiguityResolution ? SlnType::PdFixed : SlnType::PdFloat;

        case SolutionMode::IonoFreePpp:
            return SlnType::PppFloat;

        case SolutionMode::UncombinedPpp:
            return processing.integerAmbiguityResolution ? SlnType::PppFixed : SlnType::PppFloat;

        case SolutionMode::Undefined:
        default:
            return SlnType::None;
        }
    }

    double GnssSolution::getMaxSigma() const
    {
        return maxSigma_;
    }

    GnssSolution& GnssSolution::setMaxSigma(double sigma)
    {
        maxSigma_ = sigma;
        return *this;
    }

    GnssSolution& GnssSolution::setConfigData(GnssDataStorePtr dataStore)
    {
        data_ = dataStore;
        return *this;
    }

    const config::ProjectConfig& GnssSolution::projectConfig() const
    {
        return data_->config();
    }

    const config::ProcessingConfig& GnssSolution::processingConfig() const
    {
        return projectConfig().processing;
    }

    const config::StationConfig& GnssSolution::stationConfig(const std::string& siteId) const
    {
        return projectConfig().station(siteId);
    }

    IApprPosProvider& GnssSolution::apprPos()
    {
        return *data_->apprPosProvider_;
    }

    void GnssSolution::configureIonoCorrector()
    {
        const auto type = static_cast<ComputeIonoModel::IonoModelType>(processingConfig().codeIonosphereCorrectionType);
        switch (type)
        {
        case ComputeIonoModel::Zero:
            ionoCorrector_.setZeroModel();
            break;
        case ComputeIonoModel::Klobuchar:
            ionoCorrector_.setKlobucharModel(data_->bceIonoStore_);
            break;
        case ComputeIonoModel::Ionex:
            ionoCorrector_.setIonosphereMap(data_->ionexStore_);
            break;
        case ComputeIonoModel::DualFreq:
            ionoCorrector_.setDualFreqModel();
            break;
        default:
            GNSSTK_THROW(InvalidRequest("Unknown Ionospheric model type."));
        }
    }

    void GnssSolution::setupAntenna(CorrectObservables& corr,
                                    AntexReader& antex,
                                    const std::string& siteId,
                                    const std::string& rinexAntType) const
    {
        std::string model;
        bool usePcv = true;
        bool useAzimuth = false;
        if (const auto* st = projectConfig().findStation(siteId))
        {
            model = st->antennaModel;
            usePcv = st->usePcPatterns;
            useAzimuth = st->useAzimuth;
        }
        else
        {
            model = rinexAntType;
            model.erase(model.find_last_not_of(' ') + 1);
            if (model.empty())
                throw std::runtime_error("No station config and no antenna type in RINEX header for " + siteId);
        }

        Antenna ant(antex.getAntenna(model));
        corr.setAntenna(ant);
        corr.setUsePcv(usePcv);
        corr.setUseAzimuth(useAzimuth);
    }

    GnssSolution::GnssSolution(GnssDataStorePtr gnssData, double sigma)
        : data_(gnssData)
        , equations_(std::make_shared<EquationComposer>(config_))
        , maxSigma_(sigma)
        , forwardBackwardCycles_(0)
    {
    }

    void GnssSolution::computeAndStoreSolution(const KalmanSolver& solver, GnssEpoch& gEpoch) const
    {
        if (!solver.getValid())
        {
            gEpoch.slnData.insert(std::make_pair(TypeID::recSlnType, static_cast<int>(SlnType::None)));
            gEpoch.isValidSolution_ = false;
            return;
        }

        Position new_pos = nominalPos_;
        new_pos[0] += solver.getSolution(FilterParameter(TypeID::dx)); // dx
        new_pos[1] += solver.getSolution(FilterParameter(TypeID::dy)); // dy
        new_pos[2] += solver.getSolution(FilterParameter(TypeID::dz)); // dz

        gEpoch.slnData.insert(std::make_pair(TypeID::recX, new_pos.X()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recY, new_pos.Y()));
        gEpoch.slnData.insert(std::make_pair(TypeID::recZ, new_pos.Z()));

        const double var_x = solver.getVariance(FilterParameter(TypeID::dx)); // Cov dx
        const double var_y = solver.getVariance(FilterParameter(TypeID::dy)); // Cov dy
        const double var_z = solver.getVariance(FilterParameter(TypeID::dz)); // Cov dz

        gEpoch.slnData.insert(std::make_pair(TypeID::recStDevX, sqrt(var_x)));
        gEpoch.slnData.insert(std::make_pair(TypeID::recStDevY, sqrt(var_y)));
        gEpoch.slnData.insert(std::make_pair(TypeID::recStDevZ, sqrt(var_z)));

        gEpoch.slnData.insert(std::make_pair(TypeID::recUsedSV, equations_->getNumSv()));

        gEpoch.slnData.insert(std::make_pair(TypeID::recSlnType, static_cast<int>(desiredSlnType())));
        gEpoch.slnData.insert(std::make_pair(TypeID::sigma, solver.getPhaseSigma()));

        gEpoch.isValidSolution_ = true;
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
        if (!processingConfig().fullOutput)
            return;

        const double amb = solver.getSolution(param);
        if (amb != 0)
            gEpoch.satData[param.sv][param.type] = amb;
    }

    // creates GnssEpoch for output: all data if fullOutput is set, only residuals otherwise
    GnssEpoch GnssSolution::makeEpochToOutput(const gnsstk::SatTypePtrMap& body) const
    {
        const static std::set<TypeID> resTypes = []() {
            std::set<TypeID> s(KalmanSolver::allResTypes);
            s.insert(TypeID::elevation);
            return s;
        }();

        GnssEpoch ep(body);
        if (!processingConfig().fullOutput)
            ep.satData.keepOnlyTypeID(resTypes);
        return ep;
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
        return getBaseFileName() + "_" + currentDateStr();
    }

    std::string FileNameBuilder::getBaseFileName() const
    {
        return isDifferential(solution_.desiredSlnType()) ? buildWithBaseAndRover() : buildRoverOnly();
    }

    std::string FileNameBuilder::buildWithBaseAndRover() const
    {
        std::string ss_str;
        for (auto&& system : solution_.projectConfig().processing.satelliteSystems)
            ss_str += '_' + convertSatelliteSystemToCode(system);

        return solution_.projectConfig().siteBase + "-" + solution_.projectConfig().siteRover + "_"
               + slnTypeToStr(solution_.desiredSlnType()) + ss_str;
    }

    std::string FileNameBuilder::buildRoverOnly() const
    {
        std::string ss_str;
        for (auto&& system : solution_.projectConfig().processing.satelliteSystems)
            ss_str += '_' + convertSatelliteSystemToCode(system);

        return solution_.projectConfig().siteRover + "_" + slnTypeToStr(solution_.desiredSlnType()) + ss_str;
    }
} // namespace pod
