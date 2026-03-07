#include "library.hpp"

#include <iostream>

namespace {

copies::CopyStatus withdrawal_status_for_reason(exports::WithdrawalReason reason) {
    if (reason == exports::WithdrawalReason::Lost) {
        return copies::CopyStatus::Lost;
    }
    return copies::CopyStatus::Archived;
}

std::string withdrawal_details(exports::WithdrawalReason reason, const std::string& note) {
    std::string details = "reason=" + exports::to_string(reason);
    if (!note.empty()) {
        details += "; note=" + note;
    }
    return details;
}

} // namespace

Library::Library(Db& db)
    : db_(db),
      id_generator_(),
      audit_repository_(db_),
      audit_service_(audit_repository_),
      book_repository_(db_),
      book_service_(book_repository_, id_generator_),
      copy_repository_(db_),
      copy_service_(copy_repository_, id_generator_),
      location_repository_(db_),
      location_service_(location_repository_, id_generator_),
      inventory_repository_(db_),
      inventory_service_(inventory_repository_, id_generator_),
      reader_repository_(db_),
      reader_service_(reader_repository_, id_generator_),
      reputation_repository_(db_),
      reputation_service_(reputation_repository_),
      note_repository_(db_),
      note_service_(note_repository_, id_generator_),
      reservation_repository_(db_),
      reservation_service_(reservation_repository_, id_generator_),
      report_repository_(db_),
      report_service_(report_repository_, id_generator_),
      search_repository_(db_),
      search_service_(search_repository_),
      export_repository_(db_),
      export_service_(export_repository_),
      import_repository_(db_),
      csv_import_parser_(),
      import_service_(import_repository_,
                      book_service_,
                      reader_service_,
                      audit_service_,
                      id_generator_,
                      std::vector<imports::ImportParser*>{&csv_import_parser_}) {}

books::Book Library::add_book(const books::CreateBookInput& input) {
    books::Book created = book_service_.add_book(input);
    log_system_audit(audit::AuditModule::Books, "BOOK", created.public_id, "CREATE", "book added");
    return created;
}

books::Book Library::edit_book(const std::string& public_id, const books::UpdateBookInput& input) {
    books::Book updated = book_service_.edit_book(public_id, input);
    log_system_audit(audit::AuditModule::Books, "BOOK", updated.public_id, "UPDATE", "book edited");
    return updated;
}

void Library::archive_book(const std::string& public_id) {
    book_service_.archive_book(public_id);
    log_system_audit(audit::AuditModule::Books, "BOOK", public_id, "ARCHIVE", "book archived");
}

books::Book Library::get_book_details(const std::string& public_id, bool include_archived) const {
    return book_service_.get_book_details(public_id, include_archived);
}

std::vector<books::Book> Library::list_books(bool include_archived, int limit, int offset) const {
    return book_service_.list_books(include_archived, limit, offset);
}

std::vector<books::Book> Library::search_books(const books::BookQuery& query) const {
    return book_service_.search_books(query);
}

void Library::display_books(const std::vector<books::Book>& books) const {
    for (const auto& book : books) {
        std::cout << "[" << book.public_id << "] " << book.title << " by " << book.author
                  << " | ISBN: " << book.isbn;
        if (book.is_archived) {
            std::cout << " | ARCHIVED";
        }
        std::cout << '\n';
    }
}

copies::BookCopy Library::add_copy(const copies::CreateCopyInput& input) {
    copies::BookCopy created = copy_service_.add_copy(input);
    log_system_audit(audit::AuditModule::Copies, "COPY", created.public_id, "CREATE", "copy added");
    return created;
}

copies::BookCopy Library::edit_copy(const std::string& public_id, const copies::UpdateCopyInput& input) {
    copies::BookCopy updated = copy_service_.edit_copy(public_id, input);
    log_system_audit(audit::AuditModule::Copies, "COPY", updated.public_id, "UPDATE", "copy edited");
    return updated;
}

copies::BookCopy Library::get_copy(const std::string& public_id) const {
    return copy_service_.get_copy(public_id);
}

std::vector<copies::BookCopy> Library::list_book_copies(int book_id, int limit, int offset) const {
    return copy_service_.list_book_copies(book_id, limit, offset);
}

copies::BookCopy Library::change_copy_status(const std::string& public_id,
                                             copies::CopyStatus new_status,
                                             const std::string& note,
                                             const std::optional<std::string>& archival_reason) {
    copies::BookCopy updated = copy_service_.change_status(public_id, new_status, note, archival_reason);
    log_system_audit(
        audit::AuditModule::Copies, "COPY", updated.public_id, "CHANGE_STATUS", "status changed to " + copies::to_string(new_status));
    return updated;
}

copies::BookCopy Library::change_copy_location(const std::string& public_id,
                                               const std::optional<std::string>& current_location_id,
                                               const std::optional<std::string>& target_location_id,
                                               const std::string& note) {
    copies::BookCopy updated = copy_service_.change_location(public_id, current_location_id, target_location_id, note);
    log_system_audit(audit::AuditModule::Copies, "COPY", updated.public_id, "CHANGE_LOCATION", "copy location changed");
    return updated;
}

locations::Location Library::create_location(const locations::CreateLocationInput& input) {
    return location_service_.create_location(input);
}

locations::Location Library::edit_location(const std::string& public_id, const locations::UpdateLocationInput& input) {
    return location_service_.edit_location(public_id, input);
}

locations::Location Library::get_location(const std::string& public_id) const {
    return location_service_.get_location(public_id);
}

std::vector<locations::LocationNode> Library::get_location_tree() const {
    return location_service_.get_location_tree();
}

std::vector<copies::BookCopy> Library::get_location_copies(const std::string& public_id) const {
    return location_service_.get_location_copies(public_id);
}

inventory::InventorySession Library::start_inventory(const inventory::StartInventoryInput& input) {
    inventory::InventorySession session = inventory_service_.start_inventory(input);
    log_system_audit(
        audit::AuditModule::Inventory, "INVENTORY_SESSION", session.public_id, "START", "inventory started at " + session.location_public_id);
    return session;
}

inventory::InventoryScannedCopy Library::register_inventory_scan(const std::string& session_public_id,
                                                                 const inventory::RegisterScannedCopyInput& input) {
    inventory::InventoryScannedCopy scan = inventory_service_.register_scanned_copy(session_public_id, input);
    log_system_audit(audit::AuditModule::Inventory,
                     "INVENTORY_SESSION",
                     session_public_id,
                     "REGISTER_SCAN",
                     "scan registered for copy " + scan.copy_public_id);
    return scan;
}

inventory::InventoryResult Library::finish_inventory(const std::string& session_public_id,
                                                     const inventory::FinishInventoryInput& input) {
    inventory::InventoryResult result = inventory_service_.finish_inventory(session_public_id, input);
    log_system_audit(audit::AuditModule::Inventory,
                     "INVENTORY_SESSION",
                     session_public_id,
                     "FINISH",
                     result.session.summary_result);
    return result;
}

inventory::InventoryResult Library::get_inventory_result(const std::string& session_public_id) const {
    return inventory_service_.get_inventory_result(session_public_id);
}

readers::Reader Library::add_reader(const readers::CreateReaderInput& input) {
    readers::Reader created = reader_service_.add_reader(input);
    log_system_audit(audit::AuditModule::Readers, "READER", created.public_id, "CREATE", "reader added");
    return created;
}

readers::Reader Library::edit_reader(const std::string& public_id, const readers::UpdateReaderInput& input) {
    readers::Reader updated = reader_service_.edit_reader(public_id, input);
    log_system_audit(audit::AuditModule::Readers, "READER", updated.public_id, "UPDATE", "reader edited");
    return updated;
}

std::vector<readers::Reader> Library::search_readers(const readers::ReaderQuery& query) const {
    return reader_service_.search_readers(query);
}

readers::Reader Library::get_reader_details(const std::string& public_id) const {
    return reader_service_.get_reader_details(public_id);
}

readers::Reader Library::block_reader(const std::string& public_id, const std::string& reason) {
    readers::Reader updated = reader_service_.block_account(public_id, reason);
    log_system_audit(audit::AuditModule::Readers, "READER", updated.public_id, "BLOCK", reason);
    return updated;
}

readers::Reader Library::unblock_reader(const std::string& public_id) {
    readers::Reader updated = reader_service_.unblock_account(public_id);
    log_system_audit(audit::AuditModule::Readers, "READER", updated.public_id, "UNBLOCK", "reader unblocked");
    return updated;
}

int Library::get_reader_reputation(int reader_id) const {
    return reputation_service_.get_current_reputation(reader_id);
}

std::vector<readers::ReaderLoanHistoryEntry> Library::get_reader_loan_history(const std::string& reader_public_id) const {
    return reader_repository_.list_loan_history(reader_public_id);
}

std::vector<reputation::ReputationChange> Library::get_reader_reputation_history(int reader_id, int limit, int offset) const {
    return reputation_service_.get_reputation_history(reader_id, limit, offset);
}

reputation::ReputationChange Library::apply_reputation_manual_adjustment(int reader_id,
                                                                         int change_value,
                                                                         const std::string& reason,
                                                                         const std::optional<int>& related_loan_id) {
    return reputation_service_.apply_manual_adjustment(reader_id, change_value, reason, related_loan_id);
}

reputation::ReputationChange Library::apply_reputation_on_loan_return(const reputation::LoanReturnEvent& event) {
    return reputation_service_.apply_on_loan_return(event);
}

notes::Note Library::add_note(const notes::CreateNoteInput& input) {
    return note_service_.add_note(input);
}

std::vector<notes::Note> Library::get_notes_for_target(const notes::NotesForTargetQuery& query) const {
    return note_service_.get_notes_for_target(query);
}

notes::Note Library::get_note_details(const std::string& public_id, bool include_archived) const {
    return note_service_.get_note_details(public_id, include_archived);
}

void Library::archive_note(const std::string& public_id) {
    note_service_.archive_note(public_id);
}

reservations::Reservation Library::create_reservation(const reservations::CreateReservationInput& input) {
    reservations::Reservation created = reservation_service_.create_reservation(input);
    log_system_audit(audit::AuditModule::Loans, "RESERVATION", created.public_id, "CREATE", "loan reservation created");
    return created;
}

reservations::Reservation Library::get_reservation_details(const std::string& public_id) const {
    return reservation_service_.get_reservation_details(public_id);
}

reservations::Reservation Library::cancel_reservation(const std::string& public_id) {
    reservations::Reservation updated = reservation_service_.cancel_reservation(public_id);
    log_system_audit(audit::AuditModule::Loans, "RESERVATION", updated.public_id, "CANCEL", "loan reservation cancelled");
    return updated;
}

reservations::Reservation Library::expire_reservation(const std::string& public_id) {
    reservations::Reservation updated = reservation_service_.expire_reservation(public_id);
    log_system_audit(audit::AuditModule::Loans, "RESERVATION", updated.public_id, "EXPIRE", "loan reservation expired");
    return updated;
}

std::optional<reservations::Reservation> Library::find_active_reservation_for_returned_copy(int copy_id) const {
    return reservation_service_.find_active_for_returned_copy(copy_id);
}

reports::OverdueBooksReport Library::generate_overdue_books_report(const reports::ReportQueryOptions& options) {
    reports::OverdueBooksReport report = report_service_.generate_overdue_books_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "overdue books report generated");
    }
    return report;
}

reports::DelinquentReadersReport Library::generate_delinquent_readers_report(const reports::ReportQueryOptions& options) {
    reports::DelinquentReadersReport report = report_service_.generate_delinquent_readers_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "delinquent readers report generated");
    }
    return report;
}

reports::MostBorrowedBooksReport Library::generate_most_borrowed_books_report(const reports::ReportQueryOptions& options) {
    reports::MostBorrowedBooksReport report = report_service_.generate_most_borrowed_books_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "most borrowed books report generated");
    }
    return report;
}

reports::InventoryStateReport Library::generate_inventory_state_report(const reports::ReportQueryOptions& options) {
    reports::InventoryStateReport report = report_service_.generate_inventory_state_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "inventory state report generated");
    }
    return report;
}

reports::ArchivedBooksReport Library::generate_archived_books_report(const reports::ReportQueryOptions& options) {
    reports::ArchivedBooksReport report = report_service_.generate_archived_books_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "archived books report generated");
    }
    return report;
}

reports::CopiesInRepairReport Library::generate_copies_in_repair_report(const reports::ReportQueryOptions& options) {
    reports::CopiesInRepairReport report = report_service_.generate_copies_in_repair_report(options);
    if (report.public_id.has_value()) {
        log_system_audit(audit::AuditModule::Export, "REPORT", *report.public_id, "GENERATE", "copies in repair report generated");
    }
    return report;
}

search::GlobalSearchResult Library::global_search(const search::SearchQuery& query) const {
    return search_service_.search(query);
}

exports::CopyWithdrawal Library::withdraw_copy(const exports::WithdrawCopyInput& input) {
    const copies::CopyStatus resulting_status = withdrawal_status_for_reason(input.reason);

    std::optional<std::string> archival_reason = std::nullopt;
    if (resulting_status == copies::CopyStatus::Archived) {
        archival_reason = withdrawal_details(input.reason, input.note);
    }

    const copies::BookCopy updated_copy =
        copy_service_.change_status(input.copy_public_id, resulting_status, "withdrawn from circulation", archival_reason);
    const exports::CopyWithdrawal withdrawal = export_service_.withdraw_copy(input, updated_copy.status);

    log_system_audit(audit::AuditModule::Export,
                     "COPY",
                     withdrawal.copy_public_id,
                     "WITHDRAW",
                     withdrawal_details(withdrawal.reason, withdrawal.note));

    return withdrawal;
}

std::vector<exports::WithdrawnCopyView> Library::list_withdrawn_copies(int limit, int offset) const {
    return export_service_.list_withdrawn_copies(limit, offset);
}

imports::ImportReport Library::import_csv(const imports::ImportRequest& request) {
    imports::ImportRequest normalized = request;
    normalized.format = imports::ImportFormat::Csv;
    return import_service_.import_data(normalized);
}

imports::ImportReport Library::get_import_report(const std::string& public_id) const {
    return import_service_.get_import_report(public_id);
}

audit::AuditEvent Library::log_audit_event(const audit::AuditLogInput& input) {
    return audit_service_.log_event(input);
}

std::vector<audit::AuditEvent> Library::get_recent_audit_events(int limit) const {
    return audit_service_.get_recent_events(limit);
}

std::vector<audit::AuditEvent> Library::get_audit_events_for_object(const std::string& object_type,
                                                                     const std::string& object_public_id,
                                                                     int limit) const {
    return audit_service_.get_events_for_object(object_type, object_public_id, limit);
}

audit::AuditEvent Library::log_supply_event(const audit::AuditLogInput& input) {
    audit::AuditLogInput enriched = input;
    enriched.module = audit::AuditModule::Supply;
    return audit_service_.log_event(enriched);
}

audit::AuditEvent Library::log_export_event(const audit::AuditLogInput& input) {
    audit::AuditLogInput enriched = input;
    enriched.module = audit::AuditModule::Export;
    return audit_service_.log_event(enriched);
}

audit::AuditEvent Library::log_loan_event(const audit::AuditLogInput& input) {
    audit::AuditLogInput enriched = input;
    enriched.module = audit::AuditModule::Loans;
    return audit_service_.log_event(enriched);
}

void Library::log_system_audit(audit::AuditModule module,
                               const std::string& object_type,
                               const std::string& object_public_id,
                               const std::string& operation_type,
                               const std::string& details) {
    audit::AuditLogInput input;
    input.module = module;
    input.actor = "system";
    input.object_type = object_type;
    input.object_public_id = object_public_id;
    input.operation_type = operation_type;
    input.details = details;

    (void)audit_service_.log_event(input);
}
