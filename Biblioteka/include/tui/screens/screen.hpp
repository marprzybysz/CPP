#pragma once

#include <string>

namespace tui {

class Screen {
public:
    virtual ~Screen() = default;

    [[nodiscard]] virtual std::string title() const = 0;
    virtual void show() = 0;
};

} // namespace tui
