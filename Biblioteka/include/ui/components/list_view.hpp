#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::components {

class ListView {
public:
    explicit ListView(std::vector<std::string> items = {});

    void set_items(std::vector<std::string> items);
    void move_up();
    void move_down();
    [[nodiscard]] std::size_t selected_index() const;
    [[nodiscard]] const std::string* selected_item() const;
    void render(Renderer& renderer) const;

private:
    std::vector<std::string> items_;
    std::size_t selected_index_{0};
};

} // namespace ui::components
