#include "ui/style.hpp"

namespace ui::style {

namespace {

const char* ansi_prefix(TextStyle style_type) {
    switch (style_type) {
    case TextStyle::Header:
        return "\033[1;36m";
    case TextStyle::Highlight:
        return "\033[1;33m";
    case TextStyle::Error:
        return "\033[1;31m";
    case TextStyle::Success:
        return "\033[1;32m";
    case TextStyle::Warning:
        return "\033[1;35m";
    case TextStyle::Normal:
    default:
        return "\033[0m";
    }
}

} // namespace

std::string apply(const std::string& text, TextStyle style_type) {
    const char* reset = "\033[0m";
    return std::string{ansi_prefix(style_type)} + text + reset;
}

} // namespace ui::style
