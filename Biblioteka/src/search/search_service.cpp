#include "search/search_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace search {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[search] " << message << '\n';
}

} // namespace

SearchService::SearchService(SearchRepository& repository, OperationLogger logger)
    : repository_(repository), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

GlobalSearchResult SearchService::search(const SearchQuery& query) const {
    const std::string text = normalize_text(query.text);
    if (text.empty()) {
        throw errors::ValidationError("search text is required");
    }
    if (query.limit <= 0) {
        throw errors::ValidationError("search limit must be greater than zero");
    }

    const std::string pattern = "%" + text + "%";

    GlobalSearchResult result;
    result.books = repository_.search_books(pattern, query.limit);
    result.copies = repository_.search_copies(pattern, query.limit);
    result.readers = repository_.search_readers(pattern, query.limit);

    logger_("global search text='" + text + "' books=" + std::to_string(result.books.size()) +
            " copies=" + std::to_string(result.copies.size()) +
            " readers=" + std::to_string(result.readers.size()));

    return result;
}

std::string SearchService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

} // namespace search
