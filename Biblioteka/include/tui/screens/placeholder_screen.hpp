#pragma once

#include <string>

#include "tui/screens/screen.hpp"

namespace tui {

class PlaceholderScreen : public Screen {
public:
    PlaceholderScreen(std::string title, std::string message);

    [[nodiscard]] std::string title() const override;
    void show() override;

private:
    std::string title_;
    std::string message_;
};

} // namespace tui
