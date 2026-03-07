#pragma once

#include "copies/copy.hpp"
#include "locations/location.hpp"

#include <optional>
#include <string>
#include <vector>

namespace inventory {

enum class InventoryStatus {
    InProgress,
    Completed,
};

enum class InventoryItemResult {
    OnShelf,
    Justified,
    Missing,
};

struct InventorySession {
    std::optional<int> id;
    std::string public_id;
    std::string location_public_id;
    locations::LocationType scope_type{locations::LocationType::Shelf};
    InventoryStatus status{InventoryStatus::InProgress};
    std::string started_by;
    std::string started_at;
    std::optional<std::string> finished_at;
    int on_shelf_count{0};
    int justified_count{0};
    int missing_count{0};
    std::string summary_result;
};

struct InventoryScannedCopy {
    std::optional<int> id;
    std::string session_public_id;
    std::string scan_code;
    std::string copy_public_id;
    std::string scanned_at;
    std::string note;
};

struct InventoryItem {
    std::optional<int> id;
    std::string session_public_id;
    std::string copy_public_id;
    std::string inventory_number;
    std::optional<std::string> expected_location_public_id;
    std::optional<std::string> current_location_public_id;
    bool scanned{false};
    InventoryItemResult result{InventoryItemResult::Missing};
    std::string justification;
    std::string decided_at;
};

struct StartInventoryInput {
    std::string location_public_id;
    std::string started_by;
};

struct RegisterScannedCopyInput {
    std::string scan_code;
    std::string note;
};

struct JustifiedInventoryInput {
    std::string copy_public_id;
    std::string reason;
};

struct FinishInventoryInput {
    std::vector<JustifiedInventoryInput> justified;
};

struct InventoryResult {
    InventorySession session;
    std::vector<InventoryItem> on_shelf;
    std::vector<InventoryItem> justified;
    std::vector<InventoryItem> missing;
};

[[nodiscard]] std::string to_string(InventoryStatus status);
[[nodiscard]] InventoryStatus inventory_status_from_string(const std::string& raw_status);
[[nodiscard]] std::string to_string(InventoryItemResult result);
[[nodiscard]] InventoryItemResult inventory_item_result_from_string(const std::string& raw_result);

} // namespace inventory
