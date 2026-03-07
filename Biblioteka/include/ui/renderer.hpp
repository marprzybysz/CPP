#pragma once

#include <string>

#include "ui/style.hpp"

namespace ui {

class Renderer {
public:
    void clear() const;
    void draw_header(const std::string& title) const;
    void draw_line(const std::string& text, style::TextStyle style_type = style::TextStyle::Normal) const;
    void draw_separator(char symbol = '-', int length = 40) const;
};

} // namespace ui
