#include "db.h"

#include <sqlite3.h>

#include <ctime>
#include <iostream>

sqlite3 *connect_to_db(std::string dbFile) {
  sqlite3 *db = nullptr;
  int rc = sqlite3_open(dbFile.c_str(), &db);
  if (rc) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
    return nullptr;
  }
  return db;
}

bool init_db(sqlite3 *db) {
  const char *sqlCreate =
      "CREATE TABLE IF NOT EXISTS tasks ("
      " id INTEGER PRIMARY KEY AUTOINCREMENT,"
      " task_id TEXT NOT NULL,"
      " start_time INTEGER NOT NULL,"
      " stop_time INTEGER,"
      " duration REAL"
      ");";
  char *errMsg = nullptr;
  int rc = sqlite3_exec(db, sqlCreate, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "Error creating table: " << errMsg << "\n";
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

void DB::startTracking(const std::string &taskId) {
  // Check if an active task exists
  const char *checkSql = "SELECT COUNT(*) FROM tasks WHERE stop_time IS NULL;";
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) {
    std::cerr << "An active task is already running. Stop it "
                 "before starting a new one.\n";
    sqlite3_finalize(stmt);
    return;
  }
  sqlite3_finalize(stmt);

  // Insert new task record with the start time (epoch seconds)
  const char *insertSql = "INSERT INTO tasks (task_id, start_time) VALUES (?, ?);";
  rc = sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare insert: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  sqlite3_bind_text(stmt, 1, taskId.c_str(), -1, SQLITE_TRANSIENT);
  std::time_t startTime = std::time(nullptr);
  sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(startTime));

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Error inserting record: " << sqlite3_errmsg(db) << "\n";
  } else {
    std::cout << "Started tracking task: " << taskId << "\n";
  }
  sqlite3_finalize(stmt);
}
