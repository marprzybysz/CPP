#pragma once

#include <string>

namespace ui::style {

enum class TextStyle {
    Normal,
    Header,
    Highlight,
    Error,
    Success,
    Warning,
};

[[nodiscard]] std::string apply(const std::string& text, TextStyle style_type);

} // namespace ui::style
