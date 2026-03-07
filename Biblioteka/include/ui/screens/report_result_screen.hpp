#pragma once

#include <optional>
#include <string>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/reports_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReportResultScreen : public Screen {
public:
    explicit ReportResultScreen(controllers::ReportsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::ReportsController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::optional<controllers::ReportResultData> result_;
};

} // namespace ui::screens
