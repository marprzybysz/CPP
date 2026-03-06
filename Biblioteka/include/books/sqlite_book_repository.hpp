#pragma once

#include "books/book_repository.hpp"
#include "db.hpp"

namespace books {

class SqliteBookRepository final : public BookRepository {
public:
    explicit SqliteBookRepository(Db& db) : db_(db) {}

    Book create(const Book& book) override;
    Book update(const Book& book) override;
    void archive_by_public_id(const std::string& public_id) override;
    std::optional<Book> get_by_public_id(const std::string& public_id, bool include_archived = false) const override;
    std::vector<Book> list(bool include_archived = false, int limit = 100, int offset = 0) const override;
    std::vector<Book> search(const BookQuery& query) const override;
    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace books
