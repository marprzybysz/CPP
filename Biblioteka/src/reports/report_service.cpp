#include "reports/report_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <utility>

namespace reports {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[reports] " << message << '\n';
}

std::string escape_json(std::string value) {
    std::string out;
    out.reserve(value.size() + 8);

    for (const char ch : value) {
        switch (ch) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += ch;
                break;
        }
    }

    return out;
}

std::string to_payload(ReportType type, std::size_t rows, const ReportQueryOptions& options) {
    std::ostringstream out;
    out << "{";
    out << "\"type\":\"" << to_string(type) << "\",";
    out << "\"rows\":" << rows << ",";
    out << "\"limit\":" << options.limit << ",";
    out << "\"date_from\":\"" << escape_json(options.date_range.from_date.value_or("")) << "\",";
    out << "\"date_to\":\"" << escape_json(options.date_range.to_date.value_or("")) << "\"";
    out << "}";
    return out.str();
}

} // namespace

ReportService::ReportService(ReportRepository& repository,
                             common::SystemIdGenerator& id_generator,
                             OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

OverdueBooksReport ReportService::generate_overdue_books_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    OverdueBooksReport report;
    report.rows = repository_.fetch_overdue_books(options.date_range, options.limit);
    report.public_id = maybe_save_snapshot(ReportType::OverdueBooks, options, to_payload(ReportType::OverdueBooks, report.rows.size(), options));

    logger_("generated report type=OVERDUE_BOOKS rows=" + std::to_string(report.rows.size()));
    return report;
}

DelinquentReadersReport ReportService::generate_delinquent_readers_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    DelinquentReadersReport report;
    report.rows = repository_.fetch_delinquent_readers(options.date_range, options.limit);
    report.public_id =
        maybe_save_snapshot(ReportType::DelinquentReaders, options, to_payload(ReportType::DelinquentReaders, report.rows.size(), options));

    logger_("generated report type=DELINQUENT_READERS rows=" + std::to_string(report.rows.size()));
    return report;
}

MostBorrowedBooksReport ReportService::generate_most_borrowed_books_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    MostBorrowedBooksReport report;
    report.rows = repository_.fetch_most_borrowed_books(options.date_range, options.limit);
    report.public_id =
        maybe_save_snapshot(ReportType::MostBorrowedBooks, options, to_payload(ReportType::MostBorrowedBooks, report.rows.size(), options));

    logger_("generated report type=MOST_BORROWED_BOOKS rows=" + std::to_string(report.rows.size()));
    return report;
}

InventoryStateReport ReportService::generate_inventory_state_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    InventoryStateReport report;
    report.rows = repository_.fetch_inventory_state(options.date_range, options.limit);
    report.public_id = maybe_save_snapshot(ReportType::InventoryState, options, to_payload(ReportType::InventoryState, report.rows.size(), options));

    logger_("generated report type=INVENTORY_STATE rows=" + std::to_string(report.rows.size()));
    return report;
}

ArchivedBooksReport ReportService::generate_archived_books_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    ArchivedBooksReport report;
    report.rows = repository_.fetch_archived_books(options.date_range, options.limit);
    report.public_id = maybe_save_snapshot(ReportType::ArchivedBooks, options, to_payload(ReportType::ArchivedBooks, report.rows.size(), options));

    logger_("generated report type=ARCHIVED_BOOKS rows=" + std::to_string(report.rows.size()));
    return report;
}

CopiesInRepairReport ReportService::generate_copies_in_repair_report(const ReportQueryOptions& options) {
    validate_date_range(options.date_range);
    validate_limit(options.limit);

    CopiesInRepairReport report;
    report.rows = repository_.fetch_copies_in_repair(options.date_range, options.limit);
    report.public_id =
        maybe_save_snapshot(ReportType::CopiesInRepair, options, to_payload(ReportType::CopiesInRepair, report.rows.size(), options));

    logger_("generated report type=COPIES_IN_REPAIR rows=" + std::to_string(report.rows.size()));
    return report;
}

std::string ReportService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

void ReportService::validate_date_range(const DateRangeFilter& range) {
    if (range.from_date.has_value() && normalize_text(*range.from_date).empty()) {
        throw errors::ValidationError("from_date cannot be blank");
    }
    if (range.to_date.has_value() && normalize_text(*range.to_date).empty()) {
        throw errors::ValidationError("to_date cannot be blank");
    }
    if (range.from_date.has_value() && range.to_date.has_value() && *range.from_date > *range.to_date) {
        throw errors::ValidationError("from_date must be less than or equal to to_date");
    }
}

void ReportService::validate_limit(int limit) {
    if (limit <= 0) {
        throw errors::ValidationError("report limit must be greater than zero");
    }
    if (limit > 1000) {
        throw errors::ValidationError("report limit cannot be greater than 1000");
    }
}

std::optional<std::string> ReportService::maybe_save_snapshot(ReportType type,
                                                              const ReportQueryOptions& options,
                                                              const std::string& payload) {
    if (!options.save_snapshot) {
        return std::nullopt;
    }

    const int year = common::current_year();
    ensure_sequence_initialized(year);

    SavedReportSnapshot snapshot;
    snapshot.public_id = id_generator_.generate_for_year(common::IdType::Report, year);
    snapshot.type = type;
    snapshot.date_from = options.date_range.from_date;
    snapshot.date_to = options.date_range.to_date;
    snapshot.generated_by = normalize_text(options.generated_by);
    if (snapshot.generated_by.empty()) {
        throw errors::ReportError("generated_by is required when save_snapshot=true");
    }
    snapshot.payload = payload;

    const SavedReportSnapshot created = repository_.save_snapshot(snapshot);
    return created.public_id;
}

void ReportService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Report, year, next);
    initialized_years_.insert(year);
}

} // namespace reports
