#pragma once

#include "errors/error_codes.hpp"
#include <stdexcept>
#include <string>

namespace errors {

class AppError : public std::runtime_error {
public:
    AppError(ErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

class ValidationError : public AppError {
public:
    explicit ValidationError(std::string message)
        : AppError(ErrorCode::Validation, std::move(message)) {}
};

class DatabaseError : public AppError {
public:
    explicit DatabaseError(std::string message)
        : AppError(ErrorCode::Database, std::move(message)) {}
};

class NotFoundError : public AppError {
public:
    explicit NotFoundError(std::string message)
        : AppError(ErrorCode::NotFound, std::move(message)) {}
};

class BookNotFoundError : public NotFoundError {
public:
    explicit BookNotFoundError(int book_id)
        : NotFoundError("Book not found. id=" + std::to_string(book_id)) {}
};

class CopyUnavailableError : public AppError {
public:
    explicit CopyUnavailableError(int copy_id)
        : AppError(ErrorCode::Copy, "Copy is unavailable. copy_id=" + std::to_string(copy_id)) {}
};

class ReaderBlockedError : public AppError {
public:
    explicit ReaderBlockedError(int reader_id)
        : AppError(ErrorCode::Reader, "Reader is blocked. reader_id=" + std::to_string(reader_id)) {}
};

class LoanError : public AppError {
public:
    explicit LoanError(std::string message)
        : AppError(ErrorCode::Loan, std::move(message)) {}
};

class InventoryError : public AppError {
public:
    explicit InventoryError(std::string message)
        : AppError(ErrorCode::Inventory, std::move(message)) {}
};

class ReportError : public AppError {
public:
    explicit ReportError(std::string message)
        : AppError(ErrorCode::Report, std::move(message)) {}
};

} // namespace errors
