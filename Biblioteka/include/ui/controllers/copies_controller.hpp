#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "books/book.hpp"
#include "copies/copy.hpp"
#include "locations/location.hpp"

class Library;

namespace ui::controllers {

struct CopyListFilter {
    std::string book_query;
    std::optional<copies::CopyStatus> status;
    bool include_archived_books{true};
    int book_limit{80};
    int copies_per_book_limit{120};
};

struct CopyListItem {
    copies::BookCopy copy;
    std::string book_public_id;
    std::string book_title;
    std::string book_author;
    std::optional<std::string> current_location_name;
    std::optional<std::string> target_location_name;
};

struct CopyCreateDraft {
    std::string book_public_id;
    std::string inventory_number;
    copies::CopyStatus status{copies::CopyStatus::OnShelf};
    std::optional<std::string> current_location_id;
    std::optional<std::string> target_location_id;
    std::string condition_note;
    std::optional<std::string> acquisition_date;
};

class CopiesController {
public:
    explicit CopiesController(Library& library);

    [[nodiscard]] std::vector<CopyListItem> list_copies(const CopyListFilter& filter) const;
    [[nodiscard]] CopyListItem get_copy_details(const std::string& copy_public_id) const;
    [[nodiscard]] std::vector<locations::Location> list_locations() const;

    copies::BookCopy add_copy(const CopyCreateDraft& draft);
    copies::BookCopy change_status(const std::string& copy_public_id,
                                   copies::CopyStatus new_status,
                                   const std::string& note,
                                   const std::optional<std::string>& archival_reason);
    copies::BookCopy change_location(const std::string& copy_public_id,
                                     const std::optional<std::string>& current_location_id,
                                     const std::optional<std::string>& target_location_id,
                                     const std::string& note);

    void set_selected_copy(const std::string& copy_public_id);
    void clear_selected_copy();
    [[nodiscard]] const std::optional<std::string>& selected_copy() const;

private:
    [[nodiscard]] std::optional<books::Book> find_book_by_id(int id) const;
    [[nodiscard]] std::optional<std::string> resolve_location_name(
        const std::optional<std::string>& public_id,
        std::unordered_map<std::string, std::optional<std::string>>& cache) const;

    Library& library_;
    std::optional<std::string> selected_copy_;
};

} // namespace ui::controllers
