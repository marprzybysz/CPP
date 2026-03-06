#include "reservations/reservation.hpp"

#include "errors/app_error.hpp"

namespace reservations {

std::string to_string(ReservationStatus status) {
    switch (status) {
        case ReservationStatus::Active:
            return "ACTIVE";
        case ReservationStatus::Cancelled:
            return "CANCELLED";
        case ReservationStatus::Expired:
            return "EXPIRED";
        case ReservationStatus::Fulfilled:
            return "FULFILLED";
        default:
            throw errors::ValidationError("Unsupported reservation status");
    }
}

ReservationStatus reservation_status_from_string(const std::string& raw) {
    if (raw == "ACTIVE") {
        return ReservationStatus::Active;
    }
    if (raw == "CANCELLED") {
        return ReservationStatus::Cancelled;
    }
    if (raw == "EXPIRED") {
        return ReservationStatus::Expired;
    }
    if (raw == "FULFILLED") {
        return ReservationStatus::Fulfilled;
    }

    throw errors::ValidationError("Unsupported reservation status: " + raw);
}

} // namespace reservations
