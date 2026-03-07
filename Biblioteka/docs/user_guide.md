# Biblioteka - przewodnik użytkownika TUI

## 1. Cel aplikacji
`Biblioteka` to terminalowy system do obsługi biblioteki (książki, egzemplarze, czytelnicy, wypożyczenia/rezerwacje, lokalizacje, inwentaryzacja, raporty, notatki, logi zdarzeń).

Warstwa TUI jest adapterem nad fasadą `Library` i nie zawiera logiki domenowej.

## 2. Budowanie (CMake)
Wymagania:
- CMake >= 3.20
- kompilator C++20
- SQLite3

Kroki:

```bash
cd Biblioteka
cmake -S . -B build
cmake --build build
```

## 3. Uruchomienie
```bash
./build/cpp_biblioteka
```

Aplikacja tworzy/otwiera bazę `library.db`, inicjalizuje schemat i uruchamia pętlę TUI.

## 4. Nawigacja i obsługa wejścia
TUI działa w trybie wejścia liniowego (`std::getline`): po wpisaniu komendy naciskasz `Enter`.

Podstawowe klawisze/komendy:
- `strzałka góra` / `w` -> ruch w górę
- `strzałka dół` / `s` -> ruch w dół
- pusty `Enter` -> akcja domyślna (np. wejście/szczegóły/zapis)
- `b` -> „back” (obsługiwane globalnie i na części ekranów)
- `q` -> wyjście/powrót (zależnie od ekranu)

W wielu ekranach `/` uruchamia tryb filtra; właściwy tekst filtra podajesz w następnym wejściu.

## 5. Ekran startowy (Dashboard)
Po starcie aktywny jest ekran `dashboard`.

Menu główne:
- Dashboard
- Ksiazki
- Egzemplarze
- Czytelnicy
- Wypozyczenia
- Rezerwacje
- Lokalizacje
- Inwentaryzacja
- Raporty
- Notatki
- Logi zdarzen
- Wyjscie

Dashboard pokazuje metryki:
- liczba książek
- liczba czytelników
- liczba przetrzymanych książek (na podstawie raportu)
- liczba egzemplarzy: placeholder
- aktywne wypożyczenia: placeholder

## 6. Dostępne ekrany i funkcje

### 6.1 Książki
Ekrany:
- `books` (lista)
- `book_details`
- `book_form`

Lista (`books`):
- `Enter` szczegóły
- `a` dodawanie
- `e` edycja zaznaczonej
- `/` filtr/wyszukiwanie
- `f` przełączanie `include_archived`
- `q`/`esc` powrót do dashboardu

Wyszukiwanie:
- `t:<fraza>` tytuł
- `a:<fraza>` autor
- `i:<fraza>` ISBN
- `clear` reset filtrów
- bez prefiksu: domyślnie tytuł

Formularz (`book_form`):
- pola: tytuł, autor, ISBN, wydawnictwo, rok, opis
- `góra/dół` wybór pola, wpisanie tekstu ustawia wartość pola
- pusty `Enter` zapis

### 6.2 Egzemplarze
Ekrany:
- `copies`, `copy_details`, `copy_form`, `copy_status`, `copy_location`

Lista (`copies`):
- `Enter` szczegóły
- `a` dodanie egzemplarza
- `s` zmiana statusu
- `l` zmiana lokalizacji
- `/` filtr książki (fraza lub `clear`)
- `f` cykl filtra statusu (`ALL`, `ON_SHELF`, `LOANED`, `RESERVED`, `IN_REPAIR`, `ARCHIVED`, `LOST`)

Zmiana statusu (`copy_status`):
- `góra/dół` wybór statusu
- `n:<tekst>` note
- `r:<tekst>` archival reason
- pusty `Enter` zapis

Zmiana lokalizacji (`copy_location`):
- pola: current location public_id, target location public_id, note
- puste / `-` w polach lokalizacji = brak wartości

### 6.3 Czytelnicy
Ekrany:
- `readers`, `reader_details`, `reader_form`
- `reader_block_dialog`, `reader_unblock_dialog`
- `reader_reputation`, `reader_loan_history`

Lista (`readers`):
- `Enter` szczegóły
- `a` dodawanie
- `e` edycja
- `b` blokada
- `u` odblokowanie
- `h` historia wypożyczeń
- `r` reputacja
- `/` wyszukiwanie

Wyszukiwanie:
- `f:<fraza>` imię
- `l:<fraza>` nazwisko
- `c:<fraza>` numer karty
- `e:<fraza>` email
- `clear` reset
- bez prefiksu: domyślnie nazwisko

Formularz (`reader_form`):
- pola: imię, nazwisko, email, telefon, GDPR consent, status konta
- przy dodawaniu status konta nie jest wysyłany (ustawiany domenowo)
- przy edycji status konta jest aktualizowany

Dialogi:
- blokada: wpisanie powodu + `Enter`
- odblokowanie: `Enter` potwierdza

### 6.4 Wypożyczenia
Ekrany:
- `loans`, `loan_details`, `loan_create`, `loan_return_dialog`, `loan_extend_dialog`

Lista (`loans`):
- `Enter` szczegóły
- `a` nowe wypożyczenie
- `r` zwrot
- `p` przedłużenie
- `h` przełączanie: aktywne/historia
- `/` wyszukiwanie

Wyszukiwanie:
- wolna fraza (łączone wyszukiwanie)
- lub prefiksy: `reader:`, `copy:`, `card:`, `inv:`
- `clear` czyści filtr

Tworzenie (`loan_create`):
- Reader: `public_id` czytelnika lub numer karty
- Copy: `public_id` egzemplarza lub `inventory_number`
- termin zwrotu: `YYYY-MM-DD`

Zwrot (`loan_return_dialog`):
- `y` potwierdź
- `n`/`q` anuluj

Przedłużenie (`loan_extend_dialog`):
- wpisz nową datę
- pusty `Enter` zapis

### 6.5 Rezerwacje
Ekrany:
- `reservations`, `reservation_details`, `reservation_create`

Lista (`reservations`):
- `Enter` szczegóły
- `a` nowa rezerwacja
- `c` anulowanie zaznaczonej
- `f` cykl filtra statusu (`ALL`, `ACTIVE`, `CANCELLED`, `EXPIRED`, `FULFILLED`)

Tworzenie (`reservation_create`):
- Reader: `public_id` lub card number
- Target type: `copy` albo `book`
- Target token: dla copy `public_id` lub inventory, dla book `public_id` lub ISBN
- data wygaśnięcia: `YYYY-MM-DD`

### 6.6 Lokalizacje
Ekrany:
- `locations`, `location_details`

Drzewo (`locations`):
- `góra/dół` nawigacja
- `Enter` szczegóły lokalizacji

Szczegóły (`location_details`):
- dane lokalizacji + lista przypisanych egzemplarzy (widok ograniczony do 20)

### 6.7 Inwentaryzacja
Ekrany:
- `inventory`, `inventory_session`, `inventory_result`

Lista sesji (`inventory`):
- `Enter` otwarcie sesji (aktywna -> ekran sesji, zakończona -> wynik)
- `a` nowa sesja

Sesja (`inventory_session`):
- tryb startu: wybór lokalizacji + `Enter`
- `u:<operator>` zmiana operatora przed startem sesji
- aktywna sesja: `s:<scan_code>` dodanie skanu
- `f` zakończenie sesji i przejście do wyniku

Wynik (`inventory_result`):
- podsumowanie + liczebności `ON_SHELF/JUSTIFIED/MISSING`
- lista braków (do 15 pozycji)

### 6.8 Raporty
Ekrany:
- `reports`, `report_result`

Menu raportów (`reports`):
- `góra/dół` wybór typu
- `Enter` generowanie
- `/` filtr dat

Obsługiwane raporty:
- OverdueBooks
- DelinquentReaders
- MostBorrowedBooks
- InventoryState
- ArchivedBooks
- CopiesInRepair

Filtr dat:
- `from:YYYY-MM-DD`
- `to:YYYY-MM-DD`
- `clear`

### 6.9 Notatki
Ekran:
- `notes`

Funkcje:
- lista notatek
- szczegóły notatki dla zaznaczonego wiersza
- `/` filtr po typie/ID celu
- dodawanie notatki komendą tekstową

Filtr:
- `reader:<id>`
- `book:<id>`
- `copy:<id>`
- `loan:<id>`
- `clear`

Dodanie notatki:
- `a target:<reader|book|copy|loan>:<id> author:<nazwa> text:<treść>`

### 6.10 Logi zdarzeń (Audit)
Ekran:
- `logs`

Funkcje:
- lista wpisów audytu
- podgląd szczegółów zaznaczonego wpisu
- `/` filtr logów

Filtr:
- `op:<typ_operacji>`
- `date:YYYY-MM-DD`
- `module:<system|books|copies|readers|loans|inventory|supply|export|import>`
- `clear`

## 7. Akcje użytkownika (podsumowanie)
- Nawigacja: `strzałki` lub `w/s`
- Wybór opcji/otwarcie detalu: pusty `Enter`
- Powrót: najczęściej `q`, `esc`, `b` (zależnie od ekranu)
- Wyszukiwanie/filtr: najczęściej `/`, potem wejście tekstowe
- Dodawanie/edycja: zwykle `a`/`e`, potem formularz lub komenda

## 8. Komunikaty błędów i ich źródło
Ekrany TUI przechwytują wyjątki i mapują je przez `errors::to_user_message(...)` na komunikaty użytkownika (status bar).

Typowe źródła błędów:
- walidacja danych wejściowych (np. brak wymaganych pól, niejednoznaczny token)
- obiekt nie istnieje (np. nieznany `public_id`)
- ograniczenia domenowe (statusy, reguły procesu)
- błędy SQLite/dostępu do danych

## 9. Znane ograniczenia
- Dashboard ma placeholdery dla liczby egzemplarzy i liczby aktywnych wypożyczeń.
- TUI nie udostępnia pełnego UI dla modułów `imports`, `exports`, `search` (poza użyciem technicznym w kontrolerach).
- Ekran notatek nie ma osobnego formularza; dodawanie jest przez jedną komendę tekstową.
- Część ekranów pokazuje tylko fragment dużych list (np. lokalizacje/rekordy raportów).

## 10. Dalszy rozwój
- Dodać pełne ekrany TUI dla importu/eksportu i globalnego wyszukiwania.
- Dodać licznik egzemplarzy i aktywnych wypożyczeń do fasady `Library`, aby usunąć placeholdery dashboardu.
- Rozszerzyć ergonomię wejścia (np. bardziej interaktywna edycja pól i podpowiedzi kontekstowe).
- Dodać paginację i sortowanie dla dużych list (raporty, logi, notatki).
