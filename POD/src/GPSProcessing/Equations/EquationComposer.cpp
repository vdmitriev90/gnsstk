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

    void EquationComposer::updateDesignMatrix(gnsstk::IRinex& gData, gnsstk::Matrix<double>& H)
    {
        int numSVs = gData.getBody().size();
        int numMeasTypes = getMeasTypes().size();

        // number of measurements are equals number of satellites times observation types number
        numMeas_ = numSVs * numMeasTypes;

        std::vector<FilterParameter> curr_params;
        for (auto&& eq : equations_)
        {
            auto&& params = eq->getParameters();
            std::copy(params.cbegin(), params.cend(), std::back_inserter(curr_params));
        }

        layout_.build(curr_params);

        // set resize design matrix
        H.resize(numMeas_, getNumUnknowns(), 0.0);
        /*
        form the design martix H:
           | Tropo | dX dY dZ | cdt | cdt(R1) | cdt(G2) | cdt(R2) |   iono delay   |     N1     |     N2     |
           ---------------------------------------------------------------------------------------------------
           |       |          |     |         |         |         |                |            |            |
        P1 |   m   | ax,ay,az |  1  |  R?1:0  |    0    |    0    |   dI/dr1*E     |     0      |     0      |
           |       |          |     |         |         |         |                |            |            |
           ---------------------------------------------------------------------------------------------------
           |       |          |     |         |         |         |                |            |            |
        P2 |   m   | ax,ay,az |  1  |    0    |  G?1:0  |  R?1:0  |   dI/dr2*E     |     0      |     0      |
           |       |          |     |         |         |         |                |            |            |
           ---------------------------------------------------------------------------------------------------
           |       |          |     |         |         |         |                |            |            |
        L1 |   m   | ax,ay,az |  1  |  R?1:0  |    0    |    0    |  -dI/dr1*E     | lambda_1*E |     0      |
           |       |          |     |         |         |         |                |            |            | 
           ---------------------------------------------------------------------------------------------------
           |       |          |     |         |         |         |                |            |            |
        L2 |   m   | ax,ay,az |  1  |    0    |  G?1:0  |  R?1:0  |  -dI/dr2*E     |     0      | lambda_2*E |
           |       |          |     |         |         |         |                |            |            |
           ---------------------------------------------------------------------------------------------------
        */

        for (auto& eq : equations_)
            eq->contributeDesignMatrix(gData, getMeasTypes(), H, layout_);
    }

    void EquationComposer::updateTransitionMatrix(Matrix<double>& Phi) const
    {
        Phi.resize(getNumUnknowns(), getNumUnknowns(), 0.0);
        for (auto& eq : equations_)
            eq->contributeTransitionMartix(Phi, layout_);
    }

    void EquationComposer::updateProcessNoiseMatrix(Matrix<double>& Q) const
    {
        Q.resize(getNumUnknowns(), getNumUnknowns(), 0.0);
        for (auto& eq : equations_)
            eq->contributeProcessNoiseMatrix(Q, layout_);
    }

    void EquationComposer::updateWeightsMatrix(const IRinex& gData, gnsstk::Matrix<double>& weightMatrix)
    {
        const size_t numsv = gData.getBody().size();
        // Generate the appropriate weights matrix
        // Try to extract weights from GDS
        const satTypeValueMap dummy(gData.getBody().extractTypeID(TypeID::weight));

        // prepare identy matrix
        weightMatrix.resize(numMeas_, numMeas_, 0.0);

        // Check if weights match
        if (dummy.numSats() == numsv)
        {
            auto weigths = gData.getBody().getVectorOfTypeID(TypeID::weight);
            size_t k(0);
            for (const auto& observable : getMeasTypes())
            {
                const double weightFactor = pod::weighting::weightOf(observable.type);
                for (size_t i = 0; i < numsv; i++)
                {
                    const size_t idx = i + numsv * k;
                    weightMatrix(idx, idx) = weigths(i) * weightFactor;
                }
                k++;
            }
        }
        else
        {
            GNSSTK_ASSERT_MSG(false, "Weights vector size does not match number of satellites.");
        }
    }

    void EquationComposer::updateMeas(const IRinex& gData, gnsstk::Vector<double>& measVector)
    {
        measVector.resize(numMeas_, 0.0);
        int j = 0;
        for (const auto& it : getMeasTypes())
        {
            auto meas = gData.getBody().getVectorOfTypeID(it);
            size_t numSat = meas.size();
            for (size_t i = 0; i < numSat; i++)
                measVector(i + j * numSat) = meas(i);
            j++;
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
                itSat.second->get_value()[resType] = residuals(i_res++);
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