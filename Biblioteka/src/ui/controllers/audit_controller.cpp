#include "ui/controllers/audit_controller.hpp"

#include <algorithm>
#include <cctype>

#include "library.hpp"

namespace {

std::string lower_copy(std::string raw) {
    std::transform(raw.begin(), raw.end(), raw.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return raw;
}

} // namespace

namespace ui::controllers {

AuditController::AuditController(Library& library) : library_(library) {}

std::vector<audit::AuditEvent> AuditController::list_events(const AuditFilterState& filter) const {
    auto events = library_.get_recent_audit_events(filter.limit);
    std::vector<audit::AuditEvent> out;
    out.reserve(events.size());

    const auto normalized_op = filter.operation_type.has_value() ? std::optional<std::string>(lower_copy(*filter.operation_type))
                                                                 : std::nullopt;

    for (const auto& event : events) {
        if (filter.module.has_value() && event.module != *filter.module) {
            continue;
        }

        if (filter.date.has_value() && event.occurred_at.rfind(*filter.date, 0) != 0) {
            continue;
        }

        if (normalized_op.has_value()) {
            const std::string op = lower_copy(event.operation_type);
            if (op.find(*normalized_op) == std::string::npos) {
                continue;
            }
        }

        out.push_back(event);
    }

    return out;
}

} // namespace ui::controllers
