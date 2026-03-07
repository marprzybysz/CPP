#include "tui/screens/dashboard_screen.hpp"

#include <iostream>

#include "errors/error_mapper.hpp"

namespace tui {

DashboardScreen::DashboardScreen(DashboardController& controller) : controller_(controller) {}

std::string DashboardScreen::title() const {
    return "Dashboard";
}

void DashboardScreen::show() {
    std::cout << "\n=== Dashboard ===\n";
    try {
        const DashboardSummary summary = controller_.load_summary();
        std::cout << "Ksiazki (podglad): " << summary.books_count << "\n";
        std::cout << "Czytelnicy (podglad): " << summary.readers_count << "\n";
        std::cout << "Ostatnie logi audytu: " << summary.recent_audits_count << "\n";
    } catch (const std::exception& e) {
        std::cout << "Blad: " << errors::to_user_message(e) << "\n";
    }
}

} // namespace tui
