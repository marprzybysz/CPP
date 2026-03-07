#include "ui/components/footer.hpp"

#include <cstddef>
#include <sstream>
#include <utility>

#include "ui/renderer.hpp"

namespace ui::components {

Footer::Footer(std::vector<std::string> hints) : hints_(std::move(hints)) {}

void Footer::set_hints(std::vector<std::string> hints) {
    hints_ = std::move(hints);
}

void Footer::render(Renderer& renderer) const {
    renderer.draw_separator('-');

    if (hints_.empty()) {
        renderer.draw_line("Brak podpowiedzi.");
        return;
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < hints_.size(); ++i) {
        if (i > 0) {
            out << " | ";
        }
        out << hints_[i];
    }

    renderer.draw_line(out.str());
}

} // namespace ui::components
