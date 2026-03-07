#pragma once

#include "copies/copy.hpp"
#include "inventory/inventory.hpp"
#include "locations/location.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace inventory {

class InventoryRepository {
public:
    virtual ~InventoryRepository() = default;

    virtual std::optional<locations::Location> get_location_by_public_id(const std::string& public_id) const = 0;
    virtual std::vector<std::string> list_location_subtree_public_ids(const std::string& root_public_id) const = 0;

    virtual std::vector<copies::BookCopy> list_copies_expected_in_locations(const std::vector<std::string>& location_public_ids) const = 0;
    virtual std::optional<copies::BookCopy> get_copy_by_public_id(const std::string& public_id) const = 0;
    virtual std::optional<copies::BookCopy> get_copy_by_inventory_number(const std::string& inventory_number) const = 0;

    virtual InventorySession create_session(const InventorySession& session) = 0;
    virtual std::optional<InventorySession> get_session_by_public_id(const std::string& public_id) const = 0;
    virtual std::optional<InventorySession> get_active_session_for_location(const std::string& location_public_id) const = 0;

    virtual InventoryScannedCopy create_scan(const InventoryScannedCopy& scan) = 0;
    virtual std::vector<InventoryScannedCopy> list_scans_for_session(const std::string& session_public_id) const = 0;

    virtual void replace_session_items(const std::string& session_public_id, const std::vector<InventoryItem>& items) = 0;
    virtual std::vector<InventoryItem> list_items_for_session(const std::string& session_public_id) const = 0;

    virtual InventorySession complete_session(const std::string& session_public_id,
                                              int on_shelf_count,
                                              int justified_count,
                                              int missing_count,
                                              const std::string& summary_result) = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace inventory
