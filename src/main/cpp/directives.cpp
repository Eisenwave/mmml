#include <ranges>
#include <span>

#include "mmml/annotated_string.hpp"
#include "mmml/ast.hpp"
#include "mmml/code_language.hpp"
#include "mmml/directive_arguments.hpp"
#include "mmml/directives.hpp"
#include "mmml/html_writer.hpp"

namespace mmml {

Directive_Behavior* Context::find_directive(std::string_view name) const
{
    for (Name_Resolver* const resolver : std::views::reverse(m_name_resolvers)) {
        if (Directive_Behavior* result = (*resolver)(name)) {
            return result;
        }
    }
    return nullptr;
}

Directive_Behavior* Context::find_directive(const ast::Directive& directive) const
{
    return find_directive(directive.get_name(m_source));
}

namespace {

/// @brief Converts content to plaintext.
/// For text, this outputs that text literally.
/// For escaped characters, this outputs the escaped character.
/// For directives, this runs `generate_plaintext` using the behavior of that directive,
/// looked up via context.
/// @param context the current processing context
void to_plaintext(std::pmr::vector<char>& out, const ast::Content& c, Context& context)
{
    if (const auto* const t = get_if<ast::Text>(&c)) {
        const std::string_view text = t->get_text(context.get_source());
        out.insert(out.end(), text.begin(), text.end());
    }
    if (const auto* const e = get_if<ast::Escaped>(&c)) {
        out.push_back(e->get_char(context.get_source()));
    }

    auto& d = get<ast::Directive>(c);
    if (Directive_Behavior* behavior = context.find_directive(d)) {
        behavior->generate_plaintext(out, d, context);
    }
    // TODO: else error handling; possibly replace with error directive
}

void to_plaintext(
    std::pmr::vector<char>& out,
    std::span<const ast::Content> content,
    Context& context
)
{
    for (const ast::Content& c : content) {
        to_plaintext(out, c, context);
    }
}

void to_plaintext_mapped_for_highlighting(std::pmr::vector<char>&, std::pmr::vector<std::size_t>&, const ast::Directive&, Context&);

void to_plaintext_mapped_for_highlighting(std::pmr::vector<char>&, std::pmr::vector<std::size_t>&, const ast::Text&, Context&);

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Content& c,
    Context& context
)
{
    if (const auto* const t = get_if<ast::Text>(&c)) {
        to_plaintext_mapped_for_highlighting(out, out_mapping, *t, context);
    }
    if (const auto* const e = get_if<ast::Escaped>(&c)) {
        out.push_back(e->get_char(context.get_source()));
        out_mapping.push_back(e->get_char_index());
    }

    auto& d = get<ast::Directive>(c);
    to_plaintext_mapped_for_highlighting(out, out_mapping, d, context);
}

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    std::span<const ast::Content> content,
    Context& context
)
{
    for (const ast::Content& c : content) {
        to_plaintext_mapped_for_highlighting(out, out_mapping, c, context);
    }
}

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Text& t,
    Context& context
)
{
    // TODO: to be accurate, we would have to process HTML entities here so that syntax highlighting
    //       sees them as a character rather than attempting to highlight the original entity.
    //       For example, `&lt;` should be highlighted like a `<` operator.
    const std::string_view text = t.get_text(context.get_source());
    out.insert(out.end(), text.begin(), text.end());

    const Local_Source_Span pos = t.get_source_position();
    out_mapping.reserve(out_mapping.size() + pos.length);
    for (std::size_t i = pos.begin; i < pos.length; ++i) {
        out_mapping.push_back(i);
    }
}

void to_plaintext_mapped_for_highlighting(
    std::pmr::vector<char>& out,
    std::pmr::vector<std::size_t>& out_mapping,
    const ast::Directive& d,
    Context& context
)
{
    Directive_Behavior* behavior = context.find_directive(d);
    if (!behavior) {
        return;
    }
    switch (behavior->category) {
    // Meta directives such as comments cannot generate plaintext anyway.
    case Directive_Category::meta:
    // Mixed or pure HTML directives don't interoperate with syntax highlighting at all.
    // There's no way to highlighting something like a `<button>` element,
    // and even if our directive was meant to generate e.g. `Hello: <button>...`,
    // it is not reasonable to assume that `Hello: ` can be highlighted meaningfully.
    case Directive_Category::mixed:
    case Directive_Category::pure_html: break;

    // Formatting directives such as `\b` are very special;
    // it is guaranteed that we can apply syntax highlighting to the content within,
    // and feed that back into the formatting directive.
    //
    // In this function, we just recurse into the directive's contents so we know which piece of
    // content within produced what syntax-highlighted part.
    case Directive_Category::formatting:
        to_plaintext_mapped_for_highlighting(out, out_mapping, d.get_content(), context);
        break;

    // For pure plaintext directives, we just run plaintext generation.
    // This also means that we don't know exactly which generated character belongs to
    // which source character, but it doesn't really matter.
    // We never run HTML generation afterwards and substitute the plaintext directive
    // with various syntax-highlighted content.
    case Directive_Category::pure_plaintext:
        const std::size_t initial_size = out.size();
        behavior->generate_plaintext(out, d, context);
        MMML_ASSERT(out_mapping.size() >= initial_size);
        out_mapping.reserve(out_mapping.size() - initial_size);
        for (std::size_t i = initial_size; i < out.size(); ++i) {
            out_mapping.push_back(i);
        }
        break;
    }
}

void contents_to_html(
    Annotated_String& out,
    std::span<const ast::Content> content,
    Context& context
)
{
    HTML_Writer nested_writer { out };
    for (const ast::Content& c : content) {
        if (const auto* const e = get_if<ast::Escaped>(&c)) {
            const char c = e->get_char(context.get_source());
            nested_writer.write_inner_html({ &c, 1 });
        }
        if (const auto* const t = get_if<ast::Text>(&c)) {
            nested_writer.write_inner_html(t->get_text(context.get_source()));
        }
        const auto& d = get<ast::Directive>(c);
        if (Directive_Behavior* behavior = context.find_directive(d)) {
            behavior->generate_html(nested_writer, d, context);
        }
    }
}

void preprocess_content(ast::Content& c, Context& context)
{
    if (auto* const d = get_if<ast::Directive>(&c)) {
        if (Directive_Behavior* behavior = context.find_directive(*d)) {
            behavior->preprocess(*d, context);
        }
        // TODO: handle lookup failure
    }
}

void preprocess_contents(std::span<ast::Content> contents, Context& context)
{
    for (ast::Content& c : contents) {
        preprocess_content(c, context);
    }
}

void preprocess_arguments(ast::Directive& d, Context& context)
{
    for (ast::Argument& a : d.get_arguments()) {
        preprocess_contents(a.get_content(), context);
    }
}

void arguments_to_attributes(Attribute_Writer& out, const ast::Directive& d, Context& context)
{
    std::pmr::vector<char> value { context.get_transient_memory() };
    for (const ast::Argument& a : d.get_arguments()) {
        // TODO: error handling
        value.clear();
        to_plaintext(value, a.get_content(), context);
        const std::string_view value_string { value.data(), value.size() };
        if (a.has_name()) {
            out.write_attribute(a.get_name(context.get_source()), value_string);
        }
        // TODO: what if the positional argument cannot be used as an attribute name
        else {
            out.write_attribute(value_string);
        }
    }
}

struct Pure_HTML_Behavior : Directive_Behavior {

    Pure_HTML_Behavior(Directive_Display display)
        : Directive_Behavior { Directive_Category::pure_html, display }
    {
    }

    void generate_plaintext(std::pmr::vector<char>&, const ast::Directive&, Context&) const final {
    }
};

struct Do_Nothing_Behavior : Directive_Behavior {
    // TODO: diagnose ignored arguments

    Do_Nothing_Behavior(Directive_Category category, Directive_Display display)
        : Directive_Behavior { category, display }
    {
    }

    void preprocess(ast::Directive&, Context&) override { }

    void generate_plaintext(std::pmr::vector<char>&, const ast::Directive&, Context&) const override
    {
    }

    void generate_html(HTML_Writer&, const ast::Directive&, Context&) const override { }
};

/// @brief Behavior for `\\error` directives.
/// Does not processing.
/// Generates no plaintext.
/// Generates HTML with the source code of the contents wrapped in an `<error->` custom tag.
struct Error_Behavior : Do_Nothing_Behavior {
    static constexpr std::string_view id = "error-";

    Error_Behavior()
        : Do_Nothing_Behavior { Directive_Category::pure_html, Directive_Display::in_line }
    {
    }

    void generate_html(HTML_Writer& out, const ast::Directive& d, Context& context) const
    {
        out.open_tag(id);
        for (const ast::Content& c : d.get_content()) {
            out.write_inner_text(get_source(c, context.get_source()));
        }
        out.close_tag(id);
    }
};

struct Passthrough_Behavior : Directive_Behavior {

    Passthrough_Behavior(Directive_Category category, Directive_Display display)
        : Directive_Behavior { category, display }
    {
    }

    void preprocess(ast::Directive& d, Context& context) override
    {
        preprocess_arguments(d, context);
    }

    void generate_plaintext(std::pmr::vector<char>& out, const ast::Directive& d, Context& context)
        const override
    {
        to_plaintext(out, d.get_content(), context);
    }

    void generate_html(HTML_Writer& out, const ast::Directive& d, Context& context) const override
    {
        const std::string_view name = get_name(d, context);
        if (d.get_arguments().empty()) {
            out.open_tag(name);
        }
        else {
            Attribute_Writer attributes = out.open_tag_with_attributes(name);
            arguments_to_attributes(attributes, d, context);
        }
        out.close_tag(name);
    }

    [[nodiscard]]
    virtual std::string_view get_name(const ast::Directive& d, Context& context) const
        = 0;
};

constexpr char builtin_directive_prefix = '-';

/// @brief Behavior for any formatting tags that are mapped onto HTML with the same name.
/// This includes `\\i{...}`, `\\strong`, and many more.
///
/// Preprocesses and processes all arguments.
/// Generates the contents inside in plaintext.
///
/// Generates HTML where arguments are converted to HTML attributes,
/// in a tag that has the same name as the directive.
/// For example, `\\i[id = 123]{...}` generates `<i id=123>...</i>`.
struct Directive_Name_Passthrough_Behavior : Passthrough_Behavior {
private:
    const std::string_view m_name_prefix;

public:
    Directive_Name_Passthrough_Behavior(
        Directive_Category category,
        Directive_Display display,
        const std::string_view name_prefix = ""
    )
        : Passthrough_Behavior { category, display }
        , m_name_prefix { name_prefix }
    {
    }

    [[nodiscard]]
    std::string_view get_name(const ast::Directive& d, Context& context) const override
    {
        const std::string_view raw_name = d.get_name(context.get_source());
        const std::string_view name
            = raw_name.starts_with(builtin_directive_prefix) ? raw_name.substr(1) : raw_name;
        return name.substr(m_name_prefix.size());
    }
};

struct Fixed_Name_Passthrough_Behavior : Passthrough_Behavior {
private:
    const std::string_view m_name;

public:
    explicit Fixed_Name_Passthrough_Behavior(
        Directive_Category category,
        Directive_Display display,
        std::string_view name
    )
        : Passthrough_Behavior { category, display }
        , m_name { name }
    {
    }

    [[nodiscard]]
    std::string_view get_name(const ast::Directive&, Context&) const override
    {
        return m_name;
    }
};

/// @brief Behavior for the `\\html{...}`, directive.
/// This is a pure HTML directive.
///
/// Literals within this block are treated as HTML.
/// HTML generation takes place for any directives within.
struct HTML_Literal_Behavior : Pure_HTML_Behavior {

    HTML_Literal_Behavior()
        : Pure_HTML_Behavior(Directive_Display::block)
    {
    }

    void preprocess(ast::Directive& d, Context& context) override
    {
        // TODO: warn for ignored (unprocessed) arguments
        for (ast::Content& c : d.get_content()) {
            preprocess_content(c, context);
        }
    }

    void generate_html(HTML_Writer&, const ast::Directive& d, Context& context) const override
    {
        Annotated_String buffer { context.get_transient_memory() };
        contents_to_html(buffer, d.get_content(), context);
    }
};

struct Parametric_Behavior : Directive_Behavior {
protected:
    const std::span<const std::string_view> m_parameters;

public:
    Parametric_Behavior(
        Directive_Category c,
        Directive_Display d,
        std::span<const std::string_view> parameters
    )
        : Directive_Behavior { c, d }
        , m_parameters { parameters }
    {
    }

    void preprocess(ast::Directive& d, Context& context) final
    {
        Argument_Matcher args { m_parameters, context.get_transient_memory() };
        args.match(d.get_arguments(), context.get_source());
        preprocess(d, args, context);
    }

    void generate_plaintext(std::pmr::vector<char>& out, const ast::Directive& d, Context& context)
        const override
    {
        Argument_Matcher args { m_parameters, context.get_transient_memory() };
        args.match(d.get_arguments(), context.get_source());
        generate_plaintext(out, d, args, context);
    }

    void generate_html(HTML_Writer& out, const ast::Directive& d, Context& context) const override
    {
        Argument_Matcher args { m_parameters, context.get_transient_memory() };
        args.match(d.get_arguments(), context.get_source());
        generate_html(out, d, args, context);
    }

protected:
    virtual void preprocess(ast::Directive& d, const Argument_Matcher& args, Context& context) = 0;
    virtual void generate_plaintext(
        std::pmr::vector<char>& out,
        const ast::Directive& d,
        const Argument_Matcher& args,
        Context& context
    ) const
        = 0;
    virtual void generate_html(
        HTML_Writer& out,
        const ast::Directive& d,
        const Argument_Matcher& args,
        Context& context
    ) const
        = 0;
};

void preprocess_matched_arguments(
    ast::Directive& d,
    std::span<const Argument_Status> statuses,
    Context& context
)
{
    for (std::size_t i = 0; i < statuses.size(); ++i) {
        if (statuses[i] == Argument_Status::ok) {
            preprocess_contents(d.get_arguments()[i].get_content(), context);
        }
    }
}

struct Variable_Behavior : Parametric_Behavior {
    static constexpr std::string_view var_parameter = "var";
    static constexpr std::string_view parameters[] { var_parameter };

    Variable_Behavior(Directive_Category c, Directive_Display d)
        : Parametric_Behavior { c, d, parameters }
    {
    }

    void preprocess(ast::Directive& d, const Argument_Matcher& args, Context& context) override
    {
        preprocess_matched_arguments(d, args.argument_statuses(), context);
        // TODO: warn unmatched arguments
    }

    void generate_plaintext(
        std::pmr::vector<char>& out,
        const ast::Directive& d,
        const Argument_Matcher& args,
        Context& context
    ) const override
    {
        std::pmr::vector<char> data;
        const std::string_view name = get_variable_name(data, d, args, context);
        generate_var_plaintext(out, d, name, context);
    }

    void generate_html(
        HTML_Writer& out,
        const ast::Directive& d,
        const Argument_Matcher& args,
        Context& context
    ) const override
    {
        std::pmr::vector<char> data;
        const std::string_view name = get_variable_name(data, d, args, context);
        generate_var_html(out, d, name, context);
    }

protected:
    virtual void generate_var_plaintext(
        std::pmr::vector<char>& out,
        const ast::Directive& d,
        std::string_view var,
        Context& context
    ) const
        = 0;

    virtual void generate_var_html(
        HTML_Writer& out,
        const ast::Directive& d,
        std::string_view var,
        Context& context
    ) const
        = 0;

private:
    static std::string_view get_variable_name(
        std::pmr::vector<char>& out,
        const ast::Directive& d,
        const Argument_Matcher& args,
        Context& context
    )
    {
        int i = args.get_argument_index(var_parameter);
        if (i < 0) {
            // TODO: error when no variable was specified
            return {};
        }
        const ast::Argument& arg = d.get_arguments()[std::size_t(i)];
        // TODO: warn when pure HTML argument was used as variable name
        to_plaintext(out, arg.get_content(), context);
        return { out.data(), out.size() };
    }
};

struct Get_Variable_Behavior final : Variable_Behavior {

    Get_Variable_Behavior()
        : Variable_Behavior { Directive_Category::pure_plaintext, Directive_Display::in_line }
    {
    }

    void generate_var_plaintext(
        std::pmr::vector<char>& out,
        const ast::Directive&,
        std::string_view var,
        Context& context
    ) const final
    {
        const auto it = context.m_variables.find(var);
        if (it != context.m_variables.end()) {
            out.insert(out.end(), it->second.begin(), it->second.end());
        }
    }

    void generate_var_html(
        HTML_Writer& out,
        const ast::Directive&,
        std::string_view var,
        Context& context
    ) const final
    {
        const auto it = context.m_variables.find(var);
        if (it != context.m_variables.end()) {
            out.write_inner_html(it->second);
        }
    }
};

enum struct Variable_Operation {
    // TODO: add more operations
    set
};

[[nodiscard]] [[maybe_unused]]
std::pmr::string vec_to_string(const std::pmr::vector<char>& v)
{
    return { v.data(), v.size(), v.get_allocator() };
}

struct Modify_Variable_Behavior final : Variable_Behavior {
private:
    const Variable_Operation m_op;

public:
    Modify_Variable_Behavior(Variable_Operation op)
        : Variable_Behavior { Directive_Category::meta, Directive_Display::none }
        , m_op { op }
    {
    }

    void process(const ast::Directive& d, std::string_view var, Context& context) const
    {
        std::pmr::vector<char> body_string { context.get_transient_memory() };
        to_plaintext(body_string, d.get_content(), context);

        const auto it = context.m_variables.find(var);
        if (m_op == Variable_Operation::set) {
            std::pmr::string value { body_string.data(), body_string.size(),
                                     context.get_persistent_memory() };
            if (it == context.m_variables.end()) {
                std::pmr::string key { var.data(), var.size(), context.get_persistent_memory() };
                context.m_variables.emplace(std::move(key), std::move(value));
            }
            else {
                it->second = std::move(value);
            }
        }
    }

    void generate_var_plaintext(
        std::pmr::vector<char>&,
        const ast::Directive& d,
        std::string_view var,
        Context& context
    ) const final
    {
        process(d, var, context);
    }

    void
    generate_var_html(HTML_Writer&, const ast::Directive& d, std::string_view var, Context& context)
        const final
    {
        process(d, var, context);
    }
};

constexpr std::string_view html_tag_prefix = "html-";

} // namespace

struct Builtin_Directive_Set::Impl {
    Do_Nothing_Behavior do_nothing;
    Error_Behavior error;
    HTML_Literal_Behavior html {};
    Directive_Name_Passthrough_Behavior direct_formatting { Directive_Category::formatting,
                                                            Directive_Display::in_line };
    Fixed_Name_Passthrough_Behavior tt_formatting { Directive_Category::formatting,
                                                    Directive_Display::in_line, "tt-" };
    Directive_Name_Passthrough_Behavior direct_html { Directive_Category::pure_html,
                                                      Directive_Display::in_line };
    Directive_Name_Passthrough_Behavior html_tags { Directive_Category::pure_html,
                                                    Directive_Display::block, html_tag_prefix };
};

Builtin_Directive_Set::Builtin_Directive_Set() = default;
Builtin_Directive_Set::~Builtin_Directive_Set() = default;

Directive_Behavior* Builtin_Directive_Set::operator()(std::string_view name) const
{
    // Any builtin names should be found with both `\\-directive` and `\\directive`.
    // `\\def` does not permit defining directives with a hyphen prefix,
    // so this lets the user
    if (name.starts_with(builtin_directive_prefix)) {
        return (*this)(name.substr(1));
    }
    if (name.empty()) {
        return nullptr;
    }
    switch (name[0]) {
    case 'b':
        if (name == "b")
            return &m_impl->direct_formatting;
        break;

    case 'c':
        if (name == "comment")
            return &m_impl->do_nothing;
        break;

    case 'd':
        if (name == "dd" || name == "dl" || name == "dt")
            return &m_impl->direct_html;
        break;

    case 'e':
        if (name == "em")
            return &m_impl->direct_formatting;
        if (name == "error")
            return &m_impl->error;
        break;

    case 'h':
        if (name == "html")
            return &m_impl->html;
        static_assert(html_tag_prefix[0] == 'h');
        if (name.starts_with(html_tag_prefix))
            return &m_impl->html_tags;
        break;

    case 'i':
        if (name == "i" || name == "ins")
            return &m_impl->direct_formatting;
        break;

    case 'k':
        if (name == "kbd")
            return &m_impl->direct_formatting;
        break;

    case 'm':
        if (name == "mark")
            return &m_impl->direct_formatting;
        break;

    case 'o':
        if (name == "ol")
            return &m_impl->direct_html;
        break;

    case 's':
        if (name == "s" || name == "small" || name == "strong" || name == "sub" || name == "sup")
            return &m_impl->direct_formatting;
        break;

    case 't':
        if (name == "tt")
            return &m_impl->tt_formatting;
        break;

    case 'u':
        if (name == "u")
            return &m_impl->direct_formatting;
        if (name == "ul")
            return &m_impl->direct_html;
        break;
    }

    return nullptr;
}

} // namespace mmml
