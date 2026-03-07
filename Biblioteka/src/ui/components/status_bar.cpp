#include "ui/components/status_bar.hpp"

#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

namespace {

ui::style::TextStyle to_style(StatusType type) {
    switch (type) {
    case StatusType::Success:
        return ui::style::TextStyle::Success;
    case StatusType::Warning:
        return ui::style::TextStyle::Warning;
    case StatusType::Error:
        return ui::style::TextStyle::Error;
    case StatusType::Info:
    default:
        return ui::style::TextStyle::Normal;
    }
}

} // namespace

StatusBar::StatusBar(std::string text, StatusType type) : text_(std::move(text)), type_(type) {}

void StatusBar::set(std::string text, StatusType type) {
    text_ = std::move(text);
    type_ = type;
}

void StatusBar::clear() {
    text_.clear();
    type_ = StatusType::Info;
}

void StatusBar::render(Renderer& renderer) const {
    if (text_.empty()) {
        return;
    }

    renderer.draw_line("Status: " + text_, to_style(type_));
}

} // namespace ui::components
