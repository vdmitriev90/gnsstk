#ifndef POD_SHERICAL_HARMONICS_MODEL_H
#define POD_SHERICAL_HARMONICS_MODEL_H

#include "GravityModel.h"
#include "ForceModel.hpp"
#include "EarthPoleTide.hpp"
#include "EarthOceanTide.hpp"
#include "EarthSolidTide.hpp"

namespace pod
{
    class SphericalHarmonicsModel : public GravityModel
    {
    public: 
        SphericalHarmonicsModel(const GravityModelData &gData);
        
        /// Default destructor
        virtual ~SphericalHarmonicsModel() {};


        /** Computes the acceleration due to gravity in m/s^2.
        * @param E ECI to ECEF transformation matrix.
        * @return ECI acceleration in m/s^2.
        */
        gnsstk::Vector<double> gravity(const gnsstk::Matrix<double>& E);

        gnsstk::Vector<double> gravityNorm(const gnsstk::Matrix<double>& E);
        /** Computes the partial derivative of gravity with respect to position.
        * @return ECI gravity gradient matrix.
        * @param r ECI position vector.
        * @param E ECI to ECEF transformation matrix.
        */
        gnsstk::Matrix<double> gravityGradient(const gnsstk::Matrix<double>& E);


        /** Call the relevant methods to compute the acceleration.
        * @param utc Time reference class
        * @param rb  Reference body class
        * @param sc  Spacecraft parameters and state
        * @return the acceleration [m/s^s]
        */
        virtual void doCompute(gnsstk::Epoch time,  gnsstk::Spacecraft& sc) override;

        /// Return force model name
        virtual std::string modelName() const
        {
            return "SphericalHarmonicsModel";
        }

        /// return the force model index
        virtual int forceIndex() const
        {
            return gnsstk::ForceModel::FMI_GEOEARTH;
        }

    protected:

        /* Evaluates the two harmonic functions V and W.
        * @param r ECEF position vector.
        */
        void computeVW(gnsstk::Vector<double> r_bf);

        void computeNormVW(gnsstk::Vector<double> r_bf);

        /// Add tides to coefficients 
        void correctCSTides(gnsstk::Epoch t, bool solidFlag = false, bool oceanFlag = false, bool poleFlag = false);

        /// normalized coefficient
        double normFactor(int n, int m);
        double normN01(int n, int m);
        double normN02(int n, int m);
        double normFactorNN(int n);

        /// V W  (nmax+3)*(nmax+3)
        /// Harmonic function V and W
        gnsstk::Matrix<double> V, W;

        /// Objects to do earth tides correction
        gnsstk::EarthSolidTide  solidTide;
        gnsstk::EarthPoleTide   poleTide;
        gnsstk::EarthOceanTide  oceanTide;
    private :

        double delta(int n) 
        {
            return (n == 0) ? 1.0 : 0.0;
        }
        double E(int n)
        {
            return 2.0-delta(n);
        }
    };
}

#endif //!POD_SHERICAL_HARMONICS_MODEL_H