#pragma once
#include "books/book.hpp"
#include "books/book_service.hpp"
#include "books/sqlite_book_repository.hpp"
#include "common/system_id.hpp"
#include "db.hpp"
#include <string>
#include <vector>

class Library {
public:
    explicit Library(Db& db);

    books::Book add_book(const books::CreateBookInput& input);
    books::Book edit_book(const std::string& public_id, const books::UpdateBookInput& input);
    void archive_book(const std::string& public_id);
    books::Book get_book_details(const std::string& public_id, bool include_archived = false) const;
    std::vector<books::Book> list_books(bool include_archived = false, int limit = 100, int offset = 0) const;
    std::vector<books::Book> search_books(const books::BookQuery& query) const;
    void display_books(const std::vector<books::Book>& books) const;

private:
    Db& db_;
    common::SystemIdGenerator id_generator_;
    books::SqliteBookRepository book_repository_;
    books::BookService book_service_;
};
