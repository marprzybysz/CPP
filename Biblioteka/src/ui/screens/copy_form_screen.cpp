#include "ui/screens/copy_form_screen.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

constexpr std::size_t kFieldBookPublicId = 0;
constexpr std::size_t kFieldInventoryNumber = 1;
constexpr std::size_t kFieldStatus = 2;
constexpr std::size_t kFieldCurrentLocationId = 3;
constexpr std::size_t kFieldTargetLocationId = 4;
constexpr std::size_t kFieldConditionNote = 5;
constexpr std::size_t kFieldAcquisitionDate = 6;

std::string trim_copy(std::string raw) {
    while (!raw.empty() && raw.front() == ' ') {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && raw.back() == ' ') {
        raw.pop_back();
    }
    return raw;
}

std::string normalize_status_token(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        if (ch == '-' || ch == ' ') {
            return static_cast<unsigned char>('_');
        }
        return static_cast<unsigned char>(std::toupper(ch));
    });
    return raw;
}

} // namespace

namespace ui::screens {

CopyFormScreen::CopyFormScreen(controllers::CopiesController& controller)
    : controller_(controller),
      header_("Egzemplarze", "Dodaj egzemplarz"),
      footer_({"strzalki: zmiana pola", "pisz: edycja pola", "enter: nastepne (na ostatnim: zapisz)", "f2: zapisz", "esc: powrot"}) {}

std::string CopyFormScreen::id() const {
    return "copy_form";
}

std::string CopyFormScreen::title() const {
    return "Dodawanie egzemplarza";
}

void CopyFormScreen::on_show() {
    values_ = {"", "", "ON_SHELF", "", "", "", ""};
    focused_field_ = 0;

    try {
        available_locations_ = controller_.list_locations();
        status_bar_.set("Wprowadz dane nowego egzemplarza", components::StatusType::Info);
    } catch (const std::exception& e) {
        available_locations_.clear();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }

    rebuild_fields();
}

void CopyFormScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Wymagane: BOOK public_id, inventory_number, status.");
    renderer.draw_line("Dozwolone statusy: ON_SHELF, LOANED, RESERVED, IN_REPAIR, ARCHIVED, LOST");
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

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=nastepne (ostatnie: zapisz) | f2=zapisz | esc=powrot");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void CopyFormScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up || event.key == Key::Left) {
        if (focused_field_ == 0) {
            focused_field_ = fields_.empty() ? 0 : fields_.size() - 1;
        } else {
            --focused_field_;
        }
        rebuild_fields();
        return;
    }

    if (event.key == Key::Down || event.key == Key::Right) {
        if (!fields_.empty()) {
            focused_field_ = (focused_field_ + 1) % fields_.size();
        }
        rebuild_fields();
        return;
    }

    if (event.key == Key::Enter) {
        if (!fields_.empty() && focused_field_ + 1 < fields_.size()) {
            ++focused_field_;
            rebuild_fields();
        } else {
            save(manager);
        }
        return;
    }

    if (event.key == Key::Submit) {
        save(manager);
        return;
    }

    if (event.key == Key::Escape || event.key == Key::Back || event.key == Key::Quit) {
        manager.set_active("copies");
        return;
    }

    if (event.key == Key::Backspace) {
        if (focused_field_ < values_.size() && !values_[focused_field_].empty()) {
            values_[focused_field_].pop_back();
            rebuild_fields();
            status_bar_.set("Edycja pola", components::StatusType::Info);
        }
        return;
    }

    if (event.raw.size() == 1 && std::isprint(static_cast<unsigned char>(event.raw.front())) != 0) {
        values_[focused_field_] += event.raw;
        rebuild_fields();
        status_bar_.set("Edycja pola", components::StatusType::Info);
    }
}

void CopyFormScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(7);

    fields_.emplace_back("Book public_id", values_[kFieldBookPublicId], "np. BOOK-2026-000001", true);
    fields_.emplace_back("Inventory number", values_[kFieldInventoryNumber], "np. INV-000123", true);
    fields_.emplace_back("Status", values_[kFieldStatus], "ON_SHELF", true);
    fields_.emplace_back("Aktualna lokalizacja", values_[kFieldCurrentLocationId], "opcjonalnie: LOC-...", false);
    fields_.emplace_back("Docelowa lokalizacja", values_[kFieldTargetLocationId], "opcjonalnie: LOC-...", false);
    fields_.emplace_back("Condition note", values_[kFieldConditionNote], "opcjonalnie", false);
    fields_.emplace_back("Acquisition date", values_[kFieldAcquisitionDate], "YYYY-MM-DD", false);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

void CopyFormScreen::set_field_value(std::size_t index, const std::string& value) {
    if (index >= values_.size()) {
        return;
    }

    values_[index] = value;
}

bool CopyFormScreen::save(ScreenManager& manager) {
    try {
        const std::string book_public_id = trim_copy(values_[kFieldBookPublicId]);
        const std::string inventory_number = trim_copy(values_[kFieldInventoryNumber]);

        if (book_public_id.empty() || inventory_number.empty()) {
            status_bar_.set("Book public_id i inventory_number sa wymagane", components::StatusType::Warning);
            return false;
        }

        controllers::CopyCreateDraft draft;
        draft.book_public_id = book_public_id;
        draft.inventory_number = inventory_number;
        draft.status = parse_status(values_[kFieldStatus]);
        draft.current_location_id = normalize_optional(values_[kFieldCurrentLocationId]);
        draft.target_location_id = normalize_optional(values_[kFieldTargetLocationId]);
        draft.condition_note = trim_copy(values_[kFieldConditionNote]);
        draft.acquisition_date = normalize_optional(values_[kFieldAcquisitionDate]);

        const copies::BookCopy created = controller_.add_copy(draft);
        controller_.set_selected_copy(created.public_id);
        status_bar_.set("Dodano egzemplarz: " + created.public_id, components::StatusType::Success);
        manager.set_active("copy_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

copies::CopyStatus CopyFormScreen::parse_status(const std::string& raw) {
    return copies::copy_status_from_string(normalize_status_token(trim_copy(raw)));
}

std::optional<std::string> CopyFormScreen::normalize_optional(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized.empty() || normalized == "-") {
        return std::nullopt;
    }
    return normalized;
}

} // namespace ui::screens
