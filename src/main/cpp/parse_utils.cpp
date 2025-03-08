#include <charconv>
#include <optional>

#include "mmml/util/assert.hpp"
#include "mmml/util/strings.hpp"

#include "mmml/parse_utils.hpp"

namespace mmml {

Blank_Line find_blank_line(std::u8string_view str) noexcept
{
    enum struct State {
        normal,
        maybe_blank,
        blank,
    };
    State state = State::normal;

    std::size_t blank_begin;
    std::size_t blank_end;

    for (std::size_t i = 0; i < str.size(); ++i) {
        switch (state) {
        case State::normal: {
            if (str[i] == u8'\n') {
                state = State::maybe_blank;
                blank_begin = i + 1;
            }
            continue;
        }
        case State::maybe_blank: {
            if (str[i] == u8'\n') {
                state = State::blank;
                blank_end = i;
            }
            else if (!is_ascii_whitespace(str[i])) {
                state = State::normal;
            }
            continue;
        }
        case State::blank: {
            if (str[i] == u8'\n') {
                blank_end = i;
            }
            else if (!is_ascii_whitespace(str[i])) {
                return { .begin = blank_begin, .length = blank_end - blank_begin };
            }
            continue;
        }
        }
        MMML_ASSERT_UNREACHABLE(u8"Invalid state");
    }

    static_assert(!Blank_Line {}, "A value-initialized Blank_Line should be falsy");
    return {};
}

namespace {

std::optional<unsigned long long> parse_uinteger_digits(std::u8string_view text, int base)
{
    const auto sv = as_string_view(text);
    unsigned long long value;
    std::from_chars_result result = std::from_chars(sv.begin(), sv.end(), value, base);
    if (result.ec != std::errc {}) {
        return {};
    }
    return value;
}

} // namespace

std::size_t match_digits(std::u8string_view str, int base)
{
    MMML_ASSERT((base >= 2 && base <= 10) || base == 16);
    static constexpr std::u8string_view hexadecimal_digits = u8"0123456789abcdefABCDEF";

    const std::u8string_view digits
        = base == 16 ? hexadecimal_digits : hexadecimal_digits.substr(0, std::size_t(base));

    // std::min covers the case of std::u8string_view::npos
    return std::min(str.find_first_not_of(digits), str.size());
}

Literal_Match_Result match_integer_literal(std::u8string_view s) noexcept
{
    if (s.empty() || !is_ascii_digit(s[0])) {
        return { Literal_Match_Status::no_digits, 0, {} };
    }
    if (s.starts_with(u8"0b")) {
        const std::size_t digits = match_digits(s.substr(2), 2);
        if (digits == 0) {
            return { Literal_Match_Status::no_digits_following_prefix, 2, Literal_Type::binary };
        }
        return { Literal_Match_Status::ok, digits + 2, Literal_Type::binary };
    }
    if (s.starts_with(u8"0x")) {
        const std::size_t digits = match_digits(s.substr(2), 16);
        if (digits == 0) {
            return { Literal_Match_Status::no_digits_following_prefix, 2,
                     Literal_Type::hexadecimal };
        }
        return { Literal_Match_Status::ok, digits + 2, Literal_Type::hexadecimal };
    }
    if (s[0] == '0') {
        const std::size_t digits = match_digits(s, 8);
        return { Literal_Match_Status::ok, digits,
                 digits == 1 ? Literal_Type::decimal : Literal_Type::octal };
    }
    const std::size_t digits = match_digits(s, 10);

    return { Literal_Match_Status::ok, digits, Literal_Type::decimal };
}

std::optional<unsigned long long> parse_uinteger_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return {};
    }
    if (str.starts_with(u8"0b")) {
        return parse_uinteger_digits(str.substr(2), 2);
    }
    if (str.starts_with(u8"0x")) {
        return parse_uinteger_digits(str.substr(2), 16);
    }
    if (str.starts_with(u8'0')) {
        return parse_uinteger_digits(str, 8);
    }
    return parse_uinteger_digits(str, 10);
}

std::optional<long long> parse_integer_literal(std::u8string_view str) noexcept
{
    if (str.empty()) {
        return std::nullopt;
    }
    if (str.starts_with(u8'-')) {
        if (auto positive = parse_uinteger_literal(str.substr(1))) {
            // Negating as Uint is intentional and prevents overflow.
            return static_cast<long long>(-*positive);
        }
        return std::nullopt;
    }
    if (auto result = parse_uinteger_literal(str)) {
        return static_cast<long long>(*result);
    }
    return std::nullopt;
}

} // namespace mmml
