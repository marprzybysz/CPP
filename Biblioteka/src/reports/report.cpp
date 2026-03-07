#include "reports/report.hpp"

#include "errors/app_error.hpp"

namespace reports {

std::string to_string(ReportType type) {
    switch (type) {
        case ReportType::OverdueBooks:
            return "OVERDUE_BOOKS";
        case ReportType::DelinquentReaders:
            return "DELINQUENT_READERS";
        case ReportType::MostBorrowedBooks:
            return "MOST_BORROWED_BOOKS";
        case ReportType::InventoryState:
            return "INVENTORY_STATE";
        case ReportType::ArchivedBooks:
            return "ARCHIVED_BOOKS";
        case ReportType::CopiesInRepair:
            return "COPIES_IN_REPAIR";
        default:
            throw errors::ValidationError("Unsupported report type");
    }
}

ReportType report_type_from_string(const std::string& raw) {
    if (raw == "OVERDUE_BOOKS") {
        return ReportType::OverdueBooks;
    }
    if (raw == "DELINQUENT_READERS") {
        return ReportType::DelinquentReaders;
    }
    if (raw == "MOST_BORROWED_BOOKS") {
        return ReportType::MostBorrowedBooks;
    }
    if (raw == "INVENTORY_STATE") {
        return ReportType::InventoryState;
    }
    if (raw == "ARCHIVED_BOOKS") {
        return ReportType::ArchivedBooks;
    }
    if (raw == "COPIES_IN_REPAIR") {
        return ReportType::CopiesInRepair;
    }

    throw errors::ValidationError("Unsupported report type: " + raw);
}

} // namespace reports
