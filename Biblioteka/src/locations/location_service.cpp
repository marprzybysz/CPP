#include "locations/location_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <utility>

namespace locations {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[locations] " << message << '\n';
}

LocationNode build_node(const Location& location,
                        const std::unordered_map<int, std::vector<Location>>& children_by_parent) {
    LocationNode node;
    node.location = location;

    if (!location.id.has_value()) {
        return node;
    }

    const auto it = children_by_parent.find(*location.id);
    if (it == children_by_parent.end()) {
        return node;
    }

    for (const auto& child : it->second) {
        node.children.push_back(build_node(child, children_by_parent));
    }

    return node;
}

} // namespace

LocationService::LocationService(LocationRepository& repository,
                                 common::SystemIdGenerator& id_generator,
                                 OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

Location LocationService::create_location(const CreateLocationInput& input) {
    Location location;
    location.name = normalize_text(input.name);
    location.type = input.type;
    location.parent_id = input.parent_id;
    location.code = normalize_text(input.code);
    location.description = normalize_text(input.description);

    if (location.name.empty()) {
        throw errors::ValidationError("location name is required");
    }
    if (location.code.empty()) {
        throw errors::ValidationError("location code is required");
    }
    if (repository_.code_exists(location.code)) {
        throw errors::ValidationError("location code must be unique");
    }

    validate_hierarchy(location.type, location.parent_id, std::nullopt);

    const int year = common::current_year();
    ensure_sequence_initialized(year);
    location.public_id = id_generator_.generate_for_year(common::IdType::Location, year);

    Location created = repository_.create(location);
    logger_("location created public_id=" + created.public_id + " code=" + created.code);
    return created;
}

Location LocationService::edit_location(const std::string& public_id, const UpdateLocationInput& input) {
    Location location = get_location(public_id);

    if (input.name.has_value()) {
        location.name = normalize_text(*input.name);
    }
    if (input.type.has_value()) {
        location.type = *input.type;
    }
    if (input.code.has_value()) {
        location.code = normalize_text(*input.code);
    }
    if (input.description.has_value()) {
        location.description = normalize_text(*input.description);
    }
    if (input.clear_parent) {
        location.parent_id = std::nullopt;
    } else if (input.parent_id.has_value()) {
        location.parent_id = input.parent_id;
    }

    if (location.name.empty()) {
        throw errors::ValidationError("location name is required");
    }
    if (location.code.empty()) {
        throw errors::ValidationError("location code is required");
    }
    if (repository_.code_exists(location.code, &location.public_id)) {
        throw errors::ValidationError("location code must be unique");
    }

    validate_hierarchy(location.type, location.parent_id, location.id);

    Location updated = repository_.update(location);
    logger_("location updated public_id=" + updated.public_id);
    return updated;
}

Location LocationService::get_location(const std::string& public_id) const {
    const auto found = repository_.get_by_public_id(public_id);
    if (!found.has_value()) {
        throw errors::NotFoundError("Location not found. public_id=" + public_id);
    }
    return *found;
}

std::vector<LocationNode> LocationService::get_location_tree() const {
    const std::vector<Location> locations = repository_.list_all();

    std::unordered_map<int, std::vector<Location>> children_by_parent;
    std::vector<Location> roots;

    for (const auto& location : locations) {
        if (!location.parent_id.has_value()) {
            roots.push_back(location);
            continue;
        }

        children_by_parent[*location.parent_id].push_back(location);
    }

    std::vector<LocationNode> tree;
    tree.reserve(roots.size());

    for (const auto& root : roots) {
        tree.push_back(build_node(root, children_by_parent));
    }

    return tree;
}

std::vector<copies::BookCopy> LocationService::get_location_copies(const std::string& public_id) const {
    (void)get_location(public_id);
    return repository_.list_assigned_copies(public_id);
}

std::string LocationService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

bool LocationService::is_parent_relation_valid(LocationType child, LocationType parent) {
    switch (child) {
        case LocationType::Library:
            return false;
        case LocationType::Room:
            return parent == LocationType::Library;
        case LocationType::Rack:
            return parent == LocationType::Room;
        case LocationType::Shelf:
            return parent == LocationType::Rack;
        default:
            return false;
    }
}

void LocationService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Location, year, next);
    initialized_years_.insert(year);
}

void LocationService::validate_hierarchy(LocationType type,
                                         const std::optional<int>& parent_id,
                                         const std::optional<int>& self_id) const {
    if (type == LocationType::Library) {
        if (parent_id.has_value()) {
            throw errors::ValidationError("LIBRARY location cannot have parent");
        }
        return;
    }

    if (!parent_id.has_value()) {
        throw errors::ValidationError("Location type requires parent");
    }

    if (self_id.has_value() && *self_id == *parent_id) {
        throw errors::ValidationError("Location cannot be its own parent");
    }

    const auto parent = repository_.get_by_id(*parent_id);
    if (!parent.has_value()) {
        throw errors::NotFoundError("Parent location not found. id=" + std::to_string(*parent_id));
    }

    if (!is_parent_relation_valid(type, parent->type)) {
        throw errors::ValidationError("Invalid parent type for location hierarchy");
    }

    if (self_id.has_value()) {
        std::optional<int> cursor = parent->parent_id;
        while (cursor.has_value()) {
            if (*cursor == *self_id) {
                throw errors::ValidationError("Location hierarchy cycle detected");
            }
            const auto next = repository_.get_by_id(*cursor);
            if (!next.has_value()) {
                break;
            }
            cursor = next->parent_id;
        }

        const auto all = repository_.list_all();
        for (const auto& candidate : all) {
            if (!candidate.parent_id.has_value() || *candidate.parent_id != *self_id) {
                continue;
            }

            if (!is_parent_relation_valid(candidate.type, type)) {
                throw errors::ValidationError("Location type change breaks child hierarchy");
            }
        }
    }
}

} // namespace locations
