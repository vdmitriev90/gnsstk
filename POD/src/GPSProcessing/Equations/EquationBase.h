#pragma once
#include "DataStructures.hpp"
#include "FilterParameter.h"
#include "RinexEpoch.h"

namespace pod
{
    class StateLayout;

    struct RowContext
    {
        /// Satellite identifier
        const gnsstk::SatID& sat;

        /// Pointer to full satellite data
        const gnsstk::typeValueMap* data;

        /// Measurement type (prefit TypeID, e.g. prefitL1, prefitC1)
        gnsstk::TypeID type;

        /// Row index in system matrices (H, z, W)
        int row;
    };

    class EquationBase
    {
      public:
        EquationBase() {};
        virtual ~EquationBase() {};

        /* update state of equations with new observational data */
        virtual void prepare(gnsstk::IRinex& gData) = 0;

        /* return set of FilerParameters, corresponding unknown parameters  for given equations */
        virtual ParametersSet getParameters() const = 0;

        /* Put the values in state tarnsition matrix, starting with specific index,
           index will be incremented inside this method
        */
        virtual void contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const = 0;

        /*Put process noise components into corresponding matrix,
          starting with specific index, index will be incremented inside this method
        */
        virtual void contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const = 0;

        /* Put default values of state vector and it's covariance into corresponding matrices,
           starting with specific index, index will be incremented inside this method
         */
        virtual void defStateAndCovariance(gnsstk::Vector<double>& x,
                                           gnsstk::Matrix<double>& P,
                                           const StateLayout& layout) const = 0;

        /* Put  partials of the measurements with respect to the unknowns into the design (geometry)
        matrix starting with specific indices, indices will be incremented inside this method
        */
        /// Fill ONE row of the design matrix
        virtual void fillRow(const RowContext& ctx, const StateLayout& layout, gnsstk::Matrix<double>& H) const = 0;


        virtual ParametersSet getAmbSet() const
        {
            return ParametersSet();
        }
        /* return number of unknowns
         */
        virtual int getNumUnknowns() const = 0;
    };
} // namespace pod
