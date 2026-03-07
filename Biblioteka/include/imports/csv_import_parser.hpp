#pragma once

#include "imports/import_parser.hpp"

namespace imports {

class CsvImportParser final : public ImportParser {
public:
    ImportFormat format() const override { return ImportFormat::Csv; }
    std::vector<std::string> parse_header(const std::string& source) const override;
    std::vector<ParsedImportRow> parse_rows(const std::string& source) const override;
};

} // namespace imports
