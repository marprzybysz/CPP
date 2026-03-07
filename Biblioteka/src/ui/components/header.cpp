#include "ui/components/header.hpp"

#include <utility>

#include "ui/renderer.hpp"

namespace ui::components {

Header::Header(std::string title, std::string subtitle) : title_(std::move(title)), subtitle_(std::move(subtitle)) {}

void Header::set_title(std::string title) {
    title_ = std::move(title);
}

void Header::set_subtitle(std::string subtitle) {
    subtitle_ = std::move(subtitle);
}

void Header::render(Renderer& renderer) const {
    renderer.draw_header(title_);
    if (!subtitle_.empty()) {
        renderer.draw_line(subtitle_);
    }
    renderer.draw_separator('=');
}

} // namespace ui::components
