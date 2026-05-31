#include "GnssDataStore.hpp"

#include "CodeProcSvData.h"
#include "FsUtils.h"
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
        if (it != rNavHeader.mapTimeCorr.end()
            && it->second.refTime != CommonTime::BEGINNING_OF_TIME)
            return it->second.refTime;

        return std::nullopt;
    }
} // namespace

namespace pod
{
    namespace fs = std::filesystem;

    std::map<SlnType, std::string> pod::slnType2Str;
    std::map<CarrierBand, std::string> pod::carrierBand2Str;
    GnssDataStore::Initializer::Initializer()
    {
        slnType2Str[SlnType::Standalone] = "Standalone";
        slnType2Str[SlnType::CODE_DIFF] = "CODE_DIFF";
        slnType2Str[SlnType::PD_Float] = "PD_Float";
        slnType2Str[SlnType::PD_Fixed] = "PD_Fixed";
        slnType2Str[SlnType::PPP_Float] = "PPP_Float";
        slnType2Str[SlnType::PPP_Fixed] = "PPP_Fixed";
        slnType2Str[SlnType::NONE_SOLUTION] = "NONE_SOLUTION";

        carrierBand2Str[CarrierBand::L1] = "L1";
        carrierBand2Str[CarrierBand::L2] = "L2";
        carrierBand2Str[CarrierBand::L5] = "L5";
    }

    GnssDataStore::Initializer GnssDataStore::GnssDataInitializer;

    bool GnssDataStore::initReader(const char* path)
    {
        try
        {
            // Try to open the provided configuration file
            confReader->open(path);
        }
        catch (...)
        {
            std::cerr << "Problem opening file " << path << std::endl;
            std::cerr << "Maybe it doesn't exist or you don't have proper "
                      << "read permissions." << std::endl;

            exit(-1);

        } // End of 'try-catch' block

        // If a given variable is not found in the provided section, then
        // 'confReader' will look for it in the 'DEFAULT' section.
        confReader->setFallback2Default(true);

        return true;
    }

    void GnssDataStore::LoadData(const char* path)
    {
        try
        {
            initReader(path);

            opts.workingDir = fs::path(path).parent_path();

            opts.isSpaceborneRcv = confReader->getValueAsBoolean("IsSpaceborneRcv");

            opts.isSmoothCode = confReader->getValueAsBoolean("IsSmoothCode");

            opts.computeTropo = confReader->getValueAsBoolean("computeTropo");

            opts.tropoModelType = (TropoModelType)confReader->getValueAsInt("tropoModelType");

            opts.maskEl = confReader->getValueAsDouble("ElMask");

            opts.maskSNR = confReader->getValueAsDouble("SNRmask");

            opts.dynamics = (Dynamics)confReader->getValueAsInt("Dynamics");

            opts.bceDir = confReader->getValue("RinexNavFilesDir");

            opts.SiteRover = confReader->getValue("SiteRover");

            opts.SiteBase = confReader->getValue("SiteBase");

            opts.fullOutput = confReader->getValueAsBoolean("fullOutput");

            opts.slnType = (SlnType)confReader->getValueAsInt("slnType");
            std::cout << "Solution Type: " << slnType2Str[opts.slnType] << std::endl;

            // set generic files direcory
            std::string subdir = confReader->getValue("GenericFilesDir");
            opts.genericFilesDirectory = (opts.workingDir / subdir).string() + "\\";

            for (auto it : confReader->getValueListAsInt("carrierBands"))
                opts.carrierBands.insert(static_cast<CarrierBand>(it));

            std::cout << "Used Carrier bands: ";
            for_each(opts.carrierBands.begin(), opts.carrierBands.end(), [](auto&& it) {
                std::cout << carrierBand2Str[it] << " ";
            });
            std::cout << std::endl;

            for (auto it : confReader->getValueListAsInt("satSystems"))
                opts.systems.insert(static_cast<SatelliteSystem>(it));

            std::cout << "Used Sat. Systems: ";
            for_each(opts.systems.begin(), opts.systems.end(), [](auto&& ss) {
                std::cout << convertSatelliteSystemToString(ss) << " ";
            });
            std::cout << std::endl;

            std::cout << "Ephemeris Loading... ";
            std::cout << loadEphemeris() << std::endl;

            // opts.isComputeApprPos = confReader->getValueAsBoolean("calcApprPos");
            // if (opts.isComputeApprPos)
            //     apprPosFile = confReader->getValue("apprPosFile");

            // load clock data from RINEX clk files, if required
            if (confReader->getValueAsBoolean("UseRinexClock"))
            {
#if !_DEBUG
                std::cout << "Load Rinex clock data ... ";
                std::cout << loadClocks() << std::endl;
#endif
            }

            std::cout << "Load ionospheric data ... ";
            std::cout << loadIono() << std::endl;

            std::cout << "Load Glonass FCN data... ";
            std::cout << loadFcn() << std::endl;

            std::cout << "Load Earth orientation data... ";
            std::cout << loadEOPData() << std::endl;

            std::cout << "Appr. position  source: ";
            if (createPosProvider())
                std::cout << getPosSourceString(apprPos->getSource()) << std::endl;
        }
        catch (const Exception& e)
        {
            std::cout << "Failed to load input data. An error has occured: " << e.what()
                      << std::endl;
            exit(-1);
        }
        catch (const std::exception& e)
        {
            std::cout << "Failed to load input data: An error has occured: " << e.what()
                      << std::endl;
            exit(-1);
        }
    }

    //
    bool GnssDataStore::loadEphemeris()
    {

        std::string subdir = confReader->getValue("EphemerisDir");
        const auto files = FsUtils::getAllFilesInDir(opts.workingDir / subdir);
        if (files.empty())
        {
            std::cerr << "Empty ephemeris directory " << opts.workingDir / subdir
                      << std::endl;
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
        std::string subdir = confReader->getValue("RinexClockDir");
        const auto files = FsUtils::getAllFilesInDir(opts.workingDir / subdir);

        for (const auto& file : files)
        {
            // Try to load each ephemeris file
            try
            {
                sp3NavFactory_->addDataSource(file.string());
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                std::cerr << "Rinex clock file '" << file << "' doesn't exist or you don't "
                          << "have permission to read it. Skipping it." << std::endl;
                continue;
            }
        }
        return files.size() > 0;
    }

    bool GnssDataStore::loadIono()
    {
        bool isIonexLoaded = loadIonoMap();

        auto type = (ComputeIonoModel::IonoModelType)confReader->getValueAsInt("CodeIonoCorrType");
        switch (type)
        {
        case gnsstk::ComputeIonoModel::Zero:
            ionoCorrector.setZeroModel();
            break;
        case gnsstk::ComputeIonoModel::Klobuchar:
            if (!loadBceIonoModel())
                GNSSTK_THROW(
                    InvalidRequest("Can't load iono model from Rinex GPS Navigation files."));
            break;
        case gnsstk::ComputeIonoModel::Ionex:
            if (isIonexLoaded)
                ionoCorrector.setIonosphereMap(ionexStore);
            else
                GNSSTK_THROW(InvalidRequest("Can't load Ionosphere map from Ionex files."));
            break;
        case gnsstk::ComputeIonoModel::DualFreq:
            ionoCorrector.setDualFreqModel();
            break;
        default:
            GNSSTK_THROW(InvalidRequest("Unknown Ionospheric model type."));
        }
        return true;
    }

    bool GnssDataStore::loadIonoMap()
    {
        std::string subdir = confReader->getValue("IonexDir");
        const auto files = FsUtils::getAllFilesInDir(opts.workingDir / subdir);
        ionexStore.clear();
        for (const auto& file : files)
        {
            ionexStore.loadFile(file.string());
        }

        return ionexStore.size() > 0;
    }

    bool GnssDataStore::loadBceIonoModel()
    {
        const std::string gpsObsExt = ".[\\d]{2}[nN]|";
        const auto files =
            FsUtils::getFilesByExtensionRegex(opts.workingDir / opts.bceDir, gpsObsExt);
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
                              << " doesn't have valid ionospheric correction parameters."
                              << std::endl;
                }

                bceIonoStore.addIonoModel(nav_ref_time.value(), iMod);
            }
            catch (...)
            {
                std::cerr << "Problem opening file " << file << std::endl;
                std::cerr << "Maybe it doesn't exist or you don't have proper read "
                          << "permissions." << std::endl;
                return false;
            }
        }

        if (bceIonoStore.empty())
        {
            std::cerr << "No valid ionospheric model found in Rinex GPS Navigation files."
                      << std::endl;
            return false;
        }
        //
        ionoCorrector.setKlobucharModel(bceIonoStore);

        return true;
    }

    bool GnssDataStore::loadFcn()
    {
        const std::string gln_nav_ext = ".[\\d]{2}[gG]|\\.rnx";
        auto files =
            FsUtils::getFilesByExtensionRegex(opts.workingDir / opts.bceDir, gln_nav_ext);

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

        fs::path eop_dir = opts.workingDir;
        try
        {
            const std::string subdir = confReader->getValue("ErpDir");
            eop_dir.append(subdir);
        }
        catch (...)
        {
            std::cerr << "Problem get value from config: file \"ErpDir\" " << std::endl;
            exit(-1);
        }

        try
        {
            const auto files = FsUtils::getFilesByExtension(eop_dir, ".ERP");

            if (files.empty())
            {
                std::cerr << "Empty ERP directory " << eop_dir << std::endl;
                return false;
            }

            for (const auto& file : files)
                eopStore.addFile(file.string());

            if (eopStore.size() == 0)
                std::cerr << "Empty ERP store after import " << eop_dir << std::endl;
        }
        catch (gnsstk::Exception& ex)
        {
            std::cerr << "Problem opening file " << ex << std::endl;
            return false;
        }
        return eopStore.size() > 0;
    }

    bool GnssDataStore::loadCodeBiases()
    {
        std::string biasesFile = opts.genericFilesDirectory;
        try
        {
            biasesFile += confReader->getValue("IersEopFile");
        }
        catch (...)
        {
            std::cerr << "Problem get value from config: file \"IersEopFile\" " << std::endl;
            return false;
        }

        // TODO: implement DCB loading
        // DCBData.setDCBFile(biasesFile,);

        return true;
    }

    GnssDataStore::GnssDataStore(gnsstk::ConfDataReader& confReader) : confReader(&confReader)
    {
        sp3NavFactory_ = std::make_shared<SP3NavDataFactory>();
        navLibrary_.addFactory(sp3NavFactory_);
    }

    void GnssDataStore::checkObservable()
    {
        std::ofstream os((opts.workingDir / "ObsStatistic.out").string());

        for (auto obsFile : getObsFiles(opts.SiteRover))
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
                        if (opts.systems.find(it.first.system) == opts.systems.end())
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

                    os << nGPS << " " << nGLN << " " << NumBadCNo1 << " " << NumP1 << " " << NumP2
                       << std::endl;
                }
            }
        }
    }

    bool GnssDataStore::createPosProvider()
    {
        auto apprPosProvider =
            static_cast<ApprPositionSource>(confReader->getValueAsInt("ApprPosProvider"));

        switch (apprPosProvider)
        {
        case ApprPositionSource::FromConfig:
            apprPos = std::make_unique<ApprPosSimple>(getPosition(opts.SiteRover));
            return true;
        case ApprPositionSource::ComputeForEachEpoch:
            apprPos = std::make_unique<ComputeOnePos>(navLibrary_);
            return true;
        case ApprPositionSource::ComputeForFirstEpoch:
            apprPos = std::make_unique<ComputeOnePos>(navLibrary_);
            return true;
        case ApprPositionSource::LoadFromFile:
            apprPos = std::make_unique<PositionFromFile>(
                (opts.workingDir / confReader->getValue("ApprPosFile")).string());
            return true;
        default:
            return false;
        }
    }

    gnsstk::Position GnssDataStore::getPosition(std::string siteId)
    {
        Position pos;
        int i = 0;
        for (auto& it : confReader->getValueListAsDouble("nominalPosition", opts.SiteRover))
        {
            if (i >= 3)
                break;
            pos[i++] = it;
        }
        return pos;
    }

    std::list<std::string> GnssDataStore::getObsFiles(const std::string& siteID) const
    {
        std::string subdir = confReader->getValue("RinesObsDir");
        auto paths = FsUtils::getAllFilesInDir(opts.workingDir / subdir / siteID);
        std::list<std::string> result;
        for (const auto& p : paths)
            result.push_back(p.string());
        return result;
    }
} // namespace pod
