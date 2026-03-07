#include "tui/controllers/books_controller.hpp"

#include "library.hpp"

namespace tui {

BooksController::BooksController(Library& library) : library_(library) {}

std::vector<books::Book> BooksController::list_recent(int limit) const {
    return library_.list_books(false, limit, 0);
}

std::vector<books::Book> BooksController::search_by_author(const std::string& author, int limit) const {
    books::BookQuery query;
    query.author = author;
    query.limit = limit;
    query.offset = 0;
    return library_.search_books(query);
}

books::Book BooksController::add_book(const books::CreateBookInput& input) {
    return library_.add_book(input);
}

} // namespace tui
