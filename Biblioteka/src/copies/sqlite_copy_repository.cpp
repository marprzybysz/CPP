#include "copies/sqlite_copy_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace copies {

namespace {

constexpr const char* kSelectCopyColumns =
    "SELECT id, public_id, book_id, inventory_number, status, target_location_id, current_location_id, condition_note, "
    "acquisition_date, archival_reason, created_at, updated_at FROM book_copies";

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::optional<std::string> read_optional_text(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? std::optional<std::string>(reinterpret_cast<const char*>(raw)) : std::nullopt;
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

BookCopy map_copy(sqlite3_stmt* stmt) {
    BookCopy copy;
    copy.id = sqlite3_column_int(stmt, 0);
    copy.public_id = read_text(stmt, 1);
    copy.book_id = sqlite3_column_int(stmt, 2);
    copy.inventory_number = read_text(stmt, 3);
    copy.status = copy_status_from_string(read_text(stmt, 4));
    copy.target_location_id = read_optional_text(stmt, 5);
    copy.current_location_id = read_optional_text(stmt, 6);
    copy.condition_note = read_text(stmt, 7);
    copy.acquisition_date = read_optional_text(stmt, 8);
    copy.archival_reason = read_optional_text(stmt, 9);
    copy.created_at = read_text(stmt, 10);
    copy.updated_at = read_text(stmt, 11);
    return copy;
}

void bind_optional_text(sqlite3_stmt* stmt, int index, const std::optional<std::string>& value) {
    if (value.has_value()) {
        sqlite3_bind_text(stmt, index, value->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

void insert_status_history(sqlite3* db,
                           const std::string& copy_public_id,
                           CopyStatus from_status,
                           CopyStatus to_status,
                           const std::string& note) {
    const char* sql =
        "INSERT INTO copy_status_history (copy_public_id, from_status, to_status, changed_at, note) "
        "VALUES (?, ?, ?, datetime('now'), ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db, "prepare insert status history failed");
    }

    sqlite3_bind_text(stmt, 1, copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string from = to_string(from_status);
    const std::string to = to_string(to_status);
    sqlite3_bind_text(stmt, 2, from.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, to.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, note.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db, "insert status history failed");
    }

    sqlite3_finalize(stmt);
}

void insert_location_history(sqlite3* db,
                             const std::string& copy_public_id,
                             const std::optional<std::string>& from_current,
                             const std::optional<std::string>& to_current,
                             const std::optional<std::string>& from_target,
                             const std::optional<std::string>& to_target,
                             const std::string& note) {
    const char* sql =
        "INSERT INTO copy_location_history (copy_public_id, from_location_id, to_location_id, "
        "from_target_location_id, to_target_location_id, changed_at, note) "
        "VALUES (?, ?, ?, ?, ?, datetime('now'), ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db, "prepare insert location history failed");
    }

    sqlite3_bind_text(stmt, 1, copy_public_id.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 2, from_current);
    bind_optional_text(stmt, 3, to_current);
    bind_optional_text(stmt, 4, from_target);
    bind_optional_text(stmt, 5, to_target);
    sqlite3_bind_text(stmt, 6, note.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db, "insert location history failed");
    }

    sqlite3_finalize(stmt);
}

} // namespace

bool SqliteCopyRepository::book_exists(int book_id) const {
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
        throw_sqlite(db_.handle(), "book_exists query failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool SqliteCopyRepository::location_exists(const std::string& location_public_id) const {
    const char* sql = "SELECT 1 FROM locations WHERE public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare location_exists failed");
    }

    sqlite3_bind_text(stmt, 1, location_public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "location_exists query failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool SqliteCopyRepository::inventory_number_exists(const std::string& inventory_number,
                                                   const std::string* excluded_public_id) const {
    std::string sql = "SELECT 1 FROM book_copies WHERE inventory_number = ?";
    if (excluded_public_id != nullptr) {
        sql += " AND public_id != ?";
    }
    sql += " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare inventory_number_exists failed");
    }

    sqlite3_bind_text(stmt, 1, inventory_number.c_str(), -1, SQLITE_TRANSIENT);
    if (excluded_public_id != nullptr) {
        sqlite3_bind_text(stmt, 2, excluded_public_id->c_str(), -1, SQLITE_TRANSIENT);
    }

    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "inventory_number_exists query failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

BookCopy SqliteCopyRepository::create(const BookCopy& copy) {
    const char* sql =
        "INSERT INTO book_copies (public_id, book_id, inventory_number, status, target_location_id, current_location_id, "
        "condition_note, acquisition_date, archival_reason, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create copy failed");
    }

    sqlite3_bind_text(stmt, 1, copy.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, copy.book_id);
    sqlite3_bind_text(stmt, 3, copy.inventory_number.c_str(), -1, SQLITE_TRANSIENT);

    const std::string status = to_string(copy.status);
    sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 5, copy.target_location_id);
    bind_optional_text(stmt, 6, copy.current_location_id);
    sqlite3_bind_text(stmt, 7, copy.condition_note.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 8, copy.acquisition_date);
    bind_optional_text(stmt, 9, copy.archival_reason);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "insert copy failed");
    }

    sqlite3_finalize(stmt);

    insert_status_history(db_.handle(), copy.public_id, copy.status, copy.status, "copy created");
    insert_location_history(
        db_.handle(), copy.public_id, std::nullopt, copy.current_location_id, std::nullopt, copy.target_location_id, "copy created");

    return get_by_public_id(copy.public_id).value();
}

BookCopy SqliteCopyRepository::update(const BookCopy& copy) {
    const char* sql =
        "UPDATE book_copies SET inventory_number = ?, condition_note = ?, acquisition_date = ?, archival_reason = ?, "
        "updated_at = datetime('now') WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update copy failed");
    }

    sqlite3_bind_text(stmt, 1, copy.inventory_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, copy.condition_note.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 3, copy.acquisition_date);
    bind_optional_text(stmt, 4, copy.archival_reason);
    sqlite3_bind_text(stmt, 5, copy.public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update copy failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Copy not found. public_id=" + copy.public_id);
    }

    return get_by_public_id(copy.public_id).value();
}

std::optional<BookCopy> SqliteCopyRepository::get_by_public_id(const std::string& public_id) const {
    std::string sql = std::string(kSelectCopyColumns) + " WHERE public_id = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get copy failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        BookCopy copy = map_copy(stmt);
        sqlite3_finalize(stmt);
        return copy;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get copy failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<BookCopy> SqliteCopyRepository::list_by_book_id(int book_id, int limit, int offset) const {
    std::string sql = std::string(kSelectCopyColumns) + " WHERE book_id = ? ORDER BY id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list copies failed");
    }

    sqlite3_bind_int(stmt, 1, book_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    std::vector<BookCopy> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_copy(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list copies failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

BookCopy SqliteCopyRepository::update_status(const std::string& public_id,
                                             CopyStatus new_status,
                                             const std::string& note,
                                             const std::optional<std::string>& archival_reason) {
    const auto existing = get_by_public_id(public_id);
    if (!existing.has_value()) {
        throw errors::NotFoundError("Copy not found. public_id=" + public_id);
    }

    const char* sql =
        "UPDATE book_copies SET status = ?, archival_reason = ?, updated_at = datetime('now') "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update status failed");
    }

    const std::string status = to_string(new_status);
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 2, archival_reason);
    sqlite3_bind_text(stmt, 3, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update status failed");
    }

    sqlite3_finalize(stmt);

    insert_status_history(db_.handle(), public_id, existing->status, new_status, note);

    return get_by_public_id(public_id).value();
}

BookCopy SqliteCopyRepository::update_location(const std::string& public_id,
                                               const std::optional<std::string>& current_location_id,
                                               const std::optional<std::string>& target_location_id,
                                               const std::string& note) {
    const auto existing = get_by_public_id(public_id);
    if (!existing.has_value()) {
        throw errors::NotFoundError("Copy not found. public_id=" + public_id);
    }

    const char* sql =
        "UPDATE book_copies SET current_location_id = ?, target_location_id = ?, updated_at = datetime('now') "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update location failed");
    }

    bind_optional_text(stmt, 1, current_location_id);
    bind_optional_text(stmt, 2, target_location_id);
    sqlite3_bind_text(stmt, 3, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update location failed");
    }

    sqlite3_finalize(stmt);

    insert_location_history(db_.handle(),
                            public_id,
                            existing->current_location_id,
                            current_location_id,
                            existing->target_location_id,
                            target_location_id,
                            note);

    return get_by_public_id(public_id).value();
}

std::uint64_t SqliteCopyRepository::next_public_sequence(int year) const {
    const std::string prefix = "COPY-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";
    const char* sql = "SELECT public_id FROM book_copies WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next copy sequence failed");
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
            throw_sqlite(db_.handle(), "read next copy sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace copies
