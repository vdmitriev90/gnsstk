#pragma once
#include"FilterParameter.h"
#include"RefSatHandler.h"

#include"ARSimple.hpp"
#include"ARMLambda.hpp"

#include<memory>

namespace pod
{
    class AmbiguityHandler
    {
    public:
        AmbiguityHandler(
            const ParametersSet & ambiguites,
            const gnsstk::Vector<double> &state,
            const  gnsstk::Matrix<double> &cov,
            int n_core);

        ~AmbiguityHandler() {};
        
        template<class T, typename = std::enable_if_t<std::is_base_of<gnsstk::ARBase, T>::value>>
        AmbiguityHandler & setArMethod()
        {
            pAR.reset(new T());
            return *this;
        };

        void fixL1L2(gnsstk::IRinex& gData);
        
        const gnsstk::Vector<double> & CoreParamFixed()
        {
            return coreParamFixed;
        };
        
        const gnsstk::Vector<double> & DDFixedAmb()
        {
            return ddFixedAmb;
        };

        const gnsstk::Matrix<double> & DDAmbCov()
        {
            return ddAmbCov;
        };

    private:
        
        void storeDDAmbiguities(
            gnsstk::IRinex & gData, 
            const gnsstk::Vector<double> &ddFixedAmb, 
            const gnsstk::SatIDSet &refSVs) const;

        gnsstk::Vector<double>  fixDDAmbSeparately(
            const gnsstk::Vector<double> & ddAmbFloat,
            const gnsstk::Matrix<double> & ddCov) const;

        //pointer to current set of ambiguites
        const ParametersSet *pAmbs;

        //pointer to single difference float solution
        const gnsstk::Vector<double> *pSdFloatSolution;

        //pointer to single difference float solution covariance
        const gnsstk::Matrix<double> *pSdCov;

        //number of core variables
        int coreNum;

        //pointer to object for integer ambiguities resolution
        std::unique_ptr<gnsstk::ARBase> pAR;
        
        //object to choose reference satellites for double difference computation
        RefSatHandler refSatsHandler;
        
        //single diffrerence to double differnce transition matrix
        gnsstk::Matrix<double> SD2DD;

        //Core parameters values, obtained by fixed ambiguities
        gnsstk::Vector<double> coreParamFixed;

        //Double difference fixed ambiguities
        gnsstk::Vector<double> ddFixedAmb;
        
        //Double difference ambiguities covarince
        gnsstk::Matrix<double> ddAmbCov;
    };
}

