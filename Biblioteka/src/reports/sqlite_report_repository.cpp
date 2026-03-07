#include "reports/sqlite_report_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace reports {

namespace {

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

void bind_optional_text(sqlite3_stmt* stmt, int index, const std::optional<std::string>& value) {
    if (value.has_value()) {
        sqlite3_bind_text(stmt, index, value->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

void append_range_filter(std::string& sql,
                         std::vector<std::string>& bindings,
                         const DateRangeFilter& range,
                         const std::string& field_sql) {
    if (range.from_date.has_value()) {
        sql += " AND date(" + field_sql + ") >= date(?)";
        bindings.push_back(*range.from_date);
    }
    if (range.to_date.has_value()) {
        sql += " AND date(" + field_sql + ") <= date(?)";
        bindings.push_back(*range.to_date);
    }
}

SavedReportSnapshot map_snapshot(sqlite3_stmt* stmt) {
    SavedReportSnapshot snapshot;
    snapshot.id = sqlite3_column_int(stmt, 0);
    snapshot.public_id = read_text(stmt, 1);
    snapshot.type = report_type_from_string(read_text(stmt, 2));
    snapshot.date_from = read_optional_text(stmt, 3);
    snapshot.date_to = read_optional_text(stmt, 4);
    snapshot.generated_by = read_text(stmt, 5);
    snapshot.payload = read_text(stmt, 6);
    snapshot.created_at = read_text(stmt, 7);
    return snapshot;
}

} // namespace

std::vector<OverdueBookRow> SqliteReportRepository::fetch_overdue_books(const DateRangeFilter& range, int limit) const {
    std::string sql =
        "SELECT r.public_id, r.reservation_date, r.expiration_date, bc.public_id, bc.inventory_number, "
        "b.public_id, b.title, rd.public_id, (rd.first_name || ' ' || rd.last_name), "
        "CAST(julianday(date('now')) - julianday(date(r.expiration_date)) AS INTEGER) AS overdue_days "
        "FROM reservations r "
        "JOIN readers rd ON rd.id = r.reader_id "
        "LEFT JOIN book_copies bc ON bc.id = r.copy_id "
        "LEFT JOIN books b ON b.id = COALESCE(r.book_id, bc.book_id) "
        "WHERE r.status = 'ACTIVE' AND date(r.expiration_date) < date('now')";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "r.expiration_date");
    sql += " ORDER BY overdue_days DESC, r.expiration_date ASC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare overdue books report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<OverdueBookRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            OverdueBookRow row;
            row.reservation_public_id = read_text(stmt, 0);
            row.reservation_date = read_text(stmt, 1);
            row.expiration_date = read_text(stmt, 2);
            row.copy_public_id = read_text(stmt, 3);
            row.inventory_number = read_text(stmt, 4);
            row.book_public_id = read_text(stmt, 5);
            row.book_title = read_text(stmt, 6);
            row.reader_public_id = read_text(stmt, 7);
            row.reader_name = read_text(stmt, 8);
            row.overdue_days = sqlite3_column_int(stmt, 9);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute overdue books report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<DelinquentReaderRow> SqliteReportRepository::fetch_delinquent_readers(const DateRangeFilter& range, int limit) const {
    std::string sql =
        "SELECT rd.public_id, rd.card_number, (rd.first_name || ' ' || rd.last_name), rd.reputation_points, rd.is_blocked, "
        "COALESCE(exp_stats.expired_count, 0) AS expired_count, "
        "COALESCE(ovd_stats.overdue_count, 0) AS overdue_count "
        "FROM readers rd "
        "LEFT JOIN ("
        "  SELECT r.reader_id, COUNT(*) AS expired_count "
        "  FROM reservations r "
        "  WHERE r.status = 'EXPIRED'";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "r.expiration_date");

    sql +=
        "  GROUP BY r.reader_id"
        ") AS exp_stats ON exp_stats.reader_id = rd.id "
        "LEFT JOIN ("
        "  SELECT r.reader_id, COUNT(*) AS overdue_count "
        "  FROM reservations r "
        "  WHERE r.status = 'ACTIVE' AND date(r.expiration_date) < date('now')";

    append_range_filter(sql, bindings, range, "r.expiration_date");

    sql +=
        "  GROUP BY r.reader_id"
        ") AS ovd_stats ON ovd_stats.reader_id = rd.id "
        "WHERE rd.is_blocked = 1 OR rd.reputation_points < 0 OR COALESCE(exp_stats.expired_count, 0) > 0 OR "
        "COALESCE(ovd_stats.overdue_count, 0) > 0 "
        "ORDER BY rd.is_blocked DESC, rd.reputation_points ASC, expired_count DESC, overdue_count DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare delinquent readers report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<DelinquentReaderRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            DelinquentReaderRow row;
            row.reader_public_id = read_text(stmt, 0);
            row.card_number = read_text(stmt, 1);
            row.full_name = read_text(stmt, 2);
            row.reputation_points = sqlite3_column_int(stmt, 3);
            row.is_blocked = sqlite3_column_int(stmt, 4) != 0;
            row.expired_reservations = sqlite3_column_int(stmt, 5);
            row.overdue_items = sqlite3_column_int(stmt, 6);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute delinquent readers report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<MostBorrowedBookRow> SqliteReportRepository::fetch_most_borrowed_books(const DateRangeFilter& range, int limit) const {
    std::string sql =
        "SELECT b.public_id, b.title, b.author, COUNT(*) AS borrow_count "
        "FROM copy_status_history csh "
        "JOIN book_copies bc ON bc.public_id = csh.copy_public_id "
        "JOIN books b ON b.id = bc.book_id "
        "WHERE csh.to_status = 'LOANED'";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "csh.changed_at");

    sql += " GROUP BY b.id, b.public_id, b.title, b.author ORDER BY borrow_count DESC, b.title ASC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare most borrowed books report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<MostBorrowedBookRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            MostBorrowedBookRow row;
            row.book_public_id = read_text(stmt, 0);
            row.title = read_text(stmt, 1);
            row.author = read_text(stmt, 2);
            row.borrow_count = sqlite3_column_int(stmt, 3);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute most borrowed books report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<InventoryStateRow> SqliteReportRepository::fetch_inventory_state(const DateRangeFilter& range, int limit) const {
    std::string sql =
        "SELECT public_id, location_public_id, scope_type, started_by, started_at, finished_at, "
        "on_shelf_count, justified_count, missing_count "
        "FROM inventory_sessions WHERE 1=1";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "started_at");

    sql += " ORDER BY started_at DESC, id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare inventory state report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<InventoryStateRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            InventoryStateRow row;
            row.inventory_public_id = read_text(stmt, 0);
            row.location_public_id = read_text(stmt, 1);
            row.scope_type = read_text(stmt, 2);
            row.started_by = read_text(stmt, 3);
            row.started_at = read_text(stmt, 4);
            row.finished_at = read_optional_text(stmt, 5);
            row.on_shelf_count = sqlite3_column_int(stmt, 6);
            row.justified_count = sqlite3_column_int(stmt, 7);
            row.missing_count = sqlite3_column_int(stmt, 8);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute inventory state report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<ArchivedBookRow> SqliteReportRepository::fetch_archived_books(const DateRangeFilter& range, int limit) const {
    std::string sql = "SELECT public_id, title, author, isbn, archived_at FROM books WHERE is_archived = 1";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "archived_at");

    sql += " ORDER BY archived_at DESC, id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare archived books report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<ArchivedBookRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            ArchivedBookRow row;
            row.book_public_id = read_text(stmt, 0);
            row.title = read_text(stmt, 1);
            row.author = read_text(stmt, 2);
            row.isbn = read_text(stmt, 3);
            row.archived_at = read_text(stmt, 4);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute archived books report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<RepairCopyRow> SqliteReportRepository::fetch_copies_in_repair(const DateRangeFilter& range, int limit) const {
    std::string sql =
        "SELECT bc.public_id, bc.inventory_number, bc.condition_note, bc.updated_at, b.public_id, b.title, "
        "bc.current_location_id, bc.target_location_id "
        "FROM book_copies bc "
        "JOIN books b ON b.id = bc.book_id "
        "WHERE bc.status = 'IN_REPAIR'";

    std::vector<std::string> bindings;
    append_range_filter(sql, bindings, range, "bc.updated_at");

    sql += " ORDER BY bc.updated_at DESC, bc.id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare copies in repair report failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, idx, limit);

    std::vector<RepairCopyRow> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            RepairCopyRow row;
            row.copy_public_id = read_text(stmt, 0);
            row.inventory_number = read_text(stmt, 1);
            row.condition_note = read_text(stmt, 2);
            row.updated_at = read_text(stmt, 3);
            row.book_public_id = read_text(stmt, 4);
            row.book_title = read_text(stmt, 5);
            row.current_location_id = read_optional_text(stmt, 6);
            row.target_location_id = read_optional_text(stmt, 7);
            out.push_back(std::move(row));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "execute copies in repair report failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

SavedReportSnapshot SqliteReportRepository::save_snapshot(const SavedReportSnapshot& snapshot) {
    const char* sql =
        "INSERT INTO report_snapshots (public_id, type, date_from, date_to, generated_by, payload, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare save report snapshot failed");
    }

    sqlite3_bind_text(stmt, 1, snapshot.public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string type = to_string(snapshot.type);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 3, snapshot.date_from);
    bind_optional_text(stmt, 4, snapshot.date_to);
    sqlite3_bind_text(stmt, 5, snapshot.generated_by.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, snapshot.payload.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "save report snapshot failed");
    }

    sqlite3_finalize(stmt);

    const char* fetch_sql =
        "SELECT id, public_id, type, date_from, date_to, generated_by, payload, created_at "
        "FROM report_snapshots WHERE public_id = ? LIMIT 1;";

    sqlite3_stmt* fetch_stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), fetch_sql, -1, &fetch_stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare fetch report snapshot failed");
    }

    sqlite3_bind_text(fetch_stmt, 1, snapshot.public_id.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(fetch_stmt);
    if (rc == SQLITE_ROW) {
        SavedReportSnapshot out = map_snapshot(fetch_stmt);
        sqlite3_finalize(fetch_stmt);
        return out;
    }

    sqlite3_finalize(fetch_stmt);
    throw errors::DatabaseError("fetch report snapshot failed");
}

std::uint64_t SqliteReportRepository::next_public_sequence(int year) const {
    const std::string prefix = "RPT-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";
    const char* sql = "SELECT public_id FROM report_snapshots WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next report sequence failed");
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
            throw_sqlite(db_.handle(), "read next report sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace reports
