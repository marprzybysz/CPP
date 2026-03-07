#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "books/book.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/search_box.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/books_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class BookListScreen : public Screen {
public:
    explicit BookListScreen(controllers::BooksController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    [[nodiscard]] bool prefers_line_input() const override { return search_input_mode_; }
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_results();
    void sync_selected_book();
    void apply_search_input(const std::string& raw);
    void refresh_list_rows();
    [[nodiscard]] std::optional<std::size_t> selected_result_index() const;

    controllers::BooksController& controller_;
    components::Header header_;
    components::SearchBox search_box_;
    components::ListView results_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;
    controllers::BookSearchState search_state_;
    std::vector<books::Book> results_;
    bool search_input_mode_{false};
};

} // namespace ui::screens
