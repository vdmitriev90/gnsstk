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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2008, 2009, 2011
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
 * @file ComputeSatPCenter.hpp
 * This class computes the satellite antenna phase correction, in meters.
 */

#pragma once

#include "AntexReader.hpp"
#include "GNSSconstants.hpp"
#include "NavLibrary.hpp"
#include "ObservationResolver.h"
#include "Position.hpp"
#include "ProcessingClass.hpp"
#include "SatDataReader.hpp"
#include "StringUtils.hpp"
#include "SunPosition.hpp"
#include "Triple.hpp"

#include <cmath>
#include <sstream>
#include <string>

namespace pod
{

    /// @ingroup DataStructures
    //@{

    /** This class computes the satellite antenna phase correction, in meters.
     
       * The "ComputeSatPCenter" object visits every satellite in the GNSS
       * data structure and applies the corresponding satellite antenna phase
       * center correction directly to available code and phase observations.
       *
       * Be warned that if a given satellite does not have the required satellite
       * position data and no ephemeris is available, it fill be removed from the
       * data structure
       *
       */
    class ComputeSatPCenter : public gnsstk::ProcessingClass
    {
      public:
        /// Default constructor
        ComputeSatPCenter()
            : pEphemeris(nullptr)
            , nominalPos(0.0, 0.0, 0.0)
            , satData("PRN_GPS")
            , fileData("PRN_GPS")
            , pAntexReader(nullptr) {};

        /** Common constructor
          *
          * @param ephem     Satellite ephemeris.
          * @param stapos    Nominal position of receiver station.
          * @param filename  Name of "PRN_GPS"-like file containing
          *                  satellite data.
          *
          * @warning If filename is not given, this class will look for a
          * file named "PRN_GPS" in the current directory.
          */
        ComputeSatPCenter(gnsstk::NavLibrary& ephem, const gnsstk::Position& stapos, std::string filename = "PRN_GPS")
            : pEphemeris(&ephem)
            , nominalPos(stapos)
            , satData(filename)
            , fileData(filename)
            , pAntexReader(nullptr) {};

        /** Common constructor
          *
          * @param stapos    Nominal position of receiver station.
          * @param filename  Name of "PRN_GPS"-like file containing
          *                  satellite data.
          *
          * @warning If filename is not given, this class will look for a
          * file named "PRN_GPS" in the current directory.
          */
        ComputeSatPCenter(const gnsstk::Position& stapos, std::string filename = "PRN_GPS")
            : pEphemeris(nullptr)
            , nominalPos(stapos)
            , satData(filename)
            , fileData(filename)
            , pAntexReader(nullptr) {};

        /** Common constructor
          *
          * @param filename  Name of "PRN_GPS"-like file containing
          *                  satellite data.
          *
          * @warning If filename is not given, this class will look for a
          * file named "PRN_GPS" in the current directory.
          */
        ComputeSatPCenter(const std::string& filename)
            : pEphemeris(nullptr)
            , satData(filename)
            , fileData(filename)
            , pAntexReader(nullptr) {};

        /** Common constructor. Uses satellite antenna data from an Antex file.
          *
          * @param ephem     Satellite ephemeris.
          * @param stapos    Nominal position of receiver station.
          * @param antexObj  AntexReader object containing satellite
          *                  antenna data.
          *
          * @warning If 'AntexReader' object holds an Antex file with relative
          * antenna data, a simple satellite phase center model will be used.
          */
        ComputeSatPCenter(gnsstk::NavLibrary& ephem, const gnsstk::Position& stapos, gnsstk::AntexReader& antexObj)
            : pEphemeris(&ephem)
            , nominalPos(stapos)
            , pAntexReader(&antexObj) {};

        /** Common constructor. Uses satellite antenna data from an Antex file.
          *
          * @param stapos    Nominal position of receiver station.
          * @param antexObj  AntexReader object containing satellite
          *                  antenna data.
          *
          * @warning If 'AntexReader' object holds an Antex file with relative
          * antenna data, a simple satellite phase center model will be used.
          */
        ComputeSatPCenter(const gnsstk::Position& stapos, gnsstk::AntexReader& antexObj)
            : pEphemeris(nullptr)
            , nominalPos(stapos)
            , pAntexReader(&antexObj) {};

        /** Returns a satTypeValueMap object, adding the new data generated
          *  when calling this object.
          *
          * @param time      Epoch corresponding to the data.
          * @param gData     Data object holding the data.
          */
        virtual gnsstk::SatTypePtrMap& Process(const gnsstk::CommonTime& time, gnsstk::SatTypePtrMap& gData);

        /** Returns a gnnsRinex object, adding the new data generated
          *  when calling this object.
          *
          * @param gData    Data object holding the data.
          */
        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData)
        {
            Process(gData.getHeader().epoch, gData.getBody());
            return gData;
        };

        /// Returns name of "PRN_GPS"-like file containing satellite data.
        virtual std::string getFilename(void) const
        {
            return fileData;
        };

        /** Sets name of "PRN_GPS"-like file containing satellite data.
          * @param name      Name of satellite data file.
          */
        virtual ComputeSatPCenter& setFilename(const std::string& name);

        /// Returns nominal position of receiver station.
        virtual gnsstk::Position getNominalPosition(void) const
        {
            return nominalPos;
        };

        /** Sets  nominal position of receiver station.
          * @param stapos    Nominal position of receiver station.
          */
        virtual ComputeSatPCenter& setNominalPosition(const gnsstk::Position& stapos)
        {
            nominalPos = stapos;
            return (*this);
        };

        /// Returns a pointer to the satellite ephemeris object
        /// currently in use.
        virtual gnsstk::NavLibrary* getEphemeris(void) const
        {
            return pEphemeris;
        };

        /** Sets satellite ephemeris object to be used.
          *
          * @param ephem     Satellite ephemeris object.
          */
        virtual ComputeSatPCenter& setEphemeris(gnsstk::NavLibrary& ephem)
        {
            pEphemeris = &ephem;
            return (*this);
        };

        /// Returns a pointer to the AntexReader object currently in use.
        virtual gnsstk::AntexReader* getAntexReader(void) const
        {
            return pAntexReader;
        };

        /** Sets AntexReader object to be used.
          *
          * @param antexObj  AntexReader object containing satellite
          *                  antenna data.
          */
        virtual ComputeSatPCenter& setAntexReader(gnsstk::AntexReader& antexObj)
        {
            pAntexReader = &antexObj;
            return (*this);
        };

        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;
        /** Compute the value of satellite antenna phase correction, in meters
          * @param satid     Satellite ID
          * @param time      Epoch of interest
          * @param satpos    Satellite position, as a Triple
          * @param sunpos    Sun position, as a Triple
          *
          * @return Satellite antenna phase correction, in meters.
          * @param frequency Satellite antenna frequency type
          */
        double getSatPCenter(const gnsstk::SatID& satid,
                             const gnsstk::CommonTime& time,
                             const gnsstk::Triple& satpos,
                             const gnsstk::Triple& sunPosition,
                             const gnsstk::Antenna::frequencyType frequency);

        bool computeSatPos(const gnsstk::SatID& satid,
                           const gnsstk::CommonTime& time,
                           const gnsstk::typeValueMap& data,
                           gnsstk::Triple& svPos);

        void applyBandCorrections(const gnsstk::SatID& satid,
                                  const gnsstk::CommonTime& time,
                                  const gnsstk::Triple& svPos,
                                  const gnsstk::Triple& sunPos,
                                  gnsstk::typeValueMap& data);

        /** Compute satellite antenna phase correction using the absolute
          * ANTEX model.
          * @param satid     Satellite ID
          * @param time      Epoch of interest
          * @param elev      Elevation from nadir angle (degrees)
          * @param ri        Unitary ECEF vector (satellite body X-axis)
          * @param rj        Unitary ECEF vector (satellite body Y-axis)
          * @param rk        Unitary ECEF vector (satellite body Z-axis)
          * @param rrho      Unitary vector from satellite to receiver
          *
          * @return Satellite antenna phase correction, in meters.
          */
        double computeAbsoluteModel(const gnsstk::SatID& satid,
                                    const gnsstk::CommonTime& time,
                                    double elev,
                                    const gnsstk::Triple& ri,
                                    const gnsstk::Triple& rj,
                                    const gnsstk::Triple& rk,
                                    const gnsstk::Triple& rrho,
                                    const gnsstk::Antenna::frequencyType frequency);
        /// Destructor
        virtual ~ComputeSatPCenter() {};

      private:
        ObservationResolver resolver_;

        /// Satellite ephemeris to be used
        gnsstk::NavLibrary* pEphemeris;

        /// Receiver position
        gnsstk::Position nominalPos;

        /// Object to read satellite data file (PRN_GPS)
        gnsstk::SatDataReader satData;

        /// Name of "PRN_GPS"-like file containing satellite data.
        std::string fileData;

        /// Pointer to object containing satellite antenna data, if available.
        gnsstk::AntexReader* pAntexReader;

    }; // End of class 'ComputeSatPCenter'

    //@}

} // namespace pod
