#include "ui/screens/reader_list_screen.hpp"

#include <cctype>
#include <optional>
#include <string>

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

} // namespace

namespace ui::screens {

ReaderListScreen::ReaderListScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Lista, wyszukiwanie i akcje"),
      search_box_("", "uzyj f:/l:/c:/e: np. l:Kowalski"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "a: dodaj", "e: edytuj", "b: zablokuj", "u: odblokuj", "h: historia", "r: reputacja", "/: szukaj", "q: powrot"}) {}

std::string ReaderListScreen::id() const {
    return "readers";
}

std::string ReaderListScreen::title() const {
    return "Czytelnicy";
}

void ReaderListScreen::on_show() {
    search_input_mode_ = false;
    reload_results();
}

void ReaderListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Wyszukiwanie: first='" + search_state_.first_name + "' last='" + search_state_.last_name +
                       "' card='" + search_state_.card_number + "' email='" + search_state_.email + "'");
    search_box_.render(renderer);
    renderer.draw_separator();

    renderer.draw_line("Wyniki: " + std::to_string(readers_.size()));
    results_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=szczegoly | a=dodaj | e=edytuj | b=blokuj | u=odblokuj | h=historia | r=reputacja | /=szukaj");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        results_view_.move_up();
        sync_selected_reader();
        return;
    }

    if (event.key == Key::Down) {
        results_view_.move_down();
        sync_selected_reader();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_reader();
        if (controller_.selected_reader().has_value()) {
            manager.set_active("reader_details");
        } else {
            status_bar_.set("Brak zaznaczonego czytelnika", components::StatusType::Warning);
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
        status_bar_.set("Tryb wyszukiwania: f:/l:/c:/e: lub clear", components::StatusType::Info);
        return;
    }

    if (is_command(event.raw, 'a')) {
        controller_.begin_create();
        manager.set_active("reader_form");
        return;
    }

    if (is_command(event.raw, 'e')) {
        sync_selected_reader();
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Wybierz czytelnika do edycji", components::StatusType::Warning);
            return;
        }

        controller_.begin_edit(*controller_.selected_reader());
        manager.set_active("reader_form");
        return;
    }

    if (is_command(event.raw, 'b')) {
        sync_selected_reader();
        if (controller_.selected_reader().has_value()) {
            manager.set_active("reader_block_dialog");
        } else {
            status_bar_.set("Wybierz czytelnika do blokady", components::StatusType::Warning);
        }
        return;
    }

    if (is_command(event.raw, 'u')) {
        sync_selected_reader();
        if (controller_.selected_reader().has_value()) {
            manager.set_active("reader_unblock_dialog");
        } else {
            status_bar_.set("Wybierz czytelnika do odblokowania", components::StatusType::Warning);
        }
        return;
    }

    if (is_command(event.raw, 'h')) {
        sync_selected_reader();
        if (controller_.selected_reader().has_value()) {
            manager.set_active("reader_loan_history");
        } else {
            status_bar_.set("Wybierz czytelnika do podgladu historii", components::StatusType::Warning);
        }
        return;
    }

    if (is_command(event.raw, 'r')) {
        sync_selected_reader();
        if (controller_.selected_reader().has_value()) {
            manager.set_active("reader_reputation");
        } else {
            status_bar_.set("Wybierz czytelnika do podgladu reputacji", components::StatusType::Warning);
        }
        return;
    }

    if (search_input_mode_) {
        apply_search_input(event.raw);
        search_input_mode_ = false;
        search_box_.set_focus(false);
        reload_results();
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

void ReaderListScreen::reload_results() {
    try {
        readers_ = controller_.find_readers(search_state_);
        refresh_rows();
        sync_selected_reader();
        status_bar_.set("Wczytano czytelnikow", components::StatusType::Success);
    } catch (const std::exception& e) {
        readers_.clear();
        results_view_.set_items({});
        controller_.clear_selected_reader();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderListScreen::sync_selected_reader() {
    const auto index = selected_result_index();
    if (!index.has_value()) {
        controller_.clear_selected_reader();
        return;
    }
    controller_.set_selected_reader(readers_[*index].public_id);
}

void ReaderListScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(readers_.size());

    for (const auto& reader : readers_) {
        const std::string blocked = reader.is_blocked ? " [BLOCKED]" : "";
        rows.push_back(reader.card_number + " | " + reader.first_name + " " + reader.last_name + " | " + reader.email +
                       " | " + readers::to_string(reader.account_status) + blocked);
    }

    results_view_.set_items(std::move(rows));
}

void ReaderListScreen::apply_search_input(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized == "clear") {
        search_state_.first_name.clear();
        search_state_.last_name.clear();
        search_state_.card_number.clear();
        search_state_.email.clear();
        search_box_.set_query("");
        return;
    }

    auto apply_prefix = [&](const std::string& prefix, std::string& target) {
        if (normalized.rfind(prefix, 0) == 0) {
            target = normalized.substr(prefix.size());
            search_box_.set_query(normalized);
            return true;
        }
        return false;
    };

    if (apply_prefix("f:", search_state_.first_name) || apply_prefix("l:", search_state_.last_name) ||
        apply_prefix("c:", search_state_.card_number) || apply_prefix("e:", search_state_.email)) {
        return;
    }

    search_state_.last_name = normalized;
    search_box_.set_query("l:" + normalized);
}

std::optional<std::size_t> ReaderListScreen::selected_result_index() const {
    if (readers_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = results_view_.selected_index();
    if (index >= readers_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
