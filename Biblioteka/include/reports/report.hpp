#pragma once

#include <optional>
#include <string>
#include <vector>

namespace reports {

enum class ReportType {
    OverdueBooks,
    DelinquentReaders,
    MostBorrowedBooks,
    InventoryState,
    ArchivedBooks,
    CopiesInRepair,
};

struct DateRangeFilter {
    std::optional<std::string> from_date;
    std::optional<std::string> to_date;
};

struct ReportQueryOptions {
    DateRangeFilter date_range;
    int limit{100};
    bool save_snapshot{false};
    std::string generated_by;
};

struct SavedReportSnapshot {
    std::optional<int> id;
    std::string public_id;
    ReportType type{ReportType::OverdueBooks};
    std::optional<std::string> date_from;
    std::optional<std::string> date_to;
    std::string generated_by;
    std::string payload;
    std::string created_at;
};

struct OverdueBookRow {
    std::string reservation_public_id;
    std::string reservation_date;
    std::string expiration_date;
    std::string copy_public_id;
    std::string inventory_number;
    std::string book_public_id;
    std::string book_title;
    std::string reader_public_id;
    std::string reader_name;
    int overdue_days{0};
};

struct DelinquentReaderRow {
    std::string reader_public_id;
    std::string card_number;
    std::string full_name;
    int reputation_points{0};
    bool is_blocked{false};
    int expired_reservations{0};
    int overdue_items{0};
};

struct MostBorrowedBookRow {
    std::string book_public_id;
    std::string title;
    std::string author;
    int borrow_count{0};
};

struct InventoryStateRow {
    std::string inventory_public_id;
    std::string location_public_id;
    std::string scope_type;
    std::string started_by;
    std::string started_at;
    std::optional<std::string> finished_at;
    int on_shelf_count{0};
    int justified_count{0};
    int missing_count{0};
};

struct ArchivedBookRow {
    std::string book_public_id;
    std::string title;
    std::string author;
    std::string isbn;
    std::string archived_at;
};

struct RepairCopyRow {
    std::string copy_public_id;
    std::string inventory_number;
    std::string condition_note;
    std::string updated_at;
    std::string book_public_id;
    std::string book_title;
    std::optional<std::string> current_location_id;
    std::optional<std::string> target_location_id;
};

struct OverdueBooksReport {
    std::optional<std::string> public_id;
    std::vector<OverdueBookRow> rows;
};

struct DelinquentReadersReport {
    std::optional<std::string> public_id;
    std::vector<DelinquentReaderRow> rows;
};

struct MostBorrowedBooksReport {
    std::optional<std::string> public_id;
    std::vector<MostBorrowedBookRow> rows;
};

struct InventoryStateReport {
    std::optional<std::string> public_id;
    std::vector<InventoryStateRow> rows;
};

struct ArchivedBooksReport {
    std::optional<std::string> public_id;
    std::vector<ArchivedBookRow> rows;
};

struct CopiesInRepairReport {
    std::optional<std::string> public_id;
    std::vector<RepairCopyRow> rows;
};

[[nodiscard]] std::string to_string(ReportType type);
[[nodiscard]] ReportType report_type_from_string(const std::string& raw);

} // namespace reports
