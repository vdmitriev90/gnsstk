#ifndef POD_SQLITE_ADAPTER
#define POD_SQLITE_ADAPTER

#include "FsUtils.h"
#include "GnssEpochMap.h"
#include "sqlite3.h"

namespace pod
{
    class SQLiteAdapter
    {
#pragma region static meembers
      public:
        static void testSQLite(const char* f1, const char* f2);
        static const unsigned SCHEMA_VERSION;
        static const std::string createSchemaCommand;
        static const gnsstk::TypeIDSet requaredTypes;

#pragma endregion

#pragma region Create methods

      public:
        SQLiteAdapter() = delete;
        SQLiteAdapter(const std::string& spath) : firstTime_(true), fileName_(spath)
        {
            initialize();
        }

        SQLiteAdapter(char* path) : firstTime_(true), fileName_(path)
        {
            initialize();
        }

        ~SQLiteAdapter()
        {
            sqlite3_close(db_);
        }

      private:
        void initialize();
        void setPragmas();
        void create();

#pragma endregion

#pragma region Insert methods

      public:
        void addNewFile(const pod::GnssEpochMap& eMap);

      private:
        void addObsData(const std::pair<gnsstk::TypeID, double>& typeValuePair);
        void addSlnData(const gnsstk::typeValueMap& slnData);
        void addSvData(const gnsstk::satTypeValueMap& svData);
        void addNewEpoch(const std::pair<gnsstk::CommonTime, pod::GnssEpoch>& epoch);
        int addSV(const gnsstk::SatID& sv);

#pragma endregion

#pragma region service methods

      private:
        void tryExecuteNonQuery(const std::string& sql);
        void tryExecuteNonQuery(const char* sql);
        void tryExecuteNonQuery(sqlite3_stmt* stmt);
        int tryExecuteNonQueryAndGetRowId(const std::string& sql);
        int tryExecuteNonQueryAndGetRowId(const char* sql);
        int tryExecuteNonQueryAndGetRowId(sqlite3_stmt* stmt);
        void errorHandler(int errorCode, const char* error);
        void updateTransaction();
        void finalizeTransactionsSequence();

#pragma endregion

#pragma region Fields

      private:
        std::string fileName_;
        sqlite3* db_;
        int lastFileID_;
        int lastEpochID_;
        long lastTypeValuePairID_;
        bool firstTime_;
        int obsItemCounter_ = 0;
        int maxObsItemsPerTransaction_ = 100000;

#pragma endregion
    };
} // namespace pod

#endif // !POD_SQLITE_ADAPTER
