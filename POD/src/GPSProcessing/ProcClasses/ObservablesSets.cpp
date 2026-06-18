#include "ObservablesSets.h"

using namespace gnsstk;

namespace pod
{
    namespace obs_sets
    {
    
    /// Predefined set of dual-frequency observables for GPS/GLONASS (L1+L2).
    inline static const TypeIDSet dualFreqGpsGloTypes{TypeID::C1,
                                                      TypeID::P2,
                                                      TypeID::L1,
                                                      TypeID::L2,
                                                      TypeID::LLI1,
                                                      TypeID::LLI2,
                                                      TypeID::S1};

    /// Predefined set of dual-frequency observables for Galileo (E1+E5).
    inline static const TypeIDSet dualFreqGalileoTypes{TypeID::C1,
                                                       TypeID::C5,
                                                       TypeID::L1,
                                                       TypeID::L5,
                                                       TypeID::LLI1,
                                                       TypeID::LLI5,
                                                       TypeID::S1};

    ///// Predefined set of dual-frequency observables for BeiDou (B1+B2).
    //inline static const TypeIDSet dualFreqBeidouTypes{TypeID::C1,
    //                                                  TypeID::C7,
    //                                                  TypeID::L1,
    //                                                  TypeID::L7,
    //                                                  TypeID::LLI1,
    //                                                  TypeID::LLI7,
    //                                                  TypeID::S1};

	}

	RequireObservablesBuilder::RequireObservablesBuilder(const SatSystSet& systems, bool useC1,
													   bool keepOnlyRequiredTypes)
		: systems_(systems), useGpsGloC1_(useC1), keepOnlyRequiredTypes_(keepOnlyRequiredTypes)
	{}

	RequireObservables RequireObservablesBuilder::build() const
	{
		RequireObservables ro(keepOnlyRequiredTypes_);
		for (SatelliteSystem sys : systems_)
		{
			if (sys == SatelliteSystem::GPS || sys == SatelliteSystem::Glonass)
			{
                ro.addRequiredType(sys, obs_sets::dualFreqGpsGloTypes);
                if (!useGpsGloC1_)
					ro.addRequiredType(sys, TypeID::P1);
			}
			else if (sys == SatelliteSystem::Galileo)
			{
                ro.addRequiredType(sys, obs_sets::dualFreqGalileoTypes);
			}
			else if (sys == SatelliteSystem::BeiDou)
			{
				//ro.addRequiredType(sys, obs_sets::dualFreqBeidouTypes);
			}
		}
		return ro;
	}

} // namespace pod
