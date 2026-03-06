#include <iostream>
#include <string>

#include "db.hpp"
#include "errors/error_logger.hpp"
#include "errors/error_mapper.hpp"
#include "library.hpp"

int main() {
    try {
        Db db("library.db");
        db.init_schema();

        Library library(db);

        books::CreateBookInput input;
        input.title = "Clean Code";
        input.author = "Robert C. Martin";
        input.isbn = "9780132350884";
        input.categories = {"software", "engineering"};
        input.tags = {"clean-code", "best-practices"};
        input.publisher = "Prentice Hall";
        input.publication_year = 2008;
        input.description = "A handbook of agile software craftsmanship.";

        books::BookQuery by_isbn_query;
        by_isbn_query.isbn = input.isbn;
        const auto existing = library.search_books(by_isbn_query);

        if (existing.empty()) {
            const books::Book created = library.add_book(input);
            std::cout << "Dodano ksiazke: " << created.public_id << "\n";
        } else {
            std::cout << "Ksiazka juz istnieje: " << existing.front().public_id << "\n";
        }

        const auto all_books = library.list_books(false, 20, 0);
        library.display_books(all_books);

        books::BookQuery query;
        query.author = "Martin";
        const auto by_author = library.search_books(query);
        std::cout << "\nWyniki wyszukiwania po autorze=Martin: " << by_author.size() << "\n";

        return 0;
    } catch (const std::exception& e) {
        errors::log_error(e, std::cerr);
        std::cerr << "Blad: " << errors::to_user_message(e) << "\n";
        return 1;
    }
}
