#include "ui/input_handler.hpp"

#include <cctype>
#include <iostream>

namespace {

ui::Key map_input_to_key(const std::string& raw) {
    if (raw.empty()) {
        return ui::Key::Enter;
    }

    const char first = static_cast<char>(std::tolower(raw.front()));
    switch (first) {
    case 'w':
        return ui::Key::Up;
    case 's':
        return ui::Key::Down;
    case 'b':
        return ui::Key::Back;
    case 'q':
        return ui::Key::Quit;
    case 'e':
        return ui::Key::Enter;
    default:
        return ui::Key::Unknown;
    }
}

} // namespace

namespace ui {

InputEvent InputHandler::read_event() const {
    std::cout << "[w/s=move, e=enter, b=back, q=quit] > ";

    std::string raw;
    if (!std::getline(std::cin, raw)) {
        return {.key = Key::Quit, .raw = "q"};
    }

    return {.key = map_input_to_key(raw), .raw = raw};
}

} // namespace ui
