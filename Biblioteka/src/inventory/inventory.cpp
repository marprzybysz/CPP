#include "inventory/inventory.hpp"

#include "errors/app_error.hpp"

namespace inventory {

std::string to_string(InventoryStatus status) {
    switch (status) {
        case InventoryStatus::InProgress:
            return "IN_PROGRESS";
        case InventoryStatus::Completed:
            return "COMPLETED";
        default:
            throw errors::ValidationError("Unsupported inventory status");
    }
}

InventoryStatus inventory_status_from_string(const std::string& raw_status) {
    if (raw_status == "IN_PROGRESS") {
        return InventoryStatus::InProgress;
    }
    if (raw_status == "COMPLETED") {
        return InventoryStatus::Completed;
    }

    throw errors::ValidationError("Unsupported inventory status: " + raw_status);
}

std::string to_string(InventoryItemResult result) {
    switch (result) {
        case InventoryItemResult::OnShelf:
            return "ON_SHELF";
        case InventoryItemResult::Justified:
            return "JUSTIFIED";
        case InventoryItemResult::Missing:
            return "MISSING";
        default:
            throw errors::ValidationError("Unsupported inventory item result");
    }
}

InventoryItemResult inventory_item_result_from_string(const std::string& raw_result) {
    if (raw_result == "ON_SHELF") {
        return InventoryItemResult::OnShelf;
    }
    if (raw_result == "JUSTIFIED") {
        return InventoryItemResult::Justified;
    }
    if (raw_result == "MISSING") {
        return InventoryItemResult::Missing;
    }

    throw errors::ValidationError("Unsupported inventory item result: " + raw_result);
}

} // namespace inventory
