#include "ui/screens/reader_block_dialog_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

ReaderBlockDialogScreen::ReaderBlockDialogScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Blokada konta"),
      footer_({"wpisz powod blokady", "enter: potwierdz", "q: anuluj"}) {}

std::string ReaderBlockDialogScreen::id() const {
    return "reader_block_dialog";
}

std::string ReaderBlockDialogScreen::title() const {
    return "Blokada konta czytelnika";
}

void ReaderBlockDialogScreen::on_show() {
    reason_.clear();
    current_reader_.reset();

    try {
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Brak wybranego czytelnika", components::StatusType::Warning);
            return;
        }

        current_reader_ = controller_.get_reader_details(*controller_.selected_reader());
        status_bar_.set("Wpisz powod blokady i nacisnij Enter", components::StatusType::Info);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderBlockDialogScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (current_reader_.has_value()) {
        renderer.draw_line("Czytelnik: " + current_reader_->card_number + " | " + current_reader_->first_name + " " +
                           current_reader_->last_name);
    }

    renderer.draw_line("Powod blokady: " + (reason_.empty() ? std::string{"-"} : reason_));
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderBlockDialogScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Enter) {
        try {
            if (!current_reader_.has_value()) {
                status_bar_.set("Brak danych czytelnika", components::StatusType::Warning);
                return;
            }

            if (reason_.empty()) {
                status_bar_.set("Powod blokady jest wymagany", components::StatusType::Warning);
                return;
            }

            controller_.block_reader(current_reader_->public_id, reason_);
            status_bar_.set("Konto zablokowane", components::StatusType::Success);
            manager.set_active("reader_details");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("reader_details");
        return;
    }

    if (!event.raw.empty()) {
        reason_ = event.raw;
        status_bar_.set("Zaktualizowano powod", components::StatusType::Info);
    }
}

} // namespace ui::screens
