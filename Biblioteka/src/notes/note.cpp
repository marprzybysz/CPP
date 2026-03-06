#include "notes/note.hpp"

#include "errors/app_error.hpp"

namespace notes {

std::string to_string(NoteTargetType type) {
    switch (type) {
        case NoteTargetType::Reader:
            return "READER";
        case NoteTargetType::Book:
            return "BOOK";
        case NoteTargetType::Copy:
            return "COPY";
        case NoteTargetType::Loan:
            return "LOAN";
        default:
            throw errors::ValidationError("Unsupported note target type");
    }
}

NoteTargetType note_target_type_from_string(const std::string& raw) {
    if (raw == "READER") {
        return NoteTargetType::Reader;
    }
    if (raw == "BOOK") {
        return NoteTargetType::Book;
    }
    if (raw == "COPY") {
        return NoteTargetType::Copy;
    }
    if (raw == "LOAN") {
        return NoteTargetType::Loan;
    }

    throw errors::ValidationError("Unsupported note target type: " + raw);
}

} // namespace notes
