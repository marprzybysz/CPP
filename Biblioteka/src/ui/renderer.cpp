#include "ui/renderer.hpp"

#include <iostream>
#include <string>

namespace ui {

void Renderer::clear() const {
    std::cout << "\033[2J\033[H";
}

void Renderer::draw_header(const std::string& title) const {
    std::cout << style::apply("=== " + title + " ===", style::TextStyle::Header) << "\n";
}

void Renderer::draw_line(const std::string& text, style::TextStyle style_type) const {
    std::cout << style::apply(text, style_type) << "\n";
}

void Renderer::draw_separator(char symbol, int length) const {
    if (length <= 0) {
        return;
    }

    std::cout << std::string(static_cast<std::size_t>(length), symbol) << "\n";
}

} // namespace ui
