#ifndef MMML_DIAGNOSTICS_HPP
#define MMML_DIAGNOSTICS_HPP

#ifndef MMML_EMSCRIPTEN
#include <iosfwd>
#endif

#include <span>
#include <string>
#include <string_view>

#include "mmml/assert.hpp"
#include "mmml/fwd.hpp"
#include "mmml/io.hpp"
#include "mmml/source_position.hpp"

namespace mmml {

/// @brief Returns the line that contains the given index.
/// @param source the source string
/// @param index the index within the source string, in range `[0, source.size())`
/// @return A line which contains the given `index`.
[[nodiscard]]
std::string_view find_line(std::string_view source, std::size_t index);

/// @brief Prints the location of the file nicely formatted.
/// @param out the string to write to
/// @param file the file
void print_location_of_file(Annotated_String& out, std::string_view file);

/// @brief Prints a position within a file, consisting of the file name and line/column.
/// @param out the string to write to
/// @param file the file
/// @param pos the position within the file
/// @param colon_suffix if `true`, appends a `:` to the string as part of the same token
void print_file_position(
    Annotated_String& out,
    std::string_view file,
    const Local_Source_Position& pos,
    bool colon_suffix = true
);

/// @brief Prints the contents of the affected line within `source` as well as position indicators
/// which show the span which is affected by some diagnostic.
/// @param out the string to write to
/// @param source the program source
/// @param pos the position within the source
void print_affected_line(
    Annotated_String& out,
    std::string_view source,
    const Local_Source_Position& pos
);

void print_affected_line(
    Annotated_String& out,
    std::string_view source,
    const Local_Source_Span& pos
);

void print_assertion_error(Annotated_String& out, const Assertion_Error& error);

void print_io_error(Annotated_String& out, std::string_view file, IO_Error_Code error);

struct AST_Formatting_Options {
    int indent_width;
    int max_node_text_length;
};

void print_ast(
    Annotated_String& out,
    std::string_view source,
    std::span<const ast::Content> root_content,
    AST_Formatting_Options
);

void print_internal_error_notice(Annotated_String& out);

#ifndef MMML_EMSCRIPTEN
std::ostream& print_code_string(std::ostream& out, const Annotated_String& string, bool colors);
#endif

} // namespace mmml

#endif
