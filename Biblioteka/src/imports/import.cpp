#include "imports/import.hpp"

#include "errors/app_error.hpp"

namespace imports {

std::string to_string(ImportFormat format) {
    switch (format) {
        case ImportFormat::Csv:
            return "CSV";
        case ImportFormat::Excel:
            return "EXCEL";
        case ImportFormat::Database:
            return "DATABASE";
        default:
            throw errors::ValidationError("Unsupported import format");
    }
}

ImportFormat import_format_from_string(const std::string& raw) {
    if (raw == "CSV") {
        return ImportFormat::Csv;
    }
    if (raw == "EXCEL") {
        return ImportFormat::Excel;
    }
    if (raw == "DATABASE") {
        return ImportFormat::Database;
    }

    throw errors::ValidationError("Unsupported import format: " + raw);
}

std::string to_string(ImportTarget target) {
    switch (target) {
        case ImportTarget::Books:
            return "BOOKS";
        case ImportTarget::Readers:
            return "READERS";
        default:
            throw errors::ValidationError("Unsupported import target");
    }
}

ImportTarget import_target_from_string(const std::string& raw) {
    if (raw == "BOOKS") {
        return ImportTarget::Books;
    }
    if (raw == "READERS") {
        return ImportTarget::Readers;
    }

    throw errors::ValidationError("Unsupported import target: " + raw);
}

std::string to_string(ImportStatus status) {
    switch (status) {
        case ImportStatus::InProgress:
            return "IN_PROGRESS";
        case ImportStatus::Completed:
            return "COMPLETED";
        case ImportStatus::CompletedWithErrors:
            return "COMPLETED_WITH_ERRORS";
        case ImportStatus::Failed:
            return "FAILED";
        default:
            throw errors::ValidationError("Unsupported import status");
    }
}

ImportStatus import_status_from_string(const std::string& raw) {
    if (raw == "IN_PROGRESS") {
        return ImportStatus::InProgress;
    }
    if (raw == "COMPLETED") {
        return ImportStatus::Completed;
    }
    if (raw == "COMPLETED_WITH_ERRORS") {
        return ImportStatus::CompletedWithErrors;
    }
    if (raw == "FAILED") {
        return ImportStatus::Failed;
    }

    throw errors::ValidationError("Unsupported import status: " + raw);
}

} // namespace imports
