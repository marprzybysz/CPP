#pragma once

#include <optional>
#include <string>
#include <vector>

#include "copies/copy.hpp"
#include "locations/location.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/locations_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class LocationDetailsScreen : public Screen {
public:
    explicit LocationDetailsScreen(controllers::LocationsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    controllers::LocationsController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    std::optional<locations::Location> location_;
    std::vector<copies::BookCopy> copies_;
};

} // namespace ui::screens
