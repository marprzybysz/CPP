#pragma once

#include <exception>
#include <string>

namespace errors {

[[nodiscard]] std::string to_user_message(const std::exception& ex);

} // namespace errors
