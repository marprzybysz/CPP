#include "tui/screens/books_screen.hpp"

#include <iostream>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"

namespace {

std::optional<int> parse_year(const std::string& raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    try {
        return std::stoi(raw);
    } catch (...) {
        return std::nullopt;
    }
}

void print_books(const std::vector<books::Book>& books) {
    if (books.empty()) {
        std::cout << "Brak wynikow.\n";
        return;
    }

    for (const auto& book : books) {
        std::cout << "- [" << book.public_id << "] " << book.title << " / " << book.author
                  << " / ISBN: " << book.isbn << "\n";
    }
}

} // namespace

namespace tui {

BooksScreen::BooksScreen(BooksController& controller) : controller_(controller) {}

std::string BooksScreen::title() const {
    return "Ksiazki";
}

void BooksScreen::show() {
    bool done = false;
    while (!done) {
        std::cout << "\n=== Ksiazki ===\n";
        std::cout << "1. Lista ostatnich\n";
        std::cout << "2. Wyszukaj po autorze\n";
        std::cout << "3. Dodaj ksiazke\n";
        std::cout << "0. Powrot\n";
        std::cout << "Wybierz opcje: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            return;
        }

        if (choice == "1") {
            list_recent();
        } else if (choice == "2") {
            search_by_author();
        } else if (choice == "3") {
            add_book();
        } else if (choice == "0") {
            done = true;
        } else {
            std::cout << "Nieznana opcja.\n";
        }
    }
}

void BooksScreen::list_recent() {
    std::cout << "\n--- Ostatnie ksiazki ---\n";
    try {
        const auto books = controller_.list_recent(20);
        print_books(books);
    } catch (const std::exception& e) {
        std::cout << "Blad: " << errors::to_user_message(e) << "\n";
    }
}

void BooksScreen::search_by_author() {
    std::cout << "Autor: ";
    std::string author;
    if (!std::getline(std::cin, author)) {
        return;
    }

    std::cout << "\n--- Wyniki ---\n";
    try {
        const auto books = controller_.search_by_author(author, 20);
        print_books(books);
    } catch (const std::exception& e) {
        std::cout << "Blad: " << errors::to_user_message(e) << "\n";
    }
}

void BooksScreen::add_book() {
    books::CreateBookInput input;

    std::cout << "Tytul: ";
    if (!std::getline(std::cin, input.title)) {
        return;
    }

    std::cout << "Autor: ";
    if (!std::getline(std::cin, input.author)) {
        return;
    }

    std::cout << "ISBN: ";
    if (!std::getline(std::cin, input.isbn)) {
        return;
    }

    std::cout << "Wydawnictwo (opcjonalnie): ";
    if (!std::getline(std::cin, input.publisher)) {
        return;
    }

    std::cout << "Rok wydania (opcjonalnie): ";
    std::string raw_year;
    if (!std::getline(std::cin, raw_year)) {
        return;
    }
    input.publication_year = parse_year(raw_year);

    std::cout << "Opis (opcjonalnie): ";
    if (!std::getline(std::cin, input.description)) {
        return;
    }

    try {
        const books::Book created = controller_.add_book(input);
        std::cout << "Dodano ksiazke: " << created.public_id << "\n";
    } catch (const std::exception& e) {
        std::cout << "Blad: " << errors::to_user_message(e) << "\n";
    }
}

} // namespace tui
