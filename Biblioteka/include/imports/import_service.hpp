#pragma once

#include "audit/audit_service.hpp"
#include "books/book_service.hpp"
#include "common/system_id.hpp"
#include "imports/import_parser.hpp"
#include "imports/import_repository.hpp"
#include "readers/reader_service.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace imports {

class ImportService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    ImportService(ImportRepository& repository,
                  books::BookService& book_service,
                  readers::ReaderService& reader_service,
                  audit::AuditService& audit_service,
                  common::SystemIdGenerator& id_generator,
                  const std::vector<ImportParser*>& parsers,
                  OperationLogger logger = {});

    ImportReport import_data(const ImportRequest& request);
    ImportReport get_import_report(const std::string& public_id) const;

private:
    static std::string normalize_text(std::string value);
    static std::vector<std::string> split_tokens(const std::string& raw);

    static void validate_request(const ImportRequest& request);
    static std::unordered_map<std::string, std::size_t> build_header_index(const std::vector<std::string>& header);

    const ImportParser& resolve_parser(ImportFormat format) const;
    void ensure_sequence_initialized(int year);

    books::CreateBookInput to_book_input(const std::unordered_map<std::string, std::size_t>& header_index,
                                         const ParsedImportRow& row) const;
    readers::CreateReaderInput to_reader_input(const std::unordered_map<std::string, std::size_t>& header_index,
                                               const ParsedImportRow& row) const;

    void log_audit_start(const ImportRun& run) const;
    void log_audit_finish(const ImportRun& run) const;

    ImportRepository& repository_;
    books::BookService& book_service_;
    readers::ReaderService& reader_service_;
    audit::AuditService& audit_service_;
    common::SystemIdGenerator& id_generator_;
    std::unordered_map<ImportFormat, const ImportParser*> parsers_;
    OperationLogger logger_;
    mutable std::unordered_set<int> initialized_years_;
};

} // namespace imports
