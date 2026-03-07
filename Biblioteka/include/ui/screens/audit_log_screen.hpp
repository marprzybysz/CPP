#pragma once

#include <optional>
#include <string>
#include <vector>

#include "audit/audit_event.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/audit_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class AuditLogScreen : public Screen {
public:
    explicit AuditLogScreen(controllers::AuditController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_line_input() const override { return filter_input_mode_; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_events();
    void refresh_rows();
    void apply_filter_input(const std::string& raw);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string filter_text() const;

    controllers::AuditController& controller_;
    components::Header header_;
    components::ListView events_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::AuditFilterState filter_;
    std::vector<audit::AuditEvent> events_;
    bool filter_input_mode_{false};
};

} // namespace ui::screens
