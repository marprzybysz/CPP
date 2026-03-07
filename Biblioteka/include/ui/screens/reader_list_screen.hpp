#pragma once

#include <optional>
#include <string>
#include <vector>

#include "readers/reader.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/search_box.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/readers_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReaderListScreen : public Screen {
public:
    explicit ReaderListScreen(controllers::ReadersController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_results();
    void sync_selected_reader();
    void refresh_rows();
    void apply_search_input(const std::string& raw);
    [[nodiscard]] std::optional<std::size_t> selected_result_index() const;

    controllers::ReadersController& controller_;
    components::Header header_;
    components::SearchBox search_box_;
    components::ListView results_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::ReaderSearchState search_state_;
    std::vector<readers::Reader> readers_;
    bool search_input_mode_{false};
};

} // namespace ui::screens
