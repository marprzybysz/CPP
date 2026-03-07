#include "imports/sqlite_import_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace imports {

namespace {

constexpr const char* kSelectRunColumns =
    "SELECT id, public_id, format, target, status, source, operator_name, valid_records, invalid_records, started_at, finished_at "
    "FROM import_runs";

constexpr const char* kSelectErrorColumns =
    "SELECT id, run_public_id, row_number, message, raw_record FROM import_run_errors";

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

std::optional<std::string> read_optional_text(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return read_text(stmt, col);
}

ImportRun map_run(sqlite3_stmt* stmt) {
    ImportRun run;
    run.id = sqlite3_column_int(stmt, 0);
    run.public_id = read_text(stmt, 1);
    run.format = import_format_from_string(read_text(stmt, 2));
    run.target = import_target_from_string(read_text(stmt, 3));
    run.status = import_status_from_string(read_text(stmt, 4));
    run.source = read_text(stmt, 5);
    run.operator_name = read_text(stmt, 6);
    run.valid_records = sqlite3_column_int(stmt, 7);
    run.invalid_records = sqlite3_column_int(stmt, 8);
    run.started_at = read_text(stmt, 9);
    run.finished_at = read_optional_text(stmt, 10);
    return run;
}

ImportRecordError map_error(sqlite3_stmt* stmt) {
    ImportRecordError error;
    error.id = sqlite3_column_int(stmt, 0);
    error.run_public_id = read_text(stmt, 1);
    error.row_number = sqlite3_column_int(stmt, 2);
    error.message = read_text(stmt, 3);
    error.raw_record = read_text(stmt, 4);
    return error;
}

} // namespace

ImportRun SqliteImportRepository::create_run(const ImportRun& run) {
    const char* sql =
        "INSERT INTO import_runs (public_id, format, target, status, source, operator_name, valid_records, invalid_records, started_at) "
        "VALUES (?, ?, ?, ?, ?, ?, 0, 0, datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create import run failed");
    }

    sqlite3_bind_text(stmt, 1, run.public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string format = to_string(run.format);
    const std::string target = to_string(run.target);
    const std::string status = to_string(run.status);
    sqlite3_bind_text(stmt, 2, format.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, target.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, run.source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, run.operator_name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create import run failed");
    }

    sqlite3_finalize(stmt);
    return get_run_by_public_id(run.public_id).value();
}

ImportRun SqliteImportRepository::complete_run(const std::string& run_public_id,
                                                ImportStatus status,
                                                int valid_records,
                                                int invalid_records) {
    const char* sql =
        "UPDATE import_runs SET status = ?, valid_records = ?, invalid_records = ?, finished_at = datetime('now') "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare complete import run failed");
    }

    const std::string status_text = to_string(status);
    sqlite3_bind_text(stmt, 1, status_text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, valid_records);
    sqlite3_bind_int(stmt, 3, invalid_records);
    sqlite3_bind_text(stmt, 4, run_public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "complete import run failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Import run not found. public_id=" + run_public_id);
    }

    return get_run_by_public_id(run_public_id).value();
}

ImportRecordError SqliteImportRepository::add_error(const ImportRecordError& error) {
    const char* sql =
        "INSERT INTO import_run_errors (run_public_id, row_number, message, raw_record) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare add import error failed");
    }

    sqlite3_bind_text(stmt, 1, error.run_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, error.row_number);
    sqlite3_bind_text(stmt, 3, error.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, error.raw_record.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "add import error failed");
    }

    const sqlite3_int64 row_id = sqlite3_last_insert_rowid(db_.handle());
    sqlite3_finalize(stmt);

    const std::string fetch_sql = std::string(kSelectErrorColumns) + " WHERE id = ? LIMIT 1;";
    sqlite3_stmt* fetch_stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), fetch_sql.c_str(), -1, &fetch_stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare fetch import error failed");
    }

    sqlite3_bind_int64(fetch_stmt, 1, row_id);
    const int rc = sqlite3_step(fetch_stmt);

    if (rc == SQLITE_ROW) {
        ImportRecordError created = map_error(fetch_stmt);
        sqlite3_finalize(fetch_stmt);
        return created;
    }

    sqlite3_finalize(fetch_stmt);
    throw errors::DatabaseError("fetch import error failed");
}

std::optional<ImportRun> SqliteImportRepository::get_run_by_public_id(const std::string& public_id) const {
    const std::string sql = std::string(kSelectRunColumns) + " WHERE public_id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get import run by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        ImportRun run = map_run(stmt);
        sqlite3_finalize(stmt);
        return run;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get import run by public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<ImportRecordError> SqliteImportRepository::list_errors_for_run(const std::string& run_public_id) const {
    const std::string sql = std::string(kSelectErrorColumns) + " WHERE run_public_id = ? ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list import errors failed");
    }

    sqlite3_bind_text(stmt, 1, run_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<ImportRecordError> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_error(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list import errors failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::uint64_t SqliteImportRepository::next_public_sequence(int year) const {
    const std::string prefix = "IMP-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";
    const char* sql = "SELECT public_id FROM import_runs WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next import sequence failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::uint64_t max_sequence = 0;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const std::string public_id = read_text(stmt, 0);
            const std::size_t pos = public_id.rfind('-');
            if (pos == std::string::npos || pos + 1 >= public_id.size()) {
                continue;
            }

            try {
                const std::uint64_t sequence = std::stoull(public_id.substr(pos + 1));
                if (sequence > max_sequence) {
                    max_sequence = sequence;
                }
            } catch (...) {
                continue;
            }
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "read next import sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace imports
