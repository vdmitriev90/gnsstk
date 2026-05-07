#pragma once

#include"ProcessingClass.hpp"

namespace pod
{
	//Class to mark which satellites is treated as probably useable. 
	//This is doesn't mean that such satellites exactly were used in navigation solution.
	//This flag mean, that such satellites are ponentially can be used 
	//in solution during reprocessing (while forward - backward processing).
	//Only non-zero residuals are indicates that  particular satellite has been used in pvt computation.
	class UsedInPvtMarker: public gnsstk::ProcessingClass
	{


	public:
		UsedInPvtMarker(gnsstk::TypeID tp) :type(tp) {};
		UsedInPvtMarker() :type(gnsstk::TypeID::satStatus){};
		
		~UsedInPvtMarker() {};

		// mark all SV in IRinex as useable in PVT  
		gnsstk::IRinex& Process(gnsstk::IRinex& gRin) override;
		
		// mark all SV in SatTypePtrMap as useable in PVT  
		gnsstk::SatTypePtrMap& markAsUsed(gnsstk::SatTypePtrMap& satData) const;
		
		// keep only Satellites marked as useable (TypeID::satStatus)
		gnsstk::SatTypePtrMap& keepOnlyUsed(gnsstk::SatTypePtrMap& satData) const;
		
		//
		gnsstk::SatTypePtrMap& CleanScFlags(gnsstk::SatTypePtrMap& satData) const;

		gnsstk::SatTypePtrMap& CleanSatArcFlags(gnsstk::SatTypePtrMap& satData) const;

		UsedInPvtMarker& updateLastEpoch(const gnsstk::IRinex& gRin);

		virtual std::string getClassName(void) const
		{
			return "UsedInPvtMarker";
		}

	private:
		gnsstk::TypeID type;
		
		//last epoch time
		gnsstk::CommonTime preEpoch;
		//sats in view on last epoch 
		gnsstk::SatIDSet preEpochSats;
	};

}