#pragma once

#include <optional>
#include <string>
#include <vector>

#include "reservations/reservation.hpp"

class Library;

namespace ui::controllers {

enum class LoanListMode {
    Active,
    History,
};

struct LoanListState {
    LoanListMode mode{LoanListMode::Active};
    std::string query;
    int limit{120};
};

struct LoanListEntry {
    reservations::LoanListItem row;
    bool overdue{false};
};

class LoansController {
public:
    explicit LoansController(Library& library);

    [[nodiscard]] std::vector<LoanListEntry> list_loans(const LoanListState& state) const;
    [[nodiscard]] LoanListEntry get_loan_details(const std::string& public_id) const;

    reservations::Reservation create_loan(const std::string& reader_token,
                                          const std::string& copy_token,
                                          const std::string& expiration_date);
    reservations::Reservation return_loan(const std::string& public_id);
    reservations::Reservation extend_loan(const std::string& public_id, const std::string& new_expiration_date);

    void set_selected_loan(const std::string& public_id);
    void clear_selected_loan();
    [[nodiscard]] const std::optional<std::string>& selected_loan() const;

private:
    [[nodiscard]] std::vector<reservations::LoanListItem> run_query(const reservations::LoanListQuery& query) const;
    [[nodiscard]] std::vector<LoanListEntry> to_entries(std::vector<reservations::LoanListItem> rows,
                                                        LoanListMode mode) const;
    [[nodiscard]] int resolve_reader_id(const std::string& token) const;
    [[nodiscard]] int resolve_copy_id(const std::string& token) const;
    [[nodiscard]] static std::string trim_copy(std::string raw);

    Library& library_;
    std::optional<std::string> selected_loan_;
};

} // namespace ui::controllers
