#include "notes/note_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <utility>

namespace notes {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[notes] " << message << '\n';
}

} // namespace

NoteService::NoteService(NoteRepository& repository,
                         common::SystemIdGenerator& id_generator,
                         OperationLogger logger)
    : repository_(repository), id_generator_(id_generator), logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }
}

Note NoteService::add_note(const CreateNoteInput& input) {
    Note note;
    note.target_type = input.target_type;
    note.target_id = normalize_text(input.target_id);
    note.author = normalize_text(input.author);
    note.content = normalize_text(input.content);

    if (note.target_id.empty()) {
        throw errors::ValidationError("note target_id is required");
    }
    if (note.author.empty()) {
        throw errors::ValidationError("note author is required");
    }
    if (note.content.empty()) {
        throw errors::ValidationError("note content is required");
    }

    const int year = common::current_year();
    ensure_sequence_initialized(year);
    note.public_id = id_generator_.generate_for_year(common::IdType::Note, year);

    Note created = repository_.create(note);
    logger_("note created public_id=" + created.public_id + " target_type=" + to_string(created.target_type) +
            " target_id=" + created.target_id);
    return created;
}

std::vector<Note> NoteService::get_notes_for_target(const NotesForTargetQuery& query) const {
    NotesForTargetQuery normalized = query;
    normalized.target_id = normalize_text(normalized.target_id);

    if (normalized.target_id.empty()) {
        throw errors::ValidationError("notes target_id is required");
    }
    if (normalized.limit <= 0) {
        throw errors::ValidationError("notes limit must be greater than zero");
    }
    if (normalized.offset < 0) {
        throw errors::ValidationError("notes offset cannot be negative");
    }

    return repository_.list_for_target(normalized);
}

Note NoteService::get_note_details(const std::string& public_id, bool include_archived) const {
    const auto found = repository_.get_by_public_id(public_id, include_archived);
    if (!found.has_value()) {
        throw errors::NotFoundError("Note not found. public_id=" + public_id);
    }
    return *found;
}

void NoteService::archive_note(const std::string& public_id) {
    repository_.archive_by_public_id(public_id);
    logger_("note archived public_id=" + public_id);
}

std::string NoteService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

void NoteService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Note, year, next);
    initialized_years_.insert(year);
}

} // namespace notes
