#pragma once

#include "search/search.hpp"

#include <string>
#include <vector>

namespace search {

class SearchRepository {
public:
    virtual ~SearchRepository() = default;

    virtual std::vector<BookSearchHit> search_books(const std::string& pattern, int limit) const = 0;
    virtual std::vector<CopySearchHit> search_copies(const std::string& pattern, int limit) const = 0;
    virtual std::vector<ReaderSearchHit> search_readers(const std::string& pattern, int limit) const = 0;
};

} // namespace search
