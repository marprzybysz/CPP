#pragma once

#include "readers/reader.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace readers {

class ReaderRepository {
public:
    virtual ~ReaderRepository() = default;

    virtual Reader create(const Reader& reader) = 0;
    virtual Reader update(const Reader& reader) = 0;

    virtual std::optional<Reader> get_by_public_id(const std::string& public_id) const = 0;
    virtual std::optional<Reader> get_by_card_number(const std::string& card_number) const = 0;
    virtual std::vector<Reader> search(const ReaderQuery& query) const = 0;

    virtual bool email_exists(const std::string& email, const std::string* excluded_public_id = nullptr) const = 0;
    virtual bool card_number_exists(const std::string& card_number, const std::string* excluded_public_id = nullptr) const = 0;

    virtual Reader set_block_state(const std::string& public_id,
                                   bool is_blocked,
                                   const std::optional<std::string>& block_reason,
                                   AccountStatus account_status) = 0;

    virtual std::uint64_t next_card_sequence() const = 0;

    virtual std::vector<ReaderLoanHistoryEntry> list_loan_history(const std::string& reader_public_id) const = 0;
    virtual std::vector<ReaderNote> list_notes(const std::string& reader_public_id) const = 0;
};

} // namespace readers
