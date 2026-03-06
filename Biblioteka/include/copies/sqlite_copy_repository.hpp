#pragma once

#include "copies/copy_repository.hpp"
#include "db.hpp"

namespace copies {

class SqliteCopyRepository final : public CopyRepository {
public:
    explicit SqliteCopyRepository(Db& db) : db_(db) {}

    bool book_exists(int book_id) const override;
    bool location_exists(const std::string& location_public_id) const override;
    bool inventory_number_exists(const std::string& inventory_number, const std::string* excluded_public_id = nullptr) const override;

    BookCopy create(const BookCopy& copy) override;
    BookCopy update(const BookCopy& copy) override;

    std::optional<BookCopy> get_by_public_id(const std::string& public_id) const override;
    std::vector<BookCopy> list_by_book_id(int book_id, int limit = 100, int offset = 0) const override;

    BookCopy update_status(const std::string& public_id,
                           CopyStatus new_status,
                           const std::string& note,
                           const std::optional<std::string>& archival_reason) override;

    BookCopy update_location(const std::string& public_id,
                             const std::optional<std::string>& current_location_id,
                             const std::optional<std::string>& target_location_id,
                             const std::string& note) override;

    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace copies
