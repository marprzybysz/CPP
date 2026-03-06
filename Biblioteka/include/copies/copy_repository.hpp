#pragma once

#include "copies/copy.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace copies {

class CopyRepository {
public:
    virtual ~CopyRepository() = default;

    virtual bool book_exists(int book_id) const = 0;
    virtual bool inventory_number_exists(const std::string& inventory_number, const std::string* excluded_public_id = nullptr) const = 0;

    virtual BookCopy create(const BookCopy& copy) = 0;
    virtual BookCopy update(const BookCopy& copy) = 0;

    virtual std::optional<BookCopy> get_by_public_id(const std::string& public_id) const = 0;
    virtual std::vector<BookCopy> list_by_book_id(int book_id, int limit = 100, int offset = 0) const = 0;

    virtual BookCopy update_status(const std::string& public_id,
                                   CopyStatus new_status,
                                   const std::string& note,
                                   const std::optional<std::string>& archival_reason) = 0;

    virtual BookCopy update_location(const std::string& public_id,
                                     const std::optional<std::string>& current_location_id,
                                     const std::optional<std::string>& target_location_id,
                                     const std::string& note) = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace copies
