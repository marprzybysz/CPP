#pragma once

#include "common/system_id.hpp"
#include "notes/note_repository.hpp"

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace notes {

class NoteService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    NoteService(NoteRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    Note add_note(const CreateNoteInput& input);
    std::vector<Note> get_notes_for_target(const NotesForTargetQuery& query) const;
    Note get_note_details(const std::string& public_id, bool include_archived = false) const;
    void archive_note(const std::string& public_id);

private:
    static std::string normalize_text(std::string value);
    void ensure_sequence_initialized(int year);

    NoteRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace notes
