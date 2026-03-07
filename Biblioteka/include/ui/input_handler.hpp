#pragma once

#include <string>

namespace ui {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Backspace,
    Submit,
    Escape,
    Back,
    Quit,
    Unknown,
};

struct InputEvent {
    Key key{Key::Unknown};
    std::string raw;
};

class InputHandler {
public:
    [[nodiscard]] InputEvent read_event(bool prefer_line_input, bool prefer_text_input) const;
};

} // namespace ui
