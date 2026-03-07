#include "reputation/reputation_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace reputation {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[reputation] " << message << '\n';
}

std::tm parse_date(const std::string& value) {
    std::tm tm{};
    std::istringstream in(value);
    in >> std::get_time(&tm, "%Y-%m-%d");
    if (in.fail()) {
        throw errors::ValidationError("Invalid date format, expected YYYY-MM-DD: " + value);
    }
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return tm;
}

} // namespace

ReputationService::ReputationService(ReputationRepository& repository, ReputationConfig config, OperationLogger logger)
    : repository_(repository), config_(std::move(config)), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

int ReputationService::get_current_reputation(int reader_id) const {
    if (reader_id <= 0) {
        throw errors::ValidationError("reader_id must be greater than zero");
    }

    const ReaderReputationSnapshot snapshot = repository_.get_reader_snapshot(reader_id);
    if (!snapshot.exists) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(reader_id));
    }

    return snapshot.reputation_points;
}

std::vector<ReputationChange> ReputationService::get_reputation_history(int reader_id, int limit, int offset) const {
    if (reader_id <= 0) {
        throw errors::ValidationError("reader_id must be greater than zero");
    }
    if (limit <= 0) {
        throw errors::ValidationError("history limit must be greater than zero");
    }
    if (offset < 0) {
        throw errors::ValidationError("history offset cannot be negative");
    }

    const ReaderReputationSnapshot snapshot = repository_.get_reader_snapshot(reader_id);
    if (!snapshot.exists) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(reader_id));
    }

    return repository_.list_changes(reader_id, limit, offset);
}

ReputationChange ReputationService::apply_manual_adjustment(int reader_id,
                                                            int change_value,
                                                            const std::string& reason,
                                                            const std::optional<int>& related_loan_id) {
    if (reader_id <= 0) {
        throw errors::ValidationError("reader_id must be greater than zero");
    }
    if (change_value == 0) {
        throw errors::ValidationError("manual change_value cannot be zero");
    }

    const std::string normalized_reason = trim(reason);
    if (normalized_reason.empty()) {
        throw errors::ValidationError("manual adjustment reason is required");
    }

    const ReaderReputationSnapshot snapshot = repository_.get_reader_snapshot(reader_id);
    if (!snapshot.exists) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(reader_id));
    }

    const int new_reputation = snapshot.reputation_points + change_value;
    const bool should_auto_block = (new_reputation < config_.auto_block_threshold);
    const bool new_block_state = snapshot.is_blocked || should_auto_block;
    const std::optional<std::string> block_reason = should_auto_block ? std::optional<std::string>(config_.auto_block_reason)
                                                                       : std::nullopt;

    repository_.update_reader_reputation(reader_id, new_reputation, new_block_state, block_reason, should_auto_block);

    ReputationChange change;
    change.reader_id = reader_id;
    change.change_value = change_value;
    change.reason = "MANUAL: " + normalized_reason;
    change.related_loan_id = related_loan_id;

    ReputationChange saved = repository_.create_change(change);
    logger_("manual reputation adjustment reader_id=" + std::to_string(reader_id) + " value=" +
            std::to_string(change_value));
    return saved;
}

ReputationChange ReputationService::apply_on_loan_return(const LoanReturnEvent& event) {
    if (event.reader_id <= 0) {
        throw errors::ValidationError("reader_id must be greater than zero");
    }

    const ReaderReputationSnapshot snapshot = repository_.get_reader_snapshot(event.reader_id);
    if (!snapshot.exists) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(event.reader_id));
    }

    const int weeks_late = full_weeks_late(event.due_date, event.return_date);

    int delta = 0;
    std::string reason;

    if (weeks_late > 0) {
        delta = config_.late_penalty_per_full_week * weeks_late;
        reason = "LATE_RETURN_" + std::to_string(weeks_late) + "_FULL_WEEKS";
    } else if (event.was_extended) {
        delta = config_.extended_return_bonus;
        reason = "RETURN_AFTER_EXTENSION";
    } else {
        delta = config_.on_time_return_bonus;
        reason = "ON_TIME_RETURN";
    }

    const int new_reputation = snapshot.reputation_points + delta;
    const bool should_auto_block = (new_reputation < config_.auto_block_threshold);
    const bool new_block_state = snapshot.is_blocked || should_auto_block;
    const std::optional<std::string> block_reason = should_auto_block ? std::optional<std::string>(config_.auto_block_reason)
                                                                       : std::nullopt;

    repository_.update_reader_reputation(event.reader_id, new_reputation, new_block_state, block_reason, should_auto_block);

    ReputationChange change;
    change.reader_id = event.reader_id;
    change.change_value = delta;
    change.reason = reason;
    change.related_loan_id = event.related_loan_id;

    ReputationChange saved = repository_.create_change(change);
    logger_("auto reputation update reader_id=" + std::to_string(event.reader_id) + " delta=" + std::to_string(delta));
    return saved;
}

int ReputationService::full_weeks_late(const std::string& due_date, const std::string& return_date) {
    const std::tm due_tm = parse_date(due_date);
    const std::tm ret_tm = parse_date(return_date);

    std::tm due_copy = due_tm;
    std::tm ret_copy = ret_tm;

    const std::time_t due_tt = std::mktime(&due_copy);
    const std::time_t ret_tt = std::mktime(&ret_copy);

    if (due_tt == static_cast<std::time_t>(-1) || ret_tt == static_cast<std::time_t>(-1)) {
        throw errors::ValidationError("Failed to parse dates for reputation calculation");
    }

    if (ret_tt <= due_tt) {
        return 0;
    }

    const long long seconds = static_cast<long long>(ret_tt - due_tt);
    const long long days = seconds / (60LL * 60LL * 24LL);
    return static_cast<int>(days / 7LL);
}

std::string ReputationService::trim(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

} // namespace reputation
