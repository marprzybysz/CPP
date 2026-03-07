#pragma once

#include <optional>
#include <string>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/reservations_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReservationDetailsScreen : public Screen {
public:
    explicit ReservationDetailsScreen(controllers::ReservationsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::ReservationsController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::optional<controllers::ReservationListEntry> current_;
};

} // namespace ui::screens
