#include "audit/audit_event.hpp"

#include "errors/app_error.hpp"

namespace audit {

std::string to_string(AuditModule module) {
    switch (module) {
        case AuditModule::System:
            return "SYSTEM";
        case AuditModule::Books:
            return "BOOKS";
        case AuditModule::Copies:
            return "COPIES";
        case AuditModule::Readers:
            return "READERS";
        case AuditModule::Loans:
            return "LOANS";
        case AuditModule::Inventory:
            return "INVENTORY";
        case AuditModule::Supply:
            return "SUPPLY";
        case AuditModule::Export:
            return "EXPORT";
        default:
            throw errors::ValidationError("Unsupported audit module");
    }
}

AuditModule audit_module_from_string(const std::string& raw) {
    if (raw == "SYSTEM") {
        return AuditModule::System;
    }
    if (raw == "BOOKS") {
        return AuditModule::Books;
    }
    if (raw == "COPIES") {
        return AuditModule::Copies;
    }
    if (raw == "READERS") {
        return AuditModule::Readers;
    }
    if (raw == "LOANS") {
        return AuditModule::Loans;
    }
    if (raw == "INVENTORY") {
        return AuditModule::Inventory;
    }
    if (raw == "SUPPLY") {
        return AuditModule::Supply;
    }
    if (raw == "EXPORT") {
        return AuditModule::Export;
    }

    throw errors::ValidationError("Unsupported audit module: " + raw);
}

} // namespace audit
