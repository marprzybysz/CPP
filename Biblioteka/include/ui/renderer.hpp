#pragma once

#include <string>

namespace ui {

class Renderer {
public:
    void clear() const;
    void draw_header(const std::string& title) const;
    void draw_line(const std::string& text) const;
};

} // namespace ui
