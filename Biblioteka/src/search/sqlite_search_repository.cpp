#include "search/sqlite_search_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace search {

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

} // namespace

std::vector<BookSearchHit> SqliteSearchRepository::search_books(const std::string& pattern, int limit) const {
    const char* sql =
        "SELECT public_id, title, author, isbn, is_archived "
        "FROM books "
        "WHERE title LIKE ? OR author LIKE ? OR isbn LIKE ? "
        "ORDER BY is_archived ASC, title ASC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare search_books failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, limit);

    std::vector<BookSearchHit> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            BookSearchHit hit;
            hit.book_public_id = read_text(stmt, 0);
            hit.title = read_text(stmt, 1);
            hit.author = read_text(stmt, 2);
            hit.isbn = read_text(stmt, 3);
            hit.is_archived = sqlite3_column_int(stmt, 4) != 0;
            out.push_back(std::move(hit));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "search_books failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<CopySearchHit> SqliteSearchRepository::search_copies(const std::string& pattern, int limit) const {
    const char* sql =
        "SELECT bc.public_id, bc.inventory_number, bc.status, "
        "b.public_id, b.title, b.author, "
        "bc.current_location_id, lcur.name, bc.target_location_id, ltar.name, "
        "rd.public_id, rd.card_number, rd.first_name, rd.last_name "
        "FROM book_copies bc "
        "JOIN books b ON b.id = bc.book_id "
        "LEFT JOIN locations lcur ON lcur.public_id = bc.current_location_id "
        "LEFT JOIN locations ltar ON ltar.public_id = bc.target_location_id "
        "LEFT JOIN reservations r ON r.copy_id = bc.id AND r.status = 'ACTIVE' "
        "LEFT JOIN readers rd ON rd.id = r.reader_id "
        "WHERE bc.public_id LIKE ? OR bc.inventory_number LIKE ? OR b.title LIKE ? OR b.author LIKE ? OR b.isbn LIKE ? "
        "ORDER BY bc.id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare search_copies failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, limit);

    std::vector<CopySearchHit> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            CopySearchHit hit;
            hit.copy_public_id = read_text(stmt, 0);
            hit.inventory_number = read_text(stmt, 1);
            hit.status = copies::copy_status_from_string(read_text(stmt, 2));
            hit.book_public_id = read_text(stmt, 3);
            hit.book_title = read_text(stmt, 4);
            hit.book_author = read_text(stmt, 5);
            hit.current_location_public_id = read_optional_text(stmt, 6);
            hit.current_location_name = read_optional_text(stmt, 7);
            hit.target_location_public_id = read_optional_text(stmt, 8);
            hit.target_location_name = read_optional_text(stmt, 9);

            const std::optional<std::string> holder_public_id = read_optional_text(stmt, 10);
            if (holder_public_id.has_value() && !holder_public_id->empty()) {
                CopyHolderInfo holder;
                holder.reader_public_id = *holder_public_id;
                holder.card_number = read_text(stmt, 11);
                holder.full_name = read_text(stmt, 12) + " " + read_text(stmt, 13);
                hit.holder = holder;
            }

            out.push_back(std::move(hit));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "search_copies failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<ReaderSearchHit> SqliteSearchRepository::search_readers(const std::string& pattern, int limit) const {
    const char* sql =
        "SELECT public_id, card_number, first_name, last_name, email, is_blocked "
        "FROM readers "
        "WHERE public_id LIKE ? OR card_number LIKE ? OR first_name LIKE ? OR last_name LIKE ? OR email LIKE ? "
        "ORDER BY id DESC LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare search_readers failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, limit);

    std::vector<ReaderSearchHit> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            ReaderSearchHit hit;
            hit.reader_public_id = read_text(stmt, 0);
            hit.card_number = read_text(stmt, 1);
            hit.first_name = read_text(stmt, 2);
            hit.last_name = read_text(stmt, 3);
            hit.email = read_text(stmt, 4);
            hit.is_blocked = sqlite3_column_int(stmt, 5) != 0;
            out.push_back(std::move(hit));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "search_readers failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

} // namespace search
