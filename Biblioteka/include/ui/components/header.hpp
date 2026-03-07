#pragma once

#include <string>

namespace ui {
class Renderer;
}

namespace ui::components {

class Header {
public:
    Header(std::string title, std::string subtitle = "");

    void set_title(std::string title);
    void set_subtitle(std::string subtitle);
    void render(Renderer& renderer) const;

private:
    std::string title_;
    std::string subtitle_;
};

} // namespace ui::components
