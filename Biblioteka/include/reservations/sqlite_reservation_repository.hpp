#pragma once

#include "db.hpp"
#include "reservations/reservation_repository.hpp"

namespace reservations {

class SqliteReservationRepository final : public ReservationRepository {
public:
    explicit SqliteReservationRepository(Db& db) : db_(db) {}

    Reservation create(const Reservation& reservation) override;
    std::optional<Reservation> get_by_public_id(const std::string& public_id) const override;
    Reservation update_status(const std::string& public_id, ReservationStatus status) override;
    Reservation update_expiration(const std::string& public_id, const std::string& expiration_date) override;
    std::vector<LoanListItem> list_loans(const LoanListQuery& query) const override;

    ReaderReservationInfo get_reader_info(int reader_id) const override;
    CopyReservationInfo get_copy_info(int copy_id) const override;
    bool book_exists(int book_id) const override;

    bool has_active_reservation_for_copy(int copy_id) const override;

    std::optional<Reservation> find_oldest_active_for_copy(int copy_id) const override;
    std::optional<Reservation> find_oldest_active_for_book(int book_id) const override;

    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace reservations
