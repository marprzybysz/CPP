#include "inventory/inventory_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace inventory {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[inventory] " << message << '\n';
}

bool is_scope_supported(locations::LocationType type) {
    return type == locations::LocationType::Shelf || type == locations::LocationType::Rack || type == locations::LocationType::Room;
}

} // namespace

InventoryService::InventoryService(InventoryRepository& repository,
                                   common::SystemIdGenerator& id_generator,
                                   OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

InventorySession InventoryService::start_inventory(const StartInventoryInput& input) {
    const std::string location_public_id = normalize_text(input.location_public_id);
    const std::string started_by = normalize_text(input.started_by);

    if (location_public_id.empty()) {
        throw errors::ValidationError("location_public_id is required");
    }
    if (started_by.empty()) {
        throw errors::ValidationError("started_by is required");
    }

    const auto location = repository_.get_location_by_public_id(location_public_id);
    if (!location.has_value()) {
        throw errors::NotFoundError("Location not found. public_id=" + location_public_id);
    }
    if (!is_scope_supported(location->type)) {
        throw errors::InventoryError("Inventory can be started only for ROOM, RACK or SHELF");
    }

    const auto active = repository_.get_active_session_for_location(location_public_id);
    if (active.has_value()) {
        throw errors::InventoryError("Active inventory already exists for location. public_id=" + location_public_id);
    }

    const int year = common::current_year();
    ensure_sequence_initialized(year);

    InventorySession session;
    session.public_id = id_generator_.generate_for_year(common::IdType::Inventory, year);
    session.location_public_id = location_public_id;
    session.scope_type = location->type;
    session.started_by = started_by;
    session.status = InventoryStatus::InProgress;

    InventorySession created = repository_.create_session(session);
    logger_("inventory started public_id=" + created.public_id + " location=" + created.location_public_id);
    return created;
}

InventoryScannedCopy InventoryService::register_scanned_copy(const std::string& session_public_id,
                                                             const RegisterScannedCopyInput& input) {
    const std::string normalized_session = normalize_text(session_public_id);
    const std::string scan_code = normalize_text(input.scan_code);

    if (normalized_session.empty()) {
        throw errors::ValidationError("session_public_id is required");
    }
    if (scan_code.empty()) {
        throw errors::ValidationError("scan_code is required");
    }

    const auto session = repository_.get_session_by_public_id(normalized_session);
    if (!session.has_value()) {
        throw errors::NotFoundError("Inventory session not found. public_id=" + normalized_session);
    }
    if (session->status != InventoryStatus::InProgress) {
        throw errors::InventoryError("Cannot scan copies for completed inventory session");
    }

    const copies::BookCopy resolved_copy = resolve_copy_for_scan(scan_code);

    InventoryScannedCopy scan;
    scan.session_public_id = normalized_session;
    scan.scan_code = scan_code;
    scan.copy_public_id = resolved_copy.public_id;
    scan.note = normalize_text(input.note);

    InventoryScannedCopy created = repository_.create_scan(scan);
    logger_("inventory scan registered session=" + normalized_session + " copy=" + resolved_copy.public_id);
    return created;
}

InventoryResult InventoryService::finish_inventory(const std::string& session_public_id, const FinishInventoryInput& input) {
    const std::string normalized_session = normalize_text(session_public_id);
    if (normalized_session.empty()) {
        throw errors::ValidationError("session_public_id is required");
    }

    const auto session = repository_.get_session_by_public_id(normalized_session);
    if (!session.has_value()) {
        throw errors::NotFoundError("Inventory session not found. public_id=" + normalized_session);
    }
    if (session->status != InventoryStatus::InProgress) {
        throw errors::InventoryError("Inventory session is already completed");
    }

    const auto location = repository_.get_location_by_public_id(session->location_public_id);
    if (!location.has_value()) {
        throw errors::NotFoundError("Location not found. public_id=" + session->location_public_id);
    }

    const std::vector<std::string> subtree_ids = repository_.list_location_subtree_public_ids(session->location_public_id);
    const std::vector<std::string> scope_location_ids = resolve_scope_location_ids(*location, subtree_ids);

    const std::vector<copies::BookCopy> expected = repository_.list_copies_expected_in_locations(scope_location_ids);
    const std::vector<InventoryScannedCopy> scanned = repository_.list_scans_for_session(normalized_session);

    std::unordered_set<std::string> scope_set(scope_location_ids.begin(), scope_location_ids.end());
    std::unordered_map<std::string, copies::BookCopy> expected_by_copy;
    expected_by_copy.reserve(expected.size());
    for (const auto& copy : expected) {
        expected_by_copy[copy.public_id] = copy;
    }

    std::unordered_map<std::string, copies::BookCopy> scanned_by_copy;
    scanned_by_copy.reserve(scanned.size());
    for (const auto& scan : scanned) {
        const auto copy = repository_.get_copy_by_public_id(scan.copy_public_id);
        if (copy.has_value()) {
            scanned_by_copy[scan.copy_public_id] = *copy;
        }
    }

    std::unordered_map<std::string, std::string> manual_justification;
    manual_justification.reserve(input.justified.size());
    for (const auto& item : input.justified) {
        const std::string copy_public_id = normalize_text(item.copy_public_id);
        const std::string reason = normalize_text(item.reason);
        if (copy_public_id.empty() || reason.empty()) {
            throw errors::ValidationError("justified entries require copy_public_id and reason");
        }
        manual_justification[copy_public_id] = reason;
    }

    std::vector<InventoryItem> final_items;
    final_items.reserve(expected.size() + scanned_by_copy.size());

    for (const auto& [copy_public_id, copy] : expected_by_copy) {
        InventoryItem item;
        item.session_public_id = normalized_session;
        item.copy_public_id = copy_public_id;
        item.inventory_number = copy.inventory_number;
        item.expected_location_public_id = copy.target_location_id;
        item.current_location_public_id = copy.current_location_id;
        item.scanned = scanned_by_copy.find(copy_public_id) != scanned_by_copy.end();

        if (item.scanned) {
            item.result = InventoryItemResult::OnShelf;
        } else {
            const auto manual_it = manual_justification.find(copy_public_id);
            const bool outside_scope = copy.current_location_id.has_value() && scope_set.find(*copy.current_location_id) == scope_set.end();

            if (manual_it != manual_justification.end()) {
                item.result = InventoryItemResult::Justified;
                item.justification = manual_it->second;
            } else if (copy.status != copies::CopyStatus::OnShelf) {
                item.result = InventoryItemResult::Justified;
                item.justification = "status=" + copies::to_string(copy.status);
            } else if (outside_scope) {
                item.result = InventoryItemResult::Justified;
                item.justification = "copy currently assigned outside inventoried scope";
            } else {
                item.result = InventoryItemResult::Missing;
            }
        }

        final_items.push_back(std::move(item));
    }

    for (const auto& [copy_public_id, copy] : scanned_by_copy) {
        if (expected_by_copy.find(copy_public_id) != expected_by_copy.end()) {
            continue;
        }

        InventoryItem item;
        item.session_public_id = normalized_session;
        item.copy_public_id = copy_public_id;
        item.inventory_number = copy.inventory_number;
        item.expected_location_public_id = copy.target_location_id;
        item.current_location_public_id = copy.current_location_id;
        item.scanned = true;
        item.result = InventoryItemResult::Justified;

        const auto manual_it = manual_justification.find(copy_public_id);
        if (manual_it != manual_justification.end()) {
            item.justification = manual_it->second;
        } else if (copy.target_location_id.has_value() && scope_set.find(*copy.target_location_id) == scope_set.end()) {
            item.justification = "copy belongs to another target location";
        } else {
            item.justification = "scanned copy was not expected in this inventory scope";
        }

        final_items.push_back(std::move(item));
    }

    int on_shelf_count = 0;
    int justified_count = 0;
    int missing_count = 0;

    for (const auto& item : final_items) {
        switch (item.result) {
            case InventoryItemResult::OnShelf:
                ++on_shelf_count;
                break;
            case InventoryItemResult::Justified:
                ++justified_count;
                break;
            case InventoryItemResult::Missing:
                ++missing_count;
                break;
            default:
                break;
        }
    }

    const std::string summary_result =
        "on_shelf=" + std::to_string(on_shelf_count) + ";justified=" + std::to_string(justified_count) +
        ";missing=" + std::to_string(missing_count);

    repository_.replace_session_items(normalized_session, final_items);
    repository_.complete_session(normalized_session, on_shelf_count, justified_count, missing_count, summary_result);

    logger_("inventory completed public_id=" + normalized_session + " " + summary_result);
    return get_inventory_result(normalized_session);
}

InventoryResult InventoryService::get_inventory_result(const std::string& session_public_id) const {
    const std::string normalized_session = normalize_text(session_public_id);
    if (normalized_session.empty()) {
        throw errors::ValidationError("session_public_id is required");
    }

    const auto session = repository_.get_session_by_public_id(normalized_session);
    if (!session.has_value()) {
        throw errors::NotFoundError("Inventory session not found. public_id=" + normalized_session);
    }

    const std::vector<InventoryItem> items = repository_.list_items_for_session(normalized_session);

    InventoryResult result;
    result.session = *session;

    for (const auto& item : items) {
        switch (item.result) {
            case InventoryItemResult::OnShelf:
                result.on_shelf.push_back(item);
                break;
            case InventoryItemResult::Justified:
                result.justified.push_back(item);
                break;
            case InventoryItemResult::Missing:
                result.missing.push_back(item);
                break;
            default:
                throw errors::InventoryError("Unsupported inventory item result during result assembly");
        }
    }

    return result;
}

std::vector<InventorySession> InventoryService::list_sessions(int limit, int offset) const {
    if (limit <= 0) {
        throw errors::ValidationError("inventory list limit must be greater than zero");
    }
    if (offset < 0) {
        throw errors::ValidationError("inventory list offset cannot be negative");
    }

    return repository_.list_sessions(limit, offset);
}

std::string InventoryService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

std::vector<std::string> InventoryService::resolve_scope_location_ids(const locations::Location& root,
                                                                      const std::vector<std::string>& subtree_ids) {
    if (root.type == locations::LocationType::Shelf) {
        return {root.public_id};
    }

    if (root.type == locations::LocationType::Rack || root.type == locations::LocationType::Room) {
        return subtree_ids;
    }

    throw errors::InventoryError("Inventory scope must be ROOM, RACK or SHELF");
}

copies::BookCopy InventoryService::resolve_copy_for_scan(const std::string& scan_code) const {
    // Current resolver supports direct identifiers and inventory numbers,
    // and acts as a placeholder for future barcode decoding.
    const auto by_public_id = repository_.get_copy_by_public_id(scan_code);
    if (by_public_id.has_value()) {
        return *by_public_id;
    }

    const auto by_inventory_number = repository_.get_copy_by_inventory_number(scan_code);
    if (by_inventory_number.has_value()) {
        return *by_inventory_number;
    }

    throw errors::NotFoundError("Copy not found for scanned code: " + scan_code);
}

void InventoryService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Inventory, year, next);
    initialized_years_.insert(year);
}

} // namespace inventory
