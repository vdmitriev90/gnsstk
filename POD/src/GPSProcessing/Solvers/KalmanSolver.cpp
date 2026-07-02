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
        TypeID::postfitPC,
    };

    // set of all possible TypeID for  carrier phase postfit residuals
    const std::set<gnsstk::TypeID> KalmanSolver::phaseResTypes{
        TypeID::postfitL,
        TypeID::postfitL1,
        TypeID::postfitL2,
        TypeID::postfitLC,
    };

    // maximum time interval (in seconds) without data
    double KalmanSolver::maxGap = 61;

    KalmanSolver::KalmanSolver() : firstTime(true), isValid(false) {}

    KalmanSolver::KalmanSolver(EquationComposerPtr eqs) : firstTime(true), equations(eqs), isValid(false) {}

    KalmanSolver::~KalmanSolver() {}

    IRinex& KalmanSolver::Process(IRinex& gData)
    {
        // invalidate solution
        isValid = false;
        isReset = false;
        double dt = std::abs(t_pre - gData.getHeader().epoch);

        if (dt > maxGap)
        {
            isReset = true;
            FilterData[t_pre] = getState();
            reset();
            DBOUT_LINE("dt= " << dt << "->RESET")
            // resetEpoches.insert(gData.getHeader().epoch);
        }
        t_pre = gData.getHeader().epoch;
        // workaround: reset PPP engine every day
        double sec = gData.getHeader().epoch.getSecondOfDay();
        if (static_cast<int>(sec) == 0 && equations->getSlnType() == SlnType::PppFloat)
            equations->clearSvData();

        equations->prepare(gData);
        Vector<double> floatSolution;

        // if number of satellies passed to processing is less than 'MIN_NUM_SV'
        // clear all SV data except observable
        if (gData.getBody().size() < minSatNumber)
        {
            equations->keepOnlySv(gData.getBody().getSatID());

            return gData;
        }

        equations->updateSystemMatrices(gData, hMatrix, measVector, weightMatrix, phiMatrix, qMatrix);

        if (dt > maxGap)
            equations->initKfState(solution, covMatrix);
        else
            equations->updateKfState(solution, covMatrix);

        firstTime = false;

        for (int i = 0; i < 2; i++)
        {
            // if number of satellies passed to processing is less than 'MIN_NUM_SV'
            // clear all SV data except observable
            if (gData.getBody().size() < minSatNumber)
            {
                equations->keepOnlySv(gData.getBody().getSatID());

                return gData;
            }

            // DBOUT_LINE("--" << i << "--");

            /*for (auto& it : equations->currentUnknowns())
                DBOUT(it << " ");
            DBOUT_LINE("")*/
            // DBOUT_LINE("meas Vector\n" << setprecision(10) << measVector);
            // DBOUT_LINE("H\n" << hMatrix);
            // DBOUT_LINE("Cov\n" << covMatrix);
            // DBOUT_LINE("weightMatrix\n" << weightMatrix.diagCopy());
            // DBOUT_LINE("qMatrix: " << qMatrix.diagCopy());
            // DBOUT_LINE("phiMatrix: " << phiMatrix.diagCopy());

            // prepare
            Matrix<double> hMatrixTr = transpose(hMatrix);
            Matrix<double> phiMatrixTr = transpose(phiMatrix);
            Matrix<double> hTrTimesW = hMatrixTr * weightMatrix;

            // predict
            Matrix<double> Pminus = phiMatrix * covMatrix * phiMatrixTr + qMatrix;
            Vector<double> xminus = phiMatrix * solution;

            // DBOUT_LINE("Pminus\n" << Pminus);
            // correct
            try
            {
                Matrix<double> invPminus = inverseChol(Pminus);
                covMatrix = inverseChol(hTrTimesW * hMatrix + invPminus);
                solution = covMatrix * (hTrTimesW * measVector + invPminus * xminus);
            }
            catch (const gnsstk::MatrixException& e)
            {
                std::cerr << e << endl;

                Matrix<double> invPminus = inverseSVD(Pminus);
                covMatrix = inverseSVD(hTrTimesW * hMatrix + invPminus);
                solution = covMatrix * (hTrTimesW * measVector + invPminus * xminus);
            }

            postfitResiduals = measVector - hMatrix * solution;

            //DBOUT_LINE(StringUtils::formatTime(gData.getHeader().epoch) << " sln: " << solution)

            // DBOUT_LINE("solution: " << solution);
            // DBOUT_LINE("postfit Residuals: " << postfitResiduals);
            // DBOUT_LINE("CovPost\n" << covMatrix.diagCopy());

            floatSolution = solution;

            fixAmbiguities(gData);
            // storeAmbiguities(gData);

            auto vpv = postfitResiduals * weightMatrix * postfitResiduals;
            int numMeas = postfitResiduals.size();
            int numPar = solution.size();

            sigma = (numMeas > numPar) ? sqrt(vpv(0) / (numMeas - numPar)) : 0.0;
            phaseSigma = getSigma(phaseResTypes);

            if (i == 0 && checkPhase(gData) == 0)
                break;
            else
                DBOUT_LINE("Catched by residuals\n")
        }

        equations->saveResiduals(gData, postfitResiduals);

        equations->storeKfState(floatSolution, covMatrix);

        // everything is OK => set solutiuon status to VALID
        isValid = true;
        return gData;
    }

    int KalmanSolver::checkPhase(IRinex& gData)
    {
        constexpr double phaseLim = 0.1; //meters

        const ResidualInfo worst = equations->findMaxResidual(postfitResiduals, phaseResTypes);

        if (worst.row < 0 || worst.value < phaseLim)
            return 0;

        // drop the offending satellite's (contiguous) rows from H, l and W
        const std::set<int> rows = equations->getSatRows(worst.sat);

        // update H
        MatrixExtensions::removeRows(hMatrix, rows);

        // update observations
        MatrixExtensions::removeElms(measVector, rows);

        // update weigths
        MatrixExtensions::removeRows(weightMatrix, rows);
        MatrixExtensions::removeColumns(weightMatrix, rows);

        // reset Phi and Q only for THIS satellite's ambiguities, via the layout
        for (const auto& amb : equations->getCurrentAmb())
        {
            if (amb.sv != worst.sat)
                continue;

            const int ind = getUnknownIndex(amb);
            if (ind < 0)
                continue;

            phiMatrix(ind, ind) = 0;
            qMatrix(ind, ind) = 4e14;
        }

        // remove sv
        gData.getBody().removeSatID(worst.sat);

        cout << StringUtils::formatTime(gData.getHeader().epoch) << " : " << worst.sat << " "
             << TypeID::tStrings[worst.type.type] << " value: " << worst.value << endl;

        return 1;
    }

    double KalmanSolver::getSigma(const TypeIDSet& types) const
    {
        std::vector<double> phaseRes = equations->getResiduals(PostfitResiduals(), types);
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
        if (equations->getSlnType() == SlnType::PdFixed && gData.getBody().size() > 5)
        {
            int core_num = equations->getLayout().size() - equations->getCurrentAmb().size();

            AmbiguityHandler ar(equations->getCurrentAmb(), solution, covMatrix, core_num);
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

    int KalmanSolver::getUnknownIndex(const FilterParameter& parameter) const
    {
        return equations->getLayout().index(parameter);
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

    bool KalmanSolver::ResetIfRequared(const gnsstk::CommonTime& t, const KalmanSolver::filterHistory& data)
    {
        double dt = t - t_pre;

        if (std::abs(dt) > maxGap)
        {
            auto it = data.find(t);
            if (it != data.end())
            {
                setState(it->second);
                t_pre = t;
                DBOUT_LINE(StringUtils::formatTime(t) << "reset: " << dt)

                return true;
            }
        }
        return false;
    }
} // namespace pod
