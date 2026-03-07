#pragma once

#include <cstddef>
#include <optional>
#include <string>

class Library;

namespace ui::controllers {

struct DashboardMetric {
    std::optional<std::size_t> value;
    std::string note;
};

struct DashboardStats {
    DashboardMetric books;
    DashboardMetric copies;
    DashboardMetric readers;
    DashboardMetric active_loans;
    DashboardMetric overdue_books;
};

class DashboardController {
public:
    explicit DashboardController(Library& library);

    [[nodiscard]] DashboardStats load_stats() const;

private:
    [[nodiscard]] std::size_t count_books() const;
    [[nodiscard]] std::size_t count_readers() const;

    Library& library_;
};

} // namespace ui::controllers
