#include "db.hpp"
#include <stdexcept>

static void throw_sqlite(sqlite3* db, const char* msg) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw std::runtime_error(std::string(msg) + " | sqlite: " + err);
}

Db::Db(std::string path) : path_(std::move(path)) {
    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        throw_sqlite(db_, "Failed to open database");
    }
}

Db::~Db() {
    if (db_) sqlite3_close(db_);
}

void Db::init_schema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "author TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to init schema: " + err);
    }
}