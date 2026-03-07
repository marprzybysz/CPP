#pragma once

#include "db.hpp"
#include "search/search_repository.hpp"

namespace search {

class SqliteSearchRepository final : public SearchRepository {
public:
    explicit SqliteSearchRepository(Db& db) : db_(db) {}

    std::vector<BookSearchHit> search_books(const std::string& pattern, int limit) const override;
    std::vector<CopySearchHit> search_copies(const std::string& pattern, int limit) const override;
    std::vector<ReaderSearchHit> search_readers(const std::string& pattern, int limit) const override;

private:
    Db& db_;
};

} // namespace search
