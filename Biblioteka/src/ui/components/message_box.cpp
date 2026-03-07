#include "ui/components/message_box.hpp"

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

MessageBox::MessageBox(std::string title, std::string message, StatusType type)
    : title_(std::move(title)), message_(std::move(message)), type_(type) {}

void MessageBox::set_message(std::string title, std::string message, StatusType type) {
    title_ = std::move(title);
    message_ = std::move(message);
    type_ = type;
}

void MessageBox::render(Renderer& renderer) const {
    renderer.draw_separator('*');
    renderer.draw_line(title_, to_style(type_));
    renderer.draw_line(message_);
    renderer.draw_separator('*');
}

} // namespace ui::components
