#pragma once

#include <optional>
#include <string>
#include <vector>

#include "readers/reader.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/form_field.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/readers_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReaderFormScreen : public Screen {
public:
    explicit ReaderFormScreen(controllers::ReadersController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_text_input() const override { return true; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void load_form_state();
    void rebuild_fields();
    void set_field_value(std::size_t index, const std::string& value);
    bool save(ScreenManager& manager);

    [[nodiscard]] static std::optional<std::string> normalize_optional(const std::string& raw);
    [[nodiscard]] static bool parse_bool(const std::string& raw, bool default_value);
    [[nodiscard]] static std::optional<readers::AccountStatus> parse_account_status(const std::string& raw);

    controllers::ReadersController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::ReaderFormState form_state_;
    std::vector<components::FormField> fields_;
    std::vector<std::string> values_;
    std::optional<readers::Reader> original_reader_;
    std::size_t focused_field_{0};
};

} // namespace ui::screens
