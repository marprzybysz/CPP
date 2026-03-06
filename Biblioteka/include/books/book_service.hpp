#pragma once

#include "books/book.hpp"
#include "books/book_repository.hpp"
#include "common/system_id.hpp"

#include <functional>
#include <string>
#include <unordered_set>

namespace books {

class BookService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    BookService(BookRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    Book add_book(const CreateBookInput& input);
    Book get_book_details(const std::string& public_id, bool include_archived = false) const;
    std::vector<Book> list_books(bool include_archived = false, int limit = 100, int offset = 0) const;
    std::vector<Book> search_books(const BookQuery& query) const;
    Book edit_book(const std::string& public_id, const UpdateBookInput& input);
    void archive_book(const std::string& public_id);

private:
    static bool is_valid_isbn(const std::string& isbn);
    static std::string normalize_whitespace(std::string value);
    static std::vector<std::string> normalize_tokens(const std::vector<std::string>& values);

    void ensure_sequence_initialized(int year);

    BookRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace books
