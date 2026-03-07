#include "ui/controllers/copies_controller.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <utility>

#include "errors/app_error.hpp"
#include "library.hpp"

namespace {

constexpr int kPagedBatch = 200;

std::string trim_copy(std::string raw) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(raw.begin(), raw.end(), not_space);
    if (begin == raw.end()) {
        return "";
    }

    const auto end = std::find_if(raw.rbegin(), raw.rend(), not_space).base();
    return std::string(begin, end);
}

std::optional<std::string> normalize_optional_string(std::optional<std::string> value) {
    if (!value.has_value()) {
        return std::nullopt;
    }

    std::string normalized = trim_copy(*value);
    if (normalized.empty()) {
        return std::nullopt;
    }

    return normalized;
}

void flatten_location_nodes(const std::vector<locations::LocationNode>& nodes,
                            std::vector<locations::Location>& out) {
    for (const auto& node : nodes) {
        out.push_back(node.location);
        flatten_location_nodes(node.children, out);
    }
}

} // namespace

namespace ui::controllers {

CopiesController::CopiesController(Library& library) : library_(library) {}

std::vector<CopyListItem> CopiesController::list_copies(const CopyListFilter& filter) const {
    std::vector<books::Book> books;
    if (filter.book_query.empty()) {
        books = library_.list_books(filter.include_archived_books, filter.book_limit, 0);
    } else {
        books::BookQuery query;
        query.text = filter.book_query;
        query.include_archived = filter.include_archived_books;
        query.limit = filter.book_limit;
        books = library_.search_books(query);
    }

    std::unordered_map<std::string, std::optional<std::string>> location_cache;
    std::vector<CopyListItem> rows;

    for (const auto& book : books) {
        if (!book.id.has_value()) {
            continue;
        }

        const auto copies = library_.list_book_copies(*book.id, filter.copies_per_book_limit, 0);
        for (const auto& copy : copies) {
            if (filter.status.has_value() && copy.status != *filter.status) {
                continue;
            }

            CopyListItem item;
            item.copy = copy;
            item.book_public_id = book.public_id;
            item.book_title = book.title;
            item.book_author = book.author;
            item.current_location_name = resolve_location_name(copy.current_location_id, location_cache);
            item.target_location_name = resolve_location_name(copy.target_location_id, location_cache);
            rows.push_back(std::move(item));
        }
    }

    std::sort(rows.begin(), rows.end(), [](const CopyListItem& lhs, const CopyListItem& rhs) {
        return lhs.copy.inventory_number < rhs.copy.inventory_number;
    });

    return rows;
}

CopyListItem CopiesController::get_copy_details(const std::string& copy_public_id) const {
    const copies::BookCopy copy = library_.get_copy(copy_public_id);

    CopyListItem item;
    item.copy = copy;

    if (const auto maybe_book = find_book_by_id(copy.book_id); maybe_book.has_value()) {
        item.book_public_id = maybe_book->public_id;
        item.book_title = maybe_book->title;
        item.book_author = maybe_book->author;
    } else {
        item.book_public_id = "UNKNOWN";
        item.book_title = "(nieznana ksiazka)";
        item.book_author = "-";
    }

    std::unordered_map<std::string, std::optional<std::string>> location_cache;
    item.current_location_name = resolve_location_name(copy.current_location_id, location_cache);
    item.target_location_name = resolve_location_name(copy.target_location_id, location_cache);
    return item;
}

std::vector<locations::Location> CopiesController::list_locations() const {
    const auto tree = library_.get_location_tree();
    std::vector<locations::Location> locations;
    locations.reserve(64);
    flatten_location_nodes(tree, locations);
    return locations;
}

copies::BookCopy CopiesController::add_copy(const CopyCreateDraft& draft) {
    const books::Book book = library_.get_book_details(draft.book_public_id, true);
    if (!book.id.has_value()) {
        throw errors::ValidationError("Book id is missing for public_id=" + draft.book_public_id);
    }

    copies::CreateCopyInput input;
    input.book_id = *book.id;
    input.inventory_number = trim_copy(draft.inventory_number);
    input.status = draft.status;
    input.current_location_id = normalize_optional_string(draft.current_location_id);
    input.target_location_id = normalize_optional_string(draft.target_location_id);
    input.condition_note = trim_copy(draft.condition_note);
    input.acquisition_date = normalize_optional_string(draft.acquisition_date);

    copies::BookCopy created = library_.add_copy(input);
    selected_copy_ = created.public_id;
    return created;
}

copies::BookCopy CopiesController::change_status(const std::string& copy_public_id,
                                                 copies::CopyStatus new_status,
                                                 const std::string& note,
                                                 const std::optional<std::string>& archival_reason) {
    copies::BookCopy updated = library_.change_copy_status(
        copy_public_id, new_status, trim_copy(note), normalize_optional_string(archival_reason));
    selected_copy_ = updated.public_id;
    return updated;
}

copies::BookCopy CopiesController::change_location(const std::string& copy_public_id,
                                                   const std::optional<std::string>& current_location_id,
                                                   const std::optional<std::string>& target_location_id,
                                                   const std::string& note) {
    copies::BookCopy updated = library_.change_copy_location(copy_public_id,
                                                             normalize_optional_string(current_location_id),
                                                             normalize_optional_string(target_location_id),
                                                             trim_copy(note));
    selected_copy_ = updated.public_id;
    return updated;
}

void CopiesController::set_selected_copy(const std::string& copy_public_id) {
    selected_copy_ = copy_public_id;
}

void CopiesController::clear_selected_copy() {
    selected_copy_.reset();
}

const std::optional<std::string>& CopiesController::selected_copy() const {
    return selected_copy_;
}

std::optional<books::Book> CopiesController::find_book_by_id(int id) const {
    int offset = 0;
    while (true) {
        const auto page = library_.list_books(true, kPagedBatch, offset);
        if (page.empty()) {
            return std::nullopt;
        }

        for (const auto& book : page) {
            if (book.id.has_value() && *book.id == id) {
                return book;
            }
        }

        if (static_cast<int>(page.size()) < kPagedBatch) {
            return std::nullopt;
        }

        offset += kPagedBatch;
    }
}

std::optional<std::string> CopiesController::resolve_location_name(
    const std::optional<std::string>& public_id,
    std::unordered_map<std::string, std::optional<std::string>>& cache) const {
    if (!public_id.has_value()) {
        return std::nullopt;
    }

    if (const auto it = cache.find(*public_id); it != cache.end()) {
        return it->second;
    }

    try {
        const auto location = library_.get_location(*public_id);
        const std::optional<std::string> value = location.name + " (" + location.public_id + ")";
        cache.insert_or_assign(*public_id, value);
        return value;
    } catch (...) {
        const std::optional<std::string> unknown = std::string{"(nieznana lokalizacja "} + *public_id + ")";
        cache.insert_or_assign(*public_id, unknown);
        return unknown;
    }
}

} // namespace ui::controllers
