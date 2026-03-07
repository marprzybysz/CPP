#pragma once

#include <string>
#include <vector>

#include "books/book.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/form_field.hpp"
#include "ui/components/header.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/books_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class BookFormScreen : public Screen {
public:
    explicit BookFormScreen(controllers::BooksController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void load_form_state();
    void rebuild_fields();
    void set_field_value(std::size_t index, const std::string& value);
    bool save(ScreenManager& manager);

    static std::optional<int> parse_year(const std::string& raw);

    controllers::BooksController& controller_;
    components::Header header_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::BookFormState form_state_;
    std::vector<components::FormField> fields_;
    std::vector<std::string> values_;
    std::optional<books::Book> original_book_;
    std::size_t focused_field_{0};
};

} // namespace ui::screens
