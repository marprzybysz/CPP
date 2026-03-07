#include "ui/renderer.hpp"

#include <iostream>

namespace ui {

void Renderer::clear() const {
    std::cout << "\033[2J\033[H";
}

void Renderer::draw_header(const std::string& title) const {
    std::cout << "=== " << title << " ===\n";
}

void Renderer::draw_line(const std::string& text) const {
    std::cout << text << "\n";
}

} // namespace ui
