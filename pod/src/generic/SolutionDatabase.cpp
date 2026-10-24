#include "SolutionDatabase.h"

#include "StringUtils.h"

#include <filesystem>
#include <stdexcept>

using namespace gnsstk;

namespace pod
{
    namespace
    {
    const gnsstk::TypeIDSet satDataTypesFilterForEpoch{
        TypeID::C1,
        TypeID::P1,
        TypeID::P2,
        TypeID::L1,
        TypeID::L2,
        TypeID::S1,
        TypeID::LI,
        TypeID::MWubbena,
        TypeID::ionoL1,
        // TypeID::rho            ,
        // TypeID::dtSat          ,
        TypeID::elevation,
        TypeID::azimuth,
        // TypeID::CSL1           ,
        // TypeID::CSL2           ,
        TypeID::satArc,
        TypeID::satStatus,
        /*
        TypeID::prefitC          ,
        TypeID::prefitL          ,
        TypeID::prefitL1         ,

        */
        TypeID::postfitC,
        TypeID::postfitC1,
        TypeID::postfitC2,
        TypeID::postfitP1,
        TypeID::postfitP2,
        TypeID::postfitC5,
        TypeID::postfitC6,
        TypeID::postfitC7,
        TypeID::postfitC8,
        TypeID::postfitPC,

        TypeID::postfitPC,
        TypeID::postfitL1,
        TypeID::postfitL2,
        TypeID::postfitL5,
        TypeID::postfitL6,
        TypeID::postfitL7,
        TypeID::postfitL8,
        TypeID::postfitLC,

        TypeID::BL1,
        TypeID::BL2,
        TypeID::BL5,
        TypeID::BL6,
        TypeID::BL7,
        TypeID::BL8,
        TypeID::BLC,
    };

    const std::string createSchemaCommand =
        ""
        "CREATE TABLE IF NOT EXISTS `SvDataItems` ("
        "	`SV`	INTEGER NOT NULL,"
        "	`DataID`	INTEGER NOT NULL,"
        "	`EpochID`	INTEGER NOT NULL,"
        "	FOREIGN KEY(`SV`) REFERENCES `SVS`(`ID`),"
        "    FOREIGN KEY(`DataID`) REFERENCES `RinexTypePairs`(`ID`),"
        "	FOREIGN KEY(`EpochID`) REFERENCES `Epochs`(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `SlnDataItems` ("
        "	`EpochID`	INTEGER,"
        "	`DataID`	INTEGER UNIQUE,"
        "	FOREIGN KEY(`EpochID`) REFERENCES `Epochs`(`ID`),"
        "	FOREIGN KEY(`DataID`) REFERENCES `RinexTypePairs`(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `SatSystems` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`Name`	TEXT NOT NULL UNIQUE,"
        "	PRIMARY KEY(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `SVS` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`SVID`	INTEGER NOT NULL,"
        "	`SSID`	INTEGER NOT NULL,"
        "	PRIMARY KEY(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `Sites` ("
        "	`Name`	TEXT NOT NULL,"
        "	PRIMARY KEY(`Name`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `RinexTypePairs` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`Type`	TEXT NOT NULL,"
        "	`Value`	REAL NOT NULL,"
        "	PRIMARY KEY(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `SkySectors` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`A0`	REAL NOT NULL,"
        "	`A1`	REAL NOT NULL,"
        "	`E0`	REAL NOT NULL,"
        "	`E1`	REAL NOT NULL,"
        "	PRIMARY KEY(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `MpProcResults` ("
        "	`ArcID`	INTEGER NOT NULL,"
        "	`SV`	INTEGER NOT NULL,"
        "	`Sector`INTEGER NOT NULL,"
        "	`Site`	TEXT NOT NULL,"
        "    FOREIGN KEY(`ArcID`) REFERENCES `MpArcs`(`ID`),"
        //   "	FOREIGN KEY(`SV`) REFERENCES `SVS`(`USI`),"
        "    FOREIGN KEY(`Sector`) REFERENCES `SkySectors`(`ID`),"
        "    FOREIGN KEY(`Site`) REFERENCES `Sites`(`Name`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `MpArcs` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`T1`	TEXT NOT NULL,"
        "	`T2`	TEXT NOT NULL,"
        "	`minEl`	REAL NOT NULL,"
        "	`maxEl`	REAL NOT NULL,"
        "	`H`	REAL NOT NULL,"
        "	`Count`	INTEGER NOT NULL,"
        "	PRIMARY KEY(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `GnssObsFile` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`FullName`	TEXT,"
        "	`Title`	TEXT,"
        "	PRIMARY KEY(`ID`)"
        ");"
        ""
        "CREATE TABLE IF NOT EXISTS `Epochs` ("
        "	`ID`	INTEGER NOT NULL,"
        "	`Time`	TEXT NOT NULL,"
        "	`FileID`	INTEGER NOT NULL,"
        "    `OccupationID`	TEXT,"
        "	FOREIGN KEY(`FileID`) REFERENCES `GnssObsFile`(`ID`),"
        "	PRIMARY KEY(`ID`)"
        ");"
        ""
        "CREATE TABLE IF NOT EXISTS `ColorsSt` ("
        "	`SolType`	INTEGER NOT NULL,"
        "	`FileID`	INTEGER NOT NULL,"
        "	`Color`	TEXT NOT NULL,"
        "	FOREIGN KEY(`FileID`) REFERENCES `GnssObsFile`(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `ColorGen` ("
        "	`FileID`	INTEGER NOT NULL UNIQUE,"
        "	`Color`	INTEGER NOT NULL,"
        "	FOREIGN KEY(`FileID`) REFERENCES `GnssObsFile`(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `ColorSv` ("
        "   `SVID`	INTEGER NOT NULL, "
        "   `SSID`	INTEGER NOT NULL, "
        "	`FileID`	INTEGER NOT NULL,"
        "	`Color`	INTEGER NOT NULL,"
        "	FOREIGN KEY(`SVID`) REFERENCES `SVS`(`SVID`),"
        "	FOREIGN KEY(`SSID`) REFERENCES `SVS`(`SSID`),"
        "	FOREIGN KEY(`FileID`) REFERENCES `GnssObsFile`(`ID`)"
        ");"
        "CREATE TABLE IF NOT EXISTS `TypeIDsByFiles` ("
        "	`FileId`	INTEGER NOT NULL,"
        "	`TypeId`	INTEGER NOT NULL,"
        "	FOREIGN KEY(`FileId`) REFERENCES `GnssObsFile`(`ID`)"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS `SlnTypesColors_Type_File_Unique` ON `ColorsSt` ("
        "	`SolType`,"
        "	`FileID`"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS `SatColor_SV_File_Unuque` ON `ColorSv` ("
        "	`FileID`,"
        "	`SVID`,"
        "	`SSID`"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS `SkySector_Unique` ON `SkySectors` ("
        "	`A0`	ASC,"
        "	`A1`,"
        "	`E0`,"
        "	`E1`"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS `SV_Unique` ON `SVS` ("
        "	`SVID`,"
        "	`SSID`"
        ");"
        "";
    } // namespace

    const unsigned SolutionDatabase::SCHEMA_VERSION = 1;
    const std::int32_t SolutionDatabase::APPLICATION_ID = 0x47534F4C; // 'GSOL'

    SolutionDatabase::SolutionDatabase(const std::string& path) : SQLiteConnection(path)
    {
        checkApplicationId(APPLICATION_ID, "solution");
        setPragmas();
        migrate();
    }

    void SolutionDatabase::create(const std::string& finalPath, const GnssEpochMap& eMap)
    {
        create(finalPath, [&](SolutionDatabase& db) { db.addSolution(eMap); });
    }

    void SolutionDatabase::create(const std::string& finalPath, const std::function<void(SolutionDatabase&)>& writer)
    {
        namespace fs = std::filesystem;
        const fs::path final(finalPath);
        const fs::path partial(finalPath + ".partial");

        if (fs::exists(final))
            throw std::runtime_error("Solution database already exists: " + finalPath);

        std::error_code ec;
        fs::remove(partial, ec); // stale leftover from an earlier failed run
        try
        {
            {
                SolutionDatabase db(partial.string());
                writer(db);
            } // connection closed here, all data committed
            if (fs::exists(final))
                throw std::runtime_error("Solution database already exists: " + finalPath);
            fs::rename(partial, final);
        }
        catch (...)
        {
            fs::remove(partial, ec);
            throw;
        }
    }

    void SolutionDatabase::setPragmas()
    {
        applyPragmas({
            "PRAGMA temp_store = \"2\";",
            "PRAGMA journal_mode = \"MEMORY\";",
            "PRAGMA foreign_keys = ON;",
        });
    }

    void SolutionDatabase::migrate()
    {
        const unsigned version = userVersion();
        if (version == SCHEMA_VERSION)
            return;
        if (version > SCHEMA_VERSION)
            errorHandler(SQLITE_ERROR, "Solution database schema is newer than supported.");

        Transaction tx(*this);
        execScript(createSchemaCommand);
        setUserVersion(SCHEMA_VERSION);
        tx.commit();
    }

#pragma region insert methods

    std::int64_t SolutionDatabase::addSolution(const pod::GnssEpochMap& eMap)
    {
        std::int64_t id = 0;
        inTransaction([&] {
            id = addFileMetadata(eMap);
            addEpochs(eMap);
        });
        return id;
    }

    void SolutionDatabase::addEpochs(const pod::GnssEpochMap& eMap)
    {
        // Inside an outer transaction, batch commits only release nested savepoints;
        // data remains part of the outer transaction until it is committed.
        obsItemCounter_ = 0;
        std::optional<Transaction> batch;
        for (auto& it : eMap.data)
        {
            if (!batch)
                batch.emplace(*this);
            addNewEpoch(it);
            if (obsItemCounter_ > maxObsItemsPerTransaction_)
            {
                batch->commit();
                batch.reset();
                obsItemCounter_ = 0;
            }
        }
        if (batch)
            batch->commit();
    }

    std::int64_t SolutionDatabase::addFileMetadata(const pod::GnssEpochMap& eMap)
    {
        Transaction tx(*this);
        const char* sql = "INSERT INTO `GnssObsFile`(`FullName`,`Title`) VALUES( @Name, @Title);";
        sqlite3_stmt* comm = prepare(sql);
        bindText(comm, 1, eMap.title);
        bindText(comm, 2, eMap.title);

        lastFileID_ = tryExecuteNonQueryAndGetRowId(comm);

        // fill the  SV metadata
        for (const auto& it : eMap.svs)
        {
            // add sv to table of all SV
            addSV(it);
        }

        // fill the  TypeId's metadata
        for (const auto it : eMap.types)
        {
            const char* sql = "INSERT INTO `TypeIDsByFiles`(`FileId`,`TypeId`) VALUES( @FileId, @TypeId);";
            sqlite3_stmt* comm = prepare(sql);
            bindInt64(comm, 1, lastFileID_);
            bindInt64(comm, 2, it.type);
            tryExecuteNonQuery(comm);
        }
        tx.commit();

        return lastFileID_;
    }

    void SolutionDatabase::addObsData(const std::pair<TypeID, double>& typeValuePair)
    {
        const char* sql = "INSERT INTO `RinexTypePairs`(`Type`,`Value`) VALUES (@Type, @Value);";
        sqlite3_stmt* comm = prepare(sql);
        bindInt64(comm, 1, typeValuePair.first.type);
        bindDouble(comm, 2, typeValuePair.second);

        lastTypeValuePairID_ = tryExecuteNonQueryAndGetRowId(comm);

        ++obsItemCounter_;
    }

    void SolutionDatabase::addSlnData(const gnsstk::typeValueMap& slnData)
    {
        for (auto& it : slnData)
        {
            addObsData(it);
            const char* sql = "INSERT INTO `SlnDataItems`(`EpochID`,`DataID`) VALUES (@EpochID, @DataID);";
            sqlite3_stmt* comm = prepare(sql);

            bindInt64(comm, 1, lastEpochID_);
            bindInt64(comm, 2, lastTypeValuePairID_);
            tryExecuteNonQuery(comm);
        }
    }

    void SolutionDatabase::addSvData(const gnsstk::satTypeValueMap& svData)
    {
        for (auto& svIt : svData)
        {
            SatID satId = svIt.first;
            for (auto& it : svIt.second)
            {
                addObsData(it);

                const char* sql = "INSERT INTO `SvDataItems`(`SV`,`DataID`, `EpochID`) VALUES ((SELECT ID FROM "
                                  "SVS WHERE SVID = @SVID AND SSID = @SSID), @DataID, @EpochID);";
                sqlite3_stmt* comm = prepare(sql);
                bindInt64(comm, 1, satId.id);
                bindInt64(comm, 2, static_cast<int>(satId.system));
                bindInt64(comm, 3, lastTypeValuePairID_);
                bindInt64(comm, 4, lastEpochID_);

                tryExecuteNonQuery(comm);
            }
        }
    }

    void SolutionDatabase::addNewEpoch(const std::pair<CommonTime, pod::GnssEpoch>& epoch)
    {
        const char* sql = "INSERT INTO `Epochs`(`Time`,`FileID`,'OccupationID') VALUES(@time, "
                          "@FileID, @OccupationID);";
        sqlite3_stmt* comm = prepare(sql);

        std::string occ_id = StringUtils::formatTime(epoch.first);
        bindText(comm, 1, occ_id);
        bindInt64(comm, 2, lastFileID_);
        bindText(comm, 3, "");

        lastEpochID_ = tryExecuteNonQueryAndGetRowId(comm);

        addSlnData(epoch.second.slnData);
        // TypeIDSet typeSet;// { TypeID::postfitC };
        // typeSet.insert(TypeID::postfitC);

        addSvData(epoch.second.satData.extractTypeID(satDataTypesFilterForEpoch));
    }

    std::int64_t SolutionDatabase::addSV(const gnsstk::SatID& sv)
    {
        const char* sql = "INSERT OR IGNORE INTO `SVS`(`SVID`,`SSID`) VALUES (@SVID, @SSID);";
        sqlite3_stmt* comm = prepare(sql);
        bindInt64(comm, 1, sv.id);
        bindInt64(comm, 2, static_cast<int>(sv.system));

        return tryExecuteNonQueryAndGetRowId(comm);
    }

#pragma endregion

} // namespace pod
