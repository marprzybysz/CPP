#pragma once

#include <optional>
#include <string>
#include <vector>

#include "reservations/reservation.hpp"

class Library;

namespace ui::controllers {

struct ReservationListState {
    std::optional<reservations::ReservationStatus> status;
    int limit{120};
};

struct ReservationListEntry {
    reservations::LoanListItem row;
    std::string target_label;
};

class ReservationsController {
public:
    explicit ReservationsController(Library& library);

    [[nodiscard]] std::vector<ReservationListEntry> list_reservations(const ReservationListState& state) const;
    [[nodiscard]] ReservationListEntry get_reservation_details(const std::string& public_id) const;

    reservations::Reservation create_reservation(const std::string& reader_token,
                                                 const std::string& target_type,
                                                 const std::string& target_token,
                                                 const std::string& expiration_date);
    reservations::Reservation cancel_reservation(const std::string& public_id);

    void set_selected_reservation(const std::string& public_id);
    void clear_selected_reservation();
    [[nodiscard]] const std::optional<std::string>& selected_reservation() const;

private:
    [[nodiscard]] int resolve_reader_id(const std::string& token) const;
    [[nodiscard]] int resolve_copy_id(const std::string& token) const;
    [[nodiscard]] int resolve_book_id(const std::string& token) const;
    [[nodiscard]] static std::string trim_copy(std::string raw);
    [[nodiscard]] static std::string normalize_target_type(std::string raw);

    Library& library_;
    std::optional<std::string> selected_reservation_;
};

} // namespace ui::controllers
