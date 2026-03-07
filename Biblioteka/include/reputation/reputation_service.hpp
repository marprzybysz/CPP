#pragma once

#include "reputation/reputation_repository.hpp"

#include <functional>
#include <string>
#include <vector>

namespace reputation {

class ReputationService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    ReputationService(ReputationRepository& repository, ReputationConfig config = {}, OperationLogger logger = {});

    int get_current_reputation(int reader_id) const;
    std::vector<ReputationChange> get_reputation_history(int reader_id, int limit = 100, int offset = 0) const;

    ReputationChange apply_manual_adjustment(int reader_id,
                                             int change_value,
                                             const std::string& reason,
                                             const std::optional<int>& related_loan_id = std::nullopt);

    ReputationChange apply_on_loan_return(const LoanReturnEvent& event);

private:
    static int full_weeks_late(const std::string& due_date, const std::string& return_date);
    static std::string trim(std::string value);

    ReputationRepository& repository_;
    ReputationConfig config_;
    OperationLogger logger_;
};

} // namespace reputation
