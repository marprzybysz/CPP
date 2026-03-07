#include "ui/screens/loan_create_screen.hpp"

#include <cctype>
#include <ctime>

#include "errors/error_mapper.hpp"
#include "reservations/reservation.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

constexpr std::size_t kFieldReader = 0;
constexpr std::size_t kFieldCopy = 1;
constexpr std::size_t kFieldExpiration = 2;

std::string current_date_iso() {
    const auto now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    char buffer[11] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return buffer;
}

} // namespace

namespace ui::screens {

LoanCreateScreen::LoanCreateScreen(controllers::LoansController& controller)
    : controller_(controller),
      header_("Wypozyczenia", "Nowe wypozyczenie"),
      footer_({"strzalki: zmiana pola", "pisz: edycja pola", "enter: nastepne (na ostatnim: zapisz)", "f2: zapisz", "esc: powrot"}) {}

std::string LoanCreateScreen::id() const {
    return "loan_create";
}

std::string LoanCreateScreen::title() const {
    return "Nowe wypozyczenie";
}

void LoanCreateScreen::on_show() {
    values_ = {"", "", current_date_iso()};
    focused_field_ = 0;
    rebuild_fields();
    status_bar_.set("Podaj dane wypozyczenia", components::StatusType::Info);
}

void LoanCreateScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Reader: public_id lub numer karty.");
    renderer.draw_line("Copy: public_id egzemplarza lub inventory_number.");
    renderer.draw_line("Termin zwrotu: data YYYY-MM-DD.");
    renderer.draw_separator();

    for (const auto& field : fields_) {
        field.render(renderer);
    }

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=nastepne (ostatnie: zapisz) | f2=zapisz | esc=powrot");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LoanCreateScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
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
        manager.set_active("loans");
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

    if (focused_field_ < values_.size() && event.raw.size() == 1 &&
        std::isprint(static_cast<unsigned char>(event.raw.front())) != 0) {
        values_[focused_field_] += event.raw;
        rebuild_fields();
        status_bar_.set("Edycja pola", components::StatusType::Info);
    }
}

void LoanCreateScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(values_.size());

    fields_.emplace_back("Reader", values_[kFieldReader], "READER-... lub CARD-...", true);
    fields_.emplace_back("Copy", values_[kFieldCopy], "COPY-... lub inventory_number", true);
    fields_.emplace_back("Termin zwrotu", values_[kFieldExpiration], "YYYY-MM-DD", true);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

bool LoanCreateScreen::save(ScreenManager& manager) {
    try {
        const auto created = controller_.create_loan(values_[kFieldReader], values_[kFieldCopy], values_[kFieldExpiration]);
        controller_.set_selected_loan(created.public_id);
        status_bar_.set("Utworzono wypozyczenie: " + created.public_id, components::StatusType::Success);
        manager.set_active("loan_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

} // namespace ui::screens
