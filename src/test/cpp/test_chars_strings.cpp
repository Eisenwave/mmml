#include <iostream>
#include <string_view>

#include <gtest/gtest.h>

#include "mmml/parse_utils.hpp"
#include "mmml/util/chars.hpp"
#include "mmml/util/strings.hpp"

namespace mmml {

std::ostream& operator<<(std::ostream& out, Blank_Line blank) // NOLINT(misc-use-internal-linkage)
{
    return out << "Blank_Line{.begin = " << blank.begin << ", .length = " << blank.length << "}";
}

namespace {

using namespace std::literals;

TEST(Chars, is_ascii_digit)
{
    EXPECT_FALSE(is_ascii_digit(u8'a'));
    for (char8_t digit = u8'0'; digit <= u8'9'; ++digit) {
        EXPECT_TRUE(is_ascii_digit(u8'0'));
    }

    EXPECT_FALSE(is_ascii_digit(U'a'));
    for (char32_t digit = U'0'; digit <= U'9'; ++digit) {
        EXPECT_TRUE(is_ascii_digit(U'0'));
    }
}

TEST(Chars, is_ascii_lower_alpha)
{
    for (char8_t c : all_ascii_lower_alpha8) {
        EXPECT_TRUE(is_ascii_lower_alpha(c));
    }
    for (char8_t c : all_ascii_upper_alpha8) {
        EXPECT_FALSE(is_ascii_lower_alpha(c));
    }
    for (char8_t c : all_ascii_digit8) {
        EXPECT_FALSE(is_ascii_lower_alpha(c));
    }

    for (char32_t c : all_ascii_lower_alpha) {
        EXPECT_TRUE(is_ascii_lower_alpha(c));
    }
    for (char32_t c : all_ascii_upper_alpha) {
        EXPECT_FALSE(is_ascii_lower_alpha(c));
    }
    for (char32_t c : all_ascii_digit) {
        EXPECT_FALSE(is_ascii_lower_alpha(c));
    }
}

TEST(Chars, is_ascii_upper_alpha)
{
    for (char8_t c : all_ascii_lower_alpha8) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }
    for (char8_t c : all_ascii_upper_alpha8) {
        EXPECT_TRUE(is_ascii_upper_alpha(c));
    }
    for (char8_t c : all_ascii_digit8) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }

    for (char32_t c : all_ascii_lower_alpha) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }
    for (char32_t c : all_ascii_upper_alpha) {
        EXPECT_TRUE(is_ascii_upper_alpha(c));
    }
    for (char32_t c : all_ascii_digit) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }
}

TEST(Chars, is_ascii_alpha)
{
    for (char8_t c : all_ascii_alpha8) {
        EXPECT_TRUE(is_ascii_alpha(c));
    }
    for (char8_t c : all_ascii_digit8) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }

    for (char32_t c : all_ascii_alpha) {
        EXPECT_TRUE(is_ascii_alpha(c));
    }
    for (char32_t c : all_ascii_digit) {
        EXPECT_FALSE(is_ascii_upper_alpha(c));
    }
}

TEST(Chars, is_mmml_directive_name_character)
{
    for (char32_t c : all_mmml_special) {
        EXPECT_FALSE(is_mmml_directive_name_character(c));
    }
    for (char32_t c : all_ascii_alpha) {
        EXPECT_TRUE(is_mmml_directive_name_character(c));
    }
    for (char32_t c : all_ascii_digit) {
        EXPECT_TRUE(is_mmml_directive_name_character(c));
    }
}

TEST(Charsets, all_ascii_digit8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_digit8, c), is_ascii_digit(c));
    }
}

TEST(Charsets, all_ascii_digit)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_digit, c), is_ascii_digit(c));
    }
}

TEST(Charsets, all_ascii_lower_alpha8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_lower_alpha8, c), is_ascii_lower_alpha(c));
    }
}

TEST(Charsets, all_ascii_lower_alpha)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_lower_alpha, c), is_ascii_lower_alpha(c));
    }
}

TEST(Charsets, all_ascii_upper_alpha8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_upper_alpha8, c), is_ascii_upper_alpha(c));
    }
}

TEST(Charsets, all_ascii_upper_alpha)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_upper_alpha, c), is_ascii_upper_alpha(c));
    }
}

TEST(Charsets, all_ascii_alpha8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_alpha8, c), is_ascii_alpha(c));
    }
}

TEST(Charsets, all_ascii_alpha)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_alpha, c), is_ascii_alpha(c));
    }
}

TEST(Charsets, all_ascii_alphanumeric8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_alphanumeric8, c), is_ascii_alphanumeric(c));
    }
}

TEST(Charsets, all_ascii_alphanumeric)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_alphanumeric, c), is_ascii_alphanumeric(c));
    }
}

TEST(Charsets, all_ascii_whitespace8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_whitespace8, c), is_ascii_whitespace(c));
    }
}

TEST(Charsets, all_ascii_whitespace)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_whitespace, c), is_ascii_whitespace(c));
    }
}

TEST(Charsets, all_ascii_blank8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_blank8, c), is_ascii_blank(c));
    }
}

TEST(Charsets, all_ascii_blank)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_ascii_blank, c), is_ascii_blank(c));
    }
}

TEST(Charsets, all_mmml_escapeable8)
{
    for (char8_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_mmml_escapeable8, c), is_mmml_escapeable(c));
    }
}

TEST(Charsets, all_mmml_escapeable)
{
    for (char32_t c = 0; c < 128; ++c) {
        EXPECT_EQ(contains(all_mmml_escapeable, c), is_mmml_escapeable(c));
    }
}

TEST(Strings, trim_ascii_blank_left)
{
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank_left(u8"awoo"));
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank_left(u8"\n\t\v\f\r awoo"));
    EXPECT_EQ(u8"awoo\n\t\v\f\r "sv, trim_ascii_blank_left(u8"awoo\n\t\v\f\r "));
    EXPECT_EQ(u8"awoo\n\t\v\f\r "sv, trim_ascii_blank_left(u8"\n\t\v\f\r awoo\n\t\v\f\r "));
}

TEST(Strings, trim_ascii_blank_right)
{
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank_right(u8"awoo"));
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank_right(u8"awoo\n\t\v\f\r "));
    EXPECT_EQ(u8"\n\t\v\f\r awoo"sv, trim_ascii_blank_right(u8"\n\t\v\f\r awoo"));
    EXPECT_EQ(u8"\n\t\v\f\r awoo"sv, trim_ascii_blank_right(u8"\n\t\v\f\r awoo\n\t\v\f\r "));
}

TEST(Strings, trim_ascii_blank)
{
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank(u8"awoo"));
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank(u8"awoo\n\t\v\f\r "));
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank(u8"\n\t\v\f\r awoo"));
    EXPECT_EQ(u8"awoo"sv, trim_ascii_blank(u8"\n\t\v\f\r awoo\n\t\v\f\r "));
}

TEST(Strings, is_html_tag_name)
{
    EXPECT_TRUE(is_html_tag_name(u8"tag"));
    EXPECT_TRUE(is_html_tag_name(u8"tag-"));
    EXPECT_TRUE(is_html_tag_name(u8"tag-tag"));

    EXPECT_FALSE(is_html_tag_name(u8""));
    EXPECT_FALSE(is_html_tag_name(u8"-"));
    EXPECT_FALSE(is_html_tag_name(u8"-tag"));
}

TEST(Strings, is_html_attribute_name)
{
    EXPECT_TRUE(is_html_attribute_name(u8"attr"));
    EXPECT_TRUE(is_html_attribute_name(u8"attr-"));
    EXPECT_TRUE(is_html_attribute_name(u8"data-attr"));
    EXPECT_TRUE(is_html_attribute_name(u8"att<(){}[]&ss"));

    EXPECT_FALSE(is_html_attribute_name(u8""));
    EXPECT_FALSE(is_html_attribute_name(u8"attr="));
    EXPECT_FALSE(is_html_attribute_name(u8"at>tr"));
}

TEST(Strings, is_html_unquoted_attribute_value)
{
    EXPECT_TRUE(is_html_unquoted_attribute_value(u8""));
    EXPECT_TRUE(is_html_unquoted_attribute_value(u8"value"));
    EXPECT_TRUE(is_html_unquoted_attribute_value(u8"hyphen-value"));

    EXPECT_FALSE(is_html_unquoted_attribute_value(u8"a b"));
    EXPECT_FALSE(is_html_unquoted_attribute_value(u8"attr="));
    EXPECT_FALSE(is_html_unquoted_attribute_value(u8"at>tr"));
    EXPECT_FALSE(is_html_unquoted_attribute_value(u8"'val'"));
    EXPECT_FALSE(is_html_unquoted_attribute_value(u8"\"val\""));
}

TEST(Parse_Utils, find_blank_line_sequence)
{
    EXPECT_EQ(find_blank_line_sequence(u8""), (Blank_Line { 0, 0 }));
    EXPECT_EQ(find_blank_line_sequence(u8"awoo"), (Blank_Line { 0, 0 }));
    EXPECT_EQ(find_blank_line_sequence(u8"a\nw\no\no"), (Blank_Line { 0, 0 }));

    EXPECT_EQ(find_blank_line_sequence(u8"\nawoo"), (Blank_Line { 0, 1 }));
    EXPECT_EQ(find_blank_line_sequence(u8"awoo\n  \n"), (Blank_Line { 5, 3 }));
    EXPECT_EQ(find_blank_line_sequence(u8"aw\n\noo"), (Blank_Line { 3, 1 }));
}

} // namespace
} // namespace mmml
