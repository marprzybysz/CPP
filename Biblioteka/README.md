# Biblioteka

Projekt `Biblioteka` to backend/logika systemu zarządzania biblioteką napisany w C++20.
Aktualnie projekt opiera się na CMake i SQLite3 oraz zawiera podstawowy moduł katalogu książek i spójny moduł błędów.

## Cel projektu

Rozwój backendu dla systemu bibliotecznego obejmującego:
- katalog książek
- egzemplarze
- czytelników
- wypożyczenia i rezerwacje
- lokalizacje i inwentaryzację
- raporty i notatki
- reputację czytelników
- zakup i wycofywanie książek
- logi zdarzeń
- moduł błędów

## Aktualna struktura (folder `Biblioteka`)

- `CMakeLists.txt`
- `include/`
- `book.hpp`
- `db.hpp`
- `library.hpp`
- `common/`
- `system_id.hpp`
- `books/`
- `book.hpp`
- `book_repository.hpp`
- `sqlite_book_repository.hpp`
- `book_service.hpp`
- `errors/`
- `app_error.hpp`
- `error_codes.hpp`
- `error_logger.hpp`
- `error_mapper.hpp`
- `src/`
- `main.cpp`
- `db.cpp`
- `library.cpp`
- `common/`
- `system_id.cpp`
- `books/`
- `book_service.cpp`
- `sqlite_book_repository.cpp`
- `errors/`
- `error_logger.cpp`
- `error_mapper.cpp`

## Zależności

- CMake >= 3.20
- Kompilator C++20 (`g++` lub `clang++`)
- SQLite3 (biblioteka developerska)

## Budowanie projektu

```bash
cd Biblioteka
cmake -S . -B build
cmake --build build
```

## Uruchamianie

```bash
./build/cpp_biblioteka
```

Program tworzy/otwiera lokalną bazę `library.db`, inicjalizuje schemat tabeli `books` i wykonuje prosty przepływ dodania książki.

## Moduły istniejące

- `Db`: połączenie z SQLite i inicjalizacja schematu
- `Library`: fasada dla modułów domenowych
- `common::SystemIdGenerator`: uniwersalny generator identyfikatorów systemowych
- `books::SqliteBookRepository`: warstwa dostępu do danych katalogu książek
- `books::BookService`: logika biznesowa katalogu książek (walidacja ISBN, public_id, logowanie operacji)
- `errors`: hierarchia wyjątków aplikacyjnych, mapowanie błędów na komunikaty użytkownika, logowanie błędów

## Moduł katalogu książek

Obsługiwane operacje:
- dodawanie książki
- edycja książki
- archiwizacja logiczna książki (`is_archived`, `archived_at`)
- pobieranie szczegółów książki
- listowanie książek
- wyszukiwanie po `title`, `author`, `isbn`
- filtrowanie po `categories` i `tags`

Model książki (`books::Book`) zawiera:
- `id` (wewnętrzne)
- `public_id`
- `title`
- `author`
- `isbn`
- `categories`
- `tags`
- `edition`
- `publisher`
- `publication_year`
- `description`
- `created_at`
- `updated_at`
- `archived_at`

Walidacja i reguły:
- `title` i `author` są wymagane
- `isbn` jest wymagany i walidowany (ISBN-10 / ISBN-13)
- `public_id` jest generowany automatycznie przez `common::SystemIdGenerator`
- operacje serwisu są logowane przez `books::BookService`

## Konwencja identyfikatorów

Generator obsługuje formaty:
- `BOOK-2026-000001`
- `COPY-2026-000001`
- `CARD-000123`
- `LOAN-2026-000991`
- `RPT-2026-000120`
- `LOC-2026-000001`
- `NOTE-2026-000001`

Zasady:
- prefiks identyfikuje domenę (`BOOK`, `COPY`, `CARD`, `LOAN`, `RPT`, `LOC`, `NOTE`)
- dla typów rocznych numeracja jest niezależna per rok
- sekwencja jest zeropadowana do szerokości 6
- `CARD` nie zawiera roku i używa globalnej sekwencji

API modułu `common/system_id.hpp`:
- `common::rule_for(IdType)` zwraca regułę formatu dla typu
- `common::format_identifier(...)` buduje ID w sposób deterministyczny (łatwy do testowania)
- `common::SystemIdGenerator` zarządza sekwencjami i generuje kolejne ID
- `set_next_sequence(...)` i `peek_next_sequence(...)` umożliwiają kontrolę stanu generatora w testach i integracjach

## Moduł błędów

Dostępne klasy:
- `errors::AppError`
- `errors::ValidationError`
- `errors::DatabaseError`
- `errors::NotFoundError`
- `errors::BookNotFoundError`
- `errors::CopyUnavailableError`
- `errors::ReaderBlockedError`
- `errors::LoanError`
- `errors::InventoryError`

Dodatkowo:
- `errors::to_user_message(const std::exception&)` mapuje wyjątek na czytelny komunikat
- `errors::log_error(const std::exception&, std::ostream&)` zapisuje log błędu

## Zasady rozwoju

- Rozbudowuj projekt iteracyjnie, małymi krokami.
- Dodawaj nowe moduły domenowe w `include/<modul>/` i `src/<modul>/`.
- Utrzymuj spójne API klas i jawne granice odpowiedzialności.
- Stosuj wyjątki z modułu `errors` zamiast `std::runtime_error` w kodzie domenowym.

## Commitowanie

Stosuj małe, logiczne commity w stylu:
- `chore: analyze existing biblioteka structure`
- `feat: add domain error module`
- `feat: add reader model and service`
- `feat: add loan workflow`
- `docs: update biblioteka readme`
- `refactor: split library module into domain components`
