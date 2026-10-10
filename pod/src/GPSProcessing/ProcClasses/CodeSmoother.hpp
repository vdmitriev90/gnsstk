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
 * @file CodeSmoother.hpp
 * This class smoothes a given code observable using the corresponding
 * phase observable.
 */

#ifndef GPSTK_CODESMOOTHER_HPP
#define GPSTK_CODESMOOTHER_HPP

#include "ProcessingClass.hpp"
#include "ObservationResolver.h"

namespace pod
{
    /// @ingroup DataStructures
    //@{

    /** This class smoothes a given code observable using the corresponding
       *  phase observable.
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
       *   OneFreqCSDetector markCSC1;    // We MUST mark cycle slips
       *   CodeSmoother smoothC1;
       *
       *   while(rin >> gRin)
       *   {
       *      gRin >> markCSC1 >> smoothC1;
       *   }
       * @endcode
       *
       * The "CodeSmoother" object will visit every satellite in the GNSS data
       * structure that is "gRin" and will smooth the given code observation
       *  using the corresponding phase observation.
       *
       * By default, the algorithm will use the first-band code/phase observables
       * resolved per satellite system via ObservationResolver.
       *
       * When used with the ">>" operator, this class returns the same incoming
       * data structure with the code observation smoothed. Be warned that if a given satellite
       * does not have the  observations required, it will be summarily deleted
       * from the data structure.
       *
       * Another important parameter is the maxWindowSize field. By default, it
       * is set to 100 samples (you may adjust that with the setMaxWindowSize()
       * method).
       *
       * \warning Code smoothers are objets that store their internal state,
       * so you MUST NOT use the SAME object to process DIFFERENT data streams.
       *
       */
    class CodeSmoother : public gnsstk::ProcessingClass
    {
      public:
        /// Default constructor: first-band code/phase, window 100.
        CodeSmoother()
            : codeSlot(ObsSlot::FirstBandCode)
            , phaseSlot(ObsSlot::FirstBandPhase)
            , maxWindowSize(100) {};

        /** Common constructor
          *
          * @param codeSlot    ObsSlot for the code observable to smooth.
          * @param phaseSlot   ObsSlot for the corresponding phase observable.
          * @param mwSize      Maximum size of filter window, in samples.
          */
        CodeSmoother(ObsSlot codeSlot, ObsSlot phaseSlot, int mwSize = 100);

        /** Returns a satTypeValueMap object, adding the new data generated
          *  when calling this object.
          *
          * @param gData     Data object holding the data.
          */
        virtual gnsstk::SatTypePtrMap& Process(gnsstk::SatTypePtrMap& gData);

        /** Returns a gnnsRinex object, adding the new data generated when
          *  calling this object.
          *
          * @param gData    Data object holding the data.
          */
        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData)
        {
            Process(gData.getBody());
            return gData;
        };

        /// Method to get the code observation slot being used.
        virtual ObsSlot getCodeSlot() const
        {
            return codeSlot;
        };

        /** Method to set the code observation slot to be used.
          *
          * @param s    ObsSlot for code observable
          */
        virtual CodeSmoother& setCodeSlot(ObsSlot s)
        {
            codeSlot = s;
            return (*this);
        };

        /// Method to get the phase observation slot being used.
        virtual ObsSlot getPhaseSlot() const
        {
            return phaseSlot;
        };

        /** Method to set the phase observation slot to be used.
          *
          * @param s    ObsSlot for phase observable
          */
        virtual CodeSmoother& setPhaseSlot(ObsSlot s)
        {
            phaseSlot = s;
            return (*this);
        };

        /// Method to get the maximum size of filter window, in samples.
        virtual int getMaxWindowSize() const
        {
            return maxWindowSize;
        };

        /** Method to set the maximum size of filter window, in samples.
          *
          * @param maxSize       Maximum size of filter window, in samples.
          */
        virtual CodeSmoother& setMaxWindowSize(const int& maxSize);

        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;

        /// Destructor
        virtual ~CodeSmoother() {};

      protected:
        /// ObsSlot for the code observable to smooth.
        ObsSlot codeSlot;

        /// ObsSlot for the corresponding phase observable.
        ObsSlot phaseSlot;

        /// Resolver used to map ObsSlots to TypeIDs per satellite system.
        ObservationResolver resolver_;

        /// Maximum size of filter window, in samples.
        int maxWindowSize;

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

        /** Compute the smoothed code observable.
          *
          * @param sat        Satellite object.
          * @param code       Code measurement.
          * @param phase      Phase measurement.
          * @param flag       Cycle slip flag.
          */
        virtual double getSmoothing(const gnsstk::SatID& sat, const double& code, const double& phase, const double& flag);

    }; // End of class 'CodeSmoother'

    //@}

} // namespace pod

#endif // GPSTK_CODESMOOTHER_HPP
