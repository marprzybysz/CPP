#include "reservations/sqlite_reservation_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>

namespace reservations {

namespace {

constexpr const char* kSelectReservationColumns =
    "SELECT id, public_id, reader_id, copy_id, book_id, reservation_date, expiration_date, status, created_at, updated_at "
    "FROM reservations";

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

Reservation map_reservation(sqlite3_stmt* stmt) {
    Reservation reservation;
    reservation.id = sqlite3_column_int(stmt, 0);
    reservation.public_id = read_text(stmt, 1);
    reservation.reader_id = sqlite3_column_int(stmt, 2);
    reservation.copy_id = read_optional_int(stmt, 3);
    reservation.book_id = read_optional_int(stmt, 4);
    reservation.reservation_date = read_text(stmt, 5);
    reservation.expiration_date = read_text(stmt, 6);
    reservation.status = reservation_status_from_string(read_text(stmt, 7));
    reservation.created_at = read_text(stmt, 8);
    reservation.updated_at = read_text(stmt, 9);
    return reservation;
}

void bind_optional_int(sqlite3_stmt* stmt, int index, const std::optional<int>& value) {
    if (value.has_value()) {
        sqlite3_bind_int(stmt, index, *value);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

} // namespace

Reservation SqliteReservationRepository::create(const Reservation& reservation) {
    const char* sql =
        "INSERT INTO reservations (public_id, reader_id, copy_id, book_id, reservation_date, expiration_date, status, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, date('now'), ?, ?, datetime('now'), datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create reservation failed");
    }

    sqlite3_bind_text(stmt, 1, reservation.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, reservation.reader_id);
    bind_optional_int(stmt, 3, reservation.copy_id);
    bind_optional_int(stmt, 4, reservation.book_id);
    sqlite3_bind_text(stmt, 5, reservation.expiration_date.c_str(), -1, SQLITE_TRANSIENT);
    const std::string status = to_string(reservation.status);
    sqlite3_bind_text(stmt, 6, status.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create reservation failed");
    }

    sqlite3_finalize(stmt);
    return get_by_public_id(reservation.public_id).value();
}

std::optional<Reservation> SqliteReservationRepository::get_by_public_id(const std::string& public_id) const {
    std::string sql = std::string(kSelectReservationColumns) + " WHERE public_id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get reservation failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Reservation reservation = map_reservation(stmt);
        sqlite3_finalize(stmt);
        return reservation;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get reservation failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

Reservation SqliteReservationRepository::update_status(const std::string& public_id, ReservationStatus status) {
    const char* sql = "UPDATE reservations SET status = ?, updated_at = datetime('now') WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update reservation status failed");
    }

    const std::string status_text = to_string(status);
    sqlite3_bind_text(stmt, 1, status_text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update reservation status failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Reservation not found. public_id=" + public_id);
    }

    return get_by_public_id(public_id).value();
}

Reservation SqliteReservationRepository::update_expiration(const std::string& public_id, const std::string& expiration_date) {
    const char* sql = "UPDATE reservations SET expiration_date = ?, updated_at = datetime('now') WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update reservation expiration failed");
    }

    sqlite3_bind_text(stmt, 1, expiration_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update reservation expiration failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Reservation not found. public_id=" + public_id);
    }

    return get_by_public_id(public_id).value();
}

std::vector<LoanListItem> SqliteReservationRepository::list_loans(const LoanListQuery& query) const {
    std::string sql =
        "SELECT r.id, r.public_id, r.reader_id, r.copy_id, r.book_id, r.reservation_date, r.expiration_date, r.status, r.created_at, "
        "r.updated_at, "
        "rd.public_id, rd.first_name, rd.last_name, rd.card_number, "
        "cp.public_id, cp.inventory_number "
        "FROM reservations r "
        "JOIN readers rd ON rd.id = r.reader_id "
        "LEFT JOIN book_copies cp ON cp.id = r.copy_id "
        "LEFT JOIN books bk ON bk.id = COALESCE(r.book_id, cp.book_id) "
        "WHERE 1=1";

    std::vector<std::string> text_bindings;

    if (query.public_id.has_value() && !query.public_id->empty()) {
        sql += " AND r.public_id = ?";
        text_bindings.push_back(*query.public_id);
    }

    if (query.status.has_value()) {
        sql += " AND r.status = ?";
        text_bindings.push_back(to_string(*query.status));
    }

    if (query.reader_query.has_value() && !query.reader_query->empty()) {
        sql += " AND (rd.public_id LIKE ? OR rd.card_number LIKE ? OR rd.first_name LIKE ? OR rd.last_name LIKE ?)";
        const std::string pattern = "%" + *query.reader_query + "%";
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
    }

    if (query.copy_query.has_value() && !query.copy_query->empty()) {
        sql += " AND (cp.public_id LIKE ? OR cp.inventory_number LIKE ? OR bk.title LIKE ?)";
        const std::string pattern = "%" + *query.copy_query + "%";
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
    }

    if (query.card_number.has_value() && !query.card_number->empty()) {
        sql += " AND rd.card_number LIKE ?";
        text_bindings.push_back("%" + *query.card_number + "%");
    }

    if (query.inventory_number.has_value() && !query.inventory_number->empty()) {
        sql += " AND cp.inventory_number LIKE ?";
        text_bindings.push_back("%" + *query.inventory_number + "%");
    }

    sql += " ORDER BY r.created_at DESC, r.id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list loans failed");
    }

    int bind_index = 1;
    for (const auto& text : text_bindings) {
        sqlite3_bind_text(stmt, bind_index++, text.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, bind_index++, query.limit);
    sqlite3_bind_int(stmt, bind_index, query.offset);

    std::vector<LoanListItem> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            LoanListItem item;
            item.reservation = map_reservation(stmt);
            item.reader_public_id = read_text(stmt, 10);
            item.reader_name = read_text(stmt, 11) + " " + read_text(stmt, 12);
            item.card_number = read_text(stmt, 13);
            if (sqlite3_column_type(stmt, 14) != SQLITE_NULL) {
                item.copy_public_id = read_text(stmt, 14);
            }
            if (sqlite3_column_type(stmt, 15) != SQLITE_NULL) {
                item.inventory_number = read_text(stmt, 15);
            }
            item.extension_count = 0;
            out.push_back(std::move(item));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list loans failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

ReaderReservationInfo SqliteReservationRepository::get_reader_info(int reader_id) const {
    const char* sql = "SELECT is_blocked FROM readers WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get_reader_info failed");
    }

    sqlite3_bind_int(stmt, 1, reader_id);
    const int rc = sqlite3_step(stmt);

    ReaderReservationInfo info;
    if (rc == SQLITE_ROW) {
        info.exists = true;
        info.is_blocked = sqlite3_column_int(stmt, 0) != 0;
    } else if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get_reader_info failed");
    }

    sqlite3_finalize(stmt);
    return info;
}

CopyReservationInfo SqliteReservationRepository::get_copy_info(int copy_id) const {
    const char* sql = "SELECT book_id, status FROM book_copies WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get_copy_info failed");
    }

    sqlite3_bind_int(stmt, 1, copy_id);
    const int rc = sqlite3_step(stmt);

    CopyReservationInfo info;
    if (rc == SQLITE_ROW) {
        info.exists = true;
        info.book_id = sqlite3_column_int(stmt, 0);
        info.status = copies::copy_status_from_string(read_text(stmt, 1));
    } else if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get_copy_info failed");
    }

    sqlite3_finalize(stmt);
    return info;
}

bool SqliteReservationRepository::book_exists(int book_id) const {
    const char* sql = "SELECT 1 FROM books WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare book_exists failed");
    }

    sqlite3_bind_int(stmt, 1, book_id);
    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "book_exists failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool SqliteReservationRepository::has_active_reservation_for_copy(int copy_id) const {
    const char* sql = "SELECT 1 FROM reservations WHERE copy_id = ? AND status = 'ACTIVE' LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare has_active_reservation_for_copy failed");
    }

    sqlite3_bind_int(stmt, 1, copy_id);
    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "has_active_reservation_for_copy failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::optional<Reservation> SqliteReservationRepository::find_oldest_active_for_copy(int copy_id) const {
    std::string sql = std::string(kSelectReservationColumns) +
                      " WHERE copy_id = ? AND status = 'ACTIVE' ORDER BY created_at ASC, id ASC LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare find_oldest_active_for_copy failed");
    }

    sqlite3_bind_int(stmt, 1, copy_id);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Reservation reservation = map_reservation(stmt);
        sqlite3_finalize(stmt);
        return reservation;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "find_oldest_active_for_copy failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Reservation> SqliteReservationRepository::find_oldest_active_for_book(int book_id) const {
    std::string sql = std::string(kSelectReservationColumns) +
                      " WHERE book_id = ? AND copy_id IS NULL AND status = 'ACTIVE' ORDER BY created_at ASC, id ASC LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare find_oldest_active_for_book failed");
    }

    sqlite3_bind_int(stmt, 1, book_id);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Reservation reservation = map_reservation(stmt);
        sqlite3_finalize(stmt);
        return reservation;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "find_oldest_active_for_book failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::uint64_t SqliteReservationRepository::next_public_sequence(int year) const {
    const std::string prefix = "RES-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";

    const char* sql = "SELECT public_id FROM reservations WHERE public_id LIKE ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next_public_sequence failed");
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
                const std::uint64_t seq = std::stoull(public_id.substr(pos + 1));
                if (seq > max_sequence) {
                    max_sequence = seq;
                }
            } catch (...) {
                continue;
            }
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "read next_public_sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace reservations
