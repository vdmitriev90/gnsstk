#pragma once
#include"DataStructures.hpp"
#include"FilterParameter.h"

namespace pod
{
    ///select satellites with maximum elevation angle for each satellite system
    class RefSatHandler
    {
    public:
         RefSatHandler() {};
        ~RefSatHandler() {};
        
        // return satellites set, contains satellites with maximum elevation for each satellite system
        gnsstk::SatIDSet  getRefSats(gnsstk::IRinex & gRin);
        
        //return DD to SD transition matrix
        gnsstk::Matrix<double> getSD2DDMatrix(
            gnsstk::IRinex & gData,
            const  gnsstk::SatIDSet& svs,
            const  gnsstk::SatSystSet& ss,
            gnsstk::SatIDSet &refSvSet);
    };
}

