#pragma once

#include <string>

namespace ui {
class Renderer;
}

namespace ui::components {

enum class StatusType {
    Info,
    Success,
    Warning,
    Error,
};

class StatusBar {
public:
    StatusBar(std::string text = "", StatusType type = StatusType::Info);

    void set(std::string text, StatusType type = StatusType::Info);
    void clear();
    void render(Renderer& renderer) const;

private:
    std::string text_;
    StatusType type_{StatusType::Info};
};

} // namespace ui::components
