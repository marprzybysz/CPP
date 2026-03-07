#pragma once

#include "db.hpp"
#include "imports/import_repository.hpp"

namespace imports {

class SqliteImportRepository final : public ImportRepository {
public:
    explicit SqliteImportRepository(Db& db) : db_(db) {}

    ImportRun create_run(const ImportRun& run) override;
    ImportRun complete_run(const std::string& run_public_id,
                           ImportStatus status,
                           int valid_records,
                           int invalid_records) override;
    ImportRecordError add_error(const ImportRecordError& error) override;

    std::optional<ImportRun> get_run_by_public_id(const std::string& public_id) const override;
    std::vector<ImportRecordError> list_errors_for_run(const std::string& run_public_id) const override;

    std::uint64_t next_public_sequence(int year) const override;

private:
    Db& db_;
};

} // namespace imports
