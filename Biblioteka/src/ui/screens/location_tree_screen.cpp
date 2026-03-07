#include "ui/screens/location_tree_screen.hpp"

#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

LocationTreeScreen::LocationTreeScreen(controllers::LocationsController& controller)
    : controller_(controller),
      header_("Lokalizacje", "Drzewo lokalizacji"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "q: powrot"}) {}

std::string LocationTreeScreen::id() const {
    return "locations";
}

std::string LocationTreeScreen::title() const {
    return "Lokalizacje";
}

void LocationTreeScreen::on_show() {
    reload_tree();
}

void LocationTreeScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Liczba lokalizacji: " + std::to_string(rows_.size()));
    renderer.draw_separator();
    tree_view_.render(renderer);

    renderer.draw_separator();
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void LocationTreeScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        tree_view_.move_up();
        sync_selected_location();
        return;
    }

    if (event.key == Key::Down) {
        tree_view_.move_down();
        sync_selected_location();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_location();
        if (controller_.selected_location().has_value()) {
            manager.set_active("location_details");
        } else {
            status_bar_.set("Brak wybranej lokalizacji", components::StatusType::Warning);
        }
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }
}

void LocationTreeScreen::reload_tree() {
    try {
        rows_ = controller_.list_location_tree();
        refresh_rows();
        sync_selected_location();
        status_bar_.set("Wczytano drzewo lokalizacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        rows_.clear();
        tree_view_.set_items({});
        controller_.clear_selected_location();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void LocationTreeScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(this->rows_.size());

    for (const auto& item : this->rows_) {
        const std::string indent(item.depth * 2, ' ');
        rows.push_back(indent + "- " + item.location.name + " [" + item.location.public_id + "] (" +
                       locations::to_string(item.location.type) + ")");
    }

    tree_view_.set_items(std::move(rows));
}

void LocationTreeScreen::sync_selected_location() {
    const auto index = selected_index();
    if (!index.has_value()) {
        controller_.clear_selected_location();
        return;
    }

    controller_.set_selected_location(rows_[*index].location.public_id);
}

std::optional<std::size_t> LocationTreeScreen::selected_index() const {
    if (rows_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = tree_view_.selected_index();
    if (index >= rows_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
