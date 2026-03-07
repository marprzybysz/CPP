#include "exports/sqlite_export_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace exports {

namespace {

constexpr const char* kSelectWithdrawalColumns =
    "SELECT id, copy_public_id, reason, withdrawal_date, operator_name, note, resulting_status, created_at "
    "FROM copy_withdrawals";

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

void bind_optional_text(sqlite3_stmt* stmt, int idx, const std::string& value) {
    if (value.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT);
    }
}

CopyWithdrawal map_withdrawal(sqlite3_stmt* stmt) {
    CopyWithdrawal out;
    out.id = sqlite3_column_int(stmt, 0);
    out.copy_public_id = read_text(stmt, 1);
    out.reason = withdrawal_reason_from_string(read_text(stmt, 2));
    out.withdrawal_date = read_text(stmt, 3);
    out.operator_name = read_text(stmt, 4);
    out.note = read_text(stmt, 5);
    out.resulting_status = copies::copy_status_from_string(read_text(stmt, 6));
    out.created_at = read_text(stmt, 7);
    return out;
}

WithdrawnCopyView map_withdrawn_view(sqlite3_stmt* stmt) {
    WithdrawnCopyView view;

    view.withdrawal.id = sqlite3_column_int(stmt, 0);
    view.withdrawal.copy_public_id = read_text(stmt, 1);
    view.withdrawal.reason = withdrawal_reason_from_string(read_text(stmt, 2));
    view.withdrawal.withdrawal_date = read_text(stmt, 3);
    view.withdrawal.operator_name = read_text(stmt, 4);
    view.withdrawal.note = read_text(stmt, 5);
    view.withdrawal.resulting_status = copies::copy_status_from_string(read_text(stmt, 6));
    view.withdrawal.created_at = read_text(stmt, 7);

    view.inventory_number = read_text(stmt, 8);
    view.book_id = sqlite3_column_int(stmt, 9);
    view.current_status = copies::copy_status_from_string(read_text(stmt, 10));

    return view;
}

} // namespace

bool SqliteExportRepository::withdrawal_exists_for_copy(const std::string& copy_public_id) const {
    const char* sql = "SELECT 1 FROM copy_withdrawals WHERE copy_public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare withdrawal_exists_for_copy failed");
    }

    sqlite3_bind_text(stmt, 1, copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "withdrawal_exists_for_copy query failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

CopyWithdrawal SqliteExportRepository::create_withdrawal(const CopyWithdrawal& withdrawal) {
    const char* sql =
        "INSERT INTO copy_withdrawals (copy_public_id, reason, withdrawal_date, operator_name, note, resulting_status, created_at) "
        "VALUES (?, ?, COALESCE(?, date('now')), ?, ?, ?, datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create withdrawal failed");
    }

    sqlite3_bind_text(stmt, 1, withdrawal.copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string reason = to_string(withdrawal.reason);
    sqlite3_bind_text(stmt, 2, reason.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 3, withdrawal.withdrawal_date);
    sqlite3_bind_text(stmt, 4, withdrawal.operator_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, withdrawal.note.c_str(), -1, SQLITE_TRANSIENT);
    const std::string status = copies::to_string(withdrawal.resulting_status);
    sqlite3_bind_text(stmt, 6, status.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        const int code = sqlite3_errcode(db_.handle());
        sqlite3_finalize(stmt);

        if (code == SQLITE_CONSTRAINT) {
            throw errors::ExportError("Withdrawal for copy already exists. copy_public_id=" + withdrawal.copy_public_id);
        }

        throw_sqlite(db_.handle(), "create withdrawal failed");
    }

    sqlite3_finalize(stmt);

    const std::string fetch_sql = std::string(kSelectWithdrawalColumns) + " WHERE copy_public_id = ? LIMIT 1;";
    sqlite3_stmt* fetch_stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), fetch_sql.c_str(), -1, &fetch_stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare fetch created withdrawal failed");
    }

    sqlite3_bind_text(fetch_stmt, 1, withdrawal.copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(fetch_stmt);

    if (rc == SQLITE_ROW) {
        CopyWithdrawal created = map_withdrawal(fetch_stmt);
        sqlite3_finalize(fetch_stmt);
        return created;
    }

    sqlite3_finalize(fetch_stmt);
    throw errors::DatabaseError("fetch created withdrawal failed");
}

std::vector<WithdrawnCopyView> SqliteExportRepository::list_withdrawn_copies(int limit, int offset) const {
    const std::string sql =
        "SELECT w.id, w.copy_public_id, w.reason, w.withdrawal_date, w.operator_name, w.note, w.resulting_status, w.created_at, "
        "bc.inventory_number, bc.book_id, bc.status "
        "FROM copy_withdrawals w "
        "JOIN book_copies bc ON bc.public_id = w.copy_public_id "
        "ORDER BY w.withdrawal_date DESC, w.id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list withdrawn copies failed");
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    std::vector<WithdrawnCopyView> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_withdrawn_view(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list withdrawn copies failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace exports
