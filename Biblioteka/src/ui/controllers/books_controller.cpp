#include "ui/controllers/books_controller.hpp"

#include "library.hpp"

namespace ui::controllers {

BooksController::BooksController(Library& library) : library_(library) {}

std::vector<books::Book> BooksController::find_books(const BookSearchState& state) const {
    const bool use_search = !state.title.empty() || !state.author.empty() || !state.isbn.empty();
    if (!use_search) {
        return library_.list_books(state.include_archived, state.limit, 0);
    }

    books::BookQuery query;
    if (!state.title.empty()) {
        query.title = state.title;
    }
    if (!state.author.empty()) {
        query.author = state.author;
    }
    if (!state.isbn.empty()) {
        query.isbn = state.isbn;
    }
    query.include_archived = state.include_archived;
    query.limit = state.limit;
    query.offset = 0;

    return library_.search_books(query);
}

books::Book BooksController::get_book_details(const std::string& public_id, bool include_archived) const {
    return library_.get_book_details(public_id, include_archived);
}

books::Book BooksController::create_book(const books::CreateBookInput& input) {
    books::Book created = library_.add_book(input);
    selected_book_ = created.public_id;
    return created;
}

books::Book BooksController::update_book(const std::string& public_id, const books::UpdateBookInput& input) {
    books::Book updated = library_.edit_book(public_id, input);
    selected_book_ = updated.public_id;
    return updated;
}

void BooksController::set_selected_book(const std::string& public_id) {
    selected_book_ = public_id;
}

const std::optional<std::string>& BooksController::selected_book() const {
    return selected_book_;
}

void BooksController::begin_create() {
    form_state_ = {.mode = BookFormMode::Create, .target_public_id = std::nullopt};
}

void BooksController::begin_edit(const std::string& public_id) {
    form_state_ = {.mode = BookFormMode::Edit, .target_public_id = public_id};
}

const BookFormState& BooksController::form_state() const {
    return form_state_;
}

} // namespace ui::controllers
