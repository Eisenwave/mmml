#ifndef MMML_DIRECTIVE_PROCESSING_HPP
#define MMML_DIRECTIVE_PROCESSING_HPP

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "mmml/util/result.hpp"

#include "mmml/ast.hpp"
#include "mmml/fwd.hpp"

namespace mmml {

[[nodiscard]]
std::span<const ast::Content> trim_blank_text_left(std::span<const ast::Content>, Context&);
[[nodiscard]]
std::span<const ast::Content> trim_blank_text_right(std::span<const ast::Content>, Context&);

/// @brief Trims leading and trailing completely blank text content.
[[nodiscard]]
std::span<const ast::Content> trim_blank_text(std::span<const ast::Content>, Context&);

/// @brief Converts content to plaintext.
/// For text, this outputs that text literally.
/// For escaped characters, this outputs the escaped character.
/// For directives, this runs `generate_plaintext` using the behavior of that directive,
/// looked up via context.
/// @param context the current processing context
void to_plaintext(std::pmr::vector<char8_t>& out, const ast::Content& c, Context& context);

/// @brief Calls `to_plaintext` for each piece of content.
void to_plaintext(
    std::pmr::vector<char8_t>& out,
    std::span<const ast::Content> content,
    Context& context
);

/// @brief Like `to_plaintext`,
/// but ignores directives other than `pure_plaintext` and `formatting`, and
/// also appends the source code index of the piece of content that is responsible for each
/// character.
/// When performing syntax highlighting,
/// this subsequently allows to wrap text in synthesized directives.
///
/// Note that for directives with `Directive_Category::pure_html` or `Directive_Category::mixed`,
/// no plaintext is generated in general.
void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char8_t>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Content& c,
    Context& context
);

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char8_t>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Text& t,
    Context& context
);

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char8_t>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Escaped& e,
    Context& context
);

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char8_t>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Directive& d,
    Context& context
);

/// @brief Calls `to_plaintext_mapped_for_highlighting` for each piece of content.
void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char8_t>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    std::span<const ast::Content> content,
    Context& context
);

void to_html(HTML_Writer& out, const ast::Content&, Context&);
void to_html(HTML_Writer& out, const ast::Escaped&, Context&);
void to_html(HTML_Writer& out, const ast::Text&, Context&);
void to_html(HTML_Writer& out, const ast::Directive&, Context&);
void to_html(HTML_Writer& out, const ast::Generated&, Context&);

enum struct To_HTML_Mode : Default_Underlying {
    direct,
    paragraphs,
    trimmed,
    paragraphs_trimmed,
};

[[nodiscard]]
constexpr bool to_html_mode_is_trimmed(To_HTML_Mode mode)
{
    return mode == To_HTML_Mode::trimmed || mode == To_HTML_Mode::paragraphs_trimmed;
}

[[nodiscard]]
constexpr bool to_html_mode_is_paragraphed(To_HTML_Mode mode)
{
    return mode == To_HTML_Mode::paragraphs || mode == To_HTML_Mode::paragraphs_trimmed;
}

void to_html(
    HTML_Writer& out,
    std::span<const ast::Content> content,
    Context& context,
    To_HTML_Mode mode = To_HTML_Mode::direct
);

/// @brief Converts the source code of the content to HTML without any processing.
void to_html_literally(HTML_Writer& out, const ast::Content& content, Context& context);

void to_html_literally(HTML_Writer& out, std::span<const ast::Content> content, Context& context);

Result<void, Syntax_Highlight_Error> to_html_syntax_highlighted(
    HTML_Writer& out,
    std::span<const ast::Content> content,
    std::u8string_view language,
    Context& context,
    To_HTML_Mode mode = To_HTML_Mode::direct
);

void arguments_to_attributes(Attribute_Writer& out, const ast::Directive& d, Context& context);

} // namespace mmml

#endif
