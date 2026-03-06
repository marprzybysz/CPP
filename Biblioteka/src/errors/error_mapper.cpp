#include "errors/error_mapper.hpp"

#include "errors/app_error.hpp"

namespace errors {

std::string to_user_message(const std::exception& ex) {
    if (const auto* app_error = dynamic_cast<const AppError*>(&ex)) {
        switch (app_error->code()) {
            case ErrorCode::Validation:
                return "Validation error: " + std::string(app_error->what());
            case ErrorCode::Database:
                return "Database error. Please retry or contact support.";
            case ErrorCode::NotFound:
                return "Requested resource was not found.";
            case ErrorCode::Reader:
                return "Reader operation failed.";
            case ErrorCode::Copy:
                return "Book copy is currently unavailable.";
            case ErrorCode::Loan:
                return "Loan operation failed.";
            case ErrorCode::Inventory:
                return "Inventory operation failed.";
            default:
                return "Unexpected application error.";
        }
    }

    return "Unexpected system error.";
}

} // namespace errors
