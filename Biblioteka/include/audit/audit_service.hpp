#pragma once

#include "audit/audit_repository.hpp"

#include <functional>
#include <string>
#include <vector>

namespace audit {

class AuditService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    explicit AuditService(AuditRepository& repository, OperationLogger logger = {});

    AuditEvent log_event(const AuditLogInput& input);
    std::vector<AuditEvent> get_recent_events(int limit = 100) const;
    std::vector<AuditEvent> get_events_for_object(const std::string& object_type,
                                                  const std::string& object_public_id,
                                                  int limit = 100) const;

private:
    static std::string normalize_text(std::string value);

    AuditRepository& repository_;
    OperationLogger logger_;
};

} // namespace audit
