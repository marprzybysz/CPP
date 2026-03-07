#include "audit/sqlite_audit_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace audit {

namespace {

constexpr const char* kSelectAuditColumns =
    "SELECT id, module, actor, occurred_at, object_type, object_public_id, operation_type, details FROM audit_events";

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

AuditEvent map_audit_event(sqlite3_stmt* stmt) {
    AuditEvent event;
    event.id = sqlite3_column_int(stmt, 0);
    event.module = audit_module_from_string(read_text(stmt, 1));
    event.actor = read_text(stmt, 2);
    event.occurred_at = read_text(stmt, 3);
    event.object_type = read_text(stmt, 4);
    event.object_public_id = read_text(stmt, 5);
    event.operation_type = read_text(stmt, 6);
    event.details = read_text(stmt, 7);
    return event;
}

} // namespace

AuditEvent SqliteAuditRepository::create(const AuditEvent& event) {
    const char* sql =
        "INSERT INTO audit_events (module, actor, occurred_at, object_type, object_public_id, operation_type, details) "
        "VALUES (?, ?, datetime('now'), ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create audit event failed");
    }

    const std::string module = to_string(event.module);
    sqlite3_bind_text(stmt, 1, module.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, event.actor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, event.object_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, event.object_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, event.operation_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, event.details.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create audit event failed");
    }

    const sqlite3_int64 row_id = sqlite3_last_insert_rowid(db_.handle());
    sqlite3_finalize(stmt);

    const std::string select_sql = std::string(kSelectAuditColumns) + " WHERE id = ? LIMIT 1;";
    sqlite3_stmt* select_stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), select_sql.c_str(), -1, &select_stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare select created audit event failed");
    }

    sqlite3_bind_int64(select_stmt, 1, row_id);
    const int rc = sqlite3_step(select_stmt);

    if (rc == SQLITE_ROW) {
        AuditEvent created = map_audit_event(select_stmt);
        sqlite3_finalize(select_stmt);
        return created;
    }

    sqlite3_finalize(select_stmt);
    throw errors::AuditError("created audit event not found");
}

std::vector<AuditEvent> SqliteAuditRepository::list_recent(int limit) const {
    const std::string sql = std::string(kSelectAuditColumns) + " ORDER BY id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list recent audit events failed");
    }

    sqlite3_bind_int(stmt, 1, limit);

    std::vector<AuditEvent> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_audit_event(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list recent audit events failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<AuditEvent> SqliteAuditRepository::list_for_object(const std::string& object_type,
                                                                const std::string& object_public_id,
                                                                int limit) const {
    const std::string sql = std::string(kSelectAuditColumns) +
                            " WHERE object_type = ? AND object_public_id = ? ORDER BY id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list audit events for object failed");
    }

    sqlite3_bind_text(stmt, 1, object_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, object_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, limit);

    std::vector<AuditEvent> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_audit_event(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list audit events for object failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace audit
