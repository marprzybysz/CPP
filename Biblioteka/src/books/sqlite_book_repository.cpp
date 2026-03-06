#include "books/sqlite_book_repository.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace books {

namespace {

void throw_sqlite(sqlite3* db, const char* msg) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(msg) + " | sqlite: " + err);
}

std::string column_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

std::optional<std::string> column_optional_text(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return column_text(stmt, col);
}

std::optional<int> column_optional_int(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return sqlite3_column_int(stmt, col);
}

std::string to_storage_tokens(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "";
    }

    std::ostringstream out;
    out << ',';
    for (const auto& value : values) {
        out << value << ',';
    }
    return out.str();
}

std::vector<std::string> from_storage_tokens(const std::string& value) {
    std::vector<std::string> out;
    if (value.empty()) {
        return out;
    }

    std::string token;
    for (char ch : value) {
        if (ch == ',') {
            if (!token.empty()) {
                out.push_back(token);
                token.clear();
            }
            continue;
        }
        token.push_back(ch);
    }

    if (!token.empty()) {
        out.push_back(token);
    }

    return out;
}

Book map_book(sqlite3_stmt* stmt) {
    Book book;
    book.id = sqlite3_column_int(stmt, 0);
    book.public_id = column_text(stmt, 1);
    book.title = column_text(stmt, 2);
    book.author = column_text(stmt, 3);
    book.isbn = column_text(stmt, 4);
    book.categories = from_storage_tokens(column_text(stmt, 5));
    book.tags = from_storage_tokens(column_text(stmt, 6));
    book.edition = column_text(stmt, 7);
    book.publisher = column_text(stmt, 8);
    book.publication_year = column_optional_int(stmt, 9);
    book.description = column_text(stmt, 10);
    book.created_at = column_text(stmt, 11);
    book.updated_at = column_text(stmt, 12);
    book.archived_at = column_optional_text(stmt, 13);
    book.is_archived = sqlite3_column_int(stmt, 14) != 0;
    return book;
}

constexpr const char* kSelectColumns =
    "SELECT id, public_id, title, author, isbn, categories, tags, edition, publisher, publication_year, "
    "description, created_at, updated_at, archived_at, is_archived FROM books";

void bind_common_fields(sqlite3_stmt* stmt, const Book& book, int start_index) {
    sqlite3_bind_text(stmt, start_index + 0, book.public_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 1, book.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 2, book.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 3, book.isbn.c_str(), -1, SQLITE_TRANSIENT);

    const std::string categories = to_storage_tokens(book.categories);
    const std::string tags = to_storage_tokens(book.tags);

    sqlite3_bind_text(stmt, start_index + 4, categories.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 5, tags.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 6, book.edition.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, start_index + 7, book.publisher.c_str(), -1, SQLITE_TRANSIENT);

    if (book.publication_year.has_value()) {
        sqlite3_bind_int(stmt, start_index + 8, *book.publication_year);
    } else {
        sqlite3_bind_null(stmt, start_index + 8);
    }

    sqlite3_bind_text(stmt, start_index + 9, book.description.c_str(), -1, SQLITE_TRANSIENT);
}

std::string normalize_query_token(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };
    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }
    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    value = std::string(begin, end);

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    return value;
}

} // namespace

Book SqliteBookRepository::create(const Book& book) {
    const char* sql =
        "INSERT INTO books (public_id, title, author, isbn, categories, tags, edition, publisher, publication_year, "
        "description, created_at, updated_at, archived_at, is_archived) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'), NULL, 0);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create book failed");
    }

    bind_common_fields(stmt, book, 1);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "insert book failed");
    }

    sqlite3_finalize(stmt);
    return get_by_public_id(book.public_id, true).value();
}

Book SqliteBookRepository::update(const Book& book) {
    const char* sql =
        "UPDATE books SET "
        "title = ?, author = ?, isbn = ?, categories = ?, tags = ?, edition = ?, publisher = ?, publication_year = ?, "
        "description = ?, updated_at = datetime('now') "
        "WHERE public_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare update book failed");
    }

    sqlite3_bind_text(stmt, 1, book.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, book.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book.isbn.c_str(), -1, SQLITE_TRANSIENT);

    const std::string categories = to_storage_tokens(book.categories);
    const std::string tags = to_storage_tokens(book.tags);

    sqlite3_bind_text(stmt, 4, categories.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, tags.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, book.edition.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, book.publisher.c_str(), -1, SQLITE_TRANSIENT);

    if (book.publication_year.has_value()) {
        sqlite3_bind_int(stmt, 8, *book.publication_year);
    } else {
        sqlite3_bind_null(stmt, 8);
    }

    sqlite3_bind_text(stmt, 9, book.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, book.public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "update book failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Book not found. public_id=" + book.public_id);
    }

    return get_by_public_id(book.public_id, true).value();
}

void SqliteBookRepository::archive_by_public_id(const std::string& public_id) {
    const char* sql =
        "UPDATE books SET is_archived = 1, archived_at = datetime('now'), updated_at = datetime('now') "
        "WHERE public_id = ? AND is_archived = 0;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare archive book failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "archive book failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Book not found or already archived. public_id=" + public_id);
    }
}

std::optional<Book> SqliteBookRepository::get_by_public_id(const std::string& public_id, bool include_archived) const {
    std::string sql = std::string(kSelectColumns) + " WHERE public_id = ?";
    if (!include_archived) {
        sql += " AND is_archived = 0";
    }
    sql += ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get by public_id failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);

    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        Book book = map_book(stmt);
        sqlite3_finalize(stmt);
        return book;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "select by public_id failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Book> SqliteBookRepository::list(bool include_archived, int limit, int offset) const {
    std::string sql = std::string(kSelectColumns);
    if (!include_archived) {
        sql += " WHERE is_archived = 0";
    }
    sql += " ORDER BY id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list books failed");
    }

    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);

    std::vector<Book> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_book(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list books failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::vector<Book> SqliteBookRepository::search(const BookQuery& query) const {
    std::string sql = std::string(kSelectColumns) + " WHERE 1=1";
    std::vector<std::string> text_bindings;

    if (!query.include_archived) {
        sql += " AND is_archived = 0";
    }

    if (query.text.has_value() && !query.text->empty()) {
        sql += " AND (title LIKE ? OR author LIKE ? OR isbn LIKE ?)";
        const std::string pattern = "%" + *query.text + "%";
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
        text_bindings.push_back(pattern);
    }

    if (query.title.has_value() && !query.title->empty()) {
        sql += " AND title LIKE ?";
        text_bindings.push_back("%" + *query.title + "%");
    }

    if (query.author.has_value() && !query.author->empty()) {
        sql += " AND author LIKE ?";
        text_bindings.push_back("%" + *query.author + "%");
    }

    if (query.isbn.has_value() && !query.isbn->empty()) {
        sql += " AND isbn LIKE ?";
        text_bindings.push_back("%" + *query.isbn + "%");
    }

    for (const auto& category : query.categories) {
        sql += " AND categories LIKE ?";
        text_bindings.push_back("%," + normalize_query_token(category) + ",%");
    }

    for (const auto& tag : query.tags) {
        sql += " AND tags LIKE ?";
        text_bindings.push_back("%," + normalize_query_token(tag) + ",%");
    }

    sql += " ORDER BY id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare search books failed");
    }

    int bind_index = 1;
    for (const auto& binding : text_bindings) {
        sqlite3_bind_text(stmt, bind_index++, binding.c_str(), -1, SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(stmt, bind_index++, query.limit);
    sqlite3_bind_int(stmt, bind_index, query.offset);

    std::vector<Book> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_book(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "search books failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

std::uint64_t SqliteBookRepository::next_public_sequence(int year) const {
    const std::string prefix = "BOOK-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";

    const char* sql = "SELECT public_id FROM books WHERE public_id LIKE ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next sequence failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::uint64_t max_sequence = 0;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const std::string public_id = column_text(stmt, 0);
            const std::size_t pos = public_id.rfind('-');
            if (pos == std::string::npos || pos + 1 >= public_id.size()) {
                continue;
            }

            const std::string suffix = public_id.substr(pos + 1);
            try {
                const std::uint64_t sequence = std::stoull(suffix);
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
            throw_sqlite(db_.handle(), "read next sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace books
