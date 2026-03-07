# Biblioteka

## Opis projektu
`Biblioteka` to system biblioteczny napisany w C++20. Projekt jest modułowy, oparty o wzorzec `service + repository`, a dane są przechowywane w SQLite, a warstwa TUI zapewnia terminalowy interfejs użytkownika.

System obejmuje zarządzanie katalogiem książek, egzemplarzami, czytelnikami, lokalizacją, rezerwacjami, inwentaryzacją, raportowaniem, audytem, importem danych, wycofywaniem egzemplarzy oraz globalnym wyszukiwaniem.

## Cel systemu
Celem systemu jest dostarczenie spójnego backendu do:
- ewidencji książek i egzemplarzy,
- zarządzania czytelnikami,
- obsługi procesów operacyjnych (rezerwacje, inwentaryzacja, wycofania),
- raportowania i monitoringu,
- bezpiecznej obsługi błędów,
- przygotowania pod dalszą rozbudowę (frontend, API, dodatkowe integracje).

## Architektura projektu
Główne zasady architektury:
- `Library` jest fasadą łączącą wszystkie moduły.
- Każdy moduł ma własne modele domenowe (`include/<module>`), serwis (`*Service`) i repozytorium (`*Repository`).
- Implementacje SQLite są oddzielone od logiki biznesowej (`Sqlite*Repository`).
- Logika biznesowa i walidacje są w serwisach, a mapowanie SQL w repozytoriach.
- Warstwa `ui` jest adapterem TUI nad fasadą `Library` i nie zawiera logiki domenowej.
- Identyfikatory systemowe są generowane centralnie przez `common::SystemIdGenerator`.
- Błędy domenowe i komunikaty użytkownika są obsługiwane przez moduł `errors`.

## Aktualna struktura katalogów
```text
Biblioteka/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── library.hpp
│   ├── db.hpp
│   ├── common/
│   ├── errors/
│   ├── books/
│   ├── copies/
│   ├── readers/
│   ├── reservations/
│   ├── locations/
│   ├── inventory/
│   ├── reports/
│   ├── search/
│   ├── audit/
│   ├── exports/
│   ├── imports/
│   ├── notes/
│   ├── reputation/
│   └── ui/
└── src/
    ├── main.cpp
    ├── db.cpp
    ├── library.cpp
    ├── common/
    ├── errors/
    ├── books/
    ├── copies/
    ├── readers/
    ├── reservations/
    ├── locations/
    ├── inventory/
    ├── reports/
    ├── search/
    ├── audit/
    ├── exports/
    ├── imports/
    ├── notes/
    ├── reputation/
    └── ui/
```

## Opis wszystkich modułów
- `books`: katalog książek, walidacja ISBN, archiwizacja logiczna książek.
- `copies`: egzemplarze książek, statusy (`ON_SHELF`, `LOANED`, `RESERVED`, `IN_REPAIR`, `ARCHIVED`, `LOST`), historia statusu i lokalizacji.
- `readers`: konta czytelników, walidacja e-mail, numer karty, blokady.
- `reservations` (warstwa loan): rezerwacje aktywne/anulowane/wygasłe, kolejka aktywnych rezerwacji dla egzemplarza/książki.
- `locations`: hierarchia `LIBRARY -> ROOM -> RACK -> SHELF` i przypisanie egzemplarzy.
- `inventory`: spis z natury dla `ROOM/RACK/SHELF`, klasyfikacja wyników (`ON_SHELF`, `JUSTIFIED`, `MISSING`).
- `reports`: raporty operacyjne + snapshoty wyników i baza pod eksport CSV.
- `search`: globalna wyszukiwarka po książkach/egzemplarzach/czytelnikach z informacją o statusie, lokalizacji i potencjalnym posiadaczu.
- `audit`: centralne logi zdarzeń systemowych (kto, kiedy, na jakim obiekcie, typ operacji, szczegóły).
- `exports`: wycofanie egzemplarzy z obiegu z powodami biznesowymi i zmianą statusu na `ARCHIVED`/`LOST`.
- `imports`: import danych (aktualnie CSV) dla książek i czytelników, raport błędów per rekord; architektura przygotowana pod Excel i inne źródła.
- `notes`: notatki generyczne dla obiektów domenowych.
- `reputation`: punktacja reputacji czytelników i automatyczne reguły blokad.
- `errors`: typowane wyjątki aplikacyjne, mapowanie na komunikaty użytkownika, logger błędów.
- `common`: współdzielone narzędzia (m.in. generator identyfikatorów).
- `Db`: połączenie i inicjalizacja schematu SQLite.

## Wymagania i zależności
- CMake `>= 3.20`
- Kompilator C++20 (`g++` lub `clang++`)
- SQLite3 (nagłówki + biblioteka linkowana)

## Budowanie przez CMake
```bash
cd Biblioteka
cmake -S . -B build
cmake --build build
```

## Uruchomienie
```bash
./build/cpp_biblioteka
```

Aplikacja tworzy/otwiera lokalną bazę `library.db`, inicjalizuje schemat i uruchamia pętlę TUI.

## Warstwa TUI
Szkielet TUI jest podzielony na:
- `ui/Application`: uruchamia główną pętlę terminalową,
- `ui/screens/Screen`: bazowy interfejs ekranów,
- `ui/ScreenManager`: rejestracja ekranów i przełączanie widoków,
- `ui/Renderer`: rysowanie terminala,
- `ui/style`: wspólny system stylowania tekstu ANSI,
- `ui/InputHandler`: mapowanie wejścia użytkownika na zdarzenia nawigacji,
- `ui/components/Menu`: współdzielony komponent menu,
- `ui/controllers/ApplicationController`: bootstrap ekranów,
- `ui/controllers/DashboardController`: dostarcza metryki do dashboardu,
- `ui/controllers/BooksController`: adapter przypadków użycia modułu książek,
- `ui/controllers/CopiesController`: adapter przypadków użycia modułu egzemplarzy.
- `ui/controllers/LoansController`: adapter przypadków użycia modułu wypożyczeń (na bazie `reservations`).

Wspólne komponenty TUI (`ui/components`) gotowe do użycia przez moduły książek, czytelników i wypożyczeń:
- `Header`
- `Footer`
- `StatusBar`
- `ListView`
- `TableView`
- `Dialog`
- `ConfirmDialog`
- `MessageBox`
- `FormField`
- `SearchBox`

Dostępne style tekstu:
- `header`
- `highlight`
- `error`
- `success`
- `warning`

Dashboard pokazuje:
- tytuł aplikacji,
- liczbę książek (z `Library::list_books`),
- liczbę czytelników (z `Library::search_readers`),
- liczbę przetrzymanych książek (z `Library::generate_overdue_books_report`),
- placeholdery dla liczby egzemplarzy i aktywnych wypożyczeń (do czasu wystawienia liczników przez fasadę `Library`),
- główne menu: Dashboard, Książki, Egzemplarze, Czytelnicy, Wypożyczenia, Rezerwacje, Lokalizacje, Inwentaryzacja, Raporty, Notatki, Wyjście.

Nawigacja:
- `w/s` lub strzałki góra/dół do zmiany aktywnej pozycji,
- `Enter` do wejścia w ekran,
- `b` powrót do dashboardu,
- `q` wyjście.

## Ekran TUI: Książki
Moduł ekranów książek:
- `BookListScreen`: listowanie, wyszukiwanie, filtrowanie, akcje,
- `BookDetailsScreen`: szczegóły wybranej książki,
- `BookFormScreen`: dodawanie i edycja książki.

Workflow:
1. Z `Dashboard` wybierz `Książki`.
2. W `BookListScreen` przeglądaj listę (`góra/dół`) i otwieraj szczegóły (`Enter`).
3. Dodaj nową książkę (`a`) lub edytuj zaznaczoną (`e`).
4. Wyszukiwanie aktywuj przez `/`, następnie wpisz:
   - `t:<fraza>` dla tytułu,
   - `a:<fraza>` dla autora,
   - `i:<fraza>` dla ISBN,
   - `clear` aby wyczyścić kryteria.
5. Filtrowanie (`f`) przełącza `include_archived`.
6. `q` lub `esc` wraca do poprzedniego ekranu.

## Ekran TUI: Egzemplarze
Moduł ekranów egzemplarzy:
- `CopyListScreen`: listowanie egzemplarzy i filtrowanie po książce/statusie,
- `CopyDetailsScreen`: podgląd szczegółów egzemplarza,
- `CopyFormScreen`: dodawanie nowego egzemplarza,
- `CopyStatusScreen`: zmiana statusu egzemplarza,
- `CopyLocationScreen`: zmiana aktualnej/docelowej lokalizacji.

Workflow:
1. Z `Dashboard` wybierz `Egzemplarze`.
2. W `CopyListScreen` używaj `góra/dół` do wyboru i `Enter` do szczegółów.
3. Filtrowanie po książce aktywuj przez `/` (wpisz frazę lub `clear`).
4. Filtrowanie po statusie przełączaj przez `f`.
5. Operacje na zaznaczonym egzemplarzu:
   - `s` zmiana statusu,
   - `l` zmiana lokalizacji,
   - `a` dodanie nowego egzemplarza.
6. W widoku listy i szczegółów `q`/`esc` wraca do poprzedniego ekranu.

## Ekran TUI: Czytelnicy
Moduł ekranów czytelników:
- `ReaderListScreen`: listowanie i wyszukiwanie czytelników,
- `ReaderDetailsScreen`: szczegóły konta czytelnika,
- `ReaderFormScreen`: dodawanie i edycja czytelnika,
- `ReaderBlockDialogScreen`: dialog blokady konta,
- `ReaderUnblockDialogScreen`: dialog odblokowania konta,
- `ReaderReputationScreen`: podgląd reputacji i historii zmian,
- `ReaderLoanHistoryScreen`: podgląd historii wypożyczeń.

Workflow:
1. Z `Dashboard` wybierz `Czytelnicy`.
2. W `ReaderListScreen` wyszukuj po:
   - `f:<fraza>` imię,
   - `l:<fraza>` nazwisko,
   - `c:<fraza>` numer karty,
   - `e:<fraza>` email,
   - `clear` wyczyszczenie filtrów.
3. Akcje:
   - `Enter` szczegóły,
   - `a` dodaj,
   - `e` edytuj,
   - `b` zablokuj,
   - `u` odblokuj,
   - `h` historia wypożyczeń,
   - `r` reputacja,
   - `q` powrót.
4. Widok szczegółów pokazuje: numer karty, imię i nazwisko, email, telefon, status konta, punkty reputacji i informację o blokadzie.

## Ekran TUI: Wypożyczenia
Moduł ekranów wypożyczeń:
- `LoanListScreen`: lista aktywnych wypożyczeń i historia, wyszukiwanie oraz przełączanie trybu,
- `LoanDetailsScreen`: szczegóły wybranego wypożyczenia,
- `LoanCreateScreen`: tworzenie nowego wypożyczenia,
- `LoanReturnDialogScreen`: dialog zwrotu (potwierdzenie operacji),
- `LoanExtendDialogScreen`: dialog przedłużenia terminu zwrotu.

Workflow:
1. Z `Dashboard` wybierz `Wypożyczenia`.
2. W `LoanListScreen`:
   - `a` tworzy nowe wypożyczenie,
   - `r` otwiera dialog zwrotu,
   - `p` otwiera dialog przedłużenia,
   - `Enter` przechodzi do szczegółów,
   - `/` uruchamia wyszukiwanie,
   - `h` przełącza widok `AKTYWNE`/`HISTORIA`,
   - `q` wraca do dashboardu.
3. Wyszukiwanie obsługuje:
   - `reader:<fraza>` lub `r:<fraza>`,
   - `copy:<fraza>` lub `c:<fraza>`,
   - `card:<fraza>`,
   - `inv:<fraza>`,
   - lub ogólną frazę (łączoną z modułami readers/copies).
4. Błędy biznesowe (np. `ReaderBlockedError`, `CopyUnavailableError`) są mapowane przez `errors::to_user_message(...)` i pokazywane w pasku statusu.

## Opis bazy danych SQLite
Kluczowe tabele:
- Katalog: `books`, `book_copies`, `copy_status_history`, `copy_location_history`
- Czytelnicy i rezerwacje: `readers`, `reservations`, `reader_reputation_history`, `reader_loan_history`, `reader_notes`
- Lokalizacja i inwentaryzacja: `locations`, `inventory_sessions`, `inventory_scans`, `inventory_items`
- Raportowanie i eksport: `report_snapshots`, `report_exports`
- Audyt i import: `audit_events`, `import_runs`, `import_run_errors`
- Wycofania i notatki: `copy_withdrawals`, `notes`

Wszystkie tabele są tworzone idempotentnie w `Db::init_schema()`.

## Opis identyfikatorów systemowych
Identyfikatory generuje `common::SystemIdGenerator` według typu:
- `BOOK-YYYY-NNNNNN`
- `COPY-YYYY-NNNNNN`
- `RES-YYYY-NNNNNN`
- `LOC-YYYY-NNNNNN`
- `NOTE-YYYY-NNNNNN`
- `INV-YYYY-NNNNNN`
- `RPT-YYYY-NNNNNN`
- `IMP-YYYY-NNNNNN`
- `CARD-NNNNNN` (bez roku)

Sekwencje są inicjalizowane na podstawie istniejących danych w repozytoriach.

## Opis modułu błędów
Moduł `errors` zawiera:
- `ErrorCode` z kategoriami domenowymi,
- klasy wyjątków (`ValidationError`, `NotFoundError`, `DatabaseError` i modułowe np. `ImportError`, `SearchError`),
- `to_user_message(...)` do mapowania wyjątków na bezpieczne komunikaty,
- `error_logger` do logowania technicznych szczegółów błędów.

Zasada: walidacje i reguły biznesowe rzucają wyjątki domenowe, a UI/API dostaje uproszczony komunikat.

## Zasady dalszego rozwoju
- Zachowywać podział: model domenowy / serwis / repozytorium.
- Nie mieszać logiki SQL z logiką biznesową.
- Każdy nowy moduł dodawać przez `Library` jako fasadę.
- Rozszerzać `ErrorCode` i wyjątki dla nowych domen.
- Utrzymywać spójność nazewnictwa (`public_id`, `*_service`, `sqlite_*_repository`).
- Dodawać indeksy SQLite wraz z nowymi tabelami lub ciężkimi zapytaniami.
- Każdą istotną operację biznesową logować audytowo.

## Przykładowa kolejność pracy nad frontendem i API
1. Ustalić kontrakty DTO na bazie modeli wynikowych `Library`.
2. Zbudować warstwę API (REST/GraphQL) mapującą wyjątki `errors` na kody HTTP.
3. Udostępnić najpierw moduły read-only: wyszukiwarka, raporty, szczegóły obiektów.
4. Dodać operacje mutujące: książki, egzemplarze, czytelnicy, rezerwacje.
5. Dodać procesy operacyjne: inwentaryzacja, wycofania, import.
6. Podłączyć ekran audytu i monitoringu importów/raportów.
7. Dodać testy end-to-end na kluczowe przepływy biznesowe.

## Zasady commitów
- Format: `<type>: <krótki opis>`
- Zalecane `type`: `feat`, `fix`, `refactor`, `docs`, `test`, `chore`.
- Jeden commit powinien obejmować jeden logiczny zakres zmian.
- Dla zmian schematu DB i API zawsze aktualizować README.
- Przykłady:
  - `feat: add global search module`
  - `fix: validate import row parsing for empty columns`
  - `docs: finalize project documentation and cleanup`
