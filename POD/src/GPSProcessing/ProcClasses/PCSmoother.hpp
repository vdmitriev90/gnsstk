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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007, 2008, 2011
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
 * @file PCSmoother.hpp
 * This class smoothes PC code observables using the corresponding LC
 * phase observable.
 */

#ifndef GPSTK_PCSMOOTHER_HPP
#define GPSTK_PCSMOOTHER_HPP

#include "CodeSmoother.hpp"


namespace pod
{

      /// @ingroup DataStructures
      //@{


      /** This class smoothes the PC (ionosphere-free) code observable using
       * the corresponding LC phase observable.
       *
       * This class is meant to be used with the GNSS data structures objects
       * found in "DataStructures" class.
       *
       * A typical way to use this class follows:
       *
       * @code
       *   RinexObsStream rin("ebre0300.02o");
       *
       *   gnssRinex gRin;
       *
       *      // We MUST mark at least some cycle slips
       *   OneFreqCSDetector markCSL1;
       *
       *   PCSmoother smoothPC;
       *
       *   while(rin >> gRin)
       *   {
       *      gRin >> markCSL1 >> smoothPC;
       *   }
       * @endcode
       *
       * The "PCSmoother" object will visit every satellite in the GNSS data
       * structure that is "gRin" and will smooth the PC code observation using
       * the corresponding LC phase observation.
       *
       * By default, the algorithm will check both the CSL1 and CSL2 index for
       * cycle slip information. You can change these settings in the
       * constructor and also using the appropriate methods.
       *
       * When used with the ">>" operator, this class returns the same incoming
       * data structure with the PC code observation smoothed (unless the
       * resultType field is changed). Be warned that if a given satellite does
       * not have the observations required, it will be summarily deleted from
       * the data structure.
       *
       * Another important parameter is the maxWindowSize field. By default, it
       * is set to 100 samples. You may adjust that with the setMaxWindowSize()
       * method:
       *
       * @code
       *   PCSmoother smoothPC;
       *   smoothPC.setMaxWindowSize(35);
       * @endcode
       *
       * A window of 100 samples is typical and appropriate when working with
       * data sampled at 1 Hz. Note that the PC observable doesn't suffer the
       * effect of ionosphere drift.
       *
       * @sa CodeSmoother.hpp for base class.
       *
       * \warning Code smoothers are objets that store their internal state,
       * so you MUST NOT use the SAME object to process DIFFERENT data streams.
       *
       */
   class PCSmoother : public CodeSmoother
   {
   public:

         /// Default constructor: PC/LC observables, window 100.
      PCSmoother()
            : CodeSmoother(ObsSlot::CodeIonoFree, ObsSlot::PhaseIonoFree, 100)
      { };


         /** Common constructor
          *
          * @param mwSize   Maximum size of filter window, in samples.
          */
      explicit PCSmoother(int mwSize)
            : CodeSmoother(ObsSlot::CodeIonoFree, ObsSlot::PhaseIonoFree, mwSize)
      { };


         /** Returns a satTypeValueMap object, adding the new data generated
          *  when calling this object.
          *
          * @param gData     Data object holding the data.
          */
      virtual gnsstk::SatTypePtrMap& Process(gnsstk::SatTypePtrMap& gData);


         /** Method to set the maximum size of filter window, in samples.
          *
          * @param maxSize       Maximum size of filter window, in samples.
          */
      virtual PCSmoother& setMaxWindowSize(const int& maxSize);


         /// Returns a string identifying this object.
      virtual std::string getClassName(void) const;


         /// Destructor
      virtual ~PCSmoother() {};


   private:

         /// A structure used to store filter data for a SV.
      struct filterData
      {
            // Default constructor initializing the data in the structure
         filterData() : windowSize(1), previousCode(0.0), previousPhase(0.0) {};

         int windowSize;       ///< The filter window size.
         double previousCode;  ///< Accumulated mean bias (pseudorange - phase).
         double previousPhase; ///< Accumulated mean bias sigma squared.
      };


         /// Map holding the information regarding every satellite
      std::map<gnsstk::SatID, filterData> SmoothingData;


         /** Compute the smoothed PC observable.
          *
          * @param sat        Satellite object.
          * @param code       Code measurement.
          * @param phase      Phase measurement.
          * @param flag1      Cycle slip flag in L1.
          * @param flag2      Cycle slip flag in L2.
          */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
      virtual double getSmoothing( const gnsstk::SatID& sat,
                                   const double& code,
                                   const double& phase,
                                   const double& flag1,
                                   const double& flag2 );
#pragma clang diagnostic pop

   }; // End of class 'PCSmoother'

      //@}

}  // namespace pod

#endif   // GPSTK_PCSMOOTHER_HPP
