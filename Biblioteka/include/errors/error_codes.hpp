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
    Audit,
    Export,
    Import,
    Search,
};

} // namespace errors
