#ifndef POD_GNSSDATA_STORE_H
#define POD_GNSSDATA_STORE_H

#include "ApprPosProvider.hpp"
#include "CommonEnums.h"
#include "CommonTime.hpp"
#include "ComputeIonoModel.hpp"
#include "ConfDataReader.hpp"
#include "CorrectCodeBiases.hpp"
#include "EOPStore.hpp"
#include "IonexStore.hpp"
#include "IonoModelStore.hpp"
#include "NavLibrary.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace pod
{
    typedef gnsstk::ComputeIonoModel::IonoModelType IonoModelType;

    bool isDifferential(SlnType slnType);

    extern std::map<SlnType, std::string> slnType2Str;
    extern std::map<CarrierBand, std::string> carrierBand2Str;

    // class to store processing configuration and input data
    struct GnssDataStore
    {
        // GnssDataStore static data initializer
        class Initializer
        {
          public:
            Initializer();
        };
        static Initializer GnssDataInitializer;

#pragma region Constructors
      public:
        GnssDataStore() = delete;
        GnssDataStore(gnsstk::ConfDataReader& confReader);
        ~GnssDataStore() {}

#pragma endregion

#pragma region Methods

      public:
        void checkObservable();
        void LoadData(const char* path);
        std::list<std::string> getObsFiles(const std::string& siteID) const;
        gnsstk::Position getNominalPosition(std::string siteId);

      private:
        bool initReader(const char* path);
        bool loadIono();
        bool loadBceIonoModel();
        bool loadIonoMap();
        bool loadEphemeris();
        bool loadFcn();
        bool loadClocks();
        bool loadEOPData();
        bool loadCodeBiases();
        bool createPosProvider();

#pragma endregion

#pragma region Fields

      public:
        // pointer to  configuration file reader
        gnsstk::ConfDataReader* confReader;

        gnsstk::NavDataFactoryPtr sp3NavFactory_;

        // object to handle precise ephemeris and clocks
        gnsstk::NavLibrary navLibrary_;

        // Earth orintation parameters store
        gnsstk::EOPStore eopStore;

        // GPS Navigation Message based ionospheric models store
        gnsstk::IonoModelStore bceIonoStore;

        // ionosphere map store
        gnsstk::IonexStore ionexStore;

        // compute the  values related to a given GNSS ionospheric model.
        gnsstk::ComputeIonoModel ionoCorrector;

        // path to approximate position and code clock bias file
        std::string apprPosFile;

        // store of  approximate position and code clock bias
        // std::map<gnsstk::CommonTime, gnsstk::Xvt, std::less<gnsstk::CommonTime>> apprPos;
        PosProviderUPtr apprPos;

        // class to corrects observables from differential code biases
        gnsstk::CorrectCodeBiases DCBData;

        // pocessing-spacific options
        struct ProcessOpts
        {
            // rover receiver site ID
            std::string SiteRover;

            // base receiver site ID
            std::string SiteBase;

            // directory with generic files
            std::string genericFilesDirectory;

            // working directory
            std::filesystem::path workingDir;

            // Broadcast ephemeris directory
            std::string bceDir;

            // is output SV-depended data?
            bool fullOutput = false;

            // Estimate zenith troposperic delay?
            bool computeTropo = false;

            // Compute tropospheric delay gradient
            TropoModelType tropoModelType = TropoModelType::Simple;

            // Is data relate to the spacecraft-based receiver?
            bool isSpaceborneRcv = false;

            // Day of year. Used for tropospheric model object  initialization
            int DoY = 0;

            // Satellite systems used for position computation
            gnsstk::SatSystSet systems;

            // Satellite systems used for position computation
            std::set<CarrierBand> carrierBands;

            // S1 (L1 C/No) threshold for position computation
            int maskSNR = 0;

            // Elevation mask (degrees) for position computation
            double maskEl = 10;

            // Receiver dynamic mode
            Dynamics dynamics = Dynamics::Kinematic;

            // Desired type of GNSS solution
            SlnType slnType = SlnType::Standalone;

            // Use carrier-smoothing of code pseudoranges
            bool isSmoothCode = false;

        } opts;

#pragma endregion
    };

    typedef std::shared_ptr<pod::GnssDataStore> GnssDataStorePtr;
} // namespace pod
#endif // !POD_GNSSDATA_STORE_H
