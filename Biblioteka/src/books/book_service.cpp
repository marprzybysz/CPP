#include "books/book_service.hpp"

#include "errors/app_error.hpp"
#include "errors/error_logger.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace books {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[books] " << message << '\n';
}

std::string normalize_isbn(std::string isbn) {
    isbn.erase(std::remove_if(isbn.begin(), isbn.end(), [](unsigned char ch) {
                   return ch == '-' || std::isspace(ch) != 0;
               }),
               isbn.end());

    std::transform(isbn.begin(), isbn.end(), isbn.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    return isbn;
}

bool is_isbn10(const std::string& isbn) {
    if (isbn.size() != 10) {
        return false;
    }

    int sum = 0;
    for (int i = 0; i < 9; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(isbn[static_cast<std::size_t>(i)]))) {
            return false;
        }
        sum += (10 - i) * (isbn[static_cast<std::size_t>(i)] - '0');
    }

    const char checksum = isbn[9];
    if (checksum == 'X') {
        sum += 10;
    } else if (std::isdigit(static_cast<unsigned char>(checksum))) {
        sum += checksum - '0';
    } else {
        return false;
    }

    return sum % 11 == 0;
}

bool is_isbn13(const std::string& isbn) {
    if (isbn.size() != 13) {
        return false;
    }

    int sum = 0;
    for (std::size_t i = 0; i < 13; ++i) {
        const char ch = isbn[i];
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }

        const int digit = ch - '0';
        if (i % 2 == 0) {
            sum += digit;
        } else {
            sum += 3 * digit;
        }
    }

    return sum % 10 == 0;
}

} // namespace

BookService::BookService(BookRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

Book BookService::add_book(const CreateBookInput& input) {
    try {
        Book book;

        book.title = normalize_whitespace(input.title);
        book.author = normalize_whitespace(input.author);
        book.isbn = normalize_isbn(input.isbn);
        book.categories = normalize_tokens(input.categories);
        book.tags = normalize_tokens(input.tags);
        book.edition = normalize_whitespace(input.edition);
        book.publisher = normalize_whitespace(input.publisher);
        book.publication_year = input.publication_year;
        book.description = normalize_whitespace(input.description);

        if (book.title.empty()) {
            throw errors::ValidationError("Book title is required");
        }
        if (book.author.empty()) {
            throw errors::ValidationError("Book author is required");
        }
        if (!is_valid_isbn(book.isbn)) {
            throw errors::ValidationError("Invalid ISBN");
        }

        const int year = common::current_year();
        ensure_sequence_initialized(year);
        book.public_id = id_generator_.generate_for_year(common::IdType::Book, year);

        Book created = repository_.create(book);
        logger_("book created public_id=" + created.public_id);
        return created;
    } catch (const std::exception& ex) {
        errors::log_error(ex);
        throw;
    }
}

Book BookService::get_book_details(const std::string& public_id, bool include_archived) const {
    const auto book = repository_.get_by_public_id(public_id, include_archived);
    if (!book.has_value()) {
        throw errors::NotFoundError("Book not found. public_id=" + public_id);
    }
    return *book;
}

std::vector<Book> BookService::list_books(bool include_archived, int limit, int offset) const {
    if (limit <= 0) {
        throw errors::ValidationError("List limit must be greater than zero");
    }
    if (offset < 0) {
        throw errors::ValidationError("List offset cannot be negative");
    }
    return repository_.list(include_archived, limit, offset);
}

std::vector<Book> BookService::search_books(const BookQuery& query) const {
    if (query.limit <= 0) {
        throw errors::ValidationError("Search limit must be greater than zero");
    }
    if (query.offset < 0) {
        throw errors::ValidationError("Search offset cannot be negative");
    }

    BookQuery normalized = query;
    if (normalized.text.has_value()) {
        normalized.text = normalize_whitespace(*normalized.text);
    }
    if (normalized.title.has_value()) {
        normalized.title = normalize_whitespace(*normalized.title);
    }
    if (normalized.author.has_value()) {
        normalized.author = normalize_whitespace(*normalized.author);
    }
    if (normalized.isbn.has_value()) {
        normalized.isbn = normalize_isbn(*normalized.isbn);
    }
    normalized.categories = normalize_tokens(normalized.categories);
    normalized.tags = normalize_tokens(normalized.tags);

    return repository_.search(normalized);
}

Book BookService::edit_book(const std::string& public_id, const UpdateBookInput& input) {
    Book current = get_book_details(public_id, true);

    if (input.title.has_value()) {
        current.title = normalize_whitespace(*input.title);
    }
    if (input.author.has_value()) {
        current.author = normalize_whitespace(*input.author);
    }
    if (input.isbn.has_value()) {
        current.isbn = normalize_isbn(*input.isbn);
    }
    if (input.categories.has_value()) {
        current.categories = normalize_tokens(*input.categories);
    }
    if (input.tags.has_value()) {
        current.tags = normalize_tokens(*input.tags);
    }
    if (input.edition.has_value()) {
        current.edition = normalize_whitespace(*input.edition);
    }
    if (input.publisher.has_value()) {
        current.publisher = normalize_whitespace(*input.publisher);
    }
    if (input.publication_year.has_value()) {
        current.publication_year = input.publication_year;
    }
    if (input.description.has_value()) {
        current.description = normalize_whitespace(*input.description);
    }

    if (current.title.empty()) {
        throw errors::ValidationError("Book title is required");
    }
    if (current.author.empty()) {
        throw errors::ValidationError("Book author is required");
    }
    if (!is_valid_isbn(current.isbn)) {
        throw errors::ValidationError("Invalid ISBN");
    }

    Book updated = repository_.update(current);
    logger_("book updated public_id=" + updated.public_id);
    return updated;
}

void BookService::archive_book(const std::string& public_id) {
    repository_.archive_by_public_id(public_id);
    logger_("book archived public_id=" + public_id);
}

bool BookService::is_valid_isbn(const std::string& isbn) {
    const std::string normalized = normalize_isbn(isbn);
    return is_isbn10(normalized) || is_isbn13(normalized);
}

std::string BookService::normalize_whitespace(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

std::vector<std::string> BookService::normalize_tokens(const std::vector<std::string>& values) {
    std::vector<std::string> out;
    out.reserve(values.size());

    for (auto value : values) {
        value = normalize_whitespace(std::move(value));
        if (value.empty()) {
            continue;
        }

        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        value.erase(std::remove(value.begin(), value.end(), ','), value.end());
        if (!value.empty()) {
            out.push_back(std::move(value));
        }
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

void BookService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Book, year, next);
    initialized_years_.insert(year);
}

} // namespace books
