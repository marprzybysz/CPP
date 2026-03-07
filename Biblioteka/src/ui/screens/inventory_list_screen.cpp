#include "ui/screens/inventory_list_screen.hpp"

#include <optional>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace ui::screens {

InventoryListScreen::InventoryListScreen(controllers::InventoryController& controller)
    : controller_(controller),
      header_("Inwentaryzacja", "Sesje i operacje"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: otworz sesje", "a: nowa inwentaryzacja", "q: powrot"}) {}

std::string InventoryListScreen::id() const {
    return "inventory";
}

std::string InventoryListScreen::title() const {
    return "Inwentaryzacja";
}

void InventoryListScreen::on_show() {
    reload_sessions();
}

void InventoryListScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Sesje inwentaryzacji: " + std::to_string(sessions_.size()));
    renderer.draw_separator();
    sessions_view_.render(renderer);

    renderer.draw_separator();
    status_bar_.render(renderer);
    footer_.render(renderer);
}

void InventoryListScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (event.key == Key::Up) {
        sessions_view_.move_up();
        sync_selected_session();
        return;
    }

    if (event.key == Key::Down) {
        sessions_view_.move_down();
        sync_selected_session();
        return;
    }

    if (event.key == Key::Enter) {
        sync_selected_session();
        if (!controller_.selected_session().has_value()) {
            status_bar_.set("Brak wybranej sesji", components::StatusType::Warning);
            return;
        }

        const auto index = selected_index();
        if (!index.has_value()) {
            status_bar_.set("Brak wybranej sesji", components::StatusType::Warning);
            return;
        }

        if (sessions_[*index].status == inventory::InventoryStatus::Completed) {
            manager.set_active("inventory_result");
        } else {
            manager.set_active("inventory_session");
        }
        return;
    }

    if (!event.raw.empty() && event.raw.size() == 1 && (event.raw[0] == 'a' || event.raw[0] == 'A')) {
        controller_.clear_selected_session();
        manager.set_active("inventory_session");
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }
}

void InventoryListScreen::reload_sessions() {
    try {
        sessions_ = controller_.list_sessions();
        refresh_rows();
        sync_selected_session();
        status_bar_.set("Wczytano sesje inwentaryzacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        sessions_.clear();
        sessions_view_.set_items({});
        controller_.clear_selected_session();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void InventoryListScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(sessions_.size());

    for (const auto& session : sessions_) {
        rows.push_back(session.public_id + " | location=" + session.location_public_id + " | status=" +
                       inventory::to_string(session.status) + " | started=" + session.started_at);
    }

    sessions_view_.set_items(std::move(rows));
}

void InventoryListScreen::sync_selected_session() {
    const auto index = selected_index();
    if (!index.has_value()) {
        controller_.clear_selected_session();
        return;
    }

    controller_.set_selected_session(sessions_[*index].public_id);
}

std::optional<std::size_t> InventoryListScreen::selected_index() const {
    if (sessions_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = sessions_view_.selected_index();
    if (index >= sessions_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
