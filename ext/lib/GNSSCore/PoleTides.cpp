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
 * @file PoleTides.cpp
 * Computes the effect of pole tides at a given position and epoch.
 */

#include "PoleTides.hpp"
#include "MJD.hpp"


namespace gnsstk
{

      /* Returns the effect of pole tides (meters) at the given
       * position and epoch, in the Up-East-North (UEN) reference frame.
       *
       * @param[in]  t Epoch to look up
       * @param[in]  p Position of interest
       *
       * @return Triple in meters: [0] = Up, [1] = East, [2] = North.
       *
       * @throw InvalidRequest if pole coordinates have not been set.
       */
   Triple PoleTides::getPoleTide( const CommonTime& t,
                                  const Position& p ) const
   {
         if (!xySet_)
         {
            InvalidRequest ir("PoleTides: pole coordinates (xp, yp) are not "
                              "set; call setXY() first.");
            GNSSTK_THROW(ir);
         }

         double xs(0.0), ys(0.0);
         getSecularPole(t, xs, ys);

            // Wobble variables, arcseconds (IERS 2010, Eq. 7.24)
         const double m1(xdisp_ - xs);
            // NOTE: the minus sign is intentional. IERS pole y-coordinate
            // is positive toward 90 deg W longitude, whereas m2 is defined
            // positive toward 90 deg E: m2 = -(yp - ys).
         const double m2(-(ydisp_ - ys));

            // IERS uses geocentric co-latitude theta and longitude lambda
         const double theta((90.0 - p.geocentricLatitude()) * DEG_TO_RAD);
         const double lambda(p.longitude() * DEG_TO_RAD);

         const double sinlon(std::sin(lambda));
         const double coslon(std::cos(lambda));
         const double c1(m1*coslon + m2*sinlon);

            // IERS 2010, Eq. 7.26 (meters): radial, south (theta), east (lambda)
         const double sr(-0.033 * std::sin(2.0*theta) * c1);
         const double st(-0.009 * std::cos(2.0*theta) * c1);
         const double sl(+0.009 * std::cos(theta) * (m1*sinlon - m2*coslon));

            // Result order: [Up, East, North]; North = -S_theta
         return Triple(sr, sl, -st);

   }  // End of method 'PoleTides::getPoleTide()'



      /* Method to set the pole displacement parameters
       *
       * @param x     Pole displacement x, in arcseconds
       * @param y     Pole displacement y, in arcseconds
       *
       * @return This same object
       */
   PoleTides& PoleTides::setXY( const double& x,
                                const double& y )
   {

      xdisp_ = x;
      ydisp_ = y;
      xySet_ = true;

      return (*this);

   }  // End of method 'PoleTides::setXY()'



      /* Secular pole (IERS Conventions 2010, Chapter 7 updated 2018,
       * Eq. 7.25): xs = 55.0 + 1.677*t mas, ys = 320.5 + 3.460*t mas,
       * t in Julian years since J2000.0.
       */
   void PoleTides::getSecularPole( const CommonTime& t,
                                   double& xs,
                                   double& ys )
   {
      const double dt((MJD(t).mjd - 51544.5) / 365.25);

      xs = (55.0 + 1.677 * dt) * 1.0e-3;
      ys = (320.5 + 3.460 * dt) * 1.0e-3;

   }  // End of method 'PoleTides::getSecularPole()'



}  // End of namespace gnsstk
