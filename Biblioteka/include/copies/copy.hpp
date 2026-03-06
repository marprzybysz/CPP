#pragma once

#include <optional>
#include <string>
#include <vector>

namespace copies {

enum class CopyStatus {
    OnShelf,
    Loaned,
    Reserved,
    InRepair,
    Archived,
    Lost,
};

struct BookCopy {
    std::optional<int> id;
    std::string public_id;
    int book_id{0};
    std::string inventory_number;
    CopyStatus status{CopyStatus::OnShelf};
    std::optional<std::string> target_location_id;
    std::optional<std::string> current_location_id;
    std::string condition_note;
    std::optional<std::string> acquisition_date;
    std::optional<std::string> archival_reason;
    std::string created_at;
    std::string updated_at;
};

struct CreateCopyInput {
    int book_id{0};
    std::string inventory_number;
    CopyStatus status{CopyStatus::OnShelf};
    std::optional<std::string> target_location_id;
    std::optional<std::string> current_location_id;
    std::string condition_note;
    std::optional<std::string> acquisition_date;
};

struct UpdateCopyInput {
    std::optional<std::string> inventory_number;
    std::optional<std::string> condition_note;
    std::optional<std::string> acquisition_date;
    std::optional<std::string> archival_reason;
};

struct StatusHistoryEntry {
    std::optional<int> id;
    std::string copy_public_id;
    CopyStatus from_status{CopyStatus::OnShelf};
    CopyStatus to_status{CopyStatus::OnShelf};
    std::string changed_at;
    std::string note;
};

struct LocationHistoryEntry {
    std::optional<int> id;
    std::string copy_public_id;
    std::optional<std::string> from_location_id;
    std::optional<std::string> to_location_id;
    std::optional<std::string> from_target_location_id;
    std::optional<std::string> to_target_location_id;
    std::string changed_at;
    std::string note;
};

[[nodiscard]] std::string to_string(CopyStatus status);
[[nodiscard]] CopyStatus copy_status_from_string(const std::string& raw_status);

} // namespace copies
