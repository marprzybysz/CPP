#include <iostream>
#include <string>
#include "db.hpp"
#include "library.hpp"

int main() {
    try {
        Db db("library.db");
        db.init_schema();

        std::string title, author;

        std::cout << "Wpisz tytul ksiazki: ";
        std::getline(std::cin, title);

        std::cout << "Wpisz tytul autora: ";
        std::getline(std::cin, author);

        Library library(db);
        library.add_book(Book{std::nullopt, title, author});
        library.display_books();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Blad: " << e.what() << "\n";
        return 1;
    }
}