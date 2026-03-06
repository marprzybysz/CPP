#include "db.hpp"
#include "errors/app_error.hpp"

#include <string>

static void throw_sqlite(sqlite3* db, const char* msg) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(msg) + " | sqlite: " + err);
}

static void exec_or_throw(sqlite3* db, const char* sql, const char* message) {
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "unknown sqlite error";
        sqlite3_free(err_msg);
        throw errors::DatabaseError(std::string(message) + ": " + err);
    }
}

static bool has_column(sqlite3* db, const std::string& table, const std::string& column) {
    const std::string sql = "PRAGMA table_info(" + table + ");";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db, "prepare table info failed");
    }

    bool found = false;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const unsigned char* name_raw = sqlite3_column_text(stmt, 1);
            const std::string name = name_raw ? reinterpret_cast<const char*>(name_raw) : "";
            if (name == column) {
                found = true;
                break;
            }
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db, "read table info failed");
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

static void migrate_legacy_books_table(sqlite3* db) {
    const char* sql =
        "BEGIN TRANSACTION;"
        "CREATE TABLE books_v2 ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "title TEXT NOT NULL,"
        "author TEXT NOT NULL,"
        "isbn TEXT NOT NULL UNIQUE,"
        "categories TEXT NOT NULL DEFAULT '',"
        "tags TEXT NOT NULL DEFAULT '',"
        "edition TEXT NOT NULL DEFAULT '',"
        "publisher TEXT NOT NULL DEFAULT '',"
        "publication_year INTEGER,"
        "description TEXT NOT NULL DEFAULT '',"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "archived_at TEXT,"
        "is_archived INTEGER NOT NULL DEFAULT 0"
        ");"
        "INSERT INTO books_v2 ("
        "id, public_id, title, author, isbn, categories, tags, edition, publisher, publication_year, description, created_at, updated_at, archived_at, is_archived"
        ")"
        "SELECT "
        "id, "
        "printf('BOOK-%d-%06d', CAST(strftime('%Y','now') AS INTEGER), id), "
        "title, author, "
        "printf('LEGACY-%06d', id), "
        "'', '', '', '', NULL, '', datetime('now'), datetime('now'), NULL, 0 "
        "FROM books;"
        "DROP TABLE books;"
        "ALTER TABLE books_v2 RENAME TO books;"
        "COMMIT;";

    exec_or_throw(db, sql, "legacy books migration failed");
}

Db::Db(std::string path) : path_(std::move(path)) {
    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        throw_sqlite(db_, "Failed to open database");
    }

    exec_or_throw(db_, "PRAGMA foreign_keys = ON;", "failed to enable foreign keys");
}

Db::~Db() {
    if (db_) sqlite3_close(db_);
}

void Db::init_schema() {
    const char* create_books_sql =
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "title TEXT NOT NULL,"
        "author TEXT NOT NULL,"
        "isbn TEXT NOT NULL UNIQUE,"
        "categories TEXT NOT NULL DEFAULT '',"
        "tags TEXT NOT NULL DEFAULT '',"
        "edition TEXT NOT NULL DEFAULT '',"
        "publisher TEXT NOT NULL DEFAULT '',"
        "publication_year INTEGER,"
        "description TEXT NOT NULL DEFAULT '',"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "archived_at TEXT,"
        "is_archived INTEGER NOT NULL DEFAULT 0"
        ");";

    exec_or_throw(db_, create_books_sql, "failed to create books table");

    if (!has_column(db_, "books", "public_id")) {
        migrate_legacy_books_table(db_);
    }

    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);", "failed to create idx_books_title");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);", "failed to create idx_books_author");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_books_isbn ON books(isbn);", "failed to create idx_books_isbn");

    const char* create_locations_sql =
        "CREATE TABLE IF NOT EXISTS locations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "name TEXT NOT NULL,"
        "type TEXT NOT NULL CHECK(type IN ('LIBRARY','ROOM','RACK','SHELF')),"
        "parent_id INTEGER,"
        "code TEXT NOT NULL UNIQUE,"
        "description TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(parent_id) REFERENCES locations(id)"
        ");";

    exec_or_throw(db_, create_locations_sql, "failed to create locations table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_locations_parent_id ON locations(parent_id);",
                  "failed to create idx_locations_parent_id");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_locations_type ON locations(type);",
                  "failed to create idx_locations_type");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_locations_code ON locations(code);",
                  "failed to create idx_locations_code");

    const char* create_copies_sql =
        "CREATE TABLE IF NOT EXISTS book_copies ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "book_id INTEGER NOT NULL,"
        "inventory_number TEXT NOT NULL UNIQUE,"
        "status TEXT NOT NULL CHECK(status IN ('ON_SHELF','LOANED','RESERVED','IN_REPAIR','ARCHIVED','LOST')),"
        "target_location_id TEXT,"
        "current_location_id TEXT,"
        "condition_note TEXT NOT NULL DEFAULT '',"
        "acquisition_date TEXT,"
        "archival_reason TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(book_id) REFERENCES books(id),"
        "FOREIGN KEY(target_location_id) REFERENCES locations(public_id),"
        "FOREIGN KEY(current_location_id) REFERENCES locations(public_id)"
        ");";

    const char* create_copy_status_history_sql =
        "CREATE TABLE IF NOT EXISTS copy_status_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "copy_public_id TEXT NOT NULL,"
        "from_status TEXT NOT NULL,"
        "to_status TEXT NOT NULL,"
        "changed_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "note TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(copy_public_id) REFERENCES book_copies(public_id)"
        ");";

    const char* create_copy_location_history_sql =
        "CREATE TABLE IF NOT EXISTS copy_location_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "copy_public_id TEXT NOT NULL,"
        "from_location_id TEXT,"
        "to_location_id TEXT,"
        "from_target_location_id TEXT,"
        "to_target_location_id TEXT,"
        "changed_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "note TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(copy_public_id) REFERENCES book_copies(public_id)"
        ");";

    exec_or_throw(db_, create_copies_sql, "failed to create book_copies table");
    exec_or_throw(db_, create_copy_status_history_sql, "failed to create copy_status_history table");
    exec_or_throw(db_, create_copy_location_history_sql, "failed to create copy_location_history table");

    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_book_copies_book_id ON book_copies(book_id);",
                  "failed to create idx_book_copies_book_id");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_book_copies_status ON book_copies(status);",
                  "failed to create idx_book_copies_status");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_copy_status_history_copy_id ON copy_status_history(copy_public_id);",
                  "failed to create idx_copy_status_history_copy_id");
    exec_or_throw(db_,
                  "CREATE INDEX IF NOT EXISTS idx_copy_location_history_copy_id ON copy_location_history(copy_public_id);",
                  "failed to create idx_copy_location_history_copy_id");
}
