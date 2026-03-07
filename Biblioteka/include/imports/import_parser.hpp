#pragma once

#include "imports/import.hpp"

#include <string>
#include <vector>

namespace imports {

class ImportParser {
public:
    virtual ~ImportParser() = default;

    virtual ImportFormat format() const = 0;
    virtual std::vector<std::string> parse_header(const std::string& source) const = 0;
    virtual std::vector<ParsedImportRow> parse_rows(const std::string& source) const = 0;
};

} // namespace imports
