#include "ui/screens/inventory_result_screen.hpp"

#include <algorithm>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

InventoryResultScreen::InventoryResultScreen(controllers::InventoryController& controller)
    : controller_(controller),
      header_("Inwentaryzacja", "Wynik inwentaryzacji"),
      footer_({"q: powrot"}) {}

std::string InventoryResultScreen::id() const {
    return "inventory_result";
}

std::string InventoryResultScreen::title() const {
    return "Wynik inwentaryzacji";
}

void InventoryResultScreen::on_show() {
    result_.reset();

    try {
        if (!controller_.selected_session().has_value()) {
            status_bar_.set("Brak wybranej sesji", components::StatusType::Warning);
            return;
        }

        result_ = controller_.get_result(*controller_.selected_session());
        status_bar_.set("Wczytano wynik inwentaryzacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void InventoryResultScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!result_.has_value()) {
        renderer.draw_line("Brak danych wyniku.");
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    const auto& r = *result_;
    renderer.draw_line("Session: " + r.session.public_id);
    renderer.draw_line("Location: " + r.session.location_public_id);
    renderer.draw_line("Status: " + inventory::to_string(r.session.status));
    renderer.draw_line("Podsumowanie: " + r.session.summary_result);
    renderer.draw_line("ON_SHELF: " + std::to_string(r.on_shelf.size()));
    renderer.draw_line("JUSTIFIED: " + std::to_string(r.justified.size()));
    renderer.draw_line("MISSING: " + std::to_string(r.missing.size()));

    renderer.draw_separator();
    renderer.draw_line("Braki (max 15):");
    const std::size_t shown = std::min<std::size_t>(r.missing.size(), 15);
    for (std::size_t i = 0; i < shown; ++i) {
        renderer.draw_line("- " + r.missing[i].inventory_number + " [" + r.missing[i].copy_public_id + "]");
    }
    if (r.missing.size() > shown) {
        renderer.draw_line("..." + std::to_string(r.missing.size() - shown) + " wiecej");
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void InventoryResultScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("inventory");
        return;
    }
}

} // namespace ui::screens
