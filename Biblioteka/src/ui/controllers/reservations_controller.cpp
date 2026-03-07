#include "ui/controllers/reservations_controller.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

#include "books/book.hpp"
#include "errors/app_error.hpp"
#include "library.hpp"
#include "readers/reader.hpp"
#include "search/search.hpp"

namespace ui::controllers {

ReservationsController::ReservationsController(Library& library) : library_(library) {}

std::vector<ReservationListEntry> ReservationsController::list_reservations(const ReservationListState& state) const {
    reservations::LoanListQuery query;
    query.status = state.status;
    query.limit = state.limit;
    query.offset = 0;

    const auto rows = library_.list_loans(query);
    std::vector<ReservationListEntry> out;
    out.reserve(rows.size());

    for (const auto& row : rows) {
        ReservationListEntry entry;
        entry.row = row;

        if (row.inventory_number.has_value()) {
            entry.target_label = "COPY: " + *row.inventory_number;
        } else if (row.copy_public_id.has_value()) {
            entry.target_label = "COPY: " + *row.copy_public_id;
        } else if (row.reservation.book_id.has_value()) {
            entry.target_label = "BOOK_ID: " + std::to_string(*row.reservation.book_id);
        } else {
            entry.target_label = "-";
        }

        out.push_back(std::move(entry));
    }

    return out;
}

ReservationListEntry ReservationsController::get_reservation_details(const std::string& public_id) const {
    reservations::LoanListQuery query;
    query.public_id = public_id;
    query.limit = 1;
    query.offset = 0;

    const auto rows = library_.list_loans(query);
    if (!rows.empty()) {
        ReservationListEntry entry;
        entry.row = rows.front();
        if (entry.row.inventory_number.has_value()) {
            entry.target_label = "COPY: " + *entry.row.inventory_number;
        } else if (entry.row.copy_public_id.has_value()) {
            entry.target_label = "COPY: " + *entry.row.copy_public_id;
        } else if (entry.row.reservation.book_id.has_value()) {
            entry.target_label = "BOOK_ID: " + std::to_string(*entry.row.reservation.book_id);
        } else {
            entry.target_label = "-";
        }
        return entry;
    }

    throw errors::NotFoundError("Reservation not found. public_id=" + public_id);
}

reservations::Reservation ReservationsController::create_reservation(const std::string& reader_token,
                                                                    const std::string& target_type,
                                                                    const std::string& target_token,
                                                                    const std::string& expiration_date) {
    reservations::CreateReservationInput input;
    input.reader_id = resolve_reader_id(reader_token);
    input.expiration_date = trim_copy(expiration_date);

    const std::string target = normalize_target_type(target_type);
    if (target == "copy") {
        input.copy_id = resolve_copy_id(target_token);
    } else if (target == "book") {
        input.book_id = resolve_book_id(target_token);
    } else {
        throw errors::ValidationError("target_type must be copy or book");
    }

    const auto created = library_.create_reservation(input);
    selected_reservation_ = created.public_id;
    return created;
}

reservations::Reservation ReservationsController::cancel_reservation(const std::string& public_id) {
    const auto updated = library_.cancel_reservation(public_id);
    selected_reservation_ = updated.public_id;
    return updated;
}

void ReservationsController::set_selected_reservation(const std::string& public_id) {
    selected_reservation_ = public_id;
}

void ReservationsController::clear_selected_reservation() {
    selected_reservation_.reset();
}

const std::optional<std::string>& ReservationsController::selected_reservation() const {
    return selected_reservation_;
}

int ReservationsController::resolve_reader_id(const std::string& token) const {
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

int ReservationsController::resolve_copy_id(const std::string& token) const {
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

int ReservationsController::resolve_book_id(const std::string& token) const {
    const std::string normalized = trim_copy(token);
    if (normalized.empty()) {
        throw errors::ValidationError("Book token is required");
    }

    try {
        const auto book = library_.get_book_details(normalized, true);
        if (book.id.has_value()) {
            return *book.id;
        }
    } catch (...) {
    }

    books::BookQuery by_text;
    by_text.text = normalized;
    by_text.include_archived = true;
    by_text.limit = 2;
    const auto books = library_.search_books(by_text);

    if (books.size() == 1 && books.front().id.has_value()) {
        return *books.front().id;
    }

    if (books.size() > 1) {
        throw errors::ValidationError("Book query is ambiguous. Use book public_id or ISBN.");
    }

    throw errors::NotFoundError("Book not found for token=" + normalized);
}

std::string ReservationsController::trim_copy(std::string raw) {
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front())) != 0) {
        raw.erase(raw.begin());
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back())) != 0) {
        raw.pop_back();
    }
    return raw;
}

std::string ReservationsController::normalize_target_type(std::string raw) {
    raw = trim_copy(std::move(raw));
    std::transform(raw.begin(), raw.end(), raw.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return raw;
}

} // namespace ui::controllers
