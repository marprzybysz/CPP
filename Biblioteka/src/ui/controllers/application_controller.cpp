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
#include "ui/screens/placeholder_screen.hpp"
#include "ui/screens/reader_block_dialog_screen.hpp"
#include "ui/screens/reader_details_screen.hpp"
#include "ui/screens/reader_form_screen.hpp"
#include "ui/screens/reader_list_screen.hpp"
#include "ui/screens/reader_loan_history_screen.hpp"
#include "ui/screens/reader_reputation_screen.hpp"
#include "ui/screens/reader_unblock_dialog_screen.hpp"
#include "ui/screen_manager.hpp"

namespace ui::controllers {

ApplicationController::ApplicationController(Library& library, ScreenManager& screen_manager)
    : library_(library),
      screen_manager_(screen_manager),
      dashboard_controller_(library_),
      books_controller_(library_),
      copies_controller_(library_),
      readers_controller_(library_) {}

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
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("loans", "Wypozyczenia"));
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("reservations", "Rezerwacje"));
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("locations", "Lokalizacje"));
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("inventory", "Inwentaryzacja"));
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("reports", "Raporty"));
    screen_manager_.register_screen(std::make_unique<screens::PlaceholderScreen>("notes", "Notatki"));
    screen_manager_.set_active("dashboard");
}

} // namespace ui::controllers
