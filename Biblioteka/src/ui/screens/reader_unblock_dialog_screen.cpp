#include "ui/screens/reader_unblock_dialog_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

ReaderUnblockDialogScreen::ReaderUnblockDialogScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Odblokowanie konta"),
      footer_({"enter: potwierdz odblokowanie", "q: anuluj"}) {}

std::string ReaderUnblockDialogScreen::id() const {
    return "reader_unblock_dialog";
}

std::string ReaderUnblockDialogScreen::title() const {
    return "Odblokowanie konta czytelnika";
}

void ReaderUnblockDialogScreen::on_show() {
    current_reader_.reset();

    try {
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Brak wybranego czytelnika", components::StatusType::Warning);
            return;
        }

        current_reader_ = controller_.get_reader_details(*controller_.selected_reader());
        status_bar_.set("Nacisnij Enter aby odblokowac", components::StatusType::Info);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderUnblockDialogScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (current_reader_.has_value()) {
        renderer.draw_line("Czytelnik: " + current_reader_->card_number + " | " + current_reader_->first_name + " " +
                           current_reader_->last_name);
        renderer.draw_line("Aktualna blokada: " + std::string(current_reader_->is_blocked ? "TAK" : "NIE"));
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderUnblockDialogScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Enter) {
        try {
            if (!current_reader_.has_value()) {
                status_bar_.set("Brak danych czytelnika", components::StatusType::Warning);
                return;
            }

            controller_.unblock_reader(current_reader_->public_id);
            status_bar_.set("Konto odblokowane", components::StatusType::Success);
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
}

} // namespace ui::screens
