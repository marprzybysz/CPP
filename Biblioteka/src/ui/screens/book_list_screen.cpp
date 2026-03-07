#include "ui/screens/book_list_screen.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include "errors/error_mapper.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

std::string trimmed_prefix(std::string raw) {
    while (!raw.empty() && raw.front() == ' ') {
        raw.erase(raw.begin());
    }
    return raw;
}

} // namespace

namespace ui::screens {

BookListScreen::BookListScreen(controllers::BooksController& controller)
    : controller_(controller),
      header_("Ksiazki", "Lista, wyszukiwanie i akcje"),
      search_box_("", "uzyj t:/a:/i: np. t:Wiedzmin"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "a: dodaj", "e: edytuj", "/: szukaj", "f: filtr", "q/esc: powrot"}) {}

std::string BookListScreen::id() const {
    return "books";
}

std::string BookListScreen::title() const {
    return "Ksiazki";
}

void BookListScreen::on_show() {
    search_input_mode_ = false;
    reload_results();
}

void BookListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Wyszukiwanie: tytul='" + search_state_.title + "' autor='" + search_state_.author + "' isbn='" + search_state_.isbn + "'");
    renderer.draw_line("Filtr: include_archived=" + std::string(search_state_.include_archived ? "TAK" : "NIE"));
    search_box_.render(renderer);
    renderer.draw_separator();

    renderer.draw_line("Wyniki: " + std::to_string(results_.size()));
    results_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=szczegoly | a=dodaj | e=edytuj | /=szukaj | f=filtr");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void BookListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        results_view_.move_up();
        sync_selected_book();
        return;
    }

    if (event.key == Key::Down) {
        results_view_.move_down();
        sync_selected_book();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_book();
        if (controller_.selected_book().has_value()) {
            manager.set_active("book_details");
        } else {
            status_bar_.set("Brak zaznaczonej ksiazki", components::StatusType::Warning);
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
        status_bar_.set("Tryb wyszukiwania: t:/a:/i: lub clear", components::StatusType::Info);
        return;
    }

    if (is_command(event.raw, 'f')) {
        search_state_.include_archived = !search_state_.include_archived;
        reload_results();
        status_bar_.set("Zmieniono filtr include_archived", components::StatusType::Success);
        return;
    }

    if (is_command(event.raw, 'a')) {
        controller_.begin_create();
        manager.set_active("book_form");
        return;
    }

    if (is_command(event.raw, 'e')) {
        sync_selected_book();
        if (!controller_.selected_book().has_value()) {
            status_bar_.set("Wybierz ksiazke do edycji", components::StatusType::Warning);
            return;
        }

        controller_.begin_edit(*controller_.selected_book());
        manager.set_active("book_form");
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

void BookListScreen::reload_results() {
    try {
        results_ = controller_.find_books(search_state_);
        refresh_list_rows();
        sync_selected_book();
        status_bar_.set("Wczytano ksiazki", components::StatusType::Success);
    } catch (const std::exception& e) {
        results_.clear();
        results_view_.set_items({});
        controller_.clear_selected_book();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void BookListScreen::sync_selected_book() {
    const auto index = selected_result_index();
    if (!index.has_value()) {
        controller_.clear_selected_book();
        return;
    }
    controller_.set_selected_book(results_[*index].public_id);
}

void BookListScreen::apply_search_input(const std::string& raw) {
    const std::string normalized = trimmed_prefix(raw);
    if (normalized == "clear") {
        search_state_.title.clear();
        search_state_.author.clear();
        search_state_.isbn.clear();
        search_box_.set_query("");
        status_bar_.set("Wyczyszczono kryteria wyszukiwania", components::StatusType::Success);
        return;
    }

    auto apply_with_prefix = [&](const std::string& prefix, std::string& target) {
        if (normalized.rfind(prefix, 0) == 0) {
            target = normalized.substr(prefix.size());
            search_box_.set_query(normalized);
            return true;
        }
        return false;
    };

    if (apply_with_prefix("t:", search_state_.title) || apply_with_prefix("a:", search_state_.author) ||
        apply_with_prefix("i:", search_state_.isbn)) {
        status_bar_.set("Zaktualizowano filtr wyszukiwania", components::StatusType::Success);
        return;
    }

    search_state_.title = normalized;
    search_box_.set_query("t:" + normalized);
    status_bar_.set("Domyslnie ustawiono wyszukiwanie po tytule", components::StatusType::Info);
}

void BookListScreen::refresh_list_rows() {
    std::vector<std::string> rows;
    rows.reserve(results_.size());
    for (const auto& book : results_) {
        const std::string archived = book.is_archived ? " [ARCH]" : "";
        rows.push_back(book.public_id + " | " + book.title + " | " + book.author + " | " + book.isbn + archived);
    }
    results_view_.set_items(std::move(rows));
}

std::optional<std::size_t> BookListScreen::selected_result_index() const {
    if (results_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = results_view_.selected_index();
    if (index >= results_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
