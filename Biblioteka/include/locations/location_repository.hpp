#pragma once

#include "copies/copy.hpp"
#include "locations/location.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace locations {

class LocationRepository {
public:
    virtual ~LocationRepository() = default;

    virtual Location create(const Location& location) = 0;
    virtual Location update(const Location& location) = 0;

    virtual std::optional<Location> get_by_id(int id) const = 0;
    virtual std::optional<Location> get_by_public_id(const std::string& public_id) const = 0;
    virtual std::vector<Location> list_all() const = 0;

    virtual bool code_exists(const std::string& code, const std::string* excluded_public_id = nullptr) const = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;

    virtual std::vector<copies::BookCopy> list_assigned_copies(const std::string& location_public_id) const = 0;
};

} // namespace locations
