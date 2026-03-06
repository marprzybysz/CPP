#pragma once

#include "common/system_id.hpp"
#include "reservations/reservation_repository.hpp"

#include <functional>
#include <string>
#include <unordered_set>

namespace reservations {

class ReservationService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    ReservationService(ReservationRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    Reservation create_reservation(const CreateReservationInput& input);
    Reservation cancel_reservation(const std::string& public_id);
    Reservation expire_reservation(const std::string& public_id);
    Reservation get_reservation_details(const std::string& public_id) const;

    std::optional<Reservation> find_active_for_returned_copy(int copy_id) const;

private:
    void ensure_sequence_initialized(int year);

    ReservationRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace reservations
