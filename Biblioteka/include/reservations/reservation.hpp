#pragma once

#include <optional>
#include <string>

namespace reservations {

enum class ReservationStatus {
    Active,
    Cancelled,
    Expired,
    Fulfilled,
};

struct Reservation {
    std::optional<int> id;
    std::string public_id;
    int reader_id{0};
    std::optional<int> copy_id;
    std::optional<int> book_id;
    std::string reservation_date;
    std::string expiration_date;
    ReservationStatus status{ReservationStatus::Active};
    std::string created_at;
    std::string updated_at;
};

struct CreateReservationInput {
    int reader_id{0};
    std::optional<int> copy_id;
    std::optional<int> book_id;
    std::string expiration_date;
};

struct LoanListQuery {
    std::optional<std::string> public_id;
    std::optional<ReservationStatus> status;
    std::optional<std::string> reader_query;
    std::optional<std::string> copy_query;
    std::optional<std::string> card_number;
    std::optional<std::string> inventory_number;
    int limit{100};
    int offset{0};
};

struct LoanListItem {
    Reservation reservation;
    std::string reader_public_id;
    std::string reader_name;
    std::string card_number;
    std::optional<std::string> copy_public_id;
    std::optional<std::string> inventory_number;
    int extension_count{0};
};

[[nodiscard]] std::string to_string(ReservationStatus status);
[[nodiscard]] ReservationStatus reservation_status_from_string(const std::string& raw);

} // namespace reservations
