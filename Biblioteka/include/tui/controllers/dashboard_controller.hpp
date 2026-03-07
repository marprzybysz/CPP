#pragma once

#include <cstddef>

class Library;

namespace tui {

struct DashboardSummary {
    std::size_t books_count{0};
    std::size_t readers_count{0};
    std::size_t recent_audits_count{0};
};

class DashboardController {
public:
    explicit DashboardController(Library& library);

    [[nodiscard]] DashboardSummary load_summary() const;

private:
    Library& library_;
};

} // namespace tui
