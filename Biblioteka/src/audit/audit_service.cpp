#include "audit/audit_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace audit {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[audit] " << message << '\n';
}

} // namespace

AuditService::AuditService(AuditRepository& repository, OperationLogger logger)
    : repository_(repository), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

AuditEvent AuditService::log_event(const AuditLogInput& input) {
    AuditEvent event;
    event.module = input.module;
    event.actor = normalize_text(input.actor);
    event.object_type = normalize_text(input.object_type);
    event.object_public_id = normalize_text(input.object_public_id);
    event.operation_type = normalize_text(input.operation_type);
    event.details = normalize_text(input.details);

    if (event.actor.empty()) {
        throw errors::ValidationError("audit actor is required");
    }
    if (event.object_type.empty()) {
        throw errors::ValidationError("audit object_type is required");
    }
    if (event.object_public_id.empty()) {
        throw errors::ValidationError("audit object_public_id is required");
    }
    if (event.operation_type.empty()) {
        throw errors::ValidationError("audit operation_type is required");
    }

    const AuditEvent created = repository_.create(event);
    logger_("event logged module=" + to_string(created.module) + " object=" + created.object_type +
            " object_id=" + created.object_public_id + " op=" + created.operation_type);
    return created;
}

std::vector<AuditEvent> AuditService::get_recent_events(int limit) const {
    if (limit <= 0) {
        throw errors::ValidationError("audit limit must be greater than zero");
    }
    return repository_.list_recent(limit);
}

std::vector<AuditEvent> AuditService::get_events_for_object(const std::string& object_type,
                                                            const std::string& object_public_id,
                                                            int limit) const {
    const std::string normalized_type = normalize_text(object_type);
    const std::string normalized_id = normalize_text(object_public_id);

    if (normalized_type.empty()) {
        throw errors::ValidationError("audit object_type is required");
    }
    if (normalized_id.empty()) {
        throw errors::ValidationError("audit object_public_id is required");
    }
    if (limit <= 0) {
        throw errors::ValidationError("audit limit must be greater than zero");
    }

    return repository_.list_for_object(normalized_type, normalized_id, limit);
}

std::string AuditService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

} // namespace audit
