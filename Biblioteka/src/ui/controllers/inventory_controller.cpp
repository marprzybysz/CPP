#include "ui/controllers/inventory_controller.hpp"

#include "library.hpp"

namespace ui::controllers {

InventoryController::InventoryController(Library& library)
    : library_(library), locations_controller_(library_) {}

std::vector<inventory::InventorySession> InventoryController::list_sessions(int limit) const {
    return library_.list_inventory_sessions(limit, 0);
}

std::vector<LocationTreeEntry> InventoryController::list_locations() const {
    return locations_controller_.list_location_tree();
}

inventory::InventorySession InventoryController::get_session(const std::string& session_public_id) const {
    return library_.get_inventory_result(session_public_id).session;
}

inventory::InventoryResult InventoryController::get_result(const std::string& session_public_id) const {
    return library_.get_inventory_result(session_public_id);
}

inventory::InventorySession InventoryController::start_session(const std::string& location_public_id, const std::string& started_by) {
    inventory::StartInventoryInput input;
    input.location_public_id = location_public_id;
    input.started_by = started_by;

    const auto created = library_.start_inventory(input);
    selected_session_ = created.public_id;
    return created;
}

inventory::InventoryScannedCopy InventoryController::add_scan(const std::string& session_public_id,
                                                              const std::string& scan_code,
                                                              const std::string& note) {
    inventory::RegisterScannedCopyInput input;
    input.scan_code = scan_code;
    input.note = note;

    return library_.register_inventory_scan(session_public_id, input);
}

inventory::InventoryResult InventoryController::finish_session(const std::string& session_public_id) {
    inventory::FinishInventoryInput input;
    const auto result = library_.finish_inventory(session_public_id, input);
    selected_session_ = result.session.public_id;
    return result;
}

void InventoryController::set_selected_session(const std::string& session_public_id) {
    selected_session_ = session_public_id;
}

void InventoryController::clear_selected_session() {
    selected_session_.reset();
}

const std::optional<std::string>& InventoryController::selected_session() const {
    return selected_session_;
}

} // namespace ui::controllers
