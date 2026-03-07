#pragma once

#include "db.hpp"
#include "reports/report_repository.hpp"

namespace reports {

class SqliteReportRepository final : public ReportRepository {
public:
    explicit SqliteReportRepository(Db& db) : db_(db) {}

    std::vector<OverdueBookRow> fetch_overdue_books(const DateRangeFilter& range, int limit) const override;
    std::vector<DelinquentReaderRow> fetch_delinquent_readers(const DateRangeFilter& range, int limit) const override;
    std::vector<MostBorrowedBookRow> fetch_most_borrowed_books(const DateRangeFilter& range, int limit) const override;
    std::vector<InventoryStateRow> fetch_inventory_state(const DateRangeFilter& range, int limit) const override;
    std::vector<ArchivedBookRow> fetch_archived_books(const DateRangeFilter& range, int limit) const override;
    std::vector<RepairCopyRow> fetch_copies_in_repair(const DateRangeFilter& range, int limit) const override;

    SavedReportSnapshot save_snapshot(const SavedReportSnapshot& snapshot) override;

    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace reports
