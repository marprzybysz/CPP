#include "tui/controllers/dashboard_controller.hpp"

#include "library.hpp"
#include "readers/reader.hpp"

namespace tui {

DashboardController::DashboardController(Library& library) : library_(library) {}

DashboardSummary DashboardController::load_summary() const {
    readers::ReaderQuery readers_query;
    readers_query.limit = 100;
    readers_query.offset = 0;

    DashboardSummary summary;
    summary.books_count = library_.list_books(false, 100, 0).size();
    summary.readers_count = library_.search_readers(readers_query).size();
    summary.recent_audits_count = library_.get_recent_audit_events(30).size();
    return summary;
}

} // namespace tui
