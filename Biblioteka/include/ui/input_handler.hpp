#pragma once

#include <string>

namespace ui {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
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
    [[nodiscard]] InputEvent read_event() const;
};

} // namespace ui
