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
 * @file ComputeSatPCenter.cpp
 * This class computes the satellite antenna phase correction, in meters.
 */

#include "ComputeSatPCenter.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace pod
{
    using namespace std;
    using namespace gnsstk;

    namespace
    {
        bool resolveFrequency(const SatelliteSystem system,
                              const ObsSlot slot,
                              Antenna::frequencyType& frequency) noexcept
        {
            const bool firstBand = slot == ObsSlot::FirstBandCode || slot == ObsSlot::FirstBandPhase;

            const bool secondBand = slot == ObsSlot::SecondBandCode || slot == ObsSlot::SecondBandPhase;

            if (!firstBand && !secondBand)
            {
                return false;
            }

            switch (system)
            {
            case SatelliteSystem::GPS:
                frequency = firstBand ? Antenna::G01 : Antenna::G02;
                return true;

            case SatelliteSystem::Glonass:
                frequency = firstBand ? Antenna::R01 : Antenna::R02;
                return true;

            case SatelliteSystem::Galileo:
                frequency = firstBand ? Antenna::E01 : Antenna::E05;
                return true;

            default:
                return false;
            }
        }

        struct BandObservations
        {
            ObsSlot codeSlot;
            ObsSlot phaseSlot;
        };

        constexpr std::array<BandObservations, 2> bands = {
            {{ObsSlot::FirstBandCode, ObsSlot::FirstBandPhase}, {ObsSlot::SecondBandCode, ObsSlot::SecondBandPhase}}};

    } // anonymous namespace

    // Returns a string identifying this object.
    std::string ComputeSatPCenter::getClassName() const
    {
        return "ComputeSatPCenter";
    }

    /* Sets name of "PRN_GPS"-like file containing satellite data.
       * @param name      Name of satellite data file.
       */
    ComputeSatPCenter& ComputeSatPCenter::setFilename(const string& name)
    {

        fileData = name;
        satData.open(fileData);

        return (*this);

    } // End of method 'ComputeSatPCenter::setFilename()'
    /* Returns a satTypeValueMap object, adding the new data generated when
       * calling this object.
       *
       * @param time      Epoch corresponding to the data.
       * @param gData     Data object holding the data.
       */

    SatTypePtrMap& ComputeSatPCenter::Process(const CommonTime& time, SatTypePtrMap& gData)
    {
        try
        {
            SunPosition sunPosition;
            const Triple sunPos = sunPosition.getPosition(time);

            SatIDSet satRejectedSet;

            for (auto it = gData.begin(); it != gData.end(); ++it)
            {
                const SatID& satid = it->first;
                auto& data = *it->second;

                Triple svPos(0.0, 0.0, 0.0);

                if (!computeSatPos(satid, time, data, svPos))
                {
                    satRejectedSet.insert(satid);
                    continue;
                }

                applyBandCorrections(satid, time, svPos, sunPos, data);
            }

            gData.removeSatID(satRejectedSet);

            rejectedSatsTable[time] = satRejectedSet;

            return gData;
        }
        catch (Exception& u)
        {
            ProcessingException e(getClassName() + ":" + u.what());
            GNSSTK_THROW(e);
        }
    }

    bool ComputeSatPCenter::computeSatPos(const SatID& satid,
                                          const CommonTime& time,
                                          const typeValueMap& data,
                                          Triple& svPos)
    {
        const auto satXIt = data.find(TypeID::satX);
        const auto satYIt = data.find(TypeID::satY);
        const auto satZIt = data.find(TypeID::satZ);

        if (satXIt != data.end() && satYIt != data.end() && satZIt != data.end())
        {
            svPos[0] = satXIt->second;
            svPos[1] = satYIt->second;
            svPos[2] = satZIt->second;

            return true;
        }

        if (pEphemeris == nullptr)
        {
            return false;
        }

        Xvt svPosVel;

        if (!pEphemeris->getXvt(satid, time, svPosVel))
        {
            return false;
        }

        svPos[0] = svPosVel.x.theArray[0];
        svPos[1] = svPosVel.x.theArray[1];
        svPos[2] = svPosVel.x.theArray[2];

        return true;
    }

    void ComputeSatPCenter::applyBandCorrections(const SatID& satid,
                                                 const CommonTime& time,
                                                 const Triple& svPos,
                                                 const Triple& sunPos,
                                                 typeValueMap& data)
    {
        for (const auto& band : bands)
        {
            const TypeID codeType = resolver_.resolve(band.codeSlot, satid.system);
            const TypeID phaseType = resolver_.resolve(band.phaseSlot, satid.system);

            auto codeIt = data.end();
            auto phaseIt = data.end();

            if (codeType != TypeID::Unknown)
            {
                codeIt = data.find(codeType);
            }

            if (phaseType != TypeID::Unknown)
            {
                phaseIt = data.find(phaseType);
            }

            const bool hasCode = codeIt != data.end();
            const bool hasPhase = phaseIt != data.end();

            if (!hasCode && !hasPhase)
            {
                continue;
            }

            Antenna::frequencyType frequency = Antenna::G01;

            if (!resolveFrequency(satid.system, band.codeSlot, frequency))
            {
                continue;
            }

            const double correction = getSatPCenter(satid, time, svPos, sunPos, frequency);

            if (hasCode)
            {
                codeIt->second += correction;
            }

            if (hasPhase)
            {
                phaseIt->second += correction;
            }
        }
    }

    /* Compute the value of satellite antenna phase correction, in meters.
       * @param satid     Satellite ID
       * @param time      Epoch of interest
       * @param satpos    Satellite position, as a Triple
       * @param sunpos    Sun position, as a Triple
       *
       * @return Satellite antenna phase correction, in meters.
       */

    double ComputeSatPCenter::getSatPCenter(const SatID& satid,
                                            const CommonTime& time,
                                            const Triple& satpos,
                                            const Triple& sunPosition,
                                            const Antenna::frequencyType frequency)

    {

        // Unitary vector from satellite to Earth mass center (ECEF)
        Triple rk(((-1.0) * (satpos.unitVector())));

        // Unitary vector from Earth mass center to Sun (ECEF)
        Triple ri(sunPosition.unitVector());

        // rj = rk x ri: Rotation axis of solar panels (ECEF)
        Triple rj(rk.cross(ri));

        // Redefine ri: ri = rj x rk (ECEF)
        ri = rj.cross(rk);

        // Let's convert ri to an unitary vector. (ECEF)
        ri = ri.unitVector();

        // Get vector from Earth mass center to receiver
        Triple rxPos(nominalPos.X(), nominalPos.Y(), nominalPos.Z());

        // Compute unitary vector vector from satellite to RECEIVER
        Triple rrho((rxPos - satpos).unitVector());

        // When not using Antex information, if satellite belongs to block
        // "IIR" its correction is 0.0, else it will depend on satellite model.

        // This variable that will hold the correction, 0.0 by default
        double svPCcorr(0.0);

        // Check is Antex antenna information is available or not, and if
        // available, whether satellite phase center information is absolute
        // or relative
        bool absoluteModel(false);
        if (pAntexReader != nullptr)
        {
            absoluteModel = pAntexReader->isAbsolute();
        }

        if (absoluteModel)
        {
            double nadir = std::acos(rrho.dot(rk)) * RAD_TO_DEG;
            nadir = (nadir > 14) ? 14.0 : nadir;
            double elev(90.0 - nadir);

            svPCcorr = computeAbsoluteModel(satid, time, elev, ri, rj, rk, rrho, frequency);
        }
        else
        {
            // If no Antex information is given, or if phase center information
            // uses a relative model, then use a simpler, older approach

            // Please note that in this case all GLONASS satellite are
            // considered as having phase center at (0.0, 0.0, 0.0). The former
            // is not true for 'GLONASS-M' satellites (-0.545, 0.0, 0.0 ), but
            // currently there is no simple way to take this into account.

            // For satellites II and IIA:
            if ((satData.getBlock(satid, time) == "II") || (satData.getBlock(satid, time) == "IIA"))
            {

                // First, build satellite antenna vector for models II/IIA
                Triple svAntenna(0.279 * ri + 1.023 * rk);

                // Projection of "svAntenna" vector to line of sight vector rrho
                svPCcorr = (rrho.dot(svAntenna));
            }
            else
            {
                // For satellites belonging to block "I"
                if ((satData.getBlock(satid, time) == "I"))
                {

                    // First, build satellite antenna vector for model I
                    Triple svAntenna(0.210 * ri + 0.854 * rk);

                    // Projection of "svAntenna" to line of sight vector (rrho)
                    svPCcorr = (rrho.dot(svAntenna));
                }

            } // End of 'if( (satData.getBlock( satid, time ) == "II") ||...'

        } // End of 'if( absoluteModel )...'

        return svPCcorr;

    } // End of method 'ComputeSatPCenter::getSatPCenter()'

    double ComputeSatPCenter::computeAbsoluteModel(const SatID& satid,
                                                   const CommonTime& time,
                                                   const double elev,
                                                   const Triple& ri,
                                                   const Triple& rj,
                                                   const Triple& rk,
                                                   const Triple& rrho,
                                                   const Antenna::frequencyType frequency)
    {
        char prefix = '\0';

        switch (satid.system)
        {
        case SatelliteSystem::GPS:
            prefix = 'G';
            break;

        case SatelliteSystem::Glonass:
            prefix = 'R';
            break;

        case SatelliteSystem::Galileo:
            prefix = 'E';
            break;

        default:
            std::cout << "Warning: For " << satid << " phase center correction cannot be computed: "
                      << "unsupported satellite system." << std::endl;

            return 0.0;
        }

        std::ostringstream satelliteName;

        satelliteName << prefix << std::setw(2) << std::setfill('0') << satid.id;

        const Antenna antenna(pAntexReader->getAntenna(satelliteName.str(), time));

        // Both PCO and PCV are selected for the requested frequency.
        Triple satAnt = antenna.getAntennaEccentricity(frequency);

        const Triple variation = antenna.getAntennaPCVariation(frequency, elev);

        // Keep the sign convention used by the original implementation.
        satAnt = satAnt - variation;

        // Transform the satellite-fixed antenna vector to ECEF.
        const Triple svAntenna(satAnt[2] * ri + satAnt[1] * rj + satAnt[0] * rk);

        // Project onto the satellite-to-receiver line of sight.
        return rrho.dot(svAntenna);
    }

} // namespace pod
