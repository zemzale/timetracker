#include <sqlite3.h>

#include <string>

sqlite3 *connect_to_db(std::string dbFile);
bool init_db(sqlite3 *db);
