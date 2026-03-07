#include "ui/controllers/application_controller.hpp"

#include <memory>

#include "ui/screens/book_details_screen.hpp"
#include "ui/screens/book_form_screen.hpp"
#include "ui/screens/book_list_screen.hpp"
#include "ui/screens/copy_details_screen.hpp"
#include "ui/screens/copy_form_screen.hpp"
#include "ui/screens/copy_list_screen.hpp"
#include "ui/screens/copy_location_screen.hpp"
#include "ui/screens/copy_status_screen.hpp"
#include "ui/screens/dashboard_screen.hpp"
#include "ui/screens/audit_log_screen.hpp"
#include "ui/screens/inventory_list_screen.hpp"
#include "ui/screens/inventory_result_screen.hpp"
#include "ui/screens/inventory_session_screen.hpp"
#include "ui/screens/location_details_screen.hpp"
#include "ui/screens/location_tree_screen.hpp"
#include "ui/screens/loan_create_screen.hpp"
#include "ui/screens/loan_details_screen.hpp"
#include "ui/screens/loan_extend_dialog_screen.hpp"
#include "ui/screens/loan_list_screen.hpp"
#include "ui/screens/loan_return_dialog_screen.hpp"
#include "ui/screens/placeholder_screen.hpp"
#include "ui/screens/reader_block_dialog_screen.hpp"
#include "ui/screens/reader_details_screen.hpp"
#include "ui/screens/reader_form_screen.hpp"
#include "ui/screens/reader_list_screen.hpp"
#include "ui/screens/reader_loan_history_screen.hpp"
#include "ui/screens/reader_reputation_screen.hpp"
#include "ui/screens/reader_unblock_dialog_screen.hpp"
#include "ui/screens/report_menu_screen.hpp"
#include "ui/screens/report_result_screen.hpp"
#include "ui/screens/reservation_create_screen.hpp"
#include "ui/screens/reservation_details_screen.hpp"
#include "ui/screens/reservation_list_screen.hpp"
#include "ui/screens/notes_screen.hpp"
#include "ui/screen_manager.hpp"

namespace ui::controllers {

ApplicationController::ApplicationController(Library& library, ScreenManager& screen_manager)
    : library_(library),
      screen_manager_(screen_manager),
      dashboard_controller_(library_),
      audit_controller_(library_),
      books_controller_(library_),
      copies_controller_(library_),
      loans_controller_(library_),
      reports_controller_(library_),
      notes_controller_(library_),
      readers_controller_(library_),
      reservations_controller_(library_),
      locations_controller_(library_),
      inventory_controller_(library_) {}

void ApplicationController::bootstrap() {
    screen_manager_.register_screen(std::make_unique<screens::DashboardScreen>(dashboard_controller_));
    screen_manager_.register_screen(std::make_unique<screens::BookListScreen>(books_controller_));
    screen_manager_.register_screen(std::make_unique<screens::BookDetailsScreen>(books_controller_));
    screen_manager_.register_screen(std::make_unique<screens::BookFormScreen>(books_controller_));
    screen_manager_.register_screen(std::make_unique<screens::CopyListScreen>(copies_controller_));
    screen_manager_.register_screen(std::make_unique<screens::CopyDetailsScreen>(copies_controller_));
    screen_manager_.register_screen(std::make_unique<screens::CopyFormScreen>(copies_controller_));
    screen_manager_.register_screen(std::make_unique<screens::CopyStatusScreen>(copies_controller_));
    screen_manager_.register_screen(std::make_unique<screens::CopyLocationScreen>(copies_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderListScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderDetailsScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderFormScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderBlockDialogScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderUnblockDialogScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderReputationScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReaderLoanHistoryScreen>(readers_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LoanListScreen>(loans_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LoanDetailsScreen>(loans_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LoanCreateScreen>(loans_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LoanReturnDialogScreen>(loans_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LoanExtendDialogScreen>(loans_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReservationListScreen>(reservations_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReservationDetailsScreen>(reservations_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReservationCreateScreen>(reservations_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LocationTreeScreen>(locations_controller_));
    screen_manager_.register_screen(std::make_unique<screens::LocationDetailsScreen>(locations_controller_));
    screen_manager_.register_screen(std::make_unique<screens::InventoryListScreen>(inventory_controller_));
    screen_manager_.register_screen(std::make_unique<screens::InventorySessionScreen>(inventory_controller_));
    screen_manager_.register_screen(std::make_unique<screens::InventoryResultScreen>(inventory_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReportMenuScreen>(reports_controller_));
    screen_manager_.register_screen(std::make_unique<screens::ReportResultScreen>(reports_controller_));
    screen_manager_.register_screen(std::make_unique<screens::NotesScreen>(notes_controller_));
    screen_manager_.register_screen(std::make_unique<screens::AuditLogScreen>(audit_controller_));
    screen_manager_.set_active("dashboard");
}

} // namespace ui::controllers
