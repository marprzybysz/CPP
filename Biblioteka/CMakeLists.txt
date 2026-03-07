cmake_minimum_required(VERSION 3.20)
project(cpp_biblioteka VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp_biblioteka
    src/main.cpp
    src/db.cpp
    src/library.cpp
    src/common/system_id.cpp
    src/audit/audit_event.cpp
    src/audit/audit_service.cpp
    src/audit/sqlite_audit_repository.cpp
    src/books/book_service.cpp
    src/books/sqlite_book_repository.cpp
    src/copies/copy.cpp
    src/copies/copy_service.cpp
    src/copies/sqlite_copy_repository.cpp
    src/locations/location.cpp
    src/locations/location_service.cpp
    src/locations/sqlite_location_repository.cpp
    src/inventory/inventory.cpp
    src/inventory/inventory_service.cpp
    src/inventory/sqlite_inventory_repository.cpp
    src/exports/export.cpp
    src/exports/export_service.cpp
    src/exports/sqlite_export_repository.cpp
    src/imports/import.cpp
    src/imports/csv_import_parser.cpp
    src/imports/import_service.cpp
    src/imports/sqlite_import_repository.cpp
    src/notes/note.cpp
    src/notes/note_service.cpp
    src/notes/sqlite_note_repository.cpp
    src/readers/reader.cpp
    src/readers/reader_service.cpp
    src/readers/sqlite_reader_repository.cpp
    src/reputation/reputation_service.cpp
    src/reputation/sqlite_reputation_repository.cpp
    src/reports/report.cpp
    src/reports/report_service.cpp
    src/reports/sqlite_report_repository.cpp
    src/search/search_service.cpp
    src/search/sqlite_search_repository.cpp
    src/reservations/reservation.cpp
    src/reservations/reservation_service.cpp
    src/reservations/sqlite_reservation_repository.cpp
    src/errors/error_mapper.cpp
    src/errors/error_logger.cpp
    src/ui/application.cpp
    src/ui/input_handler.cpp
    src/ui/renderer.cpp
    src/ui/style.cpp
    src/ui/screen_manager.cpp
    src/ui/components/confirm_dialog.cpp
    src/ui/components/dialog.cpp
    src/ui/components/footer.cpp
    src/ui/components/form_field.cpp
    src/ui/components/header.cpp
    src/ui/components/list_view.cpp
    src/ui/components/menu.cpp
    src/ui/components/message_box.cpp
    src/ui/components/search_box.cpp
    src/ui/components/status_bar.cpp
    src/ui/components/table_view.cpp
    src/ui/controllers/application_controller.cpp
    src/ui/controllers/books_controller.cpp
    src/ui/controllers/copies_controller.cpp
    src/ui/controllers/dashboard_controller.cpp
    src/ui/controllers/readers_controller.cpp
    src/ui/screens/book_details_screen.cpp
    src/ui/screens/book_form_screen.cpp
    src/ui/screens/book_list_screen.cpp
    src/ui/screens/copy_details_screen.cpp
    src/ui/screens/copy_form_screen.cpp
    src/ui/screens/copy_list_screen.cpp
    src/ui/screens/copy_location_screen.cpp
    src/ui/screens/copy_status_screen.cpp
    src/ui/screens/dashboard_screen.cpp
    src/ui/screens/placeholder_screen.cpp
    src/ui/screens/reader_block_dialog_screen.cpp
    src/ui/screens/reader_details_screen.cpp
    src/ui/screens/reader_form_screen.cpp
    src/ui/screens/reader_list_screen.cpp
    src/ui/screens/reader_loan_history_screen.cpp
    src/ui/screens/reader_reputation_screen.cpp
    src/ui/screens/reader_unblock_dialog_screen.cpp
)

target_include_directories(cpp_biblioteka PRIVATE include)

# SQLite3
find_package(SQLite3 REQUIRED)
target_link_libraries(cpp_biblioteka PRIVATE SQLite::SQLite3)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(cpp_biblioteka PRIVATE -Wall -Wextra -Wpedantic)
endif()
