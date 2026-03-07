#include "ui/screens/reservation_details_screen.hpp"

#include <cctype>
#include <string>

#include "errors/error_mapper.hpp"
#include "reservations/reservation.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

std::string optional_int_text(const std::optional<int>& value) {
    return value.has_value() ? std::to_string(*value) : "-";
}

std::string optional_text(const std::optional<std::string>& value) {
    return value.has_value() ? *value : "-";
}

} // namespace

namespace ui::screens {

ReservationDetailsScreen::ReservationDetailsScreen(controllers::ReservationsController& controller)
    : controller_(controller),
      header_("Rezerwacje", "Szczegoly rezerwacji"),
      footer_({"c: anuluj", "q: powrot"}) {}

std::string ReservationDetailsScreen::id() const {
    return "reservation_details";
}

std::string ReservationDetailsScreen::title() const {
    return "Szczegoly rezerwacji";
}

void ReservationDetailsScreen::on_show() {
    current_.reset();

    try {
        if (!controller_.selected_reservation().has_value()) {
            status_bar_.set("Brak wybranej rezerwacji", components::StatusType::Warning);
            return;
        }

        current_ = controller_.get_reservation_details(*controller_.selected_reservation());
        status_bar_.set("Wczytano szczegoly rezerwacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReservationDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_.has_value()) {
        renderer.draw_line("Brak danych rezerwacji.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const auto& row = current_->row;
    renderer.draw_line("Public ID: " + row.reservation.public_id);
    renderer.draw_line("Czytelnik: " + row.reader_name + " [" + row.card_number + "]");
    renderer.draw_line("Target: " + current_->target_label);
    renderer.draw_line("Copy public_id: " + optional_text(row.copy_public_id));
    renderer.draw_line("Book id: " + optional_int_text(row.reservation.book_id));
    renderer.draw_line("Data rezerwacji: " + row.reservation.reservation_date);
    renderer.draw_line("Data wygasniecia: " + row.reservation.expiration_date);
    renderer.draw_line("Status: " + reservations::to_string(row.reservation.status));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReservationDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("reservations");
        return;
    }

    if (is_command(event.raw, 'c')) {
        try {
            if (!controller_.selected_reservation().has_value()) {
                status_bar_.set("Brak wybranej rezerwacji", components::StatusType::Warning);
                return;
            }

            controller_.cancel_reservation(*controller_.selected_reservation());
            status_bar_.set("Anulowano rezerwacje", components::StatusType::Success);
            manager.set_active("reservations");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (!event.raw.empty()) {
        status_bar_.set("Nieznana komenda", components::StatusType::Warning);
    }
}

} // namespace ui::screens
