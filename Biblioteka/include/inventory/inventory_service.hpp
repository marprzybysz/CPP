#pragma once

#include "common/system_id.hpp"
#include "inventory/inventory_repository.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace inventory {

class InventoryService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    InventoryService(InventoryRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    InventorySession start_inventory(const StartInventoryInput& input);
    InventoryScannedCopy register_scanned_copy(const std::string& session_public_id, const RegisterScannedCopyInput& input);
    InventoryResult finish_inventory(const std::string& session_public_id, const FinishInventoryInput& input);
    InventoryResult get_inventory_result(const std::string& session_public_id) const;

private:
    static std::string normalize_text(std::string value);
    static std::vector<std::string> resolve_scope_location_ids(const locations::Location& root,
                                                               const std::vector<std::string>& subtree_ids);

    copies::BookCopy resolve_copy_for_scan(const std::string& scan_code) const;
    void ensure_sequence_initialized(int year);

    InventoryRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace inventory
