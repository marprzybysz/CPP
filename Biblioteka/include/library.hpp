#pragma once
#include "book.hpp"
#include "db.hpp"
#include <vector>

class Library {
public:
    explicit Library(Db& db) : db_(db) {}

    void add_book(const Book& book);
    std::vector<Book> list_books() const;
    void display_books() const;

private:
    Db& db_;
};