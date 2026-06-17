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

        unknowns_.clear();
        for (auto&& eq : equations_)
        {
            auto&& params = eq->getParameters();
            unknowns_.insert(params.cbegin(), params.cend());
        }

        numUnknowns_ = getNumUnknowns();

        // set resize design matrix
        H.resize(numMeas_, numUnknowns_, 0.0);

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

        int col(0);
        for (auto& eq : equations_)
            eq->contributeDesignMatrix(gData, getMeasTypes(), H, col);
    }

    void EquationComposer::updateTransitionMatrix(Matrix<double>& Phi) const
    {
        int i = 0;
        Phi.resize(numUnknowns_, numUnknowns_, 0.0);
        for (auto& eq : equations_)
            eq->contributeTransitionMartix(Phi, i);
    }

    void EquationComposer::updateProcessNoiseMatrix(Matrix<double>& Q) const
    {
        int i = 0;
        Q.resize(numUnknowns_, numUnknowns_, 0.0);
        for (auto& eq : equations_)
            eq->contributeProcessNoiseMatrix(Q, i);
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
        int res = 0;
        for (auto& eq : equations_)
            res += eq->getNumUnknowns();
        return res;
    }

    void EquationComposer::updateKfState(gnsstk::Vector<double>& currState, gnsstk::Matrix<double>& currErrorCov) const
    {
        initKfState(currState, currErrorCov);

        int row = 0;

        // update state and covarince
        for (const auto& it_row : unknowns_)
        {
            const auto& typeRow = filterData_.find(it_row);
            if (typeRow != filterData_.end())
            {
                currState(row) = typeRow->second.value;
                int col = 0;
                for (const auto& it_col : unknowns_)
                {
                    const auto& typeCol = (typeRow->second).valCov.find(it_col);
                    if (typeCol != (typeRow->second).valCov.end())
                        currErrorCov(col, row) = currErrorCov(row, col) = typeCol->second;
                    ++col;
                }
            }
            ++row;
        }
    }

    void EquationComposer::storeKfState(const gnsstk::Vector<double>& currState,
                                        const gnsstk::Matrix<double>& currErrorCov)
    {
        int row = 0;
        for (const auto& it_row : unknowns_)
        {
            filterData_[it_row].value = currState(row);

            int col = 0;
            for (const auto& it_col : unknowns_)
            {
                filterData_[it_row].valCov[it_col] = currErrorCov(row, col);
                ++col;
            }
            ++row;
        }
    }

    void EquationComposer::initKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const
    {
        state.resize(numUnknowns_, 0.0);
        cov.resize(numUnknowns_, numUnknowns_, 0.0);

        int i = 0;
        for (auto& eq : equations_)
            eq->defStateAndCovariance(state, cov, i);
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

    // get current set unknowns TypeID's
    ParametersSet& EquationComposer::currentUnknowns()
    {
        return unknowns_;
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