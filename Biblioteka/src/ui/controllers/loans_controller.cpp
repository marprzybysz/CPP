#include "ui/controllers/loans_controller.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <unordered_map>
#include <utility>

#include "errors/app_error.hpp"
#include "library.hpp"
#include "readers/reader.hpp"
#include "search/search.hpp"

namespace ui::controllers {

namespace {

std::string current_date_iso() {
    const auto now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    char buffer[11] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return buffer;
}

} // namespace

LoansController::LoansController(Library& library) : library_(library) {}

std::vector<LoanListEntry> LoansController::list_loans(const LoanListState& state) const {
    reservations::LoanListQuery base_query;
    base_query.limit = state.limit;
    base_query.offset = 0;

    const std::string normalized = trim_copy(state.query);
    if (normalized.empty()) {
        if (state.mode == LoanListMode::Active) {
            base_query.status = reservations::ReservationStatus::Active;
        }
        return to_entries(run_query(base_query), state.mode);
    }

    auto with_prefix = [&](const std::string& prefix, std::string& value) {
        if (normalized.rfind(prefix, 0) == 0) {
            value = normalized.substr(prefix.size());
            return true;
        }
        return false;
    };

    reservations::LoanListQuery prefixed = base_query;
    std::string value;
    if (with_prefix("r:", value) || with_prefix("reader:", value)) {
        prefixed.reader_query = trim_copy(value);
        return to_entries(run_query(prefixed), state.mode);
    }
    if (with_prefix("c:", value) || with_prefix("copy:", value)) {
        prefixed.copy_query = trim_copy(value);
        return to_entries(run_query(prefixed), state.mode);
    }
    if (with_prefix("card:", value)) {
        prefixed.card_number = trim_copy(value);
        return to_entries(run_query(prefixed), state.mode);
    }
    if (with_prefix("inv:", value)) {
        prefixed.inventory_number = trim_copy(value);
        return to_entries(run_query(prefixed), state.mode);
    }

    std::vector<reservations::LoanListItem> merged;
    std::unordered_map<std::string, std::size_t> index;

    auto merge_rows = [&](std::vector<reservations::LoanListItem> rows) {
        for (auto& row : rows) {
            const auto it = index.find(row.reservation.public_id);
            if (it == index.end()) {
                index.emplace(row.reservation.public_id, merged.size());
                merged.push_back(std::move(row));
                continue;
            }

            auto& target = merged[it->second];
            if (!target.inventory_number.has_value() && row.inventory_number.has_value()) {
                target.inventory_number = row.inventory_number;
                target.copy_public_id = row.copy_public_id;
            }
        }
    };

    reservations::LoanListQuery reader_query = base_query;
    reader_query.reader_query = normalized;
    merge_rows(run_query(reader_query));

    reservations::LoanListQuery copy_query = base_query;
    copy_query.copy_query = normalized;
    merge_rows(run_query(copy_query));

    reservations::LoanListQuery card_query = base_query;
    card_query.card_number = normalized;
    merge_rows(run_query(card_query));

    reservations::LoanListQuery inventory_query = base_query;
    inventory_query.inventory_number = normalized;
    merge_rows(run_query(inventory_query));

    return to_entries(std::move(merged), state.mode);
}

LoanListEntry LoansController::get_loan_details(const std::string& public_id) const {
    reservations::LoanListQuery query;
    query.public_id = public_id;
    query.limit = 1;

    const auto rows = run_query(query);
    if (rows.empty()) {
        throw errors::NotFoundError("Loan not found. public_id=" + public_id);
    }

    LoanListEntry entry;
    entry.row = rows.front();
    entry.overdue = (entry.row.reservation.status == reservations::ReservationStatus::Active) &&
                    (entry.row.reservation.expiration_date < current_date_iso());
    return entry;
}

reservations::Reservation LoansController::create_loan(const std::string& reader_token,
                                                       const std::string& copy_token,
                                                       const std::string& expiration_date) {
    reservations::CreateReservationInput input;
    input.reader_id = resolve_reader_id(reader_token);
    input.copy_id = resolve_copy_id(copy_token);
    input.expiration_date = trim_copy(expiration_date);

    const reservations::Reservation created = library_.create_reservation(input);
    selected_loan_ = created.public_id;
    return created;
}

reservations::Reservation LoansController::return_loan(const std::string& public_id) {
    const reservations::Reservation updated = library_.fulfill_loan(public_id);
    selected_loan_ = updated.public_id;
    return updated;
}

reservations::Reservation LoansController::extend_loan(const std::string& public_id, const std::string& new_expiration_date) {
    const reservations::Reservation updated = library_.extend_loan(public_id, trim_copy(new_expiration_date));
    selected_loan_ = updated.public_id;
    return updated;
}

void LoansController::set_selected_loan(const std::string& public_id) {
    selected_loan_ = public_id;
}

void LoansController::clear_selected_loan() {
    selected_loan_.reset();
}

const std::optional<std::string>& LoansController::selected_loan() const {
    return selected_loan_;
}

std::vector<reservations::LoanListItem> LoansController::run_query(const reservations::LoanListQuery& query) const {
    return library_.list_loans(query);
}

std::vector<LoanListEntry> LoansController::to_entries(std::vector<reservations::LoanListItem> rows, LoanListMode mode) const {
    std::vector<LoanListEntry> out;
    out.reserve(rows.size());

    for (auto& row : rows) {
        if (mode == LoanListMode::Active && row.reservation.status != reservations::ReservationStatus::Active) {
            continue;
        }
        if (mode == LoanListMode::History && row.reservation.status == reservations::ReservationStatus::Active) {
            continue;
        }

        LoanListEntry entry;
        entry.overdue = (row.reservation.status == reservations::ReservationStatus::Active) &&
                        (row.reservation.expiration_date < current_date_iso());
        entry.row = std::move(row);
        out.push_back(std::move(entry));
    }

    std::sort(out.begin(), out.end(), [](const LoanListEntry& lhs, const LoanListEntry& rhs) {
        return lhs.row.reservation.created_at > rhs.row.reservation.created_at;
    });
    return out;
}

int LoansController::resolve_reader_id(const std::string& token) const {
    const std::string normalized = trim_copy(token);
    if (normalized.empty()) {
        throw errors::ValidationError("Reader token is required");
    }

    try {
        const auto reader = library_.get_reader_details(normalized);
        if (reader.id.has_value()) {
            return *reader.id;
        }
    } catch (...) {
    }

    readers::ReaderQuery by_card;
    by_card.card_number = normalized;
    by_card.limit = 2;
    const auto by_card_rows = library_.search_readers(by_card);
    if (by_card_rows.size() == 1 && by_card_rows.front().id.has_value()) {
        return *by_card_rows.front().id;
    }

    readers::ReaderQuery by_text;
    by_text.text = normalized;
    by_text.limit = 2;
    const auto by_text_rows = library_.search_readers(by_text);
    if (by_text_rows.size() == 1 && by_text_rows.front().id.has_value()) {
        return *by_text_rows.front().id;
    }

    if (by_text_rows.size() > 1 || by_card_rows.size() > 1) {
        throw errors::ValidationError("Reader query is ambiguous. Use reader public_id or card number.");
    }

    throw errors::NotFoundError("Reader not found for token=" + normalized);
}

int LoansController::resolve_copy_id(const std::string& token) const {
    const std::string normalized = trim_copy(token);
    if (normalized.empty()) {
        throw errors::ValidationError("Copy token is required");
    }

    try {
        const auto copy = library_.get_copy(normalized);
        if (copy.id.has_value()) {
            return *copy.id;
        }
    } catch (...) {
    }

    search::SearchQuery query;
    query.text = normalized;
    query.limit = 20;

    const auto result = library_.global_search(query);

    std::vector<search::CopySearchHit> exact;
    for (const auto& hit : result.copies) {
        if (hit.copy_public_id == normalized || hit.inventory_number == normalized) {
            exact.push_back(hit);
        }
    }

    const auto& source = exact.empty() ? result.copies : exact;
    if (source.size() == 1) {
        const auto copy = library_.get_copy(source.front().copy_public_id);
        if (copy.id.has_value()) {
            return *copy.id;
        }
    }

    if (source.size() > 1) {
        throw errors::ValidationError("Copy query is ambiguous. Use copy public_id or exact inventory_number.");
    }

    throw errors::NotFoundError("Copy not found for token=" + normalized);
}

std::string LoansController::trim_copy(std::string raw) {
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())) != 0) {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())) != 0) {
        raw.pop_back();
    }
    return raw;
}

} // namespace ui::controllers
