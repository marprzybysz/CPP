#include "ui/components/dialog.hpp"

#include <utility>

#include "ui/renderer.hpp"

namespace ui::components {

Dialog::Dialog(std::string title, std::vector<std::string> lines) : title_(std::move(title)), lines_(std::move(lines)) {}

void Dialog::set_title(std::string title) {
    title_ = std::move(title);
}

void Dialog::set_lines(std::vector<std::string> lines) {
    lines_ = std::move(lines);
}

void Dialog::render(Renderer& renderer) const {
    renderer.draw_separator('=');
    renderer.draw_header("Dialog: " + title_);
    for (const auto& line : lines_) {
        renderer.draw_line(line);
    }
    renderer.draw_separator('=');
}

} // namespace ui::components
