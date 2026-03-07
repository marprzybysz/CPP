#include "ui/components/list_view.hpp"

#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

ListView::ListView(std::vector<std::string> items) : items_(std::move(items)) {}

void ListView::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    if (items_.empty()) {
        selected_index_ = 0;
        return;
    }

    if (selected_index_ >= items_.size()) {
        selected_index_ = items_.size() - 1;
    }
}

void ListView::move_up() {
    if (items_.empty()) {
        return;
    }

    if (selected_index_ == 0) {
        selected_index_ = items_.size() - 1;
        return;
    }

    --selected_index_;
}

void ListView::move_down() {
    if (items_.empty()) {
        return;
    }

    selected_index_ = (selected_index_ + 1) % items_.size();
}

std::size_t ListView::selected_index() const {
    return selected_index_;
}

const std::string* ListView::selected_item() const {
    if (items_.empty()) {
        return nullptr;
    }

    return &items_[selected_index_];
}

void ListView::render(Renderer& renderer) const {
    if (items_.empty()) {
        renderer.draw_line("(pusta lista)");
        return;
    }

    for (std::size_t i = 0; i < items_.size(); ++i) {
        const bool selected = i == selected_index_;
        const std::string prefix = selected ? "> " : "  ";
        renderer.draw_line(prefix + items_[i], selected ? ui::style::TextStyle::Highlight : ui::style::TextStyle::Normal);
    }
}

} // namespace ui::components
