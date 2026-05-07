#pragma once
#include "EquationBase.h"
#include "StochasticModel.hpp"
#include<memory>

namespace pod
{

    /**This is a class to modify the GNSS data stucture: to
    * add ISB componenst of receiver clock offset
    *
    * The "InterSystemBias" object will visit every satellite in
    * the GNSS data structure and will try to add ISB model terms
    * to it.
    *
    * When used with the ">>" operator, this class returns the same
    * incoming data structure with the ISB data inserted along their
    * corresponding satellites. Be warned that if a GDS contains
    * less then two satellites of some satellite system its will
    * be summarily deleted from the data structure.
    */
    class InterSystemBias :
        public  EquationBase
    {
    private:
        static std::map< gnsstk::SatelliteSystem, FilterParameter> ss2isb;
        static std::map<FilterParameter, gnsstk::SatelliteSystem> isb2ss;

        static const  gnsstk::TypeIDSet l1Types;

    public:

        InterSystemBias();
        virtual ~InterSystemBias() {};

        virtual  void Prepare(gnsstk::IRinex& gData);

        virtual void updateH(const gnsstk::IRinex& gData, const gnsstk::TypeIDSet& types, gnsstk::Matrix<double>& H, int& col_0)  override;

        virtual ParametersSet getParameters() const override
        {
            return types;
        }

        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const override;

        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const override;

        virtual void defStateAndCovariance(gnsstk::Vector<double>& x, gnsstk::Matrix<double>& P, int& index) const override;

        virtual int getNumUnknowns() const override;

        virtual InterSystemBias& setStochasicModel(const gnsstk::SatelliteSystem& system, gnsstk::StochasticModel_uptr newModel);


    private:

        std::map< FilterParameter, gnsstk::StochasticModel_uptr> stochasticModels;

        //current set of satellite systems
        ParametersSet types;

        class Initilizer
        {
        public: Initilizer();
        };
        static Initilizer IsbSingleton;

    };
}

