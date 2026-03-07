#include "ui/components/confirm_dialog.hpp"

#include <cctype>
#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

ConfirmDialog::ConfirmDialog(std::string question) : question_(std::move(question)) {}

void ConfirmDialog::set_question(std::string question) {
    question_ = std::move(question);
}

void ConfirmDialog::handle_input(const ui::InputEvent& event) {
    if (event.key == ui::Key::Left || event.key == ui::Key::Up) {
        confirm_yes_ = true;
        return;
    }

    if (event.key == ui::Key::Right || event.key == ui::Key::Down) {
        confirm_yes_ = false;
        return;
    }

    if (!event.raw.empty()) {
        const char key = static_cast<char>(std::tolower(event.raw.front()));
        if (key == 'y') {
            confirm_yes_ = true;
        }
        if (key == 'n') {
            confirm_yes_ = false;
        }
    }
}

bool ConfirmDialog::is_confirmed() const {
    return confirm_yes_;
}

void ConfirmDialog::render(Renderer& renderer) const {
    renderer.draw_header("Potwierdzenie");
    renderer.draw_line(question_);

    const std::string yes_label = confirm_yes_ ? "[TAK]" : " TAK ";
    const std::string no_label = confirm_yes_ ? " NIE " : "[NIE]";
    renderer.draw_line(yes_label + "  " + no_label, ui::style::TextStyle::Highlight);
}

} // namespace ui::components
