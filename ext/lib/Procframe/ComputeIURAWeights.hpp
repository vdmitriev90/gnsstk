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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2006, 2007, 2008, 2011
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
 * @file ComputeIURAWeights.hpp
 * This class computes satellites weights based on URA Index and is
 * meant to be used with GNSS data structures.
 */

#ifndef GPSTK_COMPUTEIURAWEIGHTS_HPP
#define GPSTK_COMPUTEIURAWEIGHTS_HPP

#include "WeightBase.hpp"
#include "EngEphemeris.hpp"
#include "NavLibrary.hpp"
#include "ProcessingClass.hpp"


namespace gnsstk
{

      /// @ingroup DataStructures 
      //@{


      /** This class computes satellites weights based on URA Index.
       *
       * This class is meant to be used with the GNSS data structures objects
       * found in "DataStructures" class.
       *
       * A typical way to use this class follows:
       *
       * @code
       *   RinexObsStream rin("ebre0300.02o");
       *   RinexNavStream rnavin("brdc0300.02n");
       *   RinexNavData rNavData;
       *   GPSEphemerisStore bceStore;
       *   while (rnavin >> rNavData) bceStore.addEphemeris(rNavData);
       *   bceStore.SearchPast();  // This is the default
       *
       *      // Declare a GDS object
       *   gnssRinex gRin;
       *
       *      // Create a 'ComputeIURAWeights' object
       *   ComputeIURAWeights iuraW(bceStore);
       *
       *   while(rin >> gRin)
       *   {
       *      gRin >> iuraW;
       *   }
       * @endcode
       *
       * The "ComputeIURAWeights" object will visit every satellite in the
       * GNSS data structure that is "gRin" and will try to compute its weight
       * based on the corresponding IURA. For precise ephemeris, a fixed value
       * of IURA = 0.1 m will be set, returning a weight of 100.
       *
       * When used with the ">>" operator, this class returns the same incoming
       * data structure with the weights inserted along their corresponding
       * satellites. Be warned that if it is not possible to compute the
       * weight for a given satellite, it will be summarily deleted from
       * the data structure.
       *
       * @sa ComputeMOPSWeights.hpp.
       */
   class ComputeIURAWeights : public WeightBase, public ProcessingClass
   {
   public:

         /** Common constructor
          *
          * @param bcephem   GPSEphemerisStore object holding the ephemeris.
          */
      ComputeIURAWeights(NavLibrary& navLib)
         : navLib_(navLib)
      { };


         /** Returns a satTypeValueMap object, adding the new data generated
          *  when calling this object.
          *
          * @param gData     Data object holding the data.
          */
      virtual SatTypePtrMap& Process( const CommonTime& time,
                                        SatTypePtrMap& gData );


         /** Returns a gnnsRinex object, adding the new data generated
          *  when calling this object.
          *
          * @param gData    Data object holding the data.
          */
      virtual IRinex& Process(IRinex& gData)
      { Process(gData.getHeader().epoch, gData.getBody()); return gData; };


         /// Returns a string identifying this object.
      virtual std::string getClassName(void) const;


         /// Destructor
      virtual ~ComputeIURAWeights() {};


   protected:

         /// Pointer to default broadcast ephemeris to be used.
      NavLibrary& navLib_;

   }; // End of class 'ComputeIURAWeights'

      //@}

}  // End of namespace gnsstk

#endif // GPSTK_COMPUTEIURAWEIGHTS_HPP
