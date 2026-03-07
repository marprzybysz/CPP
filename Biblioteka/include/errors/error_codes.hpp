#pragma once

namespace errors {

enum class ErrorCode {
    Validation,
    Database,
    NotFound,
    Reader,
    Copy,
    Loan,
    Inventory,
    Report,
};

} // namespace errors
