#pragma once

#include "common/system_id.hpp"
#include "copies/copy_repository.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace copies {

class CopyService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    CopyService(CopyRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    BookCopy add_copy(const CreateCopyInput& input);
    BookCopy edit_copy(const std::string& public_id, const UpdateCopyInput& input);
    BookCopy get_copy(const std::string& public_id) const;
    std::vector<BookCopy> list_book_copies(int book_id, int limit = 100, int offset = 0) const;
    BookCopy change_status(const std::string& public_id,
                           CopyStatus new_status,
                           const std::string& note = "",
                           const std::optional<std::string>& archival_reason = std::nullopt);
    BookCopy change_location(const std::string& public_id,
                             const std::optional<std::string>& current_location_id,
                             const std::optional<std::string>& target_location_id,
                             const std::string& note = "");

private:
    static bool is_transition_allowed(CopyStatus from_status, CopyStatus to_status);
    static std::string normalize_text(std::string value);

    void ensure_sequence_initialized(int year);

    CopyRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace copies
