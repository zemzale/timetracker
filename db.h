#include <sqlite3.h>

#include <string>

sqlite3 *connect_to_db(std::string dbFile);
bool init_db(sqlite3 *db);

class DB {
 private:
  sqlite3 *db;

 public:
  DB(sqlite3 *db) { this->db = db; };
  ~DB() {};

  void startTracking(const std::string &taskId);
};
