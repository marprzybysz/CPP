#include "ui/screens/reservation_list_screen.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

const std::vector<std::optional<reservations::ReservationStatus>>& status_filter_options() {
    static const std::vector<std::optional<reservations::ReservationStatus>> options = {
        std::nullopt,
        reservations::ReservationStatus::Active,
        reservations::ReservationStatus::Cancelled,
        reservations::ReservationStatus::Expired,
        reservations::ReservationStatus::Fulfilled,
    };
    return options;
}

std::string target_text(const ui::controllers::ReservationListEntry& row) {
    return row.target_label;
}

} // namespace

namespace ui::screens {

ReservationListScreen::ReservationListScreen(controllers::ReservationsController& controller)
    : controller_(controller),
      header_("Rezerwacje", "Lista i operacje"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "a: dodaj", "c: anuluj", "f: filtr statusu", "q: powrot"}) {
    list_state_.status = status_filter_options()[status_filter_index_];
}

std::string ReservationListScreen::id() const {
    return "reservations";
}

std::string ReservationListScreen::title() const {
    return "Rezerwacje";
}

void ReservationListScreen::on_show() {
    reload_results();
}

void ReservationListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Filtr statusu: " + status_filter_label());
    renderer.draw_separator();

    renderer.draw_line("Wyniki: " + std::to_string(rows_.size()));
    renderer.draw_line("public_id | czytelnik | target | data rezerwacji | data wygasniecia | status");
    results_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=szczegoly | a=dodaj | c=anuluj | f=filtr statusu");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReservationListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        results_view_.move_up();
        sync_selected_reservation();
        return;
    }

    if (event.key == Key::Down) {
        results_view_.move_down();
        sync_selected_reservation();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_reservation();
        if (controller_.selected_reservation().has_value()) {
            manager.set_active("reservation_details");
        } else {
            status_bar_.set("Brak zaznaczonej rezerwacji", components::StatusType::Warning);
        }
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }

    if (is_command(event.raw, 'a')) {
        manager.set_active("reservation_create");
        return;
    }

    if (is_command(event.raw, 'f')) {
        cycle_status_filter();
        reload_results();
        return;
    }

    if (is_command(event.raw, 'c')) {
        try {
            sync_selected_reservation();
            if (!controller_.selected_reservation().has_value()) {
                status_bar_.set("Wybierz rezerwacje do anulowania", components::StatusType::Warning);
                return;
            }

            controller_.cancel_reservation(*controller_.selected_reservation());
            status_bar_.set("Anulowano rezerwacje", components::StatusType::Success);
            reload_results();
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (!event.raw.empty()) {
        status_bar_.set("Nieznana komenda", components::StatusType::Warning);
    }
}

void ReservationListScreen::reload_results() {
    try {
        rows_ = controller_.list_reservations(list_state_);
        refresh_rows();
        sync_selected_reservation();
        status_bar_.set("Wczytano rezerwacje", components::StatusType::Success);
    } catch (const std::exception& e) {
        rows_.clear();
        results_view_.set_items({});
        controller_.clear_selected_reservation();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReservationListScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(this->rows_.size());

    for (const auto& row : this->rows_) {
        rows.push_back(row.row.reservation.public_id + " | " + row.row.reader_name + " (" + row.row.card_number + ") | " +
                       target_text(row) + " | " + row.row.reservation.reservation_date + " | " +
                       row.row.reservation.expiration_date + " | " + reservations::to_string(row.row.reservation.status));
    }

    results_view_.set_items(std::move(rows));
}

void ReservationListScreen::sync_selected_reservation() {
    const auto index = selected_result_index();
    if (!index.has_value()) {
        controller_.clear_selected_reservation();
        return;
    }

    controller_.set_selected_reservation(rows_[*index].row.reservation.public_id);
}

void ReservationListScreen::cycle_status_filter() {
    const auto& options = status_filter_options();
    status_filter_index_ = (status_filter_index_ + 1) % options.size();
    list_state_.status = options[status_filter_index_];
}

std::optional<std::size_t> ReservationListScreen::selected_result_index() const {
    if (rows_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = results_view_.selected_index();
    if (index >= rows_.size()) {
        return std::nullopt;
    }

    return index;
}

std::string ReservationListScreen::status_filter_label() const {
    if (!list_state_.status.has_value()) {
        return "ALL";
    }
    return reservations::to_string(*list_state_.status);
}

} // namespace ui::screens
