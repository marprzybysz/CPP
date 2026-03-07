#include "ui/components/search_box.hpp"

#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

SearchBox::SearchBox(std::string query, std::string placeholder)
    : query_(std::move(query)), placeholder_(std::move(placeholder)) {}

void SearchBox::set_query(std::string query) {
    query_ = std::move(query);
}

void SearchBox::set_focus(bool focused) {
    focused_ = focused;
}

const std::string& SearchBox::query() const {
    return query_;
}

void SearchBox::render(Renderer& renderer) const {
    const std::string marker = focused_ ? "[Szukaj*] " : "[Szukaj] ";
    const std::string value = query_.empty() ? ("<" + placeholder_ + ">") : query_;
    renderer.draw_line(marker + value, focused_ ? ui::style::TextStyle::Highlight : ui::style::TextStyle::Normal);
}

} // namespace ui::components
