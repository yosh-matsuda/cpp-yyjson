// cpp-yyjson-codegen: C++ types with cpp-yyjson annotations from a JSON Schema.
// This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <cpp_yyjson.hpp>

// numbers of a schema are compared exactly, and switches over the kinds of a type handle the kinds that matter
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

namespace cpp_yyjson_codegen
{
    struct options
    {
        std::string ns{};     // namespace of the generated code; empty for the global namespace
        bool cpp26 = false;   // C++26 annotations ([[=...]]) instead of yyjson::annotations<T>
        bool strict = false;  // warnings are errors
        bool pattern = true;  // emit yyjson::pattern (the program provides yyjson::pattern_matcher<>)
        // a field with a default is std::optional<T>, std::nullopt for a missing key, instead of T with the default as
        // its initializer and yyjson::omit_default
        bool optional_defaults = false;
        // a struct that accepts any other key gets `std::map<std::string, yyjson::value> extra` with
        // yyjson::extra_keys, which keeps the keys that name no property
        bool keep_unknown_keys = false;
        std::string root_name{};  // name of the root type; default: the title, or "Root"
        std::string source{};     // name of the schema in the header comment
        // path of the schema; a reference to another file, such as "address.json#/$defs/Street", is resolved against
        // its directory
        std::string path{};
        // reads the file of a path made from `path` and a reference, or returns std::nullopt; without it, references
        // to other files are read as any JSON value
        std::function<std::optional<std::string>(const std::string&)> load{};
    };

    struct diagnostic
    {
        std::string location;  // URI fragment of the schema, such as "#/$defs/Person/properties/age"
        std::string message;
    };

    class error : public std::runtime_error
    {
        std::vector<diagnostic> diags_;

        static std::string what_of(const std::vector<diagnostic>& diags)
        {
            auto s = std::string();
            for (const auto& d : diags)
            {
                if (!s.empty()) s += '\n';
                s += d.location.empty() ? d.message : d.location + ": " + d.message;
            }
            return s;
        }

    public:
        explicit error(std::vector<diagnostic> diags) : std::runtime_error(what_of(diags)), diags_(std::move(diags)) {}
        [[nodiscard]] const std::vector<diagnostic>& diagnostics() const noexcept { return diags_; }
    };

    struct result
    {
        std::string code;
        std::vector<diagnostic> warnings;
    };

    namespace detail
    {
        // A JSON value detached from the yyjson document, so that the generator can build new schemas.
        struct json
        {
            enum class kind : std::uint8_t
            {
                null,
                boolean,
                number,
                string,
                array,
                object
            };
            kind k = kind::null;
            bool b = false;
            bool integer = false;  // a number written as an integer
            double num = 0;
            std::string text;      // a string, or the JSON text of a number
            std::vector<json> items;
            std::vector<std::pair<std::string, json>> members;

            // defined after the class: their definitions need std::pair<std::string, json>, which needs a complete json
            json() = default;
            json(const json&);
            json(json&&) noexcept;
            json& operator=(const json&);
            json& operator=(json&&) noexcept;
            ~json();

            [[nodiscard]] bool is_null() const { return k == kind::null; }
            [[nodiscard]] bool is_bool() const { return k == kind::boolean; }
            [[nodiscard]] bool is_number() const { return k == kind::number; }
            [[nodiscard]] bool is_string() const { return k == kind::string; }
            [[nodiscard]] bool is_array() const { return k == kind::array; }
            [[nodiscard]] bool is_object() const { return k == kind::object; }
            [[nodiscard]] bool is_integral() const
            {
                return k == kind::number && (integer || (std::isfinite(num) && num == std::trunc(num)));
            }

            [[nodiscard]] const json* find(std::string_view key) const;
            [[nodiscard]] bool has(std::string_view key) const { return find(key) != nullptr; }
            void set(std::string_view key, json v);
            void erase(std::string_view key);

            static json boolean_of(bool v);
            static json string_of(std::string_view s);
            static json array_of(std::vector<json> items);
            static json object_of();
        };

        inline json::json(const json&) = default;
        inline json::json(json&&) noexcept = default;
        inline json& json::operator=(const json&) = default;
        inline json& json::operator=(json&&) noexcept = default;
        inline json::~json() = default;
        inline const json* json::find(std::string_view key) const
        {
            if (k != kind::object) return nullptr;
            for (const auto& [mk, mv] : members)
                if (mk == key) return &mv;
            return nullptr;
        }
        inline void json::set(std::string_view key, json v)
        {
            for (auto& [mk, mv] : members)
                if (mk == key)
                {
                    mv = std::move(v);
                    return;
                }
            members.emplace_back(std::string(key), std::move(v));
        }
        inline void json::erase(std::string_view key)
        {
            std::erase_if(members, [&](const auto& m) { return m.first == key; });
        }
        inline json json::boolean_of(bool v)
        {
            auto j = json();
            j.k = kind::boolean;
            j.b = v;
            return j;
        }
        inline json json::string_of(std::string_view s)
        {
            auto j = json();
            j.k = kind::string;
            j.text = s;
            return j;
        }
        inline json json::array_of(std::vector<json> items)
        {
            auto j = json();
            j.k = kind::array;
            j.items = std::move(items);
            return j;
        }
        inline json json::object_of()
        {
            auto j = json();
            j.k = kind::object;
            return j;
        }

        template <typename V>
        json to_json(const V& v)
        {
            auto j = json();
            if (v.is_null()) return j;
            if (v.is_bool())
            {
                j.k = json::kind::boolean;
                j.b = *v.as_bool();
            }
            else if (v.is_uint() || v.is_sint() || v.is_real())
            {
                j.k = json::kind::number;
                j.integer = !v.is_real();
                j.num = *v.as_num();
                j.text = std::string_view(v.write());
            }
            else if (v.is_string())
            {
                j.k = json::kind::string;
                j.text = *v.as_string();
            }
            else if (v.is_array())
            {
                j.k = json::kind::array;
                for (const auto arr = *v.as_array(); const auto& e : arr) j.items.push_back(to_json(e));
            }
            else
            {
                j.k = json::kind::object;
                for (const auto obj = *v.as_object(); const auto& [key, e] : obj)
                    j.members.emplace_back(std::string(std::string_view(key)), to_json(e));
            }
            return j;
        }

        inline bool equal(const json& a, const json& b)
        {
            if (a.k != b.k) return false;
            switch (a.k)
            {
                case json::kind::null:
                    return true;
                case json::kind::boolean:
                    return a.b == b.b;
                case json::kind::number:
                    return a.integer && b.integer ? a.text == b.text : a.num == b.num;
                case json::kind::string:
                    return a.text == b.text;
                case json::kind::array:
                    return std::ranges::equal(a.items, b.items,
                                              [](const auto& x, const auto& y) { return equal(x, y); });
                case json::kind::object:
                    if (a.members.size() != b.members.size()) return false;
                    for (const auto& [k, v] : a.members)
                        if (const auto* w = b.find(k); w == nullptr || !equal(v, *w)) return false;
                    return true;
            }
            return false;
        }

        inline void dump_string(std::string& out, std::string_view s)
        {
            out += '"';
            for (const auto c : s)
            {
                const auto u = static_cast<unsigned char>(c);
                if (c == '"' || c == '\\')
                {
                    out += '\\';
                    out += c;
                }
                else if (u < 0x20)
                {
                    constexpr auto hex = std::string_view("0123456789abcdef");
                    out += "\\u00";
                    out += hex[u >> 4];
                    out += hex[u & 15];
                }
                else
                    out += c;
            }
            out += '"';
        }

        inline void dump(std::string& out, const json& j)
        {
            switch (j.k)
            {
                case json::kind::null:
                    out += "null";
                    break;
                case json::kind::boolean:
                    out += j.b ? "true" : "false";
                    break;
                case json::kind::number:
                    out += j.text;
                    break;
                case json::kind::string:
                    dump_string(out, j.text);
                    break;
                case json::kind::array:
                    out += '[';
                    for (auto first = true; const auto& e : j.items)
                    {
                        if (!std::exchange(first, false)) out += ',';
                        dump(out, e);
                    }
                    out += ']';
                    break;
                case json::kind::object:
                    out += '{';
                    for (auto first = true; const auto& [k, v] : j.members)
                    {
                        if (!std::exchange(first, false)) out += ',';
                        dump_string(out, k);
                        out += ':';
                        dump(out, v);
                    }
                    out += '}';
                    break;
            }
        }

        inline std::string dump(const json& j)
        {
            auto out = std::string();
            dump(out, j);
            return out;
        }

        inline std::optional<std::int64_t> as_int64(const json& j)
        {
            if (!j.is_number()) return std::nullopt;
            if (j.integer)
            {
                auto v = std::int64_t();
                const auto* end = j.text.data() + j.text.size();
                if (const auto [p, ec] = std::from_chars(j.text.data(), end, v); ec == std::errc() && p == end)
                    return v;
                return std::nullopt;
            }
            if (j.is_integral() && j.num >= -0x1p63 && j.num < 0x1p63) return static_cast<std::int64_t>(j.num);
            return std::nullopt;
        }

        inline std::optional<std::uint64_t> as_uint64(const json& j)
        {
            if (!j.is_number()) return std::nullopt;
            if (j.integer)
            {
                auto v = std::uint64_t();
                const auto* end = j.text.data() + j.text.size();
                if (const auto [p, ec] = std::from_chars(j.text.data(), end, v); ec == std::errc() && p == end)
                    return v;
                return std::nullopt;
            }
            if (j.is_integral() && j.num >= 0 && j.num < 0x1p64) return static_cast<std::uint64_t>(j.num);
            return std::nullopt;
        }

        // C++ literals

        inline std::string int_literal(std::int64_t v)
        {
            if (v == std::numeric_limits<std::int64_t>::min()) return "(-9223372036854775807 - 1)";
            return std::to_string(v);
        }

        inline std::string uint_literal(std::uint64_t v)
        {
            return std::to_string(v) +
                   (v > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ? "u" : "");
        }

        // a floating literal of the number `j`; `f` adds the suffix of float
        inline std::string float_literal(const json& j, bool f = false)
        {
            auto s = j.text;
            if (s.find_first_of(".eE") == std::string::npos) s += ".0";
            return f ? s + "f" : s;
        }

        inline std::string raw_string(std::string_view s)
        {
            auto delim = std::string();
            for (auto n = 0; s.find(")" + delim + "\"") != std::string_view::npos; ++n) delim = "j" + std::to_string(n);
            return "R\"" + delim + "(" + std::string(s) + ")" + delim + "\"";
        }

        // the lines of a text for `//` comments: a compiler ends a line also at a lone CR, and joins a line that ends
        // with a backslash to the next one, so the lines are split at CR and LF and lose their trailing backslashes
        inline std::vector<std::string_view> comment_lines(std::string_view text)
        {
            auto lines = std::vector<std::string_view>();
            while (!text.empty())
            {
                const auto nl = std::min(text.find_first_of("\r\n"), text.size());
                auto line = text.substr(0, nl);
                while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\\'))
                    line.remove_suffix(1);
                lines.push_back(line);
                text.remove_prefix(nl);
                if (text.starts_with("\r\n"))
                    text.remove_prefix(2);
                else if (!text.empty())
                    text.remove_prefix(1);
            }
            return lines;
        }

        inline std::string quoted(std::string_view s)
        {
            const auto control = std::ranges::any_of(s, [](char c) {
                const auto u = static_cast<unsigned char>(c);
                return (u < 0x20 && c != '\n' && c != '\t') || u == 0x7f;
            });
            if (!control && s.find_first_of("\"\\\n") != std::string_view::npos) return raw_string(s);
            auto out = std::string("\"");
            for (const auto c : s)
            {
                const auto u = static_cast<unsigned char>(c);
                if (c == '"' || c == '\\')
                {
                    out += '\\';
                    out += c;
                }
                else if (u < 0x20 || u == 0x7f)
                {
                    out += '\\';
                    out += static_cast<char>('0' + (u >> 6));
                    out += static_cast<char>('0' + ((u >> 3) & 7));
                    out += static_cast<char>('0' + (u & 7));
                }
                else
                    out += c;
            }
            return out + "\"";
        }

        // A string literal ends at its first NUL when it converts to a `std::string_view` or a `std::string`, so a
        // string with NUL carries its length: as `std::string_view("a\000b", 3)` for annotations, or as `type`.
        inline std::string cpp_string(std::string_view s, std::string_view type = "std::string_view")
        {
            if (s.find('\0') == std::string_view::npos) return quoted(s);
            return std::string(type) + "(" + quoted(s) + ", " + std::to_string(s.size()) + ")";
        }

        // identifiers

        inline bool is_keyword(std::string_view s)
        {
            static const auto words = std::set<std::string_view>{"alignas",       "alignof",     "and",
                                                                 "and_eq",        "asm",         "auto",
                                                                 "bitand",        "bitor",       "bool",
                                                                 "break",         "case",        "catch",
                                                                 "char",          "char8_t",     "char16_t",
                                                                 "char32_t",      "class",       "compl",
                                                                 "concept",       "const",       "consteval",
                                                                 "constexpr",     "constinit",   "const_cast",
                                                                 "continue",      "co_await",    "co_return",
                                                                 "co_yield",      "decltype",    "default",
                                                                 "delete",        "do",          "double",
                                                                 "dynamic_cast",  "else",        "enum",
                                                                 "explicit",      "export",      "extern",
                                                                 "false",         "float",       "for",
                                                                 "friend",        "goto",        "if",
                                                                 "inline",        "int",         "long",
                                                                 "mutable",       "namespace",   "new",
                                                                 "noexcept",      "not",         "not_eq",
                                                                 "nullptr",       "operator",    "or",
                                                                 "or_eq",         "private",     "protected",
                                                                 "public",        "register",    "reinterpret_cast",
                                                                 "requires",      "return",      "short",
                                                                 "signed",        "sizeof",      "static",
                                                                 "static_assert", "static_cast", "struct",
                                                                 "switch",        "template",    "this",
                                                                 "thread_local",  "throw",       "true",
                                                                 "try",           "typedef",     "typeid",
                                                                 "typename",      "union",       "unsigned",
                                                                 "using",         "virtual",     "void",
                                                                 "volatile",      "wchar_t",     "while",
                                                                 "xor",           "xor_eq",      "final",
                                                                 "override",      "import",      "module",
                                                                 "std",           "yyjson",      "field_reflection",
                                                                 "NULL",          "EOF",         "errno",
                                                                 "assert",        "stdin",       "stdout",
                                                                 "stderr",        "offsetof"};
            return words.contains(s);
        }

        inline bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
        inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
        inline bool is_alnum(char c) { return is_alpha(c) || is_digit(c); }

        inline bool is_identifier(std::string_view s)
        {
            if (s.empty() || is_digit(s[0])) return false;
            if (!std::ranges::all_of(s, [](char c) { return is_alnum(c) || c == '_'; })) return false;
            if (s.find("__") != std::string_view::npos) return false;
            if (s.size() >= 2 && s[0] == '_' && s[1] >= 'A' && s[1] <= 'Z') return false;
            return !is_keyword(s);
        }

        inline std::string sanitize(std::string_view s)
        {
            if (is_identifier(s)) return std::string(s);
            auto out = std::string();
            for (const auto c : s)
            {
                if (is_alnum(c))
                    out += c;
                else if (!out.empty() && out.back() != '_')
                    out += '_';
            }
            while (!out.empty() && out.back() == '_') out.pop_back();
            if (out.empty()) return "value";
            if (is_digit(out[0])) out.insert(0, "_");
            if (is_keyword(out)) out += '_';
            return out;
        }

        inline std::string pascal(std::string_view s)
        {
            auto out = std::string();
            auto upper = true;
            for (const auto c : s)
            {
                if (!is_alnum(c))
                {
                    upper = true;
                    continue;
                }
                out += upper && c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
                upper = false;
            }
            return out;
        }

        inline std::string pointer_token(std::string_view s)
        {
            auto out = std::string();
            for (const auto c : s)
            {
                if (c == '~')
                    out += "~0";
                else if (c == '/')
                    out += "~1";
                else
                    out += c;
            }
            return out;
        }

        inline std::string unpointer_token(std::string_view s)
        {
            auto out = std::string();
            for (std::size_t i = 0; i < s.size(); ++i)
            {
                if (s[i] == '~' && i + 1 < s.size() && (s[i + 1] == '0' || s[i + 1] == '1'))
                    out += s[++i] == '0' ? '~' : '/';
                else
                    out += s[i];
            }
            return out;
        }

        inline bool is_schema_name(std::string_view s)
        {
            return !s.empty() &&
                   std::ranges::all_of(s, [](char c) { return is_alnum(c) || c == '.' || c == '_' || c == '-'; });
        }

        // the C++ type of a schema
        enum class tk : std::uint8_t
        {
            any,        // yyjson::value
            object,     // yyjson::object
            null,       // std::nullptr_t
            monostate,  // std::monostate
            boolean,
            i32,
            i64,
            u64,
            f32,
            f64,
            string,
            named,  // a generated struct, enum or alias
            vector,
            map,    // std::map<std::string, V>
            tuple,
            variant,
            optional,
            pointer  // std::unique_ptr
        };

        // the settings of a field or of an element level: constraints and element trees
        struct settings
        {
            // a constraint rendered, such as "yyjson::minimum(0)", and the keyword and the value of JSON Schema that it
            // checks, by which the generator checks the examples
            struct constraint
            {
                std::string text, keyword;
                json value;
            };
            std::vector<constraint> constraints;
            std::optional<std::uint64_t> min_items, max_items;
            bool unique = false;
            std::vector<settings> items, contains, property_names, additional;  // zero or one
            std::vector<settings> prefix;
            std::optional<std::uint64_t> min_contains, max_contains;

            [[nodiscard]] bool empty() const
            {
                return constraints.empty() && !min_items && !max_items && !unique && items.empty() &&
                       contains.empty() && property_names.empty() && additional.empty() && prefix.empty();
            }
        };

        struct metadata
        {
            std::optional<std::string> title, description, comment;
            bool deprecated = false;
            std::vector<json> examples;
            std::vector<json> noted_examples;  // examples that are not known to read as the C++ type: comments
        };

        // a variant of the kind `combined` other than std::variant has settings and metadata for each alternative,
        // which make it `yyjson::alt<T, ...>`
        enum class combination : std::uint8_t
        {
            none,    // std::variant
            any_of,  // yyjson::any_of
            one_of   // yyjson::one_of
        };

        struct ctype
        {
            tk k = tk::any;
            int decl = -1;
            bool tagged = false;  // a variant with internal_tag
            combination combined = combination::none;
            std::vector<ctype> args{};
            std::vector<settings> sets{};   // yyjson::any_of / one_of: the settings of each alternative
            std::vector<metadata> metas{};  // yyjson::any_of / one_of: the metadata of each alternative
        };

        struct field
        {
            std::string member{}, key{};
            ctype type{};
            settings set{};
            bool required = false, omit_default = false;
            bool extra = false;          // yyjson::extra_keys: a map of the keys that name no property; no key
            std::string init{};          // " = x" or "{...}"
            std::string default_note{};  // the default of a std::optional field, for a comment
            std::string default_from{};  // the function of yyjson::default_from that makes the default
            std::optional<json> dflt{};  // the default before render() converts it to `init`
            metadata meta{};
            std::string loc{};
        };

        // `oneOf` (exactly_one) or `anyOf` of subschemas that only require keys: the groups of keys
        struct required_groups
        {
            bool exactly_one = false;
            std::vector<std::vector<std::string>> groups{};
        };

        // the fields of a struct that the reflection of C++20 takes, and that VISITABLE_STRUCT takes
        inline constexpr std::size_t max_reflected_fields = 100;
        inline constexpr std::size_t max_visitable_fields = 1365;

        struct decl
        {
            enum class kind_t : std::uint8_t
            {
                structure,
                enumeration,
                alias
            };
            kind_t kind = kind_t::structure;
            std::string name{}, schema_name{};
            metadata meta{};
            std::string loc{};
            // struct
            std::vector<field> fields{};
            bool additional = true;
            std::vector<std::pair<std::string, std::vector<std::string>>> dependent{};  // keys
            std::vector<required_groups> groups{};                                      // keys
            bool filled = false;
            bool transparent = false;  // yyjson::transparent: read and written as the value of its only field
            // enum: member name, JSON name
            std::vector<std::pair<std::string, std::string>> enumerators{};
            std::map<std::string, metadata> enumerator_meta{};  // by JSON name: comments of the enumerators
            // alias
            ctype target{};
            settings set{};
        };

        // internal_tag of a variant type, and schema_name of a named alias of it
        struct variant_note
        {
            ctype type;
            std::string key;
            std::vector<std::string> ids;
            int alias = -1;
            std::string loc;
        };

        struct typed
        {
            ctype t;
            settings s;
        };

        inline ctype make(tk k, std::vector<ctype> args = {}) { return ctype{.k = k, .args = std::move(args)}; }
        inline ctype named_type(int d) { return ctype{.k = tk::named, .decl = d}; }

        inline bool is_nullable(const ctype& t)
        {
            return t.k == tk::variant && !t.tagged && t.combined == combination::none && t.args.size() == 2 &&
                   t.args[0].k == tk::monostate;
        }

        // a variant that reads null as its first alternative, std::monostate
        inline bool holds_null(const ctype& t)
        {
            return t.k == tk::variant && !t.tagged && !t.args.empty() && t.args[0].k == tk::monostate;
        }

        inline bool is_metadata_key(std::string_view k)
        {
            static const auto keys =
                std::set<std::string_view>{"title",         "description",    "default",         "examples",
                                           "deprecated",    "readOnly",       "writeOnly",       "$comment",
                                           "$id",           "$schema",        "$anchor",         "$defs",
                                           "definitions",   "$vocabulary",    "contentEncoding", "contentMediaType",
                                           "contentSchema", "$dynamicAnchor", "$recursiveAnchor"};
            return keys.contains(k);
        }

        // the keywords that the generator does not support and leaves out of the C++ type, with a warning
        inline constexpr auto ignored_keys = std::array<std::string_view, 9>{
            "if",          "then", "else", "not", "dependentSchemas", "unevaluatedProperties", "unevaluatedItems",
            "$dynamicRef", "$recursiveRef"};
        inline bool is_ignored_key(std::string_view k)
        {
            return std::ranges::find(ignored_keys, k) != ignored_keys.end();
        }

        // the keywords from which the generator builds the C++ type; others (vendor keys such as `markdownDescription`)
        // are left out silently
        inline bool is_type_key(std::string_view k)
        {
            static const auto keys = std::set<std::string_view>{
                "$ref",           "type",          "enum",
                "const",          "properties",    "required",
                "additionalProperties", "patternProperties", "propertyNames",
                "minProperties",  "maxProperties", "dependentRequired",
                "dependencies",   "items",         "prefixItems",
                "additionalItems", "contains",     "minContains",
                "maxContains",    "minItems",      "maxItems",
                "uniqueItems",    "minLength",     "maxLength",
                "pattern",        "format",        "minimum",
                "maximum",        "exclusiveMinimum", "exclusiveMaximum",
                "multipleOf",     "allOf",         "anyOf",
                "oneOf"};
            return keys.contains(k);
        }

        // a schema that gives the C++ type nothing: true, or no keyword the generator builds a type from
        inline bool adds_nothing(const json& s)
        {
            if (s.is_bool()) return s.b;
            return s.is_object() && std::ranges::none_of(s.members, [](const auto& m) { return is_type_key(m.first); });
        }

        // a reference without keywords beside it that the generator would merge into the referred schema
        inline bool is_pure_ref(const json& s)
        {
            return s.is_object() && s.has("$ref") && std::ranges::all_of(s.members, [](const auto& m) {
                       return m.first == "$ref" || !(is_type_key(m.first) || is_ignored_key(m.first));
                   });
        }

        inline std::vector<std::string> type_list(const json& s)
        {
            auto out = std::vector<std::string>();
            if (const auto* t = s.find("type"))
            {
                if (t->is_string()) out.push_back(t->text);
                for (const auto& e : t->items)
                    if (e.is_string() && std::ranges::find(out, e.text) == out.end()) out.push_back(e.text);
            }
            return out;
        }

        inline json type_json(const std::vector<std::string>& types)
        {
            if (types.size() == 1) return json::string_of(types[0]);
            auto items = std::vector<json>();
            for (const auto& t : types) items.push_back(json::string_of(t));
            return json::array_of(std::move(items));
        }

        inline std::string percent_decode(std::string_view s)
        {
            auto out = std::string();
            const auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            for (std::size_t i = 0; i < s.size(); ++i)
            {
                if (s[i] == '%' && i + 2 < s.size() && hex(s[i + 1]) >= 0 && hex(s[i + 2]) >= 0)
                {
                    out += static_cast<char>(hex(s[i + 1]) * 16 + hex(s[i + 2]));
                    i += 2;
                }
                else
                    out += s[i];
            }
            return out;
        }

        class generator
        {
            const options& opts_;
            json root_;
            std::deque<json> store_;                        // schemas built by the generator
            std::deque<json> files_;                        // schemas of other files
            std::map<std::string, const json*> documents_;  // key of a reference ("" or a file path) -> schema
            std::map<std::string, std::string> ids_;        // absolute $id of a file -> its key
            std::string root_path_;                         // lexically normal path of the schema
            std::set<std::string> schema_names_;            // schema_name of $defs entries
            std::deque<decl> decls_;
            std::map<const json*, std::string> def_names_;  // $defs entries -> type name
            std::map<const json*, std::string> def_keys_;   // $defs entries -> schema_name
            std::map<const json*, typed> cache_;
            std::set<const json*> busy_;
            std::map<const json*, const json*> normalized_;
            std::set<const json*> normalizing_;
            std::vector<variant_note> notes_;
            std::set<std::string> type_names_;
            std::vector<int> position_;  // the place of a struct in definition order, -1 for others
            mutable bool partial_init_ = false;  // a designated initializer leaves members out
            // the functions of yyjson::default_from: name, return type and returned expression
            struct default_function
            {
                std::string name, type, value;
            };
            std::vector<default_function> default_functions_;
            // the settings of alternatives that C++20 takes through pointers to variables: on in render(), where
            // spell() names a variable after the place of the type (`ctx_`, the indices of the alternatives in
            // `path_`) and the kind of the setting, and the same setting text reuses it
            bool naming_ = false;
            mutable std::string ctx_;
            mutable std::vector<std::size_t> path_;
            mutable std::map<std::string, std::string, std::less<>> setting_vars_;  // setting text -> variable
            mutable std::vector<std::pair<std::string, std::string>> var_defs_;     // name, setting text
            mutable std::set<std::string, std::less<>> interned_;  // the strings of bounds of alternatives
            bool uses_pattern_ = false;
            std::vector<diagnostic> warnings_;
            std::set<std::pair<std::string, std::string>> warned_;

            void warn(const std::string& loc, std::string msg)
            {
                if (warned_.emplace(loc, msg).second) warnings_.push_back({loc, std::move(msg)});
            }

            [[noreturn]] static void fail(std::string loc, std::string msg)
            {
                throw error({diagnostic{std::move(loc), std::move(msg)}});
            }

            const json* store(json j)
            {
                store_.push_back(std::move(j));
                return &store_.back();
            }

            static bool is_absolute_uri(std::string_view r)
            {
                const auto colon = r.find(':');
                if (colon == std::string_view::npos || colon == 0 || !std::isalpha(static_cast<unsigned char>(r[0])))
                    return false;
                return std::ranges::all_of(r.substr(0, colon), [](char c) {
                    return std::isalnum(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.';
                });
            }

            static std::string file_key(const std::string& dir, std::string_view rel)
            {
                return (std::filesystem::path(dir) / std::filesystem::path(std::string(rel)))
                    .lexically_normal()
                    .generic_string();
            }

            static bool is_draft07(const json& doc)
            {
                const auto* d = doc.find("$schema");
                const auto has = [&](std::string_view x) { return d->text.find(x) != std::string::npos; };
                return d != nullptr && d->is_string() && (has("draft-07/") || has("draft-06/"));
            }

            void check_dialect(const json& doc, const std::string& loc)
            {
                const auto* d = doc.find("$schema");
                if (d == nullptr) return;
                const auto dialect = d->is_string() ? d->text : std::string();
                if (is_draft07(doc) || dialect.find("draft/2020-12/schema") != std::string::npos ||
                    dialect.find("draft/2019-09/schema") != std::string::npos)
                    return;
                warn(loc, "the dialect \"" + dialect + "\" is read as JSON Schema 2020-12");
            }

            // a reference as "key#fragment" or "key": the key is empty for the schema, the path of another file, or a
            // URI
            std::string canonical_ref(std::string_view r, const std::string& key, const std::string& id) const
            {
                const auto hash = r.find('#');
                const auto base = r.substr(0, hash);
                const auto fragment = hash == std::string_view::npos ? std::string_view() : r.substr(hash);
                const auto dir = std::filesystem::path(key.empty() ? root_path_ : key).parent_path().generic_string();
                auto target = std::string();
                if (base.empty())
                    target = key;
                else if (is_absolute_uri(base))
                {
                    // a URI under the directory of the $id of the file names a file beside it
                    const auto slash = id.rfind('/');
                    if (base == id)
                        target = key;
                    else if (slash != std::string::npos && base.starts_with(std::string_view(id).substr(0, slash + 1)))
                        target = file_key(dir, base.substr(slash + 1));
                    else
                        target = std::string(base);
                }
                else
                    target = file_key(dir, base);
                if (!target.empty() && target == root_path_) target.clear();
                return target.empty() && fragment.empty() ? std::string("#") : target + std::string(fragment);
            }

            // rewrites the references of a file to their canonical form, and drops the keywords beside a reference
            // of draft-07, which ignores them
            void prepare(json& j, const std::string& key, const std::string& id, bool draft07, const std::string& loc)
            {
                if (j.is_object())
                {
                    if (auto* r = const_cast<json*>(j.find("$ref")); r != nullptr && r->is_string())
                    {
                        r->text = canonical_ref(r->text, key, id);
                        if (draft07 && !is_pure_ref(j))
                        {
                            auto dropped = std::string();
                            std::erase_if(j.members, [&](const auto& m) {
                                if (m.first == "$ref" || is_metadata_key(m.first)) return false;
                                dropped += (dropped.empty() ? "`" : ", `") + m.first + "`";
                                return true;
                            });
                            warn(loc, "draft-07 ignores the keywords beside `$ref`: " + dropped);
                        }
                    }
                    for (auto& [k, v] : j.members)
                        if (k != "enum" && k != "const" && k != "default" && k != "examples")
                            prepare(v, key, id, draft07, loc + "/" + pointer_token(k));
                }
                for (std::size_t i = 0; i < j.items.size(); ++i)
                    prepare(j.items[i], key, id, draft07, loc + "/" + std::to_string(i));
            }

            static std::string absolute_id(const json& doc)
            {
                const auto* id = doc.find("$id");
                if (id == nullptr || !id->is_string() || !is_absolute_uri(id->text)) return {};
                return id->text.substr(0, id->text.find('#'));
            }

            // the schema of a key, read from its file when it is not read yet; nullptr with a warning
            const json* document(const std::string& key, const std::string& r, const std::string& loc)
            {
                if (key.empty()) return &root_;
                if (const auto it = documents_.find(key); it != documents_.end()) return it->second;
                if (const auto it = ids_.find(key); it != ids_.end()) return documents_.at(it->second);
                if (is_absolute_uri(key))
                {
                    warn(loc, "the reference \"" + r +
                                  "\" is a URI that is not the `$id` of a schema read; it is read as any JSON value");
                    return nullptr;
                }
                if (!opts_.load)
                {
                    warn(loc, "the reference \"" + r + "\" is not in this document and is read as any JSON value");
                    return nullptr;
                }
                auto text = opts_.load(key);
                if (!text)
                {
                    warn(loc, "the file \"" + key + "\" of the reference \"" + r +
                                  "\" cannot be read; it is read as any JSON value");
                    return nullptr;
                }
                auto doc = json();
                try
                {
                    doc = to_json(yyjson::read(*text));
                }
                catch (const std::exception& e)
                {
                    warn(loc, "the file \"" + key + "\" is not valid JSON (" + e.what() +
                                  "); the reference is read as any JSON value");
                    return nullptr;
                }
                const auto* d = &files_.emplace_back(std::move(doc));
                documents_[key] = d;
                check_dialect(*d, key + "#");
                const auto id = absolute_id(*d);
                if (!id.empty()) ids_.emplace(id, key);
                prepare(files_.back(), key, id, is_draft07(*d), key + "#");
                return d;
            }

            // a type name for a schema of another file: its root, or an entry of its $defs
            void name_target(const json* doc, const json* t, const std::string& key)
            {
                if (doc == &root_ || def_names_.contains(t)) return;
                if (t == doc)
                {
                    const auto* title = doc->find("title");
                    const auto stem = std::filesystem::path(key).filename().string();
                    const auto titled = title != nullptr && title->is_string() && !pascal(title->text).empty();
                    def_names_[t] =
                        unique_type_name(titled ? pascal(title->text) : pascal(stem.substr(0, stem.find('.'))));
                    return;
                }
                for (const auto* kw : {"$defs", "definitions"})
                    if (const auto* d = doc->find(kw); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                            if (&v == t) name_def(t, k);
            }

            void name_def(const json* t, const std::string& k)
            {
                def_names_[t] = unique_type_name(k);
                if (is_schema_name(k) && schema_names_.insert(k).second) def_keys_[t] = k;
            }

            // the target of a $ref, or nullptr with a warning
            const json* resolve(const json& ref, const std::string& loc)
            {
                if (!ref.is_string())
                {
                    warn(loc, "`$ref` is not a string");
                    return nullptr;
                }
                const auto& r = ref.text;
                const auto hash = r.find('#');
                const auto key = r.substr(0, hash);
                const auto* doc = document(key, r, loc);
                if (doc == nullptr) return nullptr;
                const auto fragment =
                    hash == std::string::npos ? std::string() : percent_decode(std::string_view(r).substr(hash + 1));
                if (!fragment.empty() && fragment[0] != '/')
                {
                    if (const auto* a = find_anchor(*doc, fragment))
                    {
                        name_target(doc, a, key);
                        return a;
                    }
                    warn(loc, "the anchor \"" + r + "\" is not found");
                    return nullptr;
                }
                const auto* cur = doc;
                for (std::size_t pos = 1; pos <= fragment.size() && !fragment.empty();)
                {
                    const auto end = std::min(fragment.find('/', pos), fragment.size());
                    const auto tok = unpointer_token(std::string_view(fragment).substr(pos, end - pos));
                    pos = end + 1;
                    if (cur->is_object())
                        cur = cur->find(tok);
                    else if (cur->is_array())
                    {
                        auto i = std::size_t();
                        const auto [p, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), i);
                        cur = ec == std::errc() && p == tok.data() + tok.size() && i < cur->items.size()
                                  ? &cur->items[i]
                                  : nullptr;
                    }
                    else
                        cur = nullptr;
                    if (cur == nullptr)
                    {
                        warn(loc, "the reference \"" + r + "\" is not found");
                        return nullptr;
                    }
                }
                name_target(doc, cur, key);
                return cur;
            }

            static const json* find_anchor(const json& j, std::string_view name)
            {
                if (j.is_object())
                {
                    if (const auto* a = j.find("$anchor"); a != nullptr && a->is_string() && a->text == name) return &j;
                    // draft-07 names a schema by a fragment in $id
                    if (const auto* a = j.find("$id");
                        a != nullptr && a->is_string() && a->text.size() == name.size() + 1 && a->text[0] == '#' &&
                        std::string_view(a->text).substr(1) == name)
                        return &j;
                    for (const auto& [k, v] : j.members)
                        if (k != "enum" && k != "const" && k != "default" && k != "examples")
                            if (const auto* f = find_anchor(v, name)) return f;
                }
                for (const auto& e : j.items)
                    if (const auto* f = find_anchor(e, name)) return f;
                return nullptr;
            }

            // follows a chain of references that have only metadata beside them
            const json* deref(const json* s, const std::string& loc)
            {
                auto seen = std::set<const json*>();
                while (is_pure_ref(*s))
                {
                    if (!seen.insert(s).second)
                    {
                        warn(loc, "a chain of references refers to itself");
                        return store(json::boolean_of(true));
                    }
                    const auto* t = resolve(*s->find("$ref"), loc);
                    if (t == nullptr) return store(json::boolean_of(true));
                    s = t;
                }
                return s;
            }

            const json* normalize_full(const json* s, const std::string& loc) { return normalize(deref(s, loc), loc); }

            // merges allOf, and $ref with sibling keywords, into one schema
            const json* normalize(const json* s, const std::string& loc)
            {
                if (!s->is_object()) return s;
                const auto has_all = s->has("allOf");
                const auto ref_siblings = s->has("$ref") && !is_pure_ref(*s);
                const auto* items = s->find("items");
                const auto legacy =
                    (items != nullptr && items->is_array()) || s->has("additionalItems") || s->has("dependencies");
                if (!has_all && !ref_siblings && !legacy) return s;
                if (const auto it = normalized_.find(s); it != normalized_.end()) return it->second;
                if (!normalizing_.insert(s).second)
                {
                    warn(loc, "`allOf` refers to itself; the schema is read as any JSON value");
                    return store(json::boolean_of(true));
                }
                auto base = json::object_of();
                for (const auto& [k, v] : s->members)
                    if (k != "allOf" && k != "$ref") base.members.emplace_back(k, v);
                upgrade(base);
                const auto* all = s->find("allOf");
                // The elements of allOf that give the type something.  The others give metadata, which the base
                // takes, and keywords that are ignored: merging them would turn a reference to a recursive type
                // into a copy of it at every level.
                auto parts = std::vector<std::pair<const json*, std::string>>();
                if (all != nullptr && all->is_array())
                    for (std::size_t i = 0; i < all->items.size(); ++i)
                    {
                        const auto& e = all->items[i];
                        auto l = loc + "/allOf/" + std::to_string(i);
                        if (!adds_nothing(e))
                            parts.emplace_back(&e, std::move(l));
                        else if (e.is_object())
                        {
                            warn_unsupported(e, l);
                            for (const auto& [k, v] : e.members)
                                if (is_metadata_key(k) && !base.has(k)) base.set(k, v);
                        }
                    }
                // {"allOf": [{"$ref": ...}]} with metadata only, or with a `const` that the referred schema already
                // fixes, as an enumeration of one value: keep the reference to the named type
                const auto keeps_ref = [&] {
                    if (s->has("$ref") || all == nullptr || !all->is_array() || parts.size() != 1 ||
                        !is_pure_ref(*parts[0].first))
                        return false;
                    const auto* c = base.find("const");
                    if (c == nullptr) return adds_nothing(base);
                    auto rest = base;
                    std::erase_if(rest.members, [](const auto& m) { return m.first == "const"; });
                    if (!adds_nothing(rest)) return false;
                    const auto* t = resolve(*parts[0].first->find("$ref"), parts[0].second);
                    if (t == nullptr) return false;
                    const auto* n = normalize_full(t, parts[0].second);
                    const auto* e = n->is_object() ? n->find("enum") : nullptr;
                    const auto* k = n->is_object() ? n->find("const") : nullptr;
                    if (!(k != nullptr && equal(*k, *c)) &&
                        !(e != nullptr && e->is_array() && e->items.size() == 1 && equal(e->items[0], *c)))
                        return false;
                    base = std::move(rest);
                    return true;
                };
                if (keeps_ref())
                {
                    warn_unsupported(base, loc);
                    std::erase_if(base.members, [](const auto& m) { return is_ignored_key(m.first); });
                    base.set("$ref", *parts[0].first->find("$ref"));
                    const auto* r = store(std::move(base));
                    normalizing_.erase(s);
                    return normalized_[s] = r;
                }
                if (const auto* ref = s->find("$ref"))
                    if (const auto* t = resolve(*ref, loc)) base = merge(base, *normalize_full(t, loc), loc);
                if (all != nullptr && !all->is_array()) warn(loc, "`allOf` is not an array");
                for (const auto& [e, l] : parts) base = merge(base, *normalize_full(e, l), l);
                const auto* r = store(std::move(base));
                normalizing_.erase(s);
                return normalized_[s] = r;
            }

            // the keywords of draft-07 as their 2020-12 counterparts: `items` of an array and `additionalItems` as
            // `prefixItems` and `items`, `dependencies` as `dependentRequired` and `dependentSchemas`
            static void upgrade(json& s)
            {
                const auto erase = [&](std::string_view k) {
                    std::erase_if(s.members, [&](const auto& m) { return m.first == k; });
                };
                if (const auto* items = s.find("items"); items != nullptr && items->is_array())
                {
                    auto prefix = *items;
                    const auto* rest = s.find("additionalItems");
                    auto more = rest != nullptr ? std::optional<json>(*rest) : std::nullopt;
                    erase("items");
                    erase("additionalItems");
                    s.set("prefixItems", std::move(prefix));
                    if (more) s.set("items", std::move(*more));
                }
                erase("additionalItems");  // ignored without an array of `items`
                if (const auto* deps = s.find("dependencies"); deps != nullptr && deps->is_object())
                {
                    auto required = s.has("dependentRequired") ? *s.find("dependentRequired") : json::object_of();
                    auto schemas = s.has("dependentSchemas") ? *s.find("dependentSchemas") : json::object_of();
                    for (const auto& [k, v] : deps->members)
                    {
                        auto& to = v.is_array() ? required : schemas;
                        if (!to.has(k)) to.set(k, v);
                    }
                    erase("dependencies");
                    if (!required.members.empty()) s.set("dependentRequired", std::move(required));
                    if (!schemas.members.empty()) s.set("dependentSchemas", std::move(schemas));
                }
                erase("dependencies");
            }

            // both schemas; one of them when they are equal, so that a schema repeated beside a reference to a
            // recursive type does not inline the type again at every level
            static json all_of(const json& a, const json& b)
            {
                if (equal(a, b)) return a;
                auto j = json::object_of();
                j.set("allOf", json::array_of({a, b}));
                return j;
            }

            json merge(const json& a, const json& b, const std::string& loc)
            {
                if (a.is_bool()) return a.b ? b : a;
                if (b.is_bool()) return b.b ? a : b;
                if (!a.is_object() || !b.is_object()) return a;
                auto r = a;
                for (const auto& [k, bv] : b.members)
                {
                    const auto* av = r.find(k);
                    if (av == nullptr)
                    {
                        r.set(k, bv);
                        continue;
                    }
                    if (k == "type")
                    {
                        const auto ta = type_list(r), tb = type_list(b);
                        auto out = std::vector<std::string>();
                        for (const auto& t : ta)
                        {
                            const auto in_b = [&](std::string_view x) { return std::ranges::find(tb, x) != tb.end(); };
                            auto m = in_b(t)                            ? t
                                     : t == "integer" && in_b("number") ? t
                                     : t == "number" && in_b("integer") ? "integer"
                                                                        : "";
                            if (!m.empty() && std::ranges::find(out, m) == out.end()) out.push_back(m);
                        }
                        if (out.empty())
                            warn(loc, "the types of `allOf` have no common type; the first is used");
                        else
                            r.set("type", type_json(out));
                    }
                    else if (k == "properties" && av->is_object() && bv.is_object())
                    {
                        auto props = *av;
                        for (const auto& [pk, pv] : bv.members)
                        {
                            if (const auto* x = props.find(pk))
                                props.set(pk, all_of(*x, pv));
                            else
                                props.members.emplace_back(pk, pv);
                        }
                        r.set(k, std::move(props));
                    }
                    else if (k == "required" && av->is_array() && bv.is_array())
                    {
                        auto req = *av;
                        for (const auto& e : bv.items)
                            if (std::ranges::none_of(req.items, [&](const auto& x) { return equal(x, e); }))
                                req.items.push_back(e);
                        r.set(k, std::move(req));
                    }
                    else if (k == "additionalProperties")
                    {
                        if (av->is_bool() || bv.is_bool())
                            r.set(k, merge(*av, bv, loc));
                        else
                            r.set(k, all_of(*av, bv));
                    }
                    else if (k == "items" || k == "contains" || k == "propertyNames")
                        r.set(k, all_of(*av, bv));
                    else if (k == "minimum" || k == "exclusiveMinimum" || k == "minLength" || k == "minItems" ||
                             k == "minProperties" || k == "minContains")
                    {
                        if (av->is_number() && bv.is_number() && bv.num > av->num) r.set(k, bv);
                    }
                    else if (k == "maximum" || k == "exclusiveMaximum" || k == "maxLength" || k == "maxItems" ||
                             k == "maxProperties" || k == "maxContains")
                    {
                        if (av->is_number() && bv.is_number() && bv.num < av->num) r.set(k, bv);
                    }
                    else if (k == "enum" && av->is_array() && bv.is_array())
                    {
                        auto e = json::array_of({});
                        for (const auto& x : av->items)
                            if (std::ranges::any_of(bv.items, [&](const auto& y) { return equal(x, y); }))
                                e.items.push_back(x);
                        r.set(k, std::move(e));
                    }
                    else if (k == "uniqueItems")
                    {
                        if (bv.is_bool() && bv.b) r.set(k, bv);
                    }
                    else if (k == "dependentRequired" && av->is_object() && bv.is_object())
                    {
                        auto dep = *av;
                        for (const auto& [dk, dv] : bv.members)
                        {
                            if (const auto* x = dep.find(dk); x != nullptr && x->is_array() && dv.is_array())
                            {
                                auto keys = *x;
                                for (const auto& e : dv.items)
                                    if (std::ranges::none_of(keys.items, [&](const auto& y) { return equal(y, e); }))
                                        keys.items.push_back(e);
                                dep.set(dk, std::move(keys));
                            }
                            else if (x == nullptr)
                                dep.members.emplace_back(dk, dv);
                        }
                        r.set(k, std::move(dep));
                    }
                    else if (!is_metadata_key(k) && !equal(*av, bv))
                        warn(loc, "`allOf` has different values of `" + k + "`; the first is used");
                }
                return r;
            }

            static metadata meta_of(const json& s)
            {
                auto m = metadata();
                if (!s.is_object()) return m;
                if (const auto* t = s.find("title"); t != nullptr && t->is_string()) m.title = t->text;
                if (const auto* t = s.find("description"); t != nullptr && t->is_string()) m.description = t->text;
                if (const auto* t = s.find("$comment"); t != nullptr && t->is_string()) m.comment = t->text;
                if (const auto* t = s.find("deprecated"); t != nullptr && t->is_bool()) m.deprecated = t->b;
                if (const auto* t = s.find("examples"); t != nullptr && t->is_array())
                    m.examples = t->items;
                return m;
            }

            std::string unique_type_name(std::string_view base)
            {
                const auto b = sanitize(base.empty() ? "Value" : base);
                auto name = b;
                for (auto n = 2; type_names_.contains(name); ++n) name = b + std::to_string(n);
                type_names_.insert(name);
                return name;
            }

            int add_decl(decl d)
            {
                decls_.push_back(std::move(d));
                return static_cast<int>(decls_.size()) - 1;
            }

            // the JSON types of a schema; empty for any value
            std::vector<std::string> types_of(const json& s, const std::string& loc)
            {
                auto t = type_list(s);
                if (const auto* v = s.find("type"); v != nullptr && t.empty())
                    warn(loc, "`type` is not a string or an array of strings");
                if (!t.empty())
                {
                    if (std::ranges::find(t, "number") != t.end()) std::erase(t, "integer");
                    for (const auto& x : t)
                        if (x != "null" && x != "boolean" && x != "object" && x != "array" && x != "number" &&
                            x != "string" && x != "integer")
                        {
                            warn(loc, "unknown type \"" + x + "\"; the schema is read as any JSON value");
                            return {};
                        }
                    return t;
                }
                const auto any_of_keys = [&](std::initializer_list<std::string_view> keys) {
                    return std::ranges::any_of(keys, [&](auto k) { return s.has(k); });
                };
                auto groups = std::vector<std::string>();
                if (any_of_keys({"properties", "required", "additionalProperties", "patternProperties", "minProperties",
                                 "maxProperties", "propertyNames", "dependentRequired"}))
                    groups.emplace_back("object");
                if (any_of_keys({"items", "prefixItems", "minItems", "maxItems", "uniqueItems", "contains",
                                 "minContains", "maxContains"}))
                    groups.emplace_back("array");
                if (any_of_keys({"minLength", "maxLength", "pattern", "format"})) groups.emplace_back("string");
                if (any_of_keys({"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf"}))
                    groups.emplace_back("number");
                if (groups.size() == 1)
                {
                    warn(loc, "the schema has no `type`; it is read as " + groups[0] + " from its keywords");
                    return groups;
                }
                if (groups.size() > 1)
                    warn(loc, "the schema has no `type` and keywords of several types; it is read as any JSON value");
                return {};
            }

            typed type_of(const json* s0, const std::string& loc, const std::string& hint)
            {
                if (const auto it = cache_.find(s0); it != cache_.end()) return it->second;
                if (s0->is_bool())
                {
                    if (!s0->b) warn(loc, "the schema `false` accepts no value; it is read as any JSON value");
                    return {};
                }
                if (!s0->is_object())
                {
                    warn(loc, "a schema is not an object or a boolean; it is read as any JSON value");
                    return {};
                }
                if (busy_.contains(s0))
                {
                    warn(loc, "the schema refers to itself without an object between; it is read as any JSON value");
                    return {};
                }
                if (def_names_.contains(s0)) return type_named(s0, loc);
                busy_.insert(s0);
                auto r = type_content(s0, loc, hint, false);
                busy_.erase(s0);
                return cache_[s0] = r;
            }

            int find_decl(std::string_view name) const
            {
                for (std::size_t i = 0; i < decls_.size(); ++i)
                    if (decls_[i].name == name) return static_cast<int>(i);
                return -1;
            }

            // a schema of $defs, or the root: a struct or an enum of that name, or an alias
            typed type_named(const json* s0, const std::string& loc)
            {
                const auto name = def_names_.at(s0);
                busy_.insert(s0);
                auto r = type_content(s0, loc, name, true);
                busy_.erase(s0);
                if (find_decl(name) >= 0) return cache_[s0] = r;
                if (s0 == &root_ && !r.s.empty())
                {
                    // the constraints of a root that is not an object have no field to go on: a struct of one field
                    // with yyjson::transparent, which is read and written as the value of the field, takes them
                    auto d = decl{.kind = decl::kind_t::structure, .name = name, .meta = meta_of(*s0), .loc = loc};
                    // a root that refers to an alias takes the metadata of the alias, which has no place for it
                    if (const auto* t = is_pure_ref(*s0) ? resolve(*s0->find("$ref"), loc) : nullptr; t != nullptr)
                    {
                        auto m = meta_of(*t);
                        if (!d.meta.title) d.meta.title = std::move(m.title);
                        if (!d.meta.description) d.meta.description = std::move(m.description);
                        if (!d.meta.comment) d.meta.comment = std::move(m.comment);
                        d.meta.deprecated = d.meta.deprecated || m.deprecated;
                        if (d.meta.examples.empty()) d.meta.examples = std::move(m.examples);
                    }
                    d.transparent = true;
                    d.fields.push_back(field{.member = "value", .key = "value", .type = r.t, .set = r.s, .loc = loc});
                    d.filled = true;
                    return cache_[s0] = typed{named_type(add_decl(std::move(d))), {}};
                }
                // the metadata of an alias goes only to a comment: a `using` declaration takes no annotations
                auto d = decl{.kind = decl::kind_t::alias,
                              .name = name,
                              .meta = meta_of(*s0),
                              .loc = loc,
                              .target = r.t,
                              .set = r.s};
                if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                const auto idx = add_decl(std::move(d));
                if (r.t.k == tk::variant && r.t.tagged)
                    for (auto& n : notes_)
                        if (n.alias < 0 && spell(n.type) == spell(r.t)) n.alias = idx;
                return cache_[s0] = typed{named_type(idx), r.s};
            }

            void warn_unsupported(const json& s, const std::string& loc)
            {
                for (const auto k : ignored_keys)
                    if (s.has(k)) warn(loc, "`" + std::string(k) + "` is not supported and is ignored");
            }

            typed type_content(const json* s0, const std::string& loc, const std::string& hint, bool named)
            {
                const auto* s = normalize(s0, loc);
                if (!s->is_object()) return s->is_bool() && !s->b ? type_of(s, loc, hint) : typed{};
                warn_unsupported(*s, loc);
                if (const auto* ref = s->find("$ref"))
                {
                    const auto* t = resolve(*ref, loc);
                    if (t == nullptr) return {};
                    return type_of(t, ref->text.find('#') == std::string::npos ? ref->text + "#" : ref->text, hint);
                }
                if ((s->has("anyOf") && !required_groups_of(*s, "anyOf")) ||
                    (s->has("oneOf") && !required_groups_of(*s, "oneOf")))
                    return type_variant(s0, s, loc, hint, named);
                if (s->has("const")) return type_const(*s, loc);
                if (s->has("enum")) return type_enum(s0, *s, loc, hint, named);
                auto types = types_of(*s, loc);
                const auto nullable = std::erase(types, "null") > 0;
                if (types.empty()) return nullable ? typed{make(tk::null), {}} : typed{};
                if (types.size() == 1) return type_kind(types[0], s0, *s, loc, hint, named, nullable);
                auto alts = std::vector<typed>();
                if (nullable) alts.push_back({make(tk::monostate), {}});
                for (const auto& k : types) alts.push_back(type_kind(k, s0, *s, loc, hint + pascal(k), false, false));
                return make_variant(std::move(alts), loc);
            }

            typed type_kind(const std::string& kind, const json* s0, const json& s, const std::string& loc,
                            const std::string& hint, bool named, bool nullable)
            {
                auto r = typed();
                if (kind == "object")
                {
                    if (s.has("properties") || s.has("required")) return make_struct(s0, s, loc, hint, named, nullable);
                    r = type_map(s, loc, hint);
                }
                else if (kind == "array")
                    r = type_array(s, loc, hint);
                else if (kind == "string")
                    r = {make(tk::string), {}};
                else if (kind == "integer")
                {
                    const auto* f = s.find("format");
                    const auto* mn = s.find("minimum");
                    const auto* mx = s.find("maximum");
                    if (f != nullptr && f->is_string() && f->text == "int32")
                        r = {make(tk::i32), {}};
                    else if (mn != nullptr && mn->is_number() && mn->num >= 0 && mx != nullptr && mx->is_number() &&
                             !as_int64(*mx) && as_uint64(*mx))
                        r = {make(tk::u64), {}};
                    else
                        r = {make(tk::i64), {}};
                }
                else if (kind == "number")
                {
                    const auto* f = s.find("format");
                    r = {make(f != nullptr && f->is_string() && f->text == "float" ? tk::f32 : tk::f64), {}};
                }
                else if (kind == "boolean")
                    r = {make(tk::boolean), {}};
                if (r.t.k != tk::vector && r.t.k != tk::map && r.t.k != tk::object && r.t.k != tk::any)
                    scalar_constraints(s, r.t.k, r.s, loc);
                return nullable ? make_nullable(std::move(r)) : r;
            }

            typed make_struct(const json* s0, const json& s, const std::string& loc, const std::string& hint,
                              bool named, bool nullable)
            {
                auto d =
                    decl{.kind = decl::kind_t::structure, .name = named ? hint : unique_type_name(hint), .loc = loc};
                if (named)
                {
                    d.meta = meta_of(s);
                    if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                }
                const auto idx = add_decl(std::move(d));
                auto r = typed{named_type(idx), {}};
                if (nullable) r = make_nullable(std::move(r));
                cache_[s0] = r;
                fill_struct(idx, s, loc);
                return r;
            }

            // The map that `patternProperties` and `additionalProperties` give together where one schema holds for
            // the values of every key: `values` is that schema, or nullptr for any value, and `pattern` the pattern
            // that every key matches, or none for any key.  nullopt where a key needs the schema that its name
            // selects, which C++ does not express.
            struct pattern_map
            {
                const json* values = nullptr;
                std::string values_loc;
                std::optional<std::string> pattern;
            };

            static bool accepts_anything(const json& j)
            {
                static const auto meta = std::set<std::string_view>{
                    "title", "description", "$comment", "examples", "default", "deprecated", "readOnly", "writeOnly"};
                return (j.is_bool() && j.b) ||
                       (j.is_object() && std::ranges::all_of(j.members, [](const auto& m) { return meta.contains(m.first); }));
            }

            // the keys of `patternProperties` that match every key: a pattern that matches an empty part of every
            // string; "^.*$" is not one, since `.` does not match a line break
            static bool matches_every_key(std::string_view p)
            {
                return p.empty() || p == ".*" || p == "^.*" || p == ".*$" || p == "^" || p == "$";
            }

            // `s` has `patternProperties` with at least one pattern
            std::optional<pattern_map> pattern_map_of(const json& s, const std::string& loc)
            {
                const auto& pp = *s.find("patternProperties");
                const auto* ap = s.find("additionalProperties");
                const auto ap_any = ap == nullptr || accepts_anything(*ap);
                const auto ap_false = ap != nullptr && ap->is_bool() && !ap->b;
                const auto& [first_key, first] = pp.members.front();
                if (std::ranges::any_of(pp.members, [](const auto& m) { return m.second.is_bool() && !m.second.b; }))
                    return std::nullopt;
                const auto all_any =
                    std::ranges::all_of(pp.members, [](const auto& m) { return accepts_anything(m.second); });
                if (!all_any &&
                    !std::ranges::all_of(pp.members, [&](const auto& m) { return equal(m.second, first); }))
                    return std::nullopt;
                auto r = pattern_map{.values = all_any ? nullptr : &first,
                                     .values_loc = loc + "/patternProperties/" + pointer_token(first_key),
                                     .pattern = std::nullopt};
                if (std::ranges::any_of(pp.members, [](const auto& m) { return matches_every_key(m.first); }))
                    return r;
                if (ap_false)
                {
                    auto pattern = std::string();
                    for (const auto& [k, v] : pp.members)
                        pattern += pp.members.size() == 1 ? k : (pattern.empty() ? "" : "|") + ("(?:" + k + ")");
                    r.pattern = std::move(pattern);
                    if (all_any) r.values = &first;
                    return r;
                }
                if (all_any && ap_any) return r;
                if (!all_any && ap != nullptr && equal(*ap, first))
                    return r;
                return std::nullopt;
            }

            // adds the pattern of the keys that `patternProperties` names to the settings of the keys
            void add_key_pattern(settings& out, const std::string& pattern, const std::string& loc)
            {
                if (!opts_.pattern)
                {
                    warn(loc, "the pattern of the keys of `patternProperties` is dropped (--no-pattern)");
                    return;
                }
                if (out.property_names.empty()) out.property_names.emplace_back();
                auto& names = out.property_names.front();
                if (std::ranges::any_of(names.constraints, [](const auto& c) { return c.keyword == "pattern"; }))
                {
                    warn(loc, "`propertyNames` has a pattern; the keys are not checked against the patterns of "
                              "`patternProperties`");
                    return;
                }
                names.constraints.push_back(
                    {"yyjson::pattern(" + cpp_string(pattern) + ")", "pattern", json::string_of(pattern)});
                uses_pattern_ = true;
            }

            // warns of the properties whose keys match a pattern of `patternProperties`, whose schema JSON Schema
            // checks their values against too
            void warn_matched_properties(const json& s, const std::vector<field>& fields, const std::string& loc)
            {
                for (const auto& [p, sub] : s.find("patternProperties")->members)
                {
                    if (accepts_anything(sub)) continue;
                    const auto* props = s.find("properties");
                    const auto same = [&](const field& f) {
                        const auto* own = props != nullptr ? props->find(f.key) : nullptr;
                        return own != nullptr && equal(*own, sub);
                    };
                    try
                    {
                        const auto re = std::regex(p, std::regex::ECMAScript);
                        for (const auto& f : fields)
                            if (!f.extra && !same(f) && std::regex_search(f.key, re))
                                warn(f.loc, "the key \"" + f.key + "\" matches the pattern \"" + p +
                                                "\" of `patternProperties`, whose schema is not checked for it");
                    }
                    catch (const std::regex_error&)
                    {
                        warn(loc + "/patternProperties/" + pointer_token(p),
                             "the pattern is not checked against the keys of the properties: std::regex does not "
                             "read it");
                    }
                }
            }

            // The schema of `propertyNames` `pn` for the keys that name no property of `fields`, and whether it
            // holds for the keys of the properties too, as in `{"anyOf": [{"enum": [the keys of the properties]},
            // schema]}`, which `yyjson::json_schema` gives
            static std::pair<const json*, bool> names_of_other_keys(const json& pn, const std::vector<field>& fields)
            {
                const auto* any_of = pn.find("anyOf");
                if (pn.members.size() != 1 || any_of == nullptr || !any_of->is_array() || any_of->items.size() != 2)
                    return {&pn, fields.empty()};
                for (const auto n : {0, 1})
                {
                    const auto* e = any_of->items[n].find("enum");
                    if (e == nullptr || any_of->items[n].members.size() != 1 || !e->is_array()) continue;
                    const auto is_key = [&](const json& v) {
                        return v.is_string() &&
                               std::ranges::any_of(fields, [&](const auto& f) { return !f.extra && f.key == v.text; });
                    };
                    if (!std::ranges::all_of(e->items, is_key)) continue;
                    const auto listed = std::ranges::all_of(fields, [&](const auto& f) {
                        return std::ranges::any_of(e->items, [&](const auto& v) { return v.text == f.key; });
                    });
                    return {&any_of->items[1 - n], listed};
                }
                return {&pn, fields.empty()};
            }

            // whether the key `k` satisfies the schema `names` of `propertyNames`, as far as the generator checks
            // it: `minLength`, `maxLength` and `pattern`; false for another keyword
            static bool key_satisfies(std::string_view k, const json& names)
            {
                if (names.is_bool()) return names.b;
                const auto length = static_cast<std::uint64_t>(
                    std::ranges::count_if(k, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
                const auto matches = [&](const std::string& pattern) {
                    try
                    {
                        return std::regex_search(k.begin(), k.end(), std::regex(pattern, std::regex::ECMAScript));
                    }
                    catch (const std::regex_error&)
                    {
                        return false;
                    }
                };
                return std::ranges::all_of(names.members, [&](const auto& m) {
                    const auto& [key, v] = m;
                    const auto n = as_uint64(v);
                    return key == "minLength"   ? n && length >= *n
                           : key == "maxLength" ? n && length <= *n
                           : key == "pattern"   ? v.is_string() && matches(v.text)
                                                : key == "title" || key == "description" || key == "$comment";
                });
            }

            void fill_struct(int idx, const json& s, const std::string& loc)
            {
                auto required = std::vector<std::string>();
                if (const auto* req = s.find("required"); req != nullptr && req->is_array())
                    for (const auto& e : req->items)
                        if (e.is_string() && std::ranges::find(required, e.text) == required.end())
                            required.push_back(e.text);
                const auto is_required = [&](std::string_view k) {
                    return std::ranges::find(required, k) != required.end();
                };
                auto fields = std::vector<field>();
                if (const auto* props = s.find("properties"); props != nullptr && props->is_object())
                {
                    for (const auto& [k, sub] : props->members)
                    {
                        const auto floc = loc + "/properties/" + pointer_token(k);
                        auto ft = type_of(&sub, floc, decls_[idx].name + pascal(k));
                        const auto* own = is_pure_ref(sub) ? &sub : normalize(&sub, floc);
                        const auto* target = normalize_full(&sub, floc);
                        const auto* dflt = own->find("default");
                        if (dflt == nullptr && target->is_object()) dflt = target->find("default");
                        if (dflt == nullptr && is_required(k) && target->is_object()) dflt = target->find("const");
                        auto f = field{.member = k, .key = k, .meta = meta_of(*own), .loc = floc};
                        f.set = std::move(ft.s);
                        if (is_required(k))
                        {
                            // the default of a required key is never used by reading; it initializes the member
                            if (dflt != nullptr) f.dflt = *dflt;
                            f.type = std::move(ft.t);
                            f.required = true;
                        }
                        else if (dflt == nullptr || (dflt->is_null() && !holds_null(ft.t)))
                        {
                            // no default, or `null` for a type without null, which authors write for "unset": the
                            // missing key is std::nullopt
                            f.type = make(tk::optional, {std::move(ft.t)});
                        }
                        else if (opts_.optional_defaults)
                        {
                            // std::nullopt is the missing key, which the program reads as the default in the comment
                            f.default_note = dump(*dflt);
                            f.type = make(tk::optional, {std::move(ft.t)});
                        }
                        else
                        {
                            // convert_defaults() makes it the initializer, once the structs it may construct are
                            // known, or makes the field std::optional
                            f.dflt = *dflt;
                            f.type = std::move(ft.t);
                        }
                        fields.push_back(std::move(f));
                    }
                }
                for (const auto& k : required)
                    if (std::ranges::none_of(fields, [&](const auto& f) { return f.key == k; }))
                        fields.push_back(field{
                            .member = k, .key = k, .type = make(tk::any), .required = true, .loc = loc + "/required"});
                // the keys that name no property: rejected, accepted and dropped, or kept in a field of
                // yyjson::extra_keys, whose map holds them by the schema of `additionalProperties`
                auto additional = true;
                auto extra = std::optional<typed>();
                const auto* ap = s.find("additionalProperties");
                const auto ap_any =
                    ap == nullptr || (ap->is_bool() && ap->b) || (ap->is_object() && ap->members.empty());
                auto key_pattern = std::optional<std::string>();
                const auto* pp = s.find("patternProperties");
                if (pp != nullptr && pp->is_object() && !pp->members.empty())
                {
                    if (auto m = pattern_map_of(s, loc))
                    {
                        warn_matched_properties(s, fields, loc);
                        if (m->values != nullptr)
                            extra = type_of(m->values, m->values_loc, decls_[idx].name + "Extra");
                        else if (m->pattern)
                            extra = typed{make(tk::any), {}};
                        key_pattern = std::move(m->pattern);
                    }
                    else
                    {
                        warn(loc + "/patternProperties",
                             std::string("`patternProperties` with a schema that the name of a key selects is not "
                                         "supported; the keys that name no property are kept in the field of "
                                         "yyjson::extra_keys as any JSON value") +
                                 (ap_any ? "" : ", and `additionalProperties` is not checked"));
                        extra = typed{make(tk::any), {}};
                    }
                }
                else if (ap != nullptr && ap->is_bool())
                    additional = ap->b;
                else if (!ap_any)
                    extra = type_of(ap, loc + "/additionalProperties", decls_[idx].name + "Extra");
                if (!extra && additional && opts_.keep_unknown_keys) extra = typed{make(tk::any), {}};
                for (const auto* k : {"minProperties", "maxProperties"})
                    if (s.has(k))
                        warn(loc,
                             std::string("`") + k + "` of an object with properties is not supported and is ignored");
                // `propertyNames` checks the keys that the field of yyjson::extra_keys receives
                auto names = std::optional<settings>();
                if (const auto* pn = s.find("propertyNames"); pn != nullptr && !extra)
                    warn(loc + "/propertyNames", "`propertyNames` of an object without a field of yyjson::extra_keys is "
                                                 "not supported and is ignored");
                else if (pn != nullptr)
                {
                    const auto [other, listed] = names_of_other_keys(*pn, fields);
                    const auto exact = listed || std::ranges::all_of(fields, [&](const auto& f) {
                                           return f.extra || key_satisfies(f.key, *other);
                                       });
                    names = settings_for(*other, make(tk::string), loc + "/propertyNames");
                    if (!names)
                        warn(loc + "/propertyNames", "`propertyNames` is not expressed by string constraints and is "
                                                     "ignored");
                    else if (!exact)
                        warn(loc + "/propertyNames",
                             "`propertyNames` is checked only for the keys that name no property");
                }
                auto dependent = std::vector<std::pair<std::string, std::vector<std::string>>>();
                if (const auto* dep = s.find("dependentRequired"); dep != nullptr && dep->is_object())
                {
                    const auto known = [&](std::string_view k) {
                        return std::ranges::any_of(fields, [&](const auto& f) { return f.key == k; });
                    };
                    for (const auto& [k, v] : dep->members)
                    {
                        auto keys = std::vector<std::string>();
                        auto ok = known(k) && v.is_array();
                        for (const auto& e : v.items)
                        {
                            ok = ok && e.is_string() && known(e.text);
                            if (ok) keys.push_back(e.text);
                        }
                        if (!ok)
                            warn(loc + "/dependentRequired/" + pointer_token(k),
                                 "`dependentRequired` with a key that is not a property is not supported and is "
                                 "ignored");
                        else if (!keys.empty())
                            dependent.emplace_back(k, std::move(keys));
                    }
                }
                auto groups = std::vector<required_groups>();
                for (const auto* kw : {"oneOf", "anyOf"})
                    if (auto g = required_groups_of(s, kw))
                    {
                        // a key that is not a property is read as any JSON value, as one of `required`
                        for (const auto& group : g->groups)
                            for (const auto& k : group)
                                if (std::ranges::none_of(fields, [&](const auto& f) { return f.key == k; }))
                                    fields.push_back(field{.member = k,
                                                           .key = k,
                                                           .type = make(tk::optional, {make(tk::any)}),
                                                           .loc = loc + "/" + kw});
                        groups.push_back(std::move(*g));
                    }
                if (!opts_.cpp26 && fields.size() > max_visitable_fields)
                    fail(loc, "a struct of more than " + std::to_string(max_visitable_fields) +
                                  " properties needs --cpp26: VISITABLE_STRUCT takes no more");
                if (extra && !opts_.cpp26 && fields.size() >= max_reflected_fields)
                {
                    // the field would make a struct that VISITABLE_STRUCT registers, which takes no
                    // yyjson::extra_keys
                    warn(loc, "a struct of more than " + std::to_string(max_reflected_fields) +
                                  " fields has no field of yyjson::extra_keys without --cpp26; the keys that name no "
                                  "property are dropped");
                    extra.reset();
                }
                if (extra)
                {
                    auto f = field{.type = make(tk::map, {std::move(extra->t)}), .loc = loc + "/additionalProperties"};
                    f.extra = true;
                    if (!extra->s.empty()) f.set.additional.push_back(std::move(extra->s));
                    if (names && !names->empty()) f.set.property_names.push_back(std::move(*names));
                    if (key_pattern) add_key_pattern(f.set, *key_pattern, loc + "/patternProperties");
                    fields.push_back(std::move(f));
                }
                auto& d = decls_[idx];
                d.fields = std::move(fields);
                d.additional = additional;
                d.dependent = std::move(dependent);
                d.groups = std::move(groups);
                d.filled = true;
            }

            // defaults

            // converts the defaults of the fields to initializers, in the definition order of the structs: an
            // initializer constructs only structs defined before the struct of the field. A default that C++ does not
            // express there makes a field that is not required std::optional, with the default in a comment
            void convert_defaults(const std::vector<int>& order)
            {
                position_.assign(decls_.size(), -1);
                for (std::size_t n = 0; n < order.size(); ++n) position_[order[n]] = static_cast<int>(n);
                for (const auto i : order)
                    for (auto& f : decls_[i].fields)
                    {
                        if (!f.dflt) continue;
                        const auto j = *std::exchange(f.dflt, std::nullopt);
                        // a field held through std::unique_ptr to break a cycle takes no initializer
                        auto init = f.type.k == tk::optional || f.type.k == tk::pointer
                                        ? std::nullopt
                                        : field_init(f.type, j, position_[i]);
                        if (f.required)
                        {
                            if (init)
                                f.init = std::move(*init);
                            else
                                warn(f.loc, "the default " + dump(j) + " is not converted to the C++ type and is dropped");
                        }
                        else if (init)
                        {
                            // a missing key means the default, which the member holds then; writing leaves the key
                            // out while the member still holds it, so a default outside the constraints, such as
                            // -1 for "no limit", is never written and read back
                            f.init = std::move(*init);
                            f.omit_default = true;
                        }
                        else if (auto fn = default_function_of(decls_[i], f, j))
                        {
                            // the member initializer cannot construct it: the struct holds itself, or the field is
                            // held through std::unique_ptr to break a cycle. A function defined after the structs
                            // makes it when the key is missing, and writing leaves the key out as above
                            f.default_from = fn->name;
                            default_functions_.push_back(std::move(*fn));
                        }
                        else
                        {
                            // std::nullopt is the missing key, which the program reads as the default in the comment
                            warn(f.loc, "the default " + dump(j) +
                                            " is not converted to the C++ type; the field is std::optional");
                            f.default_note = dump(j);
                            if (f.type.k != tk::optional) f.type = make(tk::optional, {std::move(f.type)});
                        }
                    }
            }

            // the function of yyjson::default_from for the default `j` of the field `f` of `d`, which returns the
            // value that the field holds through std::optional, std::unique_ptr and std::variant<std::monostate, T>;
            // it is defined after all the structs, whose initializers it may construct
            std::optional<default_function> default_function_of(const decl& d, const field& f, const json& j) const
            {
                auto t = &f.type;
                while (true)
                {
                    const auto& r = resolved(*t);
                    if (r.k == tk::optional || r.k == tk::pointer)
                        t = &r.args[0];
                    else if (is_nullable(r))
                        t = &r.args[1];
                    else
                        break;
                }
                if (j.is_null()) return std::nullopt;
                const auto v = initializer(*t, j, static_cast<int>(decls_.size()));
                if (!v) return std::nullopt;
                auto taken = [&](const std::string& name) {
                    return name_taken(name) || std::ranges::any_of(decls_, [&](const decl& e) {
                               return std::ranges::any_of(e.fields, [&](const field& g) { return g.member == name; });
                           });
                };
                // one underscore between the parts, since a name with "__" is reserved (`not_` -> `S_not_default`)
                auto base = d.name + "_" + f.member + "_default";
                base.erase(std::ranges::unique(base, [](char a, char b) { return a == '_' && b == '_'; }).begin(),
                           base.end());
                auto name = base;
                for (auto n = 2; taken(name); ++n) name = base + std::to_string(n);
                return default_function{name, spell(*t), typed_init(*t, *v)};
            }

            // the initializer of a field: " = x" or "{...}"
            std::optional<std::string> field_init(const ctype& t, const json& j, int before) const
            {
                const auto& r = resolved(t);
                if (r.k == tk::variant)
                {
                    // as the initializers of std::variant written by hand
                    if (holds_null(r) && j.is_null()) return "{}";
                    const auto i = alternative(r, j);
                    if (!i) return std::nullopt;
                    const auto v = initializer(r.args[*i], j, before);
                    if (!v) return std::nullopt;
                    return "{std::in_place_index<" + std::to_string(*i) + ">, " + typed_init(r.args[*i], *v) + "}";
                }
                auto v = initializer(t, j, before);
                if (!v) return std::nullopt;
                return v->starts_with("{") ? *v : " = " + *v;
            }

            // an expression of the type `t`, from an initializer of it
            std::string typed_init(const ctype& t, const std::string& v) const
            {
                return v.starts_with("{") ? spell(t) + v : v;
            }

            // the initializer of a value of the type `t`: an expression, or a braced list that copy-list-initializes
            // it; std::nullopt when C++ does not express the default here
            std::optional<std::string> initializer(const ctype& t, const json& j, int before) const
            {
                using namespace std::string_literals;
                const auto all = [&](const ctype& e, const std::vector<std::string>& keys,
                                     const std::vector<const json*>& values) -> std::optional<std::string> {
                    // std::initializer_list copies its elements
                    auto seen = std::vector<int>();
                    if (!values.empty() && !copyable(e, seen)) return std::nullopt;
                    auto out = "{"s;
                    for (std::size_t n = 0; n < values.size(); ++n)
                    {
                        auto v = initializer(e, *values[n], before);
                        if (!v) return std::nullopt;
                        if (n > 0) out += ", ";
                        out += keys.empty() ? typed_init(e, *v)
                                            : "{" + cpp_string(keys[n], "std::string") + ", " + typed_init(e, *v) + "}";
                    }
                    return out + "}";
                };
                switch (t.k)
                {
                    case tk::any:
                        // yyjson::value has no literal: it reads the JSON text
                        return "yyjson::value(yyjson::read(" + cpp_string(dump(j)) + "))";
                    case tk::object:
                        if (j.is_object()) return "yyjson::object(yyjson::read(" + cpp_string(dump(j)) + "))";
                        return std::nullopt;
                    case tk::null:
                        if (j.is_null()) return "nullptr"s;
                        return std::nullopt;
                    case tk::monostate:
                        if (j.is_null()) return "std::monostate()"s;
                        return std::nullopt;
                    case tk::boolean:
                        if (j.is_bool()) return j.b ? "true"s : "false"s;
                        return std::nullopt;
                    case tk::i32:
                        if (const auto v = as_int64(j); v && *v >= INT32_MIN && *v <= INT32_MAX) return int_literal(*v);
                        return std::nullopt;
                    case tk::i64:
                        if (const auto v = as_int64(j)) return int_literal(*v);
                        return std::nullopt;
                    case tk::u64:
                        if (const auto v = as_uint64(j)) return uint_literal(*v);
                        return std::nullopt;
                    case tk::f32:
                    case tk::f64:
                        if (j.is_number()) return float_literal(j, t.k == tk::f32);
                        return std::nullopt;
                    case tk::string:
                        if (j.is_string()) return cpp_string(j.text, "std::string");
                        return std::nullopt;
                    case tk::named:
                    {
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return initializer(d.target, j, before);
                        if (d.kind == decl::kind_t::structure) return struct_init(t.decl, j, before);
                        if (j.is_string())
                            for (const auto& [m, n] : d.enumerators)
                                if (n == j.text) return d.name + "::" + m;
                        return std::nullopt;
                    }
                    case tk::vector:
                    {
                        if (!j.is_array()) return std::nullopt;
                        auto values = std::vector<const json*>();
                        for (const auto& e : j.items) values.push_back(&e);
                        return all(t.args[0], {}, values);
                    }
                    case tk::map:
                    {
                        if (!j.is_object()) return std::nullopt;
                        auto keys = std::vector<std::string>();
                        auto values = std::vector<const json*>();
                        for (const auto& [k, v] : j.members)
                        {
                            keys.push_back(k);
                            values.push_back(&v);
                        }
                        return all(t.args[0], keys, values);
                    }
                    case tk::tuple:
                    {
                        if (!j.is_array() || j.items.size() != t.args.size()) return std::nullopt;
                        auto out = "{"s;
                        for (std::size_t n = 0; n < t.args.size(); ++n)
                        {
                            const auto v = initializer(t.args[n], j.items[n], before);
                            if (!v) return std::nullopt;
                            if (n > 0) out += ", ";
                            out += typed_init(t.args[n], *v);
                        }
                        return out + "}";
                    }
                    case tk::variant:
                    {
                        // std::in_place_index, since the constructor from a value may choose another alternative
                        if (holds_null(t) && j.is_null()) return "{}"s;
                        const auto i = alternative(t, j);
                        if (!i) return std::nullopt;
                        const auto v = initializer(t.args[*i], j, before);
                        if (!v) return std::nullopt;
                        return spell(t) + "(std::in_place_index<" + std::to_string(*i) + ">, " +
                               typed_init(t.args[*i], *v) + ")";
                    }
                    case tk::optional:
                    {
                        if (j.is_null()) return "std::nullopt"s;
                        const auto v = initializer(t.args[0], j, before);
                        if (!v) return std::nullopt;
                        return typed_init(t.args[0], *v);
                    }
                    case tk::pointer:
                    {
                        if (j.is_null()) return "nullptr"s;
                        const auto v = initializer(t.args[0], j, before);
                        if (!v) return std::nullopt;
                        return "std::make_unique<" + spell(t.args[0]) + ">(" + typed_init(t.args[0], *v) + ")";
                    }
                }
                return std::nullopt;
            }

            // designated initializers of the members that the object names; the other members keep their initializers
            std::optional<std::string> struct_init(int idx, const json& j, int before) const
            {
                // the definition of the struct comes first; a struct does not construct itself in its initializers
                if (position_[idx] < 0 || position_[idx] >= before) return std::nullopt;
                const auto& d = decls_[idx];
                const auto member = [&](const field& f, const std::string& v) {
                    return "." + f.member + " = " + v;
                };
                if (d.transparent)
                {
                    const auto v = initializer(d.fields[0].type, j, before);
                    if (!v) return std::nullopt;
                    return "{" + member(d.fields[0], *v) + "}";
                }
                if (!j.is_object()) return std::nullopt;
                auto items = std::vector<std::string>();
                auto rest = json();
                rest.k = json::kind::object;
                for (const auto& [k, v] : j.members)
                    if (std::ranges::none_of(d.fields, [&](const field& f) { return !f.extra && f.key == k; }))
                        rest.members.emplace_back(k, v);
                for (const auto& f : d.fields)
                {
                    if (f.extra)
                    {
                        if (rest.members.empty()) continue;
                        const auto v = initializer(f.type, rest, before);
                        if (!v) return std::nullopt;
                        items.push_back(member(f, *v));
                        rest.members.clear();
                    }
                    else if (const auto* v = j.find(f.key))
                    {
                        const auto x = initializer(f.type, *v, before);
                        if (!x) return std::nullopt;
                        items.push_back(member(f, *x));
                    }
                }
                // reading drops the other keys of an open struct
                if (!rest.members.empty() && !d.additional) return std::nullopt;
                if (items.size() < d.fields.size()) partial_init_ = true;
                return "{" + join(items) + "}";
            }

            // the alternative of a variant that reading takes for a JSON value: the alternative that the tag names,
            // the only one that reads it in yyjson::one_of, or the first alternative that reads it, when the generator
            // tells that the ones before do not
            std::optional<std::size_t> alternative(const ctype& t, const json& j) const
            {
                if (is_nullable(t)) return j.is_null() ? 0 : 1;
                const auto one = t.combined == combination::one_of;
                auto found = std::optional<std::size_t>();
                for (std::size_t i = 0; i < t.args.size(); ++i)
                {
                    const auto r = fits(j, t.args[i], i < t.sets.size() ? t.sets[i] : settings());
                    if (r == fit::no) continue;
                    if (r == fit::unknown) return std::nullopt;
                    // a tagged variant reads the alternative of the tag, whose `const` excludes the others
                    if (!t.tagged && !one) return i;
                    if (found) return std::nullopt;
                    found = i;
                }
                return found;
            }

            // whether std::initializer_list can copy a value of the type
            bool copyable(const ctype& t, std::vector<int>& seen) const
            {
                if (t.k == tk::pointer) return false;
                if (t.k == tk::named)
                {
                    const auto& d = decls_[t.decl];
                    if (d.kind == decl::kind_t::alias) return copyable(d.target, seen);
                    if (d.kind != decl::kind_t::structure || std::ranges::find(seen, t.decl) != seen.end()) return true;
                    seen.push_back(t.decl);
                    return std::ranges::all_of(d.fields, [&](const field& f) { return copyable(f.type, seen); });
                }
                return std::ranges::all_of(t.args, [&](const ctype& a) { return copyable(a, seen); });
            }

            // the spelling of a C++ type; `expand` spells aliases by their targets, `q` qualifies generated names
            std::string spell(const ctype& t, bool expand = false, std::string_view q = {}) const
            {
                const auto list = [&](std::string_view tmpl, std::size_t from = 0) {
                    auto s = std::string(tmpl) + "<";
                    for (std::size_t i = from; i < t.args.size(); ++i)
                    {
                        if (i > from) s += ", ";
                        s += spell(t.args[i], expand, q);
                    }
                    return s + ">";
                };
                switch (t.k)
                {
                    case tk::any:
                        return "yyjson::value";
                    case tk::object:
                        return "yyjson::object";
                    case tk::null:
                        return "std::nullptr_t";
                    case tk::monostate:
                        return "std::monostate";
                    case tk::boolean:
                        return "bool";
                    case tk::i32:
                        return "std::int32_t";
                    case tk::i64:
                        return "std::int64_t";
                    case tk::u64:
                        return "std::uint64_t";
                    case tk::f32:
                        return "float";
                    case tk::f64:
                        return "double";
                    case tk::string:
                        return "std::string";
                    case tk::named:
                        if (expand && decls_[t.decl].kind == decl::kind_t::alias)
                            return spell(decls_[t.decl].target, true, q);
                        return std::string(q) + decls_[t.decl].name;
                    case tk::vector:
                        return list("std::vector");
                    case tk::map:
                        return "std::map<std::string, " + spell(t.args[0], expand, q) + ">";
                    case tk::tuple:
                        return list("std::tuple");
                    case tk::variant:
                        return t.combined == combination::none ? list("std::variant") : spell_combined(t, expand, q);
                    case tk::optional:
                        return list("std::optional");
                    case tk::pointer:
                        return list("std::unique_ptr");
                }
                return {};
            }

            // yyjson::any_of or yyjson::one_of, whose alternatives with settings or metadata are yyjson::alt
            std::string spell_combined(const ctype& t, bool expand, std::string_view q) const
            {
                auto s = std::string(t.combined == combination::any_of ? "yyjson::any_of<" : "yyjson::one_of<");
                for (std::size_t i = 0; i < t.args.size(); ++i)
                {
                    if (i > 0) s += ", ";
                    path_.push_back(i);
                    auto items = render_settings(t.sets[i], t.args[i]);
                    for (auto& m : render_metadata(t.metas[i])) items.push_back(std::move(m));
                    const auto a = spell(t.args[i], expand, q);
                    if (items.empty())
                        s += a;
                    else
                    {
                        s += "yyjson::alt<" + a;
                        for (const auto& x : items) s += ", " + setting_argument(x, q);
                        s += ">";
                    }
                    path_.pop_back();
                }
                return s + ">";
            }

            // a setting as a template argument of yyjson::alt: before C++26, a setting with a string is a constexpr
            // variable, which the argument points to, so that the text is not a part of the type
            std::string setting_argument(const std::string& text, std::string_view q) const
            {
                if (!naming_ || text.find('"') == std::string::npos) return text;
                auto it = setting_vars_.find(text);
                if (it == setting_vars_.end())
                {
                    const auto open = text.find('(');
                    const auto kind = text.starts_with("yyjson::") && open != std::string::npos
                                          ? text.substr(8, open - 8)
                                          : std::string("setting");
                    auto base = ctx_.empty() ? std::string("alternative") : ctx_;
                    for (const auto i : path_) base += "_" + std::to_string(i);
                    base += "_" + kind;
                    // one underscore between the parts, since a name with "__" is reserved
                    base.erase(std::ranges::unique(base, [](char a, char b) { return a == '_' && b == '_'; }).begin(),
                               base.end());
                    auto name = base;
                    for (auto n = 2; name_taken(name); ++n) name = base + "_" + std::to_string(n);
                    var_defs_.emplace_back(name, text);
                    it = setting_vars_.emplace(text, name).first;
                }
                return "&" + std::string(q) + it->second;
            }

            // whether a name at namespace scope is taken by a type, a function or a variable of the generated code
            bool name_taken(const std::string& name) const
            {
                return type_names_.contains(name) ||
                       std::ranges::any_of(decls_, [&](const decl& e) { return e.name == name; }) ||
                       std::ranges::any_of(default_functions_,
                                           [&](const default_function& g) { return g.name == name; }) ||
                       std::ranges::any_of(var_defs_, [&](const auto& v) { return v.first == name; });
            }

            // whether the alternatives of a yyjson::any_of or yyjson::one_of in `t` share a C++ type, which index()
            // tells apart
            bool shares_type(const ctype& t) const
            {
                if (t.combined != combination::none)
                    for (std::size_t i = 0; i < t.args.size(); ++i)
                        for (auto j = i + 1; j < t.args.size(); ++j)
                            if (spell(t.args[i], true) == spell(t.args[j], true)) return true;
                return std::ranges::any_of(t.args, [&](const ctype& a) { return shares_type(a); });
            }

            // the type behind aliases
            const ctype& resolved(const ctype& t) const
            {
                if (t.k == tk::named && decls_[t.decl].kind == decl::kind_t::alias)
                    return resolved(decls_[t.decl].target);
                return t;
            }

            bool is_struct(const ctype& t) const
            {
                const auto& r = resolved(t);
                return r.k == tk::named && decls_[r.decl].kind == decl::kind_t::structure;
            }

            // an alternative of a variant has no field to carry its metadata, so an unnamed struct made from it takes
            // the metadata of the alternative's schema
            void adopt_metadata(const ctype& t, const json& s)
            {
                if (t.k != tk::named) return;
                auto& d = decls_[t.decl];
                const auto& m = d.meta;
                if (d.kind != decl::kind_t::structure || !d.schema_name.empty() || m.title || m.description ||
                    m.comment || m.deprecated || !m.examples.empty())
                    return;
                d.meta = meta_of(s);
            }

            int rank(const ctype& t) const
            {
                const auto& r = resolved(t);
                switch (r.k)
                {
                    case tk::monostate:
                    case tk::null:
                        return 0;
                    case tk::boolean:
                        return 1;
                    case tk::i32:
                    case tk::i64:
                    case tk::u64:
                        return 2;
                    case tk::f32:
                    case tk::f64:
                        return 3;
                    case tk::named:
                        return decls_[r.decl].kind == decl::kind_t::enumeration ? 4 : 8;
                    case tk::string:
                        return 5;
                    case tk::vector:
                    case tk::tuple:
                        return 6;
                    case tk::map:
                    case tk::object:
                        return 7;
                    case tk::variant:
                        return 9;
                    case tk::optional:
                    case tk::pointer:
                        return rank(r.args[0]);
                    case tk::any:
                        return 10;
                }
                return 10;
            }

            static typed make_nullable(typed r)
            {
                if (r.t.k == tk::any || r.t.k == tk::null) return r;
                if (r.t.k == tk::variant && !r.t.tagged)
                {
                    if (r.t.args[0].k != tk::monostate)
                    {
                        r.t.args.insert(r.t.args.begin(), make(tk::monostate));
                        if (r.t.combined != combination::none)
                        {
                            r.t.sets.insert(r.t.sets.begin(), settings());
                            r.t.metas.insert(r.t.metas.begin(), metadata());
                        }
                    }
                    return {std::move(r.t), {}};
                }
                return {make(tk::variant, {make(tk::monostate), std::move(r.t)}), std::move(r.s)};
            }

            static bool metadata_empty(const metadata& m)
            {
                return !m.title && !m.description && !m.deprecated && m.examples.empty() && m.noted_examples.empty();
            }

            static bool same_metadata(const metadata& a, const metadata& b)
            {
                return a.title == b.title && a.description == b.description && a.deprecated == b.deprecated &&
                       std::ranges::equal(a.examples, b.examples, equal);
            }

            // the bounds of the JSON values that the alternatives of yyjson::any_of and yyjson::one_of read, as
            // `yyjson::detail::bound_of<>()` computes them, by which the library rejects an alternative that is never
            // read: the generator orders the alternatives and drops those that the library rejects

            using json_bound = yyjson::detail::json_bound;

            std::string_view intern(std::string_view v) const { return *interned_.emplace(v).first; }

            // a number of the generated code: a literal of the integer types or a floating literal
            static yyjson::detail::bound_number bound_value(const json& j, tk k)
            {
                if (is_integer_kind(k) && j.is_integral())
                {
                    if (const auto v = as_int64(j)) return yyjson::detail::sint_number(*v);
                    if (const auto v = as_uint64(j)) return yyjson::detail::uint_number(*v);
                }
                return yyjson::detail::real_number(j.num);
            }

            // whether the settings are only `const` and `enum`, and the bounds of numbers if `numbers`, which a bound
            // describes exactly
            static bool described(const settings& s, bool numbers, bool lengths = false)
            {
                return !s.min_items && !s.max_items && !s.unique && s.items.empty() && s.contains.empty() &&
                       s.property_names.empty() && s.additional.empty() && s.prefix.empty() &&
                       std::ranges::all_of(s.constraints, [&](const settings::constraint& c) {
                           const auto& k = c.keyword;
                           return k == "const" || k == "enum" ||
                                  (numbers && (k == "minimum" || k == "maximum" || k == "exclusiveMinimum" ||
                                               k == "exclusiveMaximum")) ||
                                  (lengths && (k == "minLength" || k == "maxLength"));
                       });
            }

            static std::size_t to_count(std::uint64_t n)
            {
                return static_cast<std::size_t>(std::min<std::uint64_t>(n, std::numeric_limits<std::size_t>::max()));
            }

            // the lengths of strings that the settings `s` allow
            static yyjson::detail::count_range lengths_of(const settings& s)
            {
                auto r = yyjson::detail::count_range{};
                for (const auto& c : s.constraints)
                {
                    const auto n = as_uint64(c.value);
                    if (!n) continue;
                    if (c.keyword == "minLength") r.low = std::max(r.low, to_count(*n));
                    if (c.keyword == "maxLength") r.high = std::min(r.high, to_count(*n));
                }
                return r;
            }

            // the values of `const` among those of `enum`, or of `enum`, that the generated code lists for a value
            // of the kind `k`, as `value_constraints()` writes them; std::nullopt when it lists none
            static std::optional<std::vector<const json*>> listed_values(const settings& s, tk k)
            {
                const auto of_kind = [&](const json& v) {
                    if (k == tk::string) return v.is_string();
                    if (k == tk::boolean) return v.is_bool();
                    if (is_float_kind(k)) return v.is_number();
                    if (is_integer_kind(k))
                        return v.is_integral() && (k == tk::u64 ? as_uint64(v).has_value() : as_int64(v).has_value());
                    return false;
                };
                const json* constant = nullptr;
                const json* values = nullptr;
                for (const auto& c : s.constraints)
                    if (c.keyword == "const")
                        constant = &c.value;
                    else if (c.keyword == "enum")
                        values = &c.value;
                if (constant == nullptr && values == nullptr) return std::nullopt;
                auto out = std::vector<const json*>();
                if (values != nullptr && values->is_array())
                    for (const auto& v : values->items)
                        if (of_kind(v)) out.push_back(&v);
                // a constraint of the kind `enum` with one bool is yyjson::const_ of it
                if (constant == nullptr) return out;
                if (values != nullptr && std::ranges::none_of(out, [&](const json* v) { return equal(*v, *constant); }))
                    return std::vector<const json*>();
                return std::vector<const json*>{constant};
            }

            static yyjson::detail::number_bound number_bound_of(tk k, const settings& s)
            {
                namespace d = yyjson::detail;
                const auto integral = is_integer_kind(k);
                auto n = d::all_numbers(integral, d::bound_extent::exact);
                if (k == tk::i32)
                {
                    n.low = d::sint_number(std::numeric_limits<std::int32_t>::min());
                    n.high = d::sint_number(std::numeric_limits<std::int32_t>::max());
                }
                else if (k == tk::i64)
                {
                    n.low = d::sint_number(std::numeric_limits<std::int64_t>::min());
                    n.high = d::sint_number(std::numeric_limits<std::int64_t>::max());
                }
                else if (k == tk::u64)
                {
                    n.low = d::uint_number(0);
                    n.high = d::uint_number(std::numeric_limits<std::uint64_t>::max());
                }
                else if (k == tk::f32)
                {
                    // the doubles that round to a finite float
                    n.low = d::real_number(-340282356779733661637539395458142568448.0);
                    n.high = d::real_number(340282356779733661637539395458142568448.0);
                    if (!s.empty()) n.extent = d::bound_extent::partial;
                    return n;
                }
                auto empty = false;
                const auto bound = [&](const d::bound_number& b, bool up, bool open) {
                    auto& end = up ? n.low : n.high;
                    auto& end_open = up ? n.low_open : n.high_open;
                    if (integral)
                    {
                        const auto step = d::integer_beyond(b, up, open);
                        if (!step.exists)
                            empty = true;
                        else if (up ? d::number_less(end, step.value) : d::number_less(step.value, end))
                            end = step.value;
                    }
                    else
                    {
                        const auto o = d::compare_numbers(b, end);
                        if ((up ? std::is_gt(o) : std::is_lt(o)) || (std::is_eq(o) && open && !end_open))
                        {
                            end = b;
                            end_open = open;
                        }
                    }
                };
                for (const auto& c : s.constraints)
                {
                    if (!c.value.is_number()) continue;
                    if (c.keyword == "minimum") bound(bound_value(c.value, k), true, false);
                    if (c.keyword == "exclusiveMinimum") bound(bound_value(c.value, k), true, true);
                    if (c.keyword == "maximum") bound(bound_value(c.value, k), false, false);
                    if (c.keyword == "exclusiveMaximum") bound(bound_value(c.value, k), false, true);
                }
                const auto order = d::compare_numbers(n.low, n.high);
                if (empty || std::is_gt(order) || (std::is_eq(order) && (n.low_open || n.high_open))) return {};
                if (const auto values = listed_values(s, k))
                {
                    auto listed = n;
                    listed.listed = true;
                    for (const auto* v : *values)
                    {
                        auto p = bound_value(*v, k);
                        if (!integral) p = d::real_number(d::to_double(p));
                        if (!d::has_number(n, p) || d::has_number(listed, p)) continue;
                        if (listed.count == d::bound_capacity)
                            listed.extent = d::bound_extent::partial;
                        else
                            listed.points[listed.count++] = p;
                    }
                    if (listed.count == 0) return {};
                    if (listed.extent == d::bound_extent::exact) return listed;
                    n.extent = d::bound_extent::partial;
                }
                return n;
            }

            // the JSON values that a value of the type `t` with the settings `s` reads, or more
            json_bound bound_of(const ctype& t, const settings& s) const
            {
                namespace d = yyjson::detail;
                using d::bound_extent;
                const auto has = !s.empty();
                const auto bound = [](json_bound b, bool exact) { return exact ? b : d::partial_bound(b); };
                const auto extent = [&](bool exact) { return exact ? bound_extent::exact : bound_extent::partial; };
                auto b = json_bound{};
                switch (t.k)
                {
                    case tk::any:
                        return bound(d::universal_bound(), !has);
                    case tk::object:
                        b.object.extent = extent(!has);
                        return b;
                    case tk::null:
                    case tk::monostate:
                        b.null = extent(!has);
                        return b;
                    case tk::boolean:
                    {
                        b.boolean = extent(described(s, false));
                        b.booleans = 3;
                        if (const auto values = listed_values(s, tk::boolean))
                        {
                            b.booleans = 0;
                            for (const auto* v : *values) b.booleans |= static_cast<std::uint8_t>(v->b ? 2U : 1U);
                        }
                        if (b.booleans == 0) b.boolean = bound_extent::none;
                        return b;
                    }
                    case tk::i32:
                    case tk::i64:
                    case tk::u64:
                    case tk::f32:
                    case tk::f64:
                        b.number = number_bound_of(t.k, s);
                        return bound(b, described(s, true));
                    case tk::string:
                    {
                        b.string.lengths = lengths_of(s);
                        if (b.string.lengths.high < b.string.lengths.low) return b;
                        b.string.extent = bound_extent::exact;
                        if (const auto values = listed_values(s, tk::string))
                        {
                            b.string.listed = true;
                            for (const auto* v : *values)
                                if (d::has_count(b.string.lengths, d::code_points(v->text)))
                                    d::add_string(b.string, intern(v->text));
                            if (b.string.listed && b.string.count == 0) b.string = {};
                        }
                        return bound(b, described(s, false, true));
                    }
                    case tk::named:
                    {
                        const auto& x = decls_[t.decl];
                        if (x.kind == decl::kind_t::alias) return bound_of(x.target, s);
                        if (x.kind == decl::kind_t::enumeration)
                        {
                            b.string.extent = bound_extent::exact;
                            b.string.listed = true;
                            for (const auto& e : x.enumerators) d::add_string(b.string, intern(e.second));
                            return bound(b, !has);
                        }
                        if (x.transparent) return d::partial_bound(bound_of(x.fields[0].type, settings()));
                        b.object.extent = bound_extent::partial;
                        return b;
                    }
                    case tk::vector:
                    {
                        // std::vector reads as many elements as the JSON array has, so the numbers of items bound
                        // the arrays
                        auto others = s;
                        others.min_items.reset();
                        others.max_items.reset();
                        const auto element = bound_of(t.args[0], settings());
                        b.array.extent = extent(others.empty() && d::bound_contains(element, d::universal_bound()));
                        if (s.min_items) b.array.counts.low = to_count(*s.min_items);
                        if (s.max_items) b.array.counts.high = to_count(*s.max_items);
                        if (b.array.counts.high < b.array.counts.low) b.array = {};
                        return b;
                    }
                    case tk::map:
                        b.object.extent = bound_extent::partial;
                        return b;
                    case tk::tuple:
                        return d::kinds_bound(d::json_kind_array | d::json_kind_object);
                    case tk::optional:
                    case tk::pointer:
                        b = bound_of(t.args[0], s);
                        b.null = bound_extent::exact;
                        return b;
                    case tk::variant:
                    {
                        if (t.combined != combination::none)
                        {
                            for (std::size_t i = 0; i < t.args.size(); ++i)
                                b = d::unite_bounds(b, bound_of(t.args[i], i < t.sets.size() ? t.sets[i] : settings()));
                            return bound(b, t.combined == combination::any_of && !has);
                        }
                        if (t.tagged)
                            return d::kinds_bound(t.args[0].k == tk::monostate ? d::json_kind_object | d::json_kind_null
                                                                               : d::json_kind_object);
                        if (is_nullable(t))
                        {
                            b = bound_of(t.args[1], s);
                            b.null = bound_extent::exact;
                            return b;
                        }
                        for (const auto& a : t.args) b = d::unite_bounds(b, bound_of(a, settings()));
                        return bound(b, !has);
                    }
                }
                return d::universal_bound();
            }

            static bool reads_nothing(const json_bound& b)
            {
                using yyjson::detail::bound_extent;
                return b.null == bound_extent::none && b.boolean == bound_extent::none &&
                       b.number.extent == bound_extent::none && b.string.extent == bound_extent::none &&
                       b.array.extent == bound_extent::none && b.object.extent == bound_extent::none;
            }

            struct alternative_entry
            {
                ctype t;
                settings s;
                metadata m;
            };

            // a variant of the alternatives: std::variant (`mode` none), yyjson::any_of or yyjson::one_of, with the
            // metadata of each alternative in `metas`. std::variant becomes yyjson::any_of when an alternative has
            // settings or metadata, which only yyjson::alt carries
            typed make_variant(std::vector<typed> alts, const std::string& loc, combination mode = combination::none,
                               std::vector<metadata> metas = {})
            {
                namespace d = yyjson::detail;
                metas.resize(alts.size());
                auto flat = std::vector<alternative_entry>();
                const auto push = [&](ctype t, settings s, metadata m) {
                    if (t.k == tk::null) t = make(tk::monostate);
                    flat.push_back({std::move(t), std::move(s), std::move(m)});
                };
                const auto disjoint = [&](const ctype& v) {
                    for (std::size_t i = 0; i < v.args.size(); ++i)
                        for (auto j = i + 1; j < v.args.size(); ++j)
                            if (!d::bounds_disjoint(bound_of(v.args[i], settings()), bound_of(v.args[j], settings())))
                                return false;
                    return true;
                };
                for (std::size_t n = 0; n < alts.size(); ++n)
                {
                    auto& a = alts[n];
                    auto& m = metas[n];
                    if (a.t.k == tk::variant && is_nullable(a.t))
                    {
                        push(make(tk::monostate), {}, {});
                        push(std::move(a.t.args[1]), std::move(a.s), std::move(m));
                    }
                    else if (a.t.k == tk::variant && !a.t.tagged && a.t.combined == combination::none &&
                             metadata_empty(m) && (mode != combination::one_of || disjoint(a.t)))
                    {
                        // the values of each alternative are those of the variant, which no other alternative reads
                        // in yyjson::one_of
                        for (auto& x : a.t.args) push(std::move(x), {}, {});
                    }
                    else if (a.t.k == tk::variant && a.t.combined == combination::any_of && mode != combination::one_of &&
                             metadata_empty(m))
                    {
                        for (std::size_t i = 0; i < a.t.args.size(); ++i)
                            push(std::move(a.t.args[i]), std::move(a.t.sets[i]), std::move(a.t.metas[i]));
                    }
                    else
                        push(std::move(a.t), std::move(a.s), std::move(m));
                }
                if (std::ranges::any_of(flat, [](const auto& a) { return a.t.k == tk::any; })) return {};

                // the same type with the same settings: the first one, with its metadata
                const auto key = [&](const alternative_entry& a) {
                    return spell(a.t, true) + "\n" + join(render_settings(a.s, a.t));
                };
                auto uniq = std::vector<alternative_entry>();
                for (auto& a : flat)
                {
                    const auto k = key(a);
                    if (std::ranges::none_of(uniq, [&](const auto& u) { return key(u) == k; }))
                        uniq.push_back(std::move(a));
                    else if (mode == combination::one_of && a.t.k != tk::monostate)
                        warn(loc, "an alternative of `oneOf` is the same as another one, which never reads a value "
                                  "alone; it is dropped");
                }
                std::ranges::stable_sort(uniq, {}, [&](const auto& u) { return rank(u.t); });

                auto bounds = std::vector<json_bound>();
                for (const auto& u : uniq) bounds.push_back(bound_of(u.t, u.s));
                for (std::size_t i = 0; i < uniq.size();)
                    if (reads_nothing(bounds[i]))
                    {
                        warn(loc, "the settings of an alternative leave no value to read; it is dropped");
                        uniq.erase(uniq.begin() + static_cast<std::ptrdiff_t>(i));
                        bounds.erase(bounds.begin() + static_cast<std::ptrdiff_t>(i));
                    }
                    else
                        ++i;

                const auto combined = mode != combination::none ||
                                      std::ranges::any_of(uniq, [](const auto& u) {
                                          return !u.s.empty() || !metadata_empty(u.m);
                                      });
                const auto kind = mode != combination::none ? mode : combined ? combination::any_of : combination::none;
                auto order = std::vector<std::size_t>();
                const auto drop = [&](std::size_t p, const char* why) {
                    warn(loc, why);
                    order.erase(order.begin() + static_cast<std::ptrdiff_t>(p));
                };
                if (kind != combination::one_of)
                {
                    // an alternative goes before the first one that reads all its values; the same values twice are
                    // read by the first one only
                    for (std::size_t j = 0; j < uniq.size(); ++j)
                    {
                        auto pos = order.size();
                        auto same = false;
                        for (std::size_t p = 0; p < order.size(); ++p)
                            if (d::bound_contains(bounds[order[p]], bounds[j]))
                            {
                                same = d::bound_contains(bounds[j], bounds[order[p]]);
                                pos = p;
                                break;
                            }
                        if (!same) order.insert(order.begin() + static_cast<std::ptrdiff_t>(pos), j);
                    }
                    // yyjson::any_of rejects an alternative that is never read; std::variant reads it never
                    for (std::size_t q = 1; kind == combination::any_of && q < order.size();)
                    {
                        const auto never = std::ranges::any_of(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(q),
                                                               [&](std::size_t p) {
                                                                   return d::bound_contains(bounds[p], bounds[order[q]]);
                                                               });
                        if (never)
                            drop(q, "an alternative of `anyOf` reads only values that an alternative before it reads; "
                                    "it is dropped");
                        else
                            ++q;
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < uniq.size(); ++i) order.push_back(i);
                    // yyjson::one_of rejects an alternative of which another one reads every value, in the order of
                    // the library
                    for (auto found = true; found;)
                    {
                        found = false;
                        for (std::size_t j = 1; j < order.size() && !found; ++j)
                            for (std::size_t i = 0; i < j && !found; ++i)
                            {
                                const auto contained = d::bound_contains(bounds[order[i]], bounds[order[j]]) ? j
                                                       : d::bound_contains(bounds[order[j]], bounds[order[i]])
                                                           ? i
                                                           : order.size();
                                if (contained == order.size()) continue;
                                drop(contained, "an alternative of `oneOf` reads only values that another alternative "
                                                "reads, so it never reads a value alone; it is dropped");
                                found = true;
                            }
                    }
                }

                auto out = std::vector<alternative_entry>();
                for (const auto i : order) out.push_back(std::move(uniq[i]));
                const auto mono = !out.empty() && out[0].t.k == tk::monostate;
                if (out.empty()) return {};
                if (out.size() == 1) return mono ? typed{make(tk::null), {}} : typed{std::move(out[0].t), std::move(out[0].s)};
                if (out.size() == 2 && mono) return make_nullable({std::move(out[1].t), std::move(out[1].s)});
                auto r = make(tk::variant);
                r.combined = kind;
                for (auto& a : out)
                {
                    r.args.push_back(std::move(a.t));
                    if (kind == combination::none) continue;
                    if (a.t.k == tk::monostate) a.m = metadata();
                    a.m.comment.reset();
                    r.sets.push_back(std::move(a.s));
                    r.metas.push_back(std::move(a.m));
                }
                return {std::move(r), {}};
            }

            typed type_const(const json& s, const std::string& loc)
            {
                const auto& c = *s.find("const");
                auto r = typed();
                switch (c.k)
                {
                    case json::kind::null:
                        return {make(tk::null), {}};
                    case json::kind::boolean:
                        r.t = make(tk::boolean);
                        r.s.constraints.push_back(
                            {std::string("yyjson::const_(") + (c.b ? "true" : "false") + ")", "const", c});
                        return r;
                    case json::kind::number:
                        if (const auto v = as_int64(c); v && c.is_integral())
                        {
                            r.t = make(tk::i64);
                            r.s.constraints.push_back({"yyjson::const_(" + int_literal(*v) + ")", "const", c});
                        }
                        else if (const auto u = as_uint64(c); u && c.is_integral())
                        {
                            r.t = make(tk::u64);
                            r.s.constraints.push_back({"yyjson::const_(" + uint_literal(*u) + ")", "const", c});
                        }
                        else
                        {
                            r.t = make(tk::f64);
                            r.s.constraints.push_back({"yyjson::const_(" + float_literal(c) + ")", "const", c});
                        }
                        return r;
                    case json::kind::string:
                        r.t = make(tk::string);
                        r.s.constraints.push_back({"yyjson::const_(" + cpp_string(c.text) + ")", "const", c});
                        return r;
                    default:
                        warn(loc,
                             "`const` of an object or an array is not supported; the value is read as any JSON value");
                        return {};
                }
            }

            typed make_enum(const json* s0, const json& s, const std::vector<std::string>& values,
                            const std::string& loc, const std::string& hint, bool named)
            {
                auto d =
                    decl{.kind = decl::kind_t::enumeration, .name = named ? hint : unique_type_name(hint), .loc = loc};
                if (named)
                {
                    d.meta = meta_of(s);
                    if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                }
                auto members = std::set<std::string>();
                for (const auto& v : values)
                {
                    if (std::ranges::any_of(d.enumerators, [&](const auto& e) { return e.second == v; })) continue;
                    const auto base = sanitize(v);
                    auto m = base;
                    for (auto n = 2; members.contains(m); ++n) m = base + "_" + std::to_string(n);
                    members.insert(m);
                    d.enumerators.emplace_back(m, v);
                }
                return {named_type(add_decl(std::move(d))), {}};
            }

            typed type_enum(const json* s0, const json& s, const std::string& loc, const std::string& hint, bool named)
            {
                const auto& e = *s.find("enum");
                if (!e.is_array() || e.items.empty())
                {
                    warn(loc, "`enum` is not an array of values; the value is read as any JSON value");
                    return {};
                }
                const auto types = type_list(s);
                const auto allowed = [&](const json& v) {
                    if (types.empty()) return true;
                    const auto has = [&](std::string_view t) { return std::ranges::find(types, t) != types.end(); };
                    switch (v.k)
                    {
                        case json::kind::null:
                            return has("null");
                        case json::kind::boolean:
                            return has("boolean");
                        case json::kind::number:
                            return has("number") || (has("integer") && v.is_integral());
                        case json::kind::string:
                            return has("string");
                        case json::kind::array:
                            return has("array");
                        case json::kind::object:
                            return has("object");
                    }
                    return false;
                };
                auto nullable = false;
                auto strings = std::vector<std::string>();
                auto numbers = std::vector<const json*>();
                auto bools = std::vector<bool>();
                for (const auto& v : e.items)
                {
                    if (!allowed(v)) continue;
                    if (v.is_null())
                        nullable = true;
                    else if (v.is_string())
                        strings.push_back(v.text);
                    else if (v.is_number())
                        numbers.push_back(&v);
                    else if (v.is_bool())
                    {
                        if (std::ranges::find(bools, v.b) == bools.end()) bools.push_back(v.b);
                    }
                    else
                    {
                        warn(loc,
                             "`enum` with an object or an array is not supported; the value is read as any JSON value");
                        return {};
                    }
                }
                auto alts = std::vector<typed>();
                const auto groups = !strings.empty() + !numbers.empty() + !bools.empty();
                if (groups == 0) return nullable ? typed{make(tk::null), {}} : typed{};
                if (!bools.empty())
                {
                    auto r = typed{make(tk::boolean), {}};
                    if (bools.size() == 1)
                        r.s.constraints.push_back(
                            {std::string("yyjson::const_(") + (bools[0] ? "true" : "false") + ")", "enum", e});
                    alts.push_back(std::move(r));
                }
                if (!numbers.empty())
                {
                    const auto all_int =
                        std::ranges::all_of(numbers, [](const json* v) { return v->is_integral() && as_int64(*v); });
                    const auto all_uint =
                        std::ranges::all_of(numbers, [](const json* v) { return v->is_integral() && as_uint64(*v); });
                    auto r = typed{make(all_int ? tk::i64 : all_uint ? tk::u64 : tk::f64), {}};
                    auto c = std::string("yyjson::enum_(");
                    for (auto first = true; const auto* v : numbers)
                    {
                        if (!std::exchange(first, false)) c += ", ";
                        c += all_int    ? int_literal(*as_int64(*v))
                             : all_uint ? uint_literal(*as_uint64(*v))
                                        : float_literal(*v);
                    }
                    r.s.constraints.push_back({c + ")", "enum", e});
                    alts.push_back(std::move(r));
                }
                if (std::ranges::find(strings, std::string()) != strings.end())
                {
                    // yyjson::rename cannot give an enumerator the empty name
                    warn(loc, "`enum` has the empty string; the value is std::string with yyjson::enum_");
                    auto r = typed{make(tk::string), {}};
                    auto c = std::string("yyjson::enum_(");
                    for (auto seen = std::set<std::string>(); const auto& v : strings)
                        if (seen.insert(v).second) c += (seen.size() > 1 ? ", " : "") + cpp_string(v);
                    r.s.constraints.push_back({c + ")", "enum", e});
                    alts.push_back(std::move(r));
                }
                else if (!strings.empty())
                    alts.push_back(make_enum(s0, s, strings, loc, hint, named && groups == 1));
                if (alts.size() == 1) return nullable ? make_nullable(std::move(alts[0])) : std::move(alts[0]);
                if (nullable) alts.push_back({make(tk::monostate), {}});
                return make_variant(std::move(alts), loc);
            }

            // the value of a property that tells an alternative: `const` or an `enum` of one string
            const json* tag_value(const json& alt, std::string_view key, const std::string& loc)
            {
                const auto* props = alt.find("properties");
                const auto* p = props != nullptr ? props->find(key) : nullptr;
                if (p == nullptr) return nullptr;
                const auto* n = normalize_full(p, loc);
                // a reference that normalizing keeps, beside metadata or a `const` that it fixes
                if (is_pure_ref(*n)) n = normalize_full(n, loc);
                if (!n->is_object()) return nullptr;
                if (const auto* c = n->find("const"); c != nullptr && c->is_string()) return c;
                if (const auto* e = n->find("enum");
                    e != nullptr && e->is_array() && e->items.size() == 1 && e->items[0].is_string())
                    return &e->items[0];
                return nullptr;
            }

            static bool is_null_schema(const json& s)
            {
                if (!s.is_object()) return false;
                if (const auto* c = s.find("const"); c != nullptr && c->is_null()) return true;
                const auto t = type_list(s);
                return t.size() == 1 && t[0] == "null";
            }

            // a schema that becomes a struct
            static bool is_struct_schema(const json& s)
            {
                if (!s.has("properties") && !s.has("required")) return false;
                auto t = type_list(s);
                std::erase(t, "null");
                return t.empty() ? !s.has("type") : t.size() == 1 && t[0] == "object";
            }

            // the keys of {"required": [...]}, without other keywords than metadata; nullopt for another schema.
            // `properties` may repeat properties of the struct schema `parent` with the same schemas.
            static std::optional<std::vector<std::string>> only_required(const json& s, const json* parent = nullptr)
            {
                if (!s.is_object()) return std::nullopt;
                const auto* req = s.find("required");
                if (req == nullptr || !req->is_array() || req->items.empty()) return std::nullopt;
                const auto* own = parent != nullptr ? parent->find("properties") : nullptr;
                const auto repeated = [&](const json& props) {
                    return own != nullptr && own->is_object() && props.is_object() &&
                           std::ranges::all_of(props.members, [&](const auto& m) {
                               const auto* p = own->find(m.first);
                               return p != nullptr && equal(*p, m.second);
                           });
                };
                if (std::ranges::any_of(s.members, [&](const auto& m) {
                        return m.first != "required" && m.first != "not" && !is_metadata_key(m.first) &&
                               !(m.first == "properties" && repeated(m.second));
                    }))
                    return std::nullopt;
                auto keys = std::vector<std::string>();
                for (const auto& e : req->items)
                {
                    if (!e.is_string() || std::ranges::find(keys, e.text) != keys.end()) return std::nullopt;
                    keys.push_back(e.text);
                }
                return keys;
            }

            // `oneOf` or `anyOf` of a struct schema whose subschemas only require keys, which
            // yyjson::one_of_required or yyjson::any_of_required checks on one struct: [{"required": ["a"]},
            // {"required": ["b"]}], or the exclusive form, in which each subschema has also `not` of the other groups:
            // {"required": ["a"], "not": {"required": ["b"]}}, or "not": {"anyOf": [{"required": [...]}, ...]} for
            // more groups; that is exactly one group under `anyOf` too
            static std::optional<required_groups> required_groups_of(const json& s, std::string_view kw)
            {
                if (!is_struct_schema(s)) return std::nullopt;
                const auto* list = s.find(kw);
                if (list == nullptr || !list->is_array() || list->items.empty()) return std::nullopt;
                using group = std::vector<std::string>;
                auto r = required_groups{.exactly_one = kw == "oneOf"};
                auto nots = std::vector<std::optional<std::vector<group>>>();
                for (const auto& b : list->items)
                {
                    auto g = only_required(b, &s);
                    if (!g) return std::nullopt;
                    r.groups.push_back(std::move(*g));
                    const auto* n = b.find("not");
                    if (n == nullptr)
                    {
                        nots.emplace_back();
                        continue;
                    }
                    auto excluded = std::vector<group>();
                    if (auto x = n->has("not") ? std::nullopt : only_required(*n))
                        excluded.push_back(std::move(*x));
                    else if (const auto* a = n->is_object() ? n->find("anyOf") : nullptr;
                             a != nullptr && a->is_array() && std::ranges::all_of(n->members, [](const auto& m) {
                                 return m.first == "anyOf" || is_metadata_key(m.first);
                             }))
                    {
                        for (const auto& e : a->items)
                        {
                            auto y = e.has("not") ? std::nullopt : only_required(e);
                            if (!y) return std::nullopt;
                            excluded.push_back(std::move(*y));
                        }
                    }
                    else
                        return std::nullopt;
                    nots.emplace_back(std::move(excluded));
                }
                if (std::ranges::none_of(nots, [](const auto& n) { return n.has_value(); })) return r;
                // each subschema excludes exactly the other groups
                const auto sorted = [](std::vector<group> gs) {
                    for (auto& g : gs) std::ranges::sort(g);
                    std::ranges::sort(gs);
                    return gs;
                };
                for (std::size_t i = 0; i < r.groups.size(); ++i)
                {
                    if (!nots[i]) return std::nullopt;
                    auto others = std::vector<group>();
                    for (std::size_t j = 0; j < r.groups.size(); ++j)
                        if (j != i) others.push_back(r.groups[j]);
                    if (sorted(*nots[i]) != sorted(std::move(others))) return std::nullopt;
                }
                r.exactly_one = true;
                return r;
            }

            typed type_variant(const json* s0, const json* s, const std::string& loc, const std::string& hint,
                               bool named)
            {
                // a keyword of required groups stays in the schema of each alternative
                const auto* any = required_groups_of(*s, "anyOf") ? nullptr : s->find("anyOf");
                const auto* one = required_groups_of(*s, "oneOf") ? nullptr : s->find("oneOf");
                if (any != nullptr && one != nullptr)
                    warn(loc, "`anyOf` and `oneOf` together are not supported; `oneOf` is ignored");
                const auto* list = any != nullptr ? any : one;
                const auto kw = std::string(any != nullptr ? "anyOf" : "oneOf");
                if (!list->is_array() || list->items.empty())
                {
                    warn(loc, "`" + kw + "` is not a non-empty array; the value is read as any JSON value");
                    return {};
                }
                auto base = json::object_of();
                for (const auto& [k, v] : s->members)
                    if (!(k == "anyOf" && any != nullptr) && !(k == "oneOf" && one != nullptr) && !is_metadata_key(k) &&
                        !is_ignored_key(k))
                        base.members.emplace_back(k, v);
                auto schemas = std::vector<const json*>();
                auto locs = std::vector<std::string>();
                for (std::size_t i = 0; i < list->items.size(); ++i)
                {
                    locs.push_back(loc + "/" + kw + "/" + std::to_string(i));
                    if (base.members.empty() || ref_within_base(base, list->items[i], locs.back()))
                        schemas.push_back(&list->items[i]);
                    else
                    {
                        auto c = base;
                        c.set("allOf", json::array_of({list->items[i]}));
                        schemas.push_back(store(std::move(c)));
                    }
                }
                if (auto r = string_choices(s0, *s, base, *list, kw, loc, hint, named)) return *r;
                if (auto r = tagged_variant(schemas, locs, loc)) return *r;
                auto alts = std::vector<typed>();
                auto metas = std::vector<metadata>();
                for (std::size_t i = 0; i < schemas.size(); ++i)
                {
                    const auto* n = normalize(schemas[i], locs[i]);
                    const auto* title = n->is_object() ? n->find("title") : nullptr;
                    const auto h = title != nullptr && title->is_string() && !pascal(title->text).empty()
                                       ? pascal(title->text)
                                       : hint + "Option" + std::to_string(i + 1);
                    alts.push_back(type_of(schemas[i], locs[i], h));
                    adopt_metadata(alts.back().t, *n);
                    // the metadata of the alternative, unless its named type has the same
                    auto m = meta_of(*n);
                    m.comment.reset();
                    const auto& t = alts.back().t;
                    if (t.k == tk::named && same_metadata(decls_[t.decl].meta, m)) m = metadata();
                    metas.push_back(std::move(m));
                }
                return make_variant(std::move(alts), loc, any != nullptr ? combination::any_of : combination::one_of,
                                    std::move(metas));
            }

            // `anyOf` / `oneOf` whose subschemas are only strings by `const` or `enum` (and null) become one enum class,
            // with the metadata of each subschema as comments of its enumerators; a reference stays an alternative
            std::optional<typed> string_choices(const json* s0, const json& s, const json& base, const json& list,
                                                const std::string& kw, const std::string& loc, const std::string& hint,
                                                bool named)
            {
                auto base_null = true;
                if (!base.members.empty())
                {
                    const auto t = type_list(base);
                    if (base.members.size() != 1 || base.members[0].first != "type" || t.empty() ||
                        std::ranges::any_of(t, [](const auto& x) { return x != "string" && x != "null"; }))
                        return std::nullopt;
                    base_null = std::ranges::find(t, "null") != t.end();
                }
                auto values = std::vector<std::string>();
                auto metas = std::map<std::string, metadata>();
                auto nullable = false;
                auto duplicate = false;
                for (const auto& b : list.items)
                {
                    if (!b.is_object() ||
                        std::ranges::any_of(b.members, [](const auto& m) {
                            return m.first != "const" && m.first != "enum" && m.first != "type" &&
                                   !is_metadata_key(m.first);
                        }))
                        return std::nullopt;
                    const auto t = type_list(b);
                    const auto* c = b.find("const");
                    const auto* e = b.find("enum");
                    if (c == nullptr && e == nullptr)
                    {
                        if (t.size() != 1 || t[0] != "null" || !base_null) return std::nullopt;
                        nullable = true;
                        continue;
                    }
                    if ((c != nullptr && e != nullptr) || (!t.empty() && (t.size() != 1 || t[0] != "string")))
                        return std::nullopt;
                    if (c != nullptr && c->is_null() && t.empty() && base_null)
                    {
                        nullable = true;
                        continue;
                    }
                    const auto own = c != nullptr ? std::vector<json>{*c} : e->is_array() ? e->items : std::vector<json>();
                    if (own.empty() || std::ranges::any_of(own, [](const json& v) { return !v.is_string(); }))
                        return std::nullopt;
                    auto m = meta_of(b);
                    if (!m.description) m.description = m.title;
                    m.title.reset();
                    m.examples.clear();
                    m.deprecated = false;
                    for (const auto& v : own)
                    {
                        if (std::ranges::find(values, v.text) != values.end())
                        {
                            duplicate = true;
                            continue;
                        }
                        values.push_back(v.text);
                        if (!metadata_empty(m)) metas.emplace(v.text, m);
                    }
                }
                if (values.empty()) return std::nullopt;
                if (duplicate && kw == "oneOf")
                    warn(loc, "a string of `oneOf` is in more than one subschema, which never reads it alone; the "
                              "string is read as one enumerator");
                auto e = json::object_of();
                for (const auto& [k, v] : s.members)
                    if (k == "title" || k == "description" || k == "$comment" || k == "deprecated" || k == "examples")
                        e.members.emplace_back(k, v);
                auto items = std::vector<json>();
                for (const auto& v : values) items.push_back(json::string_of(v));
                if (nullable) items.emplace_back();
                e.set("enum", json::array_of(std::move(items)));
                const auto before = decls_.size();
                auto r = type_enum(s0, *store(std::move(e)), loc, hint, named);
                for (auto i = before; i < decls_.size(); ++i)
                    if (decls_[i].kind == decl::kind_t::enumeration) decls_[i].enumerator_meta = metas;
                return r;
            }

            // whether an alternative that only refers to a schema already meets the `type` that is the only keyword
            // beside `anyOf` / `oneOf`; it then keeps its reference, since merging the type into a copy of the referred
            // schema would expand a recursive reference without end:
            // `{"type": "object", "anyOf": [{...}, {"$ref": "#"}]}`
            bool ref_within_base(const json& base, const json& alt, const std::string& loc)
            {
                if (!is_pure_ref(alt) || base.members.size() != 1 || base.members[0].first != "type") return false;
                const auto* t = &alt;
                for (auto n = 0; t != nullptr && is_pure_ref(*t) && n < 64; ++n) t = resolve(*t->find("$ref"), loc);
                if (t == nullptr || !t->is_object()) return false;
                const auto have = type_list(*t);
                const auto allowed = type_list(base);
                return !have.empty() && std::ranges::all_of(have, [&](const auto& x) {
                    return std::ranges::find(allowed, x) != allowed.end() ||
                           (x == "integer" && std::ranges::find(allowed, "number") != allowed.end());
                });
            }

            std::optional<typed> tagged_variant(const std::vector<const json*>& schemas,
                                                const std::vector<std::string>& locs, const std::string& loc)
            {
                auto objs = std::vector<std::size_t>();
                auto nulls = std::size_t();
                auto norm = std::vector<const json*>();
                for (std::size_t i = 0; i < schemas.size(); ++i)
                {
                    const auto* n = normalize_full(schemas[i], locs[i]);
                    norm.push_back(n);
                    if (is_null_schema(*n))
                        ++nulls;
                    else if (n->is_object() && n->has("properties") &&
                             (!n->has("anyOf") || required_groups_of(*n, "anyOf")) &&
                             (!n->has("oneOf") || required_groups_of(*n, "oneOf")))
                    {
                        const auto t = type_list(*n);
                        if (t.empty() || (t.size() == 1 && t[0] == "object")) objs.push_back(i);
                    }
                }
                if (objs.size() < 2 || objs.size() + nulls != schemas.size()) return std::nullopt;
                const auto required = [](const json& s, std::string_view k) {
                    const auto* req = s.find("required");
                    return req != nullptr &&
                           std::ranges::any_of(req->items, [&](const auto& e) { return e.is_string() && e.text == k; });
                };
                const auto* first = norm[objs[0]]->find("properties");
                for (const auto& [key, unused] : first->members)
                {
                    auto ids = std::vector<std::string>();
                    for (const auto i : objs)
                    {
                        const auto* v = required(*norm[i], key) ? tag_value(*norm[i], key, locs[i]) : nullptr;
                        if (v == nullptr || std::ranges::find(ids, v->text) != ids.end()) break;
                        ids.push_back(v->text);
                    }
                    if (ids.size() != objs.size()) continue;
                    auto args = std::vector<ctype>();
                    if (nulls > 0) args.push_back(make(tk::monostate));
                    for (std::size_t j = 0; j < objs.size(); ++j)
                    {
                        const auto i = objs[j];
                        auto t = type_of(schemas[i], locs[i], pascal(ids[j]).empty() ? "Alternative" : pascal(ids[j]));
                        if (!is_struct(t.t)) return std::nullopt;
                        adopt_metadata(t.t, *norm[i]);
                        if (std::ranges::any_of(args,
                                                [&](const auto& a) { return spell(a, true) == spell(t.t, true); }))
                            return std::nullopt;
                        args.push_back(std::move(t.t));
                    }
                    auto v = ctype{.k = tk::variant, .tagged = true, .args = std::move(args)};
                    notes_.push_back({v, key, std::move(ids), -1, loc});
                    return typed{std::move(v), {}};
                }
                return std::nullopt;
            }

            static bool is_integer_kind(tk k) { return k == tk::i32 || k == tk::i64 || k == tk::u64; }
            static bool is_float_kind(tk k) { return k == tk::f32 || k == tk::f64; }

            // a bound of a number field, or nothing when the bound does not limit the type
            static std::optional<std::string> bound_literal(const json& j, tk k, bool lower)
            {
                if (!j.is_number()) return std::nullopt;
                if (is_float_kind(k)) return float_literal(j);
                const auto lo = k == tk::i32 ? -0x1p31 : k == tk::u64 ? 0.0 : -0x1p63;
                const auto hi = k == tk::i32 ? 0x1p31 : k == tk::u64 ? 0x1p64 : 0x1p63;
                if (lower ? j.num < lo : j.num >= hi) return std::nullopt;
                if (j.is_integral())
                {
                    if (const auto v = as_int64(j)) return int_literal(*v);
                    if (const auto v = as_uint64(j)) return uint_literal(*v);
                }
                return float_literal(j);
            }

            void scalar_constraints(const json& s, tk k, settings& out, const std::string& loc)
            {
                const auto count = [&](const char* key) -> std::optional<std::uint64_t> {
                    const auto* v = s.find(key);
                    if (v == nullptr) return std::nullopt;
                    if (const auto n = as_uint64(*v)) return n;
                    warn(loc, std::string("`") + key + "` is not a non-negative integer and is ignored");
                    return std::nullopt;
                };
                if (k == tk::string)
                {
                    if (const auto n = count("minLength"); n && *n > 0)
                        out.constraints.push_back(
                            {"yyjson::min_length(" + std::to_string(*n) + ")", "minLength", *s.find("minLength")});
                    if (const auto n = count("maxLength"))
                        out.constraints.push_back(
                            {"yyjson::max_length(" + std::to_string(*n) + ")", "maxLength", *s.find("maxLength")});
                    if (const auto* p = s.find("pattern"); p != nullptr && p->is_string())
                    {
                        if (opts_.pattern)
                        {
                            out.constraints.push_back({"yyjson::pattern(" + cpp_string(p->text) + ")", "pattern", *p});
                            uses_pattern_ = true;
                        }
                        else
                            warn(loc, "`pattern` is dropped (--no-pattern)");
                    }
                    if (const auto* f = s.find("format"); f != nullptr && f->is_string())
                    {
                        static const auto supported =
                            std::set<std::string_view>{"date-time",    "date",
                                                       "time",         "duration",
                                                       "email",        "hostname",
                                                       "ipv4",         "ipv6",
                                                       "uri",          "uri-reference",
                                                       "uuid",         "uri-template",
                                                       "json-pointer", "relative-json-pointer"};
                        static const auto unsupported =
                            std::set<std::string_view>{"idn-email", "idn-hostname", "iri", "iri-reference", "regex"};
                        if (supported.contains(f->text))
                            out.constraints.push_back({"yyjson::format(" + cpp_string(f->text) + ")", "format", *f});
                        else if (unsupported.contains(f->text))
                            warn(loc, "the format \"" + f->text + "\" is not supported and is ignored");
                    }
                }
                else if (is_integer_kind(k) || is_float_kind(k))
                {
                    for (const auto& [key, lower, name] :
                         {std::tuple{"minimum", true, "minimum"}, std::tuple{"maximum", false, "maximum"},
                          std::tuple{"exclusiveMinimum", true, "exclusive_minimum"},
                          std::tuple{"exclusiveMaximum", false, "exclusive_maximum"}})
                        if (const auto* v = s.find(key))
                        {
                            if (!v->is_number())
                                warn(loc, std::string("`") + key + "` is not a number and is ignored");
                            else if (const auto b = bound_literal(*v, k, lower))
                                out.constraints.push_back({std::string("yyjson::") + name + "(" + *b + ")", key, *v});
                        }
                    if (const auto* v = s.find("multipleOf"))
                    {
                        if (!v->is_number() || v->num <= 0)
                            warn(loc, "`multipleOf` is not a positive number and is ignored");
                        else if (!(is_integer_kind(k) && v->is_integral() && v->num == 1))
                        {
                            const auto lit = is_integer_kind(k) && v->is_integral() && as_int64(*v)
                                                 ? int_literal(*as_int64(*v))
                                                 : float_literal(*v);
                            out.constraints.push_back({"yyjson::multiple_of(" + lit + ")", "multipleOf", *v});
                        }
                    }
                }
            }

            // the constraints of `const` / `enum` on a value of kind `k`, or nothing when they do not apply
            std::optional<std::vector<settings::constraint>> value_constraints(const json& s, tk k)
            {
                auto out = std::vector<settings::constraint>();
                const auto lit = [&](const json& v) -> std::optional<std::string> {
                    if (k == tk::string && v.is_string()) return cpp_string(v.text);
                    if (k == tk::boolean && v.is_bool()) return v.b ? "true" : "false";
                    if (is_float_kind(k) && v.is_number()) return float_literal(v);
                    if (is_integer_kind(k) && v.is_integral())
                    {
                        if (const auto i = as_int64(v); i && k != tk::u64) return int_literal(*i);
                        if (const auto u = as_uint64(v); u && k == tk::u64) return uint_literal(*u);
                    }
                    return std::nullopt;
                };
                if (const auto* c = s.find("const"))
                {
                    const auto l = lit(*c);
                    if (!l) return std::nullopt;
                    out.push_back({"yyjson::const_(" + *l + ")", "const", *c});
                }
                if (const auto* e = s.find("enum"))
                {
                    if (!e->is_array()) return std::nullopt;
                    auto c = std::string("yyjson::enum_(");
                    auto n = 0;
                    for (const auto& v : e->items)
                    {
                        const auto l = lit(v);
                        if (!l) continue;  // a value of another type: never read into this kind
                        c += (n++ > 0 ? ", " : "") + *l;
                    }
                    if (n == 0) return std::nullopt;
                    out.push_back({c + ")", "enum", *e});
                }
                return out;
            }

            // the type of a value: behind aliases, std::optional, std::unique_ptr and nullable variants
            const ctype& value_type(const ctype& t) const
            {
                const auto& r = resolved(t);
                if (r.k == tk::optional || r.k == tk::pointer) return value_type(r.args[0]);
                if (is_nullable(r)) return value_type(r.args[1]);
                return r;
            }

            // the settings of an element level from a schema, or nothing when they cannot be expressed
            std::optional<settings> settings_for(const json& schema, const ctype& target, const std::string& loc)
            {
                const auto* n = normalize_full(&schema, loc);
                if (n->is_bool()) return n->b ? std::optional<settings>(settings()) : std::nullopt;
                if (!n->is_object()) return std::nullopt;
                for (const auto* k : {"anyOf", "oneOf", "$ref", "not", "if"})
                    if (n->has(k)) return std::nullopt;
                const auto& t = value_type(target);
                const auto is_enum = t.k == tk::named && decls_[t.decl].kind == decl::kind_t::enumeration;
                const auto is_obj = t.k == tk::map || t.k == tk::object || is_struct(t);
                if (const auto types = type_list(*n); !types.empty())
                {
                    const auto has = [&](std::string_view x) { return std::ranges::find(types, x) != types.end(); };
                    const auto covered = t.k == tk::string || is_enum              ? has("string")
                                         : is_integer_kind(t.k)                    ? has("integer") || has("number")
                                         : is_float_kind(t.k)                      ? has("number")
                                         : t.k == tk::boolean                      ? has("boolean")
                                         : t.k == tk::vector || t.k == tk::tuple   ? has("array")
                                         : is_obj                                  ? has("object")
                                         : t.k == tk::null || t.k == tk::monostate ? has("null")
                                                                                   : types.size() >= 6;
                    if (!covered) return std::nullopt;
                }
                auto out = settings();
                const auto scalar =
                    t.k == tk::string || t.k == tk::boolean || is_integer_kind(t.k) || is_float_kind(t.k);
                if (n->has("const") || n->has("enum"))
                {
                    if (!scalar) return std::nullopt;
                    const auto c = value_constraints(*n, t.k);
                    if (!c) return std::nullopt;
                    out.constraints = *c;
                }
                const auto any_key = [&](std::initializer_list<std::string_view> keys) {
                    return std::ranges::any_of(keys, [&](auto k) { return n->has(k); });
                };
                const auto string_keys = any_key({"minLength", "maxLength", "pattern", "format"});
                const auto number_keys =
                    any_key({"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf"});
                const auto array_keys = any_key({"items", "prefixItems", "contains", "minItems", "maxItems",
                                                 "uniqueItems", "minContains", "maxContains"});
                const auto object_keys =
                    any_key({"properties", "required", "additionalProperties", "patternProperties", "minProperties",
                             "maxProperties", "propertyNames", "dependentRequired"});
                if (t.k == tk::any && (string_keys || number_keys || array_keys || object_keys)) return std::nullopt;
                if (t.k == tk::string || is_integer_kind(t.k) || is_float_kind(t.k))
                    scalar_constraints(*n, t.k, out, loc);
                if (is_enum && string_keys) return std::nullopt;
                if (t.k == tk::tuple && array_keys) return std::nullopt;
                if (t.k == tk::vector && array_keys)
                {
                    if (n->has("prefixItems")) return std::nullopt;
                    if (const auto* items = n->find("items"))
                    {
                        auto e = settings_for(*items, t.args[0], loc + "/items");
                        if (!e) return std::nullopt;
                        if (!e->empty()) out.items.push_back(std::move(*e));
                    }
                    if (!array_counts(*n, out, loc)) return std::nullopt;
                    if (n->has("uniqueItems") && n->find("uniqueItems")->is_bool() && n->find("uniqueItems")->b)
                        out.unique = true;
                    if (n->has("contains") && !add_contains(*n, out, t.args[0], loc)) return std::nullopt;
                }
                if ((is_struct(t) || t.k == tk::object) && object_keys) return std::nullopt;
                if (t.k == tk::map && object_keys)
                {
                    if (any_key({"properties", "required", "patternProperties", "dependentRequired"}))
                        return std::nullopt;
                    if (const auto* ap = n->find("additionalProperties"))
                    {
                        auto e = settings_for(*ap, t.args[0], loc + "/additionalProperties");
                        if (!e) return std::nullopt;
                        if (!e->empty()) out.additional.push_back(std::move(*e));
                    }
                    if (const auto* pn = n->find("propertyNames"))
                    {
                        auto e = settings_for(*pn, make(tk::string), loc + "/propertyNames");
                        if (!e) return std::nullopt;
                        if (!e->empty()) out.property_names.push_back(std::move(*e));
                    }
                    if (const auto v = n->find("minProperties"); v != nullptr && as_uint64(*v) && *as_uint64(*v) > 0)
                        out.min_items = as_uint64(*v);
                    if (const auto v = n->find("maxProperties"); v != nullptr && as_uint64(*v))
                        out.max_items = as_uint64(*v);
                }
                return out;
            }

            // minItems / maxItems; false for a value that is not a count
            bool array_counts(const json& s, settings& out, const std::string& loc)
            {
                for (const auto* key : {"minItems", "maxItems"})
                    if (const auto* v = s.find(key))
                    {
                        const auto n = as_uint64(*v);
                        if (!n)
                        {
                            warn(loc, std::string("`") + key + "` is not a non-negative integer and is ignored");
                            continue;
                        }
                        if (key[1] == 'i' && *n > 0)
                            out.min_items = out.min_items ? std::max(*out.min_items, *n) : *n;
                        else if (key[1] == 'a')
                            out.max_items = out.max_items ? std::min(*out.max_items, *n) : *n;
                    }
                return true;
            }

            // contains / minContains / maxContains; false when `contains` cannot be expressed
            bool add_contains(const json& s, settings& out, const ctype& elem, const std::string& loc)
            {
                const auto* c = s.find("contains");
                if (c == nullptr) return true;
                auto cs = settings_for(*c, elem, loc + "/contains");
                if (!cs) return false;
                const auto* mn = s.find("minContains");
                const auto* mx = s.find("maxContains");
                const auto min_c = mn != nullptr && as_uint64(*mn) ? *as_uint64(*mn) : 1;
                const auto max_c = mx != nullptr ? as_uint64(*mx) : std::nullopt;
                if (cs->empty())
                {
                    // every element matches `contains`
                    if (min_c > 0) out.min_items = out.min_items ? std::max(*out.min_items, min_c) : min_c;
                    if (max_c) out.max_items = out.max_items ? std::min(*out.max_items, *max_c) : *max_c;
                    return true;
                }
                out.contains.push_back(std::move(*cs));
                if (mn != nullptr) out.min_contains = min_c;
                if (max_c) out.max_contains = max_c;
                return true;
            }

            static std::size_t node_count(const settings& s)
            {
                auto n = std::size_t(1);
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix})
                    for (const auto& c : *v) n += node_count(c);
                return n;
            }

            // The values of `yyjson::const_` / `yyjson::enum_` and the characters of the string literals of
            // the constraints that a position of `yyjson::prefix_items` holds, at most
            static std::pair<std::size_t, std::size_t> held_count(const settings& s)
            {
                auto values = std::size_t(0);
                auto chars = std::size_t(0);
                for (const auto& c : s.constraints)
                {
                    if (c.keyword == "const") ++values;
                    if (c.keyword == "enum" && c.value.is_array()) values += c.value.items.size();
                    if (c.value.is_string()) chars += c.value.text.size();
                    if (c.value.is_array())
                        for (const auto& v : c.value.items)
                            if (v.is_string()) chars += v.text.size();
                }
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix})
                    for (const auto& c : *v)
                    {
                        const auto [n, m] = held_count(c);
                        values += n;
                        chars += m;
                    }
                return {values, chars};
            }

            // Before C++26, a position of `yyjson::prefix_items` copies the string literals of its settings into 256
            // characters; a `std::string_view` is not copied.
            static void view_strings(settings& s)
            {
                for (auto& c : s.constraints)
                {
                    auto text = std::string();
                    for (std::size_t i = 0; i < c.text.size(); ++i)
                    {
                        if (c.text[i] != '"')
                        {
                            text += c.text[i];
                            continue;
                        }
                        auto end = i + 1;
                        while (c.text[end] != '"') end += c.text[end] == '\\' ? 2 : 1;
                        const auto literal = std::string_view(c.text).substr(i, end + 1 - i);
                        if (text.ends_with("std::string_view("))
                            text += literal;
                        else
                            text += "std::string_view(" + std::string(literal) + ")";
                        i = end;
                    }
                    c.text = std::move(text);
                }
                for (auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix})
                    for (auto& c : *v) view_strings(c);
            }

            // prefix_items settings within the limits of the library
            void set_prefix(settings& out, std::vector<settings> prefix, const std::string& loc)
            {
                if (std::ranges::all_of(prefix, [](const auto& p) { return p.empty(); })) return;
                if (prefix.size() > 16)
                {
                    warn(loc,
                         "constraints of more than 16 positions of `prefixItems` are not supported; they are dropped");
                    return;
                }
                for (auto& p : prefix)
                    if (node_count(p) > 8)
                    {
                        warn(loc, "constraints of a position of `prefixItems` nest too deeply; they are dropped");
                        p = {};
                    }
                    else if (const auto [values, chars] = held_count(p); values > 16)
                    {
                        warn(loc, "constraints of a position of `prefixItems` have more than 16 values of `const` and "
                                  "`enum`; they are dropped");
                        p = {};
                    }
                    else if (chars > 256 && !opts_.cpp26)
                        view_strings(p);
                if (std::ranges::all_of(prefix, [](const auto& p) { return p.empty(); })) return;
                out.prefix = std::move(prefix);
            }

            typed type_array(const json& s, const std::string& loc, const std::string& hint)
            {
                const auto* prefix = s.find("prefixItems");
                const auto* items = s.find("items");
                const auto items_false = items != nullptr && items->is_bool() && !items->b;
                auto r = typed();
                if (prefix != nullptr && prefix->is_array() && !prefix->items.empty())
                {
                    const auto n = prefix->items.size();
                    auto pos = std::vector<typed>();
                    for (std::size_t i = 0; i < n; ++i)
                        pos.push_back(type_of(&prefix->items[i], loc + "/prefixItems/" + std::to_string(i),
                                              hint + "Item" + std::to_string(i + 1)));
                    const auto* mi = s.find("minItems");
                    if (items_false && mi != nullptr && as_uint64(*mi) && *as_uint64(*mi) >= n)
                    {
                        r.t = make(tk::tuple);
                        auto ps = std::vector<settings>();
                        for (auto& p : pos)
                        {
                            r.t.args.push_back(std::move(p.t));
                            ps.push_back(std::move(p.s));
                        }
                        set_prefix(r.s, std::move(ps), loc);
                        if (s.has("uniqueItems") || s.has("contains"))
                            warn(loc, "`uniqueItems` and `contains` of a tuple are not supported and are ignored");
                        return r;
                    }
                    auto rest =
                        items != nullptr && !items_false ? type_of(items, loc + "/items", hint + "Item") : typed();
                    const auto elem = items_false ? spell(pos[0].t, true) : spell(rest.t, true);
                    if (std::ranges::all_of(pos, [&](const auto& p) { return spell(p.t, true) == elem; }))
                    {
                        r.t = make(tk::vector, {pos[0].t});
                        auto ps = std::vector<settings>();
                        for (auto& p : pos) ps.push_back(std::move(p.s));
                        set_prefix(r.s, std::move(ps), loc);
                        if (!items_false && !rest.s.empty()) r.s.items.push_back(std::move(rest.s));
                    }
                    else
                    {
                        warn(loc,
                             "positions of `prefixItems` of different types are read as yyjson::value; their "
                             "constraints are dropped");
                        r.t = make(tk::vector, {make(tk::any)});
                    }
                    if (items_false) r.s.max_items = n;
                }
                else if (items_false)
                {
                    r.t = make(tk::vector, {make(tk::any)});
                    r.s.max_items = 0;
                }
                else
                {
                    auto e = items != nullptr ? type_of(items, loc + "/items", hint + "Item") : typed();
                    r.t = make(tk::vector, {std::move(e.t)});
                    if (!e.s.empty()) r.s.items.push_back(std::move(e.s));
                }
                array_counts(s, r.s, loc);
                if (const auto* u = s.find("uniqueItems"); u != nullptr && u->is_bool() && u->b) r.s.unique = true;
                if (!add_contains(s, r.s, r.t.args[0], loc))
                    warn(loc + "/contains",
                         "`contains` with a type other than the element type is not supported and is ignored");
                return r;
            }

            typed type_map(const json& s, const std::string& loc, const std::string& hint)
            {
                auto r = typed{make(tk::map, {make(tk::any)}), {}};
                auto key_pattern = std::optional<std::string>();
                const auto* pp = s.find("patternProperties");
                if (pp != nullptr && pp->is_object() && !pp->members.empty())
                {
                    if (auto m = pattern_map_of(s, loc))
                    {
                        if (m->values != nullptr)
                        {
                            auto v = type_of(m->values, m->values_loc, hint + "Value");
                            r.t.args[0] = std::move(v.t);
                            if (!v.s.empty()) r.s.additional.push_back(std::move(v.s));
                        }
                        key_pattern = std::move(m->pattern);
                    }
                    else
                        warn(loc, "`patternProperties` with a schema that the name of a key selects is not supported; "
                                  "the values are read as any JSON value");
                }
                else if (const auto* ap = s.find("additionalProperties"))
                {
                    if (ap->is_bool() && !ap->b)
                        r.s.max_items = 0;
                    else if (!ap->is_bool())
                    {
                        auto v = type_of(ap, loc + "/additionalProperties", hint + "Value");
                        r.t.args[0] = std::move(v.t);
                        if (!v.s.empty()) r.s.additional.push_back(std::move(v.s));
                    }
                }
                if (const auto* v = s.find("minProperties"); v != nullptr && as_uint64(*v) && *as_uint64(*v) > 0)
                    r.s.min_items = as_uint64(*v);
                if (const auto* v = s.find("maxProperties"); v != nullptr && as_uint64(*v))
                    r.s.max_items = r.s.max_items ? std::min(*r.s.max_items, *as_uint64(*v)) : *as_uint64(*v);
                if (const auto* pn = s.find("propertyNames"))
                {
                    if (auto e = settings_for(*pn, make(tk::string), loc + "/propertyNames"); !e)
                        warn(loc + "/propertyNames",
                             "`propertyNames` is not expressed by string constraints and is ignored");
                    else if (!e->empty())
                        r.s.property_names.push_back(std::move(*e));
                }
                if (key_pattern) add_key_pattern(r.s, *key_pattern, loc + "/patternProperties");
                if (r.s.empty() && r.t.args[0].k == tk::any) return {make(tk::object), {}};
                return r;
            }

            // finalizing

            void name_members()
            {
                for (auto& d : decls_)
                {
                    if (d.kind != decl::kind_t::structure) continue;
                    auto used = std::set<std::string>();
                    for (auto& f : d.fields)
                    {
                        const auto base = f.extra ? std::string("extra") : sanitize(f.key);
                        auto m = base;
                        if (type_names_.contains(m)) m += "_";
                        for (auto n = 2; used.contains(m); ++n) m = base + "_" + std::to_string(n);
                        used.insert(m);
                        f.member = m;
                    }
                }
            }

            // structs that `t` holds by value
            void by_value(const ctype& t, std::vector<int>& out) const
            {
                switch (t.k)
                {
                    case tk::named:
                        if (decls_[t.decl].kind == decl::kind_t::structure)
                            out.push_back(t.decl);
                        else if (decls_[t.decl].kind == decl::kind_t::alias)
                            by_value(decls_[t.decl].target, out);
                        break;
                    case tk::optional:
                    case tk::variant:
                    case tk::tuple:
                        for (const auto& a : t.args) by_value(a, out);
                        break;
                    default:
                        break;
                }
            }

            void wrap(field& f)
            {
                auto t = f.type;
                if (t.k == tk::optional) t = ctype(t.args[0]);
                // an empty std::unique_ptr reads and writes null, so a variant does not need std::monostate for it
                if (t.k == tk::variant && !t.args.empty() && t.args[0].k == tk::monostate)
                {
                    auto v = t;
                    v.args.erase(v.args.begin());
                    if (v.combined != combination::none)
                    {
                        v.sets.erase(v.sets.begin());
                        v.metas.erase(v.metas.begin());
                        if (v.args.size() == 1 && f.set.empty()) f.set = std::move(v.sets[0]);
                    }
                    if (v.tagged)
                    {
                        const auto it =
                            std::ranges::find_if(notes_, [&](const auto& n) { return spell(n.type) == spell(t); });
                        if (it != notes_.end())
                        {
                            auto n = *it;
                            n.type = v;
                            n.alias = -1;
                            notes_.push_back(std::move(n));
                        }
                    }
                    t = v.args.size() == 1 && !v.tagged ? v.args[0] : std::move(v);
                }
                // only std::optional is left out when empty: an empty std::unique_ptr is written as null
                f.type = make(tk::pointer, {std::move(t)});
                if (!f.required) f.type = make(tk::optional, {std::move(f.type)});
                f.init.clear();
                f.omit_default = false;
            }

            // breaks cycles of structs held by value with std::unique_ptr: a struct that holds itself by value,
            // directly or through others, holds the field that closes the cycle through a pointer
            void break_cycles()
            {
                auto state = std::vector<int>(decls_.size(), 0);
                const auto visit = [&](const auto& self, int i) -> void {
                    state[i] = 1;
                    for (auto& f : decls_[i].fields)
                    {
                        auto deps = std::vector<int>();
                        by_value(f.type, deps);
                        if (std::ranges::any_of(deps, [&](int j) { return state[j] == 1; }))
                        {
                            wrap(f);
                            continue;
                        }
                        for (const auto j : deps)
                            if (state[j] == 0) self(self, j);
                    }
                    state[i] = 2;
                };
                for (std::size_t i = 0; i < decls_.size(); ++i)
                    if (decls_[i].kind == decl::kind_t::structure && state[i] == 0) visit(visit, static_cast<int>(i));
            }

            // returns the structs in definition order
            std::vector<int> order_structs()
            {
                auto state = std::vector<int>(decls_.size(), 0);

                // std::vector takes an incomplete element, but the standard does not promise it for std::map, and an
                // initializer that constructs a struct needs its definition. So every struct that a field names is
                // defined first, except in a cycle, which goes through containers now: the strongly connected
                // components of all named structs come in dependency order, and the structs of one component in the
                // order of the structs they hold by value
                const auto named = [&](int i) {
                    auto deps = std::vector<int>();
                    for (const auto& f : decls_[i].fields) named_in(f.type, deps);
                    for (std::size_t n = 0; n < deps.size(); ++n)
                        if (const auto j = deps[n];
                            decls_[j].kind == decl::kind_t::alias &&
                            std::ranges::find(deps.begin(), deps.begin() + n, j) == deps.begin() + n)
                            named_in(decls_[j].target, deps);
                    std::erase_if(deps, [&](int j) { return decls_[j].kind != decl::kind_t::structure; });
                    return deps;
                };
                auto index = std::vector<int>(decls_.size(), -1), low = index, component = index;
                auto stack = std::vector<int>();
                auto order = std::vector<int>();
                auto next = 0;
                const auto connect = [&](const auto& self, int i) -> void {
                    index[i] = low[i] = next++;
                    stack.push_back(i);
                    for (const auto j : named(i))
                        if (index[j] < 0)
                        {
                            self(self, j);
                            low[i] = std::min(low[i], low[j]);
                        }
                        else if (component[j] < 0)
                            low[i] = std::min(low[i], index[j]);
                    if (low[i] != index[i]) return;
                    auto members = std::vector<int>();
                    do
                    {
                        members.push_back(stack.back());
                        component[stack.back()] = i;
                        stack.pop_back();
                    } while (members.back() != i);
                    for (const auto k : members) state[k] = 0;
                    const auto place = [&](const auto& again, int k) -> void {
                        state[k] = 1;
                        for (const auto& f : decls_[k].fields)
                        {
                            auto deps = std::vector<int>();
                            by_value(f.type, deps);
                            for (const auto j : deps)
                                if (component[j] == i && state[j] == 0) again(again, j);
                        }
                        order.push_back(k);
                    };
                    for (const auto k : members | std::views::reverse)
                        if (state[k] == 0) place(place, k);
                };
                for (std::size_t i = 0; i < decls_.size(); ++i)
                    if (decls_[i].kind == decl::kind_t::structure && index[i] < 0) connect(connect, static_cast<int>(i));
                return order;
            }

            void named_in(const ctype& t, std::vector<int>& out) const
            {
                if (t.k == tk::named) out.push_back(t.decl);
                for (const auto& a : t.args) named_in(a, out);
            }

            std::vector<int> order_aliases() const
            {
                auto state = std::vector<int>(decls_.size(), 0);
                auto order = std::vector<int>();
                const auto visit = [&](const auto& self, int i) -> void {
                    state[i] = 1;
                    auto deps = std::vector<int>();
                    named_in(decls_[i].target, deps);
                    for (const auto j : deps)
                        if (decls_[j].kind == decl::kind_t::alias && state[j] == 0) self(self, j);
                    state[i] = 2;
                    order.push_back(i);
                };
                for (std::size_t i = 0; i < decls_.size(); ++i)
                    if (decls_[i].kind == decl::kind_t::alias && state[i] == 0) visit(visit, static_cast<int>(i));
                return order;
            }

            // examples

            // whether reading a JSON value into a C++ type succeeds, as far as the generator tells
            enum class fit : std::uint8_t
            {
                no,
                unknown,  // it depends on `pattern` or `contains`, which the generator does not check
                yes
            };

            static fit fits_constraints(const json& j, const settings& s)
            {
                auto r = fit::yes;
                for (const auto& c : s.constraints)
                {
                    const auto& v = c.value;
                    const auto& k = c.keyword;
                    auto ok = true;
                    if (k == "const")
                        ok = equal(j, v);
                    else if (k == "enum")
                        ok = std::ranges::any_of(v.items, [&](const json& e) { return equal(j, e); });
                    else if (k == "minLength")
                        ok = yyjson::detail::code_points(j.text) >= as_uint64(v).value_or(0);
                    else if (k == "maxLength")
                        ok = yyjson::detail::code_points(j.text) <= as_uint64(v).value_or(0);
                    else if (k == "format")
                    {
                        const auto check = yyjson::detail::builtin_format(v.text);
                        if (check == nullptr)
                            r = fit::unknown;
                        else
                            ok = check(j.text);
                    }
                    else if (k == "minimum")
                        ok = j.num >= v.num;
                    else if (k == "maximum")
                        ok = j.num <= v.num;
                    else if (k == "exclusiveMinimum")
                        ok = j.num > v.num;
                    else if (k == "exclusiveMaximum")
                        ok = j.num < v.num;
                    else if (k == "multipleOf")
                    {
                        // as the library does: integers by the remainder, and otherwise an integral quotient
                        const auto a = as_int64(j);
                        const auto m = as_int64(v);
                        if (a && m && j.is_integral() && v.is_integral())
                            ok = *a % *m == 0;
                        else
                        {
                            const auto q = j.num / v.num;
                            ok = std::isfinite(q) && q == std::trunc(q);
                        }
                    }
                    else
                        r = fit::unknown;
                    if (!ok) return fit::no;
                }
                return r;
            }

            fit fits_struct(const json& j, const decl& d) const
            {
                if (d.transparent) return fits(j, d.fields[0].type, d.fields[0].set);
                if (!j.is_object()) return fit::no;
                auto r = fit::yes;
                const field* extra = nullptr;
                for (const auto& f : d.fields)
                {
                    if (f.extra)
                    {
                        extra = &f;
                        continue;
                    }
                    if (const auto* v = j.find(f.key))
                        r = std::min(r, fits(*v, f.type, f.set));
                    else if (f.required)
                        return fit::no;
                    if (r == fit::no) return r;
                }
                for (const auto& [k, v] : j.members)
                {
                    if (std::ranges::any_of(d.fields, [&](const field& f) { return !f.extra && f.key == k; })) continue;
                    if (extra != nullptr)
                        r = std::min(r, fits(v, extra->type.args[0],
                                             extra->set.additional.empty() ? settings() : extra->set.additional[0]));
                    else if (!d.additional)
                        return fit::no;
                    if (r == fit::no) return r;
                }
                for (const auto& [k, keys] : d.dependent)
                    if (j.has(k) && std::ranges::any_of(keys, [&](const auto& x) { return !j.has(x); })) return fit::no;
                for (const auto& g : d.groups)
                {
                    const auto n = std::ranges::count_if(g.groups, [&](const auto& keys) {
                        return std::ranges::all_of(keys, [&](const auto& x) { return j.has(x); });
                    });
                    if (g.exactly_one ? n != 1 : n == 0) return fit::no;
                }
                return r;
            }

            fit fits(const json& j, const ctype& t, const settings& s) const
            {
                const auto scalar = [&](bool kind) { return kind ? fits_constraints(j, s) : fit::no; };
                const auto at = [](const std::vector<settings>& v, std::size_t i) {
                    return i < v.size() ? v[i] : settings();
                };
                switch (t.k)
                {
                    case tk::any:
                        return fit::yes;
                    case tk::object:
                        return j.is_object() ? fit::yes : fit::no;
                    case tk::null:
                    case tk::monostate:
                        return j.is_null() ? fit::yes : fit::no;
                    case tk::boolean:
                        return scalar(j.is_bool());
                    case tk::i32:
                    {
                        const auto v = as_int64(j);
                        return scalar(v && *v >= INT32_MIN && *v <= INT32_MAX);
                    }
                    case tk::i64:
                        return scalar(as_int64(j).has_value());
                    case tk::u64:
                        return scalar(as_uint64(j).has_value());
                    case tk::f32:
                        return scalar(j.is_number() && std::abs(j.num) <= std::numeric_limits<float>::max());
                    case tk::f64:
                        return scalar(j.is_number());
                    case tk::string:
                        return scalar(j.is_string());
                    case tk::named:
                    {
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return fits(j, d.target, s);
                        if (d.kind == decl::kind_t::structure) return fits_struct(j, d);
                        return scalar(j.is_string() && std::ranges::any_of(d.enumerators, [&](const auto& e) {
                                          return e.second == j.text;
                                      }));
                    }
                    case tk::vector:
                    {
                        if (!j.is_array()) return fit::no;
                        if ((s.min_items && j.items.size() < *s.min_items) ||
                            (s.max_items && j.items.size() > *s.max_items))
                            return fit::no;
                        if (s.unique)
                            for (std::size_t a = 0; a < j.items.size(); ++a)
                                for (auto b = a + 1; b < j.items.size(); ++b)
                                    if (equal(j.items[a], j.items[b])) return fit::no;
                        auto r = s.contains.empty() ? fit::yes : fit::unknown;
                        for (const auto& e : j.items)
                            if ((r = std::min(r, fits(e, t.args[0], at(s.items, 0)))) == fit::no) return r;
                        return r;
                    }
                    case tk::map:
                    {
                        if (!j.is_object()) return fit::no;
                        auto r = fit::yes;
                        for (const auto& [k, v] : j.members)
                        {
                            if (!s.property_names.empty())
                            {
                                auto key = json();
                                key.k = json::kind::string;
                                key.text = k;
                                r = std::min(r, fits_constraints(key, s.property_names[0]));
                            }
                            if ((r = std::min(r, fits(v, t.args[0], at(s.additional, 0)))) == fit::no) return r;
                        }
                        return r;
                    }
                    case tk::tuple:
                    {
                        if (!j.is_array() || j.items.size() != t.args.size()) return fit::no;
                        auto r = fit::yes;
                        for (std::size_t i = 0; i < t.args.size(); ++i)
                            if ((r = std::min(r, fits(j.items[i], t.args[i], at(s.prefix, i)))) == fit::no) return r;
                        return r;
                    }
                    case tk::variant:
                    {
                        if (t.combined == combination::one_of)
                        {
                            // exactly one alternative reads the value
                            auto yes = 0;
                            auto unknown = false;
                            for (std::size_t i = 0; i < t.args.size(); ++i)
                            {
                                const auto r = fits(j, t.args[i], at(t.sets, i));
                                yes += r == fit::yes ? 1 : 0;
                                unknown = unknown || r == fit::unknown;
                            }
                            if (yes > 1 || (yes == 0 && !unknown)) return fit::no;
                            return yes == 1 && !unknown ? fit::yes : fit::unknown;
                        }
                        // the settings of a nullable variant are those of its value; the alternatives of yyjson::any_of
                        // have their own, and those of the others none
                        auto r = fit::no;
                        for (std::size_t i = 0; i < t.args.size(); ++i)
                            r = std::max(r, fits(j, t.args[i], is_nullable(t) ? s : at(t.sets, i)));
                        return r;
                    }
                    case tk::optional:
                    case tk::pointer:
                        return j.is_null() ? fit::yes : fits(j, t.args[0], s);
                }
                return fit::unknown;
            }

            // `yyjson::json_schema<>()` reads every example of yyjson::examples as the type and throws if one is not
            // read: an example that the generator does not know to be read is moved to a comment
            void check_examples(metadata& m, const ctype& t, const settings& s, const std::string& loc)
            {
                auto no = 0;
                auto unknown = 0;
                auto kept = std::vector<json>();
                for (auto& e : m.examples)
                {
                    const auto r = fits(e, t, s);
                    if (r == fit::yes)
                        kept.push_back(std::move(e));
                    else
                    {
                        ++(r == fit::no ? no : unknown);
                        m.noted_examples.push_back(std::move(e));
                    }
                }
                m.examples = std::move(kept);
                const auto count = [](int n) {
                    return n == 1 ? std::string("an example") : std::to_string(n) + " examples";
                };
                if (no > 0)
                    warn(loc, count(no) + (no == 1 ? " does" : " do") +
                                  " not read as the C++ type; the comment `example:` has " + (no == 1 ? "it" : "them"));
                if (unknown > 0)
                    warn(loc, count(unknown) + " may not read as the C++ type: the generator does not check `pattern` "
                                               "and `contains`; the comment `example:` has " +
                                  (unknown == 1 ? "it" : "them"));
            }

            // the examples of the alternatives of yyjson::any_of and yyjson::one_of, which have no place for a
            // comment: those that the generator does not know to read are dropped
            void check_alternative_examples(ctype& t, const std::string& loc)
            {
                for (std::size_t i = 0; i < t.args.size(); ++i)
                {
                    check_alternative_examples(t.args[i], loc);
                    if (i >= t.metas.size()) continue;
                    auto& m = t.metas[i];
                    const auto n = m.examples.size();
                    std::erase_if(m.examples, [&](const json& e) { return fits(e, t.args[i], t.sets[i]) != fit::yes; });
                    if (m.examples.size() < n)
                        warn(loc, "an example of an alternative may not read as its C++ type and is dropped");
                }
            }

            void check_examples()
            {
                for (std::size_t i = 0; i < decls_.size(); ++i)
                {
                    auto& d = decls_[i];
                    if (d.kind != decl::kind_t::alias && !d.meta.examples.empty())
                        check_examples(d.meta, named_type(static_cast<int>(i)), settings(), d.loc);
                    for (auto& f : d.fields)
                        if (!f.meta.examples.empty()) check_examples(f.meta, f.type, f.set, f.loc);
                }
            }

            // rendering

            static std::string join(const std::vector<std::string>& v, std::string_view sep = ", ")
            {
                auto s = std::string();
                for (std::size_t i = 0; i < v.size(); ++i)
                {
                    if (i > 0) s += sep;
                    s += v[i];
                }
                return s;
            }

            std::vector<std::string> render_settings(const settings& s, const ctype& target) const
            {
                auto out = std::vector<std::string>();
                for (const auto& c : s.constraints) out.push_back(c.text);
                if (s.min_items) out.push_back("yyjson::min_items(" + std::to_string(*s.min_items) + ")");
                if (s.max_items) out.push_back("yyjson::max_items(" + std::to_string(*s.max_items) + ")");
                const auto& v = value_type(target);
                const auto elem = [&]() -> ctype {
                    return v.k == tk::vector || v.k == tk::map ? v.args[0] : make(tk::any);
                };
                if (s.unique) out.emplace_back("yyjson::unique_items");
                if (!s.prefix.empty())
                {
                    auto parts = std::vector<std::string>();
                    for (std::size_t i = 0; i < s.prefix.size(); ++i)
                    {
                        const auto p = render_settings(s.prefix[i], v.k == tk::tuple ? v.args[i] : elem());
                        parts.push_back(p.empty() ? "{}" : p.size() == 1 ? p[0] : "{" + join(p) + "}");
                    }
                    if (std::ranges::any_of(parts, [](const auto& p) { return p != "{}"; }))
                        out.push_back("yyjson::prefix_items(" + join(parts) + ")");
                }
                if (!s.items.empty())
                    if (const auto p = render_settings(s.items[0], elem()); !p.empty())
                        out.push_back("yyjson::items(" + join(p) + ")");
                if (!s.contains.empty())
                    if (const auto p = render_settings(s.contains[0], elem()); !p.empty())
                    {
                        out.push_back("yyjson::contains(" + join(p) + ")");
                        if (s.min_contains)
                            out.push_back("yyjson::min_contains(" + std::to_string(*s.min_contains) + ")");
                        if (s.max_contains)
                            out.push_back("yyjson::max_contains(" + std::to_string(*s.max_contains) + ")");
                    }
                if (!s.property_names.empty())
                    if (const auto p = render_settings(s.property_names[0], make(tk::string)); !p.empty())
                        out.push_back("yyjson::property_names(" + join(p) + ")");
                if (!s.additional.empty())
                    if (const auto p = render_settings(s.additional[0], elem()); !p.empty())
                        out.push_back("yyjson::additional_properties(" + join(p) + ")");
                return out;
            }

            static std::vector<std::string> render_metadata(const metadata& m)
            {
                auto out = std::vector<std::string>();
                if (m.title) out.push_back("yyjson::title(" + cpp_string(*m.title) + ")");
                if (m.description) out.push_back("yyjson::description(" + cpp_string(*m.description) + ")");
                if (m.deprecated) out.emplace_back("yyjson::deprecated");
                if (!m.examples.empty())
                {
                    auto ex = std::vector<std::string>();
                    for (const auto& e : m.examples) ex.push_back(raw_string(dump(e)));
                    out.push_back("yyjson::examples(" + join(ex) + ")");
                }
                return out;
            }

            static void render_comment(std::string& out, const metadata& m, std::string_view indent)
            {
                if (m.description)
                    for (const auto line : comment_lines(*m.description))
                        out += std::string(indent) + (line.empty() ? "///" : "/// " + std::string(line)) + "\n";
                if (m.comment)
                    for (const auto line : comment_lines(*m.comment))
                        out += std::string(indent) + (line.empty() ? "//" : "// " + std::string(line)) + "\n";
                for (const auto& e : m.noted_examples) out += std::string(indent) + "// example: " + dump(e) + "\n";
            }

            std::vector<std::string> field_items(const field& f)
            {
                auto items = std::vector<std::string>();
                if (f.extra)
                {
                    // the field has no key, and takes only the settings of its values
                    items.emplace_back("yyjson::extra_keys");
                    for (auto& s : render_settings(f.set, f.type)) items.push_back(std::move(s));
                    return items;
                }
                if (f.member != f.key) items.push_back("yyjson::rename(" + cpp_string(f.key) + ")");
                if (f.required) items.emplace_back("yyjson::required");
                if (f.omit_default) items.emplace_back("yyjson::omit_default");
                if (!f.default_from.empty())
                {
                    // the annotations of C++20 are outside the namespace
                    const auto q = opts_.cpp26 || opts_.ns.empty() ? std::string() : opts_.ns + "::";
                    items.push_back("yyjson::default_from<&" + q + f.default_from + ">");
                }
                for (auto& s : render_settings(f.set, f.type)) items.push_back(std::move(s));
                for (auto& s : render_metadata(f.meta)) items.push_back(std::move(s));
                return items;
            }

            std::vector<std::string> struct_items(const decl& d) const
            {
                auto items = std::vector<std::string>();
                if (!d.schema_name.empty()) items.push_back("yyjson::schema_name(" + cpp_string(d.schema_name) + ")");
                for (auto& s : render_metadata(d.meta)) items.push_back(std::move(s));
                if (d.transparent)
                {
                    items.emplace_back("yyjson::transparent");
                    return items;
                }
                // explicit, so that a read under yyjson::strict accepts the keys that the schema accepts
                items.emplace_back(d.additional ? "yyjson::additional_properties(true)"
                                                : "yyjson::additional_properties(false)");
                const auto member = [&](std::string_view key) {
                    for (const auto& f : d.fields)
                        if (!f.extra && f.key == key) return f.member;
                    return std::string(key);
                };
                for (const auto& [k, keys] : d.dependent)
                {
                    auto ms = std::vector<std::string>();
                    for (const auto& x : keys) ms.push_back(cpp_string(member(x)));
                    items.push_back("yyjson::dependent_required(" + cpp_string(member(k)) + ", {" + join(ms) + "})");
                }
                for (const auto& g : d.groups)
                {
                    auto gs = std::vector<std::string>();
                    for (const auto& group : g.groups)
                    {
                        auto ms = std::vector<std::string>();
                        for (const auto& x : group) ms.push_back(cpp_string(member(x)));
                        gs.push_back("{" + join(ms) + "}");
                    }
                    items.push_back(
                        std::string(g.exactly_one ? "yyjson::one_of_required(" : "yyjson::any_of_required(") +
                        join(gs) + ")");
                }
                return items;
            }

            static std::string annotation_spec(std::string_view type, const std::vector<std::string>& items)
            {
                auto s = "template <>\ninline constexpr auto yyjson::annotations<" + std::string(type) +
                         "> = yyjson::annotate(\n";
                for (std::size_t i = 0; i < items.size(); ++i)
                    s += "    " + items[i] + (i + 1 < items.size() ? ",\n" : ");\n");
                return s;
            }

            static std::string attributes(const std::vector<std::string>& items)
            {
                auto s = std::string("[[");
                for (std::size_t i = 0; i < items.size(); ++i) s += (i > 0 ? ", =" : "=") + items[i];
                return s + "]]";
            }

            std::string render()
            {
                break_cycles();
                name_members();
                const auto structs = order_structs();
                const auto aliases = order_aliases();
                for (auto& d : decls_)
                {
                    if (d.kind == decl::kind_t::alias) check_alternative_examples(d.target, d.loc);
                    for (auto& f : d.fields) check_alternative_examples(f.type, f.loc);
                }
                // the variables of the settings of yyjson::alt, named after the alias or the field that first has them;
                // the types of the functions of yyjson::default_from spell them too
                naming_ = !opts_.cpp26;
                for (const auto i : aliases)
                {
                    ctx_ = decls_[i].name;
                    (void)spell(decls_[i].target);
                }
                for (const auto i : structs)
                    for (const auto& f : decls_[i].fields)
                    {
                        ctx_ = decls_[i].name + "_" + f.member;
                        (void)spell(f.type);
                    }
                ctx_.clear();
                convert_defaults(structs);
                check_examples();
                const auto q = opts_.ns.empty() ? std::string() : opts_.ns + "::";
                auto body = std::string();
                auto after = std::string();

                for (const auto i : structs) body += "struct " + decls_[i].name + ";\n";
                if (!structs.empty()) body += "\n";

                for (std::size_t i = 0; i < decls_.size(); ++i)
                {
                    const auto& d = decls_[i];
                    if (d.kind != decl::kind_t::enumeration) continue;
                    render_comment(body, d.meta, "");
                    body += "enum class " + d.name + "\n{\n";
                    for (const auto& [m, n] : d.enumerators)
                    {
                        if (const auto it = d.enumerator_meta.find(n); it != d.enumerator_meta.end())
                            render_comment(body, it->second, "    ");
                        body += "    " + m + ",\n";
                    }
                    body += "};\n\n";
                    auto items = std::vector<std::string>();
                    if (!d.schema_name.empty())
                        items.push_back("yyjson::schema_name(" + cpp_string(d.schema_name) + ")");
                    for (auto& s : render_metadata(d.meta)) items.push_back(std::move(s));
                    for (const auto& [m, n] : d.enumerators)
                        if (m != n)
                            items.push_back("yyjson::enumerator(" + cpp_string(m) + ", yyjson::rename(" +
                                            cpp_string(n) + "))");
                    if (!items.empty()) after += "\n" + annotation_spec(q + d.name, items);
                    if (d.enumerators.size() > 129 && !opts_.cpp26)
                        after += "\ntemplate <>\nstruct field_reflection::enum_range<" + q + d.name +
                                 ">\n{\n    static constexpr int min = 0;\n    static constexpr int max = " +
                                 std::to_string(d.enumerators.size() - 1) + ";\n};\n";
                }
                // the variables of the settings of yyjson::alt go here, when every type is spelled
                const auto vars_at = body.size();

                for (const auto i : aliases)
                {
                    const auto& d = decls_[i];
                    if (d.meta.title)
                        for (const auto line : comment_lines(*d.meta.title))
                            body += line.empty() ? "///\n" : "/// " + std::string(line) + "\n";
                    render_comment(body, d.meta, "");
                    if (shares_type(d.target)) body += "// alternatives share a type; use index()\n";
                    body += "using " + d.name + " = " + spell(d.target) + ";\n\n";
                }

                // the functions of yyjson::default_from, which the annotations of the members name
                for (const auto& fn : default_functions_) body += fn.type + " " + fn.name + "();\n";
                if (!default_functions_.empty()) body += "\n";

                for (const auto i : structs)
                {
                    auto& d = decls_[i];
                    render_comment(body, d.meta, "");
                    const auto items = struct_items(d);
                    body += "struct ";
                    if (opts_.cpp26 && !items.empty()) body += attributes(items) + " ";
                    body += d.name + "\n{\n";
                    auto field_specs = std::vector<std::string>();
                    for (const auto& f : d.fields)
                    {
                        render_comment(body, f.meta, "    ");
                        if (!f.default_note.empty()) body += "    // default: " + f.default_note + "\n";
                        if (shares_type(f.type)) body += "    // alternatives share a type; use index()\n";
                        const auto fi = field_items(f);
                        body += "    ";
                        if (opts_.cpp26 && !fi.empty()) body += attributes(fi) + " ";
                        body += spell(f.type) + " " + f.member + f.init + ";\n";
                        if (!opts_.cpp26 && !fi.empty())
                        {
                            auto spec = cpp_string(f.member);
                            for (const auto& x : fi) spec += ", " + x;
                            field_specs.push_back("yyjson::field(" + spec + ")");
                        }
                    }
                    body += "};\n\n";
                    if (!opts_.cpp26)
                    {
                        auto all = items;
                        for (auto& s : field_specs) all.push_back(std::move(s));
                        if (!all.empty()) after += "\n" + annotation_spec(q + d.name, all);
                        // after the annotations, which the macro needs to see
                        if (d.fields.size() > max_reflected_fields)
                        {
                            after += "\nVISITABLE_STRUCT(" + q + d.name;
                            for (const auto& f : d.fields) after += ",\n                 " + f.member;
                            after += ");\n";
                        }
                    }
                }

                for (const auto& fn : default_functions_)
                    body += "inline " + fn.type + " " + fn.name + "()\n{\n    return " + fn.value + ";\n}\n\n";

                auto seen = std::map<std::string, const variant_note*>();
                for (const auto& n : notes_)
                {
                    const auto key = spell(n.type, true, q);
                    if (const auto it = seen.find(key); it != seen.end())
                    {
                        if (it->second->key != n.key || it->second->ids != n.ids)
                            warn(n.loc,
                                 "the variant " + spell(n.type) + " has another tag elsewhere; this tag is not used");
                        continue;
                    }
                    seen.emplace(key, &n);
                    auto ids = std::vector<std::string>();
                    for (const auto& id : n.ids) ids.push_back(cpp_string(id));
                    auto items = std::vector<std::string>{"yyjson::internal_tag(" + cpp_string(n.key) + ", {" +
                                                          join(ids) + "})"};
                    if (n.alias >= 0)
                    {
                        const auto& a = decls_[n.alias];
                        items.push_back("yyjson::schema_name(" +
                                        cpp_string(a.schema_name.empty() ? a.name : a.schema_name) + ")");
                    }
                    after += "\n" + annotation_spec(key, items);
                }

                if (!var_defs_.empty())
                {
                    auto vars = std::string();
                    for (const auto& [name, text] : var_defs_)
                        vars += "inline constexpr auto " + name + " = " + text + ";\n";
                    body.insert(vars_at, vars + "\n");
                }
                const auto alternatives = [&](const auto& self, const ctype& t) -> bool {
                    return t.combined != combination::none ||
                           std::ranges::any_of(t.args, [&](const ctype& a) { return self(self, a); });
                };
                const auto has_alternatives = std::ranges::any_of(decls_, [&](const decl& d) {
                    return alternatives(alternatives, d.target) ||
                           std::ranges::any_of(d.fields, [&](const field& f) { return alternatives(alternatives, f.type); });
                });

                auto out = std::string("// Generated by cpp-yyjson-codegen");
                if (!opts_.source.empty()) out += " from " + opts_.source;
                out += ". Do not edit.\n";
                if (!warnings_.empty())
                {
                    out += "//\n// Warnings:\n";
                    for (const auto& w : warnings_)
                    {
                        const auto text = w.location + ": " + w.message;
                        for (auto first = true; const auto line : comment_lines(text))
                            out += (std::exchange(first, false) ? "//   " : "//     ") + std::string(line) + "\n";
                    }
                }
                const auto any_field = [&](auto pred) {
                    return std::ranges::any_of(decls_,
                                               [&](const decl& d) { return std::ranges::any_of(d.fields, pred); });
                };
                if (any_field([](const field& f) { return f.omit_default; }))
                    out +=
                        "//\n// A field with a default holds it as its initializer and has `yyjson::omit_default`: a "
                        "missing key\n// reads as the default, and writing leaves the key out while the field holds "
                        "it.\n";
                if (!default_functions_.empty())
                    out +=
                        "//\n// A field with `yyjson::default_from` gets the value of the function when the key is "
                        "missing, and\n// writing leaves the key out while the field holds it: a member initializer "
                        "cannot construct\n// a struct that holds itself or a field held through std::unique_ptr.\n";
                if (std::ranges::any_of(decls_, [](const decl& d) { return d.transparent; }))
                    out +=
                        "//\n// A struct with `yyjson::transparent` is read and written as the value of its only "
                        "field, whose\n// constraints are those of the root.\n";
                if (any_field([](const field& f) { return f.extra; }))
                    out +=
                        "//\n// A field with `yyjson::extra_keys` holds the keys of the object that name no other "
                        "field, and\n// writing puts them after the fields.\n";
                if (any_field([](const field& f) { return !f.default_note.empty(); }))
                    out +=
                        "//\n// A std::optional field with a `default:` comment is std::nullopt when the key is "
                        "missing; the\n// program applies the default of the comment.\n";
                if (std::ranges::any_of(decls_, [](const decl& d) { return !d.meta.noted_examples.empty(); }) ||
                    any_field([](const field& f) { return !f.meta.noted_examples.empty(); }))
                    out +=
                        "//\n// An `example:` comment holds an example of the schema that the generator does not "
                        "know to read as\n// the C++ type: `yyjson::json_schema<>()` throws for an example of "
                        "`yyjson::examples` that does\n// not read as the type, and leaves the comments out of the "
                        "schema.\n";
                if (!opts_.cpp26 && std::ranges::any_of(decls_, [](const decl& d) {
                        return d.kind == decl::kind_t::structure && d.fields.size() > max_reflected_fields;
                    }))
                    out += "//\n// A struct of more than " + std::to_string(max_reflected_fields) +
                           " fields is registered with VISITABLE_STRUCT: the reflection of C++20\n// takes no "
                           "more.\n";
                if (has_alternatives)
                {
                    out +=
                        "//\n// `yyjson::any_of` reads the first alternative that accepts the value, and "
                        "`yyjson::one_of` the only\n// one; `yyjson::alt` gives an alternative its constraints and "
                        "metadata.";
                    if (!var_defs_.empty())
                        out += " A setting with a string is a\n// constexpr variable, to which the argument of "
                               "`yyjson::alt` points.";
                    out += "\n";
                }
                if (uses_pattern_)
                    out +=
                        "//\n// `yyjson::pattern` needs a specialization of `yyjson::pattern_matcher<>` in the "
                        "program.\n";
                out += "\n#pragma once\n\n";
                for (const auto* h : {"cstddef", "cstdint", "map", "memory", "optional", "string", "tuple", "utility",
                                      "variant", "vector"})
                    out += std::string("#include <") + h + ">\n";
                out += "\n#include <cpp_yyjson.hpp>\n\n";
                if (opts_.cpp26)
                    out +=
                        "#if !FIELD_RFL_ANNOTATIONS\n#error \"this header needs C++26 annotations "
                        "(FIELD_RFL_ANNOTATIONS)\"\n#endif\n\n";
                // the members that a designated initializer leaves out keep their initializers, as intended
                if (partial_init_)
                    out +=
                        "#if defined(__GNUC__) || defined(__clang__)\n#pragma GCC diagnostic push\n"
                        "#pragma GCC diagnostic ignored \"-Wmissing-field-initializers\"\n#endif\n\n";
                if (!opts_.ns.empty()) out += "namespace " + opts_.ns + "\n{\n";
                while (body.ends_with("\n\n")) body.pop_back();
                out += body;
                if (!opts_.ns.empty()) out += "}  // namespace " + opts_.ns + "\n";
                if (partial_init_)
                    out += "\n#if defined(__GNUC__) || defined(__clang__)\n#pragma GCC diagnostic pop\n#endif\n";
                out += after;
                return out;
            }

        public:
            explicit generator(const options& opts) : opts_(opts) {}

            result run(std::string_view text)
            {
                if (!opts_.ns.empty())
                {
                    auto ns = std::string_view(opts_.ns);
                    for (auto pos = std::size_t(); pos <= ns.size();)
                    {
                        const auto end = std::min(ns.find("::", pos), ns.size());
                        if (!is_identifier(ns.substr(pos, end - pos)))
                            fail("", "the namespace \"" + opts_.ns + "\" is not a C++ namespace name");
                        pos = end + 2;
                    }
                }
                if (!opts_.root_name.empty() && !is_identifier(opts_.root_name))
                    fail("", "the root name \"" + opts_.root_name + "\" is not a C++ identifier");
                try
                {
                    root_ = to_json(yyjson::read(text));
                }
                catch (const std::exception& e)
                {
                    fail("", std::string("the schema is not valid JSON: ") + e.what());
                }
                if (!root_.is_object() && !root_.is_bool()) fail("#", "the schema is not an object or a boolean");
                check_dialect(root_, "#");
                if (!opts_.path.empty())
                    root_path_ = std::filesystem::path(opts_.path).lexically_normal().generic_string();
                if (const auto id = absolute_id(root_); !id.empty()) ids_.emplace(id, "");
                prepare(root_, "", absolute_id(root_), is_draft07(root_), "#");
                // a root of metadata and definitions only is a file of definitions and has no type; without
                // definitions, it accepts any JSON value
                const auto root_typed =
                    root_.is_bool() ||
                    (!is_pure_ref(root_) && (std::ranges::any_of(root_.members,
                                                                 [](const auto& m) {
                                                                     return is_type_key(m.first) ||
                                                                            is_ignored_key(m.first);
                                                                 }) ||
                                             (!root_.has("$defs") && !root_.has("definitions"))));
                auto root_name = std::string();
                if (root_.is_object())
                {
                    const auto* title = root_.find("title");
                    root_name = !opts_.root_name.empty() ? opts_.root_name
                                : title != nullptr && title->is_string() && !pascal(title->text).empty()
                                    ? sanitize(pascal(title->text))
                                    : std::string("Root");
                }
                if (root_typed)
                {
                    type_names_.insert(root_name);
                    def_names_[&root_] = root_name;
                }
                auto defs = std::vector<std::pair<std::string, const json*>>();
                for (const auto* kw : {"$defs", "definitions"})
                    if (const auto* d = root_.find(kw); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                        {
                            name_def(&v, k);
                            defs.emplace_back(std::string("#/") + kw + "/" + pointer_token(k), &v);
                        }
                for (const auto& [loc, v] : defs) type_of(v, loc, def_names_.at(v));
                if (root_typed)
                    type_of(&root_, "#", root_name);
                else if (is_pure_ref(root_))
                {
                    // a root that only refers to another schema has the type of that schema, under the root name too
                    const auto* target = resolve(*root_.find("$ref"), "#");
                    const auto it = def_names_.find(target);
                    if (target != &root_ && (it == def_names_.end() || it->second != root_name))
                    {
                        const auto name = type_names_.contains(root_name) ? unique_type_name(root_name) : root_name;
                        type_names_.insert(name);
                        def_names_[&root_] = name;
                        type_named(&root_, "#");
                    }
                }
                if (decls_.empty()) warn("#", "the schema defines no type");
                auto code = render();
                if (opts_.strict && !warnings_.empty()) throw error(warnings_);
                return {std::move(code), std::move(warnings_)};
            }
        };
    }  // namespace detail

    inline result generate(std::string_view schema, const options& opts) { return detail::generator(opts).run(schema); }
}  // namespace cpp_yyjson_codegen

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
