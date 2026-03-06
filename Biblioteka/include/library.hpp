#pragma once
#include "books/book.hpp"
#include "books/book_service.hpp"
#include "books/sqlite_book_repository.hpp"
#include "common/system_id.hpp"
#include "copies/copy.hpp"
#include "copies/copy_service.hpp"
#include "copies/sqlite_copy_repository.hpp"
#include "db.hpp"
#include "locations/location.hpp"
#include "locations/location_service.hpp"
#include "locations/sqlite_location_repository.hpp"
#include "notes/note.hpp"
#include "notes/note_service.hpp"
#include "notes/sqlite_note_repository.hpp"
#include "readers/reader.hpp"
#include "readers/reader_service.hpp"
#include "readers/sqlite_reader_repository.hpp"
#include <optional>
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

    copies::BookCopy add_copy(const copies::CreateCopyInput& input);
    copies::BookCopy edit_copy(const std::string& public_id, const copies::UpdateCopyInput& input);
    copies::BookCopy get_copy(const std::string& public_id) const;
    std::vector<copies::BookCopy> list_book_copies(int book_id, int limit = 100, int offset = 0) const;
    copies::BookCopy change_copy_status(const std::string& public_id,
                                        copies::CopyStatus new_status,
                                        const std::string& note = "",
                                        const std::optional<std::string>& archival_reason = std::nullopt);
    copies::BookCopy change_copy_location(const std::string& public_id,
                                          const std::optional<std::string>& current_location_id,
                                          const std::optional<std::string>& target_location_id,
                                          const std::string& note = "");

    locations::Location create_location(const locations::CreateLocationInput& input);
    locations::Location edit_location(const std::string& public_id, const locations::UpdateLocationInput& input);
    locations::Location get_location(const std::string& public_id) const;
    std::vector<locations::LocationNode> get_location_tree() const;
    std::vector<copies::BookCopy> get_location_copies(const std::string& public_id) const;

    readers::Reader add_reader(const readers::CreateReaderInput& input);
    readers::Reader edit_reader(const std::string& public_id, const readers::UpdateReaderInput& input);
    std::vector<readers::Reader> search_readers(const readers::ReaderQuery& query) const;
    readers::Reader get_reader_details(const std::string& public_id) const;
    readers::Reader block_reader(const std::string& public_id, const std::string& reason);
    readers::Reader unblock_reader(const std::string& public_id);

    notes::Note add_note(const notes::CreateNoteInput& input);
    std::vector<notes::Note> get_notes_for_target(const notes::NotesForTargetQuery& query) const;
    notes::Note get_note_details(const std::string& public_id, bool include_archived = false) const;
    void archive_note(const std::string& public_id);

private:
    Db& db_;
    common::SystemIdGenerator id_generator_;
    books::SqliteBookRepository book_repository_;
    books::BookService book_service_;
    copies::SqliteCopyRepository copy_repository_;
    copies::CopyService copy_service_;
    locations::SqliteLocationRepository location_repository_;
    locations::LocationService location_service_;
    readers::SqliteReaderRepository reader_repository_;
    readers::ReaderService reader_service_;
    notes::SqliteNoteRepository note_repository_;
    notes::NoteService note_service_;
};
