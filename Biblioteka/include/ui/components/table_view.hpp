#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ui {
class Renderer;
}

namespace ui::components {

struct TableColumn {
    std::string title;
    std::size_t width{0};
};

using TableRow = std::vector<std::string>;

class TableView {
public:
    TableView(std::vector<TableColumn> columns = {}, std::vector<TableRow> rows = {});

    void set_columns(std::vector<TableColumn> columns);
    void set_rows(std::vector<TableRow> rows);
    void render(Renderer& renderer) const;

private:
    [[nodiscard]] static std::string fit_cell(const std::string& value, std::size_t width);

    std::vector<TableColumn> columns_;
    std::vector<TableRow> rows_;
};

} // namespace ui::components
