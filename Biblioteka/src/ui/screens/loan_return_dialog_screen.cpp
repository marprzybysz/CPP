#include "ui/screens/loan_return_dialog_screen.hpp"

#include <cctype>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool is_command(const std::string& raw, char cmd) {
    return raw.size() == 1 && static_cast<char>(std::tolower(raw.front())) == cmd;
}

} // namespace

namespace ui::screens {

LoanReturnDialogScreen::LoanReturnDialogScreen(controllers::LoansController& controller)
    : controller_(controller),
      header_("Wypozyczenia", "Zwrot egzemplarza"),
      footer_({"y: potwierdz zwrot", "n: anuluj", "q: powrot"}) {}

std::string LoanReturnDialogScreen::id() const {
    return "loan_return_dialog";
}

std::string LoanReturnDialogScreen::title() const {
    return "Zwrot egzemplarza";
}

void LoanReturnDialogScreen::on_show() {
    loan_public_id_.clear();
    if (controller_.selected_loan().has_value()) {
        loan_public_id_ = *controller_.selected_loan();
        status_bar_.set("Potwierdz zwrot wypozyczenia", components::StatusType::Warning);
        return;
    }

    status_bar_.set("Brak wybranego wypozyczenia", components::StatusType::Error);
}

void LoanReturnDialogScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (loan_public_id_.empty()) {
        renderer.draw_line("Nie wybrano wypozyczenia.");
    } else {
        renderer.draw_line("Zwrocic wypozyczenie: " + loan_public_id_ + " ?");
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LoanReturnDialogScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape || is_command(event.raw, 'n')) {
        manager.set_active("loans");
        return;
    }

    if (is_command(event.raw, 'y')) {
        try {
            if (loan_public_id_.empty()) {
                status_bar_.set("Brak wybranego wypozyczenia", components::StatusType::Error);
                return;
            }

            controller_.return_loan(loan_public_id_);
            status_bar_.set("Oznaczono zwrot", components::StatusType::Success);
            manager.set_active("loans");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    status_bar_.set("Uzyj y lub n", components::StatusType::Info);
}

} // namespace ui::screens
