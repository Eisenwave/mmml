#ifndef MMML_TOKENS_HPP
#define MMML_TOKENS_HPP

#include <compare>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

#include "mmml/ast.hpp"
#include "mmml/fwd.hpp"

namespace mmml {

enum struct AST_Instruction_Type : Default_Underlying {
    /// @brief Ignore the next `n` characters.
    /// This is used only within directive arguments,
    /// where leading and trailing whitespace generally doesn't matter.
    skip,
    /// @brief The next `n` characters are an escape sequence (e.g. `\\{`).
    escape,
    /// @brief The next `n` characters are literal text.
    text,
    /// @brief The next `n` characters are an argument name.
    argument_name,
    /// @brief Advance past `=` following an argument name.
    argument_equal,
    /// @brief Advance past `,` between arguments.
    argument_comma,
    /// @brief Begins the document.
    /// Always the first instruction.
    /// The operand is the amount of pieces that comprise the argument content,
    /// where a piece is an escape sequence, text, or a directive.
    push_document,
    pop_document,
    /// @brief Begin directive.
    /// The operand is the amount of characters to advance until the end the directive name.
    /// Note that this includes the leading `\\`.
    push_directive,
    pop_directive,
    /// @brief Begin directive arguments.
    /// The operand is the amount of arguments.
    ///
    /// Advance past `[`.
    push_arguments,
    /// @brief Advance past `]`.
    pop_arguments,
    /// @brief Begin argument.
    /// The operand is the amount of pieces that comprise the argument content,
    /// where a piece is an escape sequence, text, or a directive.
    push_argument,
    pop_argument,
    /// @brief Begin directive content.
    /// The operand is the amount of pieces that comprise the argument content,
    /// where a piece is an escape sequence, text, or a directive.
    ///
    /// Advance past `{`.
    push_block,
    /// @brief Advance past `}`.
    pop_block
};

[[nodiscard]]
constexpr bool ast_instruction_type_has_operand(AST_Instruction_Type type)
{
    using enum AST_Instruction_Type;
    switch (type) {
    case pop_document:
    case pop_directive:
    case pop_arguments:
    case pop_argument:
    case pop_block:
    case argument_comma:
    case argument_equal: return false;
    default: return true;
    }
}

[[nodiscard]]
std::u8string_view ast_instruction_type_name(AST_Instruction_Type type);

struct AST_Instruction {
    AST_Instruction_Type type;
    std::size_t n = 0;

    friend std::strong_ordering operator<=>(const AST_Instruction&, const AST_Instruction&)
        = default;
};

/// @brief Parses the MMML document.
/// This process does not result in an AST, but a vector of instructions that can be used to
/// construct an AST.
///
/// Note that parsing is infallible.
/// In the grammar, any syntax violation can fall back onto literal text,
/// so the parsed result may be undesirable, but always valid.
void parse(std::pmr::vector<AST_Instruction>& out, std::u8string_view source);

/// @brief Builds an AST from a span of instructions,
/// usually obtained from `parse`.
std::pmr::vector<ast::Content> build_ast(
    std::u8string_view source,
    std::span<const AST_Instruction> instructions,
    std::pmr::memory_resource* memory
);

/// @brief Uses the AST instructions to create syntax highlighting information.
/// A sequence of `Annotation_Span`s is appended to `out`,
/// where gaps between spans represent non-highlighted content such as plaintext or whitespace.
void build_highlight(
    std::pmr::vector<Annotation_Span<HLJS_Scope>>& out,
    std::span<const AST_Instruction> instructions
);

/// @brief Parses a document and runs `build_ast` on the results.
std::pmr::vector<ast::Content>
parse_and_build(std::u8string_view source, std::pmr::memory_resource* memory);

} // namespace mmml

#endif
