#include "ui/screens/copy_details_screen.hpp"

#include <cctype>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

std::string optional_value(const std::optional<std::string>& value) {
    return value.has_value() ? *value : "-";
}

} // namespace

namespace ui::screens {

CopyDetailsScreen::CopyDetailsScreen(controllers::CopiesController& controller)
    : controller_(controller),
      header_("Egzemplarze", "Szczegoly egzemplarza"),
      footer_({"s: zmien status", "l: zmien lokalizacje", "q: powrot"}) {}

std::string CopyDetailsScreen::id() const {
    return "copy_details";
}

std::string CopyDetailsScreen::title() const {
    return "Szczegoly egzemplarza";
}

void CopyDetailsScreen::on_show() {
    current_copy_.reset();

    try {
        if (!controller_.selected_copy().has_value()) {
            status_bar_.set("Brak wybranego egzemplarza", components::StatusType::Warning);
            return;
        }

        current_copy_ = controller_.get_copy_details(*controller_.selected_copy());
        status_bar_.set("Wczytano szczegoly egzemplarza", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void CopyDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_copy_.has_value()) {
        renderer.draw_line("Brak danych egzemplarza.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const auto& row = *current_copy_;
    renderer.draw_line("Public ID: " + row.copy.public_id);
    renderer.draw_line("Inventory number: " + row.copy.inventory_number);
    renderer.draw_line("Ksiazka: " + row.book_title + " [" + row.book_public_id + "]");
    renderer.draw_line("Status: " + copies::to_string(row.copy.status));
    renderer.draw_line("Aktualna lokalizacja: " + optional_value(row.current_location_name));
    renderer.draw_line("Docelowa lokalizacja: " + optional_value(row.target_location_name));
    renderer.draw_line("Condition note: " + row.copy.condition_note);
    renderer.draw_line("Acquisition date: " + optional_value(row.copy.acquisition_date));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void CopyDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("copies");
        return;
    }

    if (is_command(event.raw, 's')) {
        manager.set_active("copy_status");
        return;
    }

    if (is_command(event.raw, 'l')) {
        manager.set_active("copy_location");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

} // namespace ui::screens
