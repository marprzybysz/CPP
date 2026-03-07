#pragma once

#include <optional>
#include <string>
#include <vector>

#include "inventory/inventory.hpp"
#include "ui/controllers/locations_controller.hpp"

class Library;

namespace ui::controllers {

class InventoryController {
public:
    explicit InventoryController(Library& library);

    [[nodiscard]] std::vector<inventory::InventorySession> list_sessions(int limit = 80) const;
    [[nodiscard]] std::vector<LocationTreeEntry> list_locations() const;
    [[nodiscard]] inventory::InventorySession get_session(const std::string& session_public_id) const;
    [[nodiscard]] inventory::InventoryResult get_result(const std::string& session_public_id) const;

    inventory::InventorySession start_session(const std::string& location_public_id, const std::string& started_by);
    inventory::InventoryScannedCopy add_scan(const std::string& session_public_id,
                                             const std::string& scan_code,
                                             const std::string& note = "");
    inventory::InventoryResult finish_session(const std::string& session_public_id);

    void set_selected_session(const std::string& session_public_id);
    void clear_selected_session();
    [[nodiscard]] const std::optional<std::string>& selected_session() const;

private:
    Library& library_;
    LocationsController locations_controller_;
    std::optional<std::string> selected_session_;
};

} // namespace ui::controllers
