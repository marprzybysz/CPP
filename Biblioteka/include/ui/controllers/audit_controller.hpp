#pragma once

#include <optional>
#include <string>
#include <vector>

#include "audit/audit_event.hpp"

class Library;

namespace ui::controllers {

struct AuditFilterState {
    std::optional<std::string> operation_type;
    std::optional<std::string> date;
    std::optional<audit::AuditModule> module;
    int limit{300};
};

class AuditController {
public:
    explicit AuditController(Library& library);

    [[nodiscard]] std::vector<audit::AuditEvent> list_events(const AuditFilterState& filter) const;

private:
    Library& library_;
};

} // namespace ui::controllers
