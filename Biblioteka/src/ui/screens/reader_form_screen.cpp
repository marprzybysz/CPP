#include "ui/screens/reader_form_screen.hpp"

#include <algorithm>
#include <cctype>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

constexpr std::size_t kFieldFirstName = 0;
constexpr std::size_t kFieldLastName = 1;
constexpr std::size_t kFieldEmail = 2;
constexpr std::size_t kFieldPhone = 3;
constexpr std::size_t kFieldGdpr = 4;
constexpr std::size_t kFieldAccountStatus = 5;

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

ReaderFormScreen::ReaderFormScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Formularz"),
      footer_({"gora/dol: pole", "wpisz tekst: ustaw wartosc", "enter: zapisz", "q: powrot"}) {}

std::string ReaderFormScreen::id() const {
    return "reader_form";
}

std::string ReaderFormScreen::title() const {
    return "Formularz czytelnika";
}

void ReaderFormScreen::on_show() {
    load_form_state();
    rebuild_fields();
}

void ReaderFormScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line(form_state_.mode == controllers::ReaderFormMode::Create ? "Tryb: DODAWANIE" : "Tryb: EDYCJA");
    renderer.draw_line("Wymagane: imie, nazwisko, email.");
    renderer.draw_line("GDPR: yes/no | Status: ACTIVE/SUSPENDED/CLOSED (dla edycji)");
    renderer.draw_separator();

    for (const auto& field : fields_) {
        field.render(renderer);
    }

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=zapisz | q=powrot");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderFormScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
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
        manager.set_active("readers");
        return;
    }

    if (!event.raw.empty()) {
        set_field_value(focused_field_, event.raw);
        rebuild_fields();
        status_bar_.set("Zaktualizowano pole", components::StatusType::Info);
    }
}

void ReaderFormScreen::load_form_state() {
    form_state_ = controller_.form_state();
    original_reader_.reset();
    values_ = {"", "", "", "", "yes", "ACTIVE"};

    if (form_state_.mode == controllers::ReaderFormMode::Edit && form_state_.target_public_id.has_value()) {
        try {
            original_reader_ = controller_.get_reader_details(*form_state_.target_public_id);
            values_[kFieldFirstName] = original_reader_->first_name;
            values_[kFieldLastName] = original_reader_->last_name;
            values_[kFieldEmail] = original_reader_->email;
            values_[kFieldPhone] = original_reader_->phone.value_or("");
            values_[kFieldGdpr] = original_reader_->gdpr_consent ? "yes" : "no";
            values_[kFieldAccountStatus] = readers::to_string(original_reader_->account_status);
            status_bar_.set("Wczytano dane czytelnika", components::StatusType::Success);
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
    } else {
        status_bar_.set("Tryb dodawania nowego czytelnika", components::StatusType::Info);
    }

    focused_field_ = 0;
}

void ReaderFormScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(6);

    fields_.emplace_back("Imie", values_[kFieldFirstName], "np. Jan", true);
    fields_.emplace_back("Nazwisko", values_[kFieldLastName], "np. Kowalski", true);
    fields_.emplace_back("Email", values_[kFieldEmail], "np. jan@example.com", true);
    fields_.emplace_back("Telefon", values_[kFieldPhone], "opcjonalny", false);
    fields_.emplace_back("GDPR consent", values_[kFieldGdpr], "yes/no", false);
    fields_.emplace_back("Status konta", values_[kFieldAccountStatus], "ACTIVE/SUSPENDED/CLOSED", false);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

void ReaderFormScreen::set_field_value(std::size_t index, const std::string& value) {
    if (index >= values_.size()) {
        return;
    }

    values_[index] = value;
}

bool ReaderFormScreen::save(ScreenManager& manager) {
    try {
        const std::string first_name = trim_copy(values_[kFieldFirstName]);
        const std::string last_name = trim_copy(values_[kFieldLastName]);
        const std::string email = trim_copy(values_[kFieldEmail]);

        if (first_name.empty() || last_name.empty() || email.empty()) {
            status_bar_.set("Imie, nazwisko i email sa wymagane", components::StatusType::Warning);
            return false;
        }

        if (form_state_.mode == controllers::ReaderFormMode::Create) {
            readers::CreateReaderInput input;
            input.first_name = first_name;
            input.last_name = last_name;
            input.email = email;
            input.phone = normalize_optional(values_[kFieldPhone]);
            input.gdpr_consent = parse_bool(values_[kFieldGdpr], true);

            const readers::Reader created = controller_.create_reader(input);
            controller_.set_selected_reader(created.public_id);
            status_bar_.set("Dodano czytelnika: " + created.public_id, components::StatusType::Success);
            manager.set_active("reader_details");
            return true;
        }

        if (!form_state_.target_public_id.has_value()) {
            status_bar_.set("Brak wskazanego czytelnika do edycji", components::StatusType::Error);
            return false;
        }

        readers::UpdateReaderInput input;
        input.first_name = first_name;
        input.last_name = last_name;
        input.email = email;
        input.phone = normalize_optional(values_[kFieldPhone]);
        input.gdpr_consent = parse_bool(values_[kFieldGdpr], true);
        input.account_status = parse_account_status(values_[kFieldAccountStatus]);

        const readers::Reader updated = controller_.update_reader(*form_state_.target_public_id, input);
        controller_.set_selected_reader(updated.public_id);
        status_bar_.set("Zapisano zmiany: " + updated.public_id, components::StatusType::Success);
        manager.set_active("reader_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

std::optional<std::string> ReaderFormScreen::normalize_optional(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized.empty() || normalized == "-") {
        return std::nullopt;
    }
    return normalized;
}

bool ReaderFormScreen::parse_bool(const std::string& raw, bool default_value) {
    std::string normalized = trim_copy(raw);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "y") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "n") {
        return false;
    }

    return default_value;
}

std::optional<readers::AccountStatus> ReaderFormScreen::parse_account_status(const std::string& raw) {
    const std::string normalized = trim_copy(raw);
    if (normalized.empty()) {
        return std::nullopt;
    }

    try {
        return readers::account_status_from_string(normalized);
    } catch (...) {
        std::string upper = normalized;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        return readers::account_status_from_string(upper);
    }
}

} // namespace ui::screens
