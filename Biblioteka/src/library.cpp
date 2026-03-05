#include "library.hpp"
#include <iostream>
#include <stdexcept>

static void throw_sqlite(sqlite3* db, const char* msg) {
    throw std::runtime_error(std::string(msg) + " | sqlite: " + sqlite3_errmsg(db));
}

void Library::add_book(const Book& book) {
    const char* sql = "INSERT INTO books (title, author) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw_sqlite(db_.handle(), "prepare failed");

    sqlite3_bind_text(stmt, 1, book.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, book.author.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "insert failed");
    }

    sqlite3_finalize(stmt);
}

std::vector<Book> Library::list_books() const {
    const char* sql = "SELECT id, title, author FROM books ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw_sqlite(db_.handle(), "prepare failed");

    std::vector<Book> out;

    while (true) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            Book b;
            b.id = sqlite3_column_int(stmt, 0);
            b.title  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            b.author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            out.push_back(std::move(b));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "select failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

void Library::display_books() const {
    for (const auto& b : list_books()) {
        std::cout << *b.id << ". " << b.title << " by " << b.author << "\n";
    }
}