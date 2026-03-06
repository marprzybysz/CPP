#pragma once

#include <exception>
#include <iosfwd>

namespace errors {

void log_error(const std::exception& ex);
void log_error(const std::exception& ex, std::ostream& out);

} // namespace errors
