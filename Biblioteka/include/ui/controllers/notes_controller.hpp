#pragma once

#include <optional>
#include <string>
#include <vector>

#include "notes/note.hpp"

class Library;

namespace ui::controllers {

struct NotesFilterState {
    notes::NoteTargetType target_type{notes::NoteTargetType::Reader};
    std::string target_id;
    bool include_archived{false};
    int limit{120};
};

class NotesController {
public:
    explicit NotesController(Library& library);

    [[nodiscard]] std::vector<notes::Note> list_notes(const NotesFilterState& filter) const;
    notes::Note add_note(const notes::CreateNoteInput& input);

private:
    Library& library_;
};

} // namespace ui::controllers
