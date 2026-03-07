#pragma once

#include "audit/audit_repository.hpp"
#include "db.hpp"

namespace audit {

class SqliteAuditRepository final : public AuditRepository {
public:
    explicit SqliteAuditRepository(Db& db) : db_(db) {}

    AuditEvent create(const AuditEvent& event) override;
    std::vector<AuditEvent> list_recent(int limit = 100) const override;
    std::vector<AuditEvent> list_for_object(const std::string& object_type,
                                            const std::string& object_public_id,
                                            int limit = 100) const override;

private:
    Db& db_;
};

} // namespace audit
