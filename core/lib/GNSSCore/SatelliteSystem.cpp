/** @warning This code is automatically generated.
 *
 *  DO NOT EDIT THIS CODE BY HAND.
 *
 *  Refer to the documenation in the toolkit_docs gitlab project.
 */

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

#include "SatelliteSystem.hpp"
#include "StringUtils.hpp"

namespace gnsstk
{
   namespace StringUtils
   {
      std::string asString(SatelliteSystem e) noexcept
      {
         switch (e)
         {
            case SatelliteSystem::Unknown:     return "Unknown";
            case SatelliteSystem::GPS:         return "GPS";
            case SatelliteSystem::Galileo:     return "Galileo";
            case SatelliteSystem::Glonass:     return "GLONASS";
            case SatelliteSystem::Geosync:     return "Geostationary";
            case SatelliteSystem::LEO:         return "LEO";
            case SatelliteSystem::Transit:     return "Transit";
            case SatelliteSystem::BeiDou:      return "BeiDou";
            case SatelliteSystem::QZSS:        return "QZSS";
            case SatelliteSystem::IRNSS:       return "IRNSS";
            case SatelliteSystem::Mixed:       return "Mixed";
            case SatelliteSystem::UserDefined: return "UserDefined";
               default:                           return "???";
               } // switch (e)
            } // asString(SatelliteSystem)


            std::string asCode(SatelliteSystem e) noexcept
            {
               switch (e)
               {
                  case SatelliteSystem::Unknown:     return "UNK";
                  case SatelliteSystem::GPS:         return "GPS";
                  case SatelliteSystem::Galileo:     return "GAL";
                  case SatelliteSystem::Glonass:     return "GLO";
                  case SatelliteSystem::Geosync:     return "GEO";
                  case SatelliteSystem::LEO:         return "LEO";
                  case SatelliteSystem::Transit:     return "TRN";
                  case SatelliteSystem::BeiDou:      return "BDS";
                  case SatelliteSystem::QZSS:        return "QZS";
                  case SatelliteSystem::IRNSS:       return "IRN";
                  case SatelliteSystem::Mixed:       return "MIX";
                  case SatelliteSystem::UserDefined: return "USR";
                  default:                           return "???";
               } // switch (e)
            } // asCode(SatelliteSystem)


            SatelliteSystem asSatelliteSystem(const std::string& s) noexcept
      {
         std::string upper = upperCase(s);
         if (upper == "UNKNOWN")
            return SatelliteSystem::Unknown;
         if (upper == "GPS")
            return SatelliteSystem::GPS;
         if (upper == "GALILEO")
            return SatelliteSystem::Galileo;
         if (upper == "GLONASS")
            return SatelliteSystem::Glonass;
         if (upper == "GEOSTATIONARY")
            return SatelliteSystem::Geosync;
         if (upper == "LEO")
            return SatelliteSystem::LEO;
         if (upper == "TRANSIT")
            return SatelliteSystem::Transit;
         if (upper == "BEIDOU")
            return SatelliteSystem::BeiDou;
         if (upper == "QZSS")
            return SatelliteSystem::QZSS;
         if (upper == "IRNSS")
            return SatelliteSystem::IRNSS;
         if (upper == "MIXED")
            return SatelliteSystem::Mixed;
         if (upper == "USERDEFINED")
            return SatelliteSystem::UserDefined;
         return SatelliteSystem::Unknown;
      } // asSatelliteSystem(string)


      SatelliteSystem asSatelliteSystemCode(const std::string& s) noexcept
      {
         std::string upper = upperCase(s);
         if (upper == "GPS")
            return SatelliteSystem::GPS;
         if (upper == "GAL")
            return SatelliteSystem::Galileo;
         if (upper == "GLO" || upper == "GLN")
            return SatelliteSystem::Glonass;
         if (upper == "GEO")
            return SatelliteSystem::Geosync;
         if (upper == "LEO")
            return SatelliteSystem::LEO;
         if (upper == "TRN")
            return SatelliteSystem::Transit;
         if (upper == "BDS")
            return SatelliteSystem::BeiDou;
         if (upper == "QZS")
            return SatelliteSystem::QZSS;
         if (upper == "IRN")
            return SatelliteSystem::IRNSS;
         if (upper == "MIX")
            return SatelliteSystem::Mixed;
         if (upper == "USR")
            return SatelliteSystem::UserDefined;
         return SatelliteSystem::Unknown;
      } // asSatelliteSystemCode(string)
   } // namespace StringUtils
} // namespace gnsstk
