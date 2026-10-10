//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 3.0 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//  
//  Copyright 2004, The University of Texas at Austin
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007
//
//============================================================================

//============================================================================
//
//This software developed by Applied Research Laboratories at the University of
//Texas at Austin, under contract to an agency or agencies within the U.S. 
//Department of Defense. The U.S. Government retains all rights to use,
//duplicate, distribute, disclose, or release this software. 
//
//Pursuant to DoD Directive 523024 
//
// DISTRIBUTION STATEMENT A: This software has been approved for public 
//                           release, distribution is unlimited.
//
//=============================================================================

/**
 * @file SolidTides.hpp
 * Computes the effect of solid Earth tides at a given position and epoch.
 */

#ifndef SOLIDTIDES_HPP
#define SOLIDTIDES_HPP

#include <cmath>
#include <string>

#include "CommonTime.hpp"
#include "Triple.hpp"
#include "Position.hpp"
#include "GNSSconstants.hpp"
#include "AstronomicalFunctions.hpp"
#include "SunPosition.hpp"
#include "MoonPosition.hpp"



namespace gnsstk
{
      /// @ingroup GPSsolutions 
      //@{

      /** This class computes the effect of solid Earth tides at a given
       * position and epoch.
       *
       * The model follows the IERS Conventions (2010), section 7.1.1
       * (port of the IERS routine DEHANTTIDEINEL):
       *  - Step 1: in-phase degree 2 (latitude dependent h2, l2) and
       *    degree 3 corrections; out-of-phase diurnal and semidiurnal
       *    corrections; latitude dependence corrections (l(1)).
       *  - Step 2: frequency dependent corrections for diurnal band
       *    (31 terms) and long-period band (5 terms).
       *
       * The permanent tide is NOT removed, i.e. the result corresponds
       * to the "conventional tide free" system recommended by IERS.
       *
       * Given the limitations of the algorithms used to compute the Sun
       * and Moon positions, this class is limited to the period between
       * March 1st, 1900 and February 28th, 2100.
       *
       */
   class SolidTides
   {
   public:

         /// Default constructor
      SolidTides(){}

         /// Destructor
      virtual ~SolidTides() {}


         /** Returns the effect of solid Earth tides (meters) at the given
          * position and epoch, in the Up-East-North (UEN) reference frame.
          *
          * @param[in] t Epoch to look up
          * @param[in] p Position of interest
          *
          * @return a Triple with the solid tidal effect, in meters and in
          * the UEN reference frame.
          *
          * @throw InvalidRequest If the request can not be completed for any
          * reason, this is thrown. The text may have additional information
          * as to why the request failed.
          */
      Triple getSolidTide(const CommonTime& t, const Position& p) const;


         /** Returns the effect of solid Earth tides (meters) in ECEF frame
          * according to IERS Conventions 2010.
          *
          * @param[in] xsta  Station position, ECEF [m]
          * @param[in] xsun  Sun position, ECEF [m]
          * @param[in] xmon  Moon position, ECEF [m]
          * @param[in] T     Julian centuries of TT since J2000
          * @param[in] fhr   Hours of UT in the day
          */
      static Triple dehantTideInel(const Triple& xsta,
                                   const Triple& xsun,
                                   const Triple& xmon,
                                   double T,
                                   double fhr);

         /** Computes time arguments required by dehantTideInel().
          *
          * @param[in]  t    Epoch (GPS, GAL, QZS, IRN, BDT, UTC, TAI, TT)
          * @param[out] T    Julian centuries of TT since J2000
          * @param[out] fhr  Hours of UT (approximated by UTC) in the day
          */
      static void computeTimeArgs(const CommonTime& t, double& T, double& fhr);

   private:

         /// Out-of-phase corrections induced by diurnal band
      static Triple st1idiu(const Triple& xsta, const Triple& xsun,
                            const Triple& xmon, double fac2sun,
                            double fac2mon);

         /// Out-of-phase corrections induced by semidiurnal band
      static Triple st1isem(const Triple& xsta, const Triple& xsun,
                            const Triple& xmon, double fac2sun,
                            double fac2mon);

         /// Corrections induced by the latitude dependence of l(1)
      static Triple st1l1(const Triple& xsta, const Triple& xsun,
                          const Triple& xmon, double fac2sun,
                          double fac2mon);

         /// Frequency dependent corrections in the diurnal band
      static Triple step2diu(const Triple& xsta, double fhr, double T);

         /// Frequency dependent corrections in the long-period band
      static Triple step2lon(const Triple& xsta, double T);


   }; // end class SolidTides


   //@}

} // namespace gpstk
#endif  // SOLIDTIDES_HPP
