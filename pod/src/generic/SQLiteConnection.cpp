#include "SQLiteConnection.h"

#include "WinUtils.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace pod
{
	SQLiteConnection::SQLiteConnection(const std::string& path) : fileName_(path)
	{
		sqlite3_initialize();

		const int rc = sqlite3_open_v2(fileName_.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if (rc)
		{
			const std::string msg = std::string("Error opening SQLite3 database:") + sqlite3_errmsg(db_);
			sqlite3_close(db_);
			db_ = nullptr;
			errorHandler(rc, msg.c_str());
		}
	}

	SQLiteConnection::~SQLiteConnection()
	{
		userTx_.reset();
		sqlite3_close(db_);
	}

	void SQLiteConnection::applyPragmas(const std::vector<std::string>& pragmas)
	{
		for (const auto& p : pragmas)
			execScript(p);
	}

	unsigned SQLiteConnection::userVersion()
	{
		sqlite3_stmt* stmt = prepare("PRAGMA user_version;");
		unsigned v = 0;
		const int rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
			v = static_cast<unsigned>(sqlite3_column_int(stmt, 0));
		else if (rc != SQLITE_DONE)
		{
			sqlite3_finalize(stmt);
			errorHandler(rc, "Can't read user_version.");
		}
		sqlite3_finalize(stmt);
		return v;
	}

	void SQLiteConnection::checkApplicationId(std::int32_t expected, const char* databaseName)
	{
		sqlite3_stmt* stmt = prepare("PRAGMA application_id;");
		std::int32_t id = 0;
		const int rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
			id = sqlite3_column_int(stmt, 0);
		else if (rc != SQLITE_DONE)
		{
			sqlite3_finalize(stmt);
			errorHandler(rc, "Can't read application_id.");
		}
		sqlite3_finalize(stmt);

		if (id == expected)
			return;
		if (id != 0)
			errorHandler(SQLITE_MISMATCH,
						 (std::string("Wrong database type: expected ") + databaseName + " database (application_id " +
						  std::to_string(expected) + "), but the file has application_id " + std::to_string(id) + ".")
							.c_str());
		execScript("PRAGMA application_id = " + std::to_string(expected) + ";");
	}

	void SQLiteConnection::setUserVersion(unsigned version)
	{
		execScript("PRAGMA user_version = " + std::to_string(version) + ";");
	}

	void SQLiteConnection::execScript(const std::string& sql)
	{
		char* err = nullptr;
		const int rc = sqlite3_exec(db_, sql.c_str(), NULL, NULL, &err);
		if (rc != SQLITE_OK)
		{
			const std::string msg = std::string("Can't execute SQL script: ") + (err ? err : "");
			sqlite3_free(err);
			errorHandler(rc, msg.c_str());
		}
	}

	void SQLiteConnection::tryExecuteNonQuery(sqlite3_stmt* comm)
	{
		int rc;
		while ((rc = sqlite3_step(comm)) == SQLITE_ROW)
		{
			DBOUT("\r\n---")
			DBOUT(sqlite3_column_text(comm, 0));
			DBOUT("---\r\n")
		}
		if (rc != SQLITE_DONE)
		{
			const std::string msg = std::string("Can't execute SQL command: ") + sqlite3_errmsg(db_);
			sqlite3_finalize(comm);
			errorHandler(rc, msg.c_str());
		}
		sqlite3_finalize(comm);
	}

	std::int64_t SQLiteConnection::tryExecuteNonQueryAndGetRowId(sqlite3_stmt* stmt)
	{
		tryExecuteNonQuery(stmt);
		return sqlite3_last_insert_rowid(db_);
	}

	void SQLiteConnection::tryExecuteNonQuery(const std::string& sql)
	{
		tryExecuteNonQuery(sql.c_str());
	}

	void SQLiteConnection::tryExecuteNonQuery(const char* sql)
	{
		tryExecuteNonQuery(prepare(sql));
	}

	void SQLiteConnection::errorHandler(int errorCode, const char* error)
	{
		DBOUT("An Error has occured.\r\nEroor code:");
		DBOUT(errorCode);
		DBOUT("\r\nEroor message: ");
		DBOUT(error);
		throw std::runtime_error(error);
	}

	sqlite3_stmt* SQLiteConnection::prepare(const char* sql)
	{
		sqlite3_stmt* stmt = nullptr;
		const int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
		if (rc != SQLITE_OK)
		{
			const std::string msg = std::string("Can't compile SQL command: ") + sqlite3_errmsg(db_);
			sqlite3_finalize(stmt);
			errorHandler(rc, msg.c_str());
		}
		return stmt;
	}

	void SQLiteConnection::checkBind(int rc, sqlite3_stmt* stmt)
	{
		if (rc != SQLITE_OK)
		{
			const std::string msg = std::string("Can't bind SQL parameter: ") + sqlite3_errmsg(db_);
			sqlite3_finalize(stmt);
			errorHandler(rc, msg.c_str());
		}
	}

	void SQLiteConnection::bindInt64(sqlite3_stmt* stmt, int idx, std::int64_t value)
	{
		checkBind(sqlite3_bind_int64(stmt, idx, value), stmt);
	}

	void SQLiteConnection::bindDouble(sqlite3_stmt* stmt, int idx, double value)
	{
		if (!std::isfinite(value))
			checkBind(sqlite3_bind_null(stmt, idx), stmt);
		else
			checkBind(sqlite3_bind_double(stmt, idx, value), stmt);
	}

	void SQLiteConnection::bindText(sqlite3_stmt* stmt, int idx, const std::string& value)
	{
		checkBind(sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT), stmt);
	}

	void SQLiteConnection::bindOptionalText(sqlite3_stmt* stmt, int idx, const std::string& value)
	{
		if (value.empty())
			checkBind(sqlite3_bind_null(stmt, idx), stmt);
		else
			bindText(stmt, idx, value);
	}

	void SQLiteConnection::bindOptionalInt(sqlite3_stmt* stmt, int idx, const std::optional<int>& value)
	{
		if (value)
			bindInt64(stmt, idx, *value);
		else
			checkBind(sqlite3_bind_null(stmt, idx), stmt);
	}

	void SQLiteConnection::bindOptionalDouble(sqlite3_stmt* stmt, int idx, const std::optional<double>& value)
	{
		if (value && std::isfinite(*value))
			bindDouble(stmt, idx, *value);
		else
			checkBind(sqlite3_bind_null(stmt, idx), stmt);
	}

	// Executes an INSERT/UPDATE on a statement that will be reused: requires SQLITE_DONE,
	// then resets the statement and clears the bindings. The statement is finalized on error.
	void SQLiteConnection::stepAndReset(sqlite3_stmt* stmt)
	{
		const int rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			const std::string msg = std::string("Can't execute SQL command: ") + sqlite3_errmsg(db_);
			sqlite3_finalize(stmt);
			errorHandler(rc, msg.c_str());
		}
		int rc2 = sqlite3_reset(stmt);
		if (rc2 == SQLITE_OK)
			rc2 = sqlite3_clear_bindings(stmt);
		if (rc2 != SQLITE_OK)
		{
			const std::string msg = std::string("Can't reset SQL statement: ") + sqlite3_errmsg(db_);
			sqlite3_finalize(stmt);
			errorHandler(rc2, msg.c_str());
		}
	}

	SQLiteConnection::Transaction::Transaction(SQLiteConnection& db)
		: db_(db), name_("pod_sp" + std::to_string(db.savepointDepth_ + 1))
	{
		db_.tryExecuteNonQuery("SAVEPOINT " + name_ + ";");
		++db_.savepointDepth_;
	}

	void SQLiteConnection::Transaction::commit()
	{
		db_.tryExecuteNonQuery("RELEASE " + name_ + ";");
		done_ = true;
		--db_.savepointDepth_;
	}

	SQLiteConnection::Transaction::~Transaction()
	{
		if (done_)
			return;
		const std::string sql = "ROLLBACK TO " + name_ + "; RELEASE " + name_ + ";";
		sqlite3_exec(db_.db_, sql.c_str(), NULL, NULL, NULL);
		--db_.savepointDepth_;
	}

	void SQLiteConnection::beginTransaction()
	{
		if (userTx_)
			errorHandler(SQLITE_MISUSE, "Transaction is already active.");
		userTx_ = std::make_unique<Transaction>(*this);
	}

	void SQLiteConnection::commitTransaction()
	{
		if (!userTx_)
			errorHandler(SQLITE_MISUSE, "No active transaction.");
		userTx_->commit();
		userTx_.reset();
	}

	void SQLiteConnection::rollbackTransaction()
	{
		userTx_.reset();
	}
} // namespace pod
