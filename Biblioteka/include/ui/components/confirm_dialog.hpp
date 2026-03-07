#pragma once

#include <string>

#include "ui/input_handler.hpp"

namespace ui {
class Renderer;
}

namespace ui::components {

class ConfirmDialog {
public:
    explicit ConfirmDialog(std::string question = "Czy potwierdzasz?");

    void set_question(std::string question);
    void handle_input(const ui::InputEvent& event);
    [[nodiscard]] bool is_confirmed() const;
    void render(Renderer& renderer) const;

private:
    std::string question_;
    bool confirm_yes_{true};
};

} // namespace ui::components
