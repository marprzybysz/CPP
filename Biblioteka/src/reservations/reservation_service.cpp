#include "reservations/reservation_service.hpp"

#include "errors/app_error.hpp"

#include <iostream>
#include <utility>

namespace reservations {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[reservations] " << message << '\n';
}

} // namespace

ReservationService::ReservationService(ReservationRepository& repository,
                                       common::SystemIdGenerator& id_generator,
                                       OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

Reservation ReservationService::create_reservation(const CreateReservationInput& input) {
    if (input.reader_id <= 0) {
        throw errors::ValidationError("reservation reader_id must be greater than zero");
    }
    if (input.expiration_date.empty()) {
        throw errors::ValidationError("reservation expiration_date is required");
    }

    const bool has_copy = input.copy_id.has_value();
    const bool has_book = input.book_id.has_value();
    if (has_copy == has_book) {
        throw errors::ValidationError("reservation must point to exactly one target: copy_id or book_id");
    }

    const ReaderReservationInfo reader = repository_.get_reader_info(input.reader_id);
    if (!reader.exists) {
        throw errors::NotFoundError("Reader not found. id=" + std::to_string(input.reader_id));
    }
    if (reader.is_blocked) {
        throw errors::ReaderBlockedError(input.reader_id);
    }

    Reservation reservation;
    reservation.reader_id = input.reader_id;
    reservation.expiration_date = input.expiration_date;
    reservation.status = ReservationStatus::Active;

    if (has_copy) {
        const CopyReservationInfo copy = repository_.get_copy_info(*input.copy_id);
        if (!copy.exists) {
            throw errors::NotFoundError("Copy not found. id=" + std::to_string(*input.copy_id));
        }

        if (copy.status == copies::CopyStatus::Reserved || copy.status == copies::CopyStatus::InRepair ||
            copy.status == copies::CopyStatus::Archived || copy.status == copies::CopyStatus::Lost) {
            throw errors::CopyUnavailableError(*input.copy_id);
        }

        if (repository_.has_active_reservation_for_copy(*input.copy_id)) {
            throw errors::ValidationError("Active reservation already exists for copy id=" + std::to_string(*input.copy_id));
        }

        reservation.copy_id = input.copy_id;
    } else {
        if (*input.book_id <= 0) {
            throw errors::ValidationError("reservation book_id must be greater than zero");
        }
        if (!repository_.book_exists(*input.book_id)) {
            throw errors::BookNotFoundError(*input.book_id);
        }
        reservation.book_id = input.book_id;
    }

    const int year = common::current_year();
    ensure_sequence_initialized(year);
    reservation.public_id = id_generator_.generate_for_year(common::IdType::Reservation, year);

    Reservation created = repository_.create(reservation);
    logger_("reservation created public_id=" + created.public_id);
    return created;
}

Reservation ReservationService::cancel_reservation(const std::string& public_id) {
    const Reservation reservation = get_reservation_details(public_id);
    if (reservation.status != ReservationStatus::Active) {
        throw errors::ValidationError("Only ACTIVE reservation can be cancelled");
    }

    Reservation updated = repository_.update_status(public_id, ReservationStatus::Cancelled);
    logger_("reservation cancelled public_id=" + updated.public_id);
    return updated;
}

Reservation ReservationService::expire_reservation(const std::string& public_id) {
    const Reservation reservation = get_reservation_details(public_id);
    if (reservation.status != ReservationStatus::Active) {
        throw errors::ValidationError("Only ACTIVE reservation can be expired");
    }

    Reservation updated = repository_.update_status(public_id, ReservationStatus::Expired);
    logger_("reservation expired public_id=" + updated.public_id);
    return updated;
}

Reservation ReservationService::get_reservation_details(const std::string& public_id) const {
    const auto found = repository_.get_by_public_id(public_id);
    if (!found.has_value()) {
        throw errors::NotFoundError("Reservation not found. public_id=" + public_id);
    }
    return *found;
}

std::optional<Reservation> ReservationService::find_active_for_returned_copy(int copy_id) const {
    if (copy_id <= 0) {
        throw errors::ValidationError("copy_id must be greater than zero");
    }

    const auto for_copy = repository_.find_oldest_active_for_copy(copy_id);
    if (for_copy.has_value()) {
        return for_copy;
    }

    const CopyReservationInfo copy = repository_.get_copy_info(copy_id);
    if (!copy.exists) {
        throw errors::NotFoundError("Copy not found. id=" + std::to_string(copy_id));
    }

    return repository_.find_oldest_active_for_book(copy.book_id);
}

void ReservationService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Reservation, year, next);
    initialized_years_.insert(year);
}

} // namespace reservations
