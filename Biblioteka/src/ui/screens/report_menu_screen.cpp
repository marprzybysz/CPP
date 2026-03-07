#include "ui/screens/report_menu_screen.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

std::string trim_copy(std::string raw) {
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())) != 0) {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())) != 0) {
        raw.pop_back();
    }
    return raw;
}

} // namespace

namespace ui::screens {

ReportMenuScreen::ReportMenuScreen(controllers::ReportsController& controller)
    : controller_(controller),
      header_("Raporty", "Wybierz typ raportu"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: wybor", "enter: generuj", "/: filtr dat", "q: powrot"}),
      report_types_({reports::ReportType::OverdueBooks,
                     reports::ReportType::DelinquentReaders,
                     reports::ReportType::MostBorrowedBooks,
                     reports::ReportType::InventoryState,
                     reports::ReportType::ArchivedBooks,
                     reports::ReportType::CopiesInRepair}) {
    std::vector<std::string> rows;
    rows.reserve(report_types_.size());
    for (const auto type : report_types_) {
        rows.push_back(reports::to_string(type));
    }
    reports_view_.set_items(std::move(rows));
}

std::string ReportMenuScreen::id() const {
    return "reports";
}

std::string ReportMenuScreen::title() const {
    return "Raporty";
}

void ReportMenuScreen::on_show() {
    filter_input_mode_ = false;
}

void ReportMenuScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Filtr dat: " + date_filter_text());
    renderer.draw_separator();
    reports_view_.render(renderer);

    renderer.draw_separator();
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReportMenuScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (filter_input_mode_) {
        if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
            filter_input_mode_ = false;
            status_bar_.set("Anulowano filtr dat", components::StatusType::Info);
            return;
        }

        apply_filter_input(event.raw);
        filter_input_mode_ = false;
        return;
    }

    if (event.key == Key::Up) {
        reports_view_.move_up();
        return;
    }

    if (event.key == Key::Down) {
        reports_view_.move_down();
        return;
    }

    if (event.key == Key::Enter) {
        try {
            const auto type = selected_report_type();
            if (!type.has_value()) {
                status_bar_.set("Brak wybranego typu raportu", components::StatusType::Warning);
                return;
            }

            auto result = controller_.generate_report(*type, filter_);
            controller_.set_last_result(std::move(result));
            status_bar_.set("Wygenerowano raport", components::StatusType::Success);
            manager.set_active("report_result");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (!event.raw.empty() && event.raw.size() == 1 && event.raw[0] == '/') {
        filter_input_mode_ = true;
        status_bar_.set("Filtr: from:YYYY-MM-DD to:YYYY-MM-DD lub clear", components::StatusType::Info);
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }
}

std::optional<reports::ReportType> ReportMenuScreen::selected_report_type() const {
    const auto index = reports_view_.selected_index();
    if (index >= report_types_.size()) {
        return std::nullopt;
    }

    return report_types_[index];
}

std::string ReportMenuScreen::date_filter_text() const {
    return "from=" + std::string(filter_.from_date.value_or("-")) + " to=" + std::string(filter_.to_date.value_or("-"));
}

void ReportMenuScreen::apply_filter_input(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized == "clear") {
        filter_.from_date = std::nullopt;
        filter_.to_date = std::nullopt;
        status_bar_.set("Wyczyszczono filtr dat", components::StatusType::Info);
        return;
    }

    std::string token;
    std::string input = normalized;
    while (!input.empty()) {
        const auto pos = input.find(' ');
        if (pos == std::string::npos) {
            token = input;
            input.clear();
        } else {
            token = input.substr(0, pos);
            input = trim_copy(input.substr(pos + 1));
        }

        if (token.rfind("from:", 0) == 0) {
            filter_.from_date = token.substr(5);
        } else if (token.rfind("to:", 0) == 0) {
            filter_.to_date = token.substr(3);
        }
    }

    status_bar_.set("Zastosowano filtr dat", components::StatusType::Success);
}

} // namespace ui::screens
