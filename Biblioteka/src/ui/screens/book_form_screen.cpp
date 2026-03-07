#include "ui/screens/book_form_screen.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <utility>

#include "errors/error_mapper.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

constexpr std::size_t kFieldTitle = 0;
constexpr std::size_t kFieldAuthor = 1;
constexpr std::size_t kFieldIsbn = 2;
constexpr std::size_t kFieldPublisher = 3;
constexpr std::size_t kFieldYear = 4;
constexpr std::size_t kFieldDescription = 5;

} // namespace

namespace ui::screens {

BookFormScreen::BookFormScreen(controllers::BooksController& controller)
    : controller_(controller),
      header_("Ksiazki", "Formularz"),
      footer_({"gora/dol: pole", "wpisz tekst: ustaw wartosc", "enter: zapisz", "q/esc: anuluj"}) {}

std::string BookFormScreen::id() const {
    return "book_form";
}

std::string BookFormScreen::title() const {
    return "Formularz ksiazki";
}

void BookFormScreen::on_show() {
    load_form_state();
    rebuild_fields();
}

void BookFormScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line(form_state_.mode == controllers::BookFormMode::Create ? "Tryb: DODAWANIE" : "Tryb: EDYCJA");
    renderer.draw_line("Wskazowka: aby ustawic wartosc pola, wpisz tekst i zatwierdz enterem.");
    renderer.draw_separator();

    for (const auto& field : fields_) {
        field.render(renderer);
    }

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=zapisz | q/esc=powrot");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void BookFormScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        if (focused_field_ == 0) {
            focused_field_ = fields_.empty() ? 0 : fields_.size() - 1;
        } else {
            --focused_field_;
        }
        rebuild_fields();
        return;
    }

    if (event.key == Key::Down) {
        if (!fields_.empty()) {
            focused_field_ = (focused_field_ + 1) % fields_.size();
        }
        rebuild_fields();
        return;
    }

    if (event.key == Key::Enter) {
        save(manager);
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("books");
        return;
    }

    if (event.raw.empty()) {
        return;
    }

    if (is_command(event.raw, 'q')) {
        manager.set_active("books");
        return;
    }

    set_field_value(focused_field_, event.raw);
    rebuild_fields();
    status_bar_.set("Zaktualizowano pole", components::StatusType::Info);
}

void BookFormScreen::load_form_state() {
    form_state_ = controller_.form_state();
    original_book_.reset();

    values_ = {"", "", "", "", "", ""};

    if (form_state_.mode == controllers::BookFormMode::Edit && form_state_.target_public_id.has_value()) {
        try {
            original_book_ = controller_.get_book_details(*form_state_.target_public_id, true);
            values_[kFieldTitle] = original_book_->title;
            values_[kFieldAuthor] = original_book_->author;
            values_[kFieldIsbn] = original_book_->isbn;
            values_[kFieldPublisher] = original_book_->publisher;
            values_[kFieldYear] = original_book_->publication_year.has_value() ? std::to_string(*original_book_->publication_year) : "";
            values_[kFieldDescription] = original_book_->description;
            status_bar_.set("Wczytano dane ksiazki", components::StatusType::Success);
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
    } else {
        status_bar_.set("Tryb dodawania nowej ksiazki", components::StatusType::Info);
    }

    focused_field_ = 0;
}

void BookFormScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(6);

    fields_.emplace_back("Tytul", values_[kFieldTitle], "np. Pragmatic Programmer", true);
    fields_.emplace_back("Autor", values_[kFieldAuthor], "np. Hunt", true);
    fields_.emplace_back("ISBN", values_[kFieldIsbn], "13 cyfr", true);
    fields_.emplace_back("Wydawnictwo", values_[kFieldPublisher], "opcjonalne", false);
    fields_.emplace_back("Rok wydania", values_[kFieldYear], "opcjonalny", false);
    fields_.emplace_back("Opis", values_[kFieldDescription], "opcjonalny", false);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

void BookFormScreen::set_field_value(std::size_t index, const std::string& value) {
    if (index >= values_.size()) {
        return;
    }

    values_[index] = value;
}

bool BookFormScreen::save(ScreenManager& manager) {
    try {
        if (values_[kFieldTitle].empty() || values_[kFieldAuthor].empty() || values_[kFieldIsbn].empty()) {
            status_bar_.set("Tytul, autor i ISBN sa wymagane", components::StatusType::Warning);
            return false;
        }

        if (form_state_.mode == controllers::BookFormMode::Create) {
            books::CreateBookInput input;
            input.title = values_[kFieldTitle];
            input.author = values_[kFieldAuthor];
            input.isbn = values_[kFieldIsbn];
            input.publisher = values_[kFieldPublisher];
            input.publication_year = parse_year(values_[kFieldYear]);
            input.description = values_[kFieldDescription];

            const books::Book created = controller_.create_book(input);
            controller_.set_selected_book(created.public_id);
            status_bar_.set("Dodano ksiazke: " + created.public_id, components::StatusType::Success);
            manager.set_active("book_details");
            return true;
        }

        if (!form_state_.target_public_id.has_value()) {
            status_bar_.set("Brak wskazanej ksiazki do edycji", components::StatusType::Error);
            return false;
        }

        books::UpdateBookInput input;
        input.title = values_[kFieldTitle];
        input.author = values_[kFieldAuthor];
        input.isbn = values_[kFieldIsbn];
        input.publisher = values_[kFieldPublisher];
        input.publication_year = parse_year(values_[kFieldYear]);
        input.description = values_[kFieldDescription];

        const books::Book updated = controller_.update_book(*form_state_.target_public_id, input);
        controller_.set_selected_book(updated.public_id);
        status_bar_.set("Zapisano zmiany: " + updated.public_id, components::StatusType::Success);
        manager.set_active("book_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

std::optional<int> BookFormScreen::parse_year(const std::string& raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    try {
        return std::stoi(raw);
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace ui::screens
