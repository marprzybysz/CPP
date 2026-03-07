#pragma once

#include "reputation/reputation.hpp"

#include <optional>
#include <vector>

namespace reputation {

struct ReaderReputationSnapshot {
    bool exists{false};
    int reputation_points{0};
    bool is_blocked{false};
};

class ReputationRepository {
public:
    virtual ~ReputationRepository() = default;

    virtual ReaderReputationSnapshot get_reader_snapshot(int reader_id) const = 0;

    virtual int update_reader_reputation(int reader_id,
                                         int new_reputation_points,
                                         bool is_blocked,
                                         const std::optional<std::string>& block_reason,
                                         bool set_account_suspended) = 0;

    virtual ReputationChange create_change(const ReputationChange& change) = 0;
    virtual std::vector<ReputationChange> list_changes(int reader_id, int limit = 100, int offset = 0) const = 0;
};

} // namespace reputation
