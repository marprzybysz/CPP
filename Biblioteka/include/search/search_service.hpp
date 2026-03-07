#pragma once

#include "search/search_repository.hpp"

#include <functional>
#include <string>

namespace search {

class SearchService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    explicit SearchService(SearchRepository& repository, OperationLogger logger = {});

    GlobalSearchResult search(const SearchQuery& query) const;

private:
    static std::string normalize_text(std::string value);

    SearchRepository& repository_;
    OperationLogger logger_;
};

} // namespace search
