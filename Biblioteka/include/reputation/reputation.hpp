#pragma once

#include <optional>
#include <string>

namespace reputation {

struct ReputationChange {
    std::optional<int> id;
    int reader_id{0};
    int change_value{0};
    std::string reason;
    std::optional<int> related_loan_id;
    std::string created_at;
};

struct LoanReturnEvent {
    int reader_id{0};
    std::optional<int> related_loan_id;
    std::string due_date;
    std::string return_date;
    bool was_extended{false};
};

struct ReputationConfig {
    int on_time_return_bonus{5};
    int extended_return_bonus{1};
    int late_penalty_per_full_week{-4};
    int auto_block_threshold{-20};
    std::string auto_block_reason{"Auto-block: low reputation"};
};

} // namespace reputation
