// cpp-yyjson-codegen: C++ types with cpp-yyjson annotations from a JSON Schema.
// This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <optional>
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
        std::string root_name{};  // name of the root type; default: the title, or "Root"
        std::string source{};     // name of the schema in the header comment
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

        struct ctype
        {
            tk k = tk::any;
            int decl = -1;
            bool tagged = false;  // a variant with internal_tag
            std::vector<ctype> args{};
        };

        // the settings of a field or of an element level: constraints and element trees
        struct settings
        {
            std::vector<std::string> constraints;  // rendered, such as "yyjson::minimum(0)"
            std::optional<std::uint64_t> min_items, max_items;
            bool unique = false;
            std::string unique_loc;
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
            std::vector<std::string> examples;  // JSON texts
        };

        struct field
        {
            std::string member{}, key{};
            ctype type{};
            settings set{};
            bool required = false, omit_default = false;
            std::string init{};          // " = x" or "{...}"
            std::string default_note{};  // the default of a std::optional field, for a comment
            metadata meta{};
            std::string loc{};
        };

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
            bool eq = false;
            bool filled = false;
            // enum: member name, JSON name
            std::vector<std::pair<std::string, std::string>> enumerators{};
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
            return t.k == tk::variant && !t.tagged && t.args.size() == 2 && t.args[0].k == tk::monostate;
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

        inline bool is_pure_ref(const json& s)
        {
            return s.is_object() && s.has("$ref") && std::ranges::all_of(s.members, [](const auto& m) {
                       return m.first == "$ref" || is_metadata_key(m.first);
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
            std::deque<decl> decls_;
            std::map<const json*, std::string> def_names_;  // $defs entries -> type name
            std::map<const json*, std::string> def_keys_;   // $defs entries -> schema_name
            std::map<const json*, typed> cache_;
            std::set<const json*> busy_;
            std::map<const json*, const json*> normalized_;
            std::set<const json*> normalizing_;
            std::vector<variant_note> notes_;
            std::set<std::string> type_names_;
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

            // the target of a $ref, or nullptr with a warning
            const json* resolve(const json& ref, const std::string& loc)
            {
                if (!ref.is_string())
                {
                    warn(loc, "`$ref` is not a string");
                    return nullptr;
                }
                const auto& r = ref.text;
                if (r.empty() || r[0] != '#')
                {
                    warn(loc, "the reference \"" + r + "\" is not in this document and is read as any JSON value");
                    return nullptr;
                }
                const auto fragment = percent_decode(std::string_view(r).substr(1));
                if (!fragment.empty() && fragment[0] != '/')
                {
                    if (const auto* a = find_anchor(root_, fragment)) return a;
                    warn(loc, "the anchor \"" + r + "\" is not found");
                    return nullptr;
                }
                const auto* cur = &root_;
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
                return cur;
            }

            static const json* find_anchor(const json& j, std::string_view name)
            {
                if (j.is_object())
                {
                    if (const auto* a = j.find("$anchor"); a != nullptr && a->is_string() && a->text == name) return &j;
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
                if (!has_all && !ref_siblings) return s;
                if (const auto it = normalized_.find(s); it != normalized_.end()) return it->second;
                if (!normalizing_.insert(s).second)
                {
                    warn(loc, "`allOf` refers to itself; the schema is read as any JSON value");
                    return store(json::boolean_of(true));
                }
                auto base = json::object_of();
                for (const auto& [k, v] : s->members)
                    if (k != "allOf" && k != "$ref") base.members.emplace_back(k, v);
                const auto* all = s->find("allOf");
                if (!s->has("$ref") && all != nullptr && all->is_array() && all->items.size() == 1 &&
                    is_pure_ref(all->items[0]) &&
                    std::ranges::all_of(base.members, [](const auto& m) { return is_metadata_key(m.first); }))
                {
                    // {"allOf": [{"$ref": ...}]} with metadata only: keep the reference to the named type
                    base.set("$ref", *all->items[0].find("$ref"));
                    const auto* r = store(std::move(base));
                    normalizing_.erase(s);
                    return normalized_[s] = r;
                }
                if (const auto* ref = s->find("$ref"))
                    if (const auto* t = resolve(*ref, loc)) base = merge(base, *normalize_full(t, loc), loc);
                if (all != nullptr)
                {
                    if (!all->is_array())
                        warn(loc, "`allOf` is not an array");
                    else
                        for (std::size_t i = 0; i < all->items.size(); ++i)
                        {
                            const auto l = loc + "/allOf/" + std::to_string(i);
                            base = merge(base, *normalize_full(&all->items[i], l), l);
                        }
                }
                const auto* r = store(std::move(base));
                normalizing_.erase(s);
                return normalized_[s] = r;
            }

            static json all_of(const json& a, const json& b)
            {
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
                    for (const auto& e : t->items) m.examples.push_back(dump(e));
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
                auto d = decl{.kind = decl::kind_t::alias, .name = name, .loc = loc, .target = r.t, .set = r.s};
                if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                const auto idx = add_decl(std::move(d));
                if (r.t.k == tk::variant && r.t.tagged)
                    for (auto& n : notes_)
                        if (n.alias < 0 && spell(n.type) == spell(r.t)) n.alias = idx;
                return cache_[s0] = typed{named_type(idx), r.s};
            }

            void warn_unsupported(const json& s, const std::string& loc)
            {
                for (const auto* k :
                     {"if", "then", "else", "not", "dependentSchemas", "unevaluatedProperties", "unevaluatedItems",
                      "$dynamicRef", "$recursiveRef", "dependencies", "additionalItems"})
                    if (s.has(k)) warn(loc, std::string("`") + k + "` is not supported and is ignored");
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
                    return type_of(t, ref->text, hint);
                }
                if (s->has("anyOf") || s->has("oneOf")) return type_variant(s0, s, loc, hint, named);
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
                            if (dflt != nullptr) f.init = init_for(ft.t, *dflt, floc);
                            f.type = std::move(ft.t);
                            f.required = true;
                        }
                        else if (dflt == nullptr || (dflt->is_null() && !is_nullable(ft.t)))
                        {
                            // no default, or `null` for a type without null, which authors write for "unset": the
                            // missing key is std::nullopt
                            f.type = make(tk::optional, {std::move(ft.t)});
                        }
                        else if (auto init = opts_.optional_defaults ? std::nullopt : init_value(ft.t, *dflt))
                        {
                            // a missing key means the default, which the member holds then; writing leaves the key
                            // out while the member still holds it, so a default outside the constraints, such as
                            // -1 for "no limit", is never written and read back
                            f.init = std::move(*init);
                            f.type = std::move(ft.t);
                            f.omit_default = true;
                        }
                        else
                        {
                            // std::nullopt is the missing key, which the program reads as the default in the comment
                            if (!opts_.optional_defaults)
                                warn(floc, "the default " + dump(*dflt) +
                                               " is not converted to the C++ type; the field is std::optional");
                            f.default_note = dump(*dflt);
                            f.type = make(tk::optional, {std::move(ft.t)});
                        }
                        fields.push_back(std::move(f));
                    }
                }
                for (const auto& k : required)
                    if (std::ranges::none_of(fields, [&](const auto& f) { return f.key == k; }))
                        fields.push_back(field{
                            .member = k, .key = k, .type = make(tk::any), .required = true, .loc = loc + "/required"});
                auto additional = true;
                if (const auto* ap = s.find("additionalProperties"))
                {
                    if (ap->is_bool())
                        additional = ap->b;
                    else if (!(ap->is_object() && ap->members.empty()))
                        warn(loc + "/additionalProperties",
                             "a schema of `additionalProperties` beside `properties` is not supported; other keys are "
                             "accepted as any JSON value");
                }
                if (s.has("patternProperties"))
                {
                    if (!additional)
                        warn(loc, "`patternProperties` is not supported; `additionalProperties: false` is dropped");
                    else
                        warn(loc,
                             "`patternProperties` beside `properties` is not supported; other keys are accepted as any "
                             "JSON value");
                    additional = true;
                }
                for (const auto* k : {"minProperties", "maxProperties", "propertyNames"})
                    if (s.has(k))
                        warn(loc,
                             std::string("`") + k + "` of an object with properties is not supported and is ignored");
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
                auto& d = decls_[idx];
                d.fields = std::move(fields);
                d.additional = additional;
                d.dependent = std::move(dependent);
                d.filled = true;
            }

            // the initializer of a field from a JSON value: " = x", "{...}", or empty
            std::string init_for(const ctype& t, const json& j, const std::string& loc)
            {
                auto r = init_value(t, j);
                if (!r) warn(loc, "the default " + dump(j) + " is not converted to the C++ type and is dropped");
                return r ? *r : std::string();
            }

            std::optional<std::string> init_value(const ctype& t, const json& j)
            {
                using namespace std::string_literals;
                switch (t.k)
                {
                    case tk::boolean:
                        if (j.is_bool()) return j.b ? " = true"s : " = false"s;
                        return std::nullopt;
                    case tk::i32:
                        if (const auto v = as_int64(j); v && *v >= INT32_MIN && *v <= INT32_MAX)
                            return " = " + int_literal(*v);
                        return std::nullopt;
                    case tk::i64:
                        if (const auto v = as_int64(j)) return " = " + int_literal(*v);
                        return std::nullopt;
                    case tk::u64:
                        if (const auto v = as_uint64(j)) return " = " + uint_literal(*v);
                        return std::nullopt;
                    case tk::f32:
                    case tk::f64:
                        if (j.is_number()) return " = " + float_literal(j, t.k == tk::f32);
                        return std::nullopt;
                    case tk::string:
                        if (j.is_string()) return " = " + cpp_string(j.text, "std::string");
                        return std::nullopt;
                    case tk::named:
                    {
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return init_value(d.target, j);
                        if (d.kind == decl::kind_t::enumeration && j.is_string())
                            for (const auto& [m, n] : d.enumerators)
                                if (n == j.text) return " = " + d.name + "::" + m;
                        return std::nullopt;
                    }
                    case tk::variant:
                        if (is_nullable(t))
                        {
                            if (j.is_null()) return "{}"s;
                            if (const auto v = init_value(t.args[1], j); v && v->starts_with(" = "))
                                return "{std::in_place_index<1>, " + v->substr(3) + "}";
                        }
                        return std::nullopt;
                    case tk::vector:
                    {
                        if (!j.is_array()) return std::nullopt;
                        auto out = std::string("{");
                        for (auto first = true; const auto& e : j.items)
                        {
                            const auto v = init_value(t.args[0], e);
                            if (!v || !v->starts_with(" = ")) return std::nullopt;
                            if (!std::exchange(first, false)) out += ", ";
                            out += v->substr(3);
                        }
                        return out + "}";
                    }
                    default:
                        return std::nullopt;
                }
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
                        return list("std::variant");
                    case tk::optional:
                        return list("std::optional");
                    case tk::pointer:
                        return list("std::unique_ptr");
                }
                return {};
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
                    if (r.t.args[0].k != tk::monostate) r.t.args.insert(r.t.args.begin(), make(tk::monostate));
                    return {std::move(r.t), {}};
                }
                return {make(tk::variant, {make(tk::monostate), std::move(r.t)}), std::move(r.s)};
            }

            typed make_variant(std::vector<typed> alts, const std::string& loc)
            {
                auto flat = std::vector<typed>();
                for (auto& a : alts)
                {
                    if (a.t.k == tk::variant && !a.t.tagged)
                    {
                        if (is_nullable(a.t))
                        {
                            flat.push_back({make(tk::monostate), {}});
                            flat.push_back({a.t.args[1], std::move(a.s)});
                        }
                        else
                            for (auto& x : a.t.args) flat.push_back({x, {}});
                    }
                    else if (a.t.k == tk::null)
                        flat.push_back({make(tk::monostate), {}});
                    else
                        flat.push_back(std::move(a));
                }
                if (std::ranges::any_of(flat, [](const auto& a) { return a.t.k == tk::any; })) return {};
                auto uniq = std::vector<typed>();
                auto mono = false;
                for (auto& a : flat)
                {
                    if (a.t.k == tk::monostate)
                    {
                        mono = true;
                        continue;
                    }
                    const auto s = spell(a.t, true);
                    const auto it = std::ranges::find_if(uniq, [&](const auto& u) { return spell(u.t, true) == s; });
                    if (it == uniq.end())
                        uniq.push_back(std::move(a));
                    else if (!it->s.empty() || !a.s.empty())
                    {
                        warn(loc, "alternatives of the same C++ type " + s + " have constraints; they are dropped");
                        it->s = {};
                    }
                }
                if (uniq.empty()) return {make(mono ? tk::null : tk::any), {}};
                if (uniq.size() == 1) return mono ? make_nullable(std::move(uniq[0])) : std::move(uniq[0]);
                for (auto& u : uniq)
                    if (!u.s.empty())
                    {
                        warn(loc,
                             "constraints of one alternative of a std::variant are not supported; they are dropped");
                        u.s = {};
                    }
                std::ranges::stable_sort(uniq, {}, [&](const auto& u) { return rank(u.t); });
                auto args = std::vector<ctype>();
                if (mono) args.push_back(make(tk::monostate));
                for (auto& u : uniq) args.push_back(std::move(u.t));
                return {make(tk::variant, std::move(args)), {}};
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
                        r.s.constraints.push_back(std::string("yyjson::const_(") + (c.b ? "true" : "false") + ")");
                        return r;
                    case json::kind::number:
                        if (const auto v = as_int64(c); v && c.is_integral())
                        {
                            r.t = make(tk::i64);
                            r.s.constraints.push_back("yyjson::const_(" + int_literal(*v) + ")");
                        }
                        else if (const auto u = as_uint64(c); u && c.is_integral())
                        {
                            r.t = make(tk::u64);
                            r.s.constraints.push_back("yyjson::const_(" + uint_literal(*u) + ")");
                        }
                        else
                        {
                            r.t = make(tk::f64);
                            r.s.constraints.push_back("yyjson::const_(" + float_literal(c) + ")");
                        }
                        return r;
                    case json::kind::string:
                        r.t = make(tk::string);
                        r.s.constraints.push_back("yyjson::const_(" + cpp_string(c.text) + ")");
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
                        r.s.constraints.push_back(std::string("yyjson::const_(") + (bools[0] ? "true" : "false") +
                                                  ")");
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
                    r.s.constraints.push_back(c + ")");
                    alts.push_back(std::move(r));
                }
                if (!strings.empty()) alts.push_back(make_enum(s0, s, strings, loc, hint, named && groups == 1));
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

            typed type_variant(const json* s0, const json* s, const std::string& loc, const std::string& hint,
                               bool named)
            {
                const auto* any = s->find("anyOf");
                const auto* one = s->find("oneOf");
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
                    if (k != "anyOf" && k != "oneOf" && !is_metadata_key(k)) base.members.emplace_back(k, v);
                auto schemas = std::vector<const json*>();
                auto locs = std::vector<std::string>();
                for (std::size_t i = 0; i < list->items.size(); ++i)
                {
                    locs.push_back(loc + "/" + kw + "/" + std::to_string(i));
                    if (base.members.empty())
                        schemas.push_back(&list->items[i]);
                    else
                    {
                        auto c = base;
                        c.set("allOf", json::array_of({list->items[i]}));
                        schemas.push_back(store(std::move(c)));
                    }
                }
                if (auto r = tagged_variant(schemas, locs, loc)) return *r;
                auto alts = std::vector<typed>();
                for (std::size_t i = 0; i < schemas.size(); ++i)
                {
                    const auto* n = normalize(schemas[i], locs[i]);
                    const auto* title = n->is_object() ? n->find("title") : nullptr;
                    const auto h = title != nullptr && title->is_string() && !pascal(title->text).empty()
                                       ? pascal(title->text)
                                       : hint + "Option" + std::to_string(i + 1);
                    alts.push_back(type_of(schemas[i], locs[i], h));
                }
                (void)s0;
                (void)named;
                return make_variant(std::move(alts), loc);
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
                    else if (n->is_object() && n->has("properties") && !n->has("anyOf") && !n->has("oneOf"))
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
                        out.constraints.push_back("yyjson::min_length(" + std::to_string(*n) + ")");
                    if (const auto n = count("maxLength"))
                        out.constraints.push_back("yyjson::max_length(" + std::to_string(*n) + ")");
                    if (const auto* p = s.find("pattern"); p != nullptr && p->is_string())
                    {
                        if (opts_.pattern)
                        {
                            out.constraints.push_back("yyjson::pattern(" + cpp_string(p->text) + ")");
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
                            out.constraints.push_back("yyjson::format(" + cpp_string(f->text) + ")");
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
                                out.constraints.push_back(std::string("yyjson::") + name + "(" + *b + ")");
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
                            out.constraints.push_back("yyjson::multiple_of(" + lit + ")");
                        }
                    }
                }
            }

            // the constraints of `const` / `enum` on a value of kind `k`, or nothing when they do not apply
            std::optional<std::vector<std::string>> value_constraints(const json& s, tk k)
            {
                auto out = std::vector<std::string>();
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
                    out.push_back("yyjson::const_(" + *l + ")");
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
                    out.push_back(c + ")");
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
                    {
                        out.unique = true;
                        out.unique_loc = loc;
                    }
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
                if (const auto* u = s.find("uniqueItems"); u != nullptr && u->is_bool() && u->b)
                {
                    r.s.unique = true;
                    r.s.unique_loc = loc;
                }
                if (!add_contains(s, r.s, r.t.args[0], loc))
                    warn(loc + "/contains",
                         "`contains` with a type other than the element type is not supported and is ignored");
                return r;
            }

            typed type_map(const json& s, const std::string& loc, const std::string& hint)
            {
                auto r = typed{make(tk::map, {make(tk::any)}), {}};
                if (s.has("patternProperties"))
                    warn(loc, "`patternProperties` is not supported; the values are read as any JSON value");
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
                        const auto base = sanitize(f.key);
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
                if (t.k == tk::optional) t = t.args[0];
                if (is_nullable(t)) t = t.args[1];
                // only std::optional is left out when empty: an empty std::unique_ptr is written as null
                f.type = make(tk::pointer, {std::move(t)});
                if (!f.required) f.type = make(tk::optional, {std::move(f.type)});
                f.init.clear();
                f.omit_default = false;
            }

            // breaks cycles of structs held by value with std::unique_ptr; returns the structs in definition order
            std::vector<int> order_structs()
            {
                auto state = std::vector<int>(decls_.size(), 0);
                auto order = std::vector<int>();
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
                    order.push_back(i);
                };
                for (std::size_t i = 0; i < decls_.size(); ++i)
                    if (decls_[i].kind == decl::kind_t::structure && state[i] == 0) visit(visit, static_cast<int>(i));
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

            bool equality(const ctype& t) const
            {
                switch (t.k)
                {
                    case tk::any:
                    case tk::object:
                    case tk::pointer:
                        return false;
                    case tk::named:
                        if (decls_[t.decl].kind == decl::kind_t::alias) return equality(decls_[t.decl].target);
                        return decls_[t.decl].kind == decl::kind_t::enumeration || decls_[t.decl].eq;
                    default:
                        return std::ranges::all_of(t.args, [&](const auto& a) { return equality(a); });
                }
            }

            void compute_equality()
            {
                for (auto& d : decls_) d.eq = d.kind == decl::kind_t::structure;
                for (auto changed = true; changed;)
                {
                    changed = false;
                    for (auto& d : decls_)
                        if (d.eq && std::ranges::any_of(d.fields, [&](const auto& f) { return !equality(f.type); }))
                        {
                            d.eq = false;
                            changed = true;
                        }
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

            std::vector<std::string> render_settings(const settings& s, const ctype& target)
            {
                auto out = s.constraints;
                if (s.min_items) out.push_back("yyjson::min_items(" + std::to_string(*s.min_items) + ")");
                if (s.max_items) out.push_back("yyjson::max_items(" + std::to_string(*s.max_items) + ")");
                const auto& v = value_type(target);
                const auto elem = [&]() -> ctype {
                    return v.k == tk::vector || v.k == tk::map ? v.args[0] : make(tk::any);
                };
                if (s.unique)
                {
                    if (equality(elem()))
                        out.emplace_back("yyjson::unique_items");
                    else
                        warn(s.unique_loc,
                             "`uniqueItems` needs operator== of the element type " + spell(elem()) + "; it is dropped");
                }
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
                    for (const auto& e : m.examples) ex.push_back(raw_string(e));
                    out.push_back("yyjson::examples(" + join(ex) + ")");
                }
                return out;
            }

            static void render_comment(std::string& out, const metadata& m, std::string_view indent)
            {
                if (m.description)
                {
                    auto text = std::string_view(*m.description);
                    while (!text.empty())
                    {
                        const auto nl = std::min(text.find('\n'), text.size());
                        auto line = text.substr(0, nl);
                        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.remove_suffix(1);
                        out += std::string(indent) + (line.empty() ? "///" : "/// " + std::string(line)) + "\n";
                        text.remove_prefix(std::min(nl + 1, text.size()));
                    }
                }
                if (m.comment)
                {
                    auto text = std::string_view(*m.comment);
                    while (!text.empty())
                    {
                        const auto nl = std::min(text.find('\n'), text.size());
                        out += std::string(indent) + "// " + std::string(text.substr(0, nl)) + "\n";
                        text.remove_prefix(std::min(nl + 1, text.size()));
                    }
                }
            }

            std::vector<std::string> field_items(const field& f)
            {
                auto items = std::vector<std::string>();
                if (f.member != f.key) items.push_back("yyjson::rename(" + cpp_string(f.key) + ")");
                if (f.required) items.emplace_back("yyjson::required");
                if (f.omit_default) items.emplace_back("yyjson::omit_default");
                for (auto& s : render_settings(f.set, f.type)) items.push_back(std::move(s));
                for (auto& s : render_metadata(f.meta)) items.push_back(std::move(s));
                return items;
            }

            std::vector<std::string> struct_items(const decl& d) const
            {
                auto items = std::vector<std::string>();
                if (!d.schema_name.empty()) items.push_back("yyjson::schema_name(" + cpp_string(d.schema_name) + ")");
                for (auto& s : render_metadata(d.meta)) items.push_back(std::move(s));
                // explicit, so that a read under yyjson::strict accepts the keys that the schema accepts
                items.emplace_back(d.additional ? "yyjson::additional_properties(true)"
                                                : "yyjson::additional_properties(false)");
                const auto member = [&](std::string_view key) {
                    for (const auto& f : d.fields)
                        if (f.key == key) return f.member;
                    return std::string(key);
                };
                for (const auto& [k, keys] : d.dependent)
                {
                    auto ms = std::vector<std::string>();
                    for (const auto& x : keys) ms.push_back(cpp_string(member(x)));
                    items.push_back("yyjson::dependent_required(" + cpp_string(member(k)) + ", {" + join(ms) + "})");
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
                name_members();
                const auto structs = order_structs();
                const auto aliases = order_aliases();
                compute_equality();
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
                    for (const auto& [m, n] : d.enumerators) body += "    " + m + ",\n";
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

                for (const auto i : aliases)
                {
                    const auto& d = decls_[i];
                    render_comment(body, d.meta, "");
                    body += "using " + d.name + " = " + spell(d.target) + ";\n\n";
                }

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
                    if (d.eq)
                    {
                        if (!d.fields.empty()) body += "\n";
                        body += "    bool operator==(const " + d.name + "&) const = default;\n";
                    }
                    body += "};\n\n";
                    if (!opts_.cpp26)
                    {
                        auto all = items;
                        for (auto& s : field_specs) all.push_back(std::move(s));
                        if (!all.empty()) after += "\n" + annotation_spec(q + d.name, all);
                    }
                }

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

                auto out = std::string("// Generated by cpp-yyjson-codegen");
                if (!opts_.source.empty()) out += " from " + opts_.source;
                out += ". Do not edit.\n";
                if (!warnings_.empty())
                {
                    out += "//\n// Warnings:\n";
                    for (const auto& w : warnings_) out += "//   " + w.location + ": " + w.message + "\n";
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
                if (any_field([](const field& f) { return !f.default_note.empty(); }))
                    out +=
                        "//\n// A std::optional field with a `default:` comment is std::nullopt when the key is "
                        "missing; the\n// program applies the default of the comment.\n";
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
                if (!opts_.ns.empty()) out += "namespace " + opts_.ns + "\n{\n";
                while (body.ends_with("\n\n")) body.pop_back();
                out += body;
                if (!opts_.ns.empty()) out += "}  // namespace " + opts_.ns + "\n";
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
                if (const auto* d = root_.find("$schema"); d != nullptr)
                {
                    const auto dialect = d->is_string() ? d->text : std::string();
                    if (dialect.find("draft/2020-12/schema") == std::string::npos)
                        warn("#", "the dialect \"" + dialect + "\" is read as JSON Schema 2020-12");
                }
                const auto root_typed =
                    root_.is_bool() || (!is_pure_ref(root_) && std::ranges::any_of(root_.members, [](const auto& m) {
                        static const auto only = std::set<std::string_view>{
                            "$schema",     "$id",         "$defs",   "definitions", "$comment",  "title",
                            "description", "$vocabulary", "$anchor", "examples",    "deprecated"};
                        return !only.contains(m.first);
                    }));
                auto root_name = std::string();
                if (root_typed)
                {
                    const auto* title = root_.find("title");
                    root_name = !opts_.root_name.empty() ? opts_.root_name
                                : title != nullptr && title->is_string() && !pascal(title->text).empty()
                                    ? sanitize(pascal(title->text))
                                    : std::string("Root");
                    type_names_.insert(root_name);
                    def_names_[&root_] = root_name;
                }
                auto defs = std::vector<std::pair<std::string, const json*>>();
                for (const auto* kw : {"$defs", "definitions"})
                    if (const auto* d = root_.find(kw); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                        {
                            def_names_[&v] = unique_type_name(k);
                            if (is_schema_name(k)) def_keys_[&v] = k;
                            defs.emplace_back(std::string("#/") + kw + "/" + pointer_token(k), &v);
                        }
                for (const auto& [loc, v] : defs) type_of(v, loc, def_names_.at(v));
                if (root_typed)
                {
                    type_of(&root_, "#", root_name);
                    if (const auto i = find_decl(root_name);
                        i >= 0 && decls_[i].kind == decl::kind_t::alias && !decls_[i].set.empty())
                        warn("#", "constraints of a root that is not an object cannot be attached to the alias " +
                                      root_name + "; they are dropped");
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
