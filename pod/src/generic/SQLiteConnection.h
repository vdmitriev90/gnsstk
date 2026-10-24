#ifndef POD_SQLITE_CONNECTION
#define POD_SQLITE_CONNECTION

#include "sqlite3.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pod
{
	// Internal base for typed SQLite databases: connection management, prepared statements, binding,
	// error handling, savepoint-based transactions and user_version support.
	class SQLiteConnection
	{
	  public:
		SQLiteConnection(const SQLiteConnection&) = delete;
		SQLiteConnection& operator=(const SQLiteConnection&) = delete;
		virtual ~SQLiteConnection();

		// All transactions are SAVEPOINTs, so they nest: while an explicit transaction is active,
		// internal ones only release their savepoint and the whole sequence is committed (or rolled back)
		// atomically by commitTransaction()/rollbackTransaction().
		void beginTransaction();
		void commitTransaction();
		void rollbackTransaction();

		sqlite3* handle() const
		{
			return db_;
		}

		unsigned userVersion();

	  protected:
		explicit SQLiteConnection(const std::string& path);

		// RAII savepoint: rolls back on destruction unless commit() was called.
		class Transaction
		{
		  public:
			explicit Transaction(SQLiteConnection& db);
			Transaction(const Transaction&) = delete;
			Transaction& operator=(const Transaction&) = delete;
			~Transaction();
			void commit();

		  private:
			SQLiteConnection& db_;
			std::string name_;
			bool done_ = false;
		};

		// Runs 'body' atomically; nests inside any active transaction.
		template <class F>
		void inTransaction(F&& body)
		{
			Transaction tx(*this);
			body();
			tx.commit();
		}

		void applyPragmas(const std::vector<std::string>& pragmas);
		void setUserVersion(unsigned version);
		// Initializes PRAGMA application_id if it is 0, otherwise requires it to equal 'expected'.
		void checkApplicationId(std::int32_t expected, const char* databaseName);
		void execScript(const std::string& sql);

		void tryExecuteNonQuery(const std::string& sql);
		void tryExecuteNonQuery(const char* sql);
		void tryExecuteNonQuery(sqlite3_stmt* stmt);
		std::int64_t tryExecuteNonQueryAndGetRowId(sqlite3_stmt* stmt);
		[[noreturn]] void errorHandler(int errorCode, const char* error);
		sqlite3_stmt* prepare(const char* sql);
		void checkBind(int rc, sqlite3_stmt* stmt);
		void bindInt64(sqlite3_stmt* stmt, int idx, std::int64_t value);
		void bindDouble(sqlite3_stmt* stmt, int idx, double value);
		void bindText(sqlite3_stmt* stmt, int idx, const std::string& value);
		void bindOptionalText(sqlite3_stmt* stmt, int idx, const std::string& value);
		void bindOptionalInt(sqlite3_stmt* stmt, int idx, const std::optional<int>& value);
		void bindOptionalDouble(sqlite3_stmt* stmt, int idx, const std::optional<double>& value);
		void stepAndReset(sqlite3_stmt* stmt);

	  private:
		std::string fileName_;
		sqlite3* db_ = nullptr;
		std::unique_ptr<Transaction> userTx_;
		int savepointDepth_ = 0;
	};
} // namespace pod

#endif // !POD_SQLITE_CONNECTION
