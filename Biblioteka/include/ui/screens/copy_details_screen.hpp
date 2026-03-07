#pragma once

#include <optional>
#include <string>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/copies_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class CopyDetailsScreen : public Screen {
public:
    explicit CopyDetailsScreen(controllers::CopiesController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::CopiesController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::optional<controllers::CopyListItem> current_copy_;
};

} // namespace ui::screens
