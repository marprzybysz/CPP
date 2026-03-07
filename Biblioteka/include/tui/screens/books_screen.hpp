#pragma once

#include "tui/controllers/books_controller.hpp"
#include "tui/screens/screen.hpp"

namespace tui {

class BooksScreen : public Screen {
public:
    explicit BooksScreen(BooksController& controller);

    [[nodiscard]] std::string title() const override;
    void show() override;

private:
    void list_recent();
    void search_by_author();
    void add_book();

    BooksController& controller_;
};

} // namespace tui
