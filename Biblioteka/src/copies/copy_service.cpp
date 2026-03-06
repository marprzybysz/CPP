#include "copies/copy_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace copies {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[copies] " << message << '\n';
}

} // namespace

CopyService::CopyService(CopyRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

BookCopy CopyService::add_copy(const CreateCopyInput& input) {
    if (input.book_id <= 0) {
        throw errors::ValidationError("book_id must be greater than zero");
    }
    if (!repository_.book_exists(input.book_id)) {
        throw errors::BookNotFoundError(input.book_id);
    }

    BookCopy copy;
    copy.book_id = input.book_id;
    copy.inventory_number = normalize_text(input.inventory_number);
    copy.status = input.status;
    copy.target_location_id = input.target_location_id;
    copy.current_location_id = input.current_location_id;
    copy.condition_note = normalize_text(input.condition_note);
    copy.acquisition_date = input.acquisition_date;

    if (copy.inventory_number.empty()) {
        throw errors::ValidationError("inventory_number is required");
    }
    if (repository_.inventory_number_exists(copy.inventory_number)) {
        throw errors::ValidationError("inventory_number must be unique");
    }

    const int year = common::current_year();
    ensure_sequence_initialized(year);
    copy.public_id = id_generator_.generate_for_year(common::IdType::Copy, year);

    BookCopy created = repository_.create(copy);
    logger_("copy created public_id=" + created.public_id + " book_id=" + std::to_string(created.book_id));
    return created;
}

BookCopy CopyService::edit_copy(const std::string& public_id, const UpdateCopyInput& input) {
    BookCopy copy = get_copy(public_id);

    if (input.inventory_number.has_value()) {
        copy.inventory_number = normalize_text(*input.inventory_number);
    }
    if (input.condition_note.has_value()) {
        copy.condition_note = normalize_text(*input.condition_note);
    }
    if (input.acquisition_date.has_value()) {
        copy.acquisition_date = input.acquisition_date;
    }
    if (input.archival_reason.has_value()) {
        copy.archival_reason = input.archival_reason;
    }

    if (copy.inventory_number.empty()) {
        throw errors::ValidationError("inventory_number is required");
    }
    if (repository_.inventory_number_exists(copy.inventory_number, &copy.public_id)) {
        throw errors::ValidationError("inventory_number must be unique");
    }

    BookCopy updated = repository_.update(copy);
    logger_("copy updated public_id=" + updated.public_id);
    return updated;
}

BookCopy CopyService::get_copy(const std::string& public_id) const {
    const auto found = repository_.get_by_public_id(public_id);
    if (!found.has_value()) {
        throw errors::NotFoundError("Copy not found. public_id=" + public_id);
    }
    return *found;
}

std::vector<BookCopy> CopyService::list_book_copies(int book_id, int limit, int offset) const {
    if (book_id <= 0) {
        throw errors::ValidationError("book_id must be greater than zero");
    }
    if (limit <= 0) {
        throw errors::ValidationError("limit must be greater than zero");
    }
    if (offset < 0) {
        throw errors::ValidationError("offset cannot be negative");
    }

    return repository_.list_by_book_id(book_id, limit, offset);
}

BookCopy CopyService::change_status(const std::string& public_id,
                                    CopyStatus new_status,
                                    const std::string& note,
                                    const std::optional<std::string>& archival_reason) {
    const BookCopy copy = get_copy(public_id);
    if (!is_transition_allowed(copy.status, new_status)) {
        throw errors::ValidationError("Invalid status transition from " + to_string(copy.status) + " to " +
                                      to_string(new_status));
    }

    std::optional<std::string> resolved_archival_reason = copy.archival_reason;
    if (new_status == CopyStatus::Archived) {
        if (!archival_reason.has_value() || normalize_text(*archival_reason).empty()) {
            throw errors::ValidationError("archival_reason is required when status is ARCHIVED");
        }
        resolved_archival_reason = normalize_text(*archival_reason);
    }

    if (new_status == CopyStatus::OnShelf) {
        resolved_archival_reason = std::nullopt;
    }

    BookCopy updated = repository_.update_status(public_id, new_status, normalize_text(note), resolved_archival_reason);
    logger_("copy status changed public_id=" + updated.public_id + " to=" + to_string(new_status));
    return updated;
}

BookCopy CopyService::change_location(const std::string& public_id,
                                      const std::optional<std::string>& current_location_id,
                                      const std::optional<std::string>& target_location_id,
                                      const std::string& note) {
    std::optional<std::string> current = current_location_id;
    std::optional<std::string> target = target_location_id;

    if (current.has_value()) {
        *current = normalize_text(*current);
        if (current->empty()) {
            current = std::nullopt;
        }
    }

    if (target.has_value()) {
        *target = normalize_text(*target);
        if (target->empty()) {
            target = std::nullopt;
        }
    }

    BookCopy updated = repository_.update_location(public_id, current, target, normalize_text(note));
    logger_("copy location changed public_id=" + updated.public_id);
    return updated;
}

bool CopyService::is_transition_allowed(CopyStatus from_status, CopyStatus to_status) {
    if (from_status == to_status) {
        return true;
    }

    switch (from_status) {
        case CopyStatus::OnShelf:
            return to_status == CopyStatus::Loaned || to_status == CopyStatus::Reserved ||
                   to_status == CopyStatus::InRepair || to_status == CopyStatus::Archived ||
                   to_status == CopyStatus::Lost;
        case CopyStatus::Loaned:
            return to_status == CopyStatus::OnShelf || to_status == CopyStatus::Lost;
        case CopyStatus::Reserved:
            return to_status == CopyStatus::Loaned || to_status == CopyStatus::OnShelf ||
                   to_status == CopyStatus::Archived;
        case CopyStatus::InRepair:
            return to_status == CopyStatus::OnShelf || to_status == CopyStatus::Archived ||
                   to_status == CopyStatus::Lost;
        case CopyStatus::Archived:
            return false;
        case CopyStatus::Lost:
            return false;
        default:
            return false;
    }
}

std::string CopyService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

void CopyService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Copy, year, next);
    initialized_years_.insert(year);
}

} // namespace copies
