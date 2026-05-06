#pragma once
#include"StochasticModel.hpp"
namespace pod
{
    class IonoStochasticModel :
        public gnsstk::RandomWalkModel
    {
    public:
        IonoStochasticModel() {};
        
        IonoStochasticModel(double qp,
            const gnsstk::CommonTime& prevTime = gnsstk::CommonTime::BEGINNING_OF_TIME,
            const gnsstk::CommonTime& currentTime = gnsstk::CommonTime::BEGINNING_OF_TIME)
            : RandomWalkModel(qp, prevTime, prevTime) {};

        virtual ~IonoStochasticModel() {};

        /** This method provides the stochastic model with all the available
        *  information and takes appropriate actions.
        *
        * @param sat        Satellite.
        * @param gData      Data object holding the data.
        *
        */
        virtual void Prepare(const  gnsstk::SatID& sat, gnsstk::IRinex& gData) override;

        virtual double getQ() const override;
        virtual double getPhi() const override;
    private:
        double el;
        bool csFlag;

    };
}

