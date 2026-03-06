#pragma once

#include "db.hpp"
#include "readers/reader_repository.hpp"

namespace readers {

class SqliteReaderRepository final : public ReaderRepository {
public:
    explicit SqliteReaderRepository(Db& db) : db_(db) {}

    Reader create(const Reader& reader) override;
    Reader update(const Reader& reader) override;

    std::optional<Reader> get_by_public_id(const std::string& public_id) const override;
    std::optional<Reader> get_by_card_number(const std::string& card_number) const override;
    std::vector<Reader> search(const ReaderQuery& query) const override;

    bool email_exists(const std::string& email, const std::string* excluded_public_id = nullptr) const override;
    bool card_number_exists(const std::string& card_number, const std::string* excluded_public_id = nullptr) const override;

    Reader set_block_state(const std::string& public_id,
                           bool is_blocked,
                           const std::optional<std::string>& block_reason,
                           AccountStatus account_status) override;

    std::uint64_t next_card_sequence() const override;

    std::vector<ReaderLoanHistoryEntry> list_loan_history(const std::string& reader_public_id) const override;
    std::vector<ReaderNote> list_notes(const std::string& reader_public_id) const override;

private:
    Db& db_;
};

} // namespace readers
