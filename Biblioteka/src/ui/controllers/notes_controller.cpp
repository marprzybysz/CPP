#include "ui/controllers/notes_controller.hpp"

#include "library.hpp"

namespace ui::controllers {

NotesController::NotesController(Library& library) : library_(library) {}

std::vector<notes::Note> NotesController::list_notes(const NotesFilterState& filter) const {
    notes::NotesForTargetQuery query;
    query.target_type = filter.target_type;
    query.target_id = filter.target_id;
    query.include_archived = filter.include_archived;
    query.limit = filter.limit;
    query.offset = 0;
    return library_.get_notes_for_target(query);
}

notes::Note NotesController::add_note(const notes::CreateNoteInput& input) {
    return library_.add_note(input);
}

} // namespace ui::controllers
