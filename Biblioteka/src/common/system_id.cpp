#include "common/system_id.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace common {

namespace {

constexpr int kNoYearBucket = 0;

int normalize_year_bucket(IdType type, int year) {
    const IdRule rule = rule_for(type);
    if (!rule.include_year) {
        return kNoYearBucket;
    }

    if (year <= 0) {
        throw std::invalid_argument("year must be positive for year-based identifiers");
    }

    return year;
}

} // namespace

IdRule rule_for(IdType type) {
    switch (type) {
        case IdType::Book:
            return IdRule{"BOOK", true, 6};
        case IdType::Copy:
            return IdRule{"COPY", true, 6};
        case IdType::Card:
            return IdRule{"CARD", false, 6};
        case IdType::Loan:
            return IdRule{"LOAN", true, 6};
        case IdType::Reservation:
            return IdRule{"RES", true, 6};
        case IdType::Report:
            return IdRule{"RPT", true, 6};
        case IdType::Location:
            return IdRule{"LOC", true, 6};
        case IdType::Note:
            return IdRule{"NOTE", true, 6};
        case IdType::Inventory:
            return IdRule{"INV", true, 6};
        default:
            throw std::invalid_argument("unsupported identifier type");
    }
}

std::string format_identifier(const IdRule& rule, int year, std::uint64_t sequence) {
    if (rule.prefix.empty()) {
        throw std::invalid_argument("identifier prefix cannot be empty");
    }
    if (rule.width == 0) {
        throw std::invalid_argument("identifier width must be greater than zero");
    }
    if (sequence == 0) {
        throw std::invalid_argument("identifier sequence must be greater than zero");
    }

    std::ostringstream out;
    out << rule.prefix << '-';

    if (rule.include_year) {
        if (year <= 0) {
            throw std::invalid_argument("year must be positive for year-based identifiers");
        }
        out << year << '-';
    }

    out << std::setw(static_cast<int>(rule.width)) << std::setfill('0') << sequence;
    return out.str();
}

int current_year() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t as_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &as_time_t);
#else
    localtime_r(&as_time_t, &local_tm);
#endif

    return 1900 + local_tm.tm_year;
}

SystemIdGenerator::SystemIdGenerator(YearProvider year_provider)
    : year_provider_(std::move(year_provider)) {
    if (!year_provider_) {
        throw std::invalid_argument("year provider cannot be empty");
    }
}

std::string SystemIdGenerator::generate(IdType type) {
    return generate_for_year(type, year_provider_());
}

std::string SystemIdGenerator::generate_for_year(IdType type, int year) {
    const IdRule rule = rule_for(type);
    const int bucket_year = normalize_year_bucket(type, year);
    const std::uint64_t sequence = next_sequence(type, bucket_year);

    return format_identifier(rule, year, sequence);
}

void SystemIdGenerator::set_next_sequence(IdType type, std::uint64_t next_sequence_value) {
    set_next_sequence(type, year_provider_(), next_sequence_value);
}

void SystemIdGenerator::set_next_sequence(IdType type, int year, std::uint64_t next_sequence_value) {
    if (next_sequence_value == 0) {
        throw std::invalid_argument("next sequence must be greater than zero");
    }

    const int bucket_year = normalize_year_bucket(type, year);
    const SequenceKey key{type, bucket_year};
    next_sequences_[key] = next_sequence_value;
}

std::uint64_t SystemIdGenerator::peek_next_sequence(IdType type) const {
    return peek_next_sequence(type, year_provider_());
}

std::uint64_t SystemIdGenerator::peek_next_sequence(IdType type, int year) const {
    const int bucket_year = normalize_year_bucket(type, year);
    const SequenceKey key{type, bucket_year};

    const auto it = next_sequences_.find(key);
    if (it == next_sequences_.end()) {
        return 1;
    }

    return it->second;
}

std::uint64_t SystemIdGenerator::next_sequence(IdType type, int year) {
    const SequenceKey key{type, year};
    const std::uint64_t current = peek_next_sequence(type, year);
    next_sequences_[key] = current + 1;
    return current;
}

std::size_t SystemIdGenerator::SequenceKeyHash::operator()(const SequenceKey& key) const noexcept {
    return (static_cast<std::size_t>(key.type) << 32U) ^ static_cast<std::size_t>(key.year);
}

bool SystemIdGenerator::SequenceKeyEq::operator()(const SequenceKey& lhs, const SequenceKey& rhs) const noexcept {
    return lhs.type == rhs.type && lhs.year == rhs.year;
}

} // namespace common
