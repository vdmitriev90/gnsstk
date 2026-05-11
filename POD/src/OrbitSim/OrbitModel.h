#ifndef POD_ORBIT_MODEL_H
#define POD_ORBIT_MODEL_H

#include"ForceModelData.h"

#include"ForceList.h"
#include"EquationOfMotion.hpp"



namespace pod
{
    class OrbitModel : public gnsstk::EquationOfMotion
    {
    public:

        /// Default constructor
        OrbitModel();

        OrbitModel(const ForceModelData& fmc);

        /// Default destructor
        virtual ~OrbitModel()
        {
            forceList.clear();
        }

        virtual gnsstk::Vector<double> getDerivatives(const double&t, const gnsstk::Vector<double>& y);

        /// Restore the default setting
        OrbitModel& reset(const ForceModelData& fmc);

        /// set reference epoch
        OrbitModel& setRefEpoch(const gnsstk::Epoch & t)
        {
            t0 = t; 
            return (*this);
        }

        /// get reference epoch
        gnsstk::Epoch getRefEpoch() const
        {
            return t0;
        }

    protected:

        /// Reference epoch
        gnsstk::Epoch t0;

        /// Spacecraft object
        gnsstk::Spacecraft sc;

        /// Force Model List
        ForceList forceList;

    };
}
#endif // !POD_ORBIT_MODEL_H
