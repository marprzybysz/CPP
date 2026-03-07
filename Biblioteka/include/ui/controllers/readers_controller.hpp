#pragma once

#include <optional>
#include <string>
#include <vector>

#include "readers/reader.hpp"
#include "reputation/reputation.hpp"

class Library;

namespace ui::controllers {

struct ReaderSearchState {
    std::string first_name;
    std::string last_name;
    std::string card_number;
    std::string email;
    int limit{100};
};

enum class ReaderFormMode {
    Create,
    Edit,
};

struct ReaderFormState {
    ReaderFormMode mode{ReaderFormMode::Create};
    std::optional<std::string> target_public_id;
};

class ReadersController {
public:
    explicit ReadersController(Library& library);

    [[nodiscard]] std::vector<readers::Reader> find_readers(const ReaderSearchState& state) const;
    [[nodiscard]] readers::Reader get_reader_details(const std::string& public_id) const;

    readers::Reader create_reader(const readers::CreateReaderInput& input);
    readers::Reader update_reader(const std::string& public_id, const readers::UpdateReaderInput& input);
    readers::Reader block_reader(const std::string& public_id, const std::string& reason);
    readers::Reader unblock_reader(const std::string& public_id);

    [[nodiscard]] int get_reputation_points(const std::string& reader_public_id) const;
    [[nodiscard]] std::vector<reputation::ReputationChange> get_reputation_history(const std::string& reader_public_id,
                                                                                   int limit = 80) const;
    [[nodiscard]] std::vector<readers::ReaderLoanHistoryEntry> get_loan_history(const std::string& reader_public_id) const;

    void set_selected_reader(const std::string& public_id);
    void clear_selected_reader();
    [[nodiscard]] const std::optional<std::string>& selected_reader() const;

    void begin_create();
    void begin_edit(const std::string& public_id);
    [[nodiscard]] const ReaderFormState& form_state() const;

private:
    [[nodiscard]] readers::Reader require_reader_with_id(const std::string& public_id) const;

    Library& library_;
    std::optional<std::string> selected_reader_;
    ReaderFormState form_state_;
};

} // namespace ui::controllers
