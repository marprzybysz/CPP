#include "ui/screens/loan_extend_dialog_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

LoanExtendDialogScreen::LoanExtendDialogScreen(controllers::LoansController& controller)
    : controller_(controller),
      header_("Wypozyczenia", "Przedluzenie terminu"),
      footer_({"wpisz date YYYY-MM-DD", "enter: zapisz", "q: anuluj"}) {}

std::string LoanExtendDialogScreen::id() const {
    return "loan_extend_dialog";
}

std::string LoanExtendDialogScreen::title() const {
    return "Przedluzenie wypozyczenia";
}

void LoanExtendDialogScreen::on_show() {
    loan_public_id_.clear();
    current_due_date_.clear();
    new_due_date_.clear();

    try {
        if (!controller_.selected_loan().has_value()) {
            status_bar_.set("Brak wybranego wypozyczenia", components::StatusType::Error);
            return;
        }

        loan_public_id_ = *controller_.selected_loan();
        const auto details = controller_.get_loan_details(loan_public_id_);
        current_due_date_ = details.row.reservation.expiration_date;
        new_due_date_ = current_due_date_;
        status_bar_.set("Podaj nowy termin zwrotu", components::StatusType::Info);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void LoanExtendDialogScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (loan_public_id_.empty()) {
        renderer.draw_line("Nie wybrano wypozyczenia.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Wypozyczenie: " + loan_public_id_);
    renderer.draw_line("Aktualny termin: " + current_due_date_);
    renderer.draw_line("Nowy termin: " + (new_due_date_.empty() ? std::string{"-"} : new_due_date_));

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LoanExtendDialogScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("loans");
        return;
    }

    if (event.key == Key::Enter) {
        try {
            if (loan_public_id_.empty()) {
                status_bar_.set("Brak wybranego wypozyczenia", components::StatusType::Error);
                return;
            }

            controller_.extend_loan(loan_public_id_, new_due_date_);
            status_bar_.set("Przedluzono termin", components::StatusType::Success);
            manager.set_active("loan_details");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (!event.raw.empty()) {
        new_due_date_ = event.raw;
        status_bar_.set("Zmieniono nowy termin", components::StatusType::Info);
    }
}

} // namespace ui::screens
