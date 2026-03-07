#pragma once

#include "copies/copy.hpp"

#include <optional>
#include <string>
#include <vector>

namespace exports {

enum class WithdrawalReason {
    Damage,
    NoBorrowings,
    Duplicates,
    OutdatedContent,
    Lost,
};

struct CopyWithdrawal {
    std::optional<int> id;
    std::string copy_public_id;
    WithdrawalReason reason{WithdrawalReason::Damage};
    std::string withdrawal_date;
    std::string operator_name;
    std::string note;
    copies::CopyStatus resulting_status{copies::CopyStatus::Archived};
    std::string created_at;
};

struct WithdrawCopyInput {
    std::string copy_public_id;
    WithdrawalReason reason{WithdrawalReason::Damage};
    std::optional<std::string> withdrawal_date;
    std::string operator_name;
    std::string note;
};

struct WithdrawnCopyView {
    CopyWithdrawal withdrawal;
    std::string inventory_number;
    int book_id{0};
    copies::CopyStatus current_status{copies::CopyStatus::OnShelf};
};

[[nodiscard]] std::string to_string(WithdrawalReason reason);
[[nodiscard]] WithdrawalReason withdrawal_reason_from_string(const std::string& raw);

} // namespace exports
