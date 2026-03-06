#pragma once

#include "db.hpp"
#include "notes/note_repository.hpp"

namespace notes {

class SqliteNoteRepository final : public NoteRepository {
public:
    explicit SqliteNoteRepository(Db& db) : db_(db) {}

    Note create(const Note& note) override;
    std::optional<Note> get_by_public_id(const std::string& public_id, bool include_archived = false) const override;
    std::vector<Note> list_for_target(const NotesForTargetQuery& query) const override;
    void archive_by_public_id(const std::string& public_id) override;
    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace notes
