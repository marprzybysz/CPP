#pragma once

#include "exports/export_repository.hpp"

#include <functional>
#include <string>

namespace exports {

class ExportService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    explicit ExportService(ExportRepository& repository, OperationLogger logger = {});

    CopyWithdrawal withdraw_copy(const WithdrawCopyInput& input, copies::CopyStatus resulting_status);
    std::vector<WithdrawnCopyView> list_withdrawn_copies(int limit = 100, int offset = 0) const;

private:
    static std::string normalize_text(std::string value);

    ExportRepository& repository_;
    OperationLogger logger_;
};

} // namespace exports
