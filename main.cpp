#include <errno.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <ostream>
#include <string>

#include "db.h"

// Get the value of XDG_DATA_HOME or fallback to ~/.local/share
std::string getXdgDataHome() {
  const char *xdg = std::getenv("XDG_DATA_HOME");
  if (xdg) {
    return std::string(xdg);
  }
  const char *home = std::getenv("HOME");
  return home ? std::string(home) + "/.local/share" : ".";
}

// Data directory: $XDG_DATA_HOME/timetracker
std::string getDataDirectory() { return getXdgDataHome() + "/timetracker"; }

// Full path to the SQLite database file
std::string getDatabaseFile() { return getDataDirectory() + "/timetracker.db"; }

// Create a directory if it does not exist
bool createDirectory(const std::string &path) { return (mkdir(path.c_str(), 0755) == 0 || errno == EEXIST); }

// Start tracking a task with the given taskId
void startTracking(sqlite3 *db, const std::string &taskId) {
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

// Stop the currently active task
void stopTracking(sqlite3 *db) {
  // Find the active task (if any)
  const char *selectSql =
      "SELECT id, task_id, start_time FROM tasks WHERE stop_time IS NULL "
      "LIMIT 1;";
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare select statement: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    std::cerr << "No active task found.\n";
    sqlite3_finalize(stmt);
    return;
  }

  int id = sqlite3_column_int(stmt, 0);
  std::string taskId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
  std::time_t startTime = static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
  sqlite3_finalize(stmt);

  std::time_t stopTime = std::time(nullptr);
  double duration = std::difftime(stopTime, startTime);

  // Update task record with stop_time and duration
  const char *updateSql = "UPDATE tasks SET stop_time = ?, duration = ? WHERE id = ?;";
  rc = sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare update: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(stopTime));
  sqlite3_bind_double(stmt, 2, duration);
  sqlite3_bind_int(stmt, 3, id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Error updating record: " << sqlite3_errmsg(db) << "\n";
  } else {
    std::cout << "Stopped tracking task: " << taskId << "\nTotal time: " << duration << " seconds.\n";
  }
  sqlite3_finalize(stmt);
}

void listTasks(sqlite3 *db) {
  const char *selectSql = "SELECT task_id, duration FROM tasks";
  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare select statement: " << sqlite3_errmsg(db) << std::endl;
    return;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW) {
    std::cerr << "No tasks found." << std::endl;
    sqlite3_finalize(stmt);
    return;
  }

  while (rc == SQLITE_ROW) {
    auto taskId = sqlite3_column_text(stmt, 0);
    std::time_t duration = sqlite3_column_double(stmt, 1);

    std::cout << taskId << " | " << duration << "s" << std::endl;
    rc = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);

  return;
}
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage:" << std::endl
              << "\ttimetracker start [TASK - ID]" << std::endl
              << "\ttimetracker stop" << std::endl
              << "\ttimetracker list" << std::endl;
    ;
    return 1;
  }

  std::string command = argv[1];

  // Ensure the data directory exists
  std::string dataDir = getDataDirectory();
  if (!createDirectory(dataDir)) {
    std::cerr << "Failed to create data directory: " << dataDir << "\n";
    return 1;
  }

  sqlite3 *db = connect_to_db(getDatabaseFile());

  if (!init_db(db)) {
    sqlite3_close(db);
    return 1;
  }

  if (command == "start") {
    if (argc < 3) {
      std::cerr << "Please provide a task ID.\n";
      sqlite3_close(db);
      return 1;
    }
    std::string taskId = argv[2];
    startTracking(db, taskId);
  } else if (command == "stop") {
    stopTracking(db);
  } else if (command == "list") {
    listTasks(db);
  } else {
    std::cerr << "Unknown command.\n";
    sqlite3_close(db);
    return 1;
  }

  sqlite3_close(db);
  return 0;
}
