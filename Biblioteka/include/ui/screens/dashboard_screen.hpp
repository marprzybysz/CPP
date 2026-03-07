#pragma once

#include "ui/components/footer.hpp"
#include "ui/components/menu.hpp"
#include "ui/controllers/dashboard_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class DashboardScreen : public Screen {
public:
    explicit DashboardScreen(controllers::DashboardController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    [[nodiscard]] static std::string format_metric(const controllers::DashboardMetric& metric);

    controllers::DashboardController& controller_;
    controllers::DashboardStats stats_;
    components::Menu menu_;
    components::Footer footer_;
};

} // namespace ui::screens
