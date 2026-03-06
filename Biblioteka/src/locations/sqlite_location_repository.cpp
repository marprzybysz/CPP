#include "locations/sqlite_location_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace locations {

namespace {

constexpr const char* kSelectLocationColumns =
    "SELECT id, public_id, name, type, parent_id, code, description FROM locations";

constexpr const char* kSelectCopyColumns =
    "SELECT id, public_id, book_id, inventory_number, status, target_location_id, current_location_id, condition_note, "
    "acquisition_date, archival_reason, created_at, updated_at FROM book_copies";

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

Location map_location(sqlite3_stmt* stmt) {
    Location location;
    location.id = sqlite3_column_int(stmt, 0);
    location.public_id = read_text(stmt, 1);
    location.name = read_text(stmt, 2);
    location.type = location_type_from_string(read_text(stmt, 3));
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

void bind_optional_int(sqlite3_stmt* stmt, int index, const std::optional<int>& value) {
    if (value.has_value()) {
        sqlite3_bind_int(stmt, index, *value);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

} // namespace

Location SqliteLocationRepository::create(const Location& location) {
    const char* sql =
        "INSERT INTO locations (public_id, name, type, parent_id, code, description) VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create location failed");
    }

    sqlite3_bind_text(stmt, 1, location.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, location.name.c_str(), -1, SQLITE_TRANSIENT);
    const std::string type = to_string(location.type);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_int(stmt, 4, location.parent_id);
    sqlite3_bind_text(stmt, 5, location.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, location.description.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create location failed");
    }

    sqlite3_finalize(stmt);
    return get_by_public_id(location.public_id).value();
}

Location SqliteLocationRepository::update(const Location& location) {
    const char* sql =
        "UPDATE locations SET name = ?, type = ?, parent_id = ?, code = ?, description = ? WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update location failed");
    }

    sqlite3_bind_text(stmt, 1, location.name.c_str(), -1, SQLITE_TRANSIENT);
    const std::string type = to_string(location.type);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_int(stmt, 3, location.parent_id);
    sqlite3_bind_text(stmt, 4, location.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, location.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, location.public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update location failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Location not found. public_id=" + location.public_id);
    }

    return get_by_public_id(location.public_id).value();
}

std::optional<Location> SqliteLocationRepository::get_by_id(int id) const {
    std::string sql = std::string(kSelectLocationColumns) + " WHERE id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get location by id failed");
    }

    sqlite3_bind_int(stmt, 1, id);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Location location = map_location(stmt);
        sqlite3_finalize(stmt);
        return location;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get location by id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Location> SqliteLocationRepository::get_by_public_id(const std::string& public_id) const {
    std::string sql = std::string(kSelectLocationColumns) + " WHERE public_id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get location by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Location location = map_location(stmt);
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

std::vector<Location> SqliteLocationRepository::list_all() const {
    std::string sql = std::string(kSelectLocationColumns) + " ORDER BY id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list locations failed");
    }

    std::vector<Location> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_location(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list locations failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

bool SqliteLocationRepository::code_exists(const std::string& code, const std::string* excluded_public_id) const {
    std::string sql = "SELECT 1 FROM locations WHERE code = ?";
    if (excluded_public_id != nullptr) {
        sql += " AND public_id != ?";
    }
    sql += " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare code_exists failed");
    }

    sqlite3_bind_text(stmt, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    if (excluded_public_id != nullptr) {
        sqlite3_bind_text(stmt, 2, excluded_public_id->c_str(), -1, SQLITE_TRANSIENT);
    }

    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "code_exists failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::uint64_t SqliteLocationRepository::next_public_sequence(int year) const {
    const std::string prefix = "LOC-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";
    const char* sql = "SELECT public_id FROM locations WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next location sequence failed");
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
            throw_sqlite(db_.handle(), "read next location sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

std::vector<copies::BookCopy> SqliteLocationRepository::list_assigned_copies(const std::string& location_public_id) const {
    const std::string sql = std::string(kSelectCopyColumns) +
                            " WHERE current_location_id = ? OR target_location_id = ? ORDER BY id DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list_assigned_copies failed");
    }

    sqlite3_bind_text(stmt, 1, location_public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, location_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<copies::BookCopy> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_copy(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list_assigned_copies failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace locations
