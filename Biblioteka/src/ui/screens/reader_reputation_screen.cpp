#include "ui/screens/reader_reputation_screen.hpp"

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

ReaderReputationScreen::ReaderReputationScreen(controllers::ReadersController& controller)
    : controller_(controller),
      header_("Czytelnicy", "Reputacja"),
      footer_({"q: powrot"}) {}

std::string ReaderReputationScreen::id() const {
    return "reader_reputation";
}

std::string ReaderReputationScreen::title() const {
    return "Reputacja czytelnika";
}

void ReaderReputationScreen::on_show() {
    current_reader_.reset();
    points_ = 0;
    history_.clear();

    try {
        if (!controller_.selected_reader().has_value()) {
            status_bar_.set("Brak wybranego czytelnika", components::StatusType::Warning);
            return;
        }

        current_reader_ = controller_.get_reader_details(*controller_.selected_reader());
        points_ = controller_.get_reputation_points(current_reader_->public_id);
        history_ = controller_.get_reputation_history(current_reader_->public_id, 40);
        status_bar_.set("Wczytano reputacje", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void ReaderReputationScreen::render(Renderer& renderer) const {
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
    renderer.draw_line("Punkty reputacji: " + std::to_string(points_));
    renderer.draw_separator();
    renderer.draw_line("Historia reputacji:");

    if (history_.empty()) {
        renderer.draw_line("(brak wpisow)");
    } else {
        for (const auto& entry : history_) {
            renderer.draw_line("- " + entry.created_at + " | " + std::to_string(entry.change_value) + " | " + entry.reason);
        }
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void ReaderReputationScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("reader_details");
    }
}

} // namespace ui::screens
