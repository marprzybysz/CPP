#include "ui/screens/audit_log_screen.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

#include "errors/error_mapper.hpp"
#include "ui/renderer.hpp"
#include "ui/screen_manager.hpp"

namespace {

std::string trim_copy(std::string raw) {
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())) != 0) {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())) != 0) {
        raw.pop_back();
    }
    return raw;
}

std::string lower_copy(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return raw;
}

std::optional<audit::AuditModule> parse_module(const std::string& raw) {
    const std::string normalized = lower_copy(raw);
    if (normalized == "system") return audit::AuditModule::System;
    if (normalized == "books") return audit::AuditModule::Books;
    if (normalized == "copies") return audit::AuditModule::Copies;
    if (normalized == "readers") return audit::AuditModule::Readers;
    if (normalized == "loans") return audit::AuditModule::Loans;
    if (normalized == "inventory") return audit::AuditModule::Inventory;
    if (normalized == "supply") return audit::AuditModule::Supply;
    if (normalized == "export") return audit::AuditModule::Export;
    if (normalized == "import") return audit::AuditModule::Import;
    return std::nullopt;
}

} // namespace

namespace ui::screens {

AuditLogScreen::AuditLogScreen(controllers::AuditController& controller)
    : controller_(controller),
      header_("Logi zdarzen", "Audit log"),
      status_bar_("Gotowe", components::StatusType::Info),
      footer_({"gora/dol: nawigacja", "enter: szczegoly", "/: filtr", "q: powrot"}) {}

std::string AuditLogScreen::id() const {
    return "logs";
}

std::string AuditLogScreen::title() const {
    return "Logi zdarzen";
}

void AuditLogScreen::on_show() {
    reload_events();
}

void AuditLogScreen::render(Renderer& renderer) const {
    renderer.clear();
    header_.render(renderer);

    renderer.draw_line("Filtr: " + filter_text());
    renderer.draw_separator();
    events_view_.render(renderer);

    const auto idx = selected_index();
    if (idx.has_value()) {
        const auto& e = events_[*idx];
        renderer.draw_separator();
        renderer.draw_line("Szczegoly wpisu:");
        renderer.draw_line("module=" + audit::to_string(e.module) + " actor=" + e.actor + " time=" + e.occurred_at);
        renderer.draw_line("object=" + e.object_type + ":" + e.object_public_id + " op=" + e.operation_type);
        renderer.draw_line(e.details);
    }

    status_bar_.render(renderer);
    footer_.render(renderer);
}

void AuditLogScreen::handle_input(const InputEvent& event, ScreenManager& manager) {
    if (filter_input_mode_) {
        if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
            filter_input_mode_ = false;
            status_bar_.set("Anulowano filtr logow", components::StatusType::Info);
            return;
        }

        apply_filter_input(event.raw);
        filter_input_mode_ = false;
        reload_events();
        return;
    }

    if (event.key == Key::Up) {
        events_view_.move_up();
        return;
    }

    if (event.key == Key::Down) {
        events_view_.move_down();
        return;
    }

    if (event.key == Key::Enter) {
        return;
    }

    if (!event.raw.empty() && event.raw.size() == 1 && event.raw[0] == '/') {
        filter_input_mode_ = true;
        status_bar_.set("Filtr: op:<typ> date:YYYY-MM-DD module:<nazwa>", components::StatusType::Info);
        return;
    }

    if (event.key == Key::Quit || event.key == Key::Back || event.key == Key::Escape) {
        manager.set_active("dashboard");
        return;
    }
}

void AuditLogScreen::reload_events() {
    try {
        events_ = controller_.list_events(filter_);
        refresh_rows();
        status_bar_.set("Wczytano logi zdarzen", components::StatusType::Success);
    } catch (const std::exception& e) {
        events_.clear();
        events_view_.set_items({});
        status_bar_.set(errors::to_user_message(e), components::StatusType::Error);
    }
}

void AuditLogScreen::refresh_rows() {
    std::vector<std::string> rows;
    rows.reserve(events_.size());

    for (const auto& event : events_) {
        rows.push_back(event.occurred_at + " | " + audit::to_string(event.module) + " | " + event.operation_type + " | " +
                       event.object_type + ":" + event.object_public_id);
    }

    events_view_.set_items(std::move(rows));
}

void AuditLogScreen::apply_filter_input(const std::string& raw) {
    const std::string input = trim_copy(raw);
    if (input == "clear") {
        filter_ = {};
        status_bar_.set("Wyczyszczono filtr logow", components::StatusType::Info);
        return;
    }

    std::string token;
    std::string rest = input;
    while (!rest.empty()) {
        const auto pos = rest.find(' ');
        if (pos == std::string::npos) {
            token = rest;
            rest.clear();
        } else {
            token = rest.substr(0, pos);
            rest = trim_copy(rest.substr(pos + 1));
        }

        if (token.rfind("op:", 0) == 0) {
            filter_.operation_type = token.substr(3);
        } else if (token.rfind("date:", 0) == 0) {
            filter_.date = token.substr(5);
        } else if (token.rfind("module:", 0) == 0) {
            filter_.module = parse_module(token.substr(7));
        }
    }

    status_bar_.set("Zastosowano filtr logow", components::StatusType::Success);
}

std::optional<std::size_t> AuditLogScreen::selected_index() const {
    if (events_.empty()) {
        return std::nullopt;
    }

    const std::size_t idx = events_view_.selected_index();
    if (idx >= events_.size()) {
        return std::nullopt;
    }

    return idx;
}

std::string AuditLogScreen::filter_text() const {
    const std::string op = filter_.operation_type.value_or("-");
    const std::string date = filter_.date.value_or("-");
    const std::string module = filter_.module.has_value() ? audit::to_string(*filter_.module) : "-";
    return "op=" + op + " date=" + date + " module=" + module;
}

} // namespace ui::screens
