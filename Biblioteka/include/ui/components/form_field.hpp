#pragma once

#include <string>

namespace ui {
class Renderer;
}

namespace ui::components {

class FormField {
public:
    FormField(std::string label = "", std::string value = "", std::string placeholder = "", bool required = false);

    void set_value(std::string value);
    void set_focus(bool focused);
    [[nodiscard]] const std::string& value() const;
    void render(Renderer& renderer) const;

private:
    std::string label_;
    std::string value_;
    std::string placeholder_;
    bool required_{false};
    bool focused_{false};
};

} // namespace ui::components
