#include "ui/screens/dashboard_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

ui::components::Menu build_dashboard_menu() {
    return ui::components::Menu({{"dashboard", "Dashboard"},
                                 {"books", "Ksiazki"},
                                 {"copies", "Egzemplarze"},
                                 {"readers", "Czytelnicy"},
                                 {"loans", "Wypozyczenia"},
                                 {"reservations", "Rezerwacje"},
                                 {"locations", "Lokalizacje"},
                                 {"inventory", "Inwentaryzacja"},
                                 {"reports", "Raporty"},
                                 {"notes", "Notatki"},
                                 {"logs", "Logi zdarzen"},
                                 {"exit", "Wyjscie"}});
}

} // namespace

namespace ui::screens {

DashboardScreen::DashboardScreen(controllers::DashboardController& controller)
    : controller_(controller), menu_(build_dashboard_menu()) {}

std::string DashboardScreen::id() const {
    return "dashboard";
}

std::string DashboardScreen::title() const {
    return "Biblioteka - Dashboard";
}

void DashboardScreen::on_show() {
    try {
        stats_ = controller_.load_stats();
    } catch (const std::exception& e) {
        stats_ = {};
        stats_.books.note = errors::to_user_message(e);
        stats_.copies.note = "metrics unavailable";
        stats_.readers.note = "metrics unavailable";
        stats_.active_loans.note = "metrics unavailable";
        stats_.overdue_books.note = "metrics unavailable";
    }
}

void DashboardScreen::render(Renderer& renderer) const {
    renderer.clear();
    renderer.draw_header(title());
    renderer.draw_line("Nawigacja: strzalki/w/s + Enter | b=powrot | q=wyjscie");
    renderer.draw_line("");
    renderer.draw_line("Statystyki:");
    renderer.draw_line("- Liczba ksiazek: " + format_metric(stats_.books));
    renderer.draw_line("- Liczba egzemplarzy: " + format_metric(stats_.copies));
    renderer.draw_line("- Liczba czytelnikow: " + format_metric(stats_.readers));
    renderer.draw_line("- Aktywne wypozyczenia: " + format_metric(stats_.active_loans));
    renderer.draw_line("- Przetrzymane ksiazki: " + format_metric(stats_.overdue_books));
    renderer.draw_line("");
    renderer.draw_line("Szybkie akcje / Menu glowne:");

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

    if (event.key == Key::Quit) {
        manager.clear_active();
        return;
    }

    if (event.key == Key::Back) {
        manager.set_active("dashboard");
        return;
    }

    if (event.key != Key::Enter) {
        return;
    }

    const auto& selected = menu_.selected();
    if (selected.id == "exit") {
        manager.clear_active();
        return;
    }

    manager.set_active(selected.id);
}

std::string DashboardScreen::format_metric(const controllers::DashboardMetric& metric) {
    if (metric.value.has_value()) {
        return std::to_string(*metric.value);
    }

    if (!metric.note.empty()) {
        return "N/A (" + metric.note + ")";
    }

    return "N/A";
}

} // namespace ui::screens
