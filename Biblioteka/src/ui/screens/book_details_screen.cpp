#include "ui/screens/book_details_screen.hpp"

#include <cctype>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

std::string optional_int_to_text(const std::optional<int>& value) {
    if (!value.has_value()) {
        return "-";
    }
    return std::to_string(*value);
}

} // namespace

namespace ui::screens {

BookDetailsScreen::BookDetailsScreen(controllers::BooksController& controller)
    : controller_(controller),
      header_("Ksiazki", "Szczegoly ksiazki"),
      status_bar_("", components::StatusType::Info),
      footer_({"e: edytuj", "q/esc/b: powrot"}) {}

std::string BookDetailsScreen::id() const {
    return "book_details";
}

std::string BookDetailsScreen::title() const {
    return "Szczegoly ksiazki";
}

void BookDetailsScreen::on_show() {
    current_book_.reset();

    try {
        if (!controller_.selected_book().has_value()) {
            status_bar_.set("Brak wybranej ksiazki", components::StatusType::Warning);
            return;
        }

        current_book_ = controller_.get_book_details(*controller_.selected_book(), true);
        status_bar_.set("Wczytano szczegoly", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void BookDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_book_.has_value()) {
        renderer.draw_line("Brak danych ksiazki");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const books::Book& book = *current_book_;
    renderer.draw_line("ID: " + book.public_id);
    renderer.draw_line("Tytul: " + book.title);
    renderer.draw_line("Autor: " + book.author);
    renderer.draw_line("ISBN: " + book.isbn);
    renderer.draw_line("Wydawnictwo: " + book.publisher);
    renderer.draw_line("Rok wydania: " + optional_int_to_text(book.publication_year));
    renderer.draw_line("Opis: " + book.description);
    renderer.draw_line("Status: " + std::string(book.is_archived ? "ARCHIVED" : "ACTIVE"));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void BookDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("books");
        return;
    }

    if (is_command(event.raw, 'e')) {
        if (!current_book_.has_value()) {
            status_bar_.set("Brak ksiazki do edycji", components::StatusType::Warning);
            return;
        }

        controller_.begin_edit(current_book_->public_id);
        manager.set_active("book_form");
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

} // namespace ui::screens
