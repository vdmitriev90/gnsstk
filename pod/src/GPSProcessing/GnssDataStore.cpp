#include "GnssDataStore.hpp"

#include "CodeProcSvData.h"
#include "FsUtils.h"
#include "LinearCombination.h"
#include "Rinex3NavStream.hpp"
#include "SP3NavDataFactory.hpp"

using namespace gnsstk;

namespace
{
    std::optional<CommonTime> parseAiubRefTime(const Rinex3NavHeader& rNavHeader)
    {
        for (auto&& it : rNavHeader.commentList)
        {
            int doy = -1, yr = -1;
            std::cmatch res;
            std::regex rxDoY("DAY [0-9]{3}"), rxY(" [0-9]{4}");
            if (std::regex_search(it.c_str(), res, rxDoY))
            {
                std::string sDay = res[0];
                sDay = sDay.substr(sDay.size() - 4, 4);
                doy = stoi(sDay);
            }
            if (std::regex_search(it.c_str(), res, rxY))
            {
                std::string sDay = res[0];
                sDay = sDay.substr(sDay.size() - 5, 5);
                yr = stoi(sDay);
            }
            if (doy > 0 && yr > 0)
                return YDSTime(yr, doy, 0, TimeSystem::GPS);
        }
        return std::nullopt;
    }

    std::optional<CommonTime> parseRinexHeaderDate(const std::string& date)
    {
        if (date.empty())
            return std::nullopt;

        static const std::map<std::string, int> monthMap = {{"JAN", 1},
                                                            {"FEB", 2},
                                                            {"MAR", 3},
                                                            {"APR", 4},
                                                            {"MAY", 5},
                                                            {"JUN", 6},
                                                            {"JUL", 7},
                                                            {"AUG", 8},
                                                            {"SEP", 9},
                                                            {"OCT", 10},
                                                            {"NOV", 11},
                                                            {"DEC", 12}};

        int day = 0, yr = 0, hr = 0, mn = 0;
        char monStr[4] = {};
        if (sscanf(date.c_str(), "%d-%3s-%d %d:%d", &day, monStr, &yr, &hr, &mn) != 5)
            return std::nullopt;

        auto it = monthMap.find(std::string(monStr));
        if (it == monthMap.end())
            return std::nullopt;

        yr += (yr < 80) ? 2000 : 1900;
        CivilTime ct(yr, it->second, day, hr, mn, 0.0, TimeSystem::GPS);

        // Data is obtained from a joint BRDC file generated the day after actual data collection,
        // so the timestamp must be shifted back by one day.
        return ct.convertToCommonTime().addDays(-1);
    }
    std::optional<CommonTime> resolveNavHeaderRefTime(const Rinex3NavHeader& rNavHeader)
    {
        if (rNavHeader.fileAgency == "AIUB")
            if (auto time_rnx = parseAiubRefTime(rNavHeader))
                return time_rnx;

        if (auto time_rnx = parseRinexHeaderDate(rNavHeader.date))
            return time_rnx;

        const auto it = rNavHeader.mapTimeCorr.find("GPUT");
        if (it != rNavHeader.mapTimeCorr.end() && it->second.refTime != CommonTime::BEGINNING_OF_TIME)
            return it->second.refTime;

        return std::nullopt;
    }
} // namespace

namespace pod
{

    bool isDifferential(SlnType slnType)
    {
        if (slnType == SlnType::CodeDiff || slnType == SlnType::PdFloat || slnType == SlnType::PdFixed)
            return true;
        return false;
    }

    namespace fs = std::filesystem;

    std::map<SlnType, std::string> pod::slnType2Str;
    std::map<CarrierBand, std::string> pod::carrierBand2Str;
    GnssDataStore::Initializer::Initializer()
    {
        slnType2Str[SlnType::Standalone] = "Standalone";
        slnType2Str[SlnType::CodeDiff] = "Code_Diff";
        slnType2Str[SlnType::PdFloat] = "PD_Float";
        slnType2Str[SlnType::PdFixed] = "PD_Fixed";
        slnType2Str[SlnType::PppFloat] = "PPP_Float";
        slnType2Str[SlnType::PppFixed] = "PPP_Fixed";
        slnType2Str[SlnType::None] = "None";

        carrierBand2Str[CarrierBand::L1] = "L1";
        carrierBand2Str[CarrierBand::L2] = "L2";
        carrierBand2Str[CarrierBand::L5] = "L5";
    }

    GnssDataStore::Initializer GnssDataStore::GnssDataInitializer;

    void GnssDataStore::LoadData(const char* path)
    {
        try
        {
            config_ = config::ProjectConfig::load(path);
            const auto& proc = config_.processing;

            workingDir = fs::path(path).parent_path();

            bceDir = "data/nav";

            std::cout << "Solution Type: " << slnType2Str[proc.solutionType] << std::endl;

            std::cout << "Used Carrier bands: ";
            for (auto band : proc.carrierBands)
                std::cout << carrierBand2Str[band] << " ";
            std::cout << std::endl;

            std::cout << "Used Sat. Systems: ";
            for (auto ss : proc.satelliteSystems)
                std::cout << convertSatelliteSystemToCode(ss) << " ";
            std::cout << std::endl;

            const auto checkLoaded = [](bool ok, const char* what) {
                std::cout << ok << std::endl;
                if (!ok)
                {
                    std::cerr << "Failed to load " << what << ". Processing aborted." << std::endl;
                    exit(-1);
                }
            };

            std::cout << "Ephemeris Loading... ";
            checkLoaded(loadEphemeris(), "ephemeris");

            // load clock data from RINEX clk files, if required
            if (proc.useRinexClock)
            {
                std::cout << "Load Rinex clock data ... ";
                checkLoaded(loadClocks(), "Rinex clock data");
            }

            std::cout << "Load ionospheric data ... ";
            checkLoaded(loadIono(), "ionospheric data");

            std::cout << "Load Glonass FCN data... ";
            checkLoaded(loadFcn(), "Glonass FCN data");

            std::cout << "Load Earth orientation data... ";
            checkLoaded(loadEOPData(), "Earth orientation data");

            if (proc.applyCodeBias)
            {
                std::cout << "Load Bias-SINEX data... ";
                checkLoaded(loadBiasSinex(), "Bias-SINEX data");
            }

            std::cout << "Appr. position  source: ";
            if (createPosProvider())
                std::cout << getPosSourceString(apprPosProvider_->getSource()) << std::endl;
        }
        catch (const Exception& e)
        {
            std::cout << "Failed to load input data. An error has occured: " << e.what() << std::endl;
            exit(-1);
        }
        catch (const std::exception& e)
        {
            std::cout << "Failed to load input data: An error has occured: " << e.what() << std::endl;
            exit(-1);
        }
    }

    //
    bool GnssDataStore::loadEphemeris()
    {

        const std::string subdir = "data/sp3";
        const auto files = FsUtils::getAllFilesInDir(workingDir / subdir);
        if (files.empty())
        {
            std::cerr << "Empty ephemeris directory " << workingDir / subdir << std::endl;
            return false;
        }
        bool res = false;
        for (const auto& file : files)
        {
            // Try to load each ephemeris file
            try
            {
                if (sp3NavFactory_->addDataSource(file.string()))
                    res = true;
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                std::cerr << "SP3 file '" << file << "' doesn't exist or you don't "
                          << "have permission to read it. Skipping it." << std::endl;
                continue;
            }
        }
        return res;
    }

    // reading clock data
    bool GnssDataStore::loadClocks()
    {
        const std::string subdir = "data/clk";
        const auto files = FsUtils::getAllFilesInDir(workingDir / subdir);

        bool res = false;

        for (const auto& file : files)
        {
            try
            {
                res = sp3NavFactory_->addDataSource(file.string());
            }
            catch (Exception& e)
            {
                res = false;
            }
            if (!res)
            {
                std::cerr << "Failed to load Rinex clock file '" << file << "'." << std::endl;
                return false;
            }
        }
        return res;
    }

    bool GnssDataStore::loadIono()
    {
        bool isIonexLoaded = loadIonoMap();

        const auto type =
            static_cast<ComputeIonoModel::IonoModelType>(config_.processing.codeIonosphereCorrectionType);
        switch (type)
        {
        case gnsstk::ComputeIonoModel::Zero:
            ionoCorrector_.setZeroModel();
            break;
        case gnsstk::ComputeIonoModel::Klobuchar:
            if (!loadBceIonoModel())
                GNSSTK_THROW(InvalidRequest("Can't load iono model from Rinex GPS Navigation files."));
            break;
        case gnsstk::ComputeIonoModel::Ionex:
            if (isIonexLoaded)
                ionoCorrector_.setIonosphereMap(ionexStore_);
            else
                GNSSTK_THROW(InvalidRequest("Can't load Ionosphere map from Ionex files."));
            break;
        case gnsstk::ComputeIonoModel::DualFreq:
            ionoCorrector_.setDualFreqModel();
            break;
        default:
            GNSSTK_THROW(InvalidRequest("Unknown Ionospheric model type."));
        }
        return true;
    }

    bool GnssDataStore::loadIonoMap()
    {
        const std::string subdir = "data/inx";
        const auto files = FsUtils::getAllFilesInDir(workingDir / subdir);
        ionexStore_.clear();
        for (const auto& file : files)
        {
            ionexStore_.loadFile(file.string());
        }

        return ionexStore_.size() > 0;
    }

    bool GnssDataStore::loadBceIonoModel()
    {
        const std::string gpsObsExt = ".[\\d]{2}[nN]|";
        const auto files = FsUtils::getFilesByExtensionRegex(workingDir / bceDir, gpsObsExt);
        for (auto&& file : files)
        {
            try
            {
                IonoModel iMod;
                Rinex3NavStream rNavFile;
                Rinex3NavHeader rNavHeader;

                rNavFile.open(file.string().c_str(), std::ios::in);
                rNavFile >> rNavHeader;

#pragma region try get the date

                const auto nav_ref_time = resolveNavHeaderRefTime(rNavHeader);
                if (!nav_ref_time)
                    continue;
#pragma endregion

                if (rNavHeader.valid & Rinex3NavHeader::validIonoCorrGPS)
                {
                    // Extract the Alpha and Beta parameters from the header
                    double* ionAlpha = rNavHeader.mapIonoCorr["GPSA"].param;
                    double* ionBeta = rNavHeader.mapIonoCorr["GPSB"].param;

                    // Feed the ionospheric model with the parameters
                    iMod.setModel(ionAlpha, ionBeta);
                }
                else
                {
                    std::cerr << "WARNING: Navigation file " << file
                              << " doesn't have valid ionospheric correction parameters." << std::endl;
                }

                bceIonoStore_.addIonoModel(nav_ref_time.value(), iMod);
            }
            catch (...)
            {
                std::cerr << "Problem opening file " << file << std::endl;
                std::cerr << "Maybe it doesn't exist or you don't have proper read "
                          << "permissions." << std::endl;
                return false;
            }
        }

        if (bceIonoStore_.empty())
        {
            std::cerr << "No valid ionospheric model found in Rinex GPS Navigation files." << std::endl;
            return false;
        }
        //
        ionoCorrector_.setKlobucharModel(bceIonoStore_);

        return true;
    }

    bool GnssDataStore::loadFcn()
    {
        const std::string gln_nav_ext = ".[\\d]{2}[gG]|\\.rnx";
        auto files = FsUtils::getFilesByExtensionRegex(workingDir / bceDir, gln_nav_ext);

        for (auto file : files)
        {
            try
            {
                SatID::loadGloFcn(file.string().c_str());
            }
            catch (...)
            {
                std::cerr << "Problem opening file " << file << std::endl;
                std::cerr << "Maybe it doesn't exist or you don't have proper read "
                          << "permissions." << std::endl;
                return false;
            }
        }
        return SatID::glonassFcn.size() > 0;
    }

    bool GnssDataStore::loadEOPData()
    {

        const fs::path eop_dir = workingDir / "data" / "erp";

        const auto files = FsUtils::getFilesByExtension(eop_dir, ".ERP");

        if (files.empty())
        {
            std::cerr << "Empty ERP directory " << eop_dir << std::endl;
            return false;
        }

        for (const auto& file : files)
        {
            eopStore_.addERPFile(file.string());
        }

        if (eopStore_.empty())
            std::cerr << "Empty ERP store after import " << eop_dir << std::endl;
        return !eopStore_.empty();
    }

    bool GnssDataStore::loadBiasSinex()
    {
        const fs::path bias_dir = workingDir / "data" / "osb";
        const auto files = FsUtils::getAllFilesInDir(bias_dir);

        if (files.empty())
        {
            std::cerr << "Empty OSB directory " << bias_dir << std::endl;
            return false;
        }

        for (const auto& bsxFile : files)
        {
            try
            {
                biasSinexStore_.loadFile(bsxFile.string());
            }
            catch (const Exception& e)
            {
                std::cerr << "Failed to load Bias-SINEX file '" << bsxFile << "': " << e.what() << std::endl;
                return false;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to load Bias-SINEX file '" << bsxFile << "': " << e.what() << std::endl;
                return false;
            }
        }
        return true;
    }

    GnssDataStore::GnssDataStore()
    {
        sp3NavFactory_ = std::make_shared<SP3NavDataFactory>();
        navLibrary_.addFactory(sp3NavFactory_);
    }

    void GnssDataStore::checkObservable()
    {
        std::ofstream os((workingDir / "ObsStatistic.out").string());

        for (auto obsFile : getObsFiles(config_.siteRover))
        {
            // Input observation file stream
            Rinex3ObsStream rin;
            // Open Rinex observations file in read-only mode
            rin.open(obsFile, std::ios::in);

            rin.exceptions(std::ios::failbit);
            Rinex3ObsHeader roh;
            Rinex3ObsData rod;

            // read the header
            rin >> roh;

            while (rin >> rod)
            {
                if (rod.epochFlag == 0 || rod.epochFlag == 1) // Begin usable data
                {
                    int NumP1(0), NumP2(0), NumBadCNo1(0);
                    os << std::setprecision(12) << (CivilTime)rod.time << " ";
                    int nGPS = 0, nGLN = 0;
                    for (auto& it : rod.obs)
                    {
                        if (config_.processing.satelliteSystems.find(it.first.system) == config_.processing.satelliteSystems.end())
                            continue;
                        if (it.first.system == SatelliteSystem::GPS)
                            nGPS++;
                        if (it.first.system == SatelliteSystem::Glonass)
                            nGLN++;

                        auto& ids = CodeProcSvData::obsTypes[it.first.system];

                        double CNoL1 = rod.getObs(it.first, ids[TypeID::S1], roh).data;
                        if (CNoL1 < 30)
                            NumBadCNo1++;

                        double P1 = rod.getObs(it.first, ids[TypeID::P1], roh).data;
                        if (P1 > 0.0)
                            NumP1++;

                        double P2 = rod.getObs(it.first, ids[TypeID::P2], roh).data;
                        if (P2 > 0.0)
                            NumP2++;
                    }

                    os << nGPS << " " << nGLN << " " << NumBadCNo1 << " " << NumP1 << " " << NumP2 << std::endl;
                }
            }
        }
    }

    bool GnssDataStore::createPosProvider()
    {
        const auto apprPosProvider = static_cast<ApprPositionSource>(config_.processing.approximatePositionProvider);

        switch (apprPosProvider)
        {
        case ApprPositionSource::FromConfig:
            apprPosProvider_ = std::make_unique<ApprPosSimple>(getNominalPosition(config_.siteRover));
            return true;
        case ApprPositionSource::ComputeForEachEpoch:
            apprPosProvider_ = std::make_unique<ComputeOnePos>(navLibrary_);
            return true;
        case ApprPositionSource::ComputeForFirstEpoch:
            apprPosProvider_ = std::make_unique<ComputeOnePos>(navLibrary_);
            return true;
        case ApprPositionSource::LoadFromFile:
            apprPosProvider_ = std::make_unique<PositionFromFile>(
                (workingDir / config_.processing.approximatePositionFile).string());
            return true;
        default:
            return false;
        }
    }

    gnsstk::Position GnssDataStore::getNominalPosition(std::string siteId)
    {
        return Position(config_.station(siteId).nominalPositionEcefM);
    }

    std::list<std::string> GnssDataStore::getObsFiles(const std::string& siteID) const
    {
        const std::string subdir = "data/obs";
        auto paths = FsUtils::getAllFilesInDir(workingDir / subdir / siteID);
        std::list<std::string> result;
        for (const auto& p : paths)
            result.push_back(p.string());
        return result;
    }

} // namespace pod
