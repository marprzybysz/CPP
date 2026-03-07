#include "imports/csv_import_parser.hpp"

#include "errors/app_error.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace imports {

namespace {

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];

        if (ch == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (ch == ',' && !in_quotes) {
            fields.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    if (in_quotes) {
        throw errors::ImportError("Invalid CSV line: unclosed quote");
    }

    fields.push_back(current);
    return fields;
}

std::vector<std::string> read_non_empty_lines(const std::string& source) {
    std::ifstream input(source);
    if (!input.is_open()) {
        throw errors::ImportError("Cannot open CSV source: " + source);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        lines.push_back(line);
    }

    return lines;
}

} // namespace

std::vector<std::string> CsvImportParser::parse_header(const std::string& source) const {
    const std::vector<std::string> lines = read_non_empty_lines(source);
    if (lines.empty()) {
        throw errors::ImportError("CSV source is empty: " + source);
    }

    return split_csv_line(lines.front());
}

std::vector<ParsedImportRow> CsvImportParser::parse_rows(const std::string& source) const {
    const std::vector<std::string> lines = read_non_empty_lines(source);
    if (lines.empty()) {
        return {};
    }

    std::vector<ParsedImportRow> rows;
    for (std::size_t i = 1; i < lines.size(); ++i) {
        ParsedImportRow row;
        row.row_number = static_cast<int>(i + 1);
        row.raw_record = lines[i];
        row.values = split_csv_line(lines[i]);
        rows.push_back(std::move(row));
    }

    return rows;
}

} // namespace imports
