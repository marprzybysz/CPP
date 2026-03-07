#include "ui/screens/reader_details_screen.hpp"

#include <cctype>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

std::string optional_text(const std::optional<std::string>& value) {
    return value.has_value() ? *value : "-";
}

} // namespace

namespace ui::screens {

ReaderDetailsScreen::ReaderDetailsScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Szczegoly czytelnika"),
      footer_({"e: edytuj", "b: zablokuj", "u: odblokuj", "h: historia", "r: reputacja", "q: powrot"}) {}

std::string ReaderDetailsScreen::id() const {
    return "reader_details";
}

std::string ReaderDetailsScreen::title() const {
    return "Szczegoly czytelnika";
}

void ReaderDetailsScreen::on_show() {
    current_reader_.reset();

    try {
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Brak wybranego czytelnika", components::StatusType::Warning);
            return;
        }

        current_reader_ = controller_.get_reader_details(*controller_.selected_reader());
        status_bar_.set("Wczytano szczegoly czytelnika", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_reader_.has_value()) {
        renderer.draw_line("Brak danych czytelnika.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const auto& reader = *current_reader_;
    renderer.draw_line("Numer karty: " + reader.card_number);
    renderer.draw_line("Imie i nazwisko: " + reader.first_name + " " + reader.last_name);
    renderer.draw_line("Email: " + reader.email);
    renderer.draw_line("Telefon: " + optional_text(reader.phone));
    renderer.draw_line("Status konta: " + readers::to_string(reader.account_status));
    renderer.draw_line("Punkty reputacji: " + std::to_string(reader.reputation_points));
    renderer.draw_line("Blokada: " + std::string(reader.is_blocked ? "TAK" : "NIE") +
                       (reader.block_reason.has_value() ? (" (" + *reader.block_reason + ")") : ""));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("readers");
        return;
    }

    if (is_command(event.raw, 'e')) {
        if (current_reader_.has_value()) {
            controller_.begin_edit(current_reader_->public_id);
            manager.set_active("reader_form");
        }
        return;
    }

    if (is_command(event.raw, 'b')) {
        if (current_reader_.has_value()) {
            manager.set_active("reader_block_dialog");
        }
        return;
    }

    if (is_command(event.raw, 'u')) {
        if (current_reader_.has_value()) {
            manager.set_active("reader_unblock_dialog");
        }
        return;
    }

    if (is_command(event.raw, 'h')) {
        manager.set_active("reader_loan_history");
        return;
    }

    if (is_command(event.raw, 'r')) {
        manager.set_active("reader_reputation");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

} // namespace ui::screens
