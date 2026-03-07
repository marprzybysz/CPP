#include "inventory/sqlite_inventory_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace inventory {

namespace {

constexpr const char* kSelectLocationColumns =
    "SELECT id, public_id, name, type, parent_id, code, description FROM locations";

constexpr const char* kSelectCopyColumns =
    "SELECT id, public_id, book_id, inventory_number, status, target_location_id, current_location_id, condition_note, "
    "acquisition_date, archival_reason, created_at, updated_at FROM book_copies";

constexpr const char* kSelectSessionColumns =
    "SELECT id, public_id, location_public_id, scope_type, status, started_by, started_at, finished_at, "
    "on_shelf_count, justified_count, missing_count, summary_result FROM inventory_sessions";

constexpr const char* kSelectScanColumns =
    "SELECT id, session_public_id, scan_code, copy_public_id, scanned_at, note FROM inventory_scans";

constexpr const char* kSelectItemColumns =
    "SELECT id, session_public_id, copy_public_id, inventory_number, expected_location_public_id, "
    "current_location_public_id, scanned, result, justification, decided_at FROM inventory_items";

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

std::optional<int> read_optional_int(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return sqlite3_column_int(stmt, col);
}

void bind_optional_text(sqlite3_stmt* stmt, int index, const std::optional<std::string>& value) {
    if (value.has_value()) {
        sqlite3_bind_text(stmt, index, value->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

locations::Location map_location(sqlite3_stmt* stmt) {
    locations::Location location;
    location.id = sqlite3_column_int(stmt, 0);
    location.public_id = read_text(stmt, 1);
    location.name = read_text(stmt, 2);
    location.type = locations::location_type_from_string(read_text(stmt, 3));
    location.parent_id = read_optional_int(stmt, 4);
    location.code = read_text(stmt, 5);
    location.description = read_text(stmt, 6);
    return location;
}

copies::BookCopy map_copy(sqlite3_stmt* stmt) {
    copies::BookCopy copy;
    copy.id = sqlite3_column_int(stmt, 0);
    copy.public_id = read_text(stmt, 1);
    copy.book_id = sqlite3_column_int(stmt, 2);
    copy.inventory_number = read_text(stmt, 3);
    copy.status = copies::copy_status_from_string(read_text(stmt, 4));
    copy.target_location_id = read_optional_text(stmt, 5);
    copy.current_location_id = read_optional_text(stmt, 6);
    copy.condition_note = read_text(stmt, 7);
    copy.acquisition_date = read_optional_text(stmt, 8);
    copy.archival_reason = read_optional_text(stmt, 9);
    copy.created_at = read_text(stmt, 10);
    copy.updated_at = read_text(stmt, 11);
    return copy;
}

InventorySession map_session(sqlite3_stmt* stmt) {
    InventorySession session;
    session.id = sqlite3_column_int(stmt, 0);
    session.public_id = read_text(stmt, 1);
    session.location_public_id = read_text(stmt, 2);
    session.scope_type = locations::location_type_from_string(read_text(stmt, 3));
    session.status = inventory_status_from_string(read_text(stmt, 4));
    session.started_by = read_text(stmt, 5);
    session.started_at = read_text(stmt, 6);
    session.finished_at = read_optional_text(stmt, 7);
    session.on_shelf_count = sqlite3_column_int(stmt, 8);
    session.justified_count = sqlite3_column_int(stmt, 9);
    session.missing_count = sqlite3_column_int(stmt, 10);
    session.summary_result = read_text(stmt, 11);
    return session;
}

InventoryScannedCopy map_scan(sqlite3_stmt* stmt) {
    InventoryScannedCopy scan;
    scan.id = sqlite3_column_int(stmt, 0);
    scan.session_public_id = read_text(stmt, 1);
    scan.scan_code = read_text(stmt, 2);
    scan.copy_public_id = read_text(stmt, 3);
    scan.scanned_at = read_text(stmt, 4);
    scan.note = read_text(stmt, 5);
    return scan;
}

InventoryItem map_item(sqlite3_stmt* stmt) {
    InventoryItem item;
    item.id = sqlite3_column_int(stmt, 0);
    item.session_public_id = read_text(stmt, 1);
    item.copy_public_id = read_text(stmt, 2);
    item.inventory_number = read_text(stmt, 3);
    item.expected_location_public_id = read_optional_text(stmt, 4);
    item.current_location_public_id = read_optional_text(stmt, 5);
    item.scanned = sqlite3_column_int(stmt, 6) == 1;
    item.result = inventory_item_result_from_string(read_text(stmt, 7));
    item.justification = read_text(stmt, 8);
    item.decided_at = read_text(stmt, 9);
    return item;
}

} // namespace

std::optional<locations::Location> SqliteInventoryRepository::get_location_by_public_id(const std::string& public_id) const {
    const std::string sql = std::string(kSelectLocationColumns) + " WHERE public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get location by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        locations::Location location = map_location(stmt);
        sqlite3_finalize(stmt);
        return location;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get location by public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<std::string> SqliteInventoryRepository::list_location_subtree_public_ids(const std::string& root_public_id) const {
    const char* sql =
        "WITH RECURSIVE subtree(public_id, id) AS ("
        "  SELECT public_id, id FROM locations WHERE public_id = ? "
        "  UNION ALL "
        "  SELECT l.public_id, l.id FROM locations l JOIN subtree s ON l.parent_id = s.id"
        ")"
        "SELECT public_id FROM subtree;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list location subtree failed");
    }

    sqlite3_bind_text(stmt, 1, root_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<std::string> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(read_text(stmt, 0));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list location subtree failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<copies::BookCopy> SqliteInventoryRepository::list_copies_expected_in_locations(
    const std::vector<std::string>& location_public_ids) const {
    if (location_public_ids.empty()) {
        return {};
    }

    std::string sql = std::string(kSelectCopyColumns) + " WHERE target_location_id IN (";
    for (std::size_t i = 0; i < location_public_ids.size(); ++i) {
        if (i > 0) {
            sql += ",";
        }
        sql += "?";
    }
    sql += ") ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list expected copies failed");
    }

    for (std::size_t i = 0; i < location_public_ids.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), location_public_ids[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    std::vector<copies::BookCopy> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_copy(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list expected copies failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::optional<copies::BookCopy> SqliteInventoryRepository::get_copy_by_public_id(const std::string& public_id) const {
    const std::string sql = std::string(kSelectCopyColumns) + " WHERE public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get copy by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        copies::BookCopy copy = map_copy(stmt);
        sqlite3_finalize(stmt);
        return copy;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get copy by public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<copies::BookCopy> SqliteInventoryRepository::get_copy_by_inventory_number(const std::string& inventory_number) const {
    const std::string sql = std::string(kSelectCopyColumns) + " WHERE inventory_number = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get copy by inventory_number failed");
    }

    sqlite3_bind_text(stmt, 1, inventory_number.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        copies::BookCopy copy = map_copy(stmt);
        sqlite3_finalize(stmt);
        return copy;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get copy by inventory_number failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

InventorySession SqliteInventoryRepository::create_session(const InventorySession& session) {
    const char* sql =
        "INSERT INTO inventory_sessions (public_id, location_public_id, scope_type, status, started_by, started_at) "
        "VALUES (?, ?, ?, ?, ?, datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create inventory session failed");
    }

    sqlite3_bind_text(stmt, 1, session.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, session.location_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string scope_type = locations::to_string(session.scope_type);
    const std::string status = to_string(session.status);
    sqlite3_bind_text(stmt, 3, scope_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, session.started_by.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create inventory session failed");
    }

    sqlite3_finalize(stmt);
    return get_session_by_public_id(session.public_id).value();
}

std::optional<InventorySession> SqliteInventoryRepository::get_session_by_public_id(const std::string& public_id) const {
    const std::string sql = std::string(kSelectSessionColumns) + " WHERE public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get session by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        InventorySession session = map_session(stmt);
        sqlite3_finalize(stmt);
        return session;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get session by public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<InventorySession> SqliteInventoryRepository::get_active_session_for_location(const std::string& location_public_id) const {
    const std::string sql = std::string(kSelectSessionColumns) +
                            " WHERE location_public_id = ? AND status = 'IN_PROGRESS' ORDER BY id DESC LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get active session for location failed");
    }

    sqlite3_bind_text(stmt, 1, location_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        InventorySession session = map_session(stmt);
        sqlite3_finalize(stmt);
        return session;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get active session for location failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<InventorySession> SqliteInventoryRepository::list_sessions(int limit, int offset) const {
    const std::string sql =
        std::string(kSelectSessionColumns) + " ORDER BY started_at DESC, id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list sessions failed");
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    std::vector<InventorySession> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_session(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list sessions failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

InventoryScannedCopy SqliteInventoryRepository::create_scan(const InventoryScannedCopy& scan) {
    const char* sql =
        "INSERT INTO inventory_scans (session_public_id, scan_code, copy_public_id, scanned_at, note) "
        "VALUES (?, ?, ?, datetime('now'), ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create inventory scan failed");
    }

    sqlite3_bind_text(stmt, 1, scan.session_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, scan.scan_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, scan.copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, scan.note.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const int rc = sqlite3_errcode(db_.handle());
        sqlite3_finalize(stmt);

        if (rc == SQLITE_CONSTRAINT) {
            throw errors::ValidationError("Copy already scanned in this inventory session");
        }

        throw_sqlite(db_.handle(), "create inventory scan failed");
    }

    sqlite3_finalize(stmt);

    const char* fetch_sql =
        "SELECT id, session_public_id, scan_code, copy_public_id, scanned_at, note "
        "FROM inventory_scans WHERE session_public_id = ? AND copy_public_id = ? LIMIT 1;";

    sqlite3_stmt* fetch_stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), fetch_sql, -1, &fetch_stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare fetch created inventory scan failed");
    }

    sqlite3_bind_text(fetch_stmt, 1, scan.session_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(fetch_stmt, 2, scan.copy_public_id.c_str(), -1, SQLITE_TRANSIENT);

    const int fetch_rc = sqlite3_step(fetch_stmt);
    if (fetch_rc == SQLITE_ROW) {
        InventoryScannedCopy created = map_scan(fetch_stmt);
        sqlite3_finalize(fetch_stmt);
        return created;
    }

    sqlite3_finalize(fetch_stmt);
    throw errors::DatabaseError("fetch created inventory scan failed");
}

std::vector<InventoryScannedCopy> SqliteInventoryRepository::list_scans_for_session(const std::string& session_public_id) const {
    const std::string sql = std::string(kSelectScanColumns) + " WHERE session_public_id = ? ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list scans for session failed");
    }

    sqlite3_bind_text(stmt, 1, session_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<InventoryScannedCopy> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_scan(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list scans for session failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

void SqliteInventoryRepository::replace_session_items(const std::string& session_public_id,
                                                      const std::vector<InventoryItem>& items) {
    const char* begin_sql = "BEGIN TRANSACTION;";
    const char* commit_sql = "COMMIT;";
    const char* rollback_sql = "ROLLBACK;";

    if (sqlite3_exec(db_.handle(), begin_sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "begin replace session items transaction failed");
    }

    bool ok = false;
    try {
        const char* delete_sql = "DELETE FROM inventory_items WHERE session_public_id = ?;";
        sqlite3_stmt* delete_stmt = nullptr;
        if (sqlite3_prepare_v2(db_.handle(), delete_sql, -1, &delete_stmt, nullptr) != SQLITE_OK) {
            throw_sqlite(db_.handle(), "prepare delete session items failed");
        }

        sqlite3_bind_text(delete_stmt, 1, session_public_id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(delete_stmt) != SQLITE_DONE) {
            sqlite3_finalize(delete_stmt);
            throw_sqlite(db_.handle(), "delete session items failed");
        }
        sqlite3_finalize(delete_stmt);

        const char* insert_sql =
            "INSERT INTO inventory_items (session_public_id, copy_public_id, inventory_number, expected_location_public_id, "
            "current_location_public_id, scanned, result, justification, decided_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";

        sqlite3_stmt* insert_stmt = nullptr;
        if (sqlite3_prepare_v2(db_.handle(), insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
            throw_sqlite(db_.handle(), "prepare insert inventory item failed");
        }

        for (const auto& item : items) {
            sqlite3_bind_text(insert_stmt, 1, item.session_public_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insert_stmt, 2, item.copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insert_stmt, 3, item.inventory_number.c_str(), -1, SQLITE_TRANSIENT);
            bind_optional_text(insert_stmt, 4, item.expected_location_public_id);
            bind_optional_text(insert_stmt, 5, item.current_location_public_id);
            sqlite3_bind_int(insert_stmt, 6, item.scanned ? 1 : 0);
            const std::string result = to_string(item.result);
            sqlite3_bind_text(insert_stmt, 7, result.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(insert_stmt, 8, item.justification.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
                sqlite3_finalize(insert_stmt);
                throw_sqlite(db_.handle(), "insert inventory item failed");
            }

            sqlite3_reset(insert_stmt);
            sqlite3_clear_bindings(insert_stmt);
        }

        sqlite3_finalize(insert_stmt);

        if (sqlite3_exec(db_.handle(), commit_sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw_sqlite(db_.handle(), "commit replace session items transaction failed");
        }

        ok = true;
    } catch (...) {
        sqlite3_exec(db_.handle(), rollback_sql, nullptr, nullptr, nullptr);
        throw;
    }

    if (!ok) {
        throw errors::DatabaseError("replace session items failed");
    }
}

std::vector<InventoryItem> SqliteInventoryRepository::list_items_for_session(const std::string& session_public_id) const {
    const std::string sql = std::string(kSelectItemColumns) + " WHERE session_public_id = ? ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list items for session failed");
    }

    sqlite3_bind_text(stmt, 1, session_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<InventoryItem> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_item(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list items for session failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

InventorySession SqliteInventoryRepository::complete_session(const std::string& session_public_id,
                                                             int on_shelf_count,
                                                             int justified_count,
                                                             int missing_count,
                                                             const std::string& summary_result) {
    const char* sql =
        "UPDATE inventory_sessions SET status = 'COMPLETED', finished_at = datetime('now'), "
        "on_shelf_count = ?, justified_count = ?, missing_count = ?, summary_result = ? "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare complete inventory session failed");
    }

    sqlite3_bind_int(stmt, 1, on_shelf_count);
    sqlite3_bind_int(stmt, 2, justified_count);
    sqlite3_bind_int(stmt, 3, missing_count);
    sqlite3_bind_text(stmt, 4, summary_result.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, session_public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "complete inventory session failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Inventory session not found. public_id=" + session_public_id);
    }

    return get_session_by_public_id(session_public_id).value();
}

std::uint64_t SqliteInventoryRepository::next_public_sequence(int year) const {
    const std::string prefix = "INV-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";
    const char* sql = "SELECT public_id FROM inventory_sessions WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next inventory sequence failed");
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
            throw_sqlite(db_.handle(), "read next inventory sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace inventory
