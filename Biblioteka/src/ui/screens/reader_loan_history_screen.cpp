#include "ui/screens/reader_loan_history_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

ReaderLoanHistoryScreen::ReaderLoanHistoryScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Historia wypozyczen"),
      footer_({"q: powrot"}) {}

std::string ReaderLoanHistoryScreen::id() const {
    return "reader_loan_history";
}

std::string ReaderLoanHistoryScreen::title() const {
    return "Historia wypozyczen czytelnika";
}

void ReaderLoanHistoryScreen::on_show() {
    current_reader_.reset();
    history_.clear();

    try {
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Brak wybranego czytelnika", components::StatusType::Warning);
            return;
        }

        current_reader_ = controller_.get_reader_details(*controller_.selected_reader());
        history_ = controller_.get_loan_history(current_reader_->public_id);
        status_bar_.set("Wczytano historie wypozyczen", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderLoanHistoryScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!current_reader_.has_value()) {
        renderer.draw_line("Brak danych czytelnika.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Czytelnik: " + current_reader_->card_number + " | " + current_reader_->first_name + " " +
                       current_reader_->last_name);
    renderer.draw_separator();
    renderer.draw_line("Historia wypozyczen:");

    if (history_.empty()) {
        renderer.draw_line("(brak wpisow)");
    } else {
        for (const auto& entry : history_) {
            renderer.draw_line("- " + entry.action_at + " | " + entry.loan_public_id + " | " + entry.action + " | " + entry.note);
        }
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderLoanHistoryScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("reader_details");
    }
}

} // namespace ui::screens
