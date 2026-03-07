# Biblioteka - dokumentacja audytu

## 1. Cel audytu
Moduł `audit` zapewnia centralny rejestr zdarzeń operacyjnych systemu:
- kto wykonał operację,
- kiedy,
- na jakim obiekcie,
- jakiego typu była operacja,
- dodatkowe szczegóły.

Audyt służy do śledzenia zmian i diagnostyki procesu bibliotecznego.

## 2. Jakie zdarzenia są rejestrowane

### 2.1 Zdarzenia logowane przez fasadę `Library`
Rejestrowane są m.in. operacje:
- książki: `CREATE`, `UPDATE`, `ARCHIVE`
- egzemplarze: `CREATE`, `UPDATE`, `CHANGE_STATUS`, `CHANGE_LOCATION`
- inwentaryzacja: `START`, `REGISTER_SCAN`, `FINISH`
- czytelnicy: `CREATE`, `UPDATE`, `BLOCK`, `UNBLOCK`
- wypożyczenia/rezerwacje: `CREATE`, `CANCEL`, `EXPIRE`, `RETURN`, `EXTEND`
- raporty: `GENERATE` (dla zapisanych snapshotów raportów)
- wycofanie egzemplarza: `WITHDRAW`

### 2.2 Zdarzenia logowane przez moduł importu
`imports::ImportService` loguje:
- `IMPORT_RUN START`
- `IMPORT_RUN FINISH`

Dla importu aktorem jest operator z żądania importu (`operator_name`), nie `system`.

### 2.3 Zdarzenia manualne
Dostępne są także metody:
- `log_audit_event(...)`
- `log_supply_event(...)`
- `log_export_event(...)`
- `log_loan_event(...)`

## 3. Model wpisu audytowego
Model domenowy: `audit::AuditEvent`.

Trwałość: tabela SQLite `audit_events`.

### 3.1 `AuditModule`
Obsługiwane moduły:
- `SYSTEM`
- `BOOKS`
- `COPIES`
- `READERS`
- `LOANS`
- `INVENTORY`
- `SUPPLY`
- `EXPORT`
- `IMPORT`

## 4. Pola wpisu audytowego
`AuditEvent` zawiera:
- `id` (opcjonalny, ustawiany po zapisie)
- `module`
- `actor`
- `occurred_at`
- `object_type`
- `object_public_id`
- `operation_type`
- `details`

W bazie:
- `id INTEGER PRIMARY KEY AUTOINCREMENT`
- `module TEXT CHECK(...) NOT NULL`
- `actor TEXT NOT NULL`
- `occurred_at TEXT NOT NULL DEFAULT (datetime('now'))`
- `object_type TEXT NOT NULL`
- `object_public_id TEXT NOT NULL`
- `operation_type TEXT NOT NULL`
- `details TEXT NOT NULL DEFAULT ''`

Indeksy:
- `idx_audit_events_module`
- `idx_audit_events_object` (`object_type`, `object_public_id`)
- `idx_audit_events_occurred_at`

## 5. Moduły korzystające z audytu
Bezpośrednio:
- `src/library.cpp` (logowanie operacji biznesowych przez `log_system_audit`)
- `src/imports/import_service.cpp` (logowanie startu/zakończenia importu)

Odczyt i prezentacja:
- `ui/controllers/audit_controller.cpp`
- `ui/screens/audit_log_screen.cpp`

## 6. Jak interpretować logi audytowe
- `module` mówi, którego obszaru systemu dotyczy zdarzenie.
- `object_type` + `object_public_id` identyfikują encję biznesową.
- `operation_type` określa rodzaj akcji (np. `CREATE`, `RETURN`, `WITHDRAW`).
- `details` zawiera kontekst tekstowy, np. nowy status, datę lub przyczynę.
- `actor` zwykle ma wartość `system` dla operacji wykonywanych przez fasadę; w imporcie jest to operator importu.

Ekran TUI `logs` wspiera filtrowanie po:
- module,
- dacie (`date:YYYY-MM-DD`, prefiks po `occurred_at`),
- fragmencie `operation_type`.

## 7. Przykłady realnych operacji audytowych
Przykłady zgodne z aktualnym kodem:
- `module=BOOKS`, `object_type=BOOK`, `operation_type=CREATE`, `details=book added`
- `module=COPIES`, `object_type=COPY`, `operation_type=CHANGE_STATUS`, `details=status changed to LOST`
- `module=READERS`, `object_type=READER`, `operation_type=BLOCK`, `details=<powód blokady>`
- `module=LOANS`, `object_type=RESERVATION`, `operation_type=EXTEND`, `details=loan extended to 2026-03-20`
- `module=INVENTORY`, `object_type=INVENTORY_SESSION`, `operation_type=REGISTER_SCAN`, `details=scan registered for copy COPY-...`
- `module=EXPORT`, `object_type=COPY`, `operation_type=WITHDRAW`, `details=reason=LOST; note=...`
- `module=IMPORT`, `object_type=IMPORT_RUN`, `operation_type=START|FINISH`, `details=...`

## 8. Ograniczenia obecnej implementacji
- Brak poziomów ważności wpisów (`severity`).
- Brak identyfikatora korelacji/trace dla łączenia wielu zdarzeń jednego procesu.
- Brak struktury JSON w `details` (pole tekstowe, format konwencyjny).
- Brak wersjonowania stanu „przed/po” dla encji.
- TUI audytu pokazuje listę i szczegóły, ale bez paginacji i bez eksportu logów.
- Brak dedykowanego UI do ręcznego tworzenia wpisów audytu.

## 9. Propozycje dalszej rozbudowy
- Ustandaryzować `details` (np. JSON ze schematem per `operation_type`).
- Dodać `trace_id` / `correlation_id` do łączenia wpisów między modułami.
- Rozszerzyć model o poziom zdarzenia (`INFO/WARN/ERROR`).
- Dodać paginację, sortowanie i eksport logów audytowych w TUI.
- Dodać politykę retencji i archiwizacji danych audytowych.
