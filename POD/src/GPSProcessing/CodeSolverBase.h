#ifndef POD_PR_SOLVER_BASE_H
#define POD_PR_SOLVER_BASE_H

#include"typenames.hpp"
#include"PowerSum.hpp"
#include "Rinex3ObsData.hpp"
#include "XvtStore.hpp"
#include "GPSEllipsoid.hpp"
#include"IonoModelStore.hpp"
#include"Matrix.hpp"
#include"TropModel.hpp"
#include"NeillTropModel.hpp"
#include"CodeProcSvData.h"
#include"GnssDataStore.hpp"


namespace pod
{
    class CodeSolverBase
    {
    protected:

        static  double eps;
        static  gnsstk::GPSEllipsoid ellGPS;
        static void refreshSolution(gnsstk::Vector<double> &Sol, gnsstk::Vector<double> &dSol);

    public:

        CodeSolverBase(GnssDataStore_sptr data );
        virtual ~CodeSolverBase()
        {};

        std::string virtual getName()
        {
            return "PRSolverBase";
        };
        void  selectObservables(
            const gnsstk::Rinex3ObsData &rod,
            const gnsstk::Rinex3ObsHeader& roh,
            const std::set<gnsstk::SatID::SatelliteSystem> &systems,
            const ObsTypes & typeMap,
            CodeProcSvData & svData,
            bool isApplyRCO = false
        );

        void prepare(
            const gnsstk::CommonTime &t,
            const gnsstk::XvtStore<gnsstk::SatID>& Eph,
            CodeProcSvData & svData
        );

        int solve(
            const gnsstk::CommonTime &t,
            const gnsstk::IonoModelStore &iono,
            CodeProcSvData & svData
        );

       // string printSolution(const CodeProcSvData &useSat);


        virtual gnsstk::NeillTropModel initTropoModel(const gnsstk::Position &nominalPos, int DoY) = 0;

        virtual double getTropoCorrection(
            const gnsstk::Position &rxPos,
            const gnsstk::Position &svPos,
            const gnsstk::CommonTime &t) const = 0;

    protected:
         int solveInter(
            const gnsstk::CommonTime &t,
             const gnsstk::IonoModelStore &iono,
            CodeProcSvData & svData,
			 gnsstk::Matrix<double>& Cov

        );
         void CodeSolverBase::calcSigma(
             const gnsstk::Position& rxPos,
             const gnsstk::Matrix<double> & W,
             const gnsstk::Vector<double> & b,
             const CodeProcSvData &svsData);

        void calcStat(const gnsstk::Matrix<double>& Cov);

        int CodeSolverBase::catchSatByResid(
            const gnsstk::CommonTime & t,
            const gnsstk::IonoModelStore & iono,
            CodeProcSvData & svsData
        );

    public:
		gnsstk::ComputeIonoModel::IonoModelType ionoType;

        double maskSNR;
        double maskEl;

        int maxIter;
        int iter;

		gnsstk::Vector< double> Sol;

        double sigma;
        double RMS3D;
        double PDOP;
        double sigmaMax;

        std::ofstream dbg;

        friend  std::ostream& operator<<(std::ostream& strs, const CodeSolverBase& gdsMap);
    };
}

#endif // !POD_PR_SOLVER_BASE_H