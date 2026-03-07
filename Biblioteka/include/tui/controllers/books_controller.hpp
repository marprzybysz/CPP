#pragma once

#include <string>
#include <vector>

#include "books/book.hpp"

class Library;

namespace tui {

class BooksController {
public:
    explicit BooksController(Library& library);

    [[nodiscard]] std::vector<books::Book> list_recent(int limit = 20) const;
    [[nodiscard]] std::vector<books::Book> search_by_author(const std::string& author, int limit = 20) const;
    books::Book add_book(const books::CreateBookInput& input);

private:
    Library& library_;
};

} // namespace tui
