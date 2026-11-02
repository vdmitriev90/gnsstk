#include "KalmanSolver.h"

#include "AmbiguityHandler.h"
#include "GnssSolution.h"
#include "MatrixExtensions.h"
#include "PowerSum.hpp"
#include "StringUtils.h"
#include "WinUtils.h"

#include <algorithm>
#include <cmath>

using namespace std;
using namespace gnsstk;

namespace pod
{
    // set of all possible TypeID for code pseudorange postfit residuals
    const std::set<gnsstk::TypeID> KalmanSolver::codeResTypes{
        TypeID::postfitC,
        TypeID::postfitC1,
        TypeID::postfitC2,
        TypeID::postfitP1,
        TypeID::postfitP2,
        TypeID::postfitC5,
        TypeID::postfitC6,
        TypeID::postfitC7,
        TypeID::postfitC8,
        TypeID::postfitPC,
    };

    // set of all possible TypeID for  carrier phase postfit residuals
    const std::set<gnsstk::TypeID> KalmanSolver::phaseResTypes{
        TypeID::postfitL,
        TypeID::postfitL1,
        TypeID::postfitL2,
        TypeID::postfitL5,
        TypeID::postfitL6,
        TypeID::postfitL7,
        TypeID::postfitL8,
        TypeID::postfitLC,
    };

    // union of code and phase postfit residual TypeIDs
    const std::set<gnsstk::TypeID> KalmanSolver::allResTypes = [] {
        std::set<gnsstk::TypeID> s(codeResTypes);
        s.insert(phaseResTypes.begin(), phaseResTypes.end());
        return s;
    }();

    // maximum time interval (in seconds) without data
    double KalmanSolver::maxGap = 61;

    KalmanSolver::KalmanSolver() : isValid_(false) {}

    KalmanSolver::KalmanSolver(EquationComposerPtr eqs) : equations_(eqs), isValid_(false) {}

    IRinex& KalmanSolver::Process(IRinex& gData)
    {
        // invalidate solution
        isValid_ = false;
        isReset_ = false;
        double dt = std::abs(lastTime_ - gData.getHeader().epoch);

        if (dt > maxGap)
        {
            isReset_ = true;
            FilterData[lastTime_] = getState();
            reset();
            DBOUT_LINE("Too big gap: " << dt << " sec. Reset Filter")
            // resetEpoches.insert(gData.getHeader().epoch);
        }
        lastTime_ = gData.getHeader().epoch;
        // workaround: reset PPP engine every day
        double sec = gData.getHeader().epoch.getSecondOfDay();
        if (static_cast<int>(sec) == 0 && equations_->getSlnType() == SlnType::PppFloat)
            equations_->clearSvData();

        equations_->prepare(gData);

        // if number of satellies passed to processing is less than 'MIN_NUM_SV'
        // clear all SV data except observable
        if (gData.getBody().size() < minSatNumber_)
        {
            equations_->keepOnlySv(gData.getBody().getSatID());

            return gData;
        }
                
        gnsstk::Matrix<double> phiMatrix;// state transition matrix        
        gnsstk::Matrix<double> qMatrix; // Process noise matrix  
        gnsstk::Matrix<double> hMatrix; // design matrix
        gnsstk::Matrix<double> weightMatrix; // measurements vector (prefit-residuals)        
        gnsstk::Vector<double> measVector;

        equations_->updateSystemMatrices(gData, hMatrix, measVector, weightMatrix, phiMatrix, qMatrix);

        if (dt > maxGap)
            equations_->initKfState(solution, covMatrix);
        else
            equations_->updateKfState(solution, covMatrix);

        // a priori (predicted) state; solution/covMatrix stay untouched until update(),
        // so every early return leaves the filter with the a priori state
        Vector<double> xminus = phiMatrix * solution;
        Matrix<double> Pminus = phiMatrix * covMatrix * transpose(phiMatrix) + qMatrix;

        update(xminus, Pminus, hMatrix, weightMatrix, measVector);

        // carrier phase postfit check (data snooping): re-initialize ambiguities of the worst
        // satellite only, recompute the solution and residuals, and only then repeat the check
        constexpr int maxPhaseReinit = 4;
        for (int k = 0; k < maxPhaseReinit && checkPhase(gData, xminus, Pminus, hMatrix, weightMatrix) > 0; ++k)
            update(xminus, Pminus, hMatrix, weightMatrix, measVector);

        const Vector<double> floatSolution = solution;

        fixAmbiguities(gData);

        const auto vpv = postfitResiduals * weightMatrix * postfitResiduals;
        const int numMeas = postfitResiduals.size();
        const int numPar = solution.size();

        sigma_ = (numMeas > numPar) ? sqrt(vpv(0) / (numMeas - numPar)) : 0.0;
        phaseSigma_ = getSigma(phaseResTypes);

        equations_->saveResiduals(gData, postfitResiduals);

        equations_->storeKfState(floatSolution, covMatrix);

        // everything is OK => set solutiuon status to VALID
        isValid_ = true;
        return gData;
    }

    // Returns a string identifying this object.
    std::string KalmanSolver::getClassName() const
    {
        return "KalmanSolver";
    }

    EquationComposer& KalmanSolver::eqComposer()
    {
        return *equations_;
    }

    // return sqrt(vpv/(n-p)) value
    double KalmanSolver::getSigma() const
    {
        return sigma_;
    }

    double KalmanSolver::getPhaseSigma() const
    {
        return phaseSigma_;
    }

    // Measurement update in information form: sets solution, covMatrix and postfitResiduals
    void KalmanSolver::update(const Vector<double>& xminus,
                              const Matrix<double>& Pminus,
                              const Matrix<double>& hMatrix,
                              const Matrix<double>& weightMatrix,
                              const Vector<double>& measVector)
    {
        const Matrix<double> hTrTimesW = transpose(hMatrix) * weightMatrix;

        try
        {
            const Matrix<double> invPminus = inverseChol(Pminus);

            covMatrix = inverseChol(hTrTimesW * hMatrix + invPminus);
            solution = covMatrix * (hTrTimesW * measVector + invPminus * xminus);
        }
        catch (const MatrixException& e)
        {
            std::cerr << e << endl;

            const Matrix<double> invPminus = inverseSVD(Pminus);

            covMatrix = inverseSVD(hTrTimesW * hMatrix + invPminus);
            solution = covMatrix * (hTrTimesW * measVector + invPminus * xminus);
        }

        postfitResiduals = measVector - hMatrix * solution;
    }

    // Postfit carrier phase consistency check (cycle slip / wrong ambiguity detection).
    // For the optimal Kalman update Var(v_i) = R_ii - H_i * P+ * H_i^T, so the normalized
    // postfit residual v_i / sqrt(Var(v_i)) is N(0,1) for a consistent measurement.
    // Ambiguities of the single worst satellite failing the test are re-initialized in the a priori
    // state (the satellite and its code measurements are kept). The caller must recompute the solution
    // and repeat the check (data snooping: one satellite per iteration).
    // Returns 1 if an ambiguity was re-initialized, otherwise 0.
    int KalmanSolver::checkPhase(IRinex& gData,
                                 Vector<double>& xminus,
                                 Matrix<double>& Pminus,
                                 const Matrix<double>& hMatrix,
                                 const Matrix<double>& weightMatrix)
    {
        constexpr double phasePostfitGate = 4.0;

        // rows with variance reduction close to R_ii are fully absorbed by the state
        // and can not be tested
        constexpr double minVarianceRatio = 1.0e-3;

        SatID worstSat;
        TypeID worstType;
        double worstNormalized = phasePostfitGate;
        double worstAbs = 0.0;
        bool found = false;

        for (int row = 0; row < postfitResiduals.size(); ++row)
        {
            const ResidualInfo info = equations_->getResidualInfo(row);

            if (info.row < 0 || phaseResTypes.count(info.type) == 0)
                continue;

            const double weight = weightMatrix(row, row);
            if (!std::isfinite(weight) || weight <= 0.0)
                continue;

            const double measVariance = 1.0 / weight;

            double stateVariance = 0.0;
            for (int c1 = 0; c1 < hMatrix.cols(); ++c1)
            {
                const double h1 = hMatrix(row, c1);
                if (h1 == 0.0)
                    continue;

                for (int c2 = 0; c2 < hMatrix.cols(); ++c2)
                {
                    const double h2 = hMatrix(row, c2);
                    if (h2 != 0.0)
                        stateVariance += h1 * covMatrix(c1, c2) * h2;
                }
            }

            const double residVariance = measVariance - stateVariance;

            if (!std::isfinite(residVariance) || residVariance < minVarianceRatio * measVariance)
                continue;

            const double normalized = std::abs(postfitResiduals(row)) / std::sqrt(residVariance);

            if (normalized <= worstNormalized)
                continue;

            worstNormalized = normalized;
            worstAbs = std::abs(postfitResiduals(row));
            worstSat = info.sat;
            worstType = info.type;
            found = true;
        }

        if (!found)
            return 0;

        int count = 0;

        // re-initialize ONLY this satellite's ambiguities
        for (const auto& amb : equations_->getCurrentAmb())
        {
            if (amb.sv != worstSat)
                continue;

            const int ind = getUnknownIndex(amb);
            if (ind < 0)
                continue;

            // re-initialize the ambiguity in the a priori state: x = 0, P = diag(4e14)
            xminus(ind) = 0.0;
            for (int k = 0; k < Pminus.rows(); ++k)
            {
                Pminus(ind, k) = 0.0;
                Pminus(k, ind) = 0.0;
            }
            Pminus(ind, ind) = 4e14;
            ++count;
        }

        cout << StringUtils::formatTime(gData.getHeader().epoch) << " PHASE POSTFIT " << worstSat << " "
             << TypeID::tStrings[worstType.type] << " normalized: " << worstNormalized << " abs: " << worstAbs << " m"
             << endl;

        return count > 0 ? 1 : 0;
    }

    double KalmanSolver::getSigma(const TypeIDSet& types) const
    {
        std::vector<double> phaseRes = equations_->getResiduals(PostfitResiduals(), types);
        Matrix<double> res(phaseRes.size(), 1, 0.0);

        for (size_t i = 0; i < phaseRes.size(); ++i)
            res(i, 0) = phaseRes[i];

        // compute v'v
        auto vpv = transpose(res) * res;
        double sigma = sqrt(vpv(0, 0) / res.size());

        return sigma;
    }

    void KalmanSolver::fixAmbiguities(IRinex& gData)
    {
        if (equations_->getSlnType() == SlnType::PdFixed && gData.getBody().size() > 5)
        {
            int core_num = equations_->getLayout().size() - equations_->getCurrentAmb().size();

            AmbiguityHandler ar(equations_->getCurrentAmb(), solution, covMatrix, core_num);
            ar.fixL1L2(gData);

            for (int k = 0; k < core_num; k++)
                solution(k) = ar.CoreParamFixed()(k);
        }
    }

    // reject by code postfit residual
    IRinex& KalmanSolver::reject(IRinex& gData, const TypeIDSet& typeIds)
    {
        typedef SatTypePtrMap::value_type type;
        SatIDSet rejSat;
        ///!!!
        /// here we use only last type of postfit residuals, because in case of
        /// combined code/phase measurements processing this part contans carrier phase residuals
        // in case code/phase only processing we have only one element in @typeIds, which one will
        // be used
        ///!!!
        const auto& id = typeIds.rbegin();

        // get the sv - typeMap pair with largest residual value

        auto svWithMaxResidual = std::max_element(
            gData.getBody().begin(), gData.getBody().end(), [&](const type& it1, const type& it2) -> bool {
                double val1 = std::abs(it1.second->at(*id));
                double val2 = std::abs(it2.second->at(*id));
                return (val1 < val2);
            });

        // report detection
        cout << "Removed SV: " << svWithMaxResidual->first;
        cout << " with " << TypeID::tStrings[id->type] << " = ";
        cout << (*svWithMaxResidual->second)[*id] << endl;

        // remove sv
        rejSat.insert(svWithMaxResidual->first);

        gData.getBody().removeSatID(rejSat);

        return gData;
    }

    inline void KalmanSolver::reset()
    {
        equations_->clearData();
    }

    int KalmanSolver::getUnknownIndex(const FilterParameter& parameter) const
    {
        return equations_->getLayout().index(parameter);
    }

    // return minimum number of satellites requared for state esimation
    double KalmanSolver::getMinSatNumber() const
    {
        return minSatNumber_;
    }

    // return isValid
    bool KalmanSolver::getValid() const
    {
        return isValid_;
    }

    // set minimum number of satellites requared for state esimation
    KalmanSolver& KalmanSolver::setMinSatNumber(int value)
    {
        minSatNumber_ = value;
        return *this;
    }

    double KalmanSolver::getSolution(const FilterParameter& parameter) const
    {
        int i = getUnknownIndex(parameter);
        return solution(i);

    } // End of method 'SolverLMS::getSolution()'

    double KalmanSolver::getVariance(const FilterParameter& parameter) const
    {
        int i = getUnknownIndex(parameter);
        return covMatrix(i, i);

    } // End of method 'SolverLMS::getVariance()'

    const EquationComposer::FilterState& KalmanSolver::getState() const
    {
        return equations_->getState();
    }

    KalmanSolver& KalmanSolver::setState(const EquationComposer::FilterState& newState)
    {
        equations_->setState(newState);
        return *this;
    }

    bool KalmanSolver::getResetState()
    {
        return isReset_;
    }

    bool KalmanSolver::ResetIfRequared(const gnsstk::CommonTime& t, const KalmanSolver::FilterHistory& data)
    {
        double dt = t - lastTime_;

        if (std::abs(dt) > maxGap)
        {
            auto it = data.find(t);
            if (it != data.end())
            {
                setState(it->second);
                lastTime_ = t;
                DBOUT_LINE(StringUtils::formatTime(t) << "reset: " << dt)

                return true;
            }
        }
        return false;
    }

} // namespace pod
