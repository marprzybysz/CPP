#pragma once

#include "common/system_id.hpp"
#include "locations/location_repository.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace locations {

class LocationService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    LocationService(LocationRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    Location create_location(const CreateLocationInput& input);
    Location edit_location(const std::string& public_id, const UpdateLocationInput& input);
    Location get_location(const std::string& public_id) const;
    std::vector<LocationNode> get_location_tree() const;
    std::vector<copies::BookCopy> get_location_copies(const std::string& public_id) const;

private:
    static std::string normalize_text(std::string value);
    static bool is_parent_relation_valid(LocationType child, LocationType parent);

    void ensure_sequence_initialized(int year);
    void validate_hierarchy(LocationType type, const std::optional<int>& parent_id, const std::optional<int>& self_id) const;

    LocationRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace locations
