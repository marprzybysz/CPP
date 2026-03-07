#include "ui/screens/loan_details_screen.hpp"

#include <cctype>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "reservations/reservation.hpp"
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

LoanDetailsScreen::LoanDetailsScreen(controllers::LoansController& controller)
    : controller_(controller),
      header_("Wypozyczenia", "Szczegoly wypozyczenia"),
      footer_({"r: zwrot", "p: przedluzenie", "q: powrot"}) {}

std::string LoanDetailsScreen::id() const {
    return "loan_details";
}

std::string LoanDetailsScreen::title() const {
    return "Szczegoly wypozyczenia";
}

void LoanDetailsScreen::on_show() {
    current_.reset();

    try {
        if (!controller_.selected_loan().has_value()) {
            status_bar_.set("Brak wybranego wypozyczenia", components::StatusType::Warning);
            return;
        }

        current_ = controller_.get_loan_details(*controller_.selected_loan());
        status_bar_.set("Wczytano szczegoly wypozyczenia", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void LoanDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_.has_value()) {
        renderer.draw_line("Brak danych wypozyczenia.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const auto& row = current_->row;
    renderer.draw_line("Public ID: " + row.reservation.public_id);
    renderer.draw_line("Czytelnik: " + row.reader_name + " [" + row.card_number + "]");
    renderer.draw_line("Egzemplarz: " + optional_text(row.inventory_number) + " (" + optional_text(row.copy_public_id) + ")");
    renderer.draw_line("Data wypozyczenia: " + row.reservation.reservation_date);
    renderer.draw_line("Termin zwrotu: " + row.reservation.expiration_date);
    renderer.draw_line("Status: " + reservations::to_string(row.reservation.status));
    renderer.draw_line("Liczba przedluzen: " + std::to_string(row.extension_count));
    renderer.draw_line("Opoznienie: " + std::string(current_->overdue ? "TAK" : "NIE"));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LoanDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("loans");
        return;
    }

    if (is_command(event.raw, 'r')) {
        manager.set_active("loan_return_dialog");
        return;
    }

    if (is_command(event.raw, 'p')) {
        manager.set_active("loan_extend_dialog");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

} // namespace ui::screens
