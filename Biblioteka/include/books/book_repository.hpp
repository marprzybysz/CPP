#pragma once

#include "books/book.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace books {

class BookRepository {
public:
    virtual ~BookRepository() = default;

    virtual Book create(const Book& book) = 0;
    virtual Book update(const Book& book) = 0;
    virtual void archive_by_public_id(const std::string& public_id) = 0;
    virtual std::optional<Book> get_by_public_id(const std::string& public_id, bool include_archived = false) const = 0;
    virtual std::vector<Book> list(bool include_archived = false, int limit = 100, int offset = 0) const = 0;
    virtual std::vector<Book> search(const BookQuery& query) const = 0;
    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace books
