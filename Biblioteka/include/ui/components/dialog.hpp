#pragma once

#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::components {

class Dialog {
public:
    Dialog(std::string title = "", std::vector<std::string> lines = {});

    void set_title(std::string title);
    void set_lines(std::vector<std::string> lines);
    void render(Renderer& renderer) const;

private:
    std::string title_;
    std::vector<std::string> lines_;
};

} // namespace ui::components
