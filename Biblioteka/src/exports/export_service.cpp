#include "exports/export_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace exports {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[exports] " << message << '\n';
}

bool is_withdrawal_status(copies::CopyStatus status) {
    return status == copies::CopyStatus::Archived || status == copies::CopyStatus::Lost;
}

} // namespace

ExportService::ExportService(ExportRepository& repository, OperationLogger logger)
    : repository_(repository), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

CopyWithdrawal ExportService::withdraw_copy(const WithdrawCopyInput& input, copies::CopyStatus resulting_status) {
    CopyWithdrawal withdrawal;
    withdrawal.copy_public_id = normalize_text(input.copy_public_id);
    withdrawal.reason = input.reason;
    withdrawal.operator_name = normalize_text(input.operator_name);
    withdrawal.note = normalize_text(input.note);
    withdrawal.resulting_status = resulting_status;

    if (input.withdrawal_date.has_value()) {
        withdrawal.withdrawal_date = normalize_text(*input.withdrawal_date);
    }

    if (withdrawal.copy_public_id.empty()) {
        throw errors::ValidationError("copy_public_id is required for withdrawal");
    }
    if (withdrawal.operator_name.empty()) {
        throw errors::ValidationError("operator is required for withdrawal");
    }
    if (input.withdrawal_date.has_value() && withdrawal.withdrawal_date.empty()) {
        throw errors::ValidationError("withdrawal_date cannot be blank");
    }
    if (!is_withdrawal_status(resulting_status)) {
        throw errors::ExportError("resulting copy status must be ARCHIVED or LOST");
    }
    if (repository_.withdrawal_exists_for_copy(withdrawal.copy_public_id)) {
        throw errors::ExportError("Copy already withdrawn from circulation. copy_public_id=" + withdrawal.copy_public_id);
    }

    CopyWithdrawal created = repository_.create_withdrawal(withdrawal);
    logger_("withdrawal created copy=" + created.copy_public_id + " reason=" + to_string(created.reason) +
            " status=" + copies::to_string(created.resulting_status));

    return created;
}

std::vector<WithdrawnCopyView> ExportService::list_withdrawn_copies(int limit, int offset) const {
    if (limit <= 0) {
        throw errors::ValidationError("limit must be greater than zero");
    }
    if (offset < 0) {
        throw errors::ValidationError("offset cannot be negative");
    }

    return repository_.list_withdrawn_copies(limit, offset);
}

std::string ExportService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

} // namespace exports
