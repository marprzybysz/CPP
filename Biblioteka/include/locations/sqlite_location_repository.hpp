#pragma once

#include "db.hpp"
#include "locations/location_repository.hpp"

namespace locations {

class SqliteLocationRepository final : public LocationRepository {
public:
    explicit SqliteLocationRepository(Db& db) : db_(db) {}

    Location create(const Location& location) override;
    Location update(const Location& location) override;

    std::optional<Location> get_by_id(int id) const override;
    std::optional<Location> get_by_public_id(const std::string& public_id) const override;
    std::vector<Location> list_all() const override;

    bool code_exists(const std::string& code, const std::string* excluded_public_id = nullptr) const override;

    std::uint64_t next_public_sequence(int year) const override;

    std::vector<copies::BookCopy> list_assigned_copies(const std::string& location_public_id) const override;

private:
    Db& db_;
};

} // namespace locations
