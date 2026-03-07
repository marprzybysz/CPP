#include "tui/screens/placeholder_screen.hpp"

#include <iostream>
#include <utility>

namespace tui {

PlaceholderScreen::PlaceholderScreen(std::string title, std::string message)
    : title_(std::move(title)), message_(std::move(message)) {}

std::string PlaceholderScreen::title() const {
    return title_;
}

void PlaceholderScreen::show() {
    std::cout << "\n=== " << title_ << " ===\n";
    std::cout << message_ << "\n";
}

} // namespace tui
