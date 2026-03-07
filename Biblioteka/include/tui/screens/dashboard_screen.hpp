#pragma once

#include "tui/controllers/dashboard_controller.hpp"
#include "tui/screens/screen.hpp"

namespace tui {

class DashboardScreen : public Screen {
public:
    explicit DashboardScreen(DashboardController& controller);

    [[nodiscard]] std::string title() const override;
    void show() override;

private:
    DashboardController& controller_;
};

} // namespace tui
