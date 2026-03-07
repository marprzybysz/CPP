#pragma once

#include "exports/export.hpp"

#include <optional>
#include <string>
#include <vector>

namespace exports {

class ExportRepository {
public:
    virtual ~ExportRepository() = default;

    virtual bool withdrawal_exists_for_copy(const std::string& copy_public_id) const = 0;

    virtual CopyWithdrawal create_withdrawal(const CopyWithdrawal& withdrawal) = 0;
    virtual std::vector<WithdrawnCopyView> list_withdrawn_copies(int limit = 100, int offset = 0) const = 0;
};

} // namespace exports
