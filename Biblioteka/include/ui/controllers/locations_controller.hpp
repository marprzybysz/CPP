#pragma once

#include <optional>
#include <string>
#include <vector>

#include "copies/copy.hpp"
#include "locations/location.hpp"

class Library;

namespace ui::controllers {

struct LocationTreeEntry {
    locations::Location location;
    std::size_t depth{0};
};

class LocationsController {
public:
    explicit LocationsController(Library& library);

    [[nodiscard]] std::vector<LocationTreeEntry> list_location_tree() const;
    [[nodiscard]] locations::Location get_location_details(const std::string& location_public_id) const;
    [[nodiscard]] std::vector<copies::BookCopy> list_location_copies(const std::string& location_public_id) const;

    void set_selected_location(const std::string& location_public_id);
    void clear_selected_location();
    [[nodiscard]] const std::optional<std::string>& selected_location() const;

private:
    static void flatten_nodes(const std::vector<locations::LocationNode>& nodes,
                              std::size_t depth,
                              std::vector<LocationTreeEntry>& out);

    Library& library_;
    std::optional<std::string> selected_location_;
};

} // namespace ui::controllers
