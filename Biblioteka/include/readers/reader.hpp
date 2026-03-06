#pragma once

#include <optional>
#include <string>
#include <vector>

namespace readers {

enum class AccountStatus {
    Active,
    Suspended,
    Closed,
};

struct Reader {
    std::optional<int> id;
    std::string public_id;
    std::string card_number;
    std::string first_name;
    std::string last_name;
    std::string email;
    std::optional<std::string> phone;
    AccountStatus account_status{AccountStatus::Active};
    int reputation_points{0};
    bool is_blocked{false};
    std::optional<std::string> block_reason;
    bool gdpr_consent{false};
    std::string created_at;
    std::string updated_at;
};

struct CreateReaderInput {
    std::string first_name;
    std::string last_name;
    std::string email;
    std::optional<std::string> phone;
    bool gdpr_consent{false};
};

struct UpdateReaderInput {
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::optional<std::string> email;
    std::optional<std::string> phone;
    std::optional<AccountStatus> account_status;
    std::optional<int> reputation_points;
    std::optional<bool> gdpr_consent;
};

struct ReaderQuery {
    std::optional<std::string> text;
    std::optional<std::string> card_number;
    std::optional<std::string> email;
    std::optional<std::string> last_name;
    int limit{100};
    int offset{0};
};

struct ReaderLoanHistoryEntry {
    std::optional<int> id;
    std::string reader_public_id;
    std::string loan_public_id;
    std::string action;
    std::string action_at;
    std::string note;
};

struct ReaderNote {
    std::optional<int> id;
    std::string reader_public_id;
    std::string note;
    std::string created_at;
};

[[nodiscard]] std::string to_string(AccountStatus status);
[[nodiscard]] AccountStatus account_status_from_string(const std::string& raw);

} // namespace readers
