#ifndef POD_CENTRAL_MASS_MODEL_H
#define POD_CENTRAL_MASS_MODEL_H

#include "GravityModel.h"

namespace pod
{
    class CentralMassModel : public GravityModel
    {
      public:
        CentralMassModel(const GravityModelData& gMData);

        void doCompute(gnsstk::Epoch time, gnsstk::Spacecraft& sc);

        virtual void test() {};
    };
} // namespace pod

#endif //! POD_CENTRAL_MASS_MODEL_H