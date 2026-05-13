#include "GnssDataStore.hpp"

#include "CodeProcSvData.h"
#include "FsUtils.h"
#include "Rinex3NavStream.hpp"
#include "SP3NavDataFactory.hpp"

using namespace gnsstk;

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

            opts.workingDir = fs::path(path).parent_path().string();

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
            opts.genericFilesDirectory = opts.workingDir + "\\" + subdir + "\\";

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

        std::list<std::string> files;
        std::string subdir = confReader->getValue("EphemerisDir");
        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + subdir, files);

        for (auto file : files)
        {
            // Try to load each ephemeris file
            try
            {
                sp3NavFactory_->addDataSource(file);
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                std::cerr << "SP3 file '" << file << "' doesn't exist or you don't "
                          << "have permission to read it. Skipping it." << std::endl;
                exit(-1);
            }
        }
        return files.size() > 0;
    }

    // reading clock data
    bool GnssDataStore::loadClocks()
    {
        std::list<std::string> files;
        std::string subdir = confReader->getValue("RinexClockDir");
        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + subdir, files);

        for (auto file : files)
        {
            // Try to load each ephemeris file
            try
            {
                sp3NavFactory_->addDataSource(file);
            }
            catch (FileMissingException& e)
            {
                // If file doesn't exist, issue a warning
                std::cerr << "Rinex clock file '" << file << "' doesn't exist or you don't "
                          << "have permission to read it. Skipping it." << std::endl;
                exit(-1);
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
        std::list<std::string> files;
        std::string subdir = confReader->getValue("IonexDir");
        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + subdir, files);
        ionexStore.clear();
        for (auto& file : files)
        {
            ionexStore.loadFile(file);
        }

        return ionexStore.size() > 0;
    }

    bool GnssDataStore::loadBceIonoModel()
    {
        const std::string gpsObsExt = ".[\\d]{2}[nN]";
        std::list<std::string> files;

        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + opts.bceDir, gpsObsExt, files);
        int i = 0;
        for (auto file : files)
        {
            try
            {
                IonoModel iMod;
                Rinex3NavStream rNavFile;
                Rinex3NavHeader rNavHeader;

                rNavFile.open(file.c_str(), std::ios::in);
                rNavFile >> rNavHeader;

#pragma region try get the date

                CommonTime refTime = CommonTime::BEGINNING_OF_TIME;
                if (rNavHeader.fileAgency == "AIUB")
                {

                    for (auto&& it : rNavHeader.commentList)
                    {
                        int doy = -1, yr = -1;
                        std::cmatch res;
                        std::regex rxDoY("DAY [0-9]{3}"), rxY(" [0-9]{4}");
                        bool b = std::regex_search(it.c_str(), res, rxDoY);
                        if (b)
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
                        {
                            refTime = YDSTime(yr, doy, 0, TimeSystem::GPS);
                            break;
                        }
                    }
                }
                else
                {
                    const auto& time_rnx = rNavHeader.mapTimeCorr["GPUT"].refTime;
                    if (time_rnx != CommonTime::BEGINNING_OF_TIME)
                        refTime = time_rnx;
                }
#pragma endregion

                if (rNavHeader.valid & Rinex3NavHeader::validIonoCorrGPS)
                {
                    // Extract the Alpha and Beta parameters from the header
                    double* ionAlpha = rNavHeader.mapIonoCorr["GPSA"].param;
                    double* ionBeta = rNavHeader.mapIonoCorr["GPSB"].param;

                    // Feed the ionospheric model with the parameters
                    iMod.setModel(ionAlpha, ionBeta);
                    i++;
                }
                else
                {
                    std::cerr << "WARNING: Navigation file " << file
                              << " doesn't have valid ionospheric correction parameters."
                              << std::endl;
                    exit(-1);
                }

                bceIonoStore.addIonoModel(refTime, iMod);
            }
            catch (...)
            {
                std::cerr << "Problem opening file " << file << std::endl;
                std::cerr << "Maybe it doesn't exist or you don't have proper read "
                          << "permissions." << std::endl;
                exit(-1);
            }
        }

        //
        ionoCorrector.setKlobucharModel(bceIonoStore);

        return i > 0;
    }

    bool GnssDataStore::loadFcn()
    {
        const std::string glnObsExt = ".[\\d]{2}[gG]";
        std::list<std::string> files;
        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + opts.bceDir, glnObsExt, files);

        for (auto file : files)
        {
            try
            {
                SatID::loadGloFcn(file.c_str());
            }
            catch (...)
            {
                std::cerr << "Problem opening file " << file << std::endl;
                std::cerr << "Maybe it doesn't exist or you don't have proper read "
                          << "permissions." << std::endl;
                exit(-1);
            }
        }
        return SatID::glonassFcn.size() > 0;
    }

    bool GnssDataStore::loadEOPData()
    {

        std::string iersEopFile = opts.genericFilesDirectory;
        try
        {
            iersEopFile += confReader->getValue("IersEopFile");
        }
        catch (...)
        {
            std::cerr << "Problem get value from config: file \"IersEopFile\" " << std::endl;
            exit(-1);
        }

        try
        {
            eopStore.addIERSFile(iersEopFile);
        }
        catch (...)
        {
            std::cerr << "Problem opening file " << iersEopFile << std::endl;
            std::cerr << "Maybe it doesn't exist or you don't have proper read "
                      << "permissions." << std::endl;
            exit(-1);
        }
        return eopStore.size() > 0;
    }

    bool GnssDataStore::loadCodeBiades()
    {
        std::string biasesFile = opts.genericFilesDirectory;
        try
        {
            biasesFile += confReader->getValue("IersEopFile");
        }
        catch (...)
        {
            std::cerr << "Problem get value from config: file \"IersEopFile\" " << std::endl;
            exit(-1);
        }

        try
        {
            // DCBData.setDCBFile(biasesFile,);
        }
        catch (...)
        {
            std::cerr << "Problem opening file " << biasesFile << std::endl;
            std::cerr << "Maybe it doesn't exist or you don't have proper read "
                      << "permissions." << std::endl;
            exit(-1);
        }
        return eopStore.size() > 0;
    }

    GnssDataStore::GnssDataStore(gnsstk::ConfDataReader& confReader) : confReader(&confReader)
    {
        sp3NavFactory_ = std::make_shared<SP3NavDataFactory>();
        navLibrary_.addFactory(sp3NavFactory_);
    }

    void GnssDataStore::checkObservable()
    {
        std::ofstream os(opts.workingDir + "\\ObsStatisic.out");

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
                    int NumC1(0), NumP1(0), NumP2(0), NumBadCNo1(0);
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

                        double C1 = rod.getObs(it.first, ids[TypeID::C1], roh).data;

                        int CNoL1 = rod.getObs(it.first, ids[TypeID::S1], roh).data;
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
            apprPos = std::make_unique<PositionFromFile>(opts.workingDir + "\\"
                                                         + confReader->getValue("ApprPosFile"));
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
            pos[i++] = it;
        return pos;
    }

    std::list<std::string> GnssDataStore::getObsFiles(const std::string& siteID) const
    {
        std::list<std::string> ObsFiles;
        std::string subdir = confReader->getValue("RinesObsDir");
        FsUtils::getAllFilesInDir(opts.workingDir + "\\" + subdir + "\\" + siteID, ObsFiles);
        return ObsFiles;
    }
} // namespace pod
