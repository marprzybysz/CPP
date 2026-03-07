#pragma once

#include "db.hpp"
#include "exports/export_repository.hpp"

namespace exports {

class SqliteExportRepository final : public ExportRepository {
public:
    explicit SqliteExportRepository(Db& db) : db_(db) {}

    bool withdrawal_exists_for_copy(const std::string& copy_public_id) const override;

    CopyWithdrawal create_withdrawal(const CopyWithdrawal& withdrawal) override;
    std::vector<WithdrawnCopyView> list_withdrawn_copies(int limit = 100, int offset = 0) const override;

private:
    Db& db_;
};

} // namespace exports
