#include "ui/screens/location_details_screen.hpp"

#include <algorithm>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

std::string optional_text(const std::optional<std::string>& value) {
    return value.has_value() ? *value : "-";
}

} // namespace

namespace ui::screens {

LocationDetailsScreen::LocationDetailsScreen(controllers::LocationsController& controller)
    : controller_(controller),
      header_("Lokalizacje", "Szczegoly lokalizacji"),
      footer_({"q: powrot"}) {}

std::string LocationDetailsScreen::id() const {
    return "location_details";
}

std::string LocationDetailsScreen::title() const {
    return "Szczegoly lokalizacji";
}

void LocationDetailsScreen::on_show() {
    location_.reset();
    copies_.clear();

    try {
        if (!controller_.selected_location().has_value()) {
            status_bar_.set("Brak wybranej lokalizacji", components::StatusType::Warning);
            return;
        }

        location_ = controller_.get_location_details(*controller_.selected_location());
        copies_ = controller_.list_location_copies(*controller_.selected_location());
        status_bar_.set("Wczytano szczegoly lokalizacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void LocationDetailsScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!location_.has_value()) {
        renderer.draw_line("Brak danych lokalizacji.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Public ID: " + location_->public_id);
    renderer.draw_line("Nazwa: " + location_->name);
    renderer.draw_line("Typ: " + locations::to_string(location_->type));
    renderer.draw_line("Kod: " + location_->code);
    renderer.draw_line("Opis: " + location_->description);
    renderer.draw_separator();

    renderer.draw_line("Egzemplarze przypisane do lokalizacji: " + std::to_string(copies_.size()));
    const std::size_t shown = std::min<std::size_t>(copies_.size(), 20);
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& copy = copies_[i];
        renderer.draw_line("- " + copy.inventory_number + " [" + copy.public_id + "] status=" +
                           copies::to_string(copy.status) + " target=" + optional_text(copy.target_location_id));
    }
    if (copies_.size() > shown) {
        renderer.draw_line("..." + std::to_string(copies_.size() - shown) + " wiecej");
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LocationDetailsScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("locations");
        return;
    }
}

} // namespace ui::screens
