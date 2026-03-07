#pragma once

#include "reports/report.hpp"

#include <cstdint>
#include <vector>

namespace reports {

class ReportRepository {
public:
    virtual ~ReportRepository() = default;

    virtual std::vector<OverdueBookRow> fetch_overdue_books(const DateRangeFilter& range, int limit) const = 0;
    virtual std::vector<DelinquentReaderRow> fetch_delinquent_readers(const DateRangeFilter& range, int limit) const = 0;
    virtual std::vector<MostBorrowedBookRow> fetch_most_borrowed_books(const DateRangeFilter& range, int limit) const = 0;
    virtual std::vector<InventoryStateRow> fetch_inventory_state(const DateRangeFilter& range, int limit) const = 0;
    virtual std::vector<ArchivedBookRow> fetch_archived_books(const DateRangeFilter& range, int limit) const = 0;
    virtual std::vector<RepairCopyRow> fetch_copies_in_repair(const DateRangeFilter& range, int limit) const = 0;

    virtual SavedReportSnapshot save_snapshot(const SavedReportSnapshot& snapshot) = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace reports
