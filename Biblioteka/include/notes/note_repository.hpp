#pragma once

#include "notes/note.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace notes {

class NoteRepository {
public:
    virtual ~NoteRepository() = default;

    virtual Note create(const Note& note) = 0;
    virtual std::optional<Note> get_by_public_id(const std::string& public_id, bool include_archived = false) const = 0;
    virtual std::vector<Note> list_for_target(const NotesForTargetQuery& query) const = 0;
    virtual void archive_by_public_id(const std::string& public_id) = 0;
    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace notes
