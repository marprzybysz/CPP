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

class InventoryListScreen : public Screen {
public:
    explicit InventoryListScreen(controllers::InventoryController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_sessions();
    void refresh_rows();
    void sync_selected_session();
    [[nodiscard]] std::optional<std::size_t> selected_index() const;

    controllers::InventoryController& controller_;
    components::Header header_;
    components::ListView sessions_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::vector<inventory::InventorySession> sessions_;
};

} // namespace ui::screens
