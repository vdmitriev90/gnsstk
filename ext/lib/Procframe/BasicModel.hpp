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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007, 2008, 2009, 2011
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
 * @file BasicModel.hpp
 * This is a class to compute the basic parts of a GNSS model, i.e.:
 * Geometric distance, relativity correction, satellite position and
 * velocity at transmission time, satellite elevation and azimuth, etc.
 */

#ifndef GPSTK_BASICMODEL_HPP
#define GPSTK_BASICMODEL_HPP

#include "ProcessingClass.hpp"
#include "EphemerisRange.hpp"
#include "NavLibrary.hpp"


namespace gnsstk
{
      /// @ingroup GPSsolutions 
      //@{

      /** This is a class to compute the basic parts of a GNSS model, like
       *  geometric distance, relativity correction, satellite position and
       *  velocity at transmission time, satellite elevation and azimuth, etc.
       *
       * This class is intended to be used with GNSS Data Structures (GDS).
       * It is a more modular alternative to classes such as ModelObs
       * and ModelObsFixedStation.
       *
       * A typical way to use this class follows:
       *
       * @code
       *      // Input observation file stream
       *   RinexObsStream rin("ebre0300.02o");
       *
       *      // Load the precise ephemeris file
       *   SP3EphemerisStore sp3Eph;
       *   sp3Eph.loadFile("igs11513.sp3");
       *
       *      // Reference position of receiver station
       *   Position nominalPos(4833520.2269, 41537.00768, 4147461.489);
       *
       *      // Some more code and definitions here...
       *
       *   gnssRinex gRin;  // GNSS data structure for fixed station data
       *
       *      // Set defaults of models. A typical C1-based modeling is used
       *   BasicModel model( nominalPos, sp3Eph );
       *
       *   while(rin >> gRin)
       *   {
       *
       *         // Apply the model on the GDS
       *      gRin >> model;
       *   }
       *
       * @endcode
       *
       * The "BasicModel" object will visit every satellite in
       * the GNSS data structure that is "gRin" and will try to compute
       * its model: Geometric distance, relativity delay, satellite position
       * at transmission time, satellite elevation and azimuth, etc.
       *
       * When used with the ">>" operator, this class returns the same
       * incoming data structure with the extra data inserted along their
       * corresponding satellites. Be warned that if a given satellite does
       * not have ephemeris information, it will be summarily deleted
       * from the data structure.
       *
       * @sa ModelObs.hpp and ModelObsFixedStation.hpp for classes carrying
       * out a more complete model.
       *
       */
   class BasicModel : public ProcessingClass
   {
   public:

         /// Default constructor deleted - NavLibrary reference is required
      BasicModel() = delete;

      /** Explicit constructor, taking as input reference station
          *  coordinates, ephemeris to be used and whether TGD will
          *  be computed or not.
          *
          * @param RxCoordinates Reference station coordinates.
          * @param navLib        NavLibrary object to be used.
          * @param dObservable   Observable type to be used by default.
          * @param applyTGD      Whether or not C1 observable will be
          *                      corrected from TGD effect.
          * @param addTGD        Whether TGD value will be calculated and added to GDS.
          */
      BasicModel( const Position& RxCoordinates,
                  NavLibrary& navLib,
                  const TypeID& dObservable = TypeID::C1,
                  const bool& applyTGD = false,
                  const bool& addTGD = false );


         /** Explicit constructor without initial receiver position.
          *  Receiver position can be set later using setRxPosition().
          *
          * @param navLib        NavLibrary object to be used.
          * @param dObservable   Observable type to be used by default.
          * @param applyTGD      Whether or not C1 observable will be
          *                      corrected from TGD effect.
          * @param addTGD        Whether TGD value will be calculated and added to GDS.
          */
      BasicModel( NavLibrary& navLib,
                  const TypeID& dObservable = TypeID::C1,
                  const bool& applyTGD = false,
                  const bool& addTGD = false );


         /** Returns a satTypeValueMap object, adding the new data generated
          *  when calling a modeling object.
          *
          * @param time      Epoch.
          * @param gData     Data object holding the data.
          */
      virtual SatTypePtrMap& Process( const CommonTime& time,
                                        SatTypePtrMap& gData );



         /** Returns a gnnsRinex object, adding the new data generated when
          *  calling a modeling object.
          *
          * @param gData    Data object holding the data.
          */
      virtual IRinex& Process(IRinex& gData)
      { Process(gData.getHeader().epoch, gData.getBody()); return gData; };


         /// Method to get satellite elevation cut-off angle. By default, it
         /// is set to 10 degrees.
      virtual double getMinElev() const
      { return minElev; };


         /// Method to set satellite elevation cut-off angle. By default, it
         /// is set to 10 degrees.
      virtual BasicModel& setMinElev(double newElevation)
      { minElev = newElevation; return (*this); };


         /// Method to get the default observable for computations.
      virtual TypeID getDefaultObservable() const
      { return defaultObservable; };


         /** Method to set the default observable for computations.
          *
          * @param type      TypeID object to be used by default
          */
      virtual BasicModel& setDefaultObservable(const TypeID& type)
      { defaultObservable = type; return (*this); };

      virtual BasicModel& setDefaultInterval(double dt)
      {
          defInterval = dt; return (*this);
      };

      virtual double getDefaultInterval(double dt)
      {
           return dt; 
      };
      virtual BasicModel& useClkDrift(bool use)
      {
          useCdtDot = use; return (*this);
      };

         /// Method to get a reference to the NavLibrary used for ephemeris data.
      virtual NavLibrary& getNavLibrary()
      { return navLibrary; };

         /// Method to get a const reference to the NavLibrary used for ephemeris data.
      virtual const NavLibrary& getNavLibrary() const
      { return navLibrary; };


      /// Method to set the receiver position.
      virtual BasicModel& setRxPosition(const Position& pos)
      {
          rxPos = pos;
          return (*this);
      };

      /// Method to get the receiver position.
      virtual Position getRxPosition() const
      {
          return rxPos;
      };

         /// Returns a string identifying this object.
      virtual std::string getClassName(void) const;


         /// Destructor.
      virtual ~BasicModel() {};


   protected:

       /// Either estimated or "a priori" position of receiver
       Position rxPos;

         /// The elevation cut-off angle for accepted satellites.
         /// By default it is set to 10 degrees.
      double minElev;


         /// Reference to NavLibrary object for ephemeris data.
      NavLibrary& navLibrary;


         /// Default observable to be used when fed with GNSS data structures.
      TypeID defaultObservable;
        
      /// Whether the TGD effect will be applied to C1 observable or not.
      bool useTGD;
        
      ///Whether the TGD value will be calculated and added to GDS
      bool addTGD;
         
      /// Whether receiver clock linear drift partial derivative will be added  to GDS
      bool useCdtDot;
        
      /// indicates first entry
      bool isFirstTime;
       
      CommonTime currTime;

      CommonTime prevTime;

      double defInterval;

         /// Method to get TGD corrections.
      virtual double getTGDCorrections(const CommonTime& Tr, const SatID& sat);



   }; // End of class 'BasicModel'

      //@}

}  // End of namespace gnsstk

#endif   // GPSTK_BASICMODEL_HPP
