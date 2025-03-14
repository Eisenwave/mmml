#ifndef MMML_PROCESSING_HPP
#define MMML_PROCESSING_HPP

#include <memory>
#include <memory_resource>
#include <span>
#include <string_view>
#include <vector>

#include "mmml/context.hpp"
#include "mmml/fwd.hpp"
#include "mmml/util/typo.hpp"

namespace mmml {

/// @brief A category which applies to a directive behavior generally,
/// regardless of the specific directive processed at the time.
///
/// These categories are important to guide how directives that are effectively
/// put into HTML attributes (e.g. `\\html-div[id=\\something]`) should be treated,
/// as well as how syntax highlighting interacts with a directive.
enum struct Directive_Category : Default_Underlying {
    /// @brief The directive generates no plaintext or HTML.
    /// For example, `\\comment`.
    meta,
    /// @brief The directive (regardless of input content or arguments)
    /// produces purely plaintext.
    ///
    /// During syntax highlighting, such directives are eliminated entirely,
    /// and integrated into the syntax-highlighted content.
    pure_plaintext,
    /// @brief Purely HTML content, such as `\\html{...}`.
    /// Such content produces no plaintext, and using it as an HTML attribute is erroneous.
    pure_html,
    /// @brief HTML formatting wrapper for content within.
    /// Using formatting inside of HTML attributes is erroneous.
    ///
    /// During syntax highlighting, the contents of formatting directives are
    /// replaced with highlighted contents.
    /// For example, `\\code{\\b{void}}` may be turned into `\\code{\\b{\\hl-keyword{void}}}`.
    formatting,
    /// @brief Mixed plaintext and HTML content.
    /// This is a fallback category for when none of the other options apply.
    /// Using it as an HTML attribute is not erroneous, but may lead to unexpected results.
    /// For syntax highlighting, this is treated same as `pure_html`.
    mixed,
};

/// @brief Specifies how a directive should be displayed.
enum struct Directive_Display : Default_Underlying {
    /// @brief Nothing is displayed.
    none,
    /// @brief The directive is a block, such as `\\h1` or `\\codeblock`.
    /// Such directives are not integrated into other paragraphs or surround text.
    block,
    /// @brief The directive is inline, such as `\\b` or `\\code`.
    /// This means that it will be displayed within paragraphs and as part of other text.
    in_line,
};

/// @brief Implements behavior that one or multiple directives should have.
struct Directive_Behavior {
    const Directive_Category category;
    const Directive_Display display;

    constexpr Directive_Behavior(Directive_Category c, Directive_Display d)
        : category { c }
        , display { d }
    {
    }

    virtual void
    generate_plaintext(std::pmr::vector<char8_t>& out, const ast::Directive&, Context&) const
        = 0;

    virtual void generate_html(HTML_Writer& out, const ast::Directive&, Context&) const = 0;
};

struct Content_Behavior {
    const Directive_Category category;
    const Directive_Display display;

    [[nodiscard]]
    constexpr Content_Behavior(Directive_Category c, Directive_Display d)
        : category { c }
        , display { d }
    {
    }

    virtual void
    generate_plaintext(std::pmr::vector<char8_t>& out, std::span<const ast::Content>, Context&)
        const
        = 0;
    virtual void generate_html(HTML_Writer& out, std::span<const ast::Content>, Context&) const = 0;
};

struct [[nodiscard]]
Builtin_Directive_Set final : Name_Resolver {
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

public:
    Builtin_Directive_Set();

    Builtin_Directive_Set(const Builtin_Directive_Set&) = delete;
    Builtin_Directive_Set& operator=(const Builtin_Directive_Set&) = delete;

    ~Builtin_Directive_Set();

    [[nodiscard]]
    Directive_Behavior& get_error_behavior() noexcept;

    [[nodiscard]]
    Distant<std::u8string_view>
    fuzzy_lookup_name(std::u8string_view name, std::pmr::memory_resource* memory) const final;

    [[nodiscard]]
    Directive_Behavior* operator()(std::u8string_view name) const final;
};

} // namespace mmml

#endif
