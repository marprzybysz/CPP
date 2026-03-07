#pragma once

#include <optional>
#include <string>
#include <vector>

#include "notes/note.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/notes_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class NotesScreen : public Screen {
public:
    explicit NotesScreen(controllers::NotesController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_line_input() const override { return filter_input_mode_ || add_input_mode_; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_notes();
    void refresh_rows();
    void apply_filter_input(const std::string& raw);
    bool add_note(const std::string& raw);
    [[nodiscard]] std::optional<std::size_t> selected_index() const;
    [[nodiscard]] std::string filter_text() const;

    controllers::NotesController& controller_;
    components::Header header_;
    components::ListView notes_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::NotesFilterState filter_;
    std::vector<notes::Note> notes_;
    bool filter_input_mode_{false};
    bool add_input_mode_{false};
};

} // namespace ui::screens
