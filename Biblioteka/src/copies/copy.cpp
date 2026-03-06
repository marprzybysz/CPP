#include "copies/copy.hpp"

#include "errors/app_error.hpp"

namespace copies {

std::string to_string(CopyStatus status) {
    switch (status) {
        case CopyStatus::OnShelf:
            return "ON_SHELF";
        case CopyStatus::Loaned:
            return "LOANED";
        case CopyStatus::Reserved:
            return "RESERVED";
        case CopyStatus::InRepair:
            return "IN_REPAIR";
        case CopyStatus::Archived:
            return "ARCHIVED";
        case CopyStatus::Lost:
            return "LOST";
        default:
            throw errors::ValidationError("Unknown copy status");
    }
}

CopyStatus copy_status_from_string(const std::string& raw_status) {
    if (raw_status == "ON_SHELF") {
        return CopyStatus::OnShelf;
    }
    if (raw_status == "LOANED") {
        return CopyStatus::Loaned;
    }
    if (raw_status == "RESERVED") {
        return CopyStatus::Reserved;
    }
    if (raw_status == "IN_REPAIR") {
        return CopyStatus::InRepair;
    }
    if (raw_status == "ARCHIVED") {
        return CopyStatus::Archived;
    }
    if (raw_status == "LOST") {
        return CopyStatus::Lost;
    }

    throw errors::ValidationError("Unsupported copy status: " + raw_status);
}

} // namespace copies
