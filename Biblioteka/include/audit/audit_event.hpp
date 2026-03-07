#pragma once

#include <optional>
#include <string>

namespace audit {

enum class AuditModule {
    System,
    Books,
    Copies,
    Readers,
    Loans,
    Inventory,
    Supply,
    Export,
    Import,
};

struct AuditEvent {
    std::optional<int> id;
    AuditModule module{AuditModule::System};
    std::string actor;
    std::string occurred_at;
    std::string object_type;
    std::string object_public_id;
    std::string operation_type;
    std::string details;
};

struct AuditLogInput {
    AuditModule module{AuditModule::System};
    std::string actor;
    std::string object_type;
    std::string object_public_id;
    std::string operation_type;
    std::string details;
};

[[nodiscard]] std::string to_string(AuditModule module);
[[nodiscard]] AuditModule audit_module_from_string(const std::string& raw);

} // namespace audit
