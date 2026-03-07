# Biblioteka

Projekt `Biblioteka` to backend/logika systemu zarządzania biblioteką napisany w C++20.
Aktualnie projekt opiera się na CMake i SQLite3 oraz zawiera moduły: katalogu książek, egzemplarzy, lokalizacji, inwentaryzacji i błędów.

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
- `copies/`
- `copy.hpp`
- `copy_repository.hpp`
- `sqlite_copy_repository.hpp`
- `copy_service.hpp`
- `locations/`
- `location.hpp`
- `location_repository.hpp`
- `sqlite_location_repository.hpp`
- `location_service.hpp`
- `inventory/`
- `inventory.hpp`
- `inventory_repository.hpp`
- `sqlite_inventory_repository.hpp`
- `inventory_service.hpp`
- `notes/`
- `note.hpp`
- `note_repository.hpp`
- `sqlite_note_repository.hpp`
- `note_service.hpp`
- `reservations/`
- `reservation.hpp`
- `reservation_repository.hpp`
- `sqlite_reservation_repository.hpp`
- `reservation_service.hpp`
- `readers/`
- `reader.hpp`
- `reader_repository.hpp`
- `sqlite_reader_repository.hpp`
- `reader_service.hpp`
- `reputation/`
- `reputation.hpp`
- `reputation_repository.hpp`
- `sqlite_reputation_repository.hpp`
- `reputation_service.hpp`
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
- `copies/`
- `copy.cpp`
- `copy_service.cpp`
- `sqlite_copy_repository.cpp`
- `locations/`
- `location.cpp`
- `location_service.cpp`
- `sqlite_location_repository.cpp`
- `inventory/`
- `inventory.cpp`
- `inventory_service.cpp`
- `sqlite_inventory_repository.cpp`
- `notes/`
- `note.cpp`
- `note_service.cpp`
- `sqlite_note_repository.cpp`
- `reservations/`
- `reservation.cpp`
- `reservation_service.cpp`
- `sqlite_reservation_repository.cpp`
- `readers/`
- `reader.cpp`
- `reader_service.cpp`
- `sqlite_reader_repository.cpp`
- `reputation/`
- `reputation_service.cpp`
- `sqlite_reputation_repository.cpp`
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
- `copies::SqliteCopyRepository`: warstwa dostępu do danych egzemplarzy i historii zmian
- `copies::CopyService`: logika biznesowa egzemplarzy (walidacja, przejścia statusów, lokalizacja)
- `locations::SqliteLocationRepository`: warstwa danych lokalizacji i przypisanych egzemplarzy
- `locations::LocationService`: logika hierarchii lokalizacji i operacji drzewiastych
- `inventory::SqliteInventoryRepository`: warstwa danych sesji inwentaryzacji, skanów i pozycji wynikowych
- `inventory::InventoryService`: logika biznesowa spisu z natury dla `ROOM`/`RACK`/`SHELF`
- `notes::SqliteNoteRepository`: warstwa danych notatek generycznych
- `notes::NoteService`: logika tworzenia/odczytu/archiwizacji notatek
- `reservations::SqliteReservationRepository`: warstwa danych rezerwacji
- `reservations::ReservationService`: logika rezerwacji i wykrywania aktywnej kolejki po zwrocie
- `readers::SqliteReaderRepository`: warstwa danych czytelników
- `readers::ReaderService`: logika kont czytelników (walidacja e-mail, generacja kart, blokady)
- `reputation::ReputationService`: logika punktacji reputacji i automatycznej blokady
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

## Moduł egzemplarzy książek

Książka (`books::Book`) i egzemplarz (`copies::BookCopy`) to osobne byty.

Model egzemplarza zawiera:
- `id` (wewnętrzne)
- `public_id`
- `book_id`
- `inventory_number` (unikalny)
- `status`
- `target_location_id`
- `current_location_id`
- `condition_note`
- `acquisition_date`
- `archival_reason`
- `created_at`
- `updated_at`

Statusy egzemplarza:
- `ON_SHELF`
- `LOANED`
- `RESERVED`
- `IN_REPAIR`
- `ARCHIVED`
- `LOST`

Obsługiwane operacje:
- dodanie egzemplarza
- edycja egzemplarza
- pobranie egzemplarza
- listowanie egzemplarzy danej książki
- zmiana statusu z walidacją przejść
- zmiana lokalizacji

Historia zmian:
- `copy_status_history`: historia przejść statusów
- `copy_location_history`: historia zmian lokalizacji

Walidacja i reguły:
- egzemplarz musi należeć do istniejącej książki (`book_id`)
- `inventory_number` jest wymagany i unikalny
- `public_id` egzemplarza jest generowany automatycznie (`COPY-YYYY-NNNNNN`)
- dla statusu `ARCHIVED` wymagane jest `archival_reason`
- `current_location_id` i `target_location_id` muszą wskazywać istniejące lokalizacje

## Moduł lokalizacji

Hierarchia lokalizacji:
- `LIBRARY`
- `ROOM`
- `RACK`
- `SHELF`

Każda lokalizacja zawiera:
- `id`
- `public_id`
- `name`
- `type`
- `parent_id`
- `code`
- `description`

Obsługiwane operacje:
- tworzenie lokalizacji
- edycja lokalizacji
- pobranie lokalizacji
- pobranie drzewa lokalizacji
- pobranie egzemplarzy przypisanych do lokalizacji

Reguły hierarchii:
- `LIBRARY` nie ma rodzica
- `ROOM` musi mieć rodzica typu `LIBRARY`
- `RACK` musi mieć rodzica typu `ROOM`
- `SHELF` musi mieć rodzica typu `RACK`
- walidowane są cykle (`location` nie może być własnym przodkiem)

Integracja z egzemplarzami:
- `book_copies.current_location_id` i `book_copies.target_location_id` przechowują `locations.public_id`
- `LocationService::get_location_copies(...)` zwraca egzemplarze, dla których lokalizacja jest bieżąca lub docelowa

## Moduł inwentaryzacji

Spis z natury jest wspierany dla:
- `SHELF`
- `RACK`
- `ROOM`

Kluczowe encje:
- `inventory::InventorySession` (kto rozpoczął, kiedy rozpoczęto/zakończono, lokalizacja, status, wynik końcowy)
- `inventory::InventoryItem` (pozycja inwentaryzacyjna z klasyfikacją wyniku)

Obsługiwane operacje:
- rozpoczęcie inwentaryzacji (`start_inventory`)
- rejestrowanie skanów egzemplarzy (`register_inventory_scan`)
- zakończenie inwentaryzacji i wyliczenie wyniku (`finish_inventory`)
- pobranie wyniku (`get_inventory_result`)

Klasyfikacja wyniku:
- `ON_SHELF`
- `JUSTIFIED` (poza półką, ale uzasadnione)
- `MISSING`

Integracja:
- z lokalizacjami: sesja wskazuje lokalizację i typ zakresu (`ROOM`/`RACK`/`SHELF`), obsługiwane są poddrzewa lokalizacji
- z egzemplarzami: skany rozpoznają egzemplarze po `public_id` lub `inventory_number` (przygotowane pod przyszłe dekodowanie kodów)
- z modułem błędów: walidacje i błędy biznesowe zgłaszane przez `errors::ValidationError`, `errors::NotFoundError`, `errors::InventoryError`

## Moduł czytelników

Model czytelnika (`readers::Reader`) zawiera:
- `id`
- `public_id`
- `card_number`
- `first_name`
- `last_name`
- `email`
- `phone`
- `account_status`
- `reputation_points`
- `is_blocked`
- `block_reason`
- `gdpr_consent`
- `created_at`
- `updated_at`

Obsługiwane operacje:
- dodawanie czytelnika
- edycja czytelnika
- wyszukiwanie czytelnika
- pobranie szczegółów czytelnika
- blokada konta
- odblokowanie konta

Reguły:
- `card_number` jest generowany automatycznie (`CARD-000123`)
- e-mail jest walidowany i musi być unikalny
- `account_status` jest enumem (`ACTIVE`, `SUSPENDED`, `CLOSED`)
- operacje są logowane przez `readers::ReaderService`

Miejsce pod kolejne moduły:
- `reader_loan_history` (historia wypożyczeń)
- `reader_notes` (notatki o czytelniku)

## Moduł reputacji czytelników

Założenia:
- oddanie książki w terminie zwiększa reputację
- oddanie po przedłużeniu daje mniejszy bonus
- opóźnienie obniża reputację
- kara naliczana jest za każdy pełny tydzień zwłoki
- przy niskiej reputacji czytelnik jest automatycznie blokowany

Konfigurowalne parametry (`reputation::ReputationConfig`):
- `on_time_return_bonus`
- `extended_return_bonus`
- `late_penalty_per_full_week`
- `auto_block_threshold`
- `auto_block_reason`

Historia zmian reputacji (`reader_reputation_history`):
- `reader_id`
- `change_value`
- `reason`
- `related_loan_id`
- `created_at`

API modułu:
- pobranie aktualnej reputacji czytelnika
- pobranie historii reputacji
- automatyczna aktualizacja reputacji przy zwrocie książki (`LoanReturnEvent`)
- ręczna korekta reputacji przez operatora/admina

Integracja:
- moduł aktualizuje pola `readers.reputation_points`, `readers.is_blocked`, `readers.block_reason`
- posiada punkt integracyjny pod moduł wypożyczeń przez `apply_reputation_on_loan_return(...)`

## Moduł notatek

Notatki mogą być przypisane do:
- czytelnika (`READER`)
- książki (`BOOK`)
- egzemplarza (`COPY`)
- wypożyczenia (`LOAN`)

Model notatki (`notes::Note`) zawiera:
- `id`
- `public_id`
- `target_type`
- `target_id`
- `author`
- `created_at`
- `content`
- `is_archived`
- `archived_at`

Obsługiwane operacje:
- dodanie notatki
- pobranie notatek dla obiektu (`target_type` + `target_id`)
- archiwizacja notatki

Reguły:
- `target_type` jest enumem
- `public_id` notatki jest generowany automatycznie (`NOTE-YYYY-NNNNNN`)
- notatki są generyczne i mogą być używane przez różne moduły bez duplikowania logiki

## Moduł rezerwacji

System obsługuje:
- utworzenie rezerwacji
- anulowanie rezerwacji
- wygaśnięcie rezerwacji
- powiązanie rezerwacji z książką lub egzemplarzem

Model rezerwacji (`reservations::Reservation`) zawiera:
- `id`
- `public_id`
- `reader_id`
- `copy_id` lub `book_id`
- `reservation_date`
- `expiration_date`
- `status`
- `created_at`
- `updated_at`

Statusy:
- `ACTIVE`
- `CANCELLED`
- `EXPIRED`
- `FULFILLED` (pod przyszły workflow realizacji)

Zasady działania:
- czytelnik z blokadą nie może tworzyć rezerwacji
- rezerwacja musi wskazywać dokładnie jeden cel (`copy_id` albo `book_id`)
- dla rezerwacji egzemplarza walidowany jest status egzemplarza (blokowane m.in. `RESERVED`, `ARCHIVED`, `LOST`, `IN_REPAIR`)
- system potrafi wykryć najstarszą aktywną rezerwację dla zwróconego egzemplarza (najpierw po `copy_id`, potem po `book_id`)

Integracja:
- moduł korzysta z danych `readers`, `books`, `book_copies`
- przygotowane API może być użyte przez przyszły moduł wypożyczeń przy obsłudze zwrotu

## Konwencja identyfikatorów

Generator obsługuje formaty:
- `BOOK-2026-000001`
- `COPY-2026-000001`
- `CARD-000123`
- `LOAN-2026-000991`
- `RES-2026-000010`
- `RPT-2026-000120`
- `LOC-2026-000001`
- `NOTE-2026-000001`

Zasady:
- prefiks identyfikuje domenę (`BOOK`, `COPY`, `CARD`, `LOAN`, `RES`, `RPT`, `LOC`, `NOTE`)
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
