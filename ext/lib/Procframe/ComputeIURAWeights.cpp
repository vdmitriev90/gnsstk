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
 * @file ComputeIURAWeights.cpp
 * This class computes satellites weights based on URA Index and is
 * meant to be used with GNSS data structures.
 */

#include "ComputeIURAWeights.hpp"
#include "GPSLNavEph.hpp"
#include "OrbitDataSP3.hpp"


namespace gnsstk
{
    namespace
    {
        std::optional<double> getURAWeight(const SatID& satId, const CommonTime& time, NavLibrary& navLib)
        {
            const NavMessageID nav_id(satId, NavMessageType::Ephemeris);

            // Get the URA index for this satellite
            NavDataPtr nav_data_ptr = nullptr;

            if (navLib.find(nav_id, time, nav_data_ptr, SVHealth::Any, NavValidityType::Any, NavSearchOrder::Nearest))
            {
                // fixme TODO: remove dynamic cast here
                const GPSLNavEph* eph = dynamic_cast<GPSLNavEph*>(nav_data_ptr.get());

                if (eph != nullptr)
                {
                    const double sigma = gnsstk::ura2nominalAccuracy(eph->uraIndex);
                    return 1.0 / (sigma * sigma);
                }
                const OrbitDataSP3* sp3_eph = dynamic_cast<OrbitDataSP3*>(nav_data_ptr.get());
                if (sp3_eph != nullptr)
                {
                    // An URA of 0.1 m is assumed for all satellites, 
                    // so sigma = 0.1*0.1 = 0.01 m^2
                    return 100.0;
                }
            }

            return std::optional<double>();
        }
    }
    // Returns a string identifying this object.
    std::string ComputeIURAWeights::getClassName() const
    {
        return "ComputeIURAWeights";
    }

    /* Returns a satTypeValueMap object, adding the new data generated
     * when calling this object.
     *
     * @param gData     Data object holding the data.
     */
    SatTypePtrMap& ComputeIURAWeights::Process(const CommonTime& time,
        SatTypePtrMap& gData)
    {
        try
        {
            SatIDSet satRejectedSet;

            // Loop through all the satellites
            for (auto it = gData.begin(); it != gData.end(); ++it)
            {
                const SatID& sat_id = (*it).first;
                std::optional<double> weight = getURAWeight(sat_id, time, navLib_);
                if (weight.has_value())
                    (*it->second)[TypeID::weight] = weight.value();
                else
                    satRejectedSet.insert((*it).first);
            }
            // Remove satellites with missing data
            gData.removeSatID(satRejectedSet);

            return gData;

        }
        catch (Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e(getClassName() + ":"
                + u.what());

            GNSSTK_THROW(e);

        }

    }  // End of method 'ComputeIURAWeights::Process()'

}  // End of namespace gnsstk
