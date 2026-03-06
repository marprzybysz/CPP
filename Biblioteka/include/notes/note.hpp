#pragma once

#include <optional>
#include <string>

namespace notes {

enum class NoteTargetType {
    Reader,
    Book,
    Copy,
    Loan,
};

struct Note {
    std::optional<int> id;
    std::string public_id;
    NoteTargetType target_type{NoteTargetType::Reader};
    std::string target_id;
    std::string author;
    std::string created_at;
    std::string content;
    bool is_archived{false};
    std::optional<std::string> archived_at;
};

struct CreateNoteInput {
    NoteTargetType target_type{NoteTargetType::Reader};
    std::string target_id;
    std::string author;
    std::string content;
};

struct NotesForTargetQuery {
    NoteTargetType target_type{NoteTargetType::Reader};
    std::string target_id;
    bool include_archived{false};
    int limit{100};
    int offset{0};
};

[[nodiscard]] std::string to_string(NoteTargetType type);
[[nodiscard]] NoteTargetType note_target_type_from_string(const std::string& raw);

} // namespace notes
