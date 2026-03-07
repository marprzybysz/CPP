#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/copies_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class CopyStatusScreen : public Screen {
public:
    explicit CopyStatusScreen(controllers::CopiesController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    [[nodiscard]] std::optional<copies::CopyStatus> selected_status() const;

    controllers::CopiesController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    components::ListView status_view_;
    std::vector<copies::CopyStatus> statuses_;
    std::optional<controllers::CopyListItem> current_copy_;
    std::string note_;
    std::string archival_reason_;
};

} // namespace ui::screens
