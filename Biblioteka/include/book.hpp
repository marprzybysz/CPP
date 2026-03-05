#pragma once
#include <string>
#include <optional>

struct Book {
    std::optional<int> id;   // jak w Pythonie: book_id=None
    std::string title;
    std::string author;
};