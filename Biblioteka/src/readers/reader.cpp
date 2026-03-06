#include "readers/reader.hpp"

#include "errors/app_error.hpp"

namespace readers {

std::string to_string(AccountStatus status) {
    switch (status) {
        case AccountStatus::Active:
            return "ACTIVE";
        case AccountStatus::Suspended:
            return "SUSPENDED";
        case AccountStatus::Closed:
            return "CLOSED";
        default:
            throw errors::ValidationError("Unsupported account status");
    }
}

AccountStatus account_status_from_string(const std::string& raw) {
    if (raw == "ACTIVE") {
        return AccountStatus::Active;
    }
    if (raw == "SUSPENDED") {
        return AccountStatus::Suspended;
    }
    if (raw == "CLOSED") {
        return AccountStatus::Closed;
    }

    throw errors::ValidationError("Unsupported account status: " + raw);
}

} // namespace readers
