#include "exports/export.hpp"

#include "errors/app_error.hpp"

namespace exports {

std::string to_string(WithdrawalReason reason) {
    switch (reason) {
        case WithdrawalReason::Damage:
            return "DAMAGE";
        case WithdrawalReason::NoBorrowings:
            return "NO_BORROWINGS";
        case WithdrawalReason::Duplicates:
            return "DUPLICATES";
        case WithdrawalReason::OutdatedContent:
            return "OUTDATED_CONTENT";
        case WithdrawalReason::Lost:
            return "LOST";
        default:
            throw errors::ValidationError("Unsupported withdrawal reason");
    }
}

WithdrawalReason withdrawal_reason_from_string(const std::string& raw) {
    if (raw == "DAMAGE") {
        return WithdrawalReason::Damage;
    }
    if (raw == "NO_BORROWINGS") {
        return WithdrawalReason::NoBorrowings;
    }
    if (raw == "DUPLICATES") {
        return WithdrawalReason::Duplicates;
    }
    if (raw == "OUTDATED_CONTENT") {
        return WithdrawalReason::OutdatedContent;
    }
    if (raw == "LOST") {
        return WithdrawalReason::Lost;
    }

    throw errors::ValidationError("Unsupported withdrawal reason: " + raw);
}

} // namespace exports
