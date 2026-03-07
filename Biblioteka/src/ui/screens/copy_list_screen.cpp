#include "ui/screens/copy_list_screen.hpp"

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

std::string trim_copy(std::string raw) {
    while (!raw.empty() && raw.front() == ' ') {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && raw.back() == ' ') {
        raw.pop_back();
    }
    return raw;
}

const std::vector<std::optional<copies::CopyStatus>>& status_filter_options() {
    static const std::vector<std::optional<copies::CopyStatus>> options = {
        std::nullopt,
        copies::CopyStatus::OnShelf,
        copies::CopyStatus::Loaned,
        copies::CopyStatus::Reserved,
        copies::CopyStatus::InRepair,
        copies::CopyStatus::Archived,
        copies::CopyStatus::Lost,
    };
    return options;
}

std::string location_display(const std::optional<std::string>& location) {
    return location.has_value() ? *location : "-";
}

} // namespace

namespace ui::screens {

CopyListScreen::CopyListScreen(controllers::CopiesController& controller)
    : controller_(controller),
      header_("Egzemplarze", "Lista, filtrowanie i operacje"),
      search_box_("", "wpisz filtr ksiazki (tytul/autor/isbn/public_id)"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "s: status", "l: lokalizacja", "a: dodaj", "/: filtr ksiazki", "f: filtr statusu", "q: powrot"}) {
    filter_.status = status_filter_options()[status_filter_index_];
}

std::string CopyListScreen::id() const {
    return "copies";
}

std::string CopyListScreen::title() const {
    return "Egzemplarze";
}

void CopyListScreen::on_show() {
    search_input_mode_ = false;
    reload_results();
}

void CopyListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Filtr ksiazki: " + (filter_.book_query.empty() ? std::string{"(brak)"} : filter_.book_query));
    renderer.draw_line("Filtr statusu: " + status_filter_label());
    search_box_.render(renderer);
    renderer.draw_separator();

    renderer.draw_line("Wyniki: " + std::to_string(rows_.size()));
    renderer.draw_line("inventory_number | tytul ksiazki | status | aktualna lokalizacja | docelowa lokalizacja");
    results_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=szczegoly | s=status | l=lokalizacja | a=dodaj | /=filtr ksiazki | f=filtr statusu");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void CopyListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (search_input_mode_) {
        if (event.key == Key::Escape || event.key == Key::Back || event.key == Key::Quit) {
            search_input_mode_ = false;
            search_box_.set_focus(false);
            status_bar_.set("Anulowano edycje filtra ksiazki", components::StatusType::Info);
            return;
        }

        apply_book_filter(event.raw);
        search_input_mode_ = false;
        search_box_.set_focus(false);
        reload_results();
        return;
    }

    if (event.key == Key::Up) {
        results_view_.move_up();
        sync_selected_copy();
        return;
    }

    if (event.key == Key::Down) {
        results_view_.move_down();
        sync_selected_copy();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_copy();
        if (controller_.selected_copy().has_value()) {
            manager.set_active("copy_details");
        } else {
            status_bar_.set("Brak zaznaczonego egzemplarza", components::StatusType::Warning);
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
        status_bar_.set("Tryb filtra ksiazki: wpisz fraze lub clear", components::StatusType::Info);
        return;
    }

    if (is_command(event.raw, 'f')) {
        cycle_status_filter();
        reload_results();
        status_bar_.set("Przelaczono filtr statusu", components::StatusType::Success);
        return;
    }

    if (is_command(event.raw, 'a')) {
        manager.set_active("copy_form");
        return;
    }

    if (is_command(event.raw, 's')) {
        sync_selected_copy();
        if (!controller_.selected_copy().has_value()) {
            status_bar_.set("Wybierz egzemplarz do zmiany statusu", components::StatusType::Warning);
            return;
        }
        manager.set_active("copy_status");
        return;
    }

    if (is_command(event.raw, 'l')) {
        sync_selected_copy();
        if (!controller_.selected_copy().has_value()) {
            status_bar_.set("Wybierz egzemplarz do zmiany lokalizacji", components::StatusType::Warning);
            return;
        }
        manager.set_active("copy_location");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

void CopyListScreen::reload_results() {
    try {
        rows_ = controller_.list_copies(filter_);
        refresh_rows();
        sync_selected_copy();
        status_bar_.set("Wczytano egzemplarze", components::StatusType::Success);
    } catch (const std::exception& e) {
        rows_.clear();
        results_view_.set_items({});
        controller_.clear_selected_copy();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void CopyListScreen::sync_selected_copy() {
    const auto index = selected_result_index();
    if (!index.has_value()) {
        controller_.clear_selected_copy();
        return;
    }
    controller_.set_selected_copy(rows_[*index].copy.public_id);
}

void CopyListScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(this->rows_.size());
    for (const auto& row : this->rows_) {
        rows.push_back(format_row(row));
    }
    results_view_.set_items(std::move(rows));
}

void CopyListScreen::cycle_status_filter() {
    const auto& options = status_filter_options();
    status_filter_index_ = (status_filter_index_ + 1) % options.size();
    filter_.status = options[status_filter_index_];
}

void CopyListScreen::apply_book_filter(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized == "clear") {
        filter_.book_query.clear();
        search_box_.set_query("");
        return;
    }

    filter_.book_query = normalized;
    search_box_.set_query(normalized);
}

std::optional<std::size_t> CopyListScreen::selected_result_index() const {
    if (rows_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = results_view_.selected_index();
    if (index >= rows_.size()) {
        return std::nullopt;
    }

    return index;
}

std::string CopyListScreen::status_filter_label() const {
    if (!filter_.status.has_value()) {
        return "ALL";
    }
    return copies::to_string(*filter_.status);
}

std::string CopyListScreen::format_row(const controllers::CopyListItem& row) const {
    return row.copy.inventory_number + " | " + row.book_title + " | " + copies::to_string(row.copy.status) + " | " +
           location_display(row.current_location_name) + " | " + location_display(row.target_location_name);
}

} // namespace ui::screens
