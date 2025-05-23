#ifndef COWEL_DIRECTIVE_PROCESSING_HPP
#define COWEL_DIRECTIVE_PROCESSING_HPP

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "cowel/util/function_ref.hpp"
#include "cowel/util/html_writer.hpp"
#include "cowel/util/result.hpp"

#include "cowel/ast.hpp"
#include "cowel/fwd.hpp"

namespace cowel {

[[nodiscard]]
std::span<const ast::Content> trim_blank_text_left(std::span<const ast::Content>);
[[nodiscard]]
std::span<const ast::Content> trim_blank_text_right(std::span<const ast::Content>);

/// @brief Trims leading and trailing completely blank text content.
[[nodiscard]]
std::span<const ast::Content> trim_blank_text(std::span<const ast::Content>);

enum struct To_Plaintext_Mode : Default_Underlying { //
    normal,
    no_side_effects,
    trimmed,
};

enum struct To_Plaintext_Status : Default_Underlying { //
    ok,
    some_ignored,
    error
};

/// @brief Converts content to plaintext.
/// For text, this outputs that text literally.
/// For escaped characters, this outputs the escaped character.
/// For directives, this runs `generate_plaintext` using the behavior of that directive,
/// looked up via context.
/// @param context the current processing context
To_Plaintext_Status to_plaintext(
    std::pmr::vector<char8_t>& out,
    const ast::Content& c,
    Context& context,
    To_Plaintext_Mode mode = To_Plaintext_Mode::normal
);

To_Plaintext_Status to_plaintext(
    std::pmr::vector<char8_t>& out,
    const ast::Directive& d,
    Context& context,
    To_Plaintext_Mode mode = To_Plaintext_Mode::normal
);

/// @brief Calls `to_plaintext` for each piece of content.
To_Plaintext_Status to_plaintext(
    std::pmr::vector<char8_t>& out,
    std::span<const ast::Content> content,
    Context& context,
    To_Plaintext_Mode mode = To_Plaintext_Mode::normal
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

enum struct Paragraphs_State : bool {
    outside,
    inside,
};

/// @brief Converts the `content` to HTML,
/// and depending on `mode`,
/// possibly performing transformations like content trimming or paragraph splitting.
/// @param out Output.
/// @param content The content used to generate HTML.
/// @param context The context.
/// @param mode Unless set to `direct`, performs additional transformations.
/// @param paragraphs_state Only meaningful when `to_html_mode_is_paragraphed(mode)` is `true`.
/// If `inside` is given, it is assumed that the output already contains an opening `<p>` tag,
/// and this content generation begins somewhere in the middle of the paragraph.
void to_html(
    HTML_Writer& out,
    std::span<const ast::Content> content,
    Context& context,
    To_HTML_Mode mode = To_HTML_Mode::direct,
    Paragraphs_State paragraphs_state = Paragraphs_State::outside
);

/// @brief Converts the source code of the content to HTML without any processing.
void to_html_literally(HTML_Writer& out, const ast::Content& content, Context& context);

void to_html_literally(HTML_Writer& out, std::span<const ast::Content> content, Context& context);

Result<void, Syntax_Highlight_Error> to_html_syntax_highlighted(
    HTML_Writer& out,
    std::span<const ast::Content> content,
    std::u8string_view language,
    Context& context,
    std::u8string_view prefix = u8"",
    std::u8string_view suffix = u8"",
    To_HTML_Mode mode = To_HTML_Mode::direct
);

void arguments_to_attributes(
    Attribute_Writer& out,
    const ast::Directive& d,
    Context& context,
    Function_Ref<bool(std::u8string_view)> filter = {},
    Attribute_Style style = Attribute_Style::double_if_needed
);

bool argument_to_attribute(
    Attribute_Writer& out,
    const ast::Argument& a,
    Context& context,
    Function_Ref<bool(std::u8string_view)> filter = {},
    Attribute_Style style = Attribute_Style::double_if_needed
);

/// @brief Converts a specified argument to plaintext.
/// @param out The vector into which generated plaintext should be written.
/// @param d The directive.
/// @param args The arguments. Matching must have already taken place.
/// @param parameter The name of the parameter to which the argument belongs.
/// @param context The context.
/// @returns `true` iff the argument was matched.
[[nodiscard]]
bool argument_to_plaintext(
    std::pmr::vector<char8_t>& out,
    const ast::Directive& d,
    const Argument_Matcher& args,
    std::u8string_view parameter,
    Context& context
);

/// @brief If there is an error behavior in the `context`,
/// uses that behavior's `generate_plaintext` on the directive.
void try_generate_error_plaintext(
    std::pmr::vector<char8_t>& out,
    const ast::Directive& d,
    Context& context
);

/// @brief If there is an error behavior in the `context`,
/// uses that behavior's `generate_html` on the directive.
void try_generate_error_html(HTML_Writer& out, const ast::Directive& d, Context& context);

} // namespace cowel

#endif
