#include "EquationComposer.h"

#include "GnssObsMapping.h"
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
        // --- 1. Build measurement blocks (single source of truth for rows) ---
        buildObservationBlocks(gData, satBlocks_);

        const size_t num_meas = satBlocks_.totalMeasurements();

        // --- 2. Build state layout ---
        std::vector<FilterParameter> curr_params;
        for (auto& eq : equations_)
        {
            const auto& params = eq->getParameters();
            std::copy(params.cbegin(), params.cend(), std::back_inserter(curr_params));
        }

        layout_.build(curr_params);

        const int numUnknowns = getNumUnknowns();

        // --- 3. Resize matrices ---
        H.resize(num_meas, numUnknowns, 0.0);
        prefitResiduals.resize(num_meas, 0.0);
        W.resize(num_meas, num_meas, 0.0);
        Phi.resize(numUnknowns, numUnknowns, 0.0);
        Q.resize(numUnknowns, numUnknowns, 0.0);

        // --- 4. Fill H, z (prefit), and W in ONE pass ---
        int row = 0;

        for (const auto& block : satBlocks_)
        {
            for (const auto& m : block.measurements)
            {
                RowContext ctx{block.sat, block.data, m.type, row};

                // --- H: design matrix ---
                for (auto& eq : equations_)
                    eq->fillRow(ctx, layout_, H);

                // --- Measurement vector (prefit residuals) ---
                prefitResiduals(row) = m.prefitResidual;

                // --- Weight matrix (diagonal) ---
                W(row, row) = m.weight;

                ++row;
            }
        }

        // --- 5. Transition and process noise (independent of measurements) ---
        for (auto& eq : equations_)
        {
            eq->contributeTransitionMartix(Phi, layout_);
            eq->contributeProcessNoiseMatrix(Q, layout_);
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
        int row = 0;
        for (const auto& block : satBlocks_)
        {
            auto it = gData.getBody().find(block.sat);
            if (it == gData.getBody().end() || !it->second)
            {
                row += static_cast<int>(block.size());
                continue;
            }

            for (const auto& m : block.measurements)
            {
                const auto postfit = obs_mapping::prefitToPostfit(m.type);
                if (postfit != TypeID::Unknown)
                    (*it->second)[TypeID(postfit)] = residuals(row);
                ++row;
            }
        }
    }

    std::vector<double> EquationComposer::getResiduals(const gnsstk::Vector<double>& residuals,
                                                       const TypeIDSet& types) const
    {
        std::vector<double> res;
        res.reserve(residuals.size());

        int row = 0;
        for (const auto& block : satBlocks_)
        {
            for (const auto& m : block.measurements)
            {
                const auto postfit = obs_mapping::prefitToPostfit(m.type);
                if (postfit != TypeID::Unknown && types.find(TypeID(postfit)) != types.end())
                    res.push_back(residuals(row));
                ++row;
            }
        }
        return res;
    }

    ResidualInfo EquationComposer::findMaxResidual(const gnsstk::Vector<double>& residuals,
                                                   const TypeIDSet& postfitTypes) const
    {
        return pod::findMaxResidual(satBlocks_, residuals, postfitTypes);
    }

    std::set<int> EquationComposer::getSatRows(const SatID& sat) const
    {
        return pod::getSatRows(satBlocks_, sat);
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