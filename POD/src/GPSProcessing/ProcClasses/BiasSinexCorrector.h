#pragma once

#include "BiasSinex/BiasSinexStore.hpp"
#include "GNSSconstants.hpp"
#include "ProcessingClass.hpp"

#include <map>
#include <string>

namespace pod
{
	class BiasSinexCorrector : public gnsstk::ProcessingClass
	{
	  public:
		using TypeMap = std::map<gnsstk::TypeID, std::string>;

		explicit BiasSinexCorrector(const BiasSinexStore& store);

		BiasSinexCorrector& clearMappings();
		BiasSinexCorrector& setDefaultCodeMappings();
		BiasSinexCorrector& mapCodeObservable(const gnsstk::SatelliteSystem& system,
											 const gnsstk::TypeID& type,
											 const std::string& obsCode);

		gnsstk::SatTypePtrMap& Process(const gnsstk::CommonTime& epoch,
									   gnsstk::SatTypePtrMap& gData);

		gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

		std::string getClassName() const override;

	  private:
		const BiasSinexStore* store_;
		std::map<gnsstk::SatelliteSystem, TypeMap> codeMappings_;
	};
} // namespace pod
