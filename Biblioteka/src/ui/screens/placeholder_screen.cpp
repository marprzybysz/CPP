#include "ui/screens/placeholder_screen.hpp"

#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

#include <string>
#include <utility>

namespace ui::screens {

PlaceholderScreen::PlaceholderScreen(std::string screen_id, std::string screen_title)
    : id_(std::move(screen_id)), title_(std::move(screen_title)) {}

std::string PlaceholderScreen::id() const {
    return id_;
}

std::string PlaceholderScreen::title() const {
    return title_;
}

void PlaceholderScreen::render(Renderer& renderer) const {
    renderer.clear();
    renderer.draw_header(title_);
    renderer.draw_line("Ten ekran jest w przygotowaniu.");
    renderer.draw_line("nacisnij b aby wrocic do Dashboard");
}

void PlaceholderScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Back || event.key == Key::Quit) {
        manager.set_active("dashboard");
    }
}

} // namespace ui::screens
