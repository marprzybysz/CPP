#include "ui/controllers/application_controller.hpp"

#include <memory>

#include "ui/screens/dashboard_screen.hpp"
#include "ui/screen_manager.hpp"

namespace ui::controllers {

ApplicationController::ApplicationController(Library& library, ScreenManager& screen_manager)
    : library_(library), screen_manager_(screen_manager) {}

void ApplicationController::bootstrap() {
    (void)library_;
    screen_manager_.register_screen(std::make_unique<screens::DashboardScreen>());
    screen_manager_.set_active("dashboard");
}

} // namespace ui::controllers
