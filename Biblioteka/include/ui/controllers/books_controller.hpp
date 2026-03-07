#pragma once

#include <optional>
#include <string>
#include <vector>

#include "books/book.hpp"

class Library;

namespace ui::controllers {

struct BookSearchState {
    std::string title;
    std::string author;
    std::string isbn;
    bool include_archived{false};
    int limit{100};
};

enum class BookFormMode {
    Create,
    Edit,
};

struct BookFormState {
    BookFormMode mode{BookFormMode::Create};
    std::optional<std::string> target_public_id;
};

class BooksController {
public:
    explicit BooksController(Library& library);

    [[nodiscard]] std::vector<books::Book> find_books(const BookSearchState& state) const;
    [[nodiscard]] books::Book get_book_details(const std::string& public_id, bool include_archived = true) const;

    books::Book create_book(const books::CreateBookInput& input);
    books::Book update_book(const std::string& public_id, const books::UpdateBookInput& input);

    void set_selected_book(const std::string& public_id);
    [[nodiscard]] const std::optional<std::string>& selected_book() const;

    void begin_create();
    void begin_edit(const std::string& public_id);
    [[nodiscard]] const BookFormState& form_state() const;

private:
    Library& library_;
    std::optional<std::string> selected_book_;
    BookFormState form_state_;
};

} // namespace ui::controllers
