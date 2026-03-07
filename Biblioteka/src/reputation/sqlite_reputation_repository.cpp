#include "reputation/sqlite_reputation_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace reputation {

namespace {

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

std::optional<int> read_optional_int(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return sqlite3_column_int(stmt, col);
}

ReputationChange map_change(sqlite3_stmt* stmt) {
    ReputationChange change;
    change.id = sqlite3_column_int(stmt, 0);
    change.reader_id = sqlite3_column_int(stmt, 1);
    change.change_value = sqlite3_column_int(stmt, 2);
    change.reason = read_text(stmt, 3);
    change.related_loan_id = read_optional_int(stmt, 4);
    change.created_at = read_text(stmt, 5);
    return change;
}

void bind_optional_int(sqlite3_stmt* stmt, int idx, const std::optional<int>& value) {
    if (value.has_value()) {
        sqlite3_bind_int(stmt, idx, *value);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

} // namespace

ReaderReputationSnapshot SqliteReputationRepository::get_reader_snapshot(int reader_id) const {
    const char* sql = "SELECT reputation_points, is_blocked FROM readers WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get_reader_snapshot failed");
    }

    sqlite3_bind_int(stmt, 1, reader_id);
    const int rc = sqlite3_step(stmt);

    ReaderReputationSnapshot snapshot;
    if (rc == SQLITE_ROW) {
        snapshot.exists = true;
        snapshot.reputation_points = sqlite3_column_int(stmt, 0);
        snapshot.is_blocked = sqlite3_column_int(stmt, 1) != 0;
    } else if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get_reader_snapshot failed");
    }

    sqlite3_finalize(stmt);
    return snapshot;
}

int SqliteReputationRepository::update_reader_reputation(int reader_id,
                                                         int new_reputation_points,
                                                         bool is_blocked,
                                                         const std::optional<std::string>& block_reason,
                                                         bool set_account_suspended) {
    const char* sql =
        "UPDATE readers SET reputation_points = ?, is_blocked = ?, block_reason = COALESCE(?, block_reason), "
        "account_status = CASE WHEN ? = 1 THEN 'SUSPENDED' ELSE account_status END, "
        "updated_at = datetime('now') WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update_reader_reputation failed");
    }

    sqlite3_bind_int(stmt, 1, new_reputation_points);
    sqlite3_bind_int(stmt, 2, is_blocked ? 1 : 0);
    if (block_reason.has_value()) {
        sqlite3_bind_text(stmt, 3, block_reason->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    sqlite3_bind_int(stmt, 4, set_account_suspended ? 1 : 0);
    sqlite3_bind_int(stmt, 5, reader_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update_reader_reputation failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(reader_id));
    }

    return new_reputation_points;
}

ReputationChange SqliteReputationRepository::create_change(const ReputationChange& change) {
    const char* sql =
        "INSERT INTO reader_reputation_history (reader_id, change_value, reason, related_loan_id, created_at) "
        "VALUES (?, ?, ?, ?, datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create_change failed");
    }

    sqlite3_bind_int(stmt, 1, change.reader_id);
    sqlite3_bind_int(stmt, 2, change.change_value);
    sqlite3_bind_text(stmt, 3, change.reason.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_int(stmt, 4, change.related_loan_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create_change failed");
    }

    const int id = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    sqlite3_finalize(stmt);

    const char* select_sql =
        "SELECT id, reader_id, change_value, reason, related_loan_id, created_at "
        "FROM reader_reputation_history WHERE id = ? LIMIT 1;";

    sqlite3_stmt* sel = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), select_sql, -1, &sel, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare select created change failed");
    }

    sqlite3_bind_int(sel, 1, id);
    const int rc = sqlite3_step(sel);
    if (rc == SQLITE_ROW) {
        ReputationChange created = map_change(sel);
        sqlite3_finalize(sel);
        return created;
    }

    sqlite3_finalize(sel);
    throw errors::DatabaseError("Failed to fetch created reputation change");
}

std::vector<ReputationChange> SqliteReputationRepository::list_changes(int reader_id, int limit, int offset) const {
    const char* sql =
        "SELECT id, reader_id, change_value, reason, related_loan_id, created_at "
        "FROM reader_reputation_history WHERE reader_id = ? ORDER BY created_at DESC, id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list_changes failed");
    }

    sqlite3_bind_int(stmt, 1, reader_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    std::vector<ReputationChange> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_change(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list_changes failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace reputation
