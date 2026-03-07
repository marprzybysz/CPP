#include "ui/screens/copy_status_screen.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool starts_with_case_insensitive(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }

    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (static_cast<char>(std::tolower(text[i])) != static_cast<char>(std::tolower(prefix[i]))) {
            return false;
        }
    }

    return true;
}

std::optional<std::string> normalize_optional(std::string value) {
    while (!value.empty() && value.front() == ' ') {
        value.erase(value.begin());
    }
    while (!value.empty() && value.back() == ' ') {
        value.pop_back();
    }
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

std::string optional_text(const std::optional<std::string>& value) {
    return value.has_value() ? *value : "-";
}

} // namespace

namespace ui::screens {

CopyStatusScreen::CopyStatusScreen(controllers::CopiesController& controller)
    : controller_(controller),
      header_("Egzemplarze", "Zmiana statusu"),
      footer_({"gora/dol: wybor statusu", "n:<tekst>: note", "r:<tekst>: archival reason", "enter: zapisz", "q: powrot"}),
      statuses_({copies::CopyStatus::OnShelf,
                 copies::CopyStatus::Loaned,
                 copies::CopyStatus::Reserved,
                 copies::CopyStatus::InRepair,
                 copies::CopyStatus::Archived,
                 copies::CopyStatus::Lost}) {
    std::vector<std::string> rows;
    rows.reserve(statuses_.size());
    for (const auto status : statuses_) {
        rows.push_back(copies::to_string(status));
    }
    status_view_.set_items(std::move(rows));
}

std::string CopyStatusScreen::id() const {
    return "copy_status";
}

std::string CopyStatusScreen::title() const {
    return "Zmiana statusu egzemplarza";
}

void CopyStatusScreen::on_show() {
    note_.clear();
    archival_reason_.clear();
    current_copy_.reset();

    try {
        if (!controller_.selected_copy().has_value()) {
            status_bar_.set("Brak wybranego egzemplarza", components::StatusType::Warning);
            return;
        }

        current_copy_ = controller_.get_copy_details(*controller_.selected_copy());

        const auto it = std::find(statuses_.begin(), statuses_.end(), current_copy_->copy.status);
        if (it != statuses_.end()) {
            status_view_.set_selected_index(static_cast<std::size_t>(std::distance(statuses_.begin(), it)));
        }

        status_bar_.set("Wybierz nowy status", components::StatusType::Info);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void CopyStatusScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_copy_.has_value()) {
        renderer.draw_line("Brak danych egzemplarza.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Egzemplarz: " + current_copy_->copy.public_id + " | " + current_copy_->copy.inventory_number);
    renderer.draw_line("Aktualny status: " + copies::to_string(current_copy_->copy.status));
    renderer.draw_separator();

    status_view_.render(renderer);

    renderer.draw_separator();
    renderer.draw_line("Note: " + (note_.empty() ? std::string{"-"} : note_));
    renderer.draw_line("Archival reason: " + (archival_reason_.empty() ? std::string{"-"} : archival_reason_));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void CopyStatusScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        status_view_.move_up();
        return;
    }

    if (event.key == Key::Down) {
        status_view_.move_down();
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("copy_details");
        return;
    }

    if (event.key == Key::Enter) {
        try {
            if (!current_copy_.has_value()) {
                status_bar_.set("Brak danych egzemplarza", components::StatusType::Warning);
                return;
            }

            const auto status = selected_status();
            if (!status.has_value()) {
                status_bar_.set("Brak wybranego statusu", components::StatusType::Warning);
                return;
            }

            controller_.change_status(current_copy_->copy.public_id,
                                      *status,
                                      note_,
                                      normalize_optional(archival_reason_));
            status_bar_.set("Zmieniono status", components::StatusType::Success);
            manager.set_active("copy_details");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (starts_with_case_insensitive(event.raw, "n:")) {
        note_ = event.raw.substr(2);
        status_bar_.set("Zaktualizowano note", components::StatusType::Info);
        return;
    }

    if (starts_with_case_insensitive(event.raw, "r:")) {
        archival_reason_ = event.raw.substr(2);
        status_bar_.set("Zaktualizowano archival reason", components::StatusType::Info);
        return;
    }

    status_bar_.set("Nieznana komenda", components::StatusType::Warning);
}

std::optional<copies::CopyStatus> CopyStatusScreen::selected_status() const {
    const std::size_t index = status_view_.selected_index();
    if (index >= statuses_.size()) {
        return std::nullopt;
    }

    return statuses_[index];
}

} // namespace ui::screens
