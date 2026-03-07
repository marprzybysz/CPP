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

    const char* create_readers_sql =
        "CREATE TABLE IF NOT EXISTS readers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "card_number TEXT NOT NULL UNIQUE,"
        "first_name TEXT NOT NULL,"
        "last_name TEXT NOT NULL,"
        "email TEXT NOT NULL UNIQUE,"
        "phone TEXT,"
        "account_status TEXT NOT NULL CHECK(account_status IN ('ACTIVE','SUSPENDED','CLOSED')),"
        "reputation_points INTEGER NOT NULL DEFAULT 0,"
        "is_blocked INTEGER NOT NULL DEFAULT 0,"
        "block_reason TEXT,"
        "gdpr_consent INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");";

    const char* create_reader_loan_history_sql =
        "CREATE TABLE IF NOT EXISTS reader_loan_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "reader_public_id TEXT NOT NULL,"
        "loan_public_id TEXT NOT NULL,"
        "action TEXT NOT NULL,"
        "action_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "note TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(reader_public_id) REFERENCES readers(public_id)"
        ");";

    const char* create_reader_notes_sql =
        "CREATE TABLE IF NOT EXISTS reader_notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "reader_public_id TEXT NOT NULL,"
        "note TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(reader_public_id) REFERENCES readers(public_id)"
        ");";

    exec_or_throw(db_, create_readers_sql, "failed to create readers table");
    exec_or_throw(db_, create_reader_loan_history_sql, "failed to create reader_loan_history table");
    exec_or_throw(db_, create_reader_notes_sql, "failed to create reader_notes table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_readers_email ON readers(email);",
                  "failed to create idx_readers_email");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_readers_card_number ON readers(card_number);",
                  "failed to create idx_readers_card_number");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reader_loan_history_reader ON reader_loan_history(reader_public_id);",
                  "failed to create idx_reader_loan_history_reader");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reader_notes_reader ON reader_notes(reader_public_id);",
                  "failed to create idx_reader_notes_reader");

    const char* create_reader_reputation_history_sql =
        "CREATE TABLE IF NOT EXISTS reader_reputation_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "reader_id INTEGER NOT NULL,"
        "change_value INTEGER NOT NULL,"
        "reason TEXT NOT NULL,"
        "related_loan_id INTEGER,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(reader_id) REFERENCES readers(id)"
        ");";

    exec_or_throw(db_, create_reader_reputation_history_sql, "failed to create reader_reputation_history table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reader_reputation_history_reader ON reader_reputation_history(reader_id);",
                  "failed to create idx_reader_reputation_history_reader");
    exec_or_throw(db_,
                  "CREATE INDEX IF NOT EXISTS idx_reader_reputation_history_created_at ON reader_reputation_history(created_at);",
                  "failed to create idx_reader_reputation_history_created_at");

    const char* create_notes_sql =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "target_type TEXT NOT NULL CHECK(target_type IN ('READER','BOOK','COPY','LOAN')),"
        "target_id TEXT NOT NULL,"
        "author TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "content TEXT NOT NULL,"
        "is_archived INTEGER NOT NULL DEFAULT 0,"
        "archived_at TEXT"
        ");";

    exec_or_throw(db_, create_notes_sql, "failed to create notes table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_notes_target ON notes(target_type, target_id);",
                  "failed to create idx_notes_target");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_notes_created_at ON notes(created_at);",
                  "failed to create idx_notes_created_at");

    const char* create_reservations_sql =
        "CREATE TABLE IF NOT EXISTS reservations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "reader_id INTEGER NOT NULL,"
        "copy_id INTEGER,"
        "book_id INTEGER,"
        "reservation_date TEXT NOT NULL DEFAULT (date('now')),"
        "expiration_date TEXT NOT NULL,"
        "status TEXT NOT NULL CHECK(status IN ('ACTIVE','CANCELLED','EXPIRED','FULFILLED')),"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "CHECK ((copy_id IS NOT NULL AND book_id IS NULL) OR (copy_id IS NULL AND book_id IS NOT NULL)),"
        "FOREIGN KEY(reader_id) REFERENCES readers(id),"
        "FOREIGN KEY(copy_id) REFERENCES book_copies(id),"
        "FOREIGN KEY(book_id) REFERENCES books(id)"
        ");";

    exec_or_throw(db_, create_reservations_sql, "failed to create reservations table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reservations_reader_id ON reservations(reader_id);",
                  "failed to create idx_reservations_reader_id");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reservations_copy_id ON reservations(copy_id);",
                  "failed to create idx_reservations_copy_id");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reservations_book_id ON reservations(book_id);",
                  "failed to create idx_reservations_book_id");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reservations_status ON reservations(status);",
                  "failed to create idx_reservations_status");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_reservations_expiration_date ON reservations(expiration_date);",
                  "failed to create idx_reservations_expiration_date");

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

    const char* create_inventory_sessions_sql =
        "CREATE TABLE IF NOT EXISTS inventory_sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "location_public_id TEXT NOT NULL,"
        "scope_type TEXT NOT NULL CHECK(scope_type IN ('ROOM','RACK','SHELF')),"
        "status TEXT NOT NULL CHECK(status IN ('IN_PROGRESS','COMPLETED')),"
        "started_by TEXT NOT NULL,"
        "started_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "finished_at TEXT,"
        "on_shelf_count INTEGER NOT NULL DEFAULT 0,"
        "justified_count INTEGER NOT NULL DEFAULT 0,"
        "missing_count INTEGER NOT NULL DEFAULT 0,"
        "summary_result TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(location_public_id) REFERENCES locations(public_id)"
        ");";

    const char* create_inventory_scans_sql =
        "CREATE TABLE IF NOT EXISTS inventory_scans ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_public_id TEXT NOT NULL,"
        "scan_code TEXT NOT NULL,"
        "copy_public_id TEXT NOT NULL,"
        "scanned_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "note TEXT NOT NULL DEFAULT '',"
        "UNIQUE(session_public_id, copy_public_id),"
        "FOREIGN KEY(session_public_id) REFERENCES inventory_sessions(public_id),"
        "FOREIGN KEY(copy_public_id) REFERENCES book_copies(public_id)"
        ");";

    const char* create_inventory_items_sql =
        "CREATE TABLE IF NOT EXISTS inventory_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_public_id TEXT NOT NULL,"
        "copy_public_id TEXT NOT NULL,"
        "inventory_number TEXT NOT NULL,"
        "expected_location_public_id TEXT,"
        "current_location_public_id TEXT,"
        "scanned INTEGER NOT NULL DEFAULT 0,"
        "result TEXT NOT NULL CHECK(result IN ('ON_SHELF','JUSTIFIED','MISSING')),"
        "justification TEXT NOT NULL DEFAULT '',"
        "decided_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(session_public_id) REFERENCES inventory_sessions(public_id),"
        "FOREIGN KEY(copy_public_id) REFERENCES book_copies(public_id),"
        "FOREIGN KEY(expected_location_public_id) REFERENCES locations(public_id),"
        "FOREIGN KEY(current_location_public_id) REFERENCES locations(public_id)"
        ");";

    exec_or_throw(db_, create_inventory_sessions_sql, "failed to create inventory_sessions table");
    exec_or_throw(db_, create_inventory_scans_sql, "failed to create inventory_scans table");
    exec_or_throw(db_, create_inventory_items_sql, "failed to create inventory_items table");

    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_inventory_sessions_location ON inventory_sessions(location_public_id);",
                  "failed to create idx_inventory_sessions_location");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_inventory_sessions_status ON inventory_sessions(status);",
                  "failed to create idx_inventory_sessions_status");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_inventory_scans_session ON inventory_scans(session_public_id);",
                  "failed to create idx_inventory_scans_session");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_inventory_items_session ON inventory_items(session_public_id);",
                  "failed to create idx_inventory_items_session");

    const char* create_report_snapshots_sql =
        "CREATE TABLE IF NOT EXISTS report_snapshots ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "public_id TEXT NOT NULL UNIQUE,"
        "type TEXT NOT NULL CHECK(type IN ('OVERDUE_BOOKS','DELINQUENT_READERS','MOST_BORROWED_BOOKS','INVENTORY_STATE',"
        "'ARCHIVED_BOOKS','COPIES_IN_REPAIR')),"
        "date_from TEXT,"
        "date_to TEXT,"
        "generated_by TEXT NOT NULL DEFAULT '',"
        "payload TEXT NOT NULL DEFAULT '',"
        "created_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");";

    const char* create_report_exports_sql =
        "CREATE TABLE IF NOT EXISTS report_exports ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "report_public_id TEXT NOT NULL,"
        "format TEXT NOT NULL CHECK(format IN ('CSV')),"
        "status TEXT NOT NULL CHECK(status IN ('PENDING','DONE','FAILED')) DEFAULT 'PENDING',"
        "target_path TEXT,"
        "error_message TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(report_public_id) REFERENCES report_snapshots(public_id)"
        ");";

    exec_or_throw(db_, create_report_snapshots_sql, "failed to create report_snapshots table");
    exec_or_throw(db_, create_report_exports_sql, "failed to create report_exports table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_report_snapshots_type ON report_snapshots(type);",
                  "failed to create idx_report_snapshots_type");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_report_snapshots_created_at ON report_snapshots(created_at);",
                  "failed to create idx_report_snapshots_created_at");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_report_exports_report_public_id ON report_exports(report_public_id);",
                  "failed to create idx_report_exports_report_public_id");

    const char* create_audit_events_sql =
        "CREATE TABLE IF NOT EXISTS audit_events ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "module TEXT NOT NULL CHECK(module IN ('SYSTEM','BOOKS','COPIES','READERS','LOANS','INVENTORY','SUPPLY','EXPORT')),"
        "actor TEXT NOT NULL,"
        "occurred_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "object_type TEXT NOT NULL,"
        "object_public_id TEXT NOT NULL,"
        "operation_type TEXT NOT NULL,"
        "details TEXT NOT NULL DEFAULT ''"
        ");";

    exec_or_throw(db_, create_audit_events_sql, "failed to create audit_events table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_audit_events_module ON audit_events(module);",
                  "failed to create idx_audit_events_module");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_audit_events_object ON audit_events(object_type, object_public_id);",
                  "failed to create idx_audit_events_object");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_audit_events_occurred_at ON audit_events(occurred_at);",
                  "failed to create idx_audit_events_occurred_at");

    const char* create_copy_withdrawals_sql =
        "CREATE TABLE IF NOT EXISTS copy_withdrawals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "copy_public_id TEXT NOT NULL UNIQUE,"
        "reason TEXT NOT NULL CHECK(reason IN ('DAMAGE','NO_BORROWINGS','DUPLICATES','OUTDATED_CONTENT','LOST')),"
        "withdrawal_date TEXT NOT NULL,"
        "operator_name TEXT NOT NULL,"
        "note TEXT NOT NULL DEFAULT '',"
        "resulting_status TEXT NOT NULL CHECK(resulting_status IN ('ARCHIVED','LOST')),"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "FOREIGN KEY(copy_public_id) REFERENCES book_copies(public_id)"
        ");";

    exec_or_throw(db_, create_copy_withdrawals_sql, "failed to create copy_withdrawals table");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_copy_withdrawals_reason ON copy_withdrawals(reason);",
                  "failed to create idx_copy_withdrawals_reason");
    exec_or_throw(db_, "CREATE INDEX IF NOT EXISTS idx_copy_withdrawals_date ON copy_withdrawals(withdrawal_date);",
                  "failed to create idx_copy_withdrawals_date");
}
