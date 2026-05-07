#ifndef POD_EARTH_ROTATION_H
#define POD_EARTH_ROTATION_H

#include"Matrix.hpp"
#include"CivilTime.hpp"
#include"EOPStore.hpp"

using namespace gnsstk;

namespace pod
{
    class EarthRotation
	{
    public:
  
        static EarthRotation& eopStore()
        {
            static EarthRotation instance;
            return instance;
        }

        bool loadEOP(const std::string& fileName);
        /// get Inertial to terestriel matrix by IAU 2000A, CIO based, using classical angles
        Matrix<double> getJ2k2Ecef00(const CommonTime & t);
        Matrix<double> getEcef2J2k00(const CommonTime & t);
        Vector<double> convertJ2k2Ecef(const CommonTime & t, const Vector<double> pos);
        Vector<double> convertEcef2J2k(const CommonTime & t, const Vector<double> pos);

        virtual ~EarthRotation()
        {
            eopData_.clear();
        }

        bool test();

    private:
        EarthRotation() = default;
        EarthRotation(const EOPStore& eop);

        EOPStore eopData_;
    };
}
#endif // !POD_EARTH_ROTATION_H