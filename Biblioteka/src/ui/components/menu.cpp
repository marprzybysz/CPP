#include "ui/components/menu.hpp"

#include <utility>

namespace ui::components {

Menu::Menu(std::vector<MenuItem> items) : items_(std::move(items)) {}

void Menu::move_up() {
    if (items_.empty()) {
        return;
    }

    if (selected_index_ == 0) {
        selected_index_ = items_.size() - 1;
        return;
    }

    --selected_index_;
}

void Menu::move_down() {
    if (items_.empty()) {
        return;
    }

    selected_index_ = (selected_index_ + 1) % items_.size();
}

const MenuItem& Menu::selected() const {
    return items_.at(selected_index_);
}

const std::vector<MenuItem>& Menu::items() const {
    return items_;
}

std::size_t Menu::selected_index() const {
    return selected_index_;
}

} // namespace ui::components
