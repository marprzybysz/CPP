#include "ui/components/form_field.hpp"

#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

FormField::FormField(std::string label, std::string value, std::string placeholder, bool required)
    : label_(std::move(label)), value_(std::move(value)), placeholder_(std::move(placeholder)), required_(required) {}

void FormField::set_value(std::string value) {
    value_ = std::move(value);
}

void FormField::set_focus(bool focused) {
    focused_ = focused;
}

const std::string& FormField::value() const {
    return value_;
}

void FormField::render(Renderer& renderer) const {
    std::string label = label_;
    if (required_) {
        label += " *";
    }

    const std::string presented_value = value_.empty() ? ("<" + placeholder_ + ">") : value_;
    const std::string prefix = focused_ ? "> " : "  ";
    renderer.draw_line(prefix + label + ": " + presented_value,
                       focused_ ? ui::style::TextStyle::Highlight : ui::style::TextStyle::Normal);
}

} // namespace ui::components
