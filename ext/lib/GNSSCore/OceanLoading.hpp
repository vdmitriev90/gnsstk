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
 * @file OceanLoading.hpp
 * This class computes the effect of ocean tides loading at a given station
 * and epoch, following the IERS Conventions (2010) HARDISP algorithm.
 */

#ifndef OCEANLOADING_HPP
#define OCEANLOADING_HPP

#include <map>
#include <string>
#include <vector>
#include "Triple.hpp"
#include "Matrix.hpp"
#include "CommonTime.hpp"
#include "BLQDataReader.hpp"


namespace gnsstk
{

      /// @ingroup GNSSsolutions
      //@{


      /** This class computes the effect of ocean tides loading at a given
       *  station and epoch.
       *
       * A typical way to use this class follows:
       *
       * @code
       *      // Create a time object (any GNSS time system or UTC)
       *   CommonTime time = CivilTime(2009, 6, 25, 1, 10, 45.0,
       *                               TimeSystem::UTC);
       *
       *      // Load ocean loading object with ocean tides harmonics data
       *   OceanLoading ocean("OceanLoading.blq");
       *
       *      // Compute ocean loading effect in Up-East-North [UEN]
       *      // reference frame
       *   Triple tides = ocean.getOceanLoading("ONSA", time);
       * @endcode
       *
       * The model is a C++ re-implementation of the IERS Conventions (2010)
       * routine HARDISP.F (and its support routines ADMINT, TDFRPH, SPLINE,
       * EVAL), Chapter 7.1.2:
       *
       *  - The 11 main constituents given in the BLQ file (M2, S2, N2, K2,
       *    K1, O1, P1, Q1, Mf, Mm, Ssa) are used to compute the ocean
       *    loading admittance, which is interpolated with cubic splines
       *    (separately for long-period, diurnal and semi-diurnal bands) to
       *    a total of 342 constituents of the Cartwright-Tayler-Edden
       *    tidal potential. This implicitly accounts for the nodal
       *    modulations (18.6 yr) and the minor tides.
       *  - Doodson arguments are computed from the IERS 2010 Delaunay
       *    arguments, with time argument in TT and day fraction in UTC.
       *    The input epoch is converted to UTC/TT using the leap seconds
       *    table of the library, so any time system supported by
       *    getTimeSystemCorrection() may be used. If the epoch time
       *    system is Any or Unknown, it is assumed to be GPS time.
       *
       * Differences with respect to the original IERS software: the
       * computation is done in double precision for a single epoch
       * (instead of the recursive equi-spaced series), and the admittance
       * spline model of each station is cached after its first use.
       *
       * Reference: Petit, G. and Luzum, B. (eds.), IERS Conventions (2010),
       * IERS Technical Note No. 36, Chapter 7.
       * This work is derived from, but is not, IERS Conventions software;
       * it is neither distributed nor endorsed by the IERS Conventions
       * Center.
       */
   class OceanLoading
   {
   public:

         /// Number of constituents provided by a BLQ file.
      static const int NUM_BLQ_CONSTITUENTS = 11;

         /// Number of constituents used in the HARDISP model.
      static const int NUM_CONSTITUENTS = 342;


         /** Common constructor
          *
          * @param filename  Name of BLQ file containing ocean tide
          *                  harmonics data.
          *
          * @warning If filename is not given, this class will look for
          * a file named "oceanloading.blq" in the current directory.
          */
      explicit OceanLoading(const std::string& filename = "oceanloading.blq")
         : blqData(filename), fileData(filename)
      {}


         /** Returns the effect of ocean tides loading (meters) at the given
          *  station and epoch, in the Up-East-North (UEN) reference frame.
          *
          * @param[in] name  Station name (case is NOT relevant).
          * @param[in] t     Epoch to look up. Its time system is taken into
          *                  account (Any/Unknown are assumed to be GPS).
          *
          * @return a Triple with the ocean tides loading effect, in meters
          * and in the UEN reference frame. If the station is not present
          * in the BLQ file, the result is zero.
          *
          * @throw InvalidRequest If the request can not be completed for any
          * reason, this is thrown. The text may have additional information
          * about the reason the request failed.
          */
      Triple getOceanLoading( const std::string& name,
                              const CommonTime& t );


         /// Returns the name of BLQ file containing ocean tides harmonics data.
      virtual std::string getFilename() const
      { return fileData; }


         /** Sets the name of BLQ file containing ocean tides harmonics data.
          *
          * @param name      Name of BLQ tides harmonics data file.
          */
      virtual OceanLoading& setFilename(const std::string& name);


         /// Destructor
      virtual ~OceanLoading() = default;


   private:

         /// Doodson arguments (degrees) and their rates (cycles/day).
      struct DoodsonArgs
      {
         double phase[6];
         double freq[6];
      };


         /// Interpolated amplitudes (m) and phase offsets (degrees) of all
         /// the 342 constituents, for Up, West and South components.
      struct StationModel
      {
         std::vector<double> amp[3];
         std::vector<double> phase[3];
      };


         /// Object to read BLQ ocean tides harmonics data file
      BLQDataReader blqData;


         /// Name of BLQ file containing ocean tides harmonics data.
      std::string fileData;


         /// Cache of station models, indexed by upper-case station name.
      std::map<std::string, StationModel> stationModels;


         /** Compute the Doodson arguments and their rates at the given
          * epoch. This routine is based on IERS routine TDFRPH.F.
          *
          * @param time      Epoch of interest
          */
      static DoodsonArgs getDoodsonArgs(const CommonTime& time);


         /** Build the 342-constituent model of a station from the BLQ
          * harmonics. This routine is based on IERS routine ADMINT.F.
          *
          * @param harmonics Matrix of 6x11 BLQ harmonics
          * @param args      Doodson arguments used to get frequencies
          */
      static StationModel buildStationModel(const Matrix<double>& harmonics,
                                            const DoodsonArgs& args);


   }; // End of class 'OceanLoading'

      //@}

}  // End of namespace gnsstk
#endif   // OCEANLOADING_HPP
