#pragma once

#include "imports/import.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace imports {

class ImportRepository {
public:
    virtual ~ImportRepository() = default;

    virtual ImportRun create_run(const ImportRun& run) = 0;
    virtual ImportRun complete_run(const std::string& run_public_id,
                                   ImportStatus status,
                                   int valid_records,
                                   int invalid_records) = 0;
    virtual ImportRecordError add_error(const ImportRecordError& error) = 0;

    virtual std::optional<ImportRun> get_run_by_public_id(const std::string& public_id) const = 0;
    virtual std::vector<ImportRecordError> list_errors_for_run(const std::string& run_public_id) const = 0;

    virtual std::uint64_t next_public_sequence(int year) const = 0;
};

} // namespace imports
