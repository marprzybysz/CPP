#pragma once
#include <optional>
#include <string>
#include <vector>

#include "audit/audit_event.hpp"
#include "audit/audit_service.hpp"
#include "audit/sqlite_audit_repository.hpp"
#include "books/book.hpp"
#include "books/book_service.hpp"
#include "books/sqlite_book_repository.hpp"
#include "common/system_id.hpp"
#include "copies/copy.hpp"
#include "copies/copy_service.hpp"
#include "copies/sqlite_copy_repository.hpp"
#include "db.hpp"
#include "exports/export.hpp"
#include "exports/export_service.hpp"
#include "exports/sqlite_export_repository.hpp"
#include "imports/csv_import_parser.hpp"
#include "imports/import.hpp"
#include "imports/import_service.hpp"
#include "imports/sqlite_import_repository.hpp"
#include "inventory/inventory.hpp"
#include "inventory/inventory_service.hpp"
#include "inventory/sqlite_inventory_repository.hpp"
#include "locations/location.hpp"
#include "locations/location_service.hpp"
#include "locations/sqlite_location_repository.hpp"
#include "notes/note.hpp"
#include "notes/note_service.hpp"
#include "notes/sqlite_note_repository.hpp"
#include "readers/reader.hpp"
#include "readers/reader_service.hpp"
#include "readers/sqlite_reader_repository.hpp"
#include "reports/report.hpp"
#include "reports/report_service.hpp"
#include "reports/sqlite_report_repository.hpp"
#include "reputation/reputation.hpp"
#include "reputation/reputation_service.hpp"
#include "reputation/sqlite_reputation_repository.hpp"
#include "reservations/reservation.hpp"
#include "reservations/reservation_service.hpp"
#include "reservations/sqlite_reservation_repository.hpp"
#include "search/search.hpp"
#include "search/search_service.hpp"
#include "search/sqlite_search_repository.hpp"

class Library {
public:
    explicit Library(Db& db);

    books::Book add_book(const books::CreateBookInput& input);
    books::Book edit_book(const std::string& public_id, const books::UpdateBookInput& input);
    void archive_book(const std::string& public_id);
    books::Book get_book_details(const std::string& public_id, bool include_archived = false) const;
    std::vector<books::Book> list_books(bool include_archived = false, int limit = 100, int offset = 0) const;
    std::vector<books::Book> search_books(const books::BookQuery& query) const;
    void display_books(const std::vector<books::Book>& books) const;

    copies::BookCopy add_copy(const copies::CreateCopyInput& input);
    copies::BookCopy edit_copy(const std::string& public_id, const copies::UpdateCopyInput& input);
    copies::BookCopy get_copy(const std::string& public_id) const;
    std::vector<copies::BookCopy> list_book_copies(int book_id, int limit = 100, int offset = 0) const;
    copies::BookCopy change_copy_status(const std::string& public_id,
                                        copies::CopyStatus new_status,
                                        const std::string& note = "",
                                        const std::optional<std::string>& archival_reason = std::nullopt);
    copies::BookCopy change_copy_location(const std::string& public_id,
                                          const std::optional<std::string>& current_location_id,
                                          const std::optional<std::string>& target_location_id,
                                          const std::string& note = "");

    locations::Location create_location(const locations::CreateLocationInput& input);
    locations::Location edit_location(const std::string& public_id, const locations::UpdateLocationInput& input);
    locations::Location get_location(const std::string& public_id) const;
    std::vector<locations::LocationNode> get_location_tree() const;
    std::vector<copies::BookCopy> get_location_copies(const std::string& public_id) const;

    inventory::InventorySession start_inventory(const inventory::StartInventoryInput& input);
    inventory::InventoryScannedCopy register_inventory_scan(const std::string& session_public_id,
                                                            const inventory::RegisterScannedCopyInput& input);
    inventory::InventoryResult finish_inventory(const std::string& session_public_id, const inventory::FinishInventoryInput& input);
    inventory::InventoryResult get_inventory_result(const std::string& session_public_id) const;

    readers::Reader add_reader(const readers::CreateReaderInput& input);
    readers::Reader edit_reader(const std::string& public_id, const readers::UpdateReaderInput& input);
    std::vector<readers::Reader> search_readers(const readers::ReaderQuery& query) const;
    readers::Reader get_reader_details(const std::string& public_id) const;
    readers::Reader block_reader(const std::string& public_id, const std::string& reason);
    readers::Reader unblock_reader(const std::string& public_id);
    int get_reader_reputation(int reader_id) const;
    std::vector<readers::ReaderLoanHistoryEntry> get_reader_loan_history(const std::string& reader_public_id) const;
    std::vector<reputation::ReputationChange> get_reader_reputation_history(int reader_id,
                                                                            int limit = 100,
                                                                            int offset = 0) const;
    reputation::ReputationChange apply_reputation_manual_adjustment(int reader_id,
                                                                    int change_value,
                                                                    const std::string& reason,
                                                                    const std::optional<int>& related_loan_id = std::nullopt);
    reputation::ReputationChange apply_reputation_on_loan_return(const reputation::LoanReturnEvent& event);

    notes::Note add_note(const notes::CreateNoteInput& input);
    std::vector<notes::Note> get_notes_for_target(const notes::NotesForTargetQuery& query) const;
    notes::Note get_note_details(const std::string& public_id, bool include_archived = false) const;
    void archive_note(const std::string& public_id);

    reservations::Reservation create_reservation(const reservations::CreateReservationInput& input);
    reservations::Reservation get_reservation_details(const std::string& public_id) const;
    reservations::Reservation cancel_reservation(const std::string& public_id);
    reservations::Reservation expire_reservation(const std::string& public_id);
    reservations::Reservation fulfill_loan(const std::string& public_id);
    reservations::Reservation extend_loan(const std::string& public_id, const std::string& expiration_date);
    std::vector<reservations::LoanListItem> list_loans(const reservations::LoanListQuery& query) const;
    std::optional<reservations::Reservation> find_active_reservation_for_returned_copy(int copy_id) const;

    reports::OverdueBooksReport generate_overdue_books_report(const reports::ReportQueryOptions& options);
    reports::DelinquentReadersReport generate_delinquent_readers_report(const reports::ReportQueryOptions& options);
    reports::MostBorrowedBooksReport generate_most_borrowed_books_report(const reports::ReportQueryOptions& options);
    reports::InventoryStateReport generate_inventory_state_report(const reports::ReportQueryOptions& options);
    reports::ArchivedBooksReport generate_archived_books_report(const reports::ReportQueryOptions& options);
    reports::CopiesInRepairReport generate_copies_in_repair_report(const reports::ReportQueryOptions& options);
    search::GlobalSearchResult global_search(const search::SearchQuery& query) const;

    exports::CopyWithdrawal withdraw_copy(const exports::WithdrawCopyInput& input);
    std::vector<exports::WithdrawnCopyView> list_withdrawn_copies(int limit = 100, int offset = 0) const;
    imports::ImportReport import_csv(const imports::ImportRequest& request);
    imports::ImportReport get_import_report(const std::string& public_id) const;

    audit::AuditEvent log_audit_event(const audit::AuditLogInput& input);
    std::vector<audit::AuditEvent> get_recent_audit_events(int limit = 100) const;
    std::vector<audit::AuditEvent> get_audit_events_for_object(const std::string& object_type,
                                                               const std::string& object_public_id,
                                                               int limit = 100) const;

    audit::AuditEvent log_supply_event(const audit::AuditLogInput& input);
    audit::AuditEvent log_export_event(const audit::AuditLogInput& input);
    audit::AuditEvent log_loan_event(const audit::AuditLogInput& input);

private:
    void log_system_audit(audit::AuditModule module,
                          const std::string& object_type,
                          const std::string& object_public_id,
                          const std::string& operation_type,
                          const std::string& details);

    Db& db_;
    common::SystemIdGenerator id_generator_;
    audit::SqliteAuditRepository audit_repository_;
    audit::AuditService audit_service_;
    books::SqliteBookRepository book_repository_;
    books::BookService book_service_;
    copies::SqliteCopyRepository copy_repository_;
    copies::CopyService copy_service_;
    locations::SqliteLocationRepository location_repository_;
    locations::LocationService location_service_;
    inventory::SqliteInventoryRepository inventory_repository_;
    inventory::InventoryService inventory_service_;
    readers::SqliteReaderRepository reader_repository_;
    readers::ReaderService reader_service_;
    reputation::SqliteReputationRepository reputation_repository_;
    reputation::ReputationService reputation_service_;
    notes::SqliteNoteRepository note_repository_;
    notes::NoteService note_service_;
    reservations::SqliteReservationRepository reservation_repository_;
    reservations::ReservationService reservation_service_;
    reports::SqliteReportRepository report_repository_;
    reports::ReportService report_service_;
    search::SqliteSearchRepository search_repository_;
    search::SearchService search_service_;
    exports::SqliteExportRepository export_repository_;
    exports::ExportService export_service_;
    imports::SqliteImportRepository import_repository_;
    imports::CsvImportParser csv_import_parser_;
    imports::ImportService import_service_;
};
