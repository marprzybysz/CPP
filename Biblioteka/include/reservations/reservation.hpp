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

[[nodiscard]] std::string to_string(ReservationStatus status);
[[nodiscard]] ReservationStatus reservation_status_from_string(const std::string& raw);

} // namespace reservations
