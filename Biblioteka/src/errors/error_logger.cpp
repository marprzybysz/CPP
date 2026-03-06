#include "errors/error_logger.hpp"

#include "errors/app_error.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>

namespace errors {

void log_error(const std::exception& ex) {
    log_error(ex, std::cerr);
}

void log_error(const std::exception& ex, std::ostream& out) {
    const std::time_t now = std::time(nullptr);
    out << "[ERROR " << std::put_time(std::localtime(&now), "%F %T") << "] ";

    if (const auto* app_error = dynamic_cast<const AppError*>(&ex)) {
        out << "code=" << static_cast<int>(app_error->code()) << " message=" << app_error->what() << '\n';
        return;
    }

    out << "code=unknown message=" << ex.what() << '\n';
}

} // namespace errors
