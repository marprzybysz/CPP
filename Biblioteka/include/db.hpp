#pragma once
#include <string>
#include <sqlite3.h>

class Db {
public:
    explicit Db(std::string path);
    ~Db();

    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;

    sqlite3* handle() const { return db_; }

    void init_schema();  // tworzy tabelę books jak init_db.py

private:
    sqlite3* db_{nullptr};
    std::string path_;
};