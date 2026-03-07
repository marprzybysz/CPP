#pragma once

#include <string>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/loans_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class LoanReturnDialogScreen : public Screen {
public:
    explicit LoanReturnDialogScreen(controllers::LoansController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::LoansController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::string loan_public_id_;
};

} // namespace ui::screens
