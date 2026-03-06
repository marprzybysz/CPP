#include "readers/sqlite_reader_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace readers {

namespace {

constexpr const char* kSelectReaderColumns =
    "SELECT id, public_id, card_number, first_name, last_name, email, phone, account_status, reputation_points, "
    "is_blocked, block_reason, gdpr_consent, created_at, updated_at FROM readers";

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

Reader map_reader(sqlite3_stmt* stmt) {
    Reader reader;
    reader.id = sqlite3_column_int(stmt, 0);
    reader.public_id = read_text(stmt, 1);
    reader.card_number = read_text(stmt, 2);
    reader.first_name = read_text(stmt, 3);
    reader.last_name = read_text(stmt, 4);
    reader.email = read_text(stmt, 5);
    reader.phone = read_optional_text(stmt, 6);
    reader.account_status = account_status_from_string(read_text(stmt, 7));
    reader.reputation_points = sqlite3_column_int(stmt, 8);
    reader.is_blocked = sqlite3_column_int(stmt, 9) != 0;
    reader.block_reason = read_optional_text(stmt, 10);
    reader.gdpr_consent = sqlite3_column_int(stmt, 11) != 0;
    reader.created_at = read_text(stmt, 12);
    reader.updated_at = read_text(stmt, 13);
    return reader;
}

void bind_optional_text(sqlite3_stmt* stmt, int idx, const std::optional<std::string>& value) {
    if (value.has_value()) {
        sqlite3_bind_text(stmt, idx, value->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

} // namespace

Reader SqliteReaderRepository::create(const Reader& reader) {
    const char* sql =
        "INSERT INTO readers (public_id, card_number, first_name, last_name, email, phone, account_status, "
        "reputation_points, is_blocked, block_reason, gdpr_consent, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'));";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create reader failed");
    }

    sqlite3_bind_text(stmt, 1, reader.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, reader.card_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, reader.first_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, reader.last_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, reader.email.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 6, reader.phone);

    const std::string status = to_string(reader.account_status);
    sqlite3_bind_text(stmt, 7, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, reader.reputation_points);
    sqlite3_bind_int(stmt, 9, reader.is_blocked ? 1 : 0);
    bind_optional_text(stmt, 10, reader.block_reason);
    sqlite3_bind_int(stmt, 11, reader.gdpr_consent ? 1 : 0);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create reader failed");
    }

    sqlite3_finalize(stmt);
    return get_by_public_id(reader.public_id).value();
}

Reader SqliteReaderRepository::update(const Reader& reader) {
    const char* sql =
        "UPDATE readers SET first_name = ?, last_name = ?, email = ?, phone = ?, account_status = ?, "
        "reputation_points = ?, gdpr_consent = ?, updated_at = datetime('now') WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update reader failed");
    }

    sqlite3_bind_text(stmt, 1, reader.first_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, reader.last_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, reader.email.c_str(), -1, SQLITE_TRANSIENT);
    bind_optional_text(stmt, 4, reader.phone);

    const std::string status = to_string(reader.account_status);
    sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(stmt, 6, reader.reputation_points);
    sqlite3_bind_int(stmt, 7, reader.gdpr_consent ? 1 : 0);
    sqlite3_bind_text(stmt, 8, reader.public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update reader failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Reader not found. public_id=" + reader.public_id);
    }

    return get_by_public_id(reader.public_id).value();
}

std::optional<Reader> SqliteReaderRepository::get_by_public_id(const std::string& public_id) const {
    std::string sql = std::string(kSelectReaderColumns) + " WHERE public_id = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get_by_public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Reader reader = map_reader(stmt);
        sqlite3_finalize(stmt);
        return reader;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get_by_public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<Reader> SqliteReaderRepository::get_by_card_number(const std::string& card_number) const {
    std::string sql = std::string(kSelectReaderColumns) + " WHERE card_number = ? LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get_by_card_number failed");
    }

    sqlite3_bind_text(stmt, 1, card_number.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Reader reader = map_reader(stmt);
        sqlite3_finalize(stmt);
        return reader;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get_by_card_number failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Reader> SqliteReaderRepository::search(const ReaderQuery& query) const {
    std::string sql = std::string(kSelectReaderColumns) + " WHERE 1=1";
    std::vector<std::string> bindings;

    if (query.text.has_value() && !query.text->empty()) {
        sql += " AND (first_name LIKE ? OR last_name LIKE ? OR email LIKE ? OR card_number LIKE ?)";
        const std::string pattern = "%" + *query.text + "%";
        bindings.push_back(pattern);
        bindings.push_back(pattern);
        bindings.push_back(pattern);
        bindings.push_back(pattern);
    }

    if (query.card_number.has_value() && !query.card_number->empty()) {
        sql += " AND card_number LIKE ?";
        bindings.push_back("%" + *query.card_number + "%");
    }

    if (query.email.has_value() && !query.email->empty()) {
        sql += " AND email LIKE ?";
        bindings.push_back("%" + *query.email + "%");
    }

    if (query.last_name.has_value() && !query.last_name->empty()) {
        sql += " AND last_name LIKE ?";
        bindings.push_back("%" + *query.last_name + "%");
    }

    sql += " ORDER BY id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare search readers failed");
    }

    int idx = 1;
    for (const auto& value : bindings) {
        sqlite3_bind_text(stmt, idx++, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(stmt, idx++, query.limit);
    sqlite3_bind_int(stmt, idx, query.offset);

    std::vector<Reader> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_reader(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "search readers failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

bool SqliteReaderRepository::email_exists(const std::string& email, const std::string* excluded_public_id) const {
    std::string sql = "SELECT 1 FROM readers WHERE email = ?";
    if (excluded_public_id != nullptr) {
        sql += " AND public_id != ?";
    }
    sql += " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare email_exists failed");
    }

    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    if (excluded_public_id != nullptr) {
        sqlite3_bind_text(stmt, 2, excluded_public_id->c_str(), -1, SQLITE_TRANSIENT);
    }

    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "email_exists failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool SqliteReaderRepository::card_number_exists(const std::string& card_number,
                                                const std::string* excluded_public_id) const {
    std::string sql = "SELECT 1 FROM readers WHERE card_number = ?";
    if (excluded_public_id != nullptr) {
        sql += " AND public_id != ?";
    }
    sql += " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare card_number_exists failed");
    }

    sqlite3_bind_text(stmt, 1, card_number.c_str(), -1, SQLITE_TRANSIENT);
    if (excluded_public_id != nullptr) {
        sqlite3_bind_text(stmt, 2, excluded_public_id->c_str(), -1, SQLITE_TRANSIENT);
    }

    const int rc = sqlite3_step(stmt);
    const bool exists = (rc == SQLITE_ROW);

    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "card_number_exists failed");
    }

    sqlite3_finalize(stmt);
    return exists;
}

Reader SqliteReaderRepository::set_block_state(const std::string& public_id,
                                                bool is_blocked,
                                                const std::optional<std::string>& block_reason,
                                                AccountStatus account_status) {
    const char* sql =
        "UPDATE readers SET is_blocked = ?, block_reason = ?, account_status = ?, updated_at = datetime('now') "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare set_block_state failed");
    }

    sqlite3_bind_int(stmt, 1, is_blocked ? 1 : 0);
    bind_optional_text(stmt, 2, block_reason);

    const std::string status = to_string(account_status);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "set_block_state failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Reader not found. public_id=" + public_id);
    }

    return get_by_public_id(public_id).value();
}

std::uint64_t SqliteReaderRepository::next_card_sequence() const {
    const char* sql = "SELECT card_number FROM readers WHERE card_number LIKE 'CARD-%';";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next_card_sequence failed");
    }

    std::uint64_t max_sequence = 0;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const std::string card = read_text(stmt, 0);
            if (card.size() <= 5) {
                continue;
            }

            try {
                const std::uint64_t seq = std::stoull(card.substr(5));
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
            throw_sqlite(db_.handle(), "read next_card_sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

std::vector<ReaderLoanHistoryEntry> SqliteReaderRepository::list_loan_history(const std::string& reader_public_id) const {
    const char* sql =
        "SELECT id, reader_public_id, loan_public_id, action, action_at, note FROM reader_loan_history "
        "WHERE reader_public_id = ? ORDER BY action_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list_loan_history failed");
    }

    sqlite3_bind_text(stmt, 1, reader_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<ReaderLoanHistoryEntry> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            ReaderLoanHistoryEntry item;
            item.id = sqlite3_column_int(stmt, 0);
            item.reader_public_id = read_text(stmt, 1);
            item.loan_public_id = read_text(stmt, 2);
            item.action = read_text(stmt, 3);
            item.action_at = read_text(stmt, 4);
            item.note = read_text(stmt, 5);
            out.push_back(std::move(item));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list_loan_history failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<ReaderNote> SqliteReaderRepository::list_notes(const std::string& reader_public_id) const {
    const char* sql =
        "SELECT id, reader_public_id, note, created_at FROM reader_notes "
        "WHERE reader_public_id = ? ORDER BY created_at DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list_notes failed");
    }

    sqlite3_bind_text(stmt, 1, reader_public_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<ReaderNote> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            ReaderNote item;
            item.id = sqlite3_column_int(stmt, 0);
            item.reader_public_id = read_text(stmt, 1);
            item.note = read_text(stmt, 2);
            item.created_at = read_text(stmt, 3);
            out.push_back(std::move(item));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list_notes failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace readers
