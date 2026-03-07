#pragma once

#include <memory>
#include <vector>

#include "tui/controllers/books_controller.hpp"
#include "tui/controllers/dashboard_controller.hpp"
#include "tui/screens/screen.hpp"

class Library;

namespace tui {

class TuiApplication {
public:
    explicit TuiApplication(Library& library);

    int run();

private:
    void print_main_menu() const;
    int read_menu_choice() const;

    Library& library_;
    DashboardController dashboard_controller_;
    BooksController books_controller_;
    std::vector<std::unique_ptr<Screen>> screens_;
};

} // namespace tui
