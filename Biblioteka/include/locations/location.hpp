#pragma once

#include "copies/copy.hpp"

#include <optional>
#include <string>
#include <vector>

namespace locations {

enum class LocationType {
    Library,
    Room,
    Rack,
    Shelf,
};

struct Location {
    std::optional<int> id;
    std::string public_id;
    std::string name;
    LocationType type{LocationType::Library};
    std::optional<int> parent_id;
    std::string code;
    std::string description;
};

struct CreateLocationInput {
    std::string name;
    LocationType type{LocationType::Library};
    std::optional<int> parent_id;
    std::string code;
    std::string description;
};

struct UpdateLocationInput {
    std::optional<std::string> name;
    std::optional<LocationType> type;
    std::optional<int> parent_id;
    bool clear_parent{false};
    std::optional<std::string> code;
    std::optional<std::string> description;
};

struct LocationNode {
    Location location;
    std::vector<LocationNode> children;
};

[[nodiscard]] std::string to_string(LocationType type);
[[nodiscard]] LocationType location_type_from_string(const std::string& raw);

} // namespace locations
