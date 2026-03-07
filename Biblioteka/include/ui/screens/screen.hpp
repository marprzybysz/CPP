#pragma once

#include <string>

#include "ui/input_handler.hpp"

namespace ui {

class Renderer;
class ScreenManager;

class Screen {
public:
    virtual ~Screen() = default;

    [[nodiscard]] virtual std::string id() const = 0;
    [[nodiscard]] virtual std::string title() const = 0;
    [[nodiscard]] virtual bool prefers_line_input() const { return false; }
    [[nodiscard]] virtual bool prefers_text_input() const { return false; }
    virtual void on_show() {}
    virtual void render(Renderer& renderer) const = 0;
    virtual void handle_input(const InputEvent& event, ScreenManager& manager) = 0;
};

} // namespace ui
