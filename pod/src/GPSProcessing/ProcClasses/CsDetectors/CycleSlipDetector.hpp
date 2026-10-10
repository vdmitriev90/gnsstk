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
 * @file CycleSlipDetector.hpp
 * This is an abstract base class for objects to detect cycle slip event.
 */

#ifndef CYCLE_SLIP_DETECTOR_HPP
#define CYCLE_SLIP_DETECTOR_HPP

#include "ProcessingClass.hpp"
#include "ObservationResolver.h"

namespace pod
{

    class CycleSlipDetector : public gnsstk::ProcessingClass
    {

      public:
        CycleSlipDetector(gnsstk::TypeID ot1);

        CycleSlipDetector(gnsstk::TypeID ot1, double dtMax, bool useLli);

        /** Returns a satTypeValueMap object, adding the new data generated
	 *  when calling this object.
	 *
	 * @param epoch     Time of observations.
	 * @param gData     Data object holding the data.
	 * @param epochflag Epoch flag.
	 */
        virtual gnsstk::SatTypePtrMap& Process(const gnsstk::CommonTime& epoch,
                                       gnsstk::SatTypePtrMap& gData,
                                       const short& epochflag = 0);

        /** Returns a gnnsRinex object, adding the new data generated when
		 *  calling this object.
		 *
		 * @param gData    Data object holding the data.
		 */
        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData);

        virtual CycleSlipDetector& setIsReprocess(bool isrepro)
        {
            isReprocess = isrepro;
            return *this;
        }

        virtual bool getIsReprocess() const
        {
            return isReprocess;
        }

        /** Method to set the maximum interval of time allowed between two
		 *  successive epochs.
		 *
		 * @param maxDelta      Maximum interval of time, in seconds
		 */
        virtual CycleSlipDetector& setDeltaTMax(const double& maxDelta);

        /** Method to get the maximum interval of time allowed between two
		 *  successive epochs, in seconds.
		 */
        virtual double getDeltaTMax() const
        {
            return deltaTMax;
        };
        /** Method to set whether the LLI indexes will be used as
 *  an aid or not.
 *
 * @param use   Boolean value enabling/disabling LLI check.
 */
        virtual CycleSlipDetector& setUseLLI(const bool& use)
        {
            useLLI = use;
            return (*this);
        };

        virtual CycleSlipDetector& setUseEpochFlag(const bool& use)
        {
            useEpochFlag = use;
            return (*this);
        };

        /// Method to know if the LLI check is enabled or disabled.
        virtual bool getUseLLI() const
        {
            return useLLI;
        };

        /// Method to know if the LLI check is enabled or disabled.
        virtual bool getUseEpochFlag() const
        {
            return useEpochFlag;
        };

      protected:
        enum DetectionResult
        {
            NotDetected = 0,
            CsDetected = 1,
            NotEnoughData = 2,
            CsDetectedByMW = 3,
            CsDetectedByLI2 = 4,
        };

        virtual DetectionResult getDetection(const gnsstk::CommonTime& epoch,
                                             const gnsstk::SatID& sat,
                                             gnsstk::typeValueMap& tvMap,
                                             const short& epochflag,
                                             const double& li,
                                             const double& lli1,
                                             const double& lli2) = 0;

        ObservationResolver resolver_;

        /// Type of observation.
        gnsstk::TypeID obsType;

        //indicates, if CS marker will has to only reset CS flags for REJECTED satellites(SatStaatus==RelectedByCsCatcher)
        // false by default
        bool isReprocess;

        /// This field tells whether to use or ignore the LLI indexes as
        /// an aid.
        bool useLLI;

        /// This field tells whether to use or ignore the Epoch flag value as
        /// an aid.
        bool useEpochFlag;

        /// Maximum interval of time allowed between two successive epochs,
        /// in seconds.
        double deltaTMax;
    };

} // namespace pod

#endif // CYCLE_SLIP_DETECTOR_HPP