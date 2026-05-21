#pragma once
#include "DataStructures.hpp"
#include "FilterParameter.h"
#include "RinexEpoch.h"

namespace pod
{
    class EquationBase
    {
      public:
        EquationBase() {};
        virtual ~EquationBase() {};

        /* update state of equations with new observational data */
        virtual void Prepare(gnsstk::IRinex& gData) = 0;

        /* return set of FilerParameters, corresponding unknown parameters  for given equations */
        virtual ParametersSet getParameters() const = 0;

        /* Put the values in state tarnsition matrix, starting with specific index,
           index will be incremented inside this method
        */
        virtual void updatePhi(gnsstk::Matrix<double>& Phi, int& index) const = 0;

        /*Put process noise components into corresponding matrix,
          starting with specific index, index will be incremented inside this method
        */
        virtual void updateQ(gnsstk::Matrix<double>& Q, int& index) const = 0;

        /* Put default values of state vector and it's covariance into corresponding matrices,
           starting with specific index, index will be incremented inside this method
         */
        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           int& index) const = 0;

        /* Put  partials of the measurements with respect to the unknowns into the design (geometry)
        matrix starting with specific indices, indices will be incremented inside this method
        */
        virtual void updateH(const gnsstk::IRinex& gData,
                             const gnsstk::TypeIDSet& types,
                             gnsstk::Matrix<double>& H,
                             int& col_0) = 0;

        virtual ParametersSet getAmbSet() const
        {
            return ParametersSet();
        }
        /* return number of unknowns
         */
        virtual int getNumUnknowns() const = 0;
    };
} // namespace pod
