#include "ui/controllers/readers_controller.hpp"

#include "errors/app_error.hpp"
#include "library.hpp"

namespace ui::controllers {

ReadersController::ReadersController(Library& library) : library_(library) {}

std::vector<readers::Reader> ReadersController::find_readers(const ReaderSearchState& state) const {
    readers::ReaderQuery query;
    if (!state.first_name.empty()) {
        query.text = state.first_name;
    }
    if (!state.last_name.empty()) {
        query.last_name = state.last_name;
    }
    if (!state.card_number.empty()) {
        query.card_number = state.card_number;
    }
    if (!state.email.empty()) {
        query.email = state.email;
    }
    query.limit = state.limit;
    query.offset = 0;

    return library_.search_readers(query);
}

readers::Reader ReadersController::get_reader_details(const std::string& public_id) const {
    return library_.get_reader_details(public_id);
}

readers::Reader ReadersController::create_reader(const readers::CreateReaderInput& input) {
    readers::Reader created = library_.add_reader(input);
    selected_reader_ = created.public_id;
    return created;
}

readers::Reader ReadersController::update_reader(const std::string& public_id, const readers::UpdateReaderInput& input) {
    readers::Reader updated = library_.edit_reader(public_id, input);
    selected_reader_ = updated.public_id;
    return updated;
}

readers::Reader ReadersController::block_reader(const std::string& public_id, const std::string& reason) {
    readers::Reader updated = library_.block_reader(public_id, reason);
    selected_reader_ = updated.public_id;
    return updated;
}

readers::Reader ReadersController::unblock_reader(const std::string& public_id) {
    readers::Reader updated = library_.unblock_reader(public_id);
    selected_reader_ = updated.public_id;
    return updated;
}

int ReadersController::get_reputation_points(const std::string& reader_public_id) const {
    const auto reader = require_reader_with_id(reader_public_id);
    return library_.get_reader_reputation(*reader.id);
}

std::vector<reputation::ReputationChange> ReadersController::get_reputation_history(const std::string& reader_public_id,
                                                                                    int limit) const {
    const auto reader = require_reader_with_id(reader_public_id);
    return library_.get_reader_reputation_history(*reader.id, limit, 0);
}

std::vector<readers::ReaderLoanHistoryEntry> ReadersController::get_loan_history(const std::string& reader_public_id) const {
    return library_.get_reader_loan_history(reader_public_id);
}

void ReadersController::set_selected_reader(const std::string& public_id) {
    selected_reader_ = public_id;
}

void ReadersController::clear_selected_reader() {
    selected_reader_.reset();
}

const std::optional<std::string>& ReadersController::selected_reader() const {
    return selected_reader_;
}

void ReadersController::begin_create() {
    form_state_ = {.mode = ReaderFormMode::Create, .target_public_id = std::nullopt};
}

void ReadersController::begin_edit(const std::string& public_id) {
    form_state_ = {.mode = ReaderFormMode::Edit, .target_public_id = public_id};
}

const ReaderFormState& ReadersController::form_state() const {
    return form_state_;
}

readers::Reader ReadersController::require_reader_with_id(const std::string& public_id) const {
    readers::Reader reader = library_.get_reader_details(public_id);
    if (!reader.id.has_value()) {
        throw errors::ValidationError("Reader id is missing for public_id=" + public_id);
    }
    return reader;
}

} // namespace ui::controllers
