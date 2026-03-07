#pragma once

#include "copies/copy.hpp"

#include <optional>
#include <string>
#include <vector>

namespace search {

struct SearchQuery {
    std::string text;
    int limit{20};
};

struct BookSearchHit {
    std::string book_public_id;
    std::string title;
    std::string author;
    std::string isbn;
    bool is_archived{false};
};

struct CopyHolderInfo {
    std::string reader_public_id;
    std::string card_number;
    std::string full_name;
};

struct CopySearchHit {
    std::string copy_public_id;
    std::string inventory_number;
    copies::CopyStatus status{copies::CopyStatus::OnShelf};
    std::string book_public_id;
    std::string book_title;
    std::string book_author;
    std::optional<std::string> current_location_public_id;
    std::optional<std::string> current_location_name;
    std::optional<std::string> target_location_public_id;
    std::optional<std::string> target_location_name;
    std::optional<CopyHolderInfo> holder;
};

struct ReaderSearchHit {
    std::string reader_public_id;
    std::string card_number;
    std::string first_name;
    std::string last_name;
    std::string email;
    bool is_blocked{false};
};

struct GlobalSearchResult {
    std::vector<BookSearchHit> books;
    std::vector<CopySearchHit> copies;
    std::vector<ReaderSearchHit> readers;
};

} // namespace search
