#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace common {

enum class IdType {
    Book,
    Copy,
    Card,
    Loan,
    Report,
    Location,
    Note,
};

struct IdRule {
    std::string prefix;
    bool include_year;
    std::size_t width;
};

[[nodiscard]] IdRule rule_for(IdType type);
[[nodiscard]] std::string format_identifier(const IdRule& rule, int year, std::uint64_t sequence);
[[nodiscard]] int current_year();

class SystemIdGenerator {
public:
    using YearProvider = std::function<int()>;

    explicit SystemIdGenerator(YearProvider year_provider = current_year);

    [[nodiscard]] std::string generate(IdType type);
    [[nodiscard]] std::string generate_for_year(IdType type, int year);

    void set_next_sequence(IdType type, std::uint64_t next_sequence);
    void set_next_sequence(IdType type, int year, std::uint64_t next_sequence);

    [[nodiscard]] std::uint64_t peek_next_sequence(IdType type) const;
    [[nodiscard]] std::uint64_t peek_next_sequence(IdType type, int year) const;

private:
    [[nodiscard]] std::uint64_t next_sequence(IdType type, int year);

    YearProvider year_provider_;

    struct SequenceKey {
        IdType type;
        int year;
    };

    struct SequenceKeyHash {
        std::size_t operator()(const SequenceKey& key) const noexcept;
    };

    struct SequenceKeyEq {
        bool operator()(const SequenceKey& lhs, const SequenceKey& rhs) const noexcept;
    };

    std::unordered_map<SequenceKey, std::uint64_t, SequenceKeyHash, SequenceKeyEq> next_sequences_;
};

} // namespace common
