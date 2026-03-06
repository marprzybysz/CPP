#pragma once
#include "books/book.hpp"
#include "books/book_service.hpp"
#include "books/sqlite_book_repository.hpp"
#include "common/system_id.hpp"
#include "copies/copy.hpp"
#include "copies/copy_service.hpp"
#include "copies/sqlite_copy_repository.hpp"
#include "db.hpp"
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

private:
    Db& db_;
    common::SystemIdGenerator id_generator_;
    books::SqliteBookRepository book_repository_;
    books::BookService book_service_;
    copies::SqliteCopyRepository copy_repository_;
    copies::CopyService copy_service_;
};
