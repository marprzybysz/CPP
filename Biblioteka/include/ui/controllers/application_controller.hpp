#pragma once

#include "ui/controllers/dashboard_controller.hpp"

class Library;

namespace ui {

class ScreenManager;

namespace controllers {

class ApplicationController {
public:
    ApplicationController(Library& library, ScreenManager& screen_manager);

    void bootstrap();

private:
    Library& library_;
    ScreenManager& screen_manager_;
    DashboardController dashboard_controller_;
};

} // namespace controllers

} // namespace ui
