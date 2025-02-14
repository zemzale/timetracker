#include <sqlite3.h>

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
