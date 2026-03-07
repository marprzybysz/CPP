#include "ui/screens/notes_screen.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

std::string trim_copy(std::string raw) {
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())) != 0) {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())) != 0) {
        raw.pop_back();
    }
    return raw;
}

std::string lower_copy(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return raw;
}

} // namespace

namespace ui::screens {

NotesScreen::NotesScreen(controllers::NotesController& controller)
    : controller_(controller),
      header_("Notatki", "Lista i dodawanie"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "a: dodaj", "/: filtr", "q: powrot"}) {}

std::string NotesScreen::id() const {
    return "notes";
}

std::string NotesScreen::title() const {
    return "Notatki";
}

void NotesScreen::on_show() {
    filter_input_mode_ = false;
    add_input_mode_ = false;
    reload_notes();
}

void NotesScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Filtr: " + filter_text());
    renderer.draw_separator();

    notes_view_.render(renderer);

    const auto idx = selected_index();
    if (idx.has_value()) {
        renderer.draw_separator();
        renderer.draw_line("Szczegoly:");
        renderer.draw_line("ID: " + notes_[*idx].public_id + " | author=" + notes_[*idx].author + " | created=" + notes_[*idx].created_at);
        renderer.draw_line(notes_[*idx].content);
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void NotesScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (filter_input_mode_ || add_input_mode_) {
        if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
            if (filter_input_mode_) {
                filter_input_mode_ = false;
                status_bar_.set("Anulowano filtr notatek", components::StatusType::Info);
            } else {
                add_input_mode_ = false;
                status_bar_.set("Anulowano dodawanie notatki", components::StatusType::Info);
            }
            return;
        }

        if (filter_input_mode_) {
            apply_filter_input(event.raw);
            filter_input_mode_ = false;
            reload_notes();
            return;
        }

        if (add_note(event.raw)) {
            reload_notes();
        }
        add_input_mode_ = false;
        return;
    }

    if (event.key == Key::Up) {
        notes_view_.move_up();
        return;
    }

    if (event.key == Key::Down) {
        notes_view_.move_down();
        return;
    }

    if (event.key == Key::Enter) {
        return;
    }

    if (!event.raw.empty() && event.raw.size() == 1 && event.raw[0] == '/') {
        filter_input_mode_ = true;
        status_bar_.set("Filtr: reader:/book:/copy:/loan:ID", components::StatusType::Info);
        return;
    }

    if (!event.raw.empty() && (event.raw[0] == 'a' || event.raw[0] == 'A')) {
        add_input_mode_ = true;
        status_bar_.set("Dodaj: target:<reader|book|copy|loan>:ID author:NAME text:TRESC", components::StatusType::Info);
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }
}

void NotesScreen::reload_notes() {
    try {
        notes_ = controller_.list_notes(filter_);
        refresh_rows();
        status_bar_.set("Wczytano notatki", components::StatusType::Success);
    } catch (const std::exception& e) {
        notes_.clear();
        notes_view_.set_items({});
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void NotesScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(notes_.size());

    for (const auto& note : notes_) {
        rows.push_back(note.public_id + " | " + note.target_id + " | " + note.author + " | " + note.created_at);
    }

    notes_view_.set_items(std::move(rows));
}

void NotesScreen::apply_filter_input(const std::string& raw) {
    const std::string input = trim_copy(raw);
    if (input == "clear") {
        filter_.target_type = notes::NoteTargetType::Reader;
        filter_.target_id.clear();
        status_bar_.set("Wyczyszczono filtr notatek", components::StatusType::Info);
        return;
    }

    auto set_type = [&](const std::string& prefix, notes::NoteTargetType type) {
        if (input.rfind(prefix, 0) == 0) {
            filter_.target_type = type;
            filter_.target_id = trim_copy(input.substr(prefix.size()));
            return true;
        }
        return false;
    };

    if (set_type("reader:", notes::NoteTargetType::Reader) || set_type("book:", notes::NoteTargetType::Book) ||
        set_type("copy:", notes::NoteTargetType::Copy) || set_type("loan:", notes::NoteTargetType::Loan)) {
        status_bar_.set("Zastosowano filtr notatek", components::StatusType::Success);
        return;
    }

    status_bar_.set("Niepoprawny filtr. Uzyj reader:/book:/copy:/loan:", components::StatusType::Warning);
}

bool NotesScreen::add_note(const std::string& raw) {
    try {
        std::string input = trim_copy(raw);
        if (input.rfind("a ", 0) == 0) {
            input = trim_copy(input.substr(2));
        }
        const auto target_pos = input.find("target:");
        const auto author_pos = input.find(" author:");
        const auto text_pos = input.find(" text:");

        if (target_pos == std::string::npos || author_pos == std::string::npos || text_pos == std::string::npos ||
            !(target_pos < author_pos && author_pos < text_pos)) {
            status_bar_.set("Format: a target:<type>:<id> author:<name> text:<tresc>", components::StatusType::Warning);
            return false;
        }

        const std::string target_value = trim_copy(input.substr(target_pos + 7, author_pos - (target_pos + 7)));
        const std::string author = trim_copy(input.substr(author_pos + 8, text_pos - (author_pos + 8)));
        const std::string text = trim_copy(input.substr(text_pos + 6));

        const auto sep = target_value.find(':');
        if (sep == std::string::npos) {
            status_bar_.set("target musi miec format type:id", components::StatusType::Warning);
            return false;
        }

        const std::string target_type_raw = lower_copy(trim_copy(target_value.substr(0, sep)));
        const std::string target_id = trim_copy(target_value.substr(sep + 1));

        notes::CreateNoteInput input_note;
        if (target_type_raw == "reader") {
            input_note.target_type = notes::NoteTargetType::Reader;
        } else if (target_type_raw == "book") {
            input_note.target_type = notes::NoteTargetType::Book;
        } else if (target_type_raw == "copy") {
            input_note.target_type = notes::NoteTargetType::Copy;
        } else if (target_type_raw == "loan") {
            input_note.target_type = notes::NoteTargetType::Loan;
        } else {
            status_bar_.set("Nieznany target type", components::StatusType::Warning);
            return false;
        }

        input_note.target_id = target_id;
        input_note.author = author;
        input_note.content = text;

        controller_.add_note(input_note);
        status_bar_.set("Dodano notatke", components::StatusType::Success);
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

std::optional<std::size_t> NotesScreen::selected_index() const {
    if (notes_.empty()) {
        return std::nullopt;
    }

    const std::size_t idx = notes_view_.selected_index();
    if (idx >= notes_.size()) {
        return std::nullopt;
    }

    return idx;
}

std::string NotesScreen::filter_text() const {
    return notes::to_string(filter_.target_type) + ":" + (filter_.target_id.empty() ? std::string{"-"} : filter_.target_id);
}

} // namespace ui::screens
