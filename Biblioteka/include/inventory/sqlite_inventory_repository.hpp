#pragma once

#include "db.hpp"
#include "inventory/inventory_repository.hpp"

namespace inventory {

class SqliteInventoryRepository final : public InventoryRepository {
public:
    explicit SqliteInventoryRepository(Db& db) : db_(db) {}

    std::optional<locations::Location> get_location_by_public_id(const std::string& public_id) const override;
    std::vector<std::string> list_location_subtree_public_ids(const std::string& root_public_id) const override;

    std::vector<copies::BookCopy> list_copies_expected_in_locations(const std::vector<std::string>& location_public_ids) const override;
    std::optional<copies::BookCopy> get_copy_by_public_id(const std::string& public_id) const override;
    std::optional<copies::BookCopy> get_copy_by_inventory_number(const std::string& inventory_number) const override;

    InventorySession create_session(const InventorySession& session) override;
    std::optional<InventorySession> get_session_by_public_id(const std::string& public_id) const override;
    std::optional<InventorySession> get_active_session_for_location(const std::string& location_public_id) const override;
    std::vector<InventorySession> list_sessions(int limit, int offset) const override;

    InventoryScannedCopy create_scan(const InventoryScannedCopy& scan) override;
    std::vector<InventoryScannedCopy> list_scans_for_session(const std::string& session_public_id) const override;

    void replace_session_items(const std::string& session_public_id, const std::vector<InventoryItem>& items) override;
    std::vector<InventoryItem> list_items_for_session(const std::string& session_public_id) const override;

    InventorySession complete_session(const std::string& session_public_id,
                                      int on_shelf_count,
                                      int justified_count,
                                      int missing_count,
                                      const std::string& summary_result) override;

    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace inventory
