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

/** @file EOPStore.cpp
    class gnsstk::EOPStore encapsulates input, storage and retreval of
    Earth Orientation Parameters (EOPs - cf. class EarthOrientation).
*/

//------------------------------------------------------------------------------------
#include "EOPStore.hpp"
//#include "logstream.hpp"
#include <sstream>

//------------------------------------------------------------------------------------
using namespace std;

namespace gnsstk
{
      // Add to the store directly
   void EOPStore::addEOP(int mjd, EarthOrientation& eop)
   {
      mapMJD_EOP[mjd] = eop;

      if (begMJD == -1 || endMJD == -1)
      {
         begMJD = endMJD = mjd;
      }
      else if (mjd < begMJD)
      {
         begMJD = mjd;
      }
      else if (mjd > endMJD)
      {
         endMJD = mjd;
      }
   }

   //---------------------------------------------------------------------------------
      /* Add to the store by computing using an EOPPrediction file.
         @param MJD integer MJD(UTC) at which to add EOPs
         @return non-0 if MJD is outside range */
   int EOPStore::addEOP(int mjd, EOPPrediction& eopp)
   {
      EarthOrientation eo;
      try
      {
         eo = eopp.computeEOP(mjd);
      }
      catch (Exception& e)
      {
         GNSSTK_RETHROW(e);
      }

      addEOP(mjd, eo);

      return 0;
   }

      /* Add EOPs to the store via an inpu file: either an EOPP file
         or a flat file produced by USNO (see http:  maia.usno.navy.mil/
         and get either file 'finals.data' or finals2000A.data').
         @param filename Name of file to read, including path.
         @return true on success, false on failure. */
   bool EOPStore::addFile(const string& filename)
   {
      if (addEOPPFile(filename))
      {
         return true;
      }
      if (addIERSFile(filename))
      {
         return true;
      }
      if (addERPFile(filename))
      {
         return true;
      }
      cerr << "EOPStore::addFile error: could not load file " << filename << endl;
      return false;
   }

   //---------------------------------------------------------------------------------
      /* Add EOPs to the store via an EOPP file: read the EOPPrediction from the
         file and then compute EOPs for all days within the valid range.
         @param filename Name of file to read, including path.
         @return true on success, false on failure. */
   bool EOPStore::addEOPPFile(const string& filename)
   {
         // read the file into an EOPPrediction
      EOPPrediction eopp;
      try
      {
         eopp.loadFile(filename);
      }
      catch (Exception& e)
      {
         cerr << "EOPStore::addEOPPFile error: " << e.getText() << endl;
         return false;
      }

         // pull out the beginning of the valid time range
      int mjd;
      mjd = eopp.getValidTime();
         // add all 7 days
      for (int i = 0; i < 7; i++)
      {
         EarthOrientation eo;
         eo = eopp.computeEOP(mjd);
         addEOP(mjd, eo);
         mjd++;
      }
      return true;
   }

   //---------------------------------------------------------------------------------
      // see http://maia.usno.navy.mil/readme.finals
   bool EOPStore::addIERSFile(const string& filename)
   {
      int mjd;
      string line;

      ifstream inpf(filename.c_str());
      if (!inpf)
      {
         cerr << "EOPStore::addIERSFile error: could not open file " << filename << endl;
         return false;
      }

      bool loaded = false;
      while (!inpf.eof() && inpf.good())
      {
         getline(inpf, line);
         StringUtils::stripTrailing(line, '\r');
         if (inpf.eof())
         {
            break;
         }
            // line length is actually 187
         if (inpf.bad() || line.size() < 70)
         {
            continue;
         }
         EarthOrientation eo;
         mjd = StringUtils::asInt(line.substr(7, 5));
            // Bulletin A
         eo.xp      = StringUtils::asDouble(line.substr(18, 9));  // arcseconds
         eo.yp      = StringUtils::asDouble(line.substr(37, 9));  // arcseconds
         eo.UT1mUTC = StringUtils::asDouble(line.substr(58, 10)); // seconds

         addEOP(mjd, eo);
         loaded = true;
      }
      inpf.close();

      if (!loaded)
      {
         cerr << "EOPStore::addIERSFile error: no data loaded from " << filename << endl;
      }
      return loaded;
   }

   //---------------------------------------------------------------------------------
      // IGS ERP format, e.g. CODE MGEX solutions
   bool EOPStore::addERPFile(const string& filename)
   {
      string line;

      ifstream inpf(filename.c_str());
      if (!inpf)
      {
         cerr << "EOPStore::addERPFile error: could not open file " << filename << endl;
         return false;
      }

      bool loaded = false;
      bool headerDone = false;
      while (!inpf.eof() && inpf.good())
      {
         getline(inpf, line);
         StringUtils::stripTrailing(line, '\r');
         if (inpf.eof())
         {
            break;
         }
         // Data lines start with a valid MJD (numeric); skip header lines
         string trimmed = StringUtils::stripLeading(line);
         if (trimmed.empty() || !isdigit(trimmed[0]))
         {
            continue;
         }

         // Parse ERP data line
         // Columns: MJD  X-P(1e-6")  Y-P(1e-6")  UT1UTC(1e-7s) ...
         istringstream iss(line);
         double mjdVal, xp, yp, ut1utc;
         if (!(iss >> mjdVal >> xp >> yp >> ut1utc))
         {
            continue;
         }

         // only use integer MJD entries (skip fractional like .50)
         double fracPart = mjdVal - static_cast<int>(mjdVal);
         if (fracPart > 1.0e-9)
         {
            continue;
         }

         const int mjd = static_cast<int>(mjdVal);
         EarthOrientation eo;
         eo.xp      = xp * 1.0e-6;      // convert from 1e-6 arcsec to arcsec
         eo.yp      = yp * 1.0e-6;      // convert from 1e-6 arcsec to arcsec
         eo.UT1mUTC = ut1utc * 1.0e-7;  // convert from 1e-7 sec to sec

         addEOP(mjd, eo);
         loaded = true;
      }
      inpf.close();

      if (!loaded)
      {
         cerr << "EOPStore::addERPFile error: no data loaded from " << filename << endl;
      }
      return loaded;
   }

   //---------------------------------------------------------------------------------
      /* Edit the store by deleting all entries before(after) the given min(max)
         MJDs. If mjdmin is later than mjdmax, the two times are switched.
          @param mjdmin integer MJD desired earliest store time.
          @param mjdmax integer MJD desired latest store time. */
   void EOPStore::edit(int mjdmin, int mjdmax)
   {
      if (mjdmin > mjdmax)
      {
         int m  = mjdmin;
         mjdmin = mjdmax;
         mjdmax = m;
      }

      if (mjdmin > endMJD)
      {
         return;
      }
      if (mjdmax < begMJD)
      {
         return;
      }

      map<int, EarthOrientation>::iterator it;
      it = mapMJD_EOP.lower_bound(mjdmin);
      if (it != mapMJD_EOP.begin())
      {
         mapMJD_EOP.erase(mapMJD_EOP.begin(), it);
      }

      it = mapMJD_EOP.upper_bound(mjdmax);
      if (it != mapMJD_EOP.end())
      {
         mapMJD_EOP.erase(it, mapMJD_EOP.end());
      }

      it = mapMJD_EOP.begin();
      if (it == mapMJD_EOP.end())
      {
         begMJD = -1;
      }
      else
      {
         begMJD = it->first;
      }

      it = mapMJD_EOP.end();
      if (--it == mapMJD_EOP.end())
      {
         endMJD = -1;
      }
      else
      {
         endMJD = it->first;
      }
   }

   //---------------------------------------------------------------------------------
      /* Dump the store to cout.
         @param detail determines how much detail to include in the output
           0 start and stop times (MJD), and number of EOPs.
           1 list of all times and EOPs. */
   void EOPStore::dump(short detail, ostream& os) const
   {
      os << "EOPStore dump (" << mapMJD_EOP.size() << " entries):\n";
      os << " Time limits: [MJD " << begMJD << " - " << endMJD << "]";

      int yy, mm, dd;
      convertJDtoCalendar(static_cast<long>(begMJD + MJD_TO_JD), yy, mm, dd);
      os << " = [m/d/y " << mm << "/" << dd << "/" << yy;
      convertJDtoCalendar(static_cast<long>(endMJD + MJD_TO_JD), yy, mm, dd);
      os << " - " << mm << "/" << dd << "/" << yy << "]" << endl;

      if (detail > 0)
      {
         os << "   MJD      xp         yp        UT1-UTC  IERS\n";
         int lastmjd = -1;
         map<int, EarthOrientation>::const_iterator it;
         for (it = mapMJD_EOP.begin(); it != mapMJD_EOP.end(); it++)
         {
            if (lastmjd != -1 && it->first - lastmjd > 1)
            {
               os << " ....." << endl;
            }
            os << " " << it->first << " " << it->second << "     ("
               << setfill('0') << setw(3)
               << EOPPrediction::getSerialNumber(it->first) << setfill(' ')
               << ")" << endl;
            lastmjd = it->first;
         }
      }
   }

   //---------------------------------------------------------------------------------
      /* Get the EOP at the given epoch. This involves interpolation and
         corrections as prescribed by the appropriate IERS convention, using code
         in class EarthOrientation. This routine pulls data from the map for 4
         entries surrounding the input time; this array of data is passed to class
         EarthOrientation to perform the interpolation and corrections.
         @param mjd MJD(UTC) time of interest
         @param conv IERSConvention to be used.
         @throw InvalidRequest if the integer MJD falls outside the store,
           or if the store contains fewer than 4 entries
         @return EarthOrientation EOPs at mjd. */
   EarthOrientation EOPStore::getEOP(double mjd,
                                     const IERSConvention& conv)
   {
      if (mapMJD_EOP.size() < 4)
      {
         InvalidRequest ir("Store is too small for interpolation");
         GNSSTK_THROW(ir);
      }

         /* Stored data uses UTC times
            if(t.getTimeSystem() == TimeSystem::Unknown) {
              InvalidRequest ir("Time system is unknown");
              GNSSTK_THROW(ir);
        } */

         // get MJD(UTC)
      double mjdUTC(mjd);

         // find 4 points surrounding the time of interest ----------------
      map<int, EarthOrientation>::iterator lowit, hiit, it;
      it = lowit = mapMJD_EOP.find(int(mjdUTC));
      (hiit = it)++;
      if (lowit == mapMJD_EOP.end() || hiit == mapMJD_EOP.end())
      {
         InvalidRequest ir("Requested time lies outside the store");
         GNSSTK_THROW(ir);
      }
      if (mapMJD_EOP.size() < 4)
      {
         InvalidRequest ir("Store contains less than 4 entries");
         GNSSTK_THROW(ir);
      }

         // low and hi must span 4 entries and bracket t
      (it = lowit)--;
      if (it == mapMJD_EOP.end())
      {
         hiit++;
         hiit++; // L t . . H
      }
      else
      {
         lowit = it;
         (it = hiit)++;
         if (it == mapMJD_EOP.end())
         {
            lowit--; // L . . t H
         }
         else
         {
            hiit = it; // L . t . H
         }
      }

         /* fill arrays for Lagrange interpolation -----------------------
            LOG(INFO) << " LAGINT at " << fixed << setprecision(9) << mjdUTC <<
            "(UTC)"; */
      vector<double> vtime, vX, vY, vdT;
      for (it = lowit; it != mapMJD_EOP.end(); ++it)
      {
         vtime.push_back(double(it->first));
         vX.push_back(it->second.xp);
         vY.push_back(it->second.yp);
            /* LOG(INFO) << " xy " << fixed << setprecision(10) <<
               double(it->first)
                 << " " << it->second.xp << " " << it->second.yp; */
         vdT.push_back(it->second.UT1mUTC);
         if (it == hiit)
         {
            break;
         }
      }

         // let EarthOrientation do the interpolation and correction -----
      EarthOrientation eo;
      EphTime ttag;
      ttag.setMJD(mjdUTC);
      ttag.setTimeSystem(TimeSystem::UTC);
      eo.interpolateEOP(ttag, vtime, vX, vY, vdT, conv);

      return eo;
   }

} // end namespace gnsstk
