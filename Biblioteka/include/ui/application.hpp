#pragma once

#include "ui/controllers/application_controller.hpp"
#include "ui/input_handler.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

class Library;

namespace ui {

class Application {
public:
    explicit Application(Library& library);

    int run();

private:
    ScreenManager screen_manager_;
    Renderer renderer_;
    InputHandler input_handler_;
    controllers::ApplicationController application_controller_;
};

} // namespace ui
