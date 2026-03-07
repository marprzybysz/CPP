#include "ui/components/table_view.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

#include "ui/renderer.hpp"
#include "ui/style.hpp"

namespace ui::components {

TableView::TableView(std::vector<TableColumn> columns, std::vector<TableRow> rows)
    : columns_(std::move(columns)), rows_(std::move(rows)) {}

void TableView::set_columns(std::vector<TableColumn> columns) {
    columns_ = std::move(columns);
}

void TableView::set_rows(std::vector<TableRow> rows) {
    rows_ = std::move(rows);
}

void TableView::render(Renderer& renderer) const {
    if (columns_.empty()) {
        renderer.draw_line("(brak kolumn tabeli)", ui::style::TextStyle::Warning);
        return;
    }

    std::ostringstream header;
    for (const auto& column : columns_) {
        const std::size_t width = std::max<std::size_t>(column.width, column.title.size());
        header << fit_cell(column.title, width) << " | ";
    }
    renderer.draw_line(header.str(), ui::style::TextStyle::Header);

    for (const auto& row : rows_) {
        std::ostringstream line;
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            const std::string value = (i < row.size()) ? row[i] : "";
            const std::size_t width = std::max<std::size_t>(columns_[i].width, columns_[i].title.size());
            line << fit_cell(value, width) << " | ";
        }

        renderer.draw_line(line.str());
    }

    if (rows_.empty()) {
        renderer.draw_line("(brak danych)", ui::style::TextStyle::Warning);
    }
}

std::string TableView::fit_cell(const std::string& value, std::size_t width) {
    if (width == 0) {
        return value;
    }

    if (value.size() >= width) {
        if (width <= 3) {
            return value.substr(0, width);
        }
        return value.substr(0, width - 3) + "...";
    }

    return value + std::string(width - value.size(), ' ');
}

} // namespace ui::components
