#pragma once

#include "copies/copy.hpp"
#include "reservations/reservation.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace reservations {

struct ReaderReservationInfo {
    bool exists{false};
    bool is_blocked{false};
};

struct CopyReservationInfo {
    bool exists{false};
    int book_id{0};
    copies::CopyStatus status{copies::CopyStatus::OnShelf};
};

class ReservationRepository {
public:
    virtual ~ReservationRepository() = default;

    virtual Reservation create(const Reservation& reservation) = 0;
    virtual std::optional<Reservation> get_by_public_id(const std::string& public_id) const = 0;
    virtual Reservation update_status(const std::string& public_id, ReservationStatus status) = 0;
    virtual Reservation update_expiration(const std::string& public_id, const std::string& expiration_date) = 0;
    virtual std::vector<LoanListItem> list_loans(const LoanListQuery& query) const = 0;

    virtual ReaderReservationInfo get_reader_info(int reader_id) const = 0;
    virtual CopyReservationInfo get_copy_info(int copy_id) const = 0;
    virtual bool book_exists(int book_id) const = 0;

    virtual bool has_active_reservation_for_copy(int copy_id) const = 0;

    virtual std::optional<Reservation> find_oldest_active_for_copy(int copy_id) const = 0;
    virtual std::optional<Reservation> find_oldest_active_for_book(int book_id) const = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace reservations
