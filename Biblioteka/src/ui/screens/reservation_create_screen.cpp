#include "ui/screens/reservation_create_screen.hpp"

#include <ctime>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

constexpr std::size_t kFieldReader = 0;
constexpr std::size_t kFieldTargetType = 1;
constexpr std::size_t kFieldTargetToken = 2;
constexpr std::size_t kFieldExpirationDate = 3;

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

ReservationCreateScreen::ReservationCreateScreen(controllers::ReservationsController& controller)
    : controller_(controller),
      header_("Rezerwacje", "Nowa rezerwacja"),
      footer_({"gora/dol: pole", "wpisz tekst: ustaw wartosc", "enter: zapisz", "q: powrot"}) {}

std::string ReservationCreateScreen::id() const {
    return "reservation_create";
}

std::string ReservationCreateScreen::title() const {
    return "Nowa rezerwacja";
}

void ReservationCreateScreen::on_show() {
    values_ = {"", "copy", "", current_date_iso()};
    focused_field_ = 0;
    rebuild_fields();
    status_bar_.set("Wprowadz dane nowej rezerwacji", components::StatusType::Info);
}

void ReservationCreateScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Reader: public_id czytelnika lub card number.");
    renderer.draw_line("Target type: copy albo book.");
    renderer.draw_line("Target token: public_id egzemplarza/ksiazki lub inventory/ISBN.");
    renderer.draw_line("Data wygasniecia: YYYY-MM-DD.");
    renderer.draw_separator();

    for (const auto& field : fields_) {
        field.render(renderer);
    }

    renderer.draw_separator();
    renderer.draw_line("Panel akcji: enter=zapisz | q=powrot");
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReservationCreateScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
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
        manager.set_active("reservations");
        return;
    }

    if (!event.raw.empty() && focused_field_ < values_.size()) {
        values_[focused_field_] = event.raw;
        rebuild_fields();
        status_bar_.set("Zaktualizowano pole", components::StatusType::Info);
    }
}

void ReservationCreateScreen::rebuild_fields() {
    fields_.clear();
    fields_.reserve(values_.size());

    fields_.emplace_back("Reader", values_[kFieldReader], "READER-... lub CARD-...", true);
    fields_.emplace_back("Target type", values_[kFieldTargetType], "copy/book", true);
    fields_.emplace_back("Target token", values_[kFieldTargetToken], "COPY-/BOOK- lub inventory/ISBN", true);
    fields_.emplace_back("Data wygasniecia", values_[kFieldExpirationDate], "YYYY-MM-DD", true);

    for (std::size_t i = 0; i < fields_.size(); ++i) {
        fields_[i].set_focus(i == focused_field_);
    }
}

bool ReservationCreateScreen::save(ScreenManager& manager) {
    try {
        const auto created =
            controller_.create_reservation(values_[kFieldReader], values_[kFieldTargetType], values_[kFieldTargetToken], values_[kFieldExpirationDate]);

        controller_.set_selected_reservation(created.public_id);
        status_bar_.set("Utworzono rezerwacje: " + created.public_id, components::StatusType::Success);
        manager.set_active("reservation_details");
        return true;
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        return false;
    }
}

} // namespace ui::screens
