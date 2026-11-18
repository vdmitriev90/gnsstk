#ifndef POD_GNSSDATA_STORE_H
#define POD_GNSSDATA_STORE_H

#include "ApprPosProvider.hpp"
#include "BiasSinexStore.hpp"
#include "CommonEnums.h"
#include "CommonTime.hpp"
#include "ComputeIonoModel.hpp"
#include "CorrectCodeBiases.hpp"
#include "EOPStore.hpp"
#include "IonexStore.hpp"
#include "IonoModelStore.hpp"
#include "NavLibrary.hpp"
#include "ProjectConfig.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace pod
{
    typedef gnsstk::ComputeIonoModel::IonoModelType IonoModelType;

    bool isDifferential(SlnType slnType);

    const char* slnTypeToStr(SlnType slnType);
    const char* carrierBandToStr(CarrierBand band);

    // class to store processing configuration and input data
    struct GnssDataStore
    {
        public:
        GnssDataStore();

      public:
        void checkObservable();
        void LoadData(const char* path);
        std::list<std::string> getObsFiles(const std::string& siteID) const;
        gnsstk::Position getNominalPosition(std::string siteId);

        const config::ProjectConfig& config() const
        {
            return config_;
        }

      private:
        bool loadIono();
        bool loadBceIonoModel();
        bool loadIonoMap();
        bool loadEphemeris();
        bool loadFcn();
        bool loadClocks();
        bool loadEOPData();
        bool loadBiasSinex();
        bool createPosProvider();

      private:
        // strongly typed project configuration loaded from the TOML file
        config::ProjectConfig config_;

      public:
        gnsstk::NavDataFactoryPtr sp3NavFactory_;

        // object to handle precise ephemeris and clocks
        gnsstk::NavLibrary navLibrary_;

        // Earth orintation parameters store
        gnsstk::EOPStore eopStore_;

        // GPS Navigation Message based ionospheric models store
        gnsstk::IonoModelStore bceIonoStore_;

        // ionosphere map store
        gnsstk::IonexStore ionexStore_;

        // path to approximate position and code clock bias file
        std::string apprPosFile_;

        // store of  approximate position and code clock bias
        // std::map<gnsstk::CommonTime, gnsstk::Xvt, std::less<gnsstk::CommonTime>> apprPos;
        PosProviderUPtr apprPosProvider_;

        // Bias-SINEX store for satellite/station bias records
        pod::BiasSinexStore biasSinexStore_;

        // working directory
        std::filesystem::path workingDir;

        // Broadcast ephemeris directory
        std::string bceDir;

        // Day of year. Used for tropospheric model object  initialization
        int DoY = 0;
    };

    typedef std::shared_ptr<pod::GnssDataStore> GnssDataStorePtr;
} // namespace pod
#endif // !POD_GNSSDATA_STORE_H
