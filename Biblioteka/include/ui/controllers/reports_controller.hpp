#pragma once

#include <optional>
#include <string>
#include <vector>

#include "reports/report.hpp"

class Library;

namespace ui::controllers {

struct ReportFilterState {
    std::optional<std::string> from_date;
    std::optional<std::string> to_date;
    int limit{120};
};

struct ReportResultData {
    reports::ReportType type{reports::ReportType::OverdueBooks};
    std::string title;
    std::vector<std::string> rows;
};

class ReportsController {
public:
    explicit ReportsController(Library& library);

    [[nodiscard]] ReportResultData generate_report(reports::ReportType type, const ReportFilterState& filter) const;

    void set_last_result(ReportResultData result);
    [[nodiscard]] const std::optional<ReportResultData>& last_result() const;

private:
    [[nodiscard]] reports::ReportQueryOptions make_options(const ReportFilterState& filter) const;

    Library& library_;
    std::optional<ReportResultData> last_result_;
};

} // namespace ui::controllers
