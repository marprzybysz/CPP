#pragma once

#include <optional>
#include <string>
#include <vector>

#include "reports/report.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/reports_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReportMenuScreen : public Screen {
public:
    explicit ReportMenuScreen(controllers::ReportsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    [[nodiscard]] std::optional<reports::ReportType> selected_report_type() const;
    [[nodiscard]] std::string date_filter_text() const;
    void apply_filter_input(const std::string& raw);

    controllers::ReportsController& controller_;
    components::Header header_;
    components::ListView reports_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    std::vector<reports::ReportType> report_types_;
    controllers::ReportFilterState filter_;
    bool filter_input_mode_{false};
};

} // namespace ui::screens
