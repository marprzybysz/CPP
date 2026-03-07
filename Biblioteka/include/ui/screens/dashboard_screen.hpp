#pragma once

#include "ui/components/menu.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class DashboardScreen : public Screen {
public:
    DashboardScreen();

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    components::Menu menu_;
};

} // namespace ui::screens
