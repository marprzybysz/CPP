#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/locations_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class LocationTreeScreen : public Screen {
public:
    explicit LocationTreeScreen(controllers::LocationsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_tree();
    void refresh_rows();
    void sync_selected_location();
    [[nodiscard]] std::optional<std::size_t> selected_index() const;

    controllers::LocationsController& controller_;
    components::Header header_;
    components::ListView tree_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::vector<controllers::LocationTreeEntry> rows_;
};

} // namespace ui::screens
