#include "readers/reader_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <regex>
#include <utility>

namespace readers {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[readers] " << message << '\n';
}

std::string lower_copy(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return input;
}

} // namespace

ReaderService::ReaderService(ReaderRepository& repository,
                             common::SystemIdGenerator& id_generator,
                             OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

Reader ReaderService::add_reader(const CreateReaderInput& input) {
    Reader reader;
    reader.first_name = normalize_text(input.first_name);
    reader.last_name = normalize_text(input.last_name);
    reader.email = lower_copy(normalize_text(input.email));
    reader.phone = input.phone.has_value() ? std::optional<std::string>(normalize_text(*input.phone)) : std::nullopt;
    reader.gdpr_consent = input.gdpr_consent;

    if (reader.first_name.empty() || reader.last_name.empty()) {
        throw errors::ValidationError("Reader first_name and last_name are required");
    }
    if (!is_valid_email(reader.email)) {
        throw errors::ValidationError("Reader email is invalid");
    }
    if (repository_.email_exists(reader.email)) {
        throw errors::ValidationError("Reader email must be unique");
    }

    ensure_card_sequence_initialized();
    reader.card_number = id_generator_.generate(common::IdType::Card);
    if (repository_.card_number_exists(reader.card_number)) {
        throw errors::ValidationError("Reader card_number must be unique");
    }

    reader.public_id = "READER-" + reader.card_number.substr(5);
    reader.account_status = AccountStatus::Active;
    reader.reputation_points = 0;
    reader.is_blocked = false;
    reader.block_reason = std::nullopt;

    Reader created = repository_.create(reader);
    logger_("reader created public_id=" + created.public_id + " card_number=" + created.card_number);
    return created;
}

Reader ReaderService::edit_reader(const std::string& public_id, const UpdateReaderInput& input) {
    Reader reader = get_reader_details(public_id);

    if (input.first_name.has_value()) {
        reader.first_name = normalize_text(*input.first_name);
    }
    if (input.last_name.has_value()) {
        reader.last_name = normalize_text(*input.last_name);
    }
    if (input.email.has_value()) {
        reader.email = lower_copy(normalize_text(*input.email));
    }
    if (input.phone.has_value()) {
        const std::string normalized = normalize_text(*input.phone);
        reader.phone = normalized.empty() ? std::nullopt : std::optional<std::string>(normalized);
    }
    if (input.account_status.has_value()) {
        reader.account_status = *input.account_status;
    }
    if (input.reputation_points.has_value()) {
        reader.reputation_points = *input.reputation_points;
    }
    if (input.gdpr_consent.has_value()) {
        reader.gdpr_consent = *input.gdpr_consent;
    }

    if (reader.first_name.empty() || reader.last_name.empty()) {
        throw errors::ValidationError("Reader first_name and last_name are required");
    }
    if (!is_valid_email(reader.email)) {
        throw errors::ValidationError("Reader email is invalid");
    }
    if (repository_.email_exists(reader.email, &reader.public_id)) {
        throw errors::ValidationError("Reader email must be unique");
    }

    if (reader.reputation_points < 0) {
        throw errors::ValidationError("Reader reputation_points cannot be negative");
    }

    Reader updated = repository_.update(reader);
    logger_("reader updated public_id=" + updated.public_id);
    return updated;
}

std::vector<Reader> ReaderService::search_readers(const ReaderQuery& query) const {
    if (query.limit <= 0) {
        throw errors::ValidationError("Reader search limit must be greater than zero");
    }
    if (query.offset < 0) {
        throw errors::ValidationError("Reader search offset cannot be negative");
    }

    ReaderQuery normalized = query;
    if (normalized.text.has_value()) {
        normalized.text = normalize_text(*normalized.text);
    }
    if (normalized.card_number.has_value()) {
        normalized.card_number = normalize_text(*normalized.card_number);
    }
    if (normalized.email.has_value()) {
        normalized.email = lower_copy(normalize_text(*normalized.email));
    }
    if (normalized.last_name.has_value()) {
        normalized.last_name = normalize_text(*normalized.last_name);
    }

    return repository_.search(normalized);
}

Reader ReaderService::get_reader_details(const std::string& public_id) const {
    const auto found = repository_.get_by_public_id(public_id);
    if (!found.has_value()) {
        throw errors::NotFoundError("Reader not found. public_id=" + public_id);
    }
    return *found;
}

Reader ReaderService::block_account(const std::string& public_id, const std::string& reason) {
    const Reader reader = get_reader_details(public_id);
    if (reader.is_blocked) {
        if (reader.id.has_value()) {
            throw errors::ReaderBlockedError(*reader.id);
        }
        throw errors::ValidationError("Reader already blocked");
    }

    const std::string normalized_reason = normalize_text(reason);
    if (normalized_reason.empty()) {
        throw errors::ValidationError("Block reason is required");
    }

    Reader updated = repository_.set_block_state(public_id, true, normalized_reason, AccountStatus::Suspended);
    logger_("reader blocked public_id=" + updated.public_id + " reason=" + normalized_reason);
    return updated;
}

Reader ReaderService::unblock_account(const std::string& public_id) {
    const Reader reader = get_reader_details(public_id);
    if (!reader.is_blocked) {
        return reader;
    }

    Reader updated = repository_.set_block_state(public_id, false, std::nullopt, AccountStatus::Active);
    logger_("reader unblocked public_id=" + updated.public_id);
    return updated;
}

bool ReaderService::is_valid_email(const std::string& email) {
    static const std::regex pattern(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, pattern);
}

std::string ReaderService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

void ReaderService::ensure_card_sequence_initialized() {
    if (is_card_sequence_initialized_) {
        return;
    }

    const std::uint64_t next = repository_.next_card_sequence();
    id_generator_.set_next_sequence(common::IdType::Card, next);
    is_card_sequence_initialized_ = true;
}

} // namespace readers
