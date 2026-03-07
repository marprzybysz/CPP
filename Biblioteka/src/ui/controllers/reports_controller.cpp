#include "ui/controllers/reports_controller.hpp"

#include "library.hpp"

namespace ui::controllers {

ReportsController::ReportsController(Library& library) : library_(library) {}

ReportResultData ReportsController::generate_report(reports::ReportType type, const ReportFilterState& filter) const {
    const auto options = make_options(filter);

    ReportResultData out;
    out.type = type;
    out.title = reports::to_string(type);

    switch (type) {
        case reports::ReportType::OverdueBooks: {
            const auto report = library_.generate_overdue_books_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.reservation_public_id + " | " + row.reader_name + " | " + row.book_title + " | overdue=" +
                                   std::to_string(row.overdue_days));
            }
            break;
        }
        case reports::ReportType::DelinquentReaders: {
            const auto report = library_.generate_delinquent_readers_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.reader_public_id + " | " + row.full_name + " | overdue=" +
                                   std::to_string(row.overdue_items) + " | expired=" +
                                   std::to_string(row.expired_reservations));
            }
            break;
        }
        case reports::ReportType::MostBorrowedBooks: {
            const auto report = library_.generate_most_borrowed_books_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.book_public_id + " | " + row.title + " | borrows=" + std::to_string(row.borrow_count));
            }
            break;
        }
        case reports::ReportType::InventoryState: {
            const auto report = library_.generate_inventory_state_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.inventory_public_id + " | " + row.location_public_id + " | missing=" +
                                   std::to_string(row.missing_count));
            }
            break;
        }
        case reports::ReportType::ArchivedBooks: {
            const auto report = library_.generate_archived_books_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.book_public_id + " | " + row.title + " | archived_at=" + row.archived_at);
            }
            break;
        }
        case reports::ReportType::CopiesInRepair: {
            const auto report = library_.generate_copies_in_repair_report(options);
            out.rows.reserve(report.rows.size());
            for (const auto& row : report.rows) {
                out.rows.push_back(row.copy_public_id + " | " + row.inventory_number + " | " + row.book_title);
            }
            break;
        }
        default:
            break;
    }

    return out;
}

void ReportsController::set_last_result(ReportResultData result) {
    last_result_ = std::move(result);
}

const std::optional<ReportResultData>& ReportsController::last_result() const {
    return last_result_;
}

reports::ReportQueryOptions ReportsController::make_options(const ReportFilterState& filter) const {
    reports::ReportQueryOptions options;
    options.date_range.from_date = filter.from_date;
    options.date_range.to_date = filter.to_date;
    options.limit = filter.limit;
    options.save_snapshot = false;
    options.generated_by = "tui";
    return options;
}

} // namespace ui::controllers
