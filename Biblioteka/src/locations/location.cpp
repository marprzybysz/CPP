#include "locations/location.hpp"

#include "errors/app_error.hpp"

namespace locations {

std::string to_string(LocationType type) {
    switch (type) {
        case LocationType::Library:
            return "LIBRARY";
        case LocationType::Room:
            return "ROOM";
        case LocationType::Rack:
            return "RACK";
        case LocationType::Shelf:
            return "SHELF";
        default:
            throw errors::ValidationError("Unsupported location type");
    }
}

LocationType location_type_from_string(const std::string& raw) {
    if (raw == "LIBRARY") {
        return LocationType::Library;
    }
    if (raw == "ROOM") {
        return LocationType::Room;
    }
    if (raw == "RACK") {
        return LocationType::Rack;
    }
    if (raw == "SHELF") {
        return LocationType::Shelf;
    }

    throw errors::ValidationError("Unsupported location type: " + raw);
}

} // namespace locations
