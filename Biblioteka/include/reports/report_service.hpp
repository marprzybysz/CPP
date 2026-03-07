#pragma once

#include "common/system_id.hpp"
#include "reports/report_repository.hpp"

#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

namespace reports {

class ReportService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    ReportService(ReportRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    OverdueBooksReport generate_overdue_books_report(const ReportQueryOptions& options);
    DelinquentReadersReport generate_delinquent_readers_report(const ReportQueryOptions& options);
    MostBorrowedBooksReport generate_most_borrowed_books_report(const ReportQueryOptions& options);
    InventoryStateReport generate_inventory_state_report(const ReportQueryOptions& options);
    ArchivedBooksReport generate_archived_books_report(const ReportQueryOptions& options);
    CopiesInRepairReport generate_copies_in_repair_report(const ReportQueryOptions& options);

private:
    static std::string normalize_text(std::string value);
    static void validate_date_range(const DateRangeFilter& range);
    static void validate_limit(int limit);

    std::optional<std::string> maybe_save_snapshot(ReportType type,
                                                   const ReportQueryOptions& options,
                                                   const std::string& payload);
    void ensure_sequence_initialized(int year);

    ReportRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace reports
