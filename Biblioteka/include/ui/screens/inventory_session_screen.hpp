#pragma once

#include <optional>
#include <string>
#include <vector>

#include "inventory/inventory.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/inventory_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class InventorySessionScreen : public Screen {
public:
    explicit InventorySessionScreen(controllers::InventoryController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_line_input() const override { return true; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void load_locations();
    void load_session();
    [[nodiscard]] std::optional<std::size_t> selected_location_index() const;

    controllers::InventoryController& controller_;
    components::Header header_;
    components::ListView locations_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    std::vector<controllers::LocationTreeEntry> locations_;
    std::optional<inventory::InventorySession> session_;
    std::string started_by_{"tui"};
    std::string last_scan_code_;
};

} // namespace ui::screens
