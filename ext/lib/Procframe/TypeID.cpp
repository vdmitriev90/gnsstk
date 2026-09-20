//============================================================================
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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007, 2008, 2009
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
 * @file TypeID.cpp
 * gnsstk::TypeID - Identifies types of values
 */

#include "TypeID.hpp"

#define ADD_TYPE(x) tStrings[x] = #x
#define ADD_TYPE_EX(x, s) tStrings[x] = s

namespace gnsstk
{

    std::map<TypeID::ValueType, std::string> TypeID::tStrings;
    std::map<std::string, TypeID::ValueType> TypeID::fStrings;

    TypeID::Initializer TypeID::TypeIDsingleton;

    // It should be initialize by false, NEVER CHANGE IT!!!
    bool TypeID::bUserTypeIDRegistered = false;

    // Map holding user defined TypeIDs by a string
    std::map<std::string, TypeID> TypeID::mapUserTypeID;

    // Let's assign type descriptions
    TypeID::Initializer::Initializer()
    {
        ADD_TYPE_EX(Unknown, "UnknownType");

        ADD_TYPE(C1);
        ADD_TYPE(C2);
        ADD_TYPE(P1);
        ADD_TYPE(P2);
        ADD_TYPE(L1);
        ADD_TYPE(L2);
        ADD_TYPE(D1);
        ADD_TYPE(D2);
        ADD_TYPE(S1);
        ADD_TYPE(S2);
        ADD_TYPE(T1);
        ADD_TYPE(T2);
        ADD_TYPE(SSI1);
        ADD_TYPE(LLI1);
        ADD_TYPE(SSI2);
        ADD_TYPE(LLI2);

        ADD_TYPE(C5);
        ADD_TYPE(L5);
        ADD_TYPE(D5);
        ADD_TYPE(S5);
        ADD_TYPE(SSI5);
        ADD_TYPE(LLI5);

        ADD_TYPE(C6);
        ADD_TYPE(L6);
        ADD_TYPE(D6);
        ADD_TYPE(S6);
        ADD_TYPE(SSI6);
        ADD_TYPE(LLI6);

        ADD_TYPE(C7);
        ADD_TYPE(L7);
        ADD_TYPE(D7);
        ADD_TYPE(S7);
        ADD_TYPE(SSI7);
        ADD_TYPE(LLI7);

        ADD_TYPE(C8);
        ADD_TYPE(L8);
        ADD_TYPE(D8);
        ADD_TYPE(S8);
        ADD_TYPE(SSI8);
        ADD_TYPE(LLI8);

        ADD_TYPE(PC);
        ADD_TYPE(LC);
        ADD_TYPE(PI);
        ADD_TYPE(LI);
        ADD_TYPE(Pdelta);
        ADD_TYPE(Ldelta);
        ADD_TYPE(MWubbena);

        ADD_TYPE(GRAPHIC1);
        ADD_TYPE(GRAPHIC2);
        ADD_TYPE(GRAPHIC5);
        ADD_TYPE(GRAPHIC6);
        ADD_TYPE(GRAPHIC7);
        ADD_TYPE(GRAPHIC8);

        ADD_TYPE(WL);
        ADD_TYPE(WL1);
        ADD_TYPE(WL2);
        ADD_TYPE(WL3);
        ADD_TYPE(WL4);
        ADD_TYPE(EWL);

        ADD_TYPE(L1dot);
        ADD_TYPE(L1dot2);
        ADD_TYPE(L2dot);
        ADD_TYPE(L2dot2);
        ADD_TYPE(L5dot);
        ADD_TYPE(L5dot2);
        ADD_TYPE(P1dot);
        ADD_TYPE(P1dot2);
        ADD_TYPE(P2dot);
        ADD_TYPE(P2dot2);
        ADD_TYPE(P5dot);
        ADD_TYPE(P5dot2);
        ADD_TYPE(L6dot);
        ADD_TYPE(L6dot2);
        ADD_TYPE(L7dot);
        ADD_TYPE(L7dot2);
        ADD_TYPE(L8dot);
        ADD_TYPE(L8dot2);
        ADD_TYPE(LCdot);
        ADD_TYPE(LCdot2);
        ADD_TYPE(LIdot);
        ADD_TYPE(LIdot2);
        ADD_TYPE(Ldeltadot);
        ADD_TYPE(Ldeltadot2);

        ADD_TYPE(transmit);
        ADD_TYPE(rho);
        ADD_TYPE(rhodot);
        ADD_TYPE(rhodot2);
        ADD_TYPE(dtSat);
        ADD_TYPE(dtSatdot);
        ADD_TYPE(dtSatdot2);
        ADD_TYPE(rel);
        ADD_TYPE(gravDelay);
        ADD_TYPE(tropo);
        ADD_TYPE(dryTropo);

        ADD_TYPE_EX(dryMap, "dryTropoMap");

        ADD_TYPE(wetTropo);

        ADD_TYPE_EX(wetMap, "wetTropoMap");
        ADD_TYPE_EX(wetMapDot, "wetTropoMapDot");
        ADD_TYPE_EX(wetMapNorth, "wetTropoMap north component");
        ADD_TYPE_EX(wetMapEast, "wetTropoMap east component");
        ADD_TYPE_EX(tropoSlant, "slantTropo");

        ADD_TYPE(CSL1);
        ADD_TYPE(CSL2);
        ADD_TYPE(CSL5);
        ADD_TYPE(CSL6);
        ADD_TYPE(CSL7);
        ADD_TYPE(CSL8);

        ADD_TYPE(satArc);

        ADD_TYPE(dx);
        ADD_TYPE(dy);
        ADD_TYPE(dz);
        ADD_TYPE(dLat);
        ADD_TYPE(dLon);
        ADD_TYPE(dH);

        ADD_TYPE(cdt);
        ADD_TYPE(cdtSat);

        ADD_TYPE(dSatX);
        ADD_TYPE(dSatY);
        ADD_TYPE(dSatZ);
        ADD_TYPE(dSatR);
        ADD_TYPE(dSatT);
        ADD_TYPE(dSatN);

        ADD_TYPE(weight);
        ADD_TYPE(codeBias);

        ADD_TYPE(cdtC1);
        ADD_TYPE(cdtP1);
        ADD_TYPE(cdtC2);
        ADD_TYPE(cdtP2);
        ADD_TYPE(cdtC5);
        ADD_TYPE(cdtP5);
        ADD_TYPE(cdtL1);
        ADD_TYPE(cdtL2);
        ADD_TYPE(cdtL5);
        ADD_TYPE(cdtPC);
        ADD_TYPE(cdtLC);
        ADD_TYPE(cdtWL);
        ADD_TYPE(cdtWL2);
        ADD_TYPE(cdtWL3);
        ADD_TYPE(cdtWL4);
        ADD_TYPE(cdtMW);

        ADD_TYPE(cdtSatC1);
        ADD_TYPE(cdtSatP1);
        ADD_TYPE(cdtSatC2);
        ADD_TYPE(cdtSatP2);
        ADD_TYPE(cdtSatC5);
        ADD_TYPE(cdtSatP5);
        ADD_TYPE(cdtSatL1);
        ADD_TYPE(cdtSatL2);
        ADD_TYPE(cdtSatL5);
        ADD_TYPE(cdtSatPC);
        ADD_TYPE(cdtSatLC);
        ADD_TYPE(cdtSatWL);
        ADD_TYPE(cdtSatMW);

        ADD_TYPE_EX(recX, "RxPositionX");
        ADD_TYPE_EX(recY, "RxPositionY");
        ADD_TYPE_EX(recZ, "RxPositionZ");
        ADD_TYPE_EX(recCdt, "RxCdt");
        ADD_TYPE_EX(recCdtdot, "RxCdtdot");

        ADD_TYPE_EX(recISB_GLN_L1, "Rx ISB GLONASS (L1)");
        ADD_TYPE_EX(recISB_GAL_E1, "Rx ISB GALILEO (E1)");
        ADD_TYPE_EX(recISB_BDS_B1, "Rx ISB BEIDOU (B1)");

        ADD_TYPE_EX(recIFB_GPS_L2, "Rx IFB GPS L2");
        ADD_TYPE_EX(recIFB_GLN_L2, "Rx IFB GLONASS L2");
        ADD_TYPE_EX(recIFB_GAL_E5, "Rx IFB GALILEO E5");
        ADD_TYPE_EX(recIFB_BDS_B2, "Rx IFB BEIDOU B2");

        ADD_TYPE_EX(recZTropo, "RxZenithTropo");
        ADD_TYPE_EX(recZTropoWet, "RxZenithTropoWet");
        ADD_TYPE_EX(recZTropoDry, "RxZenithTropoDry");
        ADD_TYPE_EX(recTropoNorth, "North component of tropospheric delay gradient");
        ADD_TYPE_EX(recTropoEast, "East component of tropospheric delay gradient");

        ADD_TYPE_EX(recStDevX, "Rx Position StDev X");
        ADD_TYPE_EX(recStDevY, "Rx Position St.Dev. Y");
        ADD_TYPE_EX(recStDevZ, "Rx Position St.Dev. Z");
        ADD_TYPE_EX(recStDev3D, "Rx Position St.Dev. 3D");
        ADD_TYPE_EX(recCorrXY, "Rx Position Corr. X-Y");
        ADD_TYPE_EX(recCorrXZ, "Rx Position Corr. X-Z");
        ADD_TYPE_EX(recCorrYZ, "Rx Position Corr. Y-Z");
        ADD_TYPE_EX(recPDOP, "Rx PDOP");
        ADD_TYPE_EX(recHDOP, "Rx HDOP");
        ADD_TYPE_EX(recVDOP, "Rx VDOP");
        ADD_TYPE_EX(recUsedSV, "Rx Used SV");
        ADD_TYPE_EX(recSlnType, "Rx Solution Type");
        ADD_TYPE_EX(recSiteID, "RxSiteId");

        ADD_TYPE_EX(recVX, "Rx Velocity X");
        ADD_TYPE_EX(recVY, "Rx Velocity Y");
        ADD_TYPE_EX(recVZ, "Rx Velocity Z");

        ADD_TYPE_EX(recAX, "Rx Acceleration X");
        ADD_TYPE_EX(recAY, "Rx Acceleration Y");
        ADD_TYPE_EX(recAZ, "Rx Acceleration Z");

        ADD_TYPE_EX(recLat, "Rx Lat");
        ADD_TYPE_EX(recLon, "Rx Lon");
        ADD_TYPE_EX(recH, "Rx H");

        ADD_TYPE_EX(recVLat, "RxVelocityLat");
        ADD_TYPE_EX(recVLon, "RxVelocityLon");
        ADD_TYPE_EX(recVH, "RxVelocityH");

        ADD_TYPE_EX(recALat, "RxAccelerationLat");
        ADD_TYPE_EX(recALon, "RxAccelerationLon");
        ADD_TYPE_EX(recAH, "RxAccelerationH");

        ADD_TYPE_EX(recJ2kX, "RxJ2kPositionX");
        ADD_TYPE_EX(recJ2kY, "RxJ2kPositionY");
        ADD_TYPE_EX(recJ2kZ, "RxJ2kPositionZ");

        ADD_TYPE_EX(recJ2kVX, "RxJ2kVelocityX");
        ADD_TYPE_EX(recJ2kVY, "RxJ2kVelocityY");
        ADD_TYPE_EX(recJ2kVZ, "RxJ2kVelocityZ");

        ADD_TYPE_EX(recJ2kAX, "RxJ2kAccelerationX");
        ADD_TYPE_EX(recJ2kAY, "RxJ2kAccelerationY");
        ADD_TYPE_EX(recJ2kAZ, "RxJ2kAccelerationZ");

        ADD_TYPE(sigma);

        ADD_TYPE_EX(BL1, "Ambiguity L1");
        ADD_TYPE_EX(BL2, "Ambiguity L2");
        ADD_TYPE_EX(BL5, "Ambiguity L5");
        ADD_TYPE_EX(BL6, "Ambiguity L6");
        ADD_TYPE_EX(BL7, "Ambiguity L7");
        ADD_TYPE_EX(BL8, "Ambiguity L8");

        ADD_TYPE_EX(BLC, "Ambiguity LC");
        ADD_TYPE_EX(BWL, "Ambiguity WL");
        ADD_TYPE_EX(BWL2, "Ambiguity WL2");
        ADD_TYPE_EX(BWL3, "Ambiguity WL3");
        ADD_TYPE_EX(BWL4, "Ambiguity WL4");

        ADD_TYPE_EX(mpC1, "multipathC1");
        ADD_TYPE_EX(mpC2, "multipathC2");
        ADD_TYPE_EX(mpC5, "multipathC5");
        ADD_TYPE_EX(mpC6, "multipathC6");
        ADD_TYPE_EX(mpC7, "multipathC7");
        ADD_TYPE_EX(mpC8, "multipathC8");

        ADD_TYPE_EX(mpL1, "multipathL1");
        ADD_TYPE_EX(mpL2, "multipathL2");
        ADD_TYPE_EX(mpL5, "multipathL5");
        ADD_TYPE_EX(mpL6, "multipathL6");
        ADD_TYPE_EX(mpL7, "multipathL7");
        ADD_TYPE_EX(mpL8, "multipathL8");

        ADD_TYPE_EX(instC1, "instrumentalC1");
        ADD_TYPE_EX(instC2, "instrumentalC2");
        ADD_TYPE_EX(instC5, "instrumentalC5");
        ADD_TYPE_EX(instC6, "instrumentalC6");
        ADD_TYPE_EX(instC7, "instrumentalC7");
        ADD_TYPE_EX(instC8, "instrumentalC8");

        ADD_TYPE_EX(instL1, "instrumentalL1");
        ADD_TYPE_EX(instL2, "instrumentalL2");
        ADD_TYPE_EX(instL5, "instrumentalL5");
        ADD_TYPE_EX(instL6, "instrumentalL6");
        ADD_TYPE_EX(instL7, "instrumentalL7");
        ADD_TYPE_EX(instL8, "instrumentalL8");

        ADD_TYPE_EX(prefitC, "Prefit residual Code");
        ADD_TYPE_EX(prefitC1, "Prefit residual CodeC1");
        ADD_TYPE_EX(prefitP1, "Prefit residual CodeP1");
        ADD_TYPE_EX(prefitC2, "Prefit residual CodeC2");
        ADD_TYPE_EX(prefitP2, "Prefit residual CodeP2");
        ADD_TYPE_EX(prefitC5, "Prefit residual CodeC5");
        ADD_TYPE_EX(prefitC6, "Prefit residual CodeC6");
        ADD_TYPE_EX(prefitC7, "Prefit residual CodeC7");
        ADD_TYPE_EX(prefitC8, "Prefit residual CodeC8");
        ADD_TYPE_EX(prefitPC, "Prefit residual PC");

        ADD_TYPE_EX(prefitL, "Prefit residual Phase");
        ADD_TYPE_EX(prefitL1, "Prefit residual PhaseL1");
        ADD_TYPE_EX(prefitL2, "Prefit residual PhaseL2");
        ADD_TYPE_EX(prefitL5, "Prefit residual PhaseL5");
        ADD_TYPE_EX(prefitL6, "Prefit residual PhaseL6");
        ADD_TYPE_EX(prefitL7, "Prefit residual PhaseL7");
        ADD_TYPE_EX(prefitL8, "Prefit residual PhaseL8");
        ADD_TYPE_EX(prefitLC, "Prefit residual LC");

        ADD_TYPE_EX(postfitC, "Postfit residual Code");
        ADD_TYPE_EX(postfitC1, "Postfit residual CodeC1");
        ADD_TYPE_EX(postfitP1, "Postfit residual CodeP1");
        ADD_TYPE_EX(postfitC2, "Postfit residual CodeC2");
        ADD_TYPE_EX(postfitP2, "Postfit residual CodeP2");
        ADD_TYPE_EX(postfitC5, "Postfit residual CodeC5");
        ADD_TYPE_EX(postfitC6, "Postfit residual CodeC6");
        ADD_TYPE_EX(postfitC7, "Postfit residual CodeC7");
        ADD_TYPE_EX(postfitC8, "Postfit residual CodeC8");
        ADD_TYPE_EX(postfitPC, "Postfit residual PC");

        ADD_TYPE_EX(postfitL, "Postfit residual Phase");
        ADD_TYPE_EX(postfitL1, "Postfit residual PhaseL1");
        ADD_TYPE_EX(postfitL2, "Postfit residual PhaseL2");
        ADD_TYPE_EX(postfitL5, "Postfit residual PhaseL5");
        ADD_TYPE_EX(postfitL6, "Postfit residual PhaseL6");
        ADD_TYPE_EX(postfitL7, "Postfit residual PhaseL7");
        ADD_TYPE_EX(postfitL8, "Postfit residual PhaseL8");
        ADD_TYPE_EX(postfitLC, "Postfit residual LC");

        ADD_TYPE_EX(prefitGRAPHIC1, "Prefit residual GRAPHIC1");
        ADD_TYPE_EX(prefitGRAPHIC2, "Prefit residual GRAPHIC2");

        ADD_TYPE_EX(postfitGRAPHIC1, "Postfit residual GRAPHIC1");
        ADD_TYPE_EX(postfitGRAPHIC2, "Postfit residual GRAPHIC2");

        ADD_TYPE_EX(prefitMWubbena, "Prefit MWubbena");

        ADD_TYPE_EX(prefitWL, "Prefit residual WL");
        ADD_TYPE_EX(prefitWL2, "Prefit residual WL2");
        ADD_TYPE_EX(prefitWL3, "Prefit residual WL3");
        ADD_TYPE_EX(prefitWL4, "Prefit residual WL4");

        ADD_TYPE_EX(postfitWL, "Postfit residual WL");
        ADD_TYPE_EX(postfitWL2, "Postfit residual WL2");
        ADD_TYPE_EX(postfitWL3, "Postfit residual WL3");
        ADD_TYPE_EX(postfitWL4, "Postfit residual WL4");
        ADD_TYPE_EX(recX, "RxPositionX");
        ADD_TYPE_EX(recY, "RxPositionY");
        ADD_TYPE_EX(recZ, "RxPositionZ");
        ADD_TYPE_EX(recCdt, "RxCdt");
        ADD_TYPE_EX(recCdtdot, "RxCdtdot");

        ADD_TYPE_EX(recISB_GLN_L1, "Rx ISB GLONASS (L1)");
        ADD_TYPE_EX(recISB_GAL_E1, "Rx ISB GALILEO (E1)");
        ADD_TYPE_EX(recISB_BDS_B1, "Rx ISB BEIDOU (B1)");

        ADD_TYPE_EX(recIFB_GPS_L2, "Rx IFB GPS L2");
        ADD_TYPE_EX(recIFB_GLN_L2, "Rx IFB GLONASS L2");
        ADD_TYPE_EX(recIFB_GAL_E5, "Rx IFB GALILEO E5");
        ADD_TYPE_EX(recIFB_BDS_B2, "Rx IFB BEIDOU B2");

        ADD_TYPE_EX(recZTropo, "RxZenithTropo");
        ADD_TYPE_EX(recZTropoWet, "RxZenithTropoWet");
        ADD_TYPE_EX(recZTropoDry, "RxZenithTropoDry");
        ADD_TYPE_EX(recTropoNorth, "North component of tropospheric delay gradient");
        ADD_TYPE_EX(recTropoEast, "East component of tropospheric delay gradient");

        ADD_TYPE_EX(recStDevX, "Rx Position StDev X");
        ADD_TYPE_EX(recStDevY, "Rx Position St.Dev. Y");
        ADD_TYPE_EX(recStDevZ, "Rx Position St.Dev. Z");
        ADD_TYPE_EX(recStDev3D, "Rx Position St.Dev. 3D");
        ADD_TYPE_EX(recCorrXY, "Rx Position Corr. X-Y");
        ADD_TYPE_EX(recCorrXZ, "Rx Position Corr. X-Z");
        ADD_TYPE_EX(recCorrYZ, "Rx Position Corr. Y-Z");
        ADD_TYPE_EX(recPDOP, "Rx PDOP");
        ADD_TYPE_EX(recHDOP, "Rx HDOP");
        ADD_TYPE_EX(recVDOP, "Rx VDOP");
        ADD_TYPE_EX(recUsedSV, "Rx Used SV");
        ADD_TYPE_EX(recSlnType, "Rx Solution Type");
        ADD_TYPE_EX(recSiteID, "RxSiteId");

        ADD_TYPE_EX(recVX, "Rx Velocity X");
        ADD_TYPE_EX(recVY, "Rx Velocity Y");
        ADD_TYPE_EX(recVZ, "Rx Velocity Z");

        ADD_TYPE_EX(recAX, "Rx Acceleration X");
        ADD_TYPE_EX(recAY, "Rx Acceleration Y");
        ADD_TYPE_EX(recAZ, "Rx Acceleration Z");

        ADD_TYPE_EX(recLat, "Rx Lat");
        ADD_TYPE_EX(recLon, "Rx Lon");
        ADD_TYPE_EX(recH, "Rx H");

        ADD_TYPE_EX(recVLat, "RxVelocityLat");
        ADD_TYPE_EX(recVLon, "RxVelocityLon");
        ADD_TYPE_EX(recVH, "RxVelocityH");

        ADD_TYPE_EX(recALat, "RxAccelerationLat");
        ADD_TYPE_EX(recALon, "RxAccelerationLon");
        ADD_TYPE_EX(recAH, "RxAccelerationH");

        ADD_TYPE_EX(recJ2kX, "RxJ2kPositionX");
        ADD_TYPE_EX(recJ2kY, "RxJ2kPositionY");
        ADD_TYPE_EX(recJ2kZ, "RxJ2kPositionZ");

        ADD_TYPE_EX(recJ2kVX, "RxJ2kVelocityX");
        ADD_TYPE_EX(recJ2kVY, "RxJ2kVelocityY");
        ADD_TYPE_EX(recJ2kVZ, "RxJ2kVelocityZ");

        ADD_TYPE_EX(recJ2kAX, "RxJ2kAccelerationX");
        ADD_TYPE_EX(recJ2kAY, "RxJ2kAccelerationY");
        ADD_TYPE_EX(recJ2kAZ, "RxJ2kAccelerationZ");

        ADD_TYPE(sigma);

        ADD_TYPE(iura);
        ADD_TYPE(Action);

        ADD_TYPE_EX(iono, "verticalIono");
        ADD_TYPE_EX(ionoTEC, "TotalElectronContent");
        ADD_TYPE(ionoMap);
        ADD_TYPE(ionoMap2);
        ADD_TYPE_EX(ionoL1, "slantIonoL1");
        ADD_TYPE_EX(ionoL2, "slantIonoL2");
        ADD_TYPE_EX(ionoL5, "slantIonoL5");
        ADD_TYPE_EX(ionoL6, "slantIonoL6");
        ADD_TYPE_EX(ionoL7, "slantIonoL7");
        ADD_TYPE_EX(ionoL8, "slantIonoL8");

        ADD_TYPE_EX(windUp, "windup");
        ADD_TYPE_EX(satPCenter, "satPhaseCenter");

        ADD_TYPE(satX);
        ADD_TYPE(satY);
        ADD_TYPE(satZ);

        ADD_TYPE(satVX);
        ADD_TYPE(satVY);
        ADD_TYPE(satVZ);

        ADD_TYPE(satAX);
        ADD_TYPE(satAY);
        ADD_TYPE(satAZ);

        ADD_TYPE(satJ2kX);
        ADD_TYPE(satJ2kY);
        ADD_TYPE(satJ2kZ);

        ADD_TYPE(satJ2kVX);
        ADD_TYPE(satJ2kVY);
        ADD_TYPE(satJ2kVZ);

        ADD_TYPE(satJ2kAX);
        ADD_TYPE(satJ2kAY);
        ADD_TYPE(satJ2kAZ);

        ADD_TYPE(elevation);
        ADD_TYPE(azimuth);

        ADD_TYPE_EX(snrL1P, "SNR_L1P");
        ADD_TYPE_EX(snrL2C, "SNR_L2C");

        ADD_TYPE_EX(LP1, "Phase_L1P");
        ADD_TYPE_EX(LC2, "Phase_L2C");

        ADD_TYPE_EX(LbandSNR, "L-Band SNR");
        ADD_TYPE_EX(LbandAGC, "L-Band AGC");

        ADD_TYPE_EX(hrms, "HRMS");
        ADD_TYPE_EX(vrms, "VRMS");

        ADD_TYPE(heading);
        ADD_TYPE(pitch);

        ADD_TYPE_EX(correctionAge, "Correction age");
        ADD_TYPE_EX(recVel3D, "RxVelocity 3D");
        ADD_TYPE_EX(recAntDyn, "Rx Antenna Dynamics");
        ADD_TYPE_EX(satStatus, "SV status code");
        ADD_TYPE_EX(Baseline, "Baseleine length ");

        ADD_TYPE(dummy0);

        ADD_TYPE(Last);
        ADD_TYPE(Placeholder);

        fStrings.clear();
        for (auto& it : tStrings)
            fStrings[it.second] = it.first;
    }

    // Assignment operator
    TypeID TypeID::operator=(const TypeID& right)
    {
        if (this == &right)
            return (*this);
        (*this).type = right.type;
        return *this;
    }

    // Convenience output method
    std::ostream& TypeID::dump(std::ostream& s) const
    {
        s << TypeID::tStrings[type];

        return s;
    } // TypeID::dump()

    // Returns true if this is a valid TypeID. Basically just
    // checks that the enum is defined
    bool TypeID::isValid() const
    {
        return !(type == Unknown);
    }

    /* Static method to add new TypeID's
       * @param string      Identifying string for the new TypeID
       */
    TypeID::ValueType TypeID::newValueType(const std::string& s)
    {
        ValueType newId = static_cast<ValueType>(TypeID::tStrings.rbegin()->first + 1);

        TypeID::tStrings[newId] = s;

        return newId;
    }

    namespace StringUtils
    {

        // convert this object to a string representation
        std::string asString(const TypeID& p)
        {
            std::ostringstream oss;
            p.dump(oss);
            return oss.str();
        }

    } // End of namespace StringUtils

    // stream output for TypeID
    std::ostream& operator<<(std::ostream& s, const TypeID& p)
    {
        p.dump(s);
        return s;
    }

    bool IsCarrierPhase(const RinexObsType& rot)
    {
        return (rot.type[0] == 'L') ? true : false;
    }

    int GetCarrierBand(const RinexObsType& rot)
    {
        // 1 2 5 6 7 8
        try
        {
            return StringUtils::asInt(rot.type.substr(1, 1));
        }
        catch (...)
        {
            return -1;
        }
    }

    int GetCarrierBand(const RinexObsID& roi)
    {
        // 1 2 5 6 7 8
        if (roi.band == CarrierBand::L1)
            return 1;
        if (roi.band == CarrierBand::G1)
            return 1;
        if (roi.band == CarrierBand::B1)
            return 1;

        if (roi.band == CarrierBand::L2)
            return 2;
        if (roi.band == CarrierBand::G2)
            return 2;
        if (roi.band == CarrierBand::B2)
            return 2; // TD this is not correct
        if (roi.band == CarrierBand::L5)
            return 5;

        if (roi.band == CarrierBand::E6)
            return 6;
        if (roi.band == CarrierBand::B3)
            return 6;
        if (roi.band == CarrierBand::E5b)
            return 7;

        if (roi.band == CarrierBand::E5ab)
            return 8;

        return -1;
    }

    TypeID::ValueType ConvertToTypeID(const RinexObsType& rot, const RinexSatID& sat)
    {
        if (sat.system == SatelliteSystem::GPS || sat.system == SatelliteSystem::QZSS)
        {
            //GPS     L1         1575.42     C1,P1       L1         D1         S1
            //        L2         1227.60     C2,P2       L2         D2         S2
            //        L5         1176.45      C5         L5         D5         S5

            // For L1: C1 P1 L1 D1 S1
            if (rot == RinexObsHeader::C1)
                return TypeID::C1;
            if (rot == RinexObsHeader::P1)
                return TypeID::P1;
            if (rot == RinexObsHeader::L1)
                return TypeID::L1;
            if (rot == RinexObsHeader::D1)
                return TypeID::D1;
            if (rot == RinexObsHeader::S1)
                return TypeID::S1;
            // For L2: C2 P2 L2 D2 S2
            if (rot == RinexObsHeader::C2)
                return TypeID::C2;
            if (rot == RinexObsHeader::P2)
                return TypeID::P2;
            if (rot == RinexObsHeader::L2)
                return TypeID::L2;
            if (rot == RinexObsHeader::D2)
                return TypeID::D2;
            if (rot == RinexObsHeader::S2)
                return TypeID::S2;
            // For L5: C5 L5 D5 S5
            if (rot == RinexObsHeader::C5)
                return TypeID::C5;
            if (rot == RinexObsHeader::L5)
                return TypeID::L5;
            if (rot == RinexObsHeader::D5)
                return TypeID::D5;
            if (rot == RinexObsHeader::S5)
                return TypeID::S5;

            if (rot == RinexObsHeader::C6)
                return TypeID::C6;
            if (rot == RinexObsHeader::L6)
                return TypeID::L6;
            if (rot == RinexObsHeader::D6)
                return TypeID::D6;
            if (rot == RinexObsHeader::S6)
                return TypeID::S6;
        }
        else if (sat.system == SatelliteSystem::Glonass)
        {
            // Glonass G1         1602+k*9/16 C1,P1       L1         D1         S1
            //         G2         1246+k*7/16 C2,P2       L2         D2         S2

            // For L1: C1 P1 L1 D1 S1
            if (rot == RinexObsHeader::C1)
                return TypeID::C1;
            if (rot == RinexObsHeader::P1)
                return TypeID::P1;
            if (rot == RinexObsHeader::L1)
                return TypeID::L1;
            if (rot == RinexObsHeader::D1)
                return TypeID::D1;
            if (rot == RinexObsHeader::S1)
                return TypeID::S1;
            // For L2: C2 P2 L2 D2 S2
            if (rot == RinexObsHeader::C2)
                return TypeID::C2;
            if (rot == RinexObsHeader::P2)
                return TypeID::P2;
            if (rot == RinexObsHeader::L2)
                return TypeID::L2;
            if (rot == RinexObsHeader::D2)
                return TypeID::D2;
            if (rot == RinexObsHeader::S2)
                return TypeID::S2;
        }
        else if (sat.system == SatelliteSystem::Galileo)
        {
            // Galileo E2-L1-E1   1575.42      C1         L1         D1         S1
            //         E5a        1176.45      C5         L5         D5         S5
            //         E5b        1207.140     C7         L7         D7         S7
            //         E5a+b      1191.795     C8         L8         D8         S8
            //         E6         1278.75      C6         L6         D6         S6
            // E2-L1-E1
            if (rot == RinexObsHeader::C1)
                return TypeID::C1;
            if (rot == RinexObsHeader::L1)
                return TypeID::L1;
            if (rot == RinexObsHeader::D1)
                return TypeID::D1;
            if (rot == RinexObsHeader::S1)
                return TypeID::S1;
            // E5a
            if (rot == RinexObsHeader::C5)
                return TypeID::C5;
            if (rot == RinexObsHeader::L5)
                return TypeID::L5;
            if (rot == RinexObsHeader::D5)
                return TypeID::D5;
            if (rot == RinexObsHeader::S5)
                return TypeID::S5;
            // E5b
            if (rot == RinexObsHeader::C7)
                return TypeID::C7;
            if (rot == RinexObsHeader::L7)
                return TypeID::L7;
            if (rot == RinexObsHeader::D7)
                return TypeID::D7;
            if (rot == RinexObsHeader::S7)
                return TypeID::S7;
            // E5a+b
            if (rot == RinexObsHeader::C8)
                return TypeID::C8;
            if (rot == RinexObsHeader::L8)
                return TypeID::L8;
            if (rot == RinexObsHeader::D8)
                return TypeID::D8;
            if (rot == RinexObsHeader::S8)
                return TypeID::S8;
            // E6
            if (rot == RinexObsHeader::C6)
                return TypeID::C6;
            if (rot == RinexObsHeader::L6)
                return TypeID::L6;
            if (rot == RinexObsHeader::D6)
                return TypeID::D6;
            if (rot == RinexObsHeader::S6)
                return TypeID::S6;
        }
        else if (sat.system == SatelliteSystem::BeiDou)
        {
            // Compass E2   I/Q                 C2         L2         D2         S2
            //         E5b  I/Q                 C7         L7         D7         S7
            //         E6   I/Q                 C6         L6         D6         S6

            // For E2-B1
            if (rot == RinexObsHeader::C2)
                return TypeID::C2;
            if (rot == RinexObsHeader::L2)
                return TypeID::L2;
            if (rot == RinexObsHeader::D2)
                return TypeID::D2;
            if (rot == RinexObsHeader::S2)
                return TypeID::S2;
            // For E5b-B2
            if (rot == RinexObsHeader::C7)
                return TypeID::C7;
            if (rot == RinexObsHeader::L7)
                return TypeID::L7;
            if (rot == RinexObsHeader::D7)
                return TypeID::D7;
            if (rot == RinexObsHeader::S7)
                return TypeID::S7;
            // For E6-B3
            if (rot == RinexObsHeader::C6)
                return TypeID::C6;
            if (rot == RinexObsHeader::L6)
                return TypeID::L6;
            if (rot == RinexObsHeader::D6)
                return TypeID::D6;
            if (rot == RinexObsHeader::S6)
                return TypeID::S6;
        }
        else if (sat.system == SatelliteSystem::Geosync)
        {
            // SBAS    L1         1575.42      C1         L1         D1         S1
            //         L5         1176.45      C5         L5         D5         S5

            // L1
            if (rot == RinexObsHeader::C1)
                return TypeID::C1;
            if (rot == RinexObsHeader::L1)
                return TypeID::L1;
            if (rot == RinexObsHeader::D1)
                return TypeID::D1;
            if (rot == RinexObsHeader::S1)
                return TypeID::S1;
            // L5
            if (rot == RinexObsHeader::C5)
                return TypeID::C5;
            if (rot == RinexObsHeader::L5)
                return TypeID::L5;
            if (rot == RinexObsHeader::D5)
                return TypeID::D5;
            if (rot == RinexObsHeader::S5)
                return TypeID::S5;
        }

        return TypeID::Unknown;
    }

    TypeID::ValueType ConvertToTypeID(const RinexObsID& roi, const RinexSatID& sat)
    {
        if (sat.system == SatelliteSystem::GPS)
        {
            //GPS     L1         1575.42     C1,P1       L1         D1         S1
            //        L2         1227.60     C2,P2       L2         D2         S2
            //        L5         1176.45      C5         L5         D5         S5

            // For L1: C1 P1 L1 D1 S1
            if (roi.band == CarrierBand::L1)
            {
                if (roi.type == ObservationType::Range)
                    return (roi.code == TrackingCode::CA) ? TypeID::C1 : TypeID::P1;

                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            // For L2: C2 P2 L2 D2 S2
            else if (roi.band == CarrierBand::L2)
            {
                if (roi.type == ObservationType::Range)
                    return (roi.code == TrackingCode::CA || roi.code == TrackingCode::L2CM
                            || roi.code == TrackingCode::L2CL || roi.code == TrackingCode::L2CML)
                               ? TypeID::C2
                               : TypeID::P2;

                if (roi.type == ObservationType::Phase)
                    return TypeID::L2;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D2;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S2;
            }
            // For L5: C5 L5 D5 S5
            else if (roi.band == CarrierBand::L5)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C5;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L5;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D5;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S5;
            }
        }
        else if (sat.system == SatelliteSystem::Glonass)
        {
            // Glonass G1         1602+k*9/16 C1,P1       L1         D1         S1
            //         G2         1246+k*7/16 C2,P2       L2         D2         S2

            // For L1: C1 P1 L1 D1 S1
            if (roi.band == CarrierBand::G1)
            {
                if (roi.type == ObservationType::Range) // tcGCA or tcGP
                {
                    if (roi.type == ObservationType::Range)
                    {
                        switch (roi.code)
                        {
                        case TrackingCode::Standard:
                            return TypeID::C1;
                        case TrackingCode::Precise:
                            return TypeID::P1;
                        default:
                            GNSSTK_ASSERT(false, "Unknown tracking code for Glonass L1: " << roi.code);
                            break;
                        }
                    }
                }

                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            // For L2: C2 P2 L2 D2 S2
            else if (roi.band == CarrierBand::G2)
            {
                if (roi.type == ObservationType::Range)
                {
                    switch (roi.code)
                    {
                    case TrackingCode::Standard:
                        return TypeID::C2;
                    case TrackingCode::Precise:
                        return TypeID::P2;
                    default:
                        GNSSTK_ASSERT(false, "Unknown tracking code for Glonass L2: " << roi.code);
                        break;
                    }
                }

                if (roi.type == ObservationType::Phase)
                    return TypeID::L2;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D2;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S2;
            }
        }
        else if (sat.system == SatelliteSystem::Galileo)
        {
            // Galileo E2-L1-E1   1575.42      C1         L1         D1         S1
            //         E5a        1176.45      C5         L5         D5         S5
            //         E5b        1207.140     C7         L7         D7         S7
            //         E5a+b      1191.795     C8         L8         D8         S8
            //         E6         1278.75      C6         L6         D6         S6
            // E2-L1-E1
            if (roi.band == CarrierBand::L1) // E1
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C1;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            else if (roi.band == CarrierBand::L5) // E5a
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C5;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L5;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D5;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S5;
            }
            else if (roi.band == CarrierBand::E5b) // E5b
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C7;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L7;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D7;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S7;
            }
            else if (roi.band == CarrierBand::E5ab) // E5a+b
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C8;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L8;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D8;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S8;
            }
            else if (roi.band == CarrierBand::E6) // E6
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C6;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L6;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D6;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S6;
            }
        }
        else if (sat.system == SatelliteSystem::BeiDou)
        {
            // Compass B1   I/Q                 C1         L1         D1         S1
            //         E5b  I/Q                 C7         L7         D7         S7
            //         E6   I/Q                 C6         L6         D6         S6

            // For E2-B1
            //if(roi.band == ObsID::cbE1) return TypeID::Unknown;
            if (roi.band == CarrierBand::B1 || roi.band == CarrierBand::L2) // TD is cbB3 correct?
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C1;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            else if (roi.band == CarrierBand::B2)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C2;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L2;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D2;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S2;
            }
            else if (roi.band == CarrierBand::B3)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C6;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L6;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D6;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S6;
            }
        }
        else if (sat.system == SatelliteSystem::Geosync)
        {
            // SBAS    L1         1575.42      C1         L1         D1         S1
            //         L5         1176.45      C5         L5         D5         S5

            // L1
            if (roi.band == CarrierBand::L1)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C1;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            //L5
            else if (roi.band == CarrierBand::L5)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C5;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L5;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D5;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S5;
            }
        }
        else if (sat.system == SatelliteSystem::QZSS)
        {
            // For L1: C1 L1 D1 S1
            if (roi.band == CarrierBand::L1)
            {
                if (roi.type == ObservationType::Range && roi.code == TrackingCode::L1S)
                    return TypeID::C1;

                if (roi.type == ObservationType::Phase)
                    return TypeID::L1;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D1;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S1;
            }
            // For L2: C2 L2 D2 S2
            else if (roi.band == CarrierBand::L2)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C2;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L2;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D2;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S2;
            }
            // For L5: C5 L5 D5 S5
            else if (roi.band == CarrierBand::L5)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C5;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L5;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D5;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S5;
            }
            else if (roi.band == CarrierBand::E6)
            {
                if (roi.type == ObservationType::Range)
                    return TypeID::C6;
                if (roi.type == ObservationType::Phase)
                    return TypeID::L6;
                if (roi.type == ObservationType::Doppler)
                    return TypeID::D6;
                if (roi.type == ObservationType::SNR)
                    return TypeID::S6;
            }
        }

        return TypeID::Unknown;
    }

    // Return the new TypeID
    TypeID TypeID::regByName(std::string name, std::string desc)
    {

        std::map<std::string, TypeID>::iterator it = mapUserTypeID.find(name);

        if (it != mapUserTypeID.end())
        {
            return it->second;
        }
        else
        {
            TypeID newID = TypeID::newValueType(desc);

            mapUserTypeID.insert(std::pair<std::string, TypeID>(name, newID));

            return newID;
        }

    } // End of 'TypeID::registerTypeID(std::string name,std::string desc)'

    // unregister a TypeID by it's name string
    void TypeID::unregByName(std::string name)
    {
        std::map<std::string, TypeID>::iterator it = mapUserTypeID.find(name);

        if (it != mapUserTypeID.end())
        {
            TypeID delID = it->second;

            std::map<TypeID::ValueType, std::string>::iterator it2 = TypeID::tStrings.find(delID.type);
            if (it2 != TypeID::tStrings.end())
            {
                TypeID::tStrings.erase(it2);
            }

            mapUserTypeID.erase(it);
        }
        else
        {
            // the TypeID have not been registered
            // we do nothing
        }

    } // End of 'TypeID::unregisterTypeID(std::string name)'

    // unregister all TypeIDs registered by name string
    void TypeID::unregAll()
    {
        std::map<std::string, TypeID>::iterator it = mapUserTypeID.begin();

        for (it = mapUserTypeID.begin(); it != mapUserTypeID.end(); it++)
        {
            TypeID delID = it->second;

            std::map<TypeID::ValueType, std::string>::iterator it2 = TypeID::tStrings.find(delID.type);
            if (it2 != TypeID::tStrings.end())
            {
                TypeID::tStrings.erase(it2);
            }
        }
        mapUserTypeID.clear();

        bUserTypeIDRegistered = false;

    } // End of 'TypeID::unregisterAll()'

    // get the user registered TypeID by name string
    TypeID TypeID::byName(std::string name)
    {
        // registerMyTypeID();

        std::map<std::string, TypeID>::iterator it = mapUserTypeID.find(name);
        if (it != mapUserTypeID.end())
        {
            return it->second;
        }
        else
        {
            InvalidRequest e("There are no registered TypeID name as '" + name + "'.");
            GNSSTK_THROW(e);
        }
    } // End of 'TypeID TypeID::byName(std::string name)'

} // End of namespace gnsstk
