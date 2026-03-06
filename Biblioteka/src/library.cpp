#include "library.hpp"

#include <iostream>

Library::Library(Db& db)
    : db_(db),
      id_generator_(),
      book_repository_(db_),
      book_service_(book_repository_, id_generator_),
      copy_repository_(db_),
      copy_service_(copy_repository_, id_generator_) {}

books::Book Library::add_book(const books::CreateBookInput& input) {
    return book_service_.add_book(input);
}

books::Book Library::edit_book(const std::string& public_id, const books::UpdateBookInput& input) {
    return book_service_.edit_book(public_id, input);
}

void Library::archive_book(const std::string& public_id) {
    book_service_.archive_book(public_id);
}

books::Book Library::get_book_details(const std::string& public_id, bool include_archived) const {
    return book_service_.get_book_details(public_id, include_archived);
}

std::vector<books::Book> Library::list_books(bool include_archived, int limit, int offset) const {
    return book_service_.list_books(include_archived, limit, offset);
}

std::vector<books::Book> Library::search_books(const books::BookQuery& query) const {
    return book_service_.search_books(query);
}

void Library::display_books(const std::vector<books::Book>& books) const {
    for (const auto& book : books) {
        std::cout << "[" << book.public_id << "] " << book.title << " by " << book.author
                  << " | ISBN: " << book.isbn;
        if (book.is_archived) {
            std::cout << " | ARCHIVED";
        }
        std::cout << '\n';
    }
}

copies::BookCopy Library::add_copy(const copies::CreateCopyInput& input) {
    return copy_service_.add_copy(input);
}

copies::BookCopy Library::edit_copy(const std::string& public_id, const copies::UpdateCopyInput& input) {
    return copy_service_.edit_copy(public_id, input);
}

copies::BookCopy Library::get_copy(const std::string& public_id) const {
    return copy_service_.get_copy(public_id);
}

std::vector<copies::BookCopy> Library::list_book_copies(int book_id, int limit, int offset) const {
    return copy_service_.list_book_copies(book_id, limit, offset);
}

copies::BookCopy Library::change_copy_status(const std::string& public_id,
                                             copies::CopyStatus new_status,
                                             const std::string& note,
                                             const std::optional<std::string>& archival_reason) {
    return copy_service_.change_status(public_id, new_status, note, archival_reason);
}

copies::BookCopy Library::change_copy_location(const std::string& public_id,
                                               const std::optional<std::string>& current_location_id,
                                               const std::optional<std::string>& target_location_id,
                                               const std::string& note) {
    return copy_service_.change_location(public_id, current_location_id, target_location_id, note);
}
