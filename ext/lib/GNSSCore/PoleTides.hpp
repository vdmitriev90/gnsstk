//==============================================================================
//
//  This file is part of GNSSTk, the ARL:UT GNSS Toolkit.
//
//  The GNSSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 3.0 of the License, or
//  any later version.
//
//  The GNSSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GNSSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//
//  This software was developed by Applied Research Laboratories at the
//  University of Texas at Austin.
//  Copyright 2004-2022, The Board of Regents of The University of Texas System
//
//==============================================================================

//==============================================================================
//
//  This software was developed by Applied Research Laboratories at the
//  University of Texas at Austin, under contract to an agency or agencies
//  within the U.S. Department of Defense. The U.S. Government retains all
//  rights to use, duplicate, distribute, disclose, or release this software.
//
//  Pursuant to DoD Directive 523024
//
//  DISTRIBUTION STATEMENT A: This software has been approved for public
//                            release, distribution is unlimited.
//
//==============================================================================

/**
 * @file PoleTides.hpp
 * Computes the effect of pole tides at a given position and epoch.
 */

#ifndef POLETIDES_HPP
#define POLETIDES_HPP

#include <cmath>
#include <string>

#include "Triple.hpp"
#include "Position.hpp"
#include "CommonTime.hpp"
#include "GNSSconstants.hpp"
#include "GNSSconstants.hpp"



namespace gnsstk
{

      /// @ingroup GNSSsolutions
      //@{

      /** This class computes the effect of the solid Earth pole tide
       *  ("rotational deformation due to polar motion") at a given
       *  position and epoch.
       *
       *  Model: IERS Conventions 2010 (IERS TN36), Chapter 7, Section
       *  7.1.4, as updated by the IERS Conventions Centre (Chapter 7
       *  update, 2018): displacement per Eq. 7.26 and the linear
       *  "secular pole" (Eq. 7.25) as the reference pole:
       *
       *  \li xs = 55.0 + 1.677 t  [mas]
       *  \li ys = 320.5 + 3.460 t [mas], t in years since J2000.0
       *
       *  https://iers-conventions.obspm.fr/chapter7.php
       *
       *  The pole coordinates x, y (arcseconds) are e.g. from IERS
       *  EOP 20 C04 series.
       *
       *  Maximum displacements: about 25 mm vertical, 7 mm horizontal.
       *
       *  @note Ocean pole tide loading (Section 7.1.5) is not included.
       */
   class PoleTides
   {
   public:

         /** Default constructor. Pole coordinates are NOT set;
          *  setXY() must be called before getPoleTide(t, p).
          */
      PoleTides() : xdisp_(0.0), ydisp_(0.0), xySet_(false) {};


         /** Common constructor
          *
          * @param x     Pole displacement x, in arcseconds
          * @param y     Pole displacement y, in arcseconds
          */
      PoleTides( const double& x,
                 const double& y )
         : xdisp_(x), ydisp_(y), xySet_(true) {};


         /** Returns the solid Earth pole tide displacement at the given
          *  position and epoch.
          *
          * @param[in]  t Epoch to look up
          * @param[in]  p Position of interest
          *
          * @return Triple in meters, component order:
          *    [0] = Up, [1] = East, [2] = North.
          *
          * @throw InvalidRequest if pole coordinates have not been set
          *    via the (x, y) constructor or setXY().
          */
      Triple getPoleTide( const CommonTime& t,
                          const Position& p ) const;


         /** Sets pole coordinates and returns the solid Earth pole tide
          *  displacement at the given position and epoch.
          *
          * @param[in] t Epoch to look up
          * @param[in] p Position of interest
          * @param[in] x Pole coordinate x, in arcseconds
          * @param[in] y Pole coordinate y, in arcseconds
          *
          * @return Triple in meters, component order:
          *    [0] = Up, [1] = East, [2] = North.
          */
      Triple getPoleTide( const CommonTime& t,
                          const Position& p,
                          const double& x,
                          const double& y )
      { setXY(x,y); return (getPoleTide(t, p)); };


         /** Method to set the pole displacement parameters
          *
          * @param x     Pole displacement x, in arcseconds
          * @param y     Pole displacement y, in arcseconds
          *
          * @return This same object
          */
      PoleTides& setXY( const double& x,
                        const double& y );


         /// Method to get the x pole displacement parameter, in arcseconds
      double getX(void) const
      { return xdisp_; };


         /// Method to get the y pole displacement parameter, in arcseconds
      double getY(void) const
      { return ydisp_; };


         /** Secular pole coordinates (IERS 2010 Ch. 7 update 2018,
          *  Eq. 7.25), in arcseconds.
          */
      static void getSecularPole( const CommonTime& t,
                                  double& xs,
                                  double& ys );


         /// Destructor
      virtual ~PoleTides() {};


   private:


         /// Pole displacement x, in arcseconds
      double xdisp_;


         /// Pole displacement y, in arcseconds
      double ydisp_;


         /// True once pole coordinates have been explicitly set
      bool xySet_;


   }; // End of class 'PoleTides'

      //@}

}  // End of namespace gnsstk
#endif   // POLETIDES_HPP
