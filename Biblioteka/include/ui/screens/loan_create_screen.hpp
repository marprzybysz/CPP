#pragma once

#include <array>
#include <string>
#include <vector>

#include "ui/components/footer.hpp"
#include "ui/components/form_field.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/loans_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class LoanCreateScreen : public Screen {
public:
    explicit LoanCreateScreen(controllers::LoansController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_text_input() const override { return true; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void rebuild_fields();
    bool save(ScreenManager& manager);

    controllers::LoansController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::vector<components::FormField> fields_;
    std::array<std::string, 3> values_{};
    std::size_t focused_field_{0};
};

} // namespace ui::screens
