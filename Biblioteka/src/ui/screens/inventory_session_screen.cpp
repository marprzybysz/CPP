#include "ui/screens/inventory_session_screen.hpp"

#include <cctype>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

bool starts_with_case_insensitive(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }

    for (std::size_t i = 0; i < prefix.size(); ++i) {
        if (static_cast<char>(std::tolower(text[i])) != static_cast<char>(std::tolower(prefix[i]))) {
            return false;
        }
    }

    return true;
}

} // namespace

namespace ui::screens {

InventorySessionScreen::InventorySessionScreen(controllers::InventoryController& controller)
    : controller_(controller),
      header_("Inwentaryzacja", "Sesja inwentaryzacji"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"tryb start: gora/dol + enter", "s:<kod>: skanuj", "f: zakoncz", "q: powrot"}) {}

std::string InventorySessionScreen::id() const {
    return "inventory_session";
}

std::string InventorySessionScreen::title() const {
    return "Sesja inwentaryzacji";
}

void InventorySessionScreen::on_show() {
    session_.reset();
    last_scan_code_.clear();

    if (controller_.selected_session().has_value()) {
        load_session();
    } else {
        load_locations();
    }
}

void InventorySessionScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    if (!session_.has_value()) {
        renderer.draw_line("Tryb startu: wybierz lokalizacje i nacisnij Enter.");
        renderer.draw_line("Operator: " + started_by_);
        renderer.draw_separator();
        locations_view_.render(renderer);
        status_bar_.render(renderer);
        footer_.render(renderer);
        return;
    }

    renderer.draw_line("Session: " + session_->public_id);
    renderer.draw_line("Location: " + session_->location_public_id);
    renderer.draw_line("Status: " + inventory::to_string(session_->status));
    renderer.draw_line("Started by: " + session_->started_by);
    renderer.draw_line("Started at: " + session_->started_at);
    renderer.draw_line("Last scan: " + (last_scan_code_.empty() ? std::string{"-"} : last_scan_code_));
    renderer.draw_separator();
    renderer.draw_line("Uzyj s:<scan_code> aby dodac zeskanowany egzemplarz.");

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void InventorySessionScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (!session_.has_value()) {
        if (event.key == Key::Up) {
            locations_view_.move_up();
            return;
        }

        if (event.key == Key::Down) {
            locations_view_.move_down();
            return;
        }

        if (event.key == Key::Enter) {
            try {
                const auto idx = selected_location_index();
                if (!idx.has_value()) {
                    status_bar_.set("Brak wybranej lokalizacji", components::StatusType::Warning);
                    return;
                }

                const auto& location = locations_[*idx].location;
                const auto created = controller_.start_session(location.public_id, started_by_);
                controller_.set_selected_session(created.public_id);
                load_session();
                status_bar_.set("Rozpoczeto inwentaryzacje", components::StatusType::Success);
            } catch (const std::exception& e) {
                status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
            }
            return;
        }

        if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
            manager.set_active("inventory");
            return;
        }

        if (starts_with_case_insensitive(event.raw, "u:")) {
            started_by_ = event.raw.substr(2);
            status_bar_.set("Zmieniono operatora", components::StatusType::Info);
            return;
        }

        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("inventory");
        return;
    }

    if (event.raw.size() == 1 && (event.raw[0] == 'f' || event.raw[0] == 'F')) {
        try {
            const auto result = controller_.finish_session(session_->public_id);
            controller_.set_selected_session(result.session.public_id);
            status_bar_.set("Zakonczono inwentaryzacje", components::StatusType::Success);
            manager.set_active("inventory_result");
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (starts_with_case_insensitive(event.raw, "s:")) {
        try {
            const std::string scan_code = event.raw.substr(2);
            controller_.add_scan(session_->public_id, scan_code);
            last_scan_code_ = scan_code;
            status_bar_.set("Dodano skan", components::StatusType::Success);
        } catch (const std::exception& e) {
            status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
        }
        return;
    }

    if (!event.raw.empty() && (event.raw == "s" || event.raw == "S")) {
        status_bar_.set("Wpisz s:<scan_code>", components::StatusType::Info);
    }
}

void InventorySessionScreen::load_locations() {
    try {
        locations_ = controller_.list_locations();
        std::vector<std::string> rows;
        rows.reserve(locations_.size());

        for (const auto& item : locations_) {
            const std::string indent(item.depth * 2, ' ');
            rows.push_back(indent + "- " + item.location.name + " [" + item.location.public_id + "] (" +
                           locations::to_string(item.location.type) + ")");
        }

        locations_view_.set_items(std::move(rows));
        status_bar_.set("Wybierz lokalizacje dla nowej sesji", components::StatusType::Info);
    } catch (const std::exception& e) {
        locations_.clear();
        locations_view_.set_items({});
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void InventorySessionScreen::load_session() {
    try {
        if (!controller_.selected_session().has_value()) {
            status_bar_.set("Brak wybranej sesji", components::StatusType::Warning);
            return;
        }

        session_ = controller_.get_session(*controller_.selected_session());
        status_bar_.set("Wczytano sesje inwentaryzacji", components::StatusType::Success);
    } catch (const std::exception& e) {
        session_.reset();
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void InventorySessionScreen::sync_selected_location() {
    const auto idx = selected_location_index();
    if (!idx.has_value()) {
        return;
    }
}

std::optional<std::size_t> InventorySessionScreen::selected_location_index() const {
    if (locations_.empty()) {
        return std::nullopt;
    }

    const std::size_t index = locations_view_.selected_index();
    if (index >= locations_.size()) {
        return std::nullopt;
    }

    return index;
}

} // namespace ui::screens
