#include "EquationComposer.h"

#include "Weighting.h"

using namespace gnsstk;

namespace pod
{
    void EquationComposer::prepare(IRinex& gData)
    {
        // clear ambiguities set
        currAmb_.clear();
        for (auto& eq : equations_)
        {
            // prepare equations objects state
            eq->prepare(gData);

            // update current set of ambiguities
            auto ambs = eq->getAmbSet();
            currAmb_.insert(ambs.cbegin(), ambs.cend());
        }
    }

    void EquationComposer::updateSystemMatrices(gnsstk::IRinex& gData,
                                                gnsstk::Matrix<double>& H,
                                                gnsstk::Vector<double>& prefitResiduals,
                                                gnsstk::Matrix<double>& W,
                                                gnsstk::Matrix<double>& Phi,
                                                gnsstk::Matrix<double>& Q)
    {
        const auto& body = gData.getBody();
        const auto& measTypes = getMeasTypes();
        const size_t numSVs = body.size();

        auto& provider = *ObservationTypesProvider::instance();
        buildObservationBlocks(gData, provider, satBlocks_);

        numMeas_ = numSVs * measTypes.size();
        //GNSSTK_ASSERT_MSG(numMeas_ == satBlocks_.totalMeasurements(), "Number of measurements is zero.");

        std::vector<FilterParameter> curr_params;
        for (auto& eq : equations_)
        {
            const auto& params = eq->getParameters();
            std::copy(params.cbegin(), params.cend(), std::back_inserter(curr_params));
        }

        layout_.build(curr_params);

        const int numUnknowns = getNumUnknowns();
        H.resize(numMeas_, numUnknowns, 0.0);
        Phi.resize(numUnknowns, numUnknowns, 0.0);
        Q.resize(numUnknowns, numUnknowns, 0.0);

        for (auto& eq : equations_)
        {
            eq->contributeDesignMatrix(gData, measTypes, H, layout_);
            eq->contributeTransitionMartix(Phi, layout_);
            eq->contributeProcessNoiseMatrix(Q, layout_);
        }

        prefitResiduals.resize(numMeas_, 0.0);
        size_t j = 0;
        for (const auto& measType : measTypes)
        {
            const auto meas = body.getVectorOfTypeID(measType);
            for (size_t i = 0; i < numSVs; i++)
                prefitResiduals(i + j * numSVs) = meas(i);
            j++;
        }

        const satTypeValueMap weightMap(body.extractTypeID(TypeID::weight));
        GNSSTK_ASSERT_MSG(weightMap.numSats() == numSVs, "Weights vector size does not match number of satellites.");

        W.resize(numMeas_, numMeas_, 0.0);
        const auto weights = body.getVectorOfTypeID(TypeID::weight);
        size_t k = 0;
        for (const auto& measType : measTypes)
        {
            const double weightFactor = pod::weighting::weightOf(measType.type);
            for (size_t i = 0; i < numSVs; i++)
                W(i + k * numSVs, i + k * numSVs) = weights(i) * weightFactor;
            k++;
        }
    }

    int EquationComposer::getNumUnknowns() const
    {
        return static_cast<int>(layout_.size());
    }

    const StateLayout& EquationComposer::getLayout() const
    {
        return layout_;
    }

    void EquationComposer::updateKfState(gnsstk::Vector<double>& currState, gnsstk::Matrix<double>& currErrorCov) const
    {
        initKfState(currState, currErrorCov);

        // update state and covarince
        for (size_t row = 0; row < layout_.size(); ++row)
        {
            const auto& param_row = layout_.param(row);
            const auto& type_row = filterData_.find(param_row);
            if (type_row != filterData_.end())
            {
                currState(row) = type_row->second.value;
                int col = 0;

                for (size_t col = 0; col < layout_.size(); ++col)
                {
                    const auto& param_col = layout_.param(col);

                    const auto& type_col = (type_row->second).valCov.find(param_col);
                    if (type_col != (type_row->second).valCov.end())
                        currErrorCov(col, row) = currErrorCov(row, col) = type_col->second;
                }
            }
        }
    }

    void EquationComposer::storeKfState(const gnsstk::Vector<double>& currState,
                                        const gnsstk::Matrix<double>& currErrorCov)
    {
        for (int row = 0; row < static_cast<int>(layout_.size()); ++row)
        {
            const auto& param_row = layout_.param(row);

            auto& data = filterData_[param_row];
            data.value = currState(row);

            for (int col = 0; col < static_cast<int>(layout_.size()); ++col)
            {
                const auto& param_col = layout_.param(col);
                data.valCov[param_col] = currErrorCov(row, col);
            }
        }
    }

    void EquationComposer::initKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const
    {
        state.resize(getNumUnknowns(), 0.0);
        cov.resize(getNumUnknowns(), getNumUnknowns(), 0.0);

        for (auto& eq : equations_)
            eq->defStateAndCovariance(state, cov, layout_);
    }

    void EquationComposer::saveResiduals(gnsstk::IRinex& gData, const gnsstk::Vector<double>& residuals) const
    {
        int resNum = residuals.size();
        int satNum = gData.getBody().size();
        int numResTypes = getResidTypes().size();

        GNSSTK_ASSERT(satNum * numResTypes == resNum);

        int i_res = 0;
        for (auto&& resType : getResidTypes())
            for (auto&& itSat : gData.getBody())
                (*itSat.second)[resType] = residuals(i_res++);
    }

    std::vector<double> EquationComposer::getResiduals(const gnsstk::Vector<double>& residuals,
                                                       const TypeIDSet& types) const
    {
        size_t numResTypes = getResidTypes().size();
        if (numResTypes == 0)
            return {};

        size_t nsv = residuals.size() / numResTypes;

        std::vector<double> res;
        res.reserve(types.size() * nsv);

        size_t iType(0);
        for (auto&& resType : getResidTypes())
        {
            if (types.find(resType) != types.end())
                for (size_t j = 0; j < nsv; ++j)
                    res.push_back(residuals(iType * nsv + j));
            iType++;
        }
        return res;
    }

    EquationComposer& EquationComposer::setState(const EquationComposer::FilterState& newState)
    {
        filterData_ = newState;
        return *this;
    }

    const EquationComposer::FilterState& EquationComposer::getState() const
    {
        return filterData_;
    }

    TypeIDSet& EquationComposer::getMeasTypes()
    {
        return measurementsTypes_;
    }

    const TypeIDSet& EquationComposer::getMeasTypes() const
    {
        return measurementsTypes_;
    }

    TypeIDSet& EquationComposer::getResidTypes()
    {
        return residualsTypes_;
    }

    const TypeIDSet& EquationComposer::getResidTypes() const
    {
        return residualsTypes_;
    }

    const ParametersSet& EquationComposer::getCurrentAmb() const
    {
        return currAmb_;
    }

    SlnType EquationComposer::getSlnType() const
    {
        return slnType_;
    }

    EquationComposer& EquationComposer::setSlnType(SlnType sType)
    {
        slnType_ = sType;
        return *this;
    }

    /// add new equation to equation list
    EquationComposer& EquationComposer::addEquation(std::unique_ptr<EquationBase> eq)
    {
        equations_.push_back(std::move(eq));
        return *this;
    }

    /// erase equation list
    void EquationComposer::clearEquations()
    {
        equations_.clear();
    }

    /// erase stored data
    void EquationComposer::clearData()
    {
        filterData_.clear();
    }

    void EquationComposer::keepOnlySv(const SatIDSet& svs)
    {
        for (auto it = filterData_.cbegin(); it != filterData_.cend();)
        {
            if (svs.find(it->first.sv) == svs.end())
                it = filterData_.erase(it);
            else
                ++it;
        }
    }
    void EquationComposer::clearSvData(const SatIDSet& svs)
    {
        for (auto it = filterData_.cbegin(); it != filterData_.cend();)
        {
            if (svs.find(it->first.sv) != svs.end())
                it = filterData_.erase(it);
            else
                ++it;
        }
    }

    void EquationComposer::clearSvData()
    {
        for (auto it = filterData_.cbegin(); it != filterData_.cend();)
        {
            if (it->first.sv != SatID::dummy)
                it = filterData_.erase(it);
            else
                ++it;
        }
    }
} // namespace pod