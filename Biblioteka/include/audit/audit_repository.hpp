#pragma once

#include "audit/audit_event.hpp"

#include <string>
#include <vector>

namespace audit {

class AuditRepository {
public:
    virtual ~AuditRepository() = default;

    virtual AuditEvent create(const AuditEvent& event) = 0;
    virtual std::vector<AuditEvent> list_recent(int limit = 100) const = 0;
    virtual std::vector<AuditEvent> list_for_object(const std::string& object_type,
                                                    const std::string& object_public_id,
                                                    int limit = 100) const = 0;
};

} // namespace audit
