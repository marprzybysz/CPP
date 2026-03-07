#include "ui/application.hpp"

#include "ui/screens/screen.hpp"

namespace ui {

Application::Application(Library& library) : application_controller_(library, screen_manager_) {
    application_controller_.bootstrap();
}

int Application::run() {
    while (screen_manager_.has_active()) {
        Screen* screen = screen_manager_.active_screen();
        if (screen == nullptr) {
            break;
        }

        screen->render(renderer_);
        const InputEvent event = input_handler_.read_event();
        screen->handle_input(event, screen_manager_);
    }

    return 0;
}

} // namespace ui
