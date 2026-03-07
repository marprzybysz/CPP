#pragma once

#include <string>

#include "ui/screens/screen.hpp"

namespace ui::screens {

class PlaceholderScreen : public Screen {
public:
    PlaceholderScreen(std::string screen_id, std::string screen_title);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    std::string id_;
    std::string title_;
};

} // namespace ui::screens
