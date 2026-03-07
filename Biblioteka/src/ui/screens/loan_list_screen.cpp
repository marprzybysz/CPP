#include "ui/screens/loan_list_screen.hpp"

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

std::string mode_label(ui::controllers::LoanListMode mode) {
    return mode == ui::controllers::LoanListMode::Active ? "AKTYWNE" : "HISTORIA";
}

std::string display_copy(const ui::controllers::LoanListEntry& row) {
    if (row.row.inventory_number.has_value()) {
        return *row.row.inventory_number;
    }
    if (row.row.copy_public_id.has_value()) {
        return *row.row.copy_public_id;
    }
    return "-";
}

std::string display_status(const ui::controllers::LoanListEntry& row) {
    std::string status = reservations::to_string(row.row.reservation.status);
    if (row.overdue) {
        status += " [OPOZNIONE]";
    }
    return status;
}

} // namespace

namespace ui::screens {

LoanListScreen::LoanListScreen(controllers::LoansController& controller)
    : controller_(controller),
      header_("Wypozyczenia", "Lista aktywnych i historia"),
      search_box_("", "szukaj: reader:/copy:/card:/inv: lub fraza"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "a: nowe", "r: zwrot", "p: przedluzenie", "/: szukaj", "h: historia/aktywne", "q: powrot"}) {}

std::string LoanListScreen::id() const {
    return "loans";
}

std::string LoanListScreen::title() const {
    return "Wypozyczenia";
}

void LoanListScreen::on_show() {
    search_input_mode_ = false;
    reload_results();
}

void LoanListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Tryb listy: " + mode_label(state_.mode));
    renderer.draw_line("Filtr: " + (state_.query.empty() ? std::string{"(brak)"} : state_.query));
    search_box_.render(renderer);
    renderer.draw_separator();

    renderer.draw_line("Wyniki: " + std::to_string(rows_.size()));
    renderer.draw_line("public_id | czytelnik | egzemplarz | data wypozyczenia | termin zwrotu | status | przedluzenia");
    results_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: a=nowe | r=zwrot | p=przedluz | enter=szczegoly | /=szukaj | h=historia/aktywne");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LoanListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (search_input_mode_) {
        if (event.key == Key::Escape || event.key == Key::Back || event.key == Key::Quit) {
            search_input_mode_ = false;
            search_box_.set_focus(false);
            status_bar_.set("Anulowano wyszukiwanie", components::StatusType::Info);
            return;
        }

        if (event.raw == "clear") {
            state_.query.clear();
            search_box_.set_query("");
        } else {
            state_.query = event.raw;
            search_box_.set_query(event.raw);
        }

        search_input_mode_ = false;
        search_box_.set_focus(false);
        reload_results();
        return;
    }

    if (event.key == Key::Up) {
        results_view_.move_up();
        sync_selected_loan();
        return;
    }

    if (event.key == Key::Down) {
        results_view_.move_down();
        sync_selected_loan();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_loan();
        if (controller_.selected_loan().has_value()) {
            manager.set_active("loan_details");
        } else {
            status_bar_.set("Brak zaznaczonego wypozyczenia", components::StatusType::Warning);
        }
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }

    if (event.raw.empty()) {
        return;
    }

    if (is_command(event.raw, '/')) {
        search_input_mode_ = true;
        search_box_.set_focus(true);
        status_bar_.set("Tryb wyszukiwania: wpisz fraze lub clear", components::StatusType::Info);
        return;
    }

    if (is_command(event.raw, 'h')) {
        state_.mode = (state_.mode == controllers::LoanListMode::Active) ? controllers::LoanListMode::History
                                                                          : controllers::LoanListMode::Active;
        reload_results();
        return;
    }

    if (is_command(event.raw, 'a')) {
        manager.set_active("loan_create");
        return;
    }

    if (is_command(event.raw, 'r')) {
        sync_selected_loan();
        if (!controller_.selected_loan().has_value()) {
            status_bar_.set("Wybierz wypozyczenie do zwrotu", components::StatusType::Warning);
            return;
        }
        manager.set_active("loan_return_dialog");
        return;
    }

    if (is_command(event.raw, 'p')) {
        sync_selected_loan();
        if (!controller_.selected_loan().has_value()) {
            status_bar_.set("Wybierz wypozyczenie do przedluzenia", components::StatusType::Warning);
            return;
        }
        manager.set_active("loan_extend_dialog");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

void LoanListScreen::reload_results() {
    try {
        rows_ = controller_.list_loans(state_);
        refresh_rows();
        sync_selected_loan();
        status_bar_.set("Wczytano wypozyczenia", components::StatusType::Success);
    } catch (const std::exception& e) {
        rows_.clear();
        results_view_.set_items({});
        controller_.clear_selected_loan();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void LoanListScreen::sync_selected_loan() {
    const auto index = selected_result_index();
    if (!index.has_value()) {
        controller_.clear_selected_loan();
        return;
    }

    controller_.set_selected_loan(rows_[*index].row.reservation.public_id);
}

void LoanListScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(this->rows_.size());

    for (const auto& row : this->rows_) {
        rows.push_back(row.row.reservation.public_id + " | " + row.row.reader_name + " (" + row.row.card_number + ") | " +
                       display_copy(row) + " | " + row.row.reservation.reservation_date + " | " +
                       row.row.reservation.expiration_date + " | " + display_status(row) + " | " +
                       std::to_string(row.row.extension_count));
    }

    results_view_.set_items(std::move(rows));
}

std::optional<std::size_t> LoanListScreen::selected_result_index() const {
    if (rows_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = results_view_.selected_index();
    if (index >= rows_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
