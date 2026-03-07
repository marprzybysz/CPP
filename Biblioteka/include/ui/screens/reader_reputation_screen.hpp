#pragma once

#include <optional>
#include <string>
#include <vector>

#include "readers/reader.hpp"
#include "reputation/reputation.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/readers_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReaderReputationScreen : public Screen {
public:
    explicit ReaderReputationScreen(controllers::ReadersController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::ReadersController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::optional<readers::Reader> current_reader_;
    int points_{0};
    std::vector<reputation::ReputationChange> history_;
};

} // namespace ui::screens
