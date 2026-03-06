#pragma once

#include <optional>
#include <string>
#include <vector>

namespace books {

struct Book {
    std::optional<int> id;
    std::string public_id;
    std::string title;
    std::string author;
    std::string isbn;
    std::vector<std::string> categories;
    std::vector<std::string> tags;
    std::string edition;
    std::string publisher;
    std::optional<int> publication_year;
    std::string description;
    std::string created_at;
    std::string updated_at;
    std::optional<std::string> archived_at;
    bool is_archived{false};
};

struct CreateBookInput {
    std::string title;
    std::string author;
    std::string isbn;
    std::vector<std::string> categories;
    std::vector<std::string> tags;
    std::string edition;
    std::string publisher;
    std::optional<int> publication_year;
    std::string description;
};

struct UpdateBookInput {
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> isbn;
    std::optional<std::vector<std::string>> categories;
    std::optional<std::vector<std::string>> tags;
    std::optional<std::string> edition;
    std::optional<std::string> publisher;
    std::optional<int> publication_year;
    std::optional<std::string> description;
};

struct BookQuery {
    std::optional<std::string> text;
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> isbn;
    std::vector<std::string> categories;
    std::vector<std::string> tags;
    bool include_archived{false};
    int limit{100};
    int offset{0};
};

} // namespace books
