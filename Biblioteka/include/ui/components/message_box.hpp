#pragma once

#include <string>

#include "ui/components/status_bar.hpp"

namespace ui {
class Renderer;
}

namespace ui::components {

class MessageBox {
public:
    MessageBox(std::string title = "Informacja", std::string message = "", StatusType type = StatusType::Info);

    void set_message(std::string title, std::string message, StatusType type = StatusType::Info);
    void render(Renderer& renderer) const;

private:
    std::string title_;
    std::string message_;
    StatusType type_{StatusType::Info};
};

} // namespace ui::components
