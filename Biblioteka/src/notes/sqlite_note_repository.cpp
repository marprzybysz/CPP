#include "notes/sqlite_note_repository.hpp"

#include "errors/app_error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace notes {

namespace {

constexpr const char* kSelectNoteColumns =
    "SELECT id, public_id, target_type, target_id, author, created_at, content, is_archived, archived_at FROM notes";

void throw_sqlite(sqlite3* db, const char* message) {
    const char* err = db ? sqlite3_errmsg(db) : "unknown sqlite error";
    throw errors::DatabaseError(std::string(message) + " | sqlite: " + err);
}

std::string read_text(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

std::optional<std::string> read_optional_text(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return read_text(stmt, col);
}

Note map_note(sqlite3_stmt* stmt) {
    Note note;
    note.id = sqlite3_column_int(stmt, 0);
    note.public_id = read_text(stmt, 1);
    note.target_type = note_target_type_from_string(read_text(stmt, 2));
    note.target_id = read_text(stmt, 3);
    note.author = read_text(stmt, 4);
    note.created_at = read_text(stmt, 5);
    note.content = read_text(stmt, 6);
    note.is_archived = sqlite3_column_int(stmt, 7) != 0;
    note.archived_at = read_optional_text(stmt, 8);
    return note;
}

} // namespace

Note SqliteNoteRepository::create(const Note& note) {
    const char* sql =
        "INSERT INTO notes (public_id, target_type, target_id, author, created_at, content, is_archived, archived_at) "
        "VALUES (?, ?, ?, ?, datetime('now'), ?, 0, NULL);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare create note failed");
    }

    sqlite3_bind_text(stmt, 1, note.public_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string target_type = to_string(note.target_type);
    sqlite3_bind_text(stmt, 2, target_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, note.target_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, note.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, note.content.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "create note failed");
    }

    sqlite3_finalize(stmt);
    return get_by_public_id(note.public_id, true).value();
}

std::optional<Note> SqliteNoteRepository::get_by_public_id(const std::string& public_id, bool include_archived) const {
    std::string sql = std::string(kSelectNoteColumns) + " WHERE public_id = ?";
    if (!include_archived) {
        sql += " AND is_archived = 0";
    }
    sql += " LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare get note failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        Note note = map_note(stmt);
        sqlite3_finalize(stmt);
        return note;
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "get note failed");
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<Note> SqliteNoteRepository::list_for_target(const NotesForTargetQuery& query) const {
    std::string sql = std::string(kSelectNoteColumns) + " WHERE target_type = ? AND target_id = ?";
    if (!query.include_archived) {
        sql += " AND is_archived = 0";
    }
    sql += " ORDER BY created_at DESC, id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare list notes failed");
    }

    const std::string target_type = to_string(query.target_type);
    sqlite3_bind_text(stmt, 1, target_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, query.target_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, query.limit);
    sqlite3_bind_int(stmt, 4, query.offset);

    std::vector<Note> out;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            out.push_back(map_note(stmt));
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "list notes failed");
        }
    }

    sqlite3_finalize(stmt);
    return out;
}

void SqliteNoteRepository::archive_by_public_id(const std::string& public_id) {
    const char* sql = "UPDATE notes SET is_archived = 1, archived_at = datetime('now') WHERE public_id = ? AND is_archived = 0;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare archive note failed");
    }

    sqlite3_bind_text(stmt, 1, public_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw_sqlite(db_.handle(), "archive note failed");
    }

    const int changed = sqlite3_changes(db_.handle());
    sqlite3_finalize(stmt);

    if (changed == 0) {
        throw errors::NotFoundError("Note not found or already archived. public_id=" + public_id);
    }
}

std::uint64_t SqliteNoteRepository::next_public_sequence(int year) const {
    const std::string prefix = "NOTE-" + std::to_string(year) + "-";
    const std::string pattern = prefix + "%";

    const char* sql = "SELECT public_id FROM notes WHERE public_id LIKE ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw_sqlite(db_.handle(), "prepare next note sequence failed");
    }

    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::uint64_t max_sequence = 0;
    while (true) {
        const int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            const std::string public_id = read_text(stmt, 0);
            const std::size_t pos = public_id.rfind('-');
            if (pos == std::string::npos || pos + 1 >= public_id.size()) {
                continue;
            }
            try {
                const std::uint64_t seq = std::stoull(public_id.substr(pos + 1));
                if (seq > max_sequence) {
                    max_sequence = seq;
                }
            } catch (...) {
                continue;
            }
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            throw_sqlite(db_.handle(), "read next note sequence failed");
        }
    }

    sqlite3_finalize(stmt);
    return max_sequence + 1;
}

} // namespace notes
