#include "ui/screens/report_result_screen.hpp"

#include <algorithm>

#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

ReportResultScreen::ReportResultScreen(controllers::ReportsController& controller)
    : controller_(controller),
      header_("Raporty", "Wynik raportu"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"q: powrot"}) {}

std::string ReportResultScreen::id() const {
    return "report_result";
}

std::string ReportResultScreen::title() const {
    return "Wynik raportu";
}

void ReportResultScreen::on_show() {
    result_ = controller_.last_result();
    if (result_.has_value()) {
        status_bar_.set("Wczytano wynik raportu", components::StatusType::Success);
    } else {
        status_bar_.set("Brak wyniku raportu", components::StatusType::Warning);
    }
}

void ReportResultScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!result_.has_value()) {
        renderer.draw_line("Brak danych raportu.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Raport: " + result_->title);
    renderer.draw_line("Liczba rekordow: " + std::to_string(result_->rows.size()));
    renderer.draw_separator();

    const std::size_t shown = std::min<std::size_t>(result_->rows.size(), 80);
    for (std::size_t i = 0; i < shown; ++i) {
        renderer.draw_line(result_->rows[i]);
    }
    if (result_->rows.size() > shown) {
        renderer.draw_line("..." + std::to_string(result_->rows.size() - shown) + " wiecej");
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReportResultScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("reports");
    }
}

} // namespace ui::screens
