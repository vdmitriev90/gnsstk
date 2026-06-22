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
 * @file RequireObservables.cpp
 * This class filters out satellites with observations grossly out of bounds.
 */

#include "RequireObservables.hpp"

namespace gnsstk
{

    // Returns a string identifying this object.
    std::string RequireObservables::getClassName() const
    {
        return "RequireObservables";
    }

    // Returns a satTypeValueMap object, filtering the target observables.
    //
    // @param gData     Data object holding the data.
    //
    SatTypePtrMap& RequireObservables::Process(SatTypePtrMap& gData)
    {
        try
        {
            SatIDSet rejected; // can be kept if logging is needed

            for (auto it = gData.begin(); it != gData.end();)
            {
                const SatelliteSystem sys = it->first.system;

                auto reqIt = requiredTypes_.find(sys);

                // If no requirements defined, skip this satellite
                if (reqIt == requiredTypes_.end())
                {
                    ++it;
                    continue;
                }

                const TypeIDSet& required = reqIt->second;
                auto& tvMap = *it->second;

                bool reject = false;

                // 1. Check that all required observation types exist
                for (const auto& type : required)
                {
                    if (tvMap.find(type) == tvMap.end())
                    {
                        reject = true;
                        break;
                    }
                }

                if (reject)
                {
                    rejected.insert(it->first); // optional, for logging
                    it = gData.erase(it);       // erase immediately
                    continue;
                }

                // 2. Optionally remove non-required observation types in the same pass
                if (keepOnlyRequiredTypes_)
                {
                    for (auto tvIt = tvMap.begin(); tvIt != tvMap.end();)
                    {
                        if (required.find(tvIt->first) == required.end())
                            tvIt = tvMap.erase(tvIt);
                        else
                            ++tvIt;
                    }
                }

                ++it;
            }

            rejectedSatsTable[currTime_] = std::move(rejected);

            return gData;
        }
        catch (Exception& u)
        {
            ProcessingException e(getClassName() + ":" + u.what());
            GNSSTK_THROW(e);
        }
    }
} // End of namespace gnsstk
