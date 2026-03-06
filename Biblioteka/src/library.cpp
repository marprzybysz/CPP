#include "library.hpp"

#include <iostream>

Library::Library(Db& db)
    : db_(db),
      id_generator_(),
      book_repository_(db_),
      book_service_(book_repository_, id_generator_),
      copy_repository_(db_),
      copy_service_(copy_repository_, id_generator_),
      location_repository_(db_),
      location_service_(location_repository_, id_generator_),
      reader_repository_(db_),
      reader_service_(reader_repository_, id_generator_),
      note_repository_(db_),
      note_service_(note_repository_, id_generator_),
      reservation_repository_(db_),
      reservation_service_(reservation_repository_, id_generator_) {}

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

locations::Location Library::create_location(const locations::CreateLocationInput& input) {
    return location_service_.create_location(input);
}

locations::Location Library::edit_location(const std::string& public_id, const locations::UpdateLocationInput& input) {
    return location_service_.edit_location(public_id, input);
}

locations::Location Library::get_location(const std::string& public_id) const {
    return location_service_.get_location(public_id);
}

std::vector<locations::LocationNode> Library::get_location_tree() const {
    return location_service_.get_location_tree();
}

std::vector<copies::BookCopy> Library::get_location_copies(const std::string& public_id) const {
    return location_service_.get_location_copies(public_id);
}

readers::Reader Library::add_reader(const readers::CreateReaderInput& input) {
    return reader_service_.add_reader(input);
}

readers::Reader Library::edit_reader(const std::string& public_id, const readers::UpdateReaderInput& input) {
    return reader_service_.edit_reader(public_id, input);
}

std::vector<readers::Reader> Library::search_readers(const readers::ReaderQuery& query) const {
    return reader_service_.search_readers(query);
}

readers::Reader Library::get_reader_details(const std::string& public_id) const {
    return reader_service_.get_reader_details(public_id);
}

readers::Reader Library::block_reader(const std::string& public_id, const std::string& reason) {
    return reader_service_.block_account(public_id, reason);
}

readers::Reader Library::unblock_reader(const std::string& public_id) {
    return reader_service_.unblock_account(public_id);
}

notes::Note Library::add_note(const notes::CreateNoteInput& input) {
    return note_service_.add_note(input);
}

std::vector<notes::Note> Library::get_notes_for_target(const notes::NotesForTargetQuery& query) const {
    return note_service_.get_notes_for_target(query);
}

notes::Note Library::get_note_details(const std::string& public_id, bool include_archived) const {
    return note_service_.get_note_details(public_id, include_archived);
}

void Library::archive_note(const std::string& public_id) {
    note_service_.archive_note(public_id);
}

reservations::Reservation Library::create_reservation(const reservations::CreateReservationInput& input) {
    return reservation_service_.create_reservation(input);
}

reservations::Reservation Library::get_reservation_details(const std::string& public_id) const {
    return reservation_service_.get_reservation_details(public_id);
}

reservations::Reservation Library::cancel_reservation(const std::string& public_id) {
    return reservation_service_.cancel_reservation(public_id);
}

reservations::Reservation Library::expire_reservation(const std::string& public_id) {
    return reservation_service_.expire_reservation(public_id);
}

std::optional<reservations::Reservation> Library::find_active_reservation_for_returned_copy(int copy_id) const {
    return reservation_service_.find_active_for_returned_copy(copy_id);
}
