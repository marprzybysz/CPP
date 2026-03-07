#include "ui/screens/copy_location_screen.hpp"

#include <algorithm>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

constexpr std::size_t kFieldCurrentLocation = 0;
constexpr std::size_t kFieldTargetLocation = 1;
constexpr std::size_t kFieldNote = 2;

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

CopyLocationScreen::CopyLocationScreen(controllers::CopiesController& controller)
    : controller_(controller),
      header_("Egzemplarze", "Zmiana lokalizacji"),
      footer_({"gora/dol: pole", "wpisz tekst: ustaw wartosc", "enter: zapisz", "q: powrot"}) {}

std::string CopyLocationScreen::id() const {
    return "copy_location";
}

std::string CopyLocationScreen::title() const {
    return "Zmiana lokalizacji egzemplarza";
}

void CopyLocationScreen::on_show() {
    values_ = {"", "", ""};
    focused_field_ = 0;
    current_copy_.reset();

    try {
        if (!controller_.selected_copy().has_value()) {
            status_bar_.set("Brak wybranego egzemplarza", components::StatusType::Warning);
            rebuild_fields();
            return;
        }

        current_copy_ = controller_.get_copy_details(*controller_.selected_copy());
        values_[kFieldCurrentLocation] = current_copy_->copy.current_location_id.value_or("");
        values_[kFieldTargetLocation] = current_copy_->copy.target_location_id.value_or("");
        values_[kFieldNote] = "";
        available_locations_ = controller_.list_locations();
        status_bar_.set("Wprowadz nowe lokalizacje", components::StatusType::Info);
    } catch (const std::exception& e) {
        available_locations_.clear();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }

    rebuild_fields();
}

void CopyLocationScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (current_copy_.has_value()) {
        renderer.draw_line("Egzemplarz: " + current_copy_->copy.public_id + " | " + current_copy_->copy.inventory_number);
        renderer.draw_line("Aktualna nazwa lokalizacji: " + current_copy_->current_location_name.value_or("-"));
        renderer.draw_line("Docelowa nazwa lokalizacji: " + current_copy_->target_location_name.value_or("-"));
    }
    renderer.draw_separator();

    for (const auto& field : fields_) {
        field.render(renderer);
    }

    renderer.draw_separator();
    renderer.draw_line("Dostepne lokalizacje (fragment):");
    const std::size_t shown = std::min<std::size_t>(available_locations_.size(), 8);
    for (std::size_t i = 0; i < shown; ++i) {
        renderer.draw_line("- " + available_locations_[i].public_id + " | " + available_locations_[i].name);
    }
    if (available_locations_.size() > shown) {
        renderer.draw_line("..." + std::to_string(available_locations_.size() - shown) + " wiecej");
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void CopyLocationScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
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
        apply(manager);
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("copy_details");
        return;
    }

    if (!event.raw.empty()) {
        set_field_value(focused_field_, event.raw);
        rebuild_fields();
        status_bar_.set("Zaktualizowano pole", components::StatusType::Info);
    }
}

void CopyLocationScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(3);

    fields_.emplace_back("Aktualna lokalizacja (public_id)", values_[kFieldCurrentLocation], "LOC-... lub puste", false);
    fields_.emplace_back("Docelowa lokalizacja (public_id)", values_[kFieldTargetLocation], "LOC-... lub puste", false);
    fields_.emplace_back("Note", values_[kFieldNote], "opcjonalnie", false);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

void CopyLocationScreen::set_field_value(std::size_t index, const std::string& value) {
    if (index >= values_.size()) {
        return;
    }

    values_[index] = value;
}

bool CopyLocationScreen::apply(ScreenManager& manager) {
    try {
        if (!current_copy_.has_value()) {
            status_bar_.set("Brak danych egzemplarza", components::StatusType::Warning);
            return false;
        }

        controller_.change_location(current_copy_->copy.public_id,
                                    normalize_optional(values_[kFieldCurrentLocation]),
                                    normalize_optional(values_[kFieldTargetLocation]),
                                    trim_copy(values_[kFieldNote]));

        status_bar_.set("Zmieniono lokalizacje", components::StatusType::Success);
        manager.set_active("copy_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

std::optional<std::string> CopyLocationScreen::normalize_optional(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized.empty() || normalized == "-") {
        return std::nullopt;
    }
    return normalized;
}

} // namespace ui::screens
