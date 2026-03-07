#pragma once

#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::components {

class Footer {
public:
    explicit Footer(std::vector<std::string> hints = {});

    void set_hints(std::vector<std::string> hints);
    void render(Renderer& renderer) const;

private:
    std::vector<std::string> hints_;
};

} // namespace ui::components
