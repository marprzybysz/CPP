#pragma once

#include <optional>
#include <string>
#include <vector>

#include "locations/location.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/form_field.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/copies_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class CopyLocationScreen : public Screen {
public:
    explicit CopyLocationScreen(controllers::CopiesController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void rebuild_fields();
    void set_field_value(std::size_t index, const std::string& value);
    bool apply(ScreenManager& manager);
    [[nodiscard]] static std::optional<std::string> normalize_optional(const std::string& raw);

    controllers::CopiesController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    std::optional<controllers::CopyListItem> current_copy_;
    std::vector<components::FormField> fields_;
    std::vector<std::string> values_;
    std::size_t focused_field_{0};
    std::vector<locations::Location> available_locations_;
};

} // namespace ui::screens
