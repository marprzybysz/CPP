#include "imports/import_service.hpp"

#include "errors/app_error.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace imports {

namespace {

void default_logger(const std::string& message) {
    std::clog << "[imports] " << message << '\n';
}

std::string require_cell(const std::unordered_map<std::string, std::size_t>& header_index,
                         const ParsedImportRow& row,
                         const std::string& key,
                         bool required) {
    const auto it = header_index.find(key);
    if (it == header_index.end()) {
        if (required) {
            throw errors::ImportError("Missing required CSV column: " + key);
        }
        return "";
    }

    const std::size_t idx = it->second;
    if (idx >= row.values.size()) {
        return "";
    }

    return row.values[idx];
}

std::optional<int> parse_optional_int(const std::string& raw) {
    if (raw.empty()) {
        return std::nullopt;
    }

    try {
        return std::stoi(raw);
    } catch (...) {
        throw errors::ValidationError("invalid integer value: " + raw);
    }
}

bool parse_bool_loose(const std::string& raw) {
    if (raw.empty()) {
        return false;
    }

    std::string lowered = raw;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "y";
}

} // namespace

ImportService::ImportService(ImportRepository& repository,
                             books::BookService& book_service,
                             readers::ReaderService& reader_service,
                             audit::AuditService& audit_service,
                             common::SystemIdGenerator& id_generator,
                             const std::vector<ImportParser*>& parsers,
                             OperationLogger logger)
    : repository_(repository),
      book_service_(book_service),
      reader_service_(reader_service),
      audit_service_(audit_service),
      id_generator_(id_generator),
      logger_(std::move(logger)) {
    if (!logger_) {
        logger_ = default_logger;
    }

    for (const ImportParser* parser : parsers) {
        if (parser == nullptr) {
            continue;
        }
        parsers_[parser->format()] = parser;
    }
}

ImportReport ImportService::import_data(const ImportRequest& request) {
    validate_request(request);

    ImportRun run;
    run.format = request.format;
    run.target = request.target;
    run.status = ImportStatus::InProgress;
    run.source = normalize_text(request.source);
    run.operator_name = normalize_text(request.operator_name);

    const int year = common::current_year();
    ensure_sequence_initialized(year);
    run.public_id = id_generator_.generate_for_year(common::IdType::Import, year);

    run = repository_.create_run(run);
    log_audit_start(run);

    int valid_records = 0;
    int invalid_records = 0;

    try {
        const ImportParser& parser = resolve_parser(request.format);
        const std::vector<std::string> header = parser.parse_header(run.source);
        const std::unordered_map<std::string, std::size_t> header_index = build_header_index(header);
        const std::vector<ParsedImportRow> rows = parser.parse_rows(run.source);

        for (const auto& row : rows) {
            try {
                if (run.target == ImportTarget::Books) {
                    const books::CreateBookInput input = to_book_input(header_index, row);
                    (void)book_service_.add_book(input);
                } else if (run.target == ImportTarget::Readers) {
                    const readers::CreateReaderInput input = to_reader_input(header_index, row);
                    (void)reader_service_.add_reader(input);
                } else {
                    throw errors::ImportError("Unsupported import target");
                }

                ++valid_records;
            } catch (const std::exception& ex) {
                ++invalid_records;

                ImportRecordError error;
                error.run_public_id = run.public_id;
                error.row_number = row.row_number;
                error.message = ex.what();
                error.raw_record = row.raw_record;
                (void)repository_.add_error(error);
            }
        }

        const ImportStatus status = (invalid_records > 0) ? ImportStatus::CompletedWithErrors : ImportStatus::Completed;
        run = repository_.complete_run(run.public_id, status, valid_records, invalid_records);
    } catch (...) {
        run = repository_.complete_run(run.public_id, ImportStatus::Failed, valid_records, invalid_records);
        log_audit_finish(run);
        throw;
    }

    log_audit_finish(run);
    logger_("import finished public_id=" + run.public_id + " valid=" + std::to_string(run.valid_records) +
            " invalid=" + std::to_string(run.invalid_records));

    return get_import_report(run.public_id);
}

ImportReport ImportService::get_import_report(const std::string& public_id) const {
    const std::string normalized_public_id = normalize_text(public_id);
    if (normalized_public_id.empty()) {
        throw errors::ValidationError("import report public_id is required");
    }

    const auto run = repository_.get_run_by_public_id(normalized_public_id);
    if (!run.has_value()) {
        throw errors::NotFoundError("Import run not found. public_id=" + normalized_public_id);
    }

    ImportReport report;
    report.run = *run;
    report.errors = repository_.list_errors_for_run(normalized_public_id);
    return report;
}

std::string ImportService::normalize_text(std::string value) {
    auto not_space = [](unsigned char ch) { return std::isspace(ch) == 0; };

    const auto begin = std::find_if(value.begin(), value.end(), not_space);
    if (begin == value.end()) {
        return "";
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    return std::string(begin, end);
}

std::vector<std::string> ImportService::split_tokens(const std::string& raw) {
    std::vector<std::string> out;
    std::string current;

    for (char ch : raw) {
        if (ch == ',' || ch == ';' || ch == '|') {
            const std::string token = normalize_text(current);
            if (!token.empty()) {
                out.push_back(token);
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    const std::string token = normalize_text(current);
    if (!token.empty()) {
        out.push_back(token);
    }

    return out;
}

void ImportService::validate_request(const ImportRequest& request) {
    if (normalize_text(request.source).empty()) {
        throw errors::ValidationError("import source is required");
    }
    if (normalize_text(request.operator_name).empty()) {
        throw errors::ValidationError("import operator_name is required");
    }
}

std::unordered_map<std::string, std::size_t> ImportService::build_header_index(const std::vector<std::string>& header) {
    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < header.size(); ++i) {
        std::string key = normalize_text(header[i]);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (!key.empty()) {
            index[key] = i;
        }
    }
    return index;
}

const ImportParser& ImportService::resolve_parser(ImportFormat format) const {
    const auto it = parsers_.find(format);
    if (it == parsers_.end() || it->second == nullptr) {
        throw errors::ImportError("Import format is not supported yet: " + to_string(format));
    }

    return *it->second;
}

void ImportService::ensure_sequence_initialized(int year) {
    if (initialized_years_.find(year) != initialized_years_.end()) {
        return;
    }

    const std::uint64_t next = repository_.next_public_sequence(year);
    id_generator_.set_next_sequence(common::IdType::Import, year, next);
    initialized_years_.insert(year);
}

books::CreateBookInput ImportService::to_book_input(const std::unordered_map<std::string, std::size_t>& header_index,
                                                    const ParsedImportRow& row) const {
    books::CreateBookInput input;

    input.title = normalize_text(require_cell(header_index, row, "title", true));
    input.author = normalize_text(require_cell(header_index, row, "author", true));
    input.isbn = normalize_text(require_cell(header_index, row, "isbn", true));
    input.categories = split_tokens(require_cell(header_index, row, "categories", false));
    input.tags = split_tokens(require_cell(header_index, row, "tags", false));
    input.edition = normalize_text(require_cell(header_index, row, "edition", false));
    input.publisher = normalize_text(require_cell(header_index, row, "publisher", false));
    input.description = normalize_text(require_cell(header_index, row, "description", false));
    input.publication_year = parse_optional_int(normalize_text(require_cell(header_index, row, "publication_year", false)));

    if (input.title.empty() || input.author.empty() || input.isbn.empty()) {
        throw errors::ValidationError("book import row requires title, author and isbn");
    }

    return input;
}

readers::CreateReaderInput ImportService::to_reader_input(const std::unordered_map<std::string, std::size_t>& header_index,
                                                          const ParsedImportRow& row) const {
    readers::CreateReaderInput input;

    input.first_name = normalize_text(require_cell(header_index, row, "first_name", true));
    input.last_name = normalize_text(require_cell(header_index, row, "last_name", true));
    input.email = normalize_text(require_cell(header_index, row, "email", true));

    const std::string phone = normalize_text(require_cell(header_index, row, "phone", false));
    if (!phone.empty()) {
        input.phone = phone;
    }

    const std::string gdpr = normalize_text(require_cell(header_index, row, "gdpr_consent", false));
    input.gdpr_consent = parse_bool_loose(gdpr);

    if (input.first_name.empty() || input.last_name.empty() || input.email.empty()) {
        throw errors::ValidationError("reader import row requires first_name, last_name and email");
    }

    return input;
}

void ImportService::log_audit_start(const ImportRun& run) const {
    audit::AuditLogInput audit_input;
    audit_input.module = audit::AuditModule::Import;
    audit_input.actor = run.operator_name;
    audit_input.object_type = "IMPORT_RUN";
    audit_input.object_public_id = run.public_id;
    audit_input.operation_type = "START";
    audit_input.details = "target=" + to_string(run.target) + "; format=" + to_string(run.format) + "; source=" + run.source;
    (void)audit_service_.log_event(audit_input);
}

void ImportService::log_audit_finish(const ImportRun& run) const {
    audit::AuditLogInput audit_input;
    audit_input.module = audit::AuditModule::Import;
    audit_input.actor = run.operator_name;
    audit_input.object_type = "IMPORT_RUN";
    audit_input.object_public_id = run.public_id;
    audit_input.operation_type = "FINISH";

    std::ostringstream details;
    details << "status=" << to_string(run.status) << "; valid=" << run.valid_records << "; invalid=" << run.invalid_records;
    audit_input.details = details.str();

    (void)audit_service_.log_event(audit_input);
}

} // namespace imports
