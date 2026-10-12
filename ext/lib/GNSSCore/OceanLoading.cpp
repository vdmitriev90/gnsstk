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

/**
 * @file OceanLoading.cpp
 * This class computes the effect of ocean tides loading at a given station
 * and epoch, following the IERS Conventions (2010) HARDISP algorithm.
 */

#include <algorithm>
#include <cmath>
#include "OceanLoading.hpp"
#include "CivilTime.hpp"
#include "TimeSystem.hpp"
#include "StringUtils.hpp"
#include "GNSSconstants.hpp"

using namespace std;

namespace gnsstk
{

   namespace
   {

         /// Constituent of the tidal potential: Doodson number and
         /// Cartwright-Tayler-Edden amplitude (from IERS ADMINT.F).
      struct TidalConstituent
      {
         int doodson[6];
         double amp;
      };


         /// The 342 constituents used by IERS HARDISP.F (ADMINT.F).
      const TidalConstituent CONSTITUENTS[OceanLoading::NUM_CONSTITUENTS] =
      {
      { { 2, 0, 0, 0, 0, 0},   0.632208 },
      { { 2, 2,-2, 0, 0, 0},   0.294107 },
      { { 2,-1, 0, 1, 0, 0},   0.121046 },
      { { 2, 2, 0, 0, 0, 0},   0.079915 },
      { { 2, 2, 0, 0, 1, 0},   0.023818 },
      { { 2, 0, 0, 0,-1, 0},  -0.023589 },
      { { 2,-1, 2,-1, 0, 0},   0.022994 },
      { { 2,-2, 2, 0, 0, 0},   0.019333 },
      { { 2, 1, 0,-1, 0, 0},  -0.017871 },
      { { 2, 2,-3, 0, 0, 1},   0.017192 },
      { { 2,-2, 0, 2, 0, 0},   0.016018 },
      { { 2,-3, 2, 1, 0, 0},   0.004671 },
      { { 2, 1,-2, 1, 0, 0},  -0.004662 },
      { { 2,-1, 0, 1,-1, 0},  -0.004519 },
      { { 2, 3, 0,-1, 0, 0},   0.004470 },
      { { 2, 1, 0, 1, 0, 0},   0.004467 },
      { { 2, 2, 0, 0, 2, 0},   0.002589 },
      { { 2, 2,-1, 0, 0,-1},  -0.002455 },
      { { 2, 0,-1, 0, 0, 1},  -0.002172 },
      { { 2, 1, 0, 1, 1, 0},   0.001972 },
      { { 2, 3, 0,-1, 1, 0},   0.001947 },
      { { 2, 0, 1, 0, 0,-1},   0.001914 },
      { { 2, 0,-2, 2, 0, 0},  -0.001898 },
      { { 2,-3, 0, 3, 0, 0},   0.001802 },
      { { 2,-2, 3, 0, 0,-1},   0.001304 },
      { { 2, 4, 0, 0, 0, 0},   0.001170 },
      { { 2,-1, 1, 1, 0,-1},   0.001130 },
      { { 2,-1, 3,-1, 0,-1},   0.001061 },
      { { 2, 2, 0, 0,-1, 0},  -0.001022 },
      { { 2,-1,-1, 1, 0, 1},  -0.001017 },
      { { 2, 4, 0, 0, 1, 0},   0.001014 },
      { { 2,-3, 4,-1, 0, 0},   0.000901 },
      { { 2,-1, 2,-1,-1, 0},  -0.000857 },
      { { 2, 3,-2, 1, 0, 0},   0.000855 },
      { { 2, 1, 2,-1, 0, 0},   0.000855 },
      { { 2,-4, 2, 2, 0, 0},   0.000772 },
      { { 2, 4,-2, 0, 0, 0},   0.000741 },
      { { 2, 0, 2, 0, 0, 0},   0.000741 },
      { { 2,-2, 2, 0,-1, 0},  -0.000721 },
      { { 2, 2,-4, 0, 0, 2},   0.000698 },
      { { 2, 2,-2, 0,-1, 0},   0.000658 },
      { { 2, 1, 0,-1,-1, 0},   0.000654 },
      { { 2,-1, 1, 0, 0, 0},  -0.000653 },
      { { 2, 2,-1, 0, 0, 1},   0.000633 },
      { { 2, 2, 1, 0, 0,-1},   0.000626 },
      { { 2,-2, 0, 2,-1, 0},  -0.000598 },
      { { 2,-2, 4,-2, 0, 0},   0.000590 },
      { { 2, 2, 2, 0, 0, 0},   0.000544 },
      { { 2,-4, 4, 0, 0, 0},   0.000479 },
      { { 2,-1, 0,-1,-2, 0},  -0.000464 },
      { { 2, 1, 2,-1, 1, 0},   0.000413 },
      { { 2,-1,-2, 3, 0, 0},  -0.000390 },
      { { 2, 3,-2, 1, 1, 0},   0.000373 },
      { { 2, 4, 0,-2, 0, 0},   0.000366 },
      { { 2, 0, 0, 2, 0, 0},   0.000366 },
      { { 2, 0, 2,-2, 0, 0},  -0.000360 },
      { { 2, 0, 2, 0, 1, 0},  -0.000355 },
      { { 2,-3, 3, 1, 0,-1},   0.000354 },
      { { 2, 0, 0, 0,-2, 0},   0.000329 },
      { { 2, 4, 0, 0, 2, 0},   0.000328 },
      { { 2, 4,-2, 0, 1, 0},   0.000319 },
      { { 2, 0, 0, 0, 0, 2},   0.000302 },
      { { 2, 1, 0, 1, 2, 0},   0.000279 },
      { { 2, 0,-2, 0,-2, 0},  -0.000274 },
      { { 2,-2, 1, 0, 0, 1},  -0.000272 },
      { { 2,-2, 1, 2, 0,-1},   0.000248 },
      { { 2,-1, 1,-1, 0, 1},  -0.000225 },
      { { 2, 5, 0,-1, 0, 0},   0.000224 },
      { { 2, 1,-3, 1, 0, 1},  -0.000223 },
      { { 2,-2,-1, 2, 0, 1},  -0.000216 },
      { { 2, 3, 0,-1, 2, 0},   0.000211 },
      { { 2, 1,-2, 1,-1, 0},   0.000209 },
      { { 2, 5, 0,-1, 1, 0},   0.000194 },
      { { 2,-4, 0, 4, 0, 0},   0.000185 },
      { { 2,-3, 2, 1,-1, 0},  -0.000174 },
      { { 2,-2, 1, 1, 0, 0},  -0.000171 },
      { { 2, 4, 0,-2, 1, 0},   0.000159 },
      { { 2, 0, 0, 2, 1, 0},   0.000131 },
      { { 2,-5, 4, 1, 0, 0},   0.000127 },
      { { 2, 0, 2, 0, 2, 0},   0.000120 },
      { { 2,-1, 2, 1, 0, 0},   0.000118 },
      { { 2, 5,-2,-1, 0, 0},   0.000117 },
      { { 2, 1,-1, 0, 0, 0},   0.000108 },
      { { 2, 2,-2, 0, 0, 2},   0.000107 },
      { { 2,-5, 2, 3, 0, 0},   0.000105 },
      { { 2,-1,-2, 1,-2, 0},  -0.000102 },
      { { 2,-3, 5,-1, 0,-1},   0.000102 },
      { { 2,-1, 0, 0, 0, 1},   0.000099 },
      { { 2,-2, 0, 0,-2, 0},  -0.000096 },
      { { 2, 0,-1, 1, 0, 0},   0.000095 },
      { { 2,-3, 1, 1, 0, 1},  -0.000089 },
      { { 2, 3, 0,-1,-1, 0},  -0.000085 },
      { { 2, 1, 0, 1,-1, 0},  -0.000084 },
      { { 2,-1, 2, 1, 1, 0},  -0.000081 },
      { { 2, 0,-3, 2, 0, 1},  -0.000077 },
      { { 2, 1,-1,-1, 0, 1},  -0.000072 },
      { { 2,-3, 0, 3,-1, 0},  -0.000067 },
      { { 2, 0,-2, 2,-1, 0},   0.000066 },
      { { 2,-4, 3, 2, 0,-1},   0.000064 },
      { { 2,-1, 0, 1,-2, 0},   0.000063 },
      { { 2, 5, 0,-1, 2, 0},   0.000063 },
      { { 2,-4, 5, 0, 0,-1},   0.000063 },
      { { 2,-2, 4, 0, 0,-2},   0.000062 },
      { { 2,-1, 0, 1, 0, 2},   0.000062 },
      { { 2,-2,-2, 4, 0, 0},  -0.000060 },
      { { 2, 3,-2,-1,-1, 0},   0.000056 },
      { { 2,-2, 5,-2, 0,-1},   0.000053 },
      { { 2, 0,-1, 0,-1, 1},   0.000051 },
      { { 2, 5,-2,-1, 1, 0},   0.000050 },
      { { 1, 1, 0, 0, 0, 0},   0.368645 },
      { { 1,-1, 0, 0, 0, 0},  -0.262232 },
      { { 1, 1,-2, 0, 0, 0},  -0.121995 },
      { { 1,-2, 0, 1, 0, 0},  -0.050208 },
      { { 1, 1, 0, 0, 1, 0},   0.050031 },
      { { 1,-1, 0, 0,-1, 0},  -0.049470 },
      { { 1, 2, 0,-1, 0, 0},   0.020620 },
      { { 1, 0, 0, 1, 0, 0},   0.020613 },
      { { 1, 3, 0, 0, 0, 0},   0.011279 },
      { { 1,-2, 2,-1, 0, 0},  -0.009530 },
      { { 1,-2, 0, 1,-1, 0},  -0.009469 },
      { { 1,-3, 2, 0, 0, 0},  -0.008012 },
      { { 1, 0, 0,-1, 0, 0},   0.007414 },
      { { 1, 1, 0, 0,-1, 0},  -0.007300 },
      { { 1, 3, 0, 0, 1, 0},   0.007227 },
      { { 1, 1,-3, 0, 0, 1},  -0.007131 },
      { { 1,-3, 0, 2, 0, 0},  -0.006644 },
      { { 1, 1, 2, 0, 0, 0},   0.005249 },
      { { 1, 0, 0, 1, 1, 0},   0.004137 },
      { { 1, 2, 0,-1, 1, 0},   0.004087 },
      { { 1, 0, 2,-1, 0, 0},   0.003944 },
      { { 1, 2,-2, 1, 0, 0},   0.003943 },
      { { 1, 3,-2, 0, 0, 0},   0.003420 },
      { { 1,-1, 2, 0, 0, 0},   0.003418 },
      { { 1, 1, 1, 0, 0,-1},   0.002885 },
      { { 1, 1,-1, 0, 0, 1},   0.002884 },
      { { 1, 4, 0,-1, 0, 0},   0.002160 },
      { { 1,-4, 2, 1, 0, 0},  -0.001936 },
      { { 1, 0,-2, 1, 0, 0},   0.001934 },
      { { 1,-2, 2,-1,-1, 0},  -0.001798 },
      { { 1, 3, 0,-2, 0, 0},   0.001690 },
      { { 1,-1, 0, 2, 0, 0},   0.001689 },
      { { 1,-1, 0, 0,-2, 0},   0.001516 },
      { { 1, 3, 0, 0, 2, 0},   0.001514 },
      { { 1,-3, 2, 0,-1, 0},  -0.001511 },
      { { 1, 4, 0,-1, 1, 0},   0.001383 },
      { { 1, 0, 0,-1,-1, 0},   0.001372 },
      { { 1, 1,-2, 0,-1, 0},   0.001371 },
      { { 1,-3, 0, 2,-1, 0},  -0.001253 },
      { { 1, 1, 0, 0, 2, 0},  -0.001075 },
      { { 1, 1,-1, 0, 0,-1},   0.001020 },
      { { 1,-1,-1, 0, 0, 1},   0.000901 },
      { { 1, 0, 2,-1, 1, 0},   0.000865 },
      { { 1,-1, 1, 0, 0,-1},  -0.000794 },
      { { 1,-1,-2, 2, 0, 0},   0.000788 },
      { { 1, 2,-2, 1, 1, 0},   0.000782 },
      { { 1,-4, 0, 3, 0, 0},  -0.000747 },
      { { 1,-1, 2, 0, 1, 0},  -0.000745 },
      { { 1, 3,-2, 0, 1, 0},   0.000670 },
      { { 1, 2, 0,-1,-1, 0},  -0.000603 },
      { { 1, 0, 0, 1,-1, 0},  -0.000597 },
      { { 1,-2, 2, 1, 0, 0},   0.000542 },
      { { 1, 4,-2,-1, 0, 0},   0.000542 },
      { { 1,-3, 3, 0, 0,-1},  -0.000541 },
      { { 1,-2, 1, 1, 0,-1},  -0.000469 },
      { { 1,-2, 3,-1, 0,-1},  -0.000440 },
      { { 1, 0,-2, 1,-1, 0},   0.000438 },
      { { 1,-2,-1, 1, 0, 1},   0.000422 },
      { { 1, 4,-2, 1, 0, 0},   0.000410 },
      { { 1,-4, 4,-1, 0, 0},  -0.000374 },
      { { 1,-4, 2, 1,-1, 0},  -0.000365 },
      { { 1, 5,-2, 0, 0, 0},   0.000345 },
      { { 1, 3, 0,-2, 1, 0},   0.000335 },
      { { 1,-5, 2, 2, 0, 0},  -0.000321 },
      { { 1, 2, 0, 1, 0, 0},  -0.000319 },
      { { 1, 1, 3, 0, 0,-1},   0.000307 },
      { { 1,-2, 0, 1,-2, 0},   0.000291 },
      { { 1, 4, 0,-1, 2, 0},   0.000290 },
      { { 1, 1,-4, 0, 0, 2},  -0.000289 },
      { { 1, 5, 0,-2, 0, 0},   0.000286 },
      { { 1,-1, 0, 2, 1, 0},   0.000275 },
      { { 1,-2, 1, 0, 0, 0},   0.000271 },
      { { 1, 4,-2, 1, 1, 0},   0.000263 },
      { { 1,-3, 4,-2, 0, 0},  -0.000245 },
      { { 1,-1, 3, 0, 0,-1},   0.000225 },
      { { 1, 3,-3, 0, 0, 1},   0.000225 },
      { { 1, 5,-2, 0, 1, 0},   0.000221 },
      { { 1, 1, 2, 0, 1, 0},  -0.000202 },
      { { 1, 2, 0, 1, 1, 0},  -0.000200 },
      { { 1,-5, 4, 0, 0, 0},  -0.000199 },
      { { 1,-2, 0,-1,-2, 0},   0.000192 },
      { { 1, 5, 0,-2, 1, 0},   0.000183 },
      { { 1, 1, 2,-2, 0, 0},   0.000183 },
      { { 1, 1,-2, 2, 0, 0},   0.000183 },
      { { 1,-2, 2, 1, 1, 0},  -0.000170 },
      { { 1, 0, 3,-1, 0,-1},   0.000169 },
      { { 1, 2,-3, 1, 0, 1},   0.000168 },
      { { 1,-2,-2, 3, 0, 0},   0.000162 },
      { { 1,-1, 2,-2, 0, 0},   0.000149 },
      { { 1,-4, 3, 1, 0,-1},  -0.000147 },
      { { 1,-4, 0, 3,-1, 0},  -0.000141 },
      { { 1,-1,-2, 2,-1, 0},   0.000138 },
      { { 1,-2, 0, 3, 0, 0},   0.000136 },
      { { 1, 4, 0,-3, 0, 0},   0.000136 },
      { { 1, 0, 1, 1, 0,-1},   0.000127 },
      { { 1, 2,-1,-1, 0, 1},   0.000127 },
      { { 1, 2,-2, 1,-1, 0},  -0.000126 },
      { { 1, 0, 0,-1,-2, 0},  -0.000121 },
      { { 1, 2, 0, 1, 2, 0},  -0.000121 },
      { { 1, 2,-2,-1,-1, 0},   0.000117 },
      { { 1, 0, 0, 1, 2, 0},  -0.000116 },
      { { 1, 0, 1, 0, 0, 0},  -0.000114 },
      { { 1, 2,-1, 0, 0, 0},  -0.000114 },
      { { 1, 0, 2,-1,-1, 0},  -0.000114 },
      { { 1,-1,-2, 0,-2, 0},   0.000114 },
      { { 1,-3, 1, 0, 0, 1},   0.000113 },
      { { 1, 3,-2, 0,-1, 0},   0.000109 },
      { { 1,-1,-1, 0,-1, 1},   0.000108 },
      { { 1, 4,-2,-1, 1, 0},   0.000106 },
      { { 1, 2, 1,-1, 0,-1},  -0.000106 },
      { { 1, 0,-1, 1, 0, 1},  -0.000106 },
      { { 1,-2, 4,-1, 0, 0},   0.000105 },
      { { 1, 4,-4, 1, 0, 0},   0.000104 },
      { { 1,-3, 1, 2, 0,-1},  -0.000103 },
      { { 1,-3, 3, 0,-1,-1},  -0.000100 },
      { { 1, 1, 2, 0, 2, 0},  -0.000100 },
      { { 1, 1,-2, 0,-2, 0},  -0.000100 },
      { { 1, 3, 0, 0, 3, 0},   0.000099 },
      { { 1,-1, 2, 0,-1, 0},  -0.000098 },
      { { 1,-2, 1,-1, 0, 1},   0.000093 },
      { { 1, 0,-3, 1, 0, 1},   0.000093 },
      { { 1,-3,-1, 2, 0, 1},   0.000090 },
      { { 1, 2, 0,-1, 2, 0},  -0.000088 },
      { { 1, 6,-2,-1, 0, 0},   0.000083 },
      { { 1, 2, 2,-1, 0, 0},  -0.000083 },
      { { 1,-1, 1, 0,-1,-1},  -0.000082 },
      { { 1,-2, 3,-1,-1,-1},  -0.000081 },
      { { 1,-1, 0, 0, 0, 2},  -0.000079 },
      { { 1,-5, 0, 4, 0, 0},  -0.000077 },
      { { 1, 1, 0, 0, 0,-2},  -0.000075 },
      { { 1,-2, 1, 1,-1,-1},  -0.000075 },
      { { 1, 1,-1, 0, 1, 1},  -0.000075 },
      { { 1, 1, 2, 0, 0,-2},   0.000071 },
      { { 1,-3, 1, 1, 0, 0},   0.000071 },
      { { 1,-4, 4,-1,-1, 0},  -0.000071 },
      { { 1, 1, 0,-2,-1, 0},   0.000068 },
      { { 1,-2,-1, 1,-1, 1},   0.000068 },
      { { 1,-3, 2, 2, 0, 0},   0.000065 },
      { { 1, 5,-2,-2, 0, 0},   0.000065 },
      { { 1, 3,-4, 2, 0, 0},   0.000064 },
      { { 1, 1,-2, 0, 0, 2},   0.000064 },
      { { 1,-1, 4,-2, 0, 0},   0.000064 },
      { { 1, 2, 2,-1, 1, 0},  -0.000064 },
      { { 1,-5, 2, 2,-1, 0},  -0.000060 },
      { { 1, 1,-3, 0,-1, 1},   0.000056 },
      { { 1, 1, 1, 0, 1,-1},   0.000056 },
      { { 1, 6,-2,-1, 1, 0},   0.000053 },
      { { 1,-2, 2,-1,-2, 0},   0.000053 },
      { { 1, 4,-2, 1, 2, 0},   0.000053 },
      { { 1,-6, 4, 1, 0, 0},  -0.000053 },
      { { 1, 5,-4, 0, 0, 0},   0.000053 },
      { { 1,-3, 4, 0, 0, 0},   0.000053 },
      { { 1, 1, 2,-2, 1, 0},   0.000052 },
      { { 1,-2, 1, 0,-1, 0},   0.000050 },
      { { 0, 2, 0, 0, 0, 0},  -0.066607 },
      { { 0, 1, 0,-1, 0, 0},  -0.035184 },
      { { 0, 0, 2, 0, 0, 0},  -0.030988 },
      { { 0, 0, 0, 0, 1, 0},   0.027929 },
      { { 0, 2, 0, 0, 1, 0},  -0.027616 },
      { { 0, 3, 0,-1, 0, 0},  -0.012753 },
      { { 0, 1,-2, 1, 0, 0},  -0.006728 },
      { { 0, 2,-2, 0, 0, 0},  -0.005837 },
      { { 0, 3, 0,-1, 1, 0},  -0.005286 },
      { { 0, 0, 1, 0, 0,-1},  -0.004921 },
      { { 0, 2, 0,-2, 0, 0},  -0.002884 },
      { { 0, 2, 0, 0, 2, 0},  -0.002583 },
      { { 0, 3,-2, 1, 0, 0},  -0.002422 },
      { { 0, 1, 0,-1,-1, 0},   0.002310 },
      { { 0, 1, 0,-1, 1, 0},   0.002283 },
      { { 0, 4,-2, 0, 0, 0},  -0.002037 },
      { { 0, 1, 0, 1, 0, 0},   0.001883 },
      { { 0, 0, 3, 0, 0,-1},  -0.001811 },
      { { 0, 4, 0,-2, 0, 0},  -0.001687 },
      { { 0, 3,-2, 1, 1, 0},  -0.001004 },
      { { 0, 3,-2,-1, 0, 0},  -0.000925 },
      { { 0, 4,-2, 0, 1, 0},  -0.000844 },
      { { 0, 0, 2, 0, 1, 0},   0.000766 },
      { { 0, 1, 0, 1, 1, 0},   0.000766 },
      { { 0, 4, 0,-2, 1, 0},  -0.000700 },
      { { 0, 3, 0,-1, 2, 0},  -0.000495 },
      { { 0, 5,-2,-1, 0, 0},  -0.000492 },
      { { 0, 1, 2,-1, 0, 0},   0.000491 },
      { { 0, 1,-2, 1,-1, 0},   0.000483 },
      { { 0, 1,-2, 1, 1, 0},   0.000437 },
      { { 0, 2,-2, 0,-1, 0},  -0.000416 },
      { { 0, 2,-3, 0, 0, 1},  -0.000384 },
      { { 0, 2,-2, 0, 1, 0},   0.000374 },
      { { 0, 0, 2,-2, 0, 0},  -0.000312 },
      { { 0, 1,-3, 1, 0, 1},  -0.000288 },
      { { 0, 0, 0, 0, 2, 0},  -0.000273 },
      { { 0, 0, 1, 0, 0, 1},   0.000259 },
      { { 0, 1, 2,-1, 1, 0},   0.000245 },
      { { 0, 3, 0,-3, 0, 0},  -0.000232 },
      { { 0, 2, 1, 0, 0,-1},   0.000229 },
      { { 0, 1,-1,-1, 0, 1},  -0.000216 },
      { { 0, 1, 0, 1, 2, 0},   0.000206 },
      { { 0, 5,-2,-1, 1, 0},  -0.000204 },
      { { 0, 2,-1, 0, 0, 1},  -0.000202 },
      { { 0, 2, 2,-2, 0, 0},   0.000200 },
      { { 0, 1,-1, 0, 0, 0},   0.000195 },
      { { 0, 5, 0,-3, 0, 0},  -0.000190 },
      { { 0, 2, 0,-2, 1, 0},   0.000187 },
      { { 0, 1, 1,-1, 0,-1},   0.000180 },
      { { 0, 3,-4, 1, 0, 0},  -0.000179 },
      { { 0, 0, 2, 0, 2, 0},   0.000170 },
      { { 0, 2, 0,-2,-1, 0},   0.000153 },
      { { 0, 4,-3, 0, 0, 1},  -0.000137 },
      { { 0, 3,-1,-1, 0, 1},  -0.000119 },
      { { 0, 0, 2, 0, 0,-2},  -0.000119 },
      { { 0, 3,-3, 1, 0, 1},  -0.000112 },
      { { 0, 2,-4, 2, 0, 0},  -0.000110 },
      { { 0, 4,-2,-2, 0, 0},  -0.000110 },
      { { 0, 3, 1,-1, 0,-1},   0.000107 },
      { { 0, 5,-4, 1, 0, 0},  -0.000095 },
      { { 0, 3,-2,-1,-1, 0},  -0.000095 },
      { { 0, 3,-2, 1, 2, 0},  -0.000091 },
      { { 0, 4,-4, 0, 0, 0},  -0.000090 },
      { { 0, 6,-2,-2, 0, 0},  -0.000081 },
      { { 0, 5, 0,-3, 1, 0},  -0.000079 },
      { { 0, 4,-2, 0, 2, 0},  -0.000079 },
      { { 0, 2, 2,-2, 1, 0},   0.000077 },
      { { 0, 0, 4, 0, 0,-2},  -0.000073 },
      { { 0, 3,-1, 0, 0, 0},   0.000069 },
      { { 0, 3,-3,-1, 0, 1},  -0.000067 },
      { { 0, 4, 0,-2, 2, 0},  -0.000066 },
      { { 0, 1,-2,-1,-1, 0},   0.000065 },
      { { 0, 2,-1, 0, 0,-1},   0.000064 },
      { { 0, 4,-4, 2, 0, 0},  -0.000062 },
      { { 0, 2, 1, 0, 1,-1},   0.000060 },
      { { 0, 3,-2,-1, 1, 0},   0.000059 },
      { { 0, 4,-3, 0, 1, 1},  -0.000056 },
      { { 0, 2, 0, 0, 3, 0},   0.000055 },
      { { 0, 6,-4, 0, 0, 0},  -0.000051 },
      };


         /// Doodson numbers of the BLQ constituents, in BLQ column order:
         /// M2, S2, N2, K2, K1, O1, P1, Q1, Mf, Mm, Ssa (from HARDISP.F).
      const int BLQ_DOODSON[OceanLoading::NUM_BLQ_CONSTITUENTS][6] =
      {
         { 2, 0, 0, 0, 0, 0 },
         { 2, 2,-2, 0, 0, 0 },
         { 2,-1, 0, 1, 0, 0 },
         { 2, 2, 0, 0, 0, 0 },
         { 1, 1, 0, 0, 0, 0 },
         { 1,-1, 0, 0, 0, 0 },
         { 1, 1,-2, 0, 0, 0 },
         { 1,-2, 0, 1, 0, 0 },
         { 0, 2, 0, 0, 0, 0 },
         { 0, 1, 0,-1, 0, 0 },
         { 0, 0, 2, 0, 0, 0 }
      };


         /// Frequency (cycles/day) and phase (degrees, 0 to 360) of a
         /// constituent with the given Doodson number.
      void constituentFreqPhase( const int doodson[6],
                                 const double argPhase[6],
                                 const double argFreq[6],
                                 double& freq,
                                 double& phase )
      {
         freq = 0.0;
         phase = 0.0;
         for (int i = 0; i < 6; i++)
         {
            freq += doodson[i] * argFreq[i];
            phase += doodson[i] * argPhase[i];
         }
         phase = std::fmod(phase, 360.0);
         if (phase < 0.0)
         {
            phase += 360.0;
         }
      }


         /// Set up the second derivatives of a cubic spline (IERS SPLINE.F).
         /// For three or less points, linear interpolation is used.
      void splineSetup( int n,
                        const double* x,
                        const double* u,
                        double* s )
      {
         if (n <= 3)
         {
            std::fill(s, s + n, 0.0);
            return;
         }

         auto q = [](double u1, double x1, double u2, double x2)
         { return (u1/(x1*x1) - u2/(x2*x2)) / (1.0/x1 - 1.0/x2); };

         const double q1 = q(u[1]-u[0], x[1]-x[0], u[2]-u[0], x[2]-x[0]);
         const double qn = q( u[n-2]-u[n-1], x[n-2]-x[n-1],
                              u[n-3]-u[n-1], x[n-3]-x[n-1] );

         std::vector<double> a(n, 0.0);

         s[0] = 6.0*((u[1]-u[0])/(x[1]-x[0]) - q1);
         for (int i = 1; i < n-1; i++)
         {
            s[i] = ( u[i-1]/(x[i]-x[i-1])
                     - u[i]*(1.0/(x[i]-x[i-1]) + 1.0/(x[i+1]-x[i]))
                     + u[i+1]/(x[i+1]-x[i]) ) * 6.0;
         }
         s[n-1] = 6.0*(qn + (u[n-2]-u[n-1])/(x[n-1]-x[n-2]));

         a[0] = 2.0*(x[1]-x[0]);
         a[1] = 1.5*(x[1]-x[0]) + 2.0*(x[2]-x[1]);
         s[1] -= 0.5*s[0];
         for (int i = 2; i < n-1; i++)
         {
            const double c = (x[i]-x[i-1])/a[i-1];
            a[i] = 2.0*(x[i+1]-x[i-1]) - c*(x[i]-x[i-1]);
            s[i] -= c*s[i-1];
         }
         const double c = (x[n-1]-x[n-2])/a[n-2];
         a[n-1] = (2.0-c)*(x[n-1]-x[n-2]);
         s[n-1] -= c*s[n-2];

            // Back substitute
         s[n-1] /= a[n-1];
         for (int i = n-2; i >= 0; i--)
         {
            s[i] = (s[i] - (x[i+1]-x[i])*s[i+1])/a[i];
         }
      }


         /// Evaluate a cubic spline at y (IERS EVAL.F). Out of range values
         /// are replaced by the end point values.
      double splineEval( double y,
                         int n,
                         const double* x,
                         const double* u,
                         const double* s )
      {
         if (n <= 0)
         {
            return 0.0;
         }
         if (y <= x[0])
         {
            return u[0];
         }
         if (y >= x[n-1])
         {
            return u[n-1];
         }

         int k2 = 1;
         while (!(x[k2-1] < y && x[k2] >= y))
         {
            ++k2;
         }
         const int k1 = k2 - 1;

         const double dy = x[k2] - y;
         const double dy1 = y - x[k1];
         const double dk = x[k2] - x[k1];
         const double f1 = (s[k1]*dy*dy*dy + s[k2]*dy1*dy1*dy1)/(6.0*dk);
         const double f2 = dy1*(u[k2]/dk - s[k2]*dk/6.0);
         const double f3 = dy*(u[k1]/dk - s[k1]*dk/6.0);
         return f1 + f2 + f3;
      }

   }  // End of anonymous namespace



      /* Returns the effect of ocean tides loading (meters) at the given
       * station and epoch, in the Up-East-North (UEN) reference frame.
       */
   Triple OceanLoading::getOceanLoading( const string& name,
                                         const CommonTime& t )
   {
      try
      {
         const DoodsonArgs args(getDoodsonArgs(t));

         const string key(StringUtils::upperCase(name));
         auto it = stationModels.find(key);
         if (it == stationModels.end())
         {
            const Matrix<double> harmonics(blqData.getTideHarmonics(key));
            if ( harmonics.rows() != 6 ||
                 harmonics.cols() != NUM_BLQ_CONSTITUENTS )
            {
               InvalidRequest e("Invalid BLQ harmonics for station " + name);
               GNSSTK_THROW(e);
            }
            it = stationModels.emplace( key,
                                        buildStationModel(harmonics, args) )
                              .first;
         }
         const StationModel& model(it->second);

            // Displacement in Up, West, South reference frame
         double disp[3] = { 0.0, 0.0, 0.0 };
         for (int k = 0; k < NUM_CONSTITUENTS; k++)
         {
            double freq, phase;
            constituentFreqPhase( CONSTITUENTS[k].doodson,
                                  args.phase, args.freq, freq, phase );
            for (int c = 0; c < 3; c++)
            {
               disp[c] += model.amp[c][k] *
                          std::cos((phase + model.phase[c][k])*DEG_TO_RAD);
            }
         }

            // Let's change Triple to Up, East, North [UEN] reference frame
         return Triple(disp[0], -disp[1], -disp[2]);
      }
      catch (InvalidRequest& e)
      {
         GNSSTK_RETHROW(e);
      }
      catch (Exception& e)
      {
         InvalidRequest ir(e);
         GNSSTK_THROW(ir);
      }

   }  // End of method 'OceanLoading::getOceanLoading()'



      /* Sets the name of BLQ file containing ocean tides harmonics data.
       *
       * @param name      Name of BLQ tides harmonics data file.
       */
   OceanLoading& OceanLoading::setFilename(const string& name)
   {

      fileData = name;

      blqData.open(fileData);

      stationModels.clear();

      return (*this);

   }  // End of method 'OceanLoading::setFilename()'



      /* Compute the Doodson arguments and their rates at the given epoch.
       * Based on IERS routine TDFRPH.F, with exact UTC/TT handling.
       */
   OceanLoading::DoodsonArgs OceanLoading::getDoodsonArgs(
                                                   const CommonTime& time )
   {
      TimeSystem ts(time.getTimeSystem());
      if (ts == TimeSystem::Any || ts == TimeSystem::Unknown)
      {
         ts = TimeSystem::GPS;
      }

         // Convert epoch to UTC
      CommonTime utc(time);
      if (ts != TimeSystem::UTC)
      {
         const CivilTime civ(time);
         utc += getTimeSystemCorrection( ts, TimeSystem::UTC,
                                         civ.year, civ.month, civ.day );
      }
      utc.setTimeSystem(TimeSystem::UTC);

         // TT - UTC = 32.184 s + leap seconds
      const CivilTime civUtc(utc);
      const double ttMinusUtc(
         getTimeSystemCorrection( TimeSystem::UTC, TimeSystem::TT,
                                  civUtc.year, civUtc.month, civUtc.day ) );

         // CommonTime day starts at midnight: JD(0h UTC) = day - 0.5
      long day;
      double sod;
      utc.get(day, sod);
      const double dayFrac(sod/SEC_PER_DAY);
      const double djd(static_cast<double>(day) - 0.5 + dayFrac);

         // Julian centuries from J2000.0, in TT
      const double T( (djd - 2451545.0 + ttMinusUtc/SEC_PER_DAY)/36525.0 );

         // IERS expressions for the Delaunay arguments, in degrees
      const double F1 =     134.9634025100 +
         T*( 477198.8675605000 +
         T*(      0.0088553333 +
         T*(      0.0000143431 +
         T*(     -0.0000000680 ))));
      const double F2 =     357.5291091806 +
         T*(  35999.0502911389 +
         T*(     -0.0001536667 +
         T*(      0.0000000378 +
         T*(     -0.0000000032 ))));
      const double F3 =      93.2720906200 +
         T*( 483202.0174577222 +
         T*(     -0.0035420000 +
         T*(     -0.0000002881 +
         T*(      0.0000000012 ))));
      const double F4 =     297.8501954694 +
         T*( 445267.1114469445 +
         T*(     -0.0017696111 +
         T*(      0.0000018314 +
         T*(     -0.0000000088 ))));
      const double F5 =     125.0445550100 +
         T*(  -1934.1362619722 +
         T*(      0.0020756111 +
         T*(      0.0000021394 +
         T*(     -0.0000000165 ))));

      DoodsonArgs args;

         // Convert to Doodson (Darwin) variables
      args.phase[0] = 360.0*dayFrac - F4;
      args.phase[1] = F3 + F5;
      args.phase[2] = args.phase[1] - F4;
      args.phase[3] = args.phase[1] - F1;
      args.phase[4] = -F5;
      args.phase[5] = args.phase[2] - F2;

         // Frequencies of Delaunay variables (cycles/day), and from these
         // the same for the Doodson arguments
      const double FD1 =  0.0362916471 + 0.0000000013*T;
      const double FD2 =  0.0027377786;
      const double FD3 =  0.0367481951 - 0.0000000005*T;
      const double FD4 =  0.0338631920 - 0.0000000003*T;
      const double FD5 = -0.0001470938 + 0.0000000003*T;
      args.freq[0] = 1.0 - FD4;
      args.freq[1] = FD3 + FD5;
      args.freq[2] = args.freq[1] - FD4;
      args.freq[3] = args.freq[1] - FD1;
      args.freq[4] = -FD5;
      args.freq[5] = args.freq[2] - FD2;

      return args;

   }  // End of method 'OceanLoading::getDoodsonArgs()'



      /* Build the 342-constituent model of a station from the BLQ
       * harmonics. Based on IERS routine ADMINT.F: the admittance of the
       * 11 BLQ constituents is spline-interpolated in frequency, for each
       * tidal band, to all the 342 constituents.
       */
   OceanLoading::StationModel OceanLoading::buildStationModel(
                                          const Matrix<double>& harmonics,
                                          const DoodsonArgs& args )
   {
      const int N = NUM_BLQ_CONSTITUENTS;

         // Find BLQ constituents in table, and get their frequencies
      int tableIndex[N];
      double blqFreq[N];
      for (int ll = 0; ll < N; ll++)
      {
         tableIndex[ll] = -1;
         for (int kk = 0; kk < NUM_CONSTITUENTS; kk++)
         {
            if (std::equal( BLQ_DOODSON[ll], BLQ_DOODSON[ll] + 6,
                            CONSTITUENTS[kk].doodson ))
            {
               tableIndex[ll] = kk;
               break;
            }
         }
         double phase;
         constituentFreqPhase( BLQ_DOODSON[ll], args.phase, args.freq,
                               blqFreq[ll], phase );
      }

         // Sort BLQ constituents by frequency and count them by band
      int order[N];
      for (int i = 0; i < N; i++)
      {
         order[i] = i;
      }
      std::sort( order, order + N,
                 [&blqFreq](int a, int b) { return blqFreq[a] < blqFreq[b]; } );

      double rf[N];
      int nlp(0), ndi(0), nsd(0);
      for (int i = 0; i < N; i++)
      {
         rf[i] = blqFreq[order[i]];
         if (rf[i] < 0.5) ++nlp;
         if (rf[i] < 1.5 && rf[i] > 0.5) ++ndi;
         if (rf[i] < 2.5 && rf[i] > 1.5) ++nsd;
      }
      const int bandStart[3] = { 0, nlp, nlp + ndi };
      const int bandSize[3] = { nlp, ndi, nsd };

         // Frequencies of all the constituents
      std::vector<double> freq(NUM_CONSTITUENTS);
      for (int k = 0; k < NUM_CONSTITUENTS; k++)
      {
         double phase;
         constituentFreqPhase( CONSTITUENTS[k].doodson, args.phase,
                               args.freq, freq[k], phase );
      }

      StationModel model;

         // Components: Up (radial), West (EW), South (NS)
      for (int c = 0; c < 3; c++)
      {
            // Real and imaginary parts of admittance, sorted by frequency.
            // Phase sign is changed to be negative for lags.
         double rl[N], aim[N];
         for (int i = 0; i < N; i++)
         {
            const int ll(order[i]);
            const double amp(harmonics(c, ll));
            const double phase(-harmonics(c+3, ll)*DEG_TO_RAD);
            const double tamp(std::fabs(CONSTITUENTS[tableIndex[ll]].amp));
            rl[i] = amp*std::cos(phase)/tamp;
            aim[i] = amp*std::sin(phase)/tamp;
         }

            // Set up splines for each band, real and imaginary parts
         double sr[N], si[N];
         for (int b = 0; b < 3; b++)
         {
            const int s0(bandStart[b]);
            splineSetup(bandSize[b], rf + s0, rl + s0, sr + s0);
            splineSetup(bandSize[b], rf + s0, aim + s0, si + s0);
         }

            // Evaluate all harmonics using the interpolated admittance
         model.amp[c].assign(NUM_CONSTITUENTS, 0.0);
         model.phase[c].assign(NUM_CONSTITUENTS, 0.0);
         for (int k = 0; k < NUM_CONSTITUENTS; k++)
         {
            const int species(CONSTITUENTS[k].doodson[0]);
            if (species < 0 || species > 2 || bandSize[species] == 0)
            {
               continue;
            }

            const int s0(bandStart[species]);
            const int n(bandSize[species]);
            const double re(splineEval(freq[k], n, rf + s0, rl + s0, sr + s0));
            const double am(splineEval(freq[k], n, rf + s0, aim + s0, si + s0));

            double phaseOffset(0.0);
            if (species == 0) phaseOffset = 180.0;
            if (species == 1) phaseOffset = 90.0;

            model.amp[c][k] = CONSTITUENTS[k].amp*std::sqrt(re*re + am*am);
            model.phase[c][k] = phaseOffset + std::atan2(am, re)/DEG_TO_RAD;
         }
      }

      return model;

   }  // End of method 'OceanLoading::buildStationModel()'



}  // End of namespace gnsstk
