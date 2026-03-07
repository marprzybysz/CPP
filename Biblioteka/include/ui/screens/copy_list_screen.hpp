#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/search_box.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/copies_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class CopyListScreen : public Screen {
public:
    explicit CopyListScreen(controllers::CopiesController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_results();
    void sync_selected_copy();
    void refresh_rows();
    void cycle_status_filter();
    void apply_book_filter(const std::string& raw);
    [[nodiscard]] std::optional<std::size_t> selected_result_index() const;
    [[nodiscard]] std::string status_filter_label() const;
    [[nodiscard]] std::string format_row(const controllers::CopyListItem& row) const;

    controllers::CopiesController& controller_;
    components::Header header_;
    components::SearchBox search_box_;
    components::ListView results_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::CopyListFilter filter_;
    std::vector<controllers::CopyListItem> rows_;
    bool search_input_mode_{false};
    std::size_t status_filter_index_{0};
};

} // namespace ui::screens
