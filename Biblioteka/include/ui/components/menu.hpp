#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ui::components {

struct MenuItem {
    std::string id;
    std::string label;
};

class Menu {
public:
    explicit Menu(std::vector<MenuItem> items = {});

    void move_up();
    void move_down();
    [[nodiscard]] const MenuItem& selected() const;
    [[nodiscard]] const std::vector<MenuItem>& items() const;
    [[nodiscard]] std::size_t selected_index() const;

private:
    std::vector<MenuItem> items_;
    std::size_t selected_index_{0};
};

} // namespace ui::components
