#pragma once

#include "db.hpp"
#include "reputation/reputation_repository.hpp"

namespace reputation {

class SqliteReputationRepository final : public ReputationRepository {
public:
    explicit SqliteReputationRepository(Db& db) : db_(db) {}

    ReaderReputationSnapshot get_reader_snapshot(int reader_id) const override;

    int update_reader_reputation(int reader_id,
                                 int new_reputation_points,
                                 bool is_blocked,
                                 const std::optional<std::string>& block_reason,
                                 bool set_account_suspended) override;

    ReputationChange create_change(const ReputationChange& change) override;
    std::vector<ReputationChange> list_changes(int reader_id, int limit = 100, int offset = 0) const override;

private:
    Db& db_;
};

} // namespace reputation
