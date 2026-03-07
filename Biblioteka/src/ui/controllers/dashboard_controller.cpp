#include "ui/controllers/dashboard_controller.hpp"

#include "library.hpp"
#include "readers/reader.hpp"
#include "reports/report.hpp"

namespace {

constexpr int kPageSize = 200;

std::string placeholder_note(const char* reason) {
    return std::string{"placeholder: "} + reason;
}

} // namespace

namespace ui::controllers {

DashboardController::DashboardController(Library& library) : library_(library) {}

DashboardStats DashboardController::load_stats() const {
    DashboardStats stats;

    stats.books.value = count_books();
    stats.readers.value = count_readers();

    reports::ReportQueryOptions report_options;
    report_options.limit = 500;
    const auto overdue_report = library_.generate_overdue_books_report(report_options);
    stats.overdue_books.value = overdue_report.rows.size();

    stats.copies.value = std::nullopt;
    stats.copies.note = placeholder_note("global copies counter is not exposed by Library facade yet");

    stats.active_loans.value = std::nullopt;
    stats.active_loans.note = placeholder_note("active loans counter is not exposed by Library facade yet");

    return stats;
}

std::size_t DashboardController::count_books() const {
    std::size_t total = 0;
    int offset = 0;

    while (true) {
        const auto page = library_.list_books(false, kPageSize, offset);
        total += page.size();
        if (static_cast<int>(page.size()) < kPageSize) {
            break;
        }

        offset += kPageSize;
    }

    return total;
}

std::size_t DashboardController::count_readers() const {
    std::size_t total = 0;
    int offset = 0;

    while (true) {
        readers::ReaderQuery query;
        query.limit = kPageSize;
        query.offset = offset;

        const auto page = library_.search_readers(query);
        total += page.size();
        if (static_cast<int>(page.size()) < kPageSize) {
            break;
        }

        offset += kPageSize;
    }

    return total;
}

} // namespace ui::controllers
