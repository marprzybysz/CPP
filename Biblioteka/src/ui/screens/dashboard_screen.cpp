#include "ui/screens/dashboard_screen.hpp"

#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

DashboardScreen::DashboardScreen()
    : menu_({{"books", "Ksiazki (w przygotowaniu)"},
             {"copies", "Egzemplarze (w przygotowaniu)"},
             {"readers", "Czytelnicy (w przygotowaniu)"},
             {"reservations", "Rezerwacje (w przygotowaniu)"},
             {"reports", "Raporty (w przygotowaniu)"},
             {"exit", "Wyjscie"}}) {}

std::string DashboardScreen::id() const {
    return "dashboard";
}

std::string DashboardScreen::title() const {
    return "System Biblioteka - TUI";
}

void DashboardScreen::render(Renderer& renderer) const {
    renderer.clear();
    renderer.draw_header(title());
    renderer.draw_line("Nawigacja: w/s + e");
    renderer.draw_line("");

    const auto& items = menu_.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const std::string prefix = (i == menu_.selected_index()) ? "> " : "  ";
        renderer.draw_line(prefix + items[i].label);
    }
}

void DashboardScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        menu_.move_up();
        return;
    }

    if (event.key == Key::Down) {
        menu_.move_down();
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back) {
        manager.clear_active();
        return;
    }

    if (event.key == Key::Enter && menu_.selected().id == "exit") {
        manager.clear_active();
    }
}

} // namespace ui::screens
