#ifndef MMML_DIRECTIVE_PROCESSING_HPP
#define MMML_DIRECTIVE_PROCESSING_HPP

#include <vector>

#include "mmml/ast.hpp"
#include "mmml/directives.hpp"

namespace mmml {

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

/// @brief Converts the source code of the content to HTML without any processing.
void to_html_literally(HTML_Writer& out, const ast::Content& content, Context& context);

void to_html_literally(HTML_Writer& out, std::span<const ast::Content> content, Context& context);

void preprocess(ast::Content& c, Context& context);

void preprocess(std::span<ast::Content> contents, Context& context);

void preprocess_arguments(ast::Directive& d, Context& context);

void arguments_to_attributes(Attribute_Writer& out, const ast::Directive& d, Context& context);

} // namespace mmml

#endif
