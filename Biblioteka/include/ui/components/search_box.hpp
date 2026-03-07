#pragma once

#include <string>

namespace ui {
class Renderer;
}

namespace ui::components {

class SearchBox {
public:
    explicit SearchBox(std::string query = "", std::string placeholder = "Szukaj...");

    void set_query(std::string query);
    void set_focus(bool focused);
    [[nodiscard]] const std::string& query() const;
    void render(Renderer& renderer) const;

private:
    std::string query_;
    std::string placeholder_;
    bool focused_{false};
};

} // namespace ui::components
