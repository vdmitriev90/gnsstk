#ifndef POD_PROJECT_CONFIG_H
#define POD_PROJECT_CONFIG_H

#include "ApprPosProvider.hpp"
#include "CommonEnums.h"
#include "CommonTime.hpp"
#include "SatID.hpp"
#include "Triple.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace pod::config
{
	class ConfigError : public std::runtime_error
	{
	  public:
		using std::runtime_error::runtime_error;
	};

	struct StationConfig
	{
		gnsstk::Triple nominalPositionEcefM;
		gnsstk::Triple markerToArpUenM;
		std::string antennaModel;
		bool usePcPatterns{true};
		bool useAzimuth{false};
	};

	struct GenericFilesConfig
	{
		std::filesystem::path directory;
		std::filesystem::path satelliteDataFile;
		std::filesystem::path antexFile;
		std::filesystem::path oceanLoadingFile;

		// full paths: directory / file
		std::string antexFilePath() const { return (directory / antexFile).string(); }
		std::string oceanLoadingFilePath() const { return (directory / oceanLoadingFile).string(); }
		std::string satelliteDataFilePath() const { return (directory / satelliteDataFile).string(); }
	};

	struct ProcessingConfig
	{
		SlnType solutionType{SlnType::Standalone};
		bool applyCodeBias{false};
		bool useRinexClock{false};
		bool fullOutput{false};
		gnsstk::SatSystSet satelliteSystems;
		std::set<CarrierBand> carrierBands;

		bool computeTroposphere{false};
		double troposphereVerticalProcessNoise{0};
		TropoModelType troposphereModelType{TropoModelType::Simple};
		double troposphereQ1{0};
		double troposphereQ2{0};

		bool useAdvancedClockModel{false};
		double clockQ1{0};
		double clockQ2{0};

		bool computeIonosphere{false};
		int ionosphereModelType{0};
		double ionosphereSigmaM{0};
		int codeIonosphereCorrectionType{0};
		bool smoothCode{false};
		bool spaceborneReceiver{false};

		Dynamics dynamics{Dynamics::Kinematic};
		double elevationMaskDeg{0};
		double snrMaskDbHz{0};

		int forwardBackwardCycles{0};
		std::vector<double> phaseLimits;
		std::vector<double> codeLimits;
		double positionSigmaM{0};
		double clockSigmaM{0};
		double weightFactor{1};
		double lossOfLockThreshold{0};
		double MelbourneWubbenaThreshold{0};

		int decimationIntervalSeconds{0};
		double decimationToleranceSeconds{0};

		ApprPositionSource approximatePositionProvider{ApprPositionSource::FromRinexHeader};
		std::filesystem::path approximatePositionFile;
	};

	struct ProjectConfig
	{
		std::string siteRover;
		std::string siteBase;
		GenericFilesConfig genericFiles;
		ProcessingConfig processing;
		std::map<std::string, StationConfig> stations;

		const StationConfig& station(const std::string& siteId) const;
		const StationConfig* findStation(const std::string& siteId) const;

		// Parses a TOML configuration file. Throws ConfigError on any missing or invalid value.
		static ProjectConfig load(const std::filesystem::path& file);
	};
} // namespace pod::config

#endif // !POD_PROJECT_CONFIG_H
