#include "ui/controllers/locations_controller.hpp"

#include "library.hpp"

namespace ui::controllers {

LocationsController::LocationsController(Library& library) : library_(library) {}

std::vector<LocationTreeEntry> LocationsController::list_location_tree() const {
    const auto tree = library_.get_location_tree();
    std::vector<LocationTreeEntry> out;
    out.reserve(64);
    flatten_nodes(tree, 0, out);
    return out;
}

locations::Location LocationsController::get_location_details(const std::string& location_public_id) const {
    return library_.get_location(location_public_id);
}

std::vector<copies::BookCopy> LocationsController::list_location_copies(const std::string& location_public_id) const {
    return library_.get_location_copies(location_public_id);
}

void LocationsController::set_selected_location(const std::string& location_public_id) {
    selected_location_ = location_public_id;
}

void LocationsController::clear_selected_location() {
    selected_location_.reset();
}

const std::optional<std::string>& LocationsController::selected_location() const {
    return selected_location_;
}

void LocationsController::flatten_nodes(const std::vector<locations::LocationNode>& nodes,
                                        std::size_t depth,
                                        std::vector<LocationTreeEntry>& out) {
    for (const auto& node : nodes) {
        out.push_back(LocationTreeEntry{.location = node.location, .depth = depth});
        flatten_nodes(node.children, depth + 1, out);
    }
}

} // namespace ui::controllers
