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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007
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
 * @file SolidTides.cpp
 * Computes the effect of solid Earth tides on a given position and epoch.
 * Implementation follows IERS Conventions (2010), section 7.1.1
 * (routine DEHANTTIDEINEL and its subroutines).
 */

#include "SolidTides.hpp"
#include "CivilTime.hpp"
#include "MJD.hpp"
#include "TimeSystem.hpp"


namespace gnsstk
{

   namespace
   {
         // IERS 2010 constants
      const double MASS_RATIO_SUN  = 332946.0482;
      const double MASS_RATIO_MOON = 0.0123000371;
      const double RE              = 6378136.6;

      inline double dot3(const Triple& a, const Triple& b)
      {
         return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
      }

      inline double norm3(const Triple& a)
      {
         return std::sqrt(dot3(a, a));
      }

      inline double mod360(double x)
      {
         x = std::fmod(x, 360.0);
         return (x < 0.0) ? x + 360.0 : x;
      }

         // Convert local (radial, north, east) displacement to ECEF
      inline void addRNE(Triple& out, double dr, double dn, double de,
                         double sinphi, double cosphi,
                         double sinla, double cosla)
      {
         out[0] += dr*cosla*cosphi - de*sinla - dn*sinphi*cosla;
         out[1] += dr*sinla*cosphi + de*cosla - dn*sinphi*sinla;
         out[2] += dr*sinphi + dn*cosphi;
      }

         // Geocentric latitude/longitude helpers of the station
      struct StaGeo
      {
         double rsta, sinphi, cosphi, sinla, cosla;

         explicit StaGeo(const Triple& xsta)
         {
            rsta = norm3(xsta);
            sinphi = xsta[2] / rsta;
            cosphi = std::sqrt(xsta[0]*xsta[0] + xsta[1]*xsta[1]) / rsta;
            sinla = xsta[1] / cosphi / rsta;
            cosla = xsta[0] / cosphi / rsta;
         }
      };

         // Fundamental arguments used in step 2 (degrees)
      void fundArgs(double T, double& s, double& h, double& p,
                    double& zns, double& ps)
      {
         s = 218.31664563 + (481267.88194 + (-0.0014663889
             + 0.00000185139*T)*T)*T;
         const double pr = (1.396971278 + (0.000308889 + (0.000000021
                           + 0.000000007*T)*T)*T)*T;
         s += pr;
         h = 280.46645 + (36000.7697489 + (0.00030322222 + (0.000000020
             + (-0.00000000654)*T)*T)*T)*T;
         p = 83.35324312 + (4069.01363525 + (-0.01032172222
             + (-0.0000124991 + 0.00000005263*T)*T)*T)*T;
         zns = 234.95544499 + (1934.13626197 + (-0.00207561111
               + (-0.00000213944 + 0.00000001650*T)*T)*T)*T;
         ps = 282.93734098 + (1.71945766667 + (0.00045688889
              + (-0.00000001778 + (-0.00000000334)*T)*T)*T)*T;

         s = mod360(s);
         h = mod360(h);
         p = mod360(p);
         zns = mod360(zns);
         ps = mod360(ps);
      }

   } // anonymous namespace


      /* Returns the effect of solid Earth tides (meters) at the given
       * position and epoch, in the Up-East-North (UEN) reference frame.
       */
   Triple SolidTides::getSolidTide(const CommonTime& t, const Position& p) const
   {
      try
      {
            // Sun and Moon positions, ECEF [m]
         SunPosition  sunPosition;
         MoonPosition moonPosition;
         const Triple xsun(sunPosition.getPosition(t));
         const Triple xmon(moonPosition.getPosition(t));

         Position pos(p);
         pos.asECEF();
         const Triple xsta(pos.X(), pos.Y(), pos.Z());

         double T, fhr;
         computeTimeArgs(t, T, fhr);

         const Triple dx(dehantTideInel(xsta, xsun, xmon, T, fhr));

            // ECEF -> UEN at the geodetic position of the station
         const double lat(pos.geodeticLatitude() * DEG2RAD);
         const double lon(pos.longitude() * DEG2RAD);
         const double sl(std::sin(lat)), cl(std::cos(lat));
         const double so(std::sin(lon)), co(std::cos(lon));

         Triple res;
         res[0] =  cl*co*dx[0] + cl*so*dx[1] + sl*dx[2];
         res[1] = -so*dx[0] + co*dx[1];
         res[2] = -sl*co*dx[0] - sl*so*dx[1] + cl*dx[2];

         return res;
      }
      catch (InvalidRequest& ir)
      {
         GNSSTK_RETHROW(ir);
      }

   } // End SolidTides::getSolidTide


   void SolidTides::computeTimeArgs(const CommonTime& t, double& T, double& fhr)
   {
      const CivilTime ct(t);
         // TAI-UTC, seconds
      const double leap = getLeapSeconds(ct.year, ct.month, ct.day);

         // Offset (input time system -> TAI), seconds
      double toTAI(19.0);
      switch (t.getTimeSystem())
      {
         case TimeSystem::TAI: toTAI = 0.0; break;
         case TimeSystem::TT:
         case TimeSystem::TDB: toTAI = -32.184; break;
         case TimeSystem::BDT: toTAI = 33.0; break;
         case TimeSystem::UTC: toTAI = leap; break;
         default: toTAI = 19.0; break;  // GPS-like time scales
      }

         // Split epoch into integer MJD and seconds of day to keep precision
      const long double mjd = MJD(t).mjd;
      const long double day = std::floor(mjd);
      const long double sod = std::round((mjd - day) * 86400.0L * 1.0e6L)
                              / 1.0e6L;

         // UTC seconds of day
      double sodUTC = static_cast<double>(sod) + toTAI - leap;
      if (sodUTC < 0.0)
         sodUTC += 86400.0;
      else if (sodUTC >= 86400.0)
         sodUTC -= 86400.0;
      fhr = sodUTC / 3600.0;

         // TT seconds relative to the start of the MJD day
      const double sodTT = static_cast<double>(sod) + toTAI + 32.184;
      T = (static_cast<double>(day - 51544.5L) + sodTT / 86400.0) / 36525.0;

   } // End SolidTides::computeTimeArgs


   Triple SolidTides::dehantTideInel(const Triple& xsta,
                                     const Triple& xsun,
                                     const Triple& xmon,
                                     double T,
                                     double fhr)
   {
         // Nominal second degree and third degree Love numbers
      const double h20 = 0.6078, l20 = 0.0847, h3 = 0.292, l3 = 0.015;

      const double rsta = norm3(xsta);
      const double rsun = norm3(xsun);
      const double rmon = norm3(xmon);

      const double scsun = dot3(xsta, xsun) / rsta / rsun;
      const double scmon = dot3(xsta, xmon) / rsta / rmon;

         // Latitude dependence of h2 and l2
      const double cosphi = std::sqrt(xsta[0]*xsta[0] + xsta[1]*xsta[1])
                            / rsta;
      const double h2 = h20 - 0.0006*(1.0 - 1.5*cosphi*cosphi);
      const double l2 = l20 + 0.0002*(1.0 - 1.5*cosphi*cosphi);

      const double p2sun = 3.0*(h2/2.0 - l2)*scsun*scsun - h2/2.0;
      const double p2mon = 3.0*(h2/2.0 - l2)*scmon*scmon - h2/2.0;

      const double p3sun = 2.5*(h3 - 3.0*l3)*scsun*scsun*scsun
                           + 1.5*(l3 - h3)*scsun;
      const double p3mon = 2.5*(h3 - 3.0*l3)*scmon*scmon*scmon
                           + 1.5*(l3 - h3)*scmon;

      const double x2sun = 3.0*l2*scsun;
      const double x2mon = 3.0*l2*scmon;
      const double x3sun = 1.5*l3*(5.0*scsun*scsun - 1.0);
      const double x3mon = 1.5*l3*(5.0*scmon*scmon - 1.0);

      const double fac2sun = MASS_RATIO_SUN*RE*std::pow(RE/rsun, 3);
      const double fac2mon = MASS_RATIO_MOON*RE*std::pow(RE/rmon, 3);
      const double fac3sun = fac2sun*(RE/rsun);
      const double fac3mon = fac2mon*(RE/rmon);

         // Step 1: in-phase corrections, degree 2 and 3
      Triple dx;
      for (int i = 0; i < 3; ++i)
      {
         dx[i] = fac2sun*(x2sun*xsun[i]/rsun + p2sun*xsta[i]/rsta)
               + fac2mon*(x2mon*xmon[i]/rmon + p2mon*xsta[i]/rsta)
               + fac3sun*(x3sun*xsun[i]/rsun + p3sun*xsta[i]/rsta)
               + fac3mon*(x3mon*xmon[i]/rmon + p3mon*xsta[i]/rsta);
      }

         // Step 1: out-of-phase and latitude dependence corrections
      dx = dx + st1idiu(xsta, xsun, xmon, fac2sun, fac2mon);
      dx = dx + st1isem(xsta, xsun, xmon, fac2sun, fac2mon);
      dx = dx + st1l1(xsta, xsun, xmon, fac2sun, fac2mon);

         // Step 2: frequency dependent corrections
      dx = dx + step2diu(xsta, fhr, T);
      dx = dx + step2lon(xsta, T);

      return dx;

   } // End SolidTides::dehantTideInel


   Triple SolidTides::st1idiu(const Triple& xsta, const Triple& xsun,
                              const Triple& xmon, double fac2sun,
                              double fac2mon)
   {
      const double dhi = -0.0025, dli = -0.0007;

      const StaGeo g(xsta);
      const double cos2phi = g.cosphi*g.cosphi - g.sinphi*g.sinphi;

      const double rsun2 = dot3(xsun, xsun);
      const double rmon2 = dot3(xmon, xmon);

      const double sSun = xsun[2]*(xsun[0]*g.sinla - xsun[1]*g.cosla)/rsun2;
      const double sMon = xmon[2]*(xmon[0]*g.sinla - xmon[1]*g.cosla)/rmon2;
      const double cSun = xsun[2]*(xsun[0]*g.cosla + xsun[1]*g.sinla)/rsun2;
      const double cMon = xmon[2]*(xmon[0]*g.cosla + xmon[1]*g.sinla)/rmon2;

      const double dr = -3.0*dhi*g.sinphi*g.cosphi
                        *(fac2sun*sSun + fac2mon*sMon);
      const double dn = -3.0*dli*cos2phi*(fac2sun*sSun + fac2mon*sMon);
      const double de = -3.0*dli*g.sinphi*(fac2sun*cSun + fac2mon*cMon);

      Triple res(0.0, 0.0, 0.0);
      addRNE(res, dr, dn, de, g.sinphi, g.cosphi, g.sinla, g.cosla);
      return res;

   } // End SolidTides::st1idiu


   Triple SolidTides::st1isem(const Triple& xsta, const Triple& xsun,
                              const Triple& xmon, double fac2sun,
                              double fac2mon)
   {
      const double dhi = -0.0022, dli = -0.0007;

      const StaGeo g(xsta);
      const double costwola = g.cosla*g.cosla - g.sinla*g.sinla;
      const double sintwola = 2.0*g.cosla*g.sinla;

      const double rsun2 = dot3(xsun, xsun);
      const double rmon2 = dot3(xmon, xmon);

      const double aSun = ((xsun[0]*xsun[0] - xsun[1]*xsun[1])*sintwola
                          - 2.0*xsun[0]*xsun[1]*costwola)/rsun2;
      const double aMon = ((xmon[0]*xmon[0] - xmon[1]*xmon[1])*sintwola
                          - 2.0*xmon[0]*xmon[1]*costwola)/rmon2;
      const double bSun = ((xsun[0]*xsun[0] - xsun[1]*xsun[1])*costwola
                          + 2.0*xsun[0]*xsun[1]*sintwola)/rsun2;
      const double bMon = ((xmon[0]*xmon[0] - xmon[1]*xmon[1])*costwola
                          + 2.0*xmon[0]*xmon[1]*sintwola)/rmon2;

      const double dr = -0.75*dhi*g.cosphi*g.cosphi
                        *(fac2sun*aSun + fac2mon*aMon);
      const double dn = 1.5*dli*g.sinphi*g.cosphi
                        *(fac2sun*aSun + fac2mon*aMon);
      const double de = -1.5*dli*g.cosphi*(fac2sun*bSun + fac2mon*bMon);

      Triple res(0.0, 0.0, 0.0);
      addRNE(res, dr, dn, de, g.sinphi, g.cosphi, g.sinla, g.cosla);
      return res;

   } // End SolidTides::st1isem


   Triple SolidTides::st1l1(const Triple& xsta, const Triple& xsun,
                            const Triple& xmon, double fac2sun,
                            double fac2mon)
   {
      const double l1d = 0.0012, l1sd = 0.0024;

      const StaGeo g(xsta);
      const double cos2phi = g.cosphi*g.cosphi - g.sinphi*g.sinphi;

      const double rsun2 = dot3(xsun, xsun);
      const double rmon2 = dot3(xmon, xmon);

      Triple res(0.0, 0.0, 0.0);

         // Diurnal band
      {
         const double cSun = xsun[2]*(xsun[0]*g.cosla + xsun[1]*g.sinla)/rsun2;
         const double cMon = xmon[2]*(xmon[0]*g.cosla + xmon[1]*g.sinla)/rmon2;
         const double sSun = xsun[2]*(xsun[0]*g.sinla - xsun[1]*g.cosla)/rsun2;
         const double sMon = xmon[2]*(xmon[0]*g.sinla - xmon[1]*g.cosla)/rmon2;

         const double dn = 3.0*(-l1d*g.sinphi*g.sinphi)
                           *(fac2sun*cSun + fac2mon*cMon);
         const double de = 3.0*(l1d*g.sinphi*cos2phi)
                           *(fac2sun*sSun + fac2mon*sMon);

         addRNE(res, 0.0, dn, de, g.sinphi, g.cosphi, g.sinla, g.cosla);
      }

         // Semidiurnal band
      {
         const double costwola = g.cosla*g.cosla - g.sinla*g.sinla;
         const double sintwola = 2.0*g.cosla*g.sinla;

         const double bSun = ((xsun[0]*xsun[0] - xsun[1]*xsun[1])*costwola
                             + 2.0*xsun[0]*xsun[1]*sintwola)/rsun2;
         const double bMon = ((xmon[0]*xmon[0] - xmon[1]*xmon[1])*costwola
                             + 2.0*xmon[0]*xmon[1]*sintwola)/rmon2;
         const double aSun = ((xsun[0]*xsun[0] - xsun[1]*xsun[1])*sintwola
                             - 2.0*xsun[0]*xsun[1]*costwola)/rsun2;
         const double aMon = ((xmon[0]*xmon[0] - xmon[1]*xmon[1])*sintwola
                             - 2.0*xmon[0]*xmon[1]*costwola)/rmon2;

         const double dn = 3.0*(-l1sd/2.0*g.sinphi*g.cosphi)
                           *(fac2sun*bSun + fac2mon*bMon);
         const double de = 3.0*(-l1sd/2.0*g.sinphi*g.sinphi*g.cosphi)
                           *(fac2sun*aSun + fac2mon*aMon);

         addRNE(res, 0.0, dn, de, g.sinphi, g.cosphi, g.sinla, g.cosla);
      }

      return res;

   } // End SolidTides::st1l1


   Triple SolidTides::step2diu(const Triple& xsta, double fhr, double T)
   {
         // Table 7.3a, IERS Conventions 2010:
         // multipliers of s, h, p, N', ps; dR(ip), dR(op), dT(ip), dT(op) [mm]
      static const double datdi[31][9] = {
         {-3.0,  0.0,  2.0,  0.0,  0.0, -0.01,  0.0,   0.0,   0.0 },
         {-3.0,  2.0,  0.0,  0.0,  0.0, -0.01,  0.0,   0.0,   0.0 },
         {-2.0,  0.0,  1.0, -1.0,  0.0, -0.02,  0.0,   0.0,   0.0 },
         {-2.0,  0.0,  1.0,  0.0,  0.0, -0.08,  0.0,  -0.01,  0.01},
         {-2.0,  2.0, -1.0,  0.0,  0.0, -0.02,  0.0,   0.0,   0.0 },
         {-1.0,  0.0,  0.0, -1.0,  0.0, -0.10,  0.0,   0.0,   0.0 },
         {-1.0,  0.0,  0.0,  0.0,  0.0, -0.51,  0.0,  -0.02,  0.03},
         {-1.0,  2.0,  0.0,  0.0,  0.0,  0.01,  0.0,   0.0,   0.0 },
         { 0.0, -2.0,  1.0,  0.0,  0.0,  0.01,  0.0,   0.0,   0.0 },
         { 0.0,  0.0, -1.0,  0.0,  0.0,  0.02,  0.0,   0.0,   0.0 },
         { 0.0,  0.0,  1.0,  0.0,  0.0,  0.06,  0.0,   0.0,   0.0 },
         { 0.0,  0.0,  1.0,  1.0,  0.0,  0.01,  0.0,   0.0,   0.0 },
         { 0.0,  2.0, -1.0,  0.0,  0.0,  0.01,  0.0,   0.0,   0.0 },
         { 1.0, -3.0,  0.0,  0.0,  1.0, -0.06,  0.0,   0.0,   0.0 },
         { 1.0, -2.0,  0.0, -1.0,  0.0,  0.01,  0.0,   0.0,   0.0 },
         { 1.0, -2.0,  0.0,  0.0,  0.0, -1.23, -0.07,  0.06,  0.01},
         { 1.0, -1.0,  0.0,  0.0, -1.0,  0.02,  0.0,   0.0,   0.0 },
         { 1.0, -1.0,  0.0,  0.0,  1.0,  0.04,  0.0,   0.0,   0.0 },
         { 1.0,  0.0,  0.0, -1.0,  0.0, -0.22,  0.01,  0.01,  0.0 },
         { 1.0,  0.0,  0.0,  0.0,  0.0, 12.00, -0.80, -0.67, -0.03},
         { 1.0,  0.0,  0.0,  1.0,  0.0,  1.73, -0.12, -0.10,  0.0 },
         { 1.0,  0.0,  0.0,  2.0,  0.0, -0.04,  0.0,   0.0,   0.0 },
         { 1.0,  1.0,  0.0,  0.0, -1.0, -0.50, -0.01,  0.03,  0.0 },
         { 1.0,  1.0,  0.0,  0.0,  1.0,  0.01,  0.0,   0.0,   0.0 },
         { 0.0,  1.0,  0.0,  1.0, -1.0, -0.01,  0.0,   0.0,   0.0 },
         { 1.0,  2.0, -2.0,  0.0,  0.0, -0.01,  0.0,   0.0,   0.0 },
         { 1.0,  2.0,  0.0,  0.0,  0.0, -0.11,  0.01,  0.01,  0.0 },
         { 2.0, -2.0,  1.0,  0.0,  0.0, -0.01,  0.0,   0.0,   0.0 },
         { 2.0,  0.0, -1.0,  0.0,  0.0, -0.02,  0.0,   0.0,   0.0 },
         { 3.0,  0.0,  0.0,  0.0,  0.0,  0.0,   0.0,   0.0,   0.0 },
         { 3.0,  0.0,  0.0,  1.0,  0.0,  0.0,   0.0,   0.0,   0.0 }
      };

      double s, h, p, zns, ps;
      fundArgs(T, s, h, p, zns, ps);

         // tau is computed with the mean longitude of the Moon without
         // the general precession term
      const double s0 = 218.31664563 + (481267.88194 + (-0.0014663889
                        + 0.00000185139*T)*T)*T;
      double tau = (280.4606184 + (36000.7700536 + (0.00038793
                   + (-0.0000000258)*T)*T)*T) + fhr*15.0 - s0;
      tau = mod360(tau);

      const StaGeo g(xsta);
      const double zla = std::atan2(xsta[1], xsta[0]);
      const double cos2phi = g.cosphi*g.cosphi - g.sinphi*g.sinphi;

      Triple res(0.0, 0.0, 0.0);
      for (int j = 0; j < 31; ++j)
      {
         const double* d = datdi[j];
         const double thetaf = (tau + d[0]*s + d[1]*h + d[2]*p
                                + d[3]*zns + d[4]*ps) * DEG2RAD;
         const double sa = std::sin(thetaf + zla);
         const double ca = std::cos(thetaf + zla);

         const double dr = d[5]*2.0*g.sinphi*g.cosphi*sa
                         + d[6]*2.0*g.sinphi*g.cosphi*ca;
         const double dn = d[7]*cos2phi*sa + d[8]*cos2phi*ca;
         const double de = d[7]*g.sinphi*ca - d[8]*g.sinphi*sa;

         addRNE(res, dr, dn, de, g.sinphi, g.cosphi, g.sinla, g.cosla);
      }

         // mm -> m
      for (int i = 0; i < 3; ++i)
         res[i] /= 1000.0;

      return res;

   } // End SolidTides::step2diu


   Triple SolidTides::step2lon(const Triple& xsta, double T)
   {
         // Table 7.3b, IERS Conventions 2010:
         // multipliers of s, h, p, N', ps; dR(ip), dT(ip), dR(op), dT(op) [mm]
      static const double datdi[5][9] = {
         { 0.0, 0.0,  0.0, 1.0, 0.0,  0.47,  0.23,  0.16,  0.07},
         { 0.0, 2.0,  0.0, 0.0, 0.0, -0.20, -0.12, -0.11, -0.05},
         { 1.0, 0.0, -1.0, 0.0, 0.0, -0.11, -0.08, -0.09, -0.04},
         { 2.0, 0.0,  0.0, 0.0, 0.0, -0.13, -0.11, -0.15, -0.07},
         { 2.0, 0.0,  0.0, 1.0, 0.0, -0.05, -0.05, -0.06, -0.03}
      };

      double s, h, p, zns, ps;
      fundArgs(T, s, h, p, zns, ps);

      const StaGeo g(xsta);
      const double p20 = (3.0*g.sinphi*g.sinphi - 1.0)/2.0;
      const double s2phi = 2.0*g.cosphi*g.sinphi;

      Triple res(0.0, 0.0, 0.0);
      for (int j = 0; j < 5; ++j)
      {
         const double* d = datdi[j];
         const double thetaf = (d[0]*s + d[1]*h + d[2]*p
                                + d[3]*zns + d[4]*ps) * DEG2RAD;
         const double ct = std::cos(thetaf);
         const double st = std::sin(thetaf);

         const double dr = d[5]*p20*ct + d[7]*p20*st;
         const double dn = d[6]*s2phi*ct + d[8]*s2phi*st;

         addRNE(res, dr, dn, 0.0, g.sinphi, g.cosphi, g.sinla, g.cosla);
      }

         // mm -> m
      for (int i = 0; i < 3; ++i)
         res[i] /= 1000.0;

      return res;

   } // End SolidTides::step2lon


} // end namespace gnsstk

