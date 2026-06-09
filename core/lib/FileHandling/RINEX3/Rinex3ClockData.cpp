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

/// @file Rinex3ClockData.cpp
/// Encapsulate Rinex3Clock file data, including I/O

#include "Rinex3ClockStream.hpp"
#include "Rinex3ClockHeader.hpp"
#include "Rinex3ClockData.hpp"
#include "RinexSatID.hpp"
#include "StringUtils.hpp"
#include "TimeString.hpp"
#include "CivilTime.hpp"

using namespace gnsstk::StringUtils;
using namespace std;

namespace gnsstk
{
   Rinex3ClockData::Rinex3ClockData()
         : sat(-1,SatelliteSystem::GPS), time(CommonTime::BEGINNING_OF_TIME),
           bias(0), sig_bias(0), drift(0), sig_drift(0), accel(0), sig_accel(0)
   {
   }


   void Rinex3ClockData::reallyPutRecord(FFStream& ffs) const
   {
         // cast the stream to be an Rinex3ClockStream
      Rinex3ClockStream& strm = dynamic_cast<Rinex3ClockStream&>(ffs);

      strm << datatype << ' ';

      if (datatype == string("AR"))
      {
         strm << setw(4) << right << site;
      }
      else if (datatype == string("AS"))
      {
         strm << sat.systemChar() << setw(2) << right << setfill('0') << sat.id
              << setfill(' ') << ' ';
      }
      else
      {
         FFStreamError e("Unknown data type: " + datatype);
         GNSSTK_THROW(e);
      }
      strm << ' ' << printTime(time,"%4Y %02m %02d %02H %02M %9.6f");

         // must count the data to output
      int n(2);
      if (drift != 0.0)
         n=3;
      if (sig_drift != 0.0)
         n=4;
      if (accel != 0.0)
         n=5;
      if (sig_accel != 0.0)
         n=6;
      strm << setw(3) << n << "   " << bias << ' ' << sig_bias << endl;

      strm.lineNumber++;

         // continuation line

      if (n > 2)
      {
         strm << drift << ' ';
         if (n > 3)
         {
            strm << sig_drift << ' ';
         }
         if (n > 4)
         {
            strm << accel << ' ';
         }
         if (n > 5)
         {
            strm << sig_accel << ' ';
         }
         strm << endl;
         strm.lineNumber++;
      }

   }  // end reallyPutRecord()



void Rinex3ClockData::reallyGetRecord(FFStream& ffs)
   {
       Rinex3ClockStream& strm = dynamic_cast<Rinex3ClockStream&>(ffs);

       clear();

       string line;
       strm.formattedGetLine(line, true);
       stripTrailing(line);

       if (line.length() < 10)
       {
           FFStreamError e("Short line : " + line);
           GNSSTK_THROW(e);
       }

       datatype = strip(line.substr(0, 2));

       // ============================================================
       //  AS recorfd type parsing
       // ============================================================
       if (datatype == "AS")
       {
           try
           {
               std::istringstream iss(line);

               string recType;
               string satStr;
               int year, month, day, hour, minute;
               double sec;
               int n;

               double biasVal = 0.0;
               double sigBiasVal = 0.0;

               iss >> recType >> satStr >> year >> month >> day >> hour >> minute >> sec >> n
                   >> biasVal;

               if (n > 1)
                   iss >> sigBiasVal;

               datatype = recType;

               try
               {
                   sat.fromString(satStr);
               }
               catch (Exception& exc)
               {
                   FFStreamError e(exc);
                   e.addText("Invalid sat : /" + satStr + "/");
                   GNSSTK_THROW(e);
               }

               time = CivilTime(year, month, day, hour, minute, sec, TimeSystem::Any);

               bias = asString(biasVal);
               sig_bias = asString(sigBiasVal);

               // --- continuation line ---
               if (n > 2)
               {
                   strm.formattedGetLine(line, true);
                   stripTrailing(line);

                   std::istringstream iss2(line);

                   double driftVal = 0.0, sigDriftVal = 0.0;
                   double accelVal = 0.0, sigAccelVal = 0.0;

                   iss2 >> driftVal;

                   if (n > 3)
                       iss2 >> sigDriftVal;
                   if (n > 4)
                       iss2 >> accelVal;
                   if (n > 5)
                       iss2 >> sigAccelVal;

                   drift = asString(driftVal);
                   sig_drift = asString(sigDriftVal);
                   accel = asString(accelVal);
                   sig_accel = asString(sigAccelVal);
               }

               site.clear();
               return;
           }
           catch (std::exception& e)
           {
               FFStreamError err(string("Error parsing AS record: ") + e.what());
               GNSSTK_THROW(err);
           }
       }

       // ============================================================
       //  AR recorfd type parsing
       // ============================================================
       site = line.substr(3, 4);

       time = CivilTime(asInt(line.substr(8, 4)),
                        asInt(line.substr(12, 3)),
                        asInt(line.substr(15, 3)),
                        asInt(line.substr(18, 3)),
                        asInt(line.substr(21, 3)),
                        asDouble(line.substr(24, 10)),
                        TimeSystem::Any);

       int n(asInt(line.substr(34, 3)));

       bias = line.substr(40, 19);

       if (n > 1 && line.length() >= 59)
           sig_bias = line.substr(60, 19);

       if (n > 2)
       {
           strm.formattedGetLine(line, true);
           stripTrailing(line);

           drift = line.substr(0, 19);

           if (n > 3)
               sig_drift = line.substr(20, 19);
           if (n > 4)
               accel = line.substr(40, 19);
           if (n > 5)
               sig_accel = line.substr(60, 19);
       }
   }

   void Rinex3ClockData::dump(ostream& s) const noexcept
   {
         // dump record type, sat id / site, current epoch, and data
      s << " " << datatype;
      if (datatype == string("AR"))
      {
         s << " " << site;
      }
      else
      {
         s << " " << sat.toString();
      }
      s << " " << printTime(time,"%Y/%02m/%02d %2H:%02M:%06.3f = %F/%10.3g %P")
        << " " << bias << " " << sig_bias << " " << drift << " " << sig_drift
        << " " << accel << " " << sig_accel << endl;
   }  // end dump()

} // namespace

//------------------------------------------------------------------------------------
