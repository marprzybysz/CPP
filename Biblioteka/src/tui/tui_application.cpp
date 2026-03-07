#include "tui/tui_application.hpp"

#include <iostream>
#include <string>

#include "errors/error_mapper.hpp"
#include "tui/screens/books_screen.hpp"
#include "tui/screens/dashboard_screen.hpp"
#include "tui/screens/placeholder_screen.hpp"

namespace {

int parse_choice_or_default(const std::string& value, int default_value) {
    try {
        return std::stoi(value);
    } catch (...) {
        return default_value;
    }
}

} // namespace

namespace tui {

TuiApplication::TuiApplication(Library& library)
    : library_(library),
      dashboard_controller_(library_),
      books_controller_(library_) {
    screens_.push_back(std::make_unique<DashboardScreen>(dashboard_controller_));
    screens_.push_back(std::make_unique<BooksScreen>(books_controller_));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Egzemplarze", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Czytelnicy", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Wypozyczenia", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Rezerwacje", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Lokalizacje", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Inwentaryzacja", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Raporty", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Notatki", "Modul w przygotowaniu."));
    screens_.push_back(std::make_unique<PlaceholderScreen>("Logi zdarzen", "Modul w przygotowaniu."));
}

int TuiApplication::run() {
    bool running = true;
    while (running) {
        print_main_menu();

        const int choice = read_menu_choice();
        if (choice == 0) {
            running = false;
            continue;
        }

        if (choice < 0 || static_cast<std::size_t>(choice) > screens_.size()) {
            std::cout << "Nieznana opcja.\n";
            continue;
        }

        try {
            screens_[static_cast<std::size_t>(choice - 1)]->show();
        } catch (const std::exception& e) {
            std::cout << "Blad: " << errors::to_user_message(e) << "\n";
        }
    }

    std::cout << "Koniec pracy.\n";
    return 0;
}

void TuiApplication::print_main_menu() const {
    std::cout << "\n=== System Biblioteka (TUI) ===\n";
    for (std::size_t i = 0; i < screens_.size(); ++i) {
        std::cout << (i + 1) << ". " << screens_[i]->title() << "\n";
    }
    std::cout << "0. Wyjscie\n";
}

int TuiApplication::read_menu_choice() const {
    std::cout << "Wybierz opcje: ";
    std::string raw_choice;
    if (!std::getline(std::cin, raw_choice)) {
        return 0;
    }

    return parse_choice_or_default(raw_choice, -1);
}

} // namespace tui
