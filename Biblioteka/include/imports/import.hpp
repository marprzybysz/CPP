#pragma once

#include <optional>
#include <string>
#include <vector>

namespace imports {

enum class ImportFormat {
    Csv,
    Excel,
    Database,
};

enum class ImportTarget {
    Books,
    Readers,
};

enum class ImportStatus {
    InProgress,
    Completed,
    CompletedWithErrors,
    Failed,
};

struct ImportRecordError {
    std::optional<int> id;
    std::string run_public_id;
    int row_number{0};
    std::string message;
    std::string raw_record;
};

struct ImportRun {
    std::optional<int> id;
    std::string public_id;
    ImportFormat format{ImportFormat::Csv};
    ImportTarget target{ImportTarget::Books};
    ImportStatus status{ImportStatus::InProgress};
    std::string source;
    std::string operator_name;
    int valid_records{0};
    int invalid_records{0};
    std::string started_at;
    std::optional<std::string> finished_at;
};

struct ImportReport {
    ImportRun run;
    std::vector<ImportRecordError> errors;
};

struct ImportRequest {
    ImportFormat format{ImportFormat::Csv};
    ImportTarget target{ImportTarget::Books};
    std::string source;
    std::string operator_name;
};

struct ParsedImportRow {
    int row_number{0};
    std::string raw_record;
    std::vector<std::string> values;
};

[[nodiscard]] std::string to_string(ImportFormat format);
[[nodiscard]] ImportFormat import_format_from_string(const std::string& raw);

[[nodiscard]] std::string to_string(ImportTarget target);
[[nodiscard]] ImportTarget import_target_from_string(const std::string& raw);

[[nodiscard]] std::string to_string(ImportStatus status);
[[nodiscard]] ImportStatus import_status_from_string(const std::string& raw);

} // namespace imports
