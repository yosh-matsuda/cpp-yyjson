// cpp-yyjson-codegen: C++ types with cpp-yyjson annotations from a JSON Schema.
// This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <ranges>
#include <regex>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <cpp_yyjson.hpp>

#include "meta_schemas.hpp"
#include "reserved_names.hpp"

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
        // emit yyjson::pattern, yyjson::pattern_properties and yyjson::format("regex") (the program provides
        // yyjson::pattern_matcher<>)
        bool pattern = true;
        // a field with a default is std::optional<T>, std::nullopt for a missing key, instead of T with the default as
        // its initializer and yyjson::omit_default
        bool optional_defaults = false;
        // a struct that accepts any other key gets `std::map<std::string, yyjson::value> extra` with
        // yyjson::extra_keys, which keeps the keys that name no property
        bool keep_unknown_keys = false;
        // a schema without `type` whose keywords belong to one type is read as that type, with a warning; otherwise it
        // reads every JSON value and its keywords constrain only the values of their types, as JSON Schema says
        bool infer_types = false;
        // a variant of more alternatives than this, with std::monostate for null, is yyjson::value with a warning; 0 for
        // no limit. The compiler builds std::variant in time and memory that grow faster than the square of the
        // number of its alternatives
        std::size_t max_alternatives = 0;
        std::string root_name{};  // name of the root type; default: the title, or "Root"
        std::string source{};     // name of the schema in the header comment
        // path of the schema; a reference to another file, such as "address.json#/$defs/Street", is resolved against
        // its directory
        std::string path{};
        // reads the file of a path made from `path` and a reference, or returns std::nullopt; without it, references
        // to other files are read as any JSON value
        std::function<std::optional<std::string>(const std::string&)> load{};
        // more schemas, read through `load` before the references: a reference to the `$id` of one of them, or to its
        // path, is resolved in it
        std::vector<std::string> schemas{};
        // (prefix, directory): a reference to a URI that starts with the prefix, and is not the `$id` of a schema
        // read, is read from the directory, with the rest of the URI as a relative path
        std::vector<std::pair<std::string, std::string>> uri_map{};
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

        // The positive number of the JSON text `text` as m * 10^e, or nullopt for another text or one that m does
        // not hold.
        inline std::optional<std::pair<std::uint64_t, int>> decimal_of(std::string_view text)
        {
            const auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
            auto digits = std::string();
            auto e = 0;
            auto i = std::size_t();
            for (; i < text.size() && is_digit(text[i]); ++i) digits += text[i];
            if (i < text.size() && text[i] == '.')
                for (++i; i < text.size() && is_digit(text[i]); ++i, --e) digits += text[i];
            if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
            {
                if (++i < text.size() && text[i] == '+') ++i;
                auto x = 0;
                const auto* end = text.data() + text.size();
                if (const auto [p, ec] = std::from_chars(text.data() + i, end, x); ec != std::errc() || p != end)
                    return std::nullopt;
                if (x < -1000 || x > 1000) return std::nullopt;
                e += x;
                i = text.size();
            }
            if (i != text.size()) return std::nullopt;
            for (; !digits.empty() && digits.back() == '0'; digits.pop_back()) ++e;
            const auto first = digits.find_first_not_of('0');
            if (first == std::string::npos) return std::nullopt;
            auto m = std::uint64_t();
            const auto* end = digits.data() + digits.size();
            if (const auto [p, ec] = std::from_chars(digits.data() + first, end, m); ec != std::errc() || p != end)
                return std::nullopt;
            return std::pair{m, e};
        }

        // The least common multiple of the positive numbers `a` and `b`, whose multiples are the numbers that are
        // multiples of both, or nullopt when 64 bits do not hold it.
        inline std::optional<json> decimal_lcm(const json& a, const json& b)
        {
            const auto x = decimal_of(a.text), y = decimal_of(b.text);
            if (!x || !y) return std::nullopt;
            const auto least = std::min(x->second, y->second);
            const auto scale = [&](std::uint64_t m, int n) -> std::optional<std::uint64_t> {
                for (; n > least; --n)
                {
                    if (m > std::numeric_limits<std::uint64_t>::max() / 10) return std::nullopt;
                    m *= 10;
                }
                return m;
            };
            const auto p = scale(x->first, x->second), q = scale(y->first, y->second);
            if (!p || !q) return std::nullopt;
            const auto k = *p / std::gcd(*p, *q);
            if (k > std::numeric_limits<std::uint64_t>::max() / *q) return std::nullopt;
            auto l = k * *q;
            auto e = least;
            for (; l % 10 == 0; l /= 10) ++e;
            auto digits = std::to_string(l);
            auto r = json();
            r.k = json::kind::number;
            if (e >= 0 && e <= 19 - static_cast<int>(digits.size()))
            {
                r.integer = true;
                r.text = digits + std::string(static_cast<std::size_t>(e), '0');
            }
            else if (e < 0)
            {
                const auto n = static_cast<std::size_t>(-e);
                if (digits.size() <= n) digits.insert(0, n + 1 - digits.size(), '0');
                r.text = digits.insert(digits.size() - n, ".");
            }
            else
                r.text = digits + "e" + std::to_string(e);
            r.num = std::strtod(r.text.c_str(), nullptr);
            return r;
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
        // with a backslash, or with the trigraph `??/` of a backslash, to the next one, so the lines are split at CR
        // and LF and lose their trailing backslashes and the slash of a trailing `??/`
        inline std::vector<std::string_view> comment_lines(std::string_view text)
        {
            auto lines = std::vector<std::string_view>();
            while (!text.empty())
            {
                const auto nl = std::min(text.find_first_of("\r\n"), text.size());
                auto line = text.substr(0, nl);
                while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\\' ||
                                         line.ends_with("?\?/")))
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

        // a string literal; a raw string literal when the text has quotes, backslashes or line feeds. In an ordinary
        // literal, a `?` after a `?` is escaped, so that no trigraph appears, which -Wtrigraphs warns about
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
                else if (c == '?' && out.ends_with('?'))
                    out += "\\?";
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

        inline std::set<std::string_view> split_names(std::string_view text)
        {
            auto out = std::set<std::string_view>();
            for (const auto w : text | std::views::split(' '))
                if (!w.empty()) out.emplace(w.begin(), w.end());
            return out;
        }

        // a keyword, or a macro of the standard libraries that would replace the name
        inline bool is_reserved(std::string_view s)
        {
            static const auto macros = split_names(reserved_macro_names);
            return is_keyword(s) || macros.contains(s);
        }

        // a name of the global namespace, which a type defined there cannot take
        inline bool is_global_name(std::string_view s)
        {
            static const auto names = split_names(reserved_global_names);
            return names.contains(s);
        }

        // `base` with a number, without the `__` that C++ reserves: `a_2`, `a_` -> `a_2`
        inline std::string numbered(std::string_view base, int n)
        {
            return std::string(base) + (base.ends_with('_') ? "" : "_") + std::to_string(n);
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
            return !is_reserved(s);
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
            if (is_reserved(out)) out += '_';
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

        // "Start Time" and "startTime" -> "start_time": the name of a member from a title
        inline std::string snake(std::string_view s)
        {
            auto out = std::string();
            for (std::size_t i = 0; i < s.size(); ++i)
            {
                const auto c = s[i];
                if (!is_alnum(c))
                {
                    if (!out.empty() && out.back() != '_') out += '_';
                    continue;
                }
                const auto upper = c >= 'A' && c <= 'Z';
                if (upper && !out.empty() && out.back() != '_' &&
                    ((s[i - 1] >= 'a' && s[i - 1] <= 'z') || is_digit(s[i - 1]) ||
                     (i + 1 < s.size() && s[i + 1] >= 'a' && s[i + 1] <= 'z' && s[i - 1] >= 'A' && s[i - 1] <= 'Z')))
                    out += '_';
                out += upper ? static_cast<char>(c - 'A' + 'a') : c;
            }
            while (!out.empty() && out.back() == '_') out.pop_back();
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
            indirect  // yyjson::indirect, or std::indirect with --cpp26
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
                // yyjson::const_ / yyjson::enum_ that lists nullptr
                bool null = false;
            };
            std::vector<constraint> constraints;
            std::optional<std::uint64_t> min_items, max_items;
            bool unique = false;
            std::vector<settings> items, contains, property_names, additional;  // zero or one
            std::vector<settings> prefix;
            std::optional<std::uint64_t> min_contains, max_contains;
            // the level of `contains` over elements of yyjson::value or yyjson::object: yyjson::is_type<T>, an index
            // into the types of the generator, or -1
            int type_test = -1;
            // yyjson::not_(...) of the value: the value is rejected when every setting of an entry holds
            std::vector<settings> negated;
            // yyjson::if_then_else(...) of the value, three entries for each: the settings of yyjson::if_, those of
            // yyjson::then and those of yyjson::else_, a part without settings left out
            std::vector<settings> conditional;

            [[nodiscard]] bool empty() const
            {
                return constraints.empty() && !min_items && !max_items && !unique && items.empty() &&
                       contains.empty() && property_names.empty() && additional.empty() && prefix.empty() &&
                       type_test < 0 && negated.empty() && conditional.empty();
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
            bool tagged = false;     // a variant with internal_tag
            bool reordered = false;  // a variant whose first alternative is not the first of the schema
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
            // yyjson::extra_keys: a map of the keys that name no property; no key. In a struct of yyjson::as_array,
            // yyjson::extra_items: a vector of the elements after the positions
            bool extra = false;
            // with `extra`, a map of yyjson::pattern_properties: the keys that name no property and match the pattern
            std::optional<std::string> pattern{};
            std::string init{};          // " = x" or "{...}"
            std::string default_note{};  // the default of a std::optional field, for a comment
            std::string default_from{};  // the function of yyjson::default_from that makes the default
            std::optional<json> dflt{};  // the default before render() converts it to `init`
            const json* schema = nullptr;  // a key that no property declares: the schema that gives its type
            // a property that is only a `$ref`: its target, which a subschema of `if`, `then` or `else` that refers
            // to it again does not narrow
            const json* ref = nullptr;
            // the normalized schema of its property, which a subschema of `then` or `else` that repeats it does not
            // narrow
            const json* prop = nullptr;
            metadata meta{};
            std::string loc{};
        };

        // `oneOf` (exactly_one) or `anyOf` of subschemas that only require keys: the groups of keys
        struct required_groups
        {
            bool exactly_one = false;
            std::vector<std::vector<std::string>> groups{};
        };

        // a field of yyjson::if_, yyjson::then, yyjson::else_ or yyjson::not_: the key of the field of the struct,
        // with settings, yyjson::required (presence) and yyjson::is_type<T>. With `counts`, it is not a field but
        // yyjson::min_properties / yyjson::max_properties of the part, from min_items / max_items of `set`
        struct cond_field
        {
            std::string key{};
            settings set{};
            bool required = false;
            std::optional<ctype> is_type{};
            bool counts = false;
        };

        // yyjson::if_then_else, or yyjson::not_ of the fields of `if_` when `negated`; `then_closed` and
        // `else_closed` add yyjson::additional_properties(false) to the part
        struct condition
        {
            bool negated = false;
            std::vector<cond_field> if_{}, then_{}, else_{};
            bool then_closed = false, else_closed = false;
            std::string loc{};
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
            std::vector<condition> conditions{};
            // yyjson::min_properties / yyjson::max_properties: the number of the keys of the object
            std::optional<std::uint64_t> min_properties{}, max_properties{};
            bool filled = false;
            bool transparent = false;  // yyjson::transparent: read and written as the value of its only field
            // yyjson::as_array: read and written as a JSON array whose positions are the fields; `additional` is
            // yyjson::items(bool) then
            bool as_array = false;
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

        // whether the type names indirect, outside the types it names
        inline bool holds_indirect(const ctype& t)
        {
            return t.k == tk::indirect || std::ranges::any_of(t.args, [](const ctype& a) { return holds_indirect(a); });
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
        inline constexpr auto ignored_keys = std::array<std::string_view, 2>{"$dynamicRef", "$recursiveRef"};
        inline bool is_ignored_key(std::string_view k)
        {
            return std::ranges::find(ignored_keys, k) != ignored_keys.end();
        }

        // the keywords that condition the keys of an object, which normalize() collects into "$conditions" and a
        // struct takes as yyjson::if_then_else and yyjson::not_
        inline bool is_condition_key(std::string_view k)
        {
            return k == "if" || k == "then" || k == "else" || k == "not" || k == "dependentSchemas" ||
                   k == "$conditions";
        }

        // the keywords that apply only to objects
        inline bool is_object_key(std::string_view k)
        {
            return k == "properties" || k == "required" || k == "additionalProperties" || k == "patternProperties" ||
                   k == "minProperties" || k == "maxProperties" || k == "propertyNames" || k == "dependentRequired" ||
                   k == "unevaluatedProperties";
        }

        // the keywords that apply only to strings, numbers or arrays, which every object satisfies
        inline bool is_non_object_key(std::string_view k)
        {
            static const auto keys = std::set<std::string_view>{
                "minLength",   "maxLength",   "pattern",          "format",           "minimum",
                "maximum",     "exclusiveMinimum", "exclusiveMaximum", "multipleOf",  "items",
                "prefixItems", "additionalItems",  "contains",         "minContains", "maxContains",
                "minItems",    "maxItems",    "uniqueItems",      "unevaluatedItems"};
            return keys.contains(k);
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
                "oneOf",          "unevaluatedProperties", "unevaluatedItems"};
            return keys.contains(k);
        }

        // a keyword of JSON Schema; validators ignore the other keys of a schema, such as vendor keys (x-*, _metadata)
        inline bool is_keyword_key(std::string_view k)
        {
            return is_type_key(k) || is_condition_key(k) || is_ignored_key(k) || is_metadata_key(k);
        }

        // whether the schemas `a` and `b` accept the same values because they differ only in metadata and vendor keys
        // at the top
        inline bool same_assertions(const json& a, const json& b)
        {
            if (!a.is_object() || !b.is_object()) return equal(a, b);
            const auto asserts = [](const std::string& k) {
                return !is_metadata_key(k) && (is_keyword_key(k) || k.starts_with('$'));
            };
            const auto covered = [&](const json& x, const json& y) {
                return std::ranges::all_of(x.members, [&](const auto& m) {
                    if (!asserts(m.first)) return true;
                    const auto* w = y.find(m.first);
                    return w != nullptr && equal(m.second, *w);
                });
            };
            return covered(a, b) && covered(b, a);
        }

        // a schema that gives the C++ type nothing: true, or no keyword the generator builds a type from
        inline bool adds_nothing(const json& s)
        {
            if (s.is_bool()) return s.b;
            return s.is_object() && std::ranges::none_of(s.members, [](const auto& m) {
                       return is_type_key(m.first) || is_condition_key(m.first);
                   });
        }

        // a reference without keywords beside it that the generator would merge into the referred schema
        inline bool is_pure_ref(const json& s)
        {
            return s.is_object() && s.has("$ref") && std::ranges::all_of(s.members, [](const auto& m) {
                       return m.first == "$ref" ||
                              !(is_type_key(m.first) || is_ignored_key(m.first) || is_condition_key(m.first));
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
            std::map<std::string, const json*> merged_;     // text without `$loc` -> schema in store_
            std::deque<json> files_;                        // schemas of other files
            // schema resources: the base URI of a `$id`, the path of a file, or "" for the root -> the schema
            std::map<std::string, const json*> resources_;
            std::set<const json*> resource_roots_;  // schemas that are resources, except the root
            std::map<std::pair<const json*, std::string>, const json*> anchors_;  // (resource, name) -> schema
            std::map<const json*, std::string> def_entries_;  // $defs entries of resources other than the root -> key
            // files read, the schema included, with an absolute base URI: a URI beside that one names a file beside
            std::vector<std::pair<std::string, std::string>> uri_files_;  // (path, base URI)
            std::string root_path_;                                       // lexically normal path of the schema
            std::set<std::string> schema_names_;            // schema_name of $defs entries
            std::deque<decl> decls_;
            std::map<const json*, std::string> def_names_;  // $defs entries -> type name
            std::map<const json*, std::string> def_keys_;   // $defs entries -> schema_name
            std::map<const json*, typed> cache_;
            std::map<const json*, std::string> busy_;  // schemas whose types are being made -> the names they take
            // schemas that hold themselves without an object between -> their transparent structs, which are filled
            // once the schema has its type
            std::map<const json*, int> self_structs_;
            mutable std::vector<int> bound_seen_;  // transparent structs that bound_of is in
            std::map<const json*, const json*> normalized_;
            std::set<const json*> normalizing_;
            std::map<const json*, bool> no_choice_;  // schemas whose subschemas of anyOf / oneOf all accept no value
            std::set<const json*> choosing_;
            // while no_choice() normalizes subschemas, warnings are not given and the schemas normalized are not
            // kept, so that the warnings come later from the place where the schema is read, as before
            int quiet_ = 0;
            std::vector<const json*> quiet_normalized_;
            std::set<const json*> conditions_used_;  // normalized schemas whose "$conditions" a struct took
            int folding_ = 0;                        // the depth of fold_if() in type_kind()
            std::vector<variant_note> notes_;
            std::set<std::string> type_names_;
            std::vector<int> position_;  // the place of a struct in definition order, -1 for others
            mutable bool partial_init_ = false;  // a designated initializer leaves members out
            mutable std::map<std::string, std::optional<std::regex>, std::less<>> regexes_;  // nullopt: not read
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
            bool uses_regex_format_ = false;
            bool widened_variants_ = false;  // a variant became yyjson::value under options::max_alternatives
            std::vector<diagnostic> warnings_;
            std::set<std::pair<std::string, std::string>> warned_;
            // the calls of warn(), repeated ones too, by which a caller tells that the type of a schema may read more
            // than the schema allows; type_of() counts a call again for a schema typed so before (`loose_`)
            std::size_t warn_calls_ = 0;
            std::set<const json*> loose_;
            std::vector<ctype> type_tests_;       // the types of yyjson::is_type<T>: settings::type_test
            std::set<const json*> handled_nots_;  // the value-level `not` that type_body() took or warned about
            std::set<const json*> handled_ifs_;   // the value-level `if` that type_body() took or warned about
            std::set<const json*> reported_false_;  // schemas whose normalization to `false` was warned about
            // merged schemas whose `contains` accepts no value: {the schema read with `contains` false, the copy}
            std::map<const json*, std::pair<const json*, const json*>> contains_nothing_;
            std::set<const json*> containing_;  // merged schemas whose `contains` is being normalized

            // the key that a warning shows for the URI of a document: "" for the root, the path for a file; for the
            // key of a copy made for `$dynamicRef`, the location of the schema copied, which replaces the '#' too
            std::map<std::string, std::string> loc_keys_;

            // `$dynamicRef` and `$recursiveRef` (see specialize()): the generator runs again in the dynamic mode when
            // it finds one, so that a schema without them is read as before
            bool dynamic_ = false;
            bool dynamic_found_ = false;
            using binding = std::map<std::string, const json*>;  // dynamic anchor name -> the resource that binds it
            std::map<const json*, const json*> resource_of_;      // schema -> its resource
            std::map<const json*, std::string> resource_keys_;    // resource -> the key of a reference to it
            std::map<const json*, std::string> loc_of_;           // schema -> its location
            // (resource, name) -> the schema of `$dynamicAnchor`, or of `$recursiveAnchor: true` under recursive_name
            std::map<std::pair<const json*, std::string>, const json*> dyn_anchors_;
            std::map<const json*, std::vector<std::string>> dyn_names_;  // resource -> the names of its anchors
            std::map<const json*, std::set<std::string>> reach_;  // see specialize()
            std::map<std::pair<const json*, binding>, const json*> clones_;
            std::map<const json*, std::string> clone_keys_;  // copy -> its key in resources_
            std::set<const json*> renamed_;                  // schemas whose name a copy took
            const json* root_copy_ = nullptr;
            static constexpr std::string_view recursive_name = "\x01" "rec";

            void warn(std::string loc, std::string msg)
            {
                if (quiet_ > 0) return;
                ++warn_calls_;
                if (const auto it = loc_keys_.find(loc.substr(0, loc.find('#'))); it != loc_keys_.end())
                    loc.replace(0,
                                it->first.size() +
                                    (it->second.find('#') != std::string::npos && loc.size() > it->first.size()),
                                it->second);
                if (warned_.emplace(loc, msg).second) warnings_.push_back({loc, std::move(msg)});
            }

            // whether a variant of `n` alternatives is more than options::max_alternatives, which makes it yyjson::value
            bool too_many_alternatives(std::size_t n, const std::string& loc)
            {
                if (opts_.max_alternatives == 0 || n <= opts_.max_alternatives) return false;
                warn(loc, "a variant of " + std::to_string(n) + " alternatives is more than " +
                              std::to_string(opts_.max_alternatives) +
                              " (--max-alternatives); it is yyjson::value, which reads any JSON value without the "
                              "checks of the alternatives");
                widened_variants_ = true;
                return true;
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

            // a schema built from an alternative and the keywords beside it, the same one each time the same
            // alternative and keywords meet: a recursive type reached again through them is then seen as busy
            static void strip_locs(json& x)
            {
                std::erase_if(x.members, [](const auto& m) { return m.first == "$loc"; });
                for (auto& [k, v] : x.members) strip_locs(v);
                for (auto& v : x.items) strip_locs(v);
            }

            const json* store_merged(json j)
            {
                auto key = j;
                strip_locs(key);
                auto text = dump(key);
                if (const auto it = merged_.find(text); it != merged_.end()) return it->second;
                return merged_[std::move(text)] = store(std::move(j));
            }

            // a scheme of one letter is a drive of Windows: "C:/schemas/a.json" is a file
            static bool is_absolute_uri(std::string_view r)
            {
                const auto colon = r.find(':');
                if (colon == std::string_view::npos || colon < 2 || !std::isalpha(static_cast<unsigned char>(r[0])))
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

            // the parts of a URI without its fragment (RFC 3986)
            struct uri_parts
            {
                std::string scheme;
                std::optional<std::string> authority;
                std::string path;
                std::optional<std::string> query;
            };

            static uri_parts parse_uri(std::string_view u)
            {
                auto p = uri_parts();
                if (is_absolute_uri(u))
                {
                    const auto colon = u.find(':');
                    p.scheme = u.substr(0, colon);
                    u.remove_prefix(colon + 1);
                }
                if (u.starts_with("//"))
                {
                    u.remove_prefix(2);
                    const auto end = std::min(u.find_first_of("/?"), u.size());
                    p.authority = std::string(u.substr(0, end));
                    u.remove_prefix(end);
                }
                if (const auto q = u.find('?'); q != std::string_view::npos)
                {
                    p.query = std::string(u.substr(q + 1));
                    u = u.substr(0, q);
                }
                p.path = u;
                return p;
            }

            static std::string remove_dot_segments(std::string_view in)
            {
                auto out = std::string();
                const auto pop = [&] {
                    const auto slash = out.rfind('/');
                    out.erase(slash == std::string::npos ? 0 : slash);
                };
                while (!in.empty())
                {
                    if (in.starts_with("../"))
                        in.remove_prefix(3);
                    else if (in.starts_with("./") || in.starts_with("/./"))
                        in.remove_prefix(2);
                    else if (in == "/.")
                        in = "/";
                    else if (in.starts_with("/../"))
                    {
                        in.remove_prefix(3);
                        pop();
                    }
                    else if (in == "/..")
                    {
                        in = "/";
                        pop();
                    }
                    else if (in == "." || in == "..")
                        in = {};
                    else
                    {
                        const auto end = std::min(in.find('/', in[0] == '/' ? 1 : 0), in.size());
                        out += in.substr(0, end);
                        in.remove_prefix(end);
                    }
                }
                return out;
            }

            static std::string compose_uri(const uri_parts& p)
            {
                auto u = p.scheme.empty() ? std::string() : p.scheme + ":";
                if (p.authority) u += "//" + *p.authority;
                u += p.path;
                if (p.query) u += "?" + *p.query;
                return u;
            }

            // a reference without its fragment, resolved against a base URI; a URN has no hierarchy, so only a
            // reference to its fragment stays in it
            static std::string resolve_uri(std::string_view base, std::string_view r)
            {
                auto t = parse_uri(r);
                if (!t.scheme.empty() || t.authority)
                {
                    if (t.scheme.empty()) t.scheme = parse_uri(base).scheme;
                    t.path = remove_dot_segments(t.path);
                    return compose_uri(t);
                }
                const auto b = parse_uri(base);
                t.scheme = b.scheme;
                t.authority = b.authority;
                if (t.path.empty())
                {
                    t.path = b.path;
                    if (!t.query) t.query = b.query;
                }
                else if (t.path[0] == '/')
                    t.path = remove_dot_segments(t.path);
                else
                {
                    const auto slash = b.path.rfind('/');
                    t.path = remove_dot_segments(b.authority && b.path.empty()   ? "/" + t.path
                                                 : slash == std::string::npos ? t.path
                                                                              : b.path.substr(0, slash + 1) + t.path);
                }
                return compose_uri(t);
            }

            // the key of a reference without its fragment (not empty) from the base of the schema that holds it: a
            // URI, or a path when neither is a URI, which is "" for the schema itself
            std::string resolve_key(const std::string& base, std::string_view r) const
            {
                if (is_absolute_uri(r) || is_absolute_uri(base)) return resolve_uri(base, r);
                const auto dir = std::filesystem::path(base.empty() ? root_path_ : base).parent_path().generic_string();
                auto target = file_key(dir, r);
                return target == root_path_ ? std::string() : target;
            }

            // a reference as "key#fragment", the key a URI, a path or "" (see resolve_key); `resolve` splits at the
            // last '#', since a path may hold one, and the fragment holds none
            std::string canonical_ref(std::string_view r, const std::string& base) const
            {
                const auto hash = r.find('#');
                const auto part = r.substr(0, hash);
                auto fragment = std::string();
                if (hash != std::string_view::npos)
                {
                    for (const auto c : r.substr(hash + 1))
                        fragment += c == '#' ? std::string_view("%23") : std::string_view(&c, 1);
                }
                return (part.empty() ? base : resolve_key(base, part)) + "#" + fragment;
            }

            static bool is_subschema_key(std::string_view k)
            {
                static constexpr auto keys = std::array<std::string_view, 22>{
                    "allOf",       "anyOf",           "oneOf",         "not",          "if",
                    "then",        "else",            "prefixItems",   "items",        "additionalItems",
                    "contains",    "properties",      "patternProperties", "additionalProperties",
                    "propertyNames", "unevaluatedItems", "unevaluatedProperties", "dependentSchemas",
                    "dependencies", "$defs",          "definitions",   "contentSchema"};
                return std::ranges::find(keys, k) != keys.end();
            }

            // the base of a schema whose own base is `base`
            std::string base_of(const json& j, const std::string& base) const
            {
                const auto* id = j.find("$id");
                if (id == nullptr || !id->is_string()) return base;
                const auto part = std::string_view(id->text).substr(0, id->text.find('#'));
                return part.empty() ? base : resolve_key(base, part);
            }

            // registers the resources ($id) and anchors of a file and rewrites its references to their canonical form;
            // drops the keywords beside a reference of draft-07, which ignores them, and the keywords that draft-07 does
            // not have. `names` tells that `j` maps names to schemas, as `properties` does; `known` that `j` is a
            // schema under the keywords of schemas, where an `$id` or an anchor counts
            void prepare(json& j, std::string base, const json* resource, bool draft07, const std::string& loc,
                         bool names = false, bool known = true)
            {
                if (j.is_object())
                {
                    if (!names)
                    {
                        if (draft07)
                            std::erase_if(j.members, [&](const auto& m) {
                                if (m.first != "unevaluatedProperties" && m.first != "unevaluatedItems") return false;
                                warn(loc, "draft-07 has no `" + m.first + "`; it is ignored");
                                return true;
                            });
                        // 2020-12 takes the `$id` of a schema as the base of its `$ref`; draft-07 ignores an `$id`
                        // beside `$ref`, and names a schema by the fragment of its `$id`
                        if (const auto* id = j.find("$id");
                            known && id != nullptr && id->is_string() && !(draft07 && j.has("$ref")))
                        {
                            const auto hash = id->text.find('#');
                            if (!id->text.empty() && hash != 0)
                            {
                                base = base_of(j, base);
                                resource = &j;
                                resources_.emplace(base, &j);
                                if (&j != &root_) resource_roots_.insert(&j);
                            }
                            if (hash != std::string::npos && hash + 1 < id->text.size() && id->text[hash + 1] != '/')
                                anchors_.emplace(std::pair(resource, id->text.substr(hash + 1)), &j);
                        }
                        if (known)
                            for (const auto* kw : {"$anchor", "$dynamicAnchor"})
                                if (const auto* a = j.find(kw); a != nullptr && a->is_string())
                                    anchors_.emplace(std::pair(resource, a->text), &j);
                        if (known && !draft07)
                            for (const auto* kw : {"$dynamicRef", "$recursiveRef"})
                                if (auto* r = const_cast<json*>(j.find(kw)); r != nullptr && r->is_string())
                                {
                                    dynamic_found_ = true;
                                    if (dynamic_) r->text = canonical_ref(r->text, base);
                                }
                        if (known && dynamic_)
                        {
                            resource_of_.emplace(&j, resource);
                            loc_of_.emplace(&j, loc);
                            if (resource == &j) resource_keys_.emplace(&j, base);
                            const auto anchor = [&](std::string name) {
                                if (dyn_anchors_.emplace(std::pair(resource, name), &j).second)
                                    dyn_names_[resource].push_back(std::move(name));
                            };
                            if (const auto* a = j.find("$dynamicAnchor"); a != nullptr && a->is_string())
                                anchor(a->text);
                            if (const auto* a = j.find("$recursiveAnchor");
                                resource == &j && a != nullptr && a->is_bool() && a->b)
                                anchor(std::string(recursive_name));
                        }
                        if (auto* r = const_cast<json*>(j.find("$ref")); r != nullptr && r->is_string())
                        {
                            r->text = canonical_ref(r->text, base);
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
                        if (known && resource == &j && &j != &root_)
                            for (const auto* kw : {"$defs", "definitions"})
                                if (const auto* d = j.find(kw); d != nullptr && d->is_object())
                                    for (const auto& [k, v] : d->members) def_entries_.emplace(&v, k);
                    }
                    for (auto& [k, v] : j.members)
                    {
                        if (names)
                            prepare(v, base, resource, draft07, loc + "/" + pointer_token(k), false, known);
                        else if (k != "enum" && k != "const" && k != "default" && k != "examples")
                            prepare(v, base, resource, draft07, loc + "/" + pointer_token(k),
                                    k == "properties" || k == "patternProperties" || k == "$defs" ||
                                        k == "definitions" || k == "dependentSchemas" || k == "dependencies",
                                    known && is_subschema_key(k));
                    }
                }
                for (std::size_t i = 0; i < j.items.size(); ++i)
                    prepare(j.items[i], base, resource, draft07, loc + "/" + std::to_string(i), false, known);
            }

            // reads a file, registers it under its path and the URI it is read for, and prepares it; nullptr when it
            // cannot be read, or with the message of `error` when it is not JSON
            const json* load_file(const std::string& path, const std::string& uri, std::string& error)
            {
                auto text = opts_.load(path);
                if (!text) return nullptr;
                auto doc = json();
                try
                {
                    doc = to_json(yyjson::read(*text));
                }
                catch (const std::exception& e)
                {
                    error = e.what();
                    return nullptr;
                }
                const auto* d = &files_.emplace_back(std::move(doc));
                resources_.emplace(path, d);
                if (!uri.empty()) resources_.emplace(uri, d);
                resource_roots_.insert(d);
                check_dialect(*d, path + "#");
                const auto base = uri.empty() ? path : uri;
                const auto b = base_of(*d, base);
                if (is_absolute_uri(b)) uri_files_.emplace_back(path, b);
                for (const auto& k : {uri, b})
                    if (!k.empty() && k != path) loc_keys_.emplace(k, path);
                prepare(files_.back(), base, d, is_draft07(*d), path + "#");
                return d;
            }

            // a meta-schema of JSON Schema that the generator holds, read for its URI; nullptr when there is none
            const json* load_meta_schema(const std::string& uri)
            {
                const auto it = std::ranges::find(detail::meta_schemas, uri, &detail::meta_schema::uri);
                if (it == std::ranges::end(detail::meta_schemas)) return nullptr;
                const auto* d = &files_.emplace_back(to_json(yyjson::read(it->text)));
                resources_.emplace(uri, d);
                resource_roots_.insert(d);
                prepare(files_.back(), uri, d, is_draft07(*d), uri + "#");
                return d;
            }

            // the files that a URI may name: under a prefix of `uri_map` (the longest), or beside a file read whose
            // base URI has the same scheme and authority
            std::vector<std::string> uri_paths(const std::string& u) const
            {
                auto paths = std::vector<std::string>();
                const auto pu = parse_uri(u);
                const auto* best = static_cast<const std::pair<std::string, std::string>*>(nullptr);
                for (const auto& m : opts_.uri_map)
                    if (u.starts_with(m.first) && (best == nullptr || m.first.size() > best->first.size())) best = &m;
                if (best != nullptr)
                {
                    const auto rest = std::string_view(u).substr(best->first.size());
                    const auto path = rest.substr(0, rest.find('?'));
                    if (!path.empty()) paths.push_back(file_key(best->second, path));
                }
                for (const auto& [path, base] : uri_files_)
                {
                    const auto pb = parse_uri(base);
                    if (!pu.authority || pb.scheme != pu.scheme || pb.authority != pu.authority) continue;
                    const auto rel = std::filesystem::path(remove_dot_segments(pu.path))
                                         .lexically_relative(std::filesystem::path(pb.path).parent_path());
                    if (rel.empty() || rel == ".") continue;
                    auto p = file_key(std::filesystem::path(path).parent_path().generic_string(), rel.generic_string());
                    if (std::ranges::find(paths, p) == paths.end()) paths.push_back(std::move(p));
                }
                return paths;
            }

            // the schema resource of a key, read from its file when it is not read yet; nullptr with a warning, which
            // `report` false leaves out
            const json* document(const std::string& key, const std::string& r, const std::string& loc,
                                 bool report = true)
            {
                if (const auto it = resources_.find(key); it != resources_.end()) return it->second;
                auto error = std::string();
                const auto warn = [&](const std::string& l, std::string msg) {
                    if (report) this->warn(l, std::move(msg));
                };
                const auto unusable = [&](const std::string& path) {
                    if (!error.empty())
                        warn(loc, "the file \"" + path + "\" is not valid JSON (" + error +
                                      "); the reference is read as any JSON value");
                    else
                        warn(loc, "the file \"" + path + "\" of the reference \"" + r +
                                      "\" cannot be read; it is read as any JSON value");
                    return nullptr;
                };
                if (!is_absolute_uri(key))
                {
                    if (!opts_.load)
                    {
                        warn(loc, "the reference \"" + r + "\" is not in this document and is read as any JSON value");
                        return nullptr;
                    }
                    if (const auto* d = load_file(key, "", error)) return d;
                    return unusable(key);
                }
                const auto paths = opts_.load ? uri_paths(key) : std::vector<std::string>();
                for (const auto& p : paths)
                {
                    if (const auto it = resources_.find(p); it != resources_.end())
                    {
                        loc_keys_.emplace(key, it->second == &root_ ? std::string() : p);
                        return resources_.emplace(key, it->second).first->second;
                    }
                    if (const auto* d = load_file(p, key, error)) return d;
                    if (!error.empty()) return unusable(p);
                }
                if (const auto* d = load_meta_schema(key)) return d;
                if (!paths.empty()) return unusable(paths.front());
                warn(loc, "the reference \"" + r +
                              "\" is a URI that is not the `$id` of a schema read and is not mapped to a file; it is "
                              "read as any JSON value");
                return nullptr;
            }

            // a type name for a schema referred to: an entry of $defs of another resource, or another resource, by
            // its title or the name of its file
            void name_target(const json* t, const std::string& key)
            {
                if (t == &root_ || def_names_.contains(t)) return;
                if (const auto it = def_entries_.find(t); it != def_entries_.end())
                {
                    name_def(t, it->second);
                    return;
                }
                if (!resource_roots_.contains(t)) return;
                const auto* title = t->find("title");
                const auto path = is_absolute_uri(key) ? parse_uri(key).path : key;
                const auto stem = std::filesystem::path(path).filename().string();
                const auto titled = title != nullptr && title->is_string() && !pascal(title->text).empty();
                def_names_[t] = unique_type_name(titled ? pascal(title->text) : pascal(stem.substr(0, stem.find('.'))));
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
                const auto hash = ref.text.rfind('#');
                const auto key = ref.text.substr(0, hash);
                // the reference in messages, without the '#' of the canonical form of a whole file
                const auto r = hash + 1 == ref.text.size() && hash > 0 ? key : ref.text;
                const auto* doc = document(key, r, loc);
                if (doc == nullptr) return nullptr;
                const auto fragment = fragment_of(ref.text);
                const auto* cur = find_in(doc, fragment);
                if (cur == nullptr)
                {
                    warn(loc, (!fragment.empty() && fragment[0] != '/' ? "the anchor \"" : "the reference \"") + r +
                                  "\" is not found");
                    return nullptr;
                }
                name_target(cur, key);
                return cur;
            }

            static std::string fragment_of(std::string_view ref)
            {
                const auto hash = ref.rfind('#');
                return hash == std::string_view::npos ? std::string() : percent_decode(ref.substr(hash + 1));
            }

            // the schema of a fragment, an anchor or a JSON pointer, in a document; nullptr when there is none
            const json* find_in(const json* doc, const std::string& fragment) const
            {
                if (!fragment.empty() && fragment[0] != '/')
                {
                    const auto it = anchors_.find(std::pair(doc, fragment));
                    return it != anchors_.end() ? it->second : nullptr;
                }
                const auto* cur = doc;
                for (std::size_t pos = 1; pos <= fragment.size() && !fragment.empty() && cur != nullptr;)
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
                }
                return cur;
            }

            // the target of a reference in its canonical form, without warnings and without naming it
            const json* lookup(const std::string& ref)
            {
                const auto* doc = document(ref.substr(0, ref.rfind('#')), ref, "", false);
                return doc == nullptr ? nullptr : find_in(doc, fragment_of(ref));
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

            // `$dynamicRef` / `$recursiveRef`.  Their target depends on the resources that the evaluation passed
            // through (the dynamic scope), while the generator gives a schema one type.  So the schemas from which a
            // dynamic reference can be reached are copied once per binding of the anchors that they reach, with the
            // dynamic references of the copies rewritten to `$ref` of their target in that binding, and the copies
            // refer to each other by keys "dyn:N" of resources_.  A binding takes the outermost resource of the scope
            // that has an anchor of the name: the first resource entered.

            // calls f with every subschema of a schema, but those of `$defs` / `definitions`
            template <typename J, typename F>
            static void for_subschemas(J& j, F&& f)
            {
                if (!j.is_object()) return;
                for (auto& [k, v] : j.members)
                {
                    if (!is_subschema_key(k) || k == "$defs" || k == "definitions") continue;
                    if (k == "properties" || k == "patternProperties" || k == "dependentSchemas" || k == "dependencies")
                        for (auto& m : v.members) f(m.second);
                    else if (v.is_array())
                        for (auto& i : v.items) f(i);
                    else
                        f(v);
                }
            }

            static void strip_defs(json& j)
            {
                j.erase("$defs");
                j.erase("definitions");
                for_subschemas(j, [](json& c) { strip_defs(c); });
            }

            const json* resource_of(const json* s) const
            {
                const auto it = resource_of_.find(s);
                return it != resource_of_.end() ? it->second : nullptr;
            }

            binding enter(binding c, const json* r) const
            {
                if (const auto it = dyn_names_.find(r); it != dyn_names_.end())
                    for (const auto& n : it->second) c.try_emplace(n, r);
                return c;
            }

            // the static target of the reference of `kw` in a schema, and the name of its anchor when the reference
            // is dynamic: the target has the dynamic anchor of the fragment, or `$recursiveAnchor: true`
            struct dynamic_ref
            {
                const json* target = nullptr;
                std::string name;
                bool dynamic = false;
            };
            dynamic_ref dynamic_of(const json& j, std::string_view kw)
            {
                auto d = dynamic_ref();
                const auto* r = j.find(kw);
                if (r == nullptr || !r->is_string() || (d.target = lookup(r->text)) == nullptr) return d;
                d.name = kw == "$recursiveRef" ? std::string(recursive_name) : fragment_of(r->text);
                if (d.name.empty() || d.name[0] == '/') return d;
                const auto it = dyn_anchors_.find(std::pair(resource_of(d.target), d.name));
                d.dynamic = it != dyn_anchors_.end() && it->second == d.target;
                return d;
            }

            // reach_: for every schema reached from `roots`, the names of the dynamic anchors that its type depends
            // on, with "" for a schema from which a `$dynamicRef` / `$recursiveRef` is reached at all; reading the
            // files that the references name.  Returns the schemas reached from the first of `roots`.
            std::set<const json*> compute_reach(const std::vector<const json*>& roots)
            {
                auto preds = std::map<const json*, std::vector<const json*>>();
                auto seen = std::set<const json*>();
                auto stack = roots;
                auto dynamic = std::set<std::pair<const json*, std::string>>();  // (schema, name of its reference)
                auto linked = std::set<std::pair<const json*, const json*>>();
                const auto edge = [&](const json* from, const json* to) {
                    if (to != nullptr && linked.emplace(from, to).second)
                    {
                        preds[to].push_back(from);
                        stack.push_back(to);
                    }
                };
                for (auto changed = true; changed;)
                {
                    while (!stack.empty())
                    {
                        const auto* n = stack.back();
                        stack.pop_back();
                        if (!n->is_object() || !seen.insert(n).second) continue;
                        for_subschemas(*n, [&](const json& c) { edge(n, &c); });
                        if (const auto* r = n->find("$ref"); r != nullptr && r->is_string()) edge(n, lookup(r->text));
                        for (const auto* kw : {"$dynamicRef", "$recursiveRef"})
                        {
                            if (!n->has(kw)) continue;
                            reach_[n].insert("");
                            const auto d = dynamic_of(*n, kw);
                            edge(n, d.target);
                            if (!d.dynamic) continue;
                            reach_[n].insert(d.name);
                            dynamic.emplace(n, d.name);
                        }
                    }
                    // the anchors that a dynamic reference may resolve to, which may read more files
                    changed = false;
                    for (const auto& [n, name] : dynamic)
                        for (const auto& [key, a] : dyn_anchors_)
                            if (key.second == name && linked.emplace(n, a).second)
                            {
                                preds[a].push_back(n);
                                stack.push_back(a);
                                changed = true;
                            }
                }
                auto from_first = std::set<const json*>{roots.front()};
                for (auto todo = std::vector<const json*>{roots.front()}; !todo.empty();)
                {
                    const auto* n = todo.back();
                    todo.pop_back();
                    for (auto it = linked.lower_bound(std::pair(n, static_cast<const json*>(nullptr)));
                         it != linked.end() && it->first == n; ++it)
                        if (from_first.insert(it->second).second) todo.push_back(it->second);
                }
                auto work = std::vector<const json*>();
                for (const auto& [n, r] : reach_) work.push_back(n);
                while (!work.empty())
                {
                    const auto* n = work.back();
                    work.pop_back();
                    const auto it = preds.find(n);
                    if (it == preds.end()) continue;
                    const auto names = reach_[n];
                    for (const auto* p : it->second)
                    {
                        auto& r = reach_[p];
                        const auto size = r.size();
                        r.insert(names.begin(), names.end());
                        if (r.size() != size) work.push_back(p);
                    }
                }
                return from_first;
            }

            // the part of a type name that tells a resource that binds an anchor
            std::string resource_label(const json* r) const
            {
                if (r == &root_)
                {
                    if (const auto it = def_names_.find(r); it != def_names_.end()) return it->second;
                    if (!opts_.root_name.empty()) return opts_.root_name;
                }
                if (const auto it = def_entries_.find(r); it != def_entries_.end()) return pascal(it->second);
                for (const auto* kw : {"$defs", "definitions"})
                    if (const auto* d = root_.find(kw); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                            if (&v == r) return pascal(k);
                if (const auto* t = r->find("title"); t != nullptr && t->is_string() && !pascal(t->text).empty())
                    return pascal(t->text);
                const auto it = resource_keys_.find(r);
                const auto key = it != resource_keys_.end() ? it->second : std::string();
                const auto path = is_absolute_uri(key) ? parse_uri(key).path : key;
                const auto stem = std::filesystem::path(path).filename().string();
                const auto label = pascal(stem.substr(0, stem.find('.')));
                return !label.empty() || r != &root_ ? label : "Root";
            }

            // names a copy of a schema that has a name: the first copy whose anchors are bound by the root or by
            // the resource of the schema keeps the name, the others add "In" and the resources that bind their
            // anchors
            void name_copy(const json* s, const json* copy, const binding& c)
            {
                auto base = std::string();
                const auto def = def_entries_.find(s);
                if (const auto it = def_names_.find(s); it != def_names_.end())
                    base = it->second;
                else if (def != def_entries_.end())
                    base = def->second;
                else if (resource_roots_.contains(s))
                {
                    const auto* title = s->find("title");
                    base = title != nullptr && title->is_string() && !pascal(title->text).empty()
                               ? pascal(title->text)
                               : resource_label(s);
                }
                else
                    return;
                const auto labels_of = [&](bool all) {
                    auto labels = std::set<std::string>();
                    for (const auto& [n, r] : c)
                        if (all || (r != &root_ && r != resource_of(s)))
                            if (auto l = resource_label(r); !l.empty()) labels.insert(std::move(l));
                    return labels;
                };
                auto labels = labels_of(false);
                if (labels.empty() && renamed_.insert(s).second)
                {
                    if (const auto it = def_names_.find(s); it != def_names_.end())
                    {
                        def_names_[copy] = it->second;
                        if (const auto k = def_keys_.find(s); k != def_keys_.end()) def_keys_[copy] = k->second;
                    }
                    else if (def != def_entries_.end())
                        name_def(copy, def->second);
                    else
                        def_names_[copy] = unique_type_name(base);
                    return;
                }
                if (labels.empty()) labels = labels_of(true);
                auto suffix = std::string("In");
                for (const auto& l : labels) suffix += l;
                def_names_[copy] = unique_type_name(base + suffix);
            }

            // the schema for `s` in the binding `c`: `s` itself when no dynamic reference is reached from it
            const json* expand(const json* s, const binding& c)
            {
                const auto it = reach_.find(s);
                if (it == reach_.end() || it->second.empty()) return s;
                auto key = binding();
                for (const auto& [n, r] : c)
                    if (it->second.contains(n)) key.emplace(n, r);
                if (const auto m = clones_.find(std::pair(s, key)); m != clones_.end()) return m->second;
                auto& copy = store_.emplace_back(*s);
                strip_defs(copy);
                clones_.emplace(std::pair(s, key), &copy);
                const auto k = "dyn:" + std::to_string(clone_keys_.size());
                clone_keys_.emplace(&copy, k);
                resources_.emplace(k, &copy);
                if (const auto l = loc_of_.find(s); l != loc_of_.end()) loc_keys_.emplace(k, l->second);
                name_copy(s, &copy, key);
                rewrite(copy, *s, key);
                return &copy;
            }

            // rewrites a copy of `s` in the binding `c`: its subschemas, its `$ref`, and its dynamic references
            void rewrite(json& copy, const json& s, const binding& c)
            {
                auto cs = std::vector<json*>();
                for_subschemas(copy, [&](json& x) { cs.push_back(&x); });
                auto ss = std::vector<const json*>();
                for_subschemas(s, [&](const json& x) { ss.push_back(&x); });
                for (std::size_t i = 0; i < ss.size() && i < cs.size(); ++i)
                    if (const auto it = reach_.find(ss[i]); it != reach_.end() && !it->second.empty())
                        rewrite(*cs[i], *ss[i], resource_roots_.contains(ss[i]) ? enter(c, ss[i]) : c);
                const auto target = [&](const json* t, std::string text) {
                    const auto* t2 = expand(t, enter(c, resource_of(t)));
                    return t2 != t ? clone_keys_.at(t2) + "#" : text;
                };
                if (const auto* r = s.find("$ref"); r != nullptr && r->is_string())
                    if (const auto* t = lookup(r->text))
                        const_cast<json*>(copy.find("$ref"))->text = target(t, r->text);
                for (const auto* kw : {"$dynamicRef", "$recursiveRef"})
                {
                    const auto d = dynamic_of(s, kw);
                    if (d.target == nullptr) continue;
                    auto text = target(d.target, s.find(kw)->text);
                    if (const auto b = d.dynamic ? c.find(d.name) : c.end(); b != c.end())
                    {
                        const auto* f = dyn_anchors_.at(std::pair(b->second, d.name));
                        text = target(f, resource_keys_.at(b->second) + "#" +
                                             (d.name == recursive_name ? std::string() : d.name));
                    }
                    copy.erase(kw);
                    if (!copy.has("$ref"))
                        copy.set("$ref", json::string_of(text));
                    else
                    {
                        auto ref = json::object_of();
                        ref.set("$ref", json::string_of(text));
                        if (auto* all = const_cast<json*>(copy.find("allOf")); all != nullptr && all->is_array())
                            all->items.push_back(std::move(ref));
                        else
                            copy.set("allOf", json::array_of({std::move(ref)}));
                    }
                }
            }

            // copies the root and the entries of its `$defs`, and what they reach.  An entry that the root reaches
            // is in the dynamic scope of the root; another entry starts its own scope.
            void specialize(bool root_typed, std::vector<std::pair<std::string, const json*>>& defs)
            {
                auto roots = std::vector<const json*>{&root_};
                for (const auto& [loc, v] : defs) roots.push_back(v);
                const auto from_root = compute_reach(roots);
                const auto c0 = enter({}, &root_);
                if (root_typed || is_pure_ref(root_)) root_copy_ = expand(&root_, c0);
                for (auto& [loc, v] : defs)
                    v = expand(v, from_root.contains(v) ? enter(c0, resource_of(v)) : enter({}, resource_of(v)));
            }

            bool is_root(const json* s) const { return s == &root_ || (s != nullptr && s == root_copy_); }

            // What the in-place applicators of a schema (the schema, `allOf`, `$ref`) evaluate, for
            // `unevaluatedProperties` (`items` false) or `unevaluatedItems` (`items` true).  The generator merges
            // these into one schema, so the keyword becomes `additionalProperties` / `items` of the merged schema.
            struct evaluation_scope
            {
                bool all = false;  // `additionalProperties` / `items`, or another unevaluated keyword, evaluates all
                bool opaque = false;      // `if`, `dependentSchemas`, `$dynamicRef`, ...: not followed by the generator
                bool dynamic = false;     // `$dynamicRef` / `$recursiveRef`, of those
                bool unresolved = false;  // a reference that the generator cannot follow
                std::vector<const json*> contains;  // the subschemas of `contains`
                // `patternProperties`, of which merge keeps one (merge joins `prefixItems` position by position, so
                // the merged schema evaluates the positions of them all)
                std::vector<const json*> layouts;
                std::size_t lists = 0;             // `anyOf` / `oneOf` whose subschemas evaluate
                bool any = false;                  // one of them is `anyOf`
            };

            // "$conditions" of a normalized schema, which evaluates through `if` and `dependentSchemas` (`not` does
            // not)
            static bool evaluating_conditions(const json& v, bool items)
            {
                return v.is_array() && std::ranges::any_of(v.items, [&](const json& c) {
                           return c.has("if") || (!items && c.has("dependentSchemas"));
                       });
            }

            static bool evaluating_key(std::string_view k, bool items)
            {
                if (k == "if" || k == "then" || k == "else" || k == "$dynamicRef" || k == "$recursiveRef") return true;
                if (items)
                    return k == "prefixItems" || k == "items" || k == "additionalItems" || k == "contains" ||
                           k == "unevaluatedItems";
                return k == "properties" || k == "patternProperties" || k == "additionalProperties" ||
                       k == "unevaluatedProperties" || k == "dependentSchemas" || k == "dependencies";
            }

            // whether a schema evaluates properties / items through any applicator
            bool evaluates(const json& s, bool items, std::set<const json*>& seen, const std::string& loc)
            {
                if (!s.is_object() || !seen.insert(&s).second) return false;
                for (const auto& [k, v] : s.members)
                {
                    if (k == "$conditions")
                    {
                        if (evaluating_conditions(v, items)) return true;
                    }
                    else if (evaluating_key(k, items))
                    {
                        if (k != "properties" || (v.is_object() && !v.members.empty())) return true;
                    }
                    else if (k == "allOf" || k == "anyOf" || k == "oneOf")
                    {
                        if (v.is_array() &&
                            std::ranges::any_of(v.items, [&](const json& e) { return evaluates(e, items, seen, loc); }))
                            return true;
                    }
                    else if (k == "$ref")
                    {
                        const auto* t = resolve(v, loc);
                        if (t == nullptr || evaluates(*t, items, seen, loc)) return true;
                    }
                }
                return false;
            }

            // whether a schema evaluates items that are not a prefix of the array: through `contains`, conditions,
            // `$dynamicRef` / `$recursiveRef`, or a reference that the generator cannot follow
            bool evaluates_beyond_prefix(const json& s, std::set<const json*>& seen, const std::string& loc)
            {
                if (!s.is_object() || !seen.insert(&s).second) return false;
                for (const auto& [k, v] : s.members)
                {
                    if (k == "contains" || k == "if" || k == "then" || k == "else" || k == "$dynamicRef" ||
                        k == "$recursiveRef" || (k == "$conditions" && evaluating_conditions(v, true)))
                        return true;
                    if ((k == "allOf" || k == "anyOf" || k == "oneOf") && v.is_array() &&
                        std::ranges::any_of(v.items,
                                            [&](const json& e) { return evaluates_beyond_prefix(e, seen, loc); }))
                        return true;
                    if (k == "$ref")
                    {
                        const auto* t = resolve(v, loc);
                        if (t == nullptr || evaluates_beyond_prefix(*t, seen, loc)) return true;
                    }
                }
                return false;
            }

            void scan_scope(const json& s, bool items, bool top, evaluation_scope& sc, std::set<const json*>& seen,
                            const std::string& loc)
            {
                if (!s.is_object() || !seen.insert(&s).second) return;
                const auto* own = items ? "unevaluatedItems" : "unevaluatedProperties";
                for (const auto& [k, v] : s.members)
                {
                    if (k == own)
                    {
                        if (!top && !(v.is_bool() && !v.b)) sc.all = true;
                    }
                    else if (k == "if" || k == "then" || k == "else" || k == "$dynamicRef" || k == "$recursiveRef" ||
                             (k == "$conditions" && evaluating_conditions(v, items)) ||
                             (!items && (k == "dependentSchemas" || (k == "dependencies" && v.is_object() &&
                                                                     std::ranges::any_of(v.members, [](const auto& m) {
                                                                         return !m.second.is_array();
                                                                     })))))
                    {
                        sc.opaque = true;
                        sc.dynamic = sc.dynamic || k == "$dynamicRef" || k == "$recursiveRef";
                    }
                    else if (items ? k == "additionalItems" || (k == "items" && !v.is_array())
                                   : k == "additionalProperties")
                        sc.all = true;
                    else if (!items && k == "patternProperties")
                    {
                        if (std::ranges::none_of(sc.layouts, [&](const json* x) { return equal(*x, v); }))
                            sc.layouts.push_back(&v);
                    }
                    else if (items && k == "contains")
                    {
                        if (std::ranges::none_of(sc.contains, [&](const json* x) { return equal(*x, v); }))
                            sc.contains.push_back(&v);
                    }
                    else if (k == "allOf" && v.is_array())
                    {
                        for (const auto& e : v.items) scan_scope(e, items, false, sc, seen, loc);
                    }
                    else if (k == "$ref")
                    {
                        if (const auto* t = resolve(v, loc))
                            scan_scope(*t, items, false, sc, seen, loc);
                        else
                            sc.unresolved = true;
                    }
                    else if ((k == "anyOf" || k == "oneOf") && v.is_array())
                    {
                        if (std::ranges::any_of(v.items, [&](const json& e) {
                                auto visited = std::set<const json*>();
                                return evaluates(e, items, visited, loc);
                            }))
                        {
                            ++sc.lists;
                            sc.any = sc.any || k == "anyOf";
                        }
                    }
                }
            }

            // the keys that the subschema `x` of a condition or of `anyOf` evaluates, those of its `properties`;
            // false where it evaluates through other keywords
            bool evaluated_keys(const json& x, std::vector<std::string>& out, const std::string& loc)
            {
                if (x.is_bool()) return true;
                if (!x.is_object()) return false;
                auto rest = x;
                rest.erase("properties");
                auto seen = std::set<const json*>();
                if (evaluates(rest, false, seen, loc)) return false;
                if (const auto* p = x.find("properties"); p != nullptr && p->is_object())
                    for (const auto& m : p->members) out.push_back(m.first);
                return true;
            }

            // `unevaluatedProperties` `u` of `base` beside `if`, `dependentSchemas` or one `anyOf` that evaluate keys
            // through their `properties`.  A key evaluated only where a subschema `x` holds (`then`, `if`, the
            // subschema of `dependentSchemas` with its key, an alternative of `anyOf`) is read by `u` where `x`
            // fails, and one evaluated only by `else` where `x` holds; `u` false makes the key require `x` (or its
            // negation) through `dependentSchemas`.  The keys become properties, and `u` `additionalProperties`.
            // False where a subschema evaluates through other keywords or a key pattern cannot be read.
            bool settle_conditional_keys(json& base, const json& u, const std::string& loc)
            {
                struct source
                {
                    json test;      // the subschema whose result decides
                    bool positive;  // the key is evaluated where `test` holds; else where it fails
                };
                auto sources = std::map<std::string, std::vector<source>>();
                auto order = std::vector<std::string>();
                const auto add = [&](const std::string& k, const json& test, bool positive) {
                    auto& v = sources[k];
                    if (v.empty()) order.push_back(k);
                    v.push_back({test, positive});
                };
                auto conds = base.has("$conditions") ? *base.find("$conditions") : json::array_of({});
                const auto evaluating = static_cast<std::size_t>(std::ranges::count_if(
                    conds.items, [](const json& c) { return c.has("if") || c.has("dependentSchemas"); }));
                const auto* any = base.find("anyOf");
                const auto alone =
                    u.is_bool() && !u.b && evaluating == 1 && any == nullptr && !base.has("patternProperties");
                const auto* props = base.find("properties");
                const auto declared = [&](const std::string& k) { return props != nullptr && props->has(k); };
                // `if` alone whose subschema constrains only keys that no other keyword names, beside
                // `unevaluatedProperties: false`: where it fails, a key that it names is not evaluated, so the
                // object must satisfy it, as a part of allOf
                for (auto it = conds.items.begin(); alone && it != conds.items.end(); ++it)
                {
                    const auto* i = it->find("if");
                    if (i == nullptr || it->has("then") || it->has("else") || !i->is_object() ||
                        !std::ranges::all_of(i->members, [](const auto& m) {
                            return m.first == "properties" || m.first == "patternProperties" ||
                                   is_metadata_key(m.first);
                        }))
                        continue;
                    if (const auto* p = i->find("properties"); p != nullptr && p->is_object())
                        if (std::ranges::any_of(p->members, [&](const auto& m) { return declared(m.first); })) break;
                    if (const auto* pp = i->find("patternProperties"); pp != nullptr && pp->is_object() &&
                                                                       props != nullptr)
                        for (const auto& [re, _] : pp->members)
                            for (const auto& [k, _2] : props->members)
                                if (const auto m = pattern_matches(re, k); !m || *m) return false;
                    const auto part = *i;
                    conds.items.erase(it);
                    if (conds.items.empty())
                        base.erase("$conditions");
                    else
                        base.set("$conditions", conds);
                    base = merge(base, part, loc + "/if");
                    base.set("additionalProperties", u);
                    base.erase("unevaluatedProperties");
                    return true;
                }
                for (const auto& c : conds.items)
                {
                    if (const auto* i = c.find("if"))
                    {
                        auto when = std::vector<std::string>();  // `if` and `then`
                        auto otherwise = std::vector<std::string>();
                        if (!evaluated_keys(*i, when, loc)) return false;
                        if (const auto* t = c.find("then"); t != nullptr && !evaluated_keys(*t, when, loc))
                            return false;
                        if (const auto* e = c.find("else"); e != nullptr && !evaluated_keys(*e, otherwise, loc))
                            return false;
                        for (auto* v : {&when, &otherwise})
                            for (auto it = v->begin(); it != v->end();)
                                it = std::find(v->begin(), it, *it) != it ? v->erase(it) : it + 1;
                        for (const auto& k : when)
                            if (std::ranges::find(otherwise, k) == otherwise.end()) add(k, *i, true);
                        for (const auto& k : otherwise)
                            if (std::ranges::find(when, k) == when.end()) add(k, *i, false);
                        // a key that both evaluate is always evaluated, which is a source of its own
                        for (const auto& k : when)
                            if (std::ranges::find(otherwise, k) != otherwise.end())
                                add(k, json::boolean_of(true), true);
                    }
                    else if (const auto* d = c.find("dependentSchemas"); d != nullptr && d->is_object())
                    {
                        for (const auto& [k, x] : d->members)
                        {
                            auto keys = std::vector<std::string>();
                            if (!evaluated_keys(x, keys, loc)) return false;
                            auto test = json::object_of();
                            test.set("required", json::array_of({json::string_of(k)}));
                            for (const auto& m : keys) add(m, test, true);
                        }
                    }
                }
                if (any != nullptr && any->is_array())
                    for (const auto& a : any->items)
                    {
                        auto keys = std::vector<std::string>();
                        if (!evaluated_keys(a, keys, loc)) return false;
                        for (const auto& k : keys) add(k, a, true);
                    }
                const auto* patterns = base.find("patternProperties");
                auto added = json::array_of({});
                auto properties = props != nullptr && props->is_object() ? *props : json::object_of();
                const auto ref_loc = loc + "/unevaluatedProperties";
                for (const auto& k : order)
                {
                    if (declared(k)) continue;
                    if (patterns != nullptr && patterns->is_object())
                    {
                        auto matched = false;
                        for (const auto& [re, _] : patterns->members)
                        {
                            const auto m = pattern_matches(re, k);
                            if (!m) return false;
                            matched = matched || *m;
                        }
                        if (matched) continue;
                    }
                    properties.set(k, json::boolean_of(true));
                    const auto& v = sources[k];
                    if (std::ranges::any_of(v, [](const source& x) { return x.test.is_bool(); })) continue;
                    if (v.size() > 1)
                    {
                        warn(ref_loc, "the key \"" + k +
                                          "\" is evaluated by several subschemas of `if`, `dependentSchemas` or "
                                          "`anyOf`; `unevaluatedProperties` does not apply to it");
                        continue;
                    }
                    const auto& [test, positive] = v.front();
                    auto entry = json::object_of();
                    if (u.is_bool() && !u.b)
                    {
                        // the key requires the subschema, or its negation
                        auto x = test;
                        if (!positive)
                        {
                            x = json::object_of();
                            x.set("not", test);
                        }
                        auto dep = json::object_of();
                        dep.set(k, std::move(x));
                        entry.set("dependentSchemas", std::move(dep));
                    }
                    else
                    {
                        auto p = json::object_of();
                        p.set(k, u);
                        auto branch = json::object_of();
                        branch.set("properties", std::move(p));
                        entry.set("if", test);
                        entry.set(positive ? "else" : "then", std::move(branch));
                    }
                    entry.set("$loc", json::string_of(ref_loc));
                    added.items.push_back(std::move(entry));
                }
                for (auto& e : added.items) conds.items.push_back(std::move(e));
                if (!conds.items.empty()) base.set("$conditions", std::move(conds));
                base.set("properties", std::move(properties));
                base.set("additionalProperties", u);
                base.erase("unevaluatedProperties");
                return true;
            }

            // the number of leading items that the subschema `x` evaluates where it holds, SIZE_MAX for all of them;
            // nullopt where it evaluates others (`contains`, conditions, `anyOf` / `oneOf` that evaluate, ...)
            std::optional<std::size_t> evaluated_prefix(const json* x, const std::string& loc)
            {
                if (x == nullptr || x->is_bool()) return 0;
                auto seen = std::set<const json*>();
                if (evaluates_beyond_prefix(*x, seen, loc)) return std::nullopt;
                const auto* n = normalize_full(x, loc);
                if (n->is_bool()) return 0;
                if (!n->is_object() || n->has("unevaluatedItems")) return std::nullopt;
                for (const auto* k : {"anyOf", "oneOf"})
                    if (const auto* l = n->find(k); l != nullptr && l->is_array() &&
                                                    std::ranges::any_of(l->items, [&](const json& e) {
                                                        auto visited = std::set<const json*>();
                                                        return evaluates(e, true, visited, loc);
                                                    }))
                        return std::nullopt;
                if (const auto* i = n->find("items"); (i != nullptr && !i->is_array()) || n->has("additionalItems"))
                    return SIZE_MAX;
                const auto* p = n->find("prefixItems");
                return p != nullptr && p->is_array() ? p->items.size() : 0;
            }

            // `unevaluatedItems` `u` of `base` beside one `if` whose subschemas evaluate a prefix of the array: the
            // items that `base` and `if` with `then` evaluate where `if` holds, and those that `base` and `else`
            // evaluate where it fails, are each a prefix, so `u` goes into `then` and `else` as `items` after the
            // longer prefix (a failing `if` evaluates nothing).  False where that does not hold.
            bool settle_conditional_items(json& base, const json& u, const std::string& loc)
            {
                const auto* list = base.find("$conditions");
                if (list == nullptr || !list->is_array()) return false;
                auto conds = *list;
                auto* entry = static_cast<json*>(nullptr);
                for (auto& c : conds.items)
                    if (c.has("if"))
                    {
                        if (entry != nullptr) return false;
                        entry = &c;
                    }
                if (entry == nullptr) return false;
                const auto* bp = base.find("prefixItems");
                const auto nb = bp != nullptr && bp->is_array() ? bp->items.size() : std::size_t{0};
                // stored copies: normalizing caches by address, and the branches of `conds` are rewritten below
                const auto stable = [&](const char* k) {
                    const auto* x = entry->find(k);
                    return x != nullptr ? store(json(*x)) : nullptr;
                };
                const auto ni = evaluated_prefix(stable("if"), loc + "/if");
                const auto nt = evaluated_prefix(stable("then"), loc + "/then");
                const auto ne = evaluated_prefix(stable("else"), loc + "/else");
                if (!ni || !nt || !ne) return false;
                const auto branch = [&](const char* key, std::size_t n) {
                    if (n == SIZE_MAX) return;
                    auto rest = json::object_of();
                    auto prefix = json::array_of({});
                    for (std::size_t i = 0; i < n; ++i) prefix.items.push_back(json::boolean_of(true));
                    if (n > 0) rest.set("prefixItems", std::move(prefix));
                    rest.set("items", u);
                    if (const auto* b = entry->find(key))
                    {
                        auto all = json::object_of();
                        all.set("allOf", json::array_of({*b, std::move(rest)}));
                        entry->set(key, std::move(all));
                    }
                    else
                        entry->set(key, std::move(rest));
                };
                branch("then", std::max({nb, *ni, *nt}));
                branch("else", std::max(nb, *ne));
                base.set("$conditions", std::move(conds));
                std::erase_if(base.members, [](const auto& m) { return m.first == "unevaluatedItems"; });
                return true;
            }

            // `unevaluatedProperties` / `unevaluatedItems` of `s`, whose in-place applicators are merged into `base`:
            // as `additionalProperties` / `items` of `base` when that reads every valid value, kept for the
            // alternatives of the one `oneOf` whose subschemas evaluate (each alternative settles it again), else
            // dropped, which reads more than the schema accepts
            void settle_unevaluated(const json& s, json& base, bool items, const std::string& loc)
            {
                const auto* key = items ? "unevaluatedItems" : "unevaluatedProperties";
                const auto* u = base.find(key);
                if (u == nullptr) return;
                const auto drop = [&](std::string why) {
                    if (!why.empty()) warn(loc, "`" + std::string(key) + "` " + why + "; it is ignored");
                    std::erase_if(base.members, [&](const auto& m) { return m.first == key; });
                };
                const auto types = type_list(base);
                if (adds_nothing(*u) || base.has(items ? "items" : "additionalProperties") ||
                    (!types.empty() && std::ranges::find(types, items ? "array" : "object") == types.end()))
                    return drop("");
                auto sc = evaluation_scope();
                auto seen = std::set<const json*>();
                scan_scope(s, items, true, sc, seen, loc);
                if (sc.all) return drop("");
                if (!sc.dynamic && !sc.unresolved && sc.layouts.size() <= 1 && sc.contains.empty() &&
                    !items && (sc.lists == 0 ? sc.opaque : sc.lists == 1 && sc.any) &&
                    settle_conditional_keys(base, *u, loc))
                    return;
                if (items && sc.opaque && !sc.dynamic && !sc.unresolved && sc.contains.empty() && sc.lists == 0 &&
                    settle_conditional_items(base, *u, loc))
                    return;
                const auto kinds = [](const json& x) {
                    auto t = type_list(x);
                    std::ranges::sort(t);
                    return t;
                };
                if (items && !sc.contains.empty() && !sc.opaque && !sc.unresolved && sc.lists == 0 &&
                    ((u->is_bool() && !u->b) ||
                     std::ranges::all_of(sc.contains, [&](const json* c) { return kinds(*c) == kinds(*u); })))
                {
                    // the items that no subschema of `contains` evaluates are read by the unevaluated schema; beside
                    // a schema of other types, the elements would be of several types, which `contains` cannot test
                    auto alts = json::array_of({});
                    for (const auto* c : sc.contains) alts.items.push_back(*c);
                    if (!(u->is_bool() && !u->b)) alts.items.push_back(*u);
                    auto v = json();
                    if (alts.items.size() == 1)
                        v = alts.items[0];
                    else
                    {
                        v = json::object_of();
                        v.set("anyOf", std::move(alts));
                    }
                    drop("");
                    base.set("items", std::move(v));
                    return;
                }
                if (sc.opaque)
                    return drop("beside `if`, `then`, `else`, " +
                                std::string(items ? "" : "`dependentSchemas`, ") +
                                "`$dynamicRef` or `$recursiveRef` is not supported");
                if (sc.unresolved) return drop("beside a reference that cannot be read is not supported");
                if (!sc.contains.empty()) return drop("beside `contains` is not supported");
                if (sc.layouts.size() > 1) return drop("beside several `patternProperties` is not supported");
                if (sc.lists == 0)
                {
                    auto v = *u;
                    drop("");
                    base.set(items ? "items" : "additionalProperties", std::move(v));
                    return;
                }
                auto seen_one = std::set<const json*>();
                const auto evaluating = [&](const json& e) { return evaluates(e, items, seen_one, loc); };
                const auto* one = base.find("oneOf");
                const auto* any = base.find("anyOf");
                if (sc.lists == 1 && !sc.any && one != nullptr && one->is_array() &&
                    std::ranges::any_of(one->items, evaluating) &&
                    (any == nullptr || required_groups_of(base, "anyOf")))
                    return;
                // the items that a subschema evaluates without `contains` and conditions are a prefix of the array,
                // so those of the alternatives that hold are the longest of their prefixes, which is what the
                // alternative of that prefix evaluates: the keyword is kept for each alternative as for `oneOf`
                if (items && sc.lists == 1 && sc.any && any != nullptr && any->is_array() &&
                    std::ranges::none_of(any->items, [&](const json& e) {
                        auto visited = std::set<const json*>();
                        return evaluates_beyond_prefix(e, visited, loc);
                    }))
                    return;
                drop(sc.any && sc.lists == 1 ? "beside `anyOf` whose subschemas evaluate " +
                                                   std::string(items ? "items" : "properties") + " is not supported"
                                             : "beside several `anyOf` / `oneOf` is not supported");
            }

            const json* normalize_full(const json* s, const std::string& loc) { return normalize(deref(s, loc), loc); }

            // merges allOf, and $ref with sibling keywords, into one schema, which is `false` when no subschema of
            // its `anyOf` / `oneOf` accepts a value
            const json* normalize(const json* s, const std::string& loc)
            {
                const auto* n = merge_parts(s, loc);
                if (const auto* f = contains_nothing(s, n, loc); f != nullptr) n = f;
                if (!n->is_object() || !no_choice(n, loc)) return n;
                if (const auto it = normalized_.find(s); it != normalized_.end() && it->second->is_bool())
                    return it->second;
                const auto* f = store(json::boolean_of(false));
                return quiet_ > 0 ? f : normalized_[s] = f;
            }

            // `n`, the merged schema of `s`, read as if its `contains` were `false` when the subschema of `contains`
            // accepts no value through a reference or keywords that no value satisfies together; null otherwise
            const json* contains_nothing(const json* s, const json* n, const std::string& loc)
            {
                if (!n->is_object()) return nullptr;
                const auto* c = n->find("contains");
                if (c == nullptr || c->is_bool()) return nullptr;
                if (const auto it = contains_nothing_.find(n); it != contains_nothing_.end())
                    return it->second.first;
                if (!containing_.insert(n).second) return nullptr;
                const auto* cn = normalize_full(c, loc + "/contains");
                containing_.erase(n);
                auto r = std::pair<const json*, const json*>();
                if (cn->is_bool() && !cn->b)
                {
                    auto copy = json(*n);
                    copy.set("contains", json::boolean_of(false));
                    r.second = store(std::move(copy));
                    r.first = merge_parts(r.second, loc);
                }
                if (r.second != nullptr && reported_false_.contains(r.second) && quiet_ == 0)
                    reported_false_.insert(s);
                if (quiet_ == 0) contains_nothing_[n] = r;
                return r.first;
            }

            // whether no subschema of `anyOf` / `oneOf`, with the keywords beside it, accepts a value
            bool no_choice(const json* s, const std::string& loc)
            {
                if (!s->has("anyOf") && !s->has("oneOf")) return false;
                if (const auto it = no_choice_.find(s); it != no_choice_.end()) return it->second;
                if (!choosing_.insert(s).second) return false;
                ++quiet_;
                const auto c = choices_of(*s, loc);
                auto r = c && !c->schemas.empty();
                for (std::size_t i = 0; r && i < c->schemas.size(); ++i)
                {
                    const auto* n = normalize(c->schemas[i], c->locs[i]);
                    r = n->is_bool() && !n->b;
                }
                if (--quiet_ == 0)
                {
                    for (const auto* q : quiet_normalized_) normalized_.erase(q);
                    quiet_normalized_.clear();
                }
                choosing_.erase(s);
                return no_choice_[s] = r;
            }

            const json* keep_normalized(const json* s, const json* n)
            {
                if (quiet_ > 0) quiet_normalized_.push_back(s);
                return normalized_[s] = n;
            }

            const json* merge_parts(const json* s, const std::string& loc)
            {
                if (!s->is_object()) return s;
                const auto has_all = s->has("allOf");
                const auto ref_siblings = s->has("$ref") && !is_pure_ref(*s);
                const auto* items = s->find("items");
                const auto legacy =
                    (items != nullptr && items->is_array()) || s->has("additionalItems") || s->has("dependencies");
                const auto unevaluated = s->has("unevaluatedProperties") || s->has("unevaluatedItems");
                const auto conditional = std::ranges::any_of(
                    s->members, [](const auto& m) { return is_condition_key(m.first) && m.first != "$conditions"; });
                const auto* one = s->find("oneOf");
                const auto open_choices =
                    one != nullptr && one->is_array()
                        ? std::ranges::count_if(one->items, [](const json& e) { return accepts_anything(e); })
                        : 0;
                const auto* ct = s->find("contains");
                const auto no_contains = ct != nullptr && ct->is_bool() && !ct->b;
                if (!has_all && !ref_siblings && !legacy && !unevaluated && !conditional && open_choices == 0 &&
                    !no_contains)
                    return s;
                if (const auto it = normalized_.find(s); it != normalized_.end()) return it->second;
                // `not` of a schema that accepts every value accepts no value, and so does `oneOf` of two subschemas
                // that accept every value
                if (const auto* nt = s->find("not");
                    open_choices > 1 || (nt != nullptr && (nt->is_bool() || nt->is_object()) && adds_nothing(*nt)))
                    return keep_normalized(s, store(json::boolean_of(false)));
                if (!normalizing_.insert(s).second)
                {
                    warn(loc, "`allOf` refers to itself; the schema is read as any JSON value");
                    return store(json::boolean_of(true));
                }
                auto base = json::object_of();
                for (const auto& [k, v] : s->members)
                    if (k != "allOf" && k != "$ref") base.members.emplace_back(k, v);
                upgrade(base);
                // keywords that come down to others: `oneOf` with one subschema that accepts every value is `not` of
                // `anyOf` of the others, `if` that accepts every value or none is its `then` or its `else`, and
                // `contains` false is `not` of arrays unless `minContains` is 0
                auto rewritten = std::vector<std::pair<const json*, std::string>>();
                if (open_choices == 1)
                {
                    auto others = json::array_of({});
                    for (const auto& e : one->items)
                        if (!accepts_anything(e)) others.items.push_back(e);
                    base.erase("oneOf");
                    if (!others.items.empty())
                    {
                        auto any = json::object_of();
                        any.set("anyOf", std::move(others));
                        auto n = json::object_of();
                        n.set("not", std::move(any));
                        rewritten.emplace_back(store(std::move(n)), loc + "/oneOf");
                    }
                }
                if (const auto* i = base.find("if"); i != nullptr && (accepts_anything(*i) || (i->is_bool() && !i->b)))
                {
                    const auto* branch = accepts_anything(*i) ? "then" : "else";
                    if (const auto* b = base.find(branch)) rewritten.emplace_back(store(json(*b)), loc + "/" + branch);
                    for (const auto* k : {"if", "then", "else"}) base.erase(k);
                }
                if (no_contains)
                {
                    const auto* mn = base.find("minContains");
                    const auto min_c = mn != nullptr && as_uint64(*mn) ? *as_uint64(*mn) : 1;
                    if (const auto* ty = base.find("type"); min_c > 0 && ty != nullptr && ty->is_string() &&
                                                            ty->text == "array")
                    {
                        warn(loc + "/contains", "`contains` false accepts no array; reading it always fails");
                        if (quiet_ == 0) reported_false_.insert(s);
                        return keep_normalized(s, store(json::boolean_of(false)));
                    }
                    for (const auto* k : {"contains", "minContains", "maxContains"}) base.erase(k);
                    if (min_c > 0)
                    {
                        auto arrays = json::object_of();
                        arrays.set("type", json::string_of("array"));
                        auto n = json::object_of();
                        n.set("not", std::move(arrays));
                        rewritten.emplace_back(store(std::move(n)), loc + "/contains");
                    }
                }
                // `not` of `not` is the inner schema, a part of allOf; not beside unevaluated*, for which the keywords
                // inside `not` evaluate nothing
                const json* twice = nullptr;
                if (const auto* nt = s->find("not");
                    !unevaluated && nt != nullptr && nt->is_object() && nt->has("not") &&
                    std::ranges::all_of(nt->members,
                                        [](const auto& m) { return m.first == "not" || is_metadata_key(m.first); }))
                {
                    twice = nt->find("not");
                    base.erase("not");
                }
                collect_conditions(base, loc);
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
                if (twice != nullptr && !adds_nothing(*twice)) parts.emplace_back(twice, loc + "/not/not");
                for (auto& r : rewritten)
                    if (!adds_nothing(*r.first)) parts.push_back(std::move(r));
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
                    return keep_normalized(s, r);
                }
                if (const auto* ref = s->find("$ref"))
                    if (const auto* t = resolve(*ref, loc)) base = merge_body(base, normalize_full(t, loc), loc);
                if (all != nullptr && !all->is_array()) warn(loc, "`allOf` is not an array");
                for (const auto& [e, l] : parts) base = merge_body(base, normalize_full(e, l), l);
                settle_unevaluated(*s, base, false, loc);
                settle_unevaluated(*s, base, true, loc);
                // by text: a reference beside other keywords to a schema that holds it gives a copy of it at every
                // level, which would be typed without end
                const auto* r = store_merged(std::move(base));
                normalizing_.erase(s);
                return keep_normalized(s, r);
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

            // `if` with `then` and `else`, `not` and `dependentSchemas` as entries of "$conditions", after those that
            // the schema has, with the location of the schema in "$loc": merging schemas concatenates them
            static void collect_conditions(json& s, const std::string& loc)
            {
                auto list = s.has("$conditions") ? *s.find("$conditions") : json::array_of({});
                const auto add = [&](std::initializer_list<const char*> keys) {
                    auto c = json::object_of();
                    for (const auto* k : keys)
                        if (const auto* v = s.find(k)) c.set(k, *v);
                    c.set("$loc", json::string_of(loc));
                    list.items.push_back(std::move(c));
                };
                // `then` and `else` without `if` do nothing
                if (s.has("if")) add({"if", "then", "else"});
                if (s.has("not")) add({"not"});
                if (s.has("dependentSchemas")) add({"dependentSchemas"});
                std::erase_if(s.members, [](const auto& m) { return is_condition_key(m.first); });
                if (!list.items.empty()) s.set("$conditions", std::move(list));
            }

            static bool same_condition(const json& a, const json& b)
            {
                auto x = a, y = b;
                std::erase_if(x.members, [](const auto& m) { return m.first == "$loc"; });
                std::erase_if(y.members, [](const auto& m) { return m.first == "$loc"; });
                return equal(x, y);
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

            // the value of `v` that the schema `x` accepts too
            static json both(const json& v, const json& x)
            {
                if (x.is_bool()) return x.b ? v : x;
                if (v.is_bool()) return v.b ? x : v;
                return all_of(v, x);
            }

            // The schema that the schema `s` applies to the key `k`, which its `properties` do not declare: those of
            // the patterns of `patternProperties` that `k` matches, else `additionalProperties`; nullopt where it
            // applies none, or where std::regex cannot read a pattern (with a warning)
            std::optional<json> undeclared_schema(const json& s, const std::string& k, const std::string& loc)
            {
                if (const auto* pp = s.find("patternProperties"); pp != nullptr && pp->is_object())
                {
                    auto r = std::optional<json>();
                    for (const auto& [p, sub] : pp->members)
                    {
                        try
                        {
                            if (!std::regex_search(k, std::regex(p, std::regex::ECMAScript))) continue;
                        }
                        catch (const std::regex_error&)
                        {
                            warn(loc, "the property \"" + k + "\" is not checked against the pattern \"" + p +
                                          "\" of the other schema of `allOf`: std::regex does not read the pattern");
                            return std::nullopt;
                        }
                        r = r ? both(*r, sub) : sub;
                    }
                    if (r) return r;
                }
                if (const auto* ap = s.find("additionalProperties")) return *ap;
                return std::nullopt;
            }

            // The schema `s` beside the schema `other` in `allOf`: a property that `other` does not declare also
            // takes what `other` applies to such a key, the schemas of the patterns of `s` what `other` applies to
            // any key when it has no patterns, and a position of `prefixItems` beyond those of `other` its `items`.
            // An `additionalProperties` or `items` that comes from `unevaluatedProperties` or `unevaluatedItems` of a
            // subschema thereby evaluates that subschema only.
            json beside(const json& s, const json& other, const std::string& loc)
            {
                if (!s.is_object() || !other.is_object()) return s;
                auto r = s;
                if (const auto* props = s.find("properties"); props != nullptr && props->is_object())
                {
                    const auto* declared = other.find("properties");
                    auto p = *props;
                    for (auto& [k, v] : p.members)
                        if (declared == nullptr || !declared->is_object() || !declared->has(k))
                            if (auto x = undeclared_schema(other, k, loc)) v = both(v, *x);
                    r.set("properties", std::move(p));
                }
                if (const auto* pp = s.find("patternProperties");
                    pp != nullptr && pp->is_object() && !other.has("patternProperties"))
                    if (const auto* ap = other.find("additionalProperties"))
                    {
                        auto p = *pp;
                        for (auto& [k, v] : p.members) v = both(v, *ap);
                        r.set("patternProperties", std::move(p));
                    }
                if (const auto* prefix = s.find("prefixItems"); prefix != nullptr && prefix->is_array())
                    if (const auto* items = other.find("items"))
                    {
                        const auto* theirs = other.find("prefixItems");
                        const auto n = theirs != nullptr && theirs->is_array() ? theirs->items.size() : 0;
                        auto p = *prefix;
                        for (auto i = n; i < p.items.size(); ++i) p.items[i] = both(p.items[i], *items);
                        r.set("prefixItems", std::move(p));
                    }
                return r;
            }

            // `merge` of `a` and the normalized schema `n`, whose `$ref`, kept for a named type when `allOf` holds
            // one reference, is merged as the schema it refers to
            json merge_body(const json& a, const json* n, const std::string& loc)
            {
                auto seen = std::set<const json*>();
                auto r = a;
                while (n->is_object())
                {
                    const auto* ref = n->find("$ref");
                    const auto* t = ref != nullptr ? resolve(*ref, loc) : nullptr;
                    if (t == nullptr || !seen.insert(n).second) break;
                    auto rest = *n;
                    rest.erase("$ref");
                    r = merge(r, rest, loc);
                    n = normalize_full(t, loc);
                }
                return merge(r, *n, loc);
            }

            json merge(const json& a0, const json& b0, const std::string& loc)
            {
                if (a0.is_bool()) return a0.b ? b0 : a0;
                if (b0.is_bool()) return b0.b ? a0 : b0;
                if (!a0.is_object() || !b0.is_object()) return a0;
                const auto a = beside(a0, b0, loc);
                auto b = beside(b0, a0, loc);
                // an array that has an element of each `contains` may have different elements for them; one
                // `contains` cannot say that, so the counts of the second go with it
                if (const auto *ac = a.find("contains"), *bc = b.find("contains");
                    ac != nullptr && bc != nullptr && !equal(*ac, *bc))
                {
                    warn(loc, "`allOf` has different values of `contains`; the first is used");
                    std::erase_if(b.members, [](const auto& m) {
                        return m.first == "contains" || m.first == "minContains" || m.first == "maxContains";
                    });
                }
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
                        // types that have no value in common leave no value to the schema
                        if (out.empty()) return json::boolean_of(false);
                        r.set("type", type_json(out));
                    }
                    else if (k == "properties" && av->is_object() && bv.is_object())
                    {
                        auto props = *av;
                        for (const auto& [pk, pv] : bv.members)
                        {
                            if (const auto* x = props.find(pk))
                                props.set(pk, both(*x, pv));
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
                    else if (k == "additionalProperties" || k == "unevaluatedProperties" || k == "unevaluatedItems")
                    {
                        if (av->is_bool() || bv.is_bool())
                            r.set(k, merge(*av, bv, loc));
                        else
                            r.set(k, all_of(*av, bv));
                    }
                    else if (k == "items" || k == "contains" || k == "propertyNames")
                        r.set(k, all_of(*av, bv));
                    // several patterns stay as an array of them, each a yyjson::pattern of the value
                    else if (k == "pattern")
                    {
                        auto list = json::array_of({});
                        for (const auto* v : {av, &bv})
                            for (const auto& x : v->is_array() ? v->items : std::vector<json>{*v})
                                if (x.is_string() &&
                                    std::ranges::none_of(list.items, [&](const json& y) { return equal(x, y); }))
                                    list.items.push_back(x);
                        if (list.items.size() == 1)
                            r.set(k, std::move(list.items[0]));
                        else
                            r.set(k, std::move(list));
                    }
                    // the multiples of both numbers are those of their least common multiple
                    else if (k == "multipleOf" && av->is_number() && bv.is_number() && !equal(*av, bv))
                    {
                        if (const auto m = decimal_lcm(*av, bv))
                            r.set(k, *m);
                        else
                            warn(loc, "`allOf` has values of `multipleOf` whose least common multiple is not "
                                      "computed; the first is used");
                    }
                    // no value equals two different values
                    else if (k == "const" && !equal(*av, bv))
                        return json::boolean_of(false);
                    else if (k == "prefixItems" && av->is_array() && bv.is_array())
                    {
                        auto p = *av;
                        for (std::size_t i = 0; i < bv.items.size(); ++i)
                            if (i < p.items.size())
                                p.items[i] = both(p.items[i], bv.items[i]);
                            else
                                p.items.push_back(bv.items[i]);
                        r.set(k, std::move(p));
                    }
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
                    else if (k == "$conditions" && av->is_array() && bv.is_array())
                    {
                        auto list = *av;
                        for (const auto& c : bv.items)
                            if (std::ranges::none_of(list.items, [&](const json& x) { return same_condition(x, c); }))
                                list.items.push_back(c);
                        r.set(k, std::move(list));
                    }
                    else if (!is_metadata_key(k) && !equal(*av, bv))
                        warn(loc, "`allOf` has different values of `" + k + "`; the first is used");
                }
                // a position that accepts no value ends the array before it
                if (const auto* prefix = r.find("prefixItems"); prefix != nullptr && prefix->is_array())
                    if (const auto it = std::ranges::find_if(prefix->items,
                                                             [](const json& e) { return e.is_bool() && !e.b; });
                        it != prefix->items.end())
                    {
                        const auto n = static_cast<std::size_t>(it - prefix->items.begin());
                        auto p = *prefix;
                        p.items.resize(n);
                        if (const auto* m = r.find("maxItems"); m == nullptr || !m->is_number() || m->num > double(n))
                        {
                            auto j = json();
                            j.k = json::kind::number;
                            j.integer = true;
                            j.num = static_cast<double>(n);
                            j.text = std::to_string(n);
                            r.set("maxItems", std::move(j));
                        }
                        if (n == 0)
                            std::erase_if(r.members, [](const auto& m) { return m.first == "prefixItems"; });
                        else
                            r.set("prefixItems", std::move(p));
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
                const auto b = global_safe(sanitize(base.empty() ? "Value" : base));
                auto name = b;
                for (auto n = 2; type_names_.contains(name); ++n) name = b + std::to_string(n);
                type_names_.insert(name);
                return name;
            }

            // a type of the global namespace cannot share the name of a declaration there
            std::string global_safe(std::string name) const
            {
                if (opts_.ns.empty() && is_global_name(name)) name += '_';
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
                                 "maxProperties", "propertyNames", "dependentRequired"}) ||
                    !condition_keys(s).empty())
                    groups.emplace_back("object");
                if (any_of_keys({"items", "prefixItems", "minItems", "maxItems", "uniqueItems", "contains",
                                 "minContains", "maxContains"}))
                    groups.emplace_back("array");
                if (any_of_keys({"minLength", "maxLength", "pattern", "format"})) groups.emplace_back("string");
                if (any_of_keys({"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf"}))
                    groups.emplace_back("number");
                if (groups.empty()) return {};
                // the keywords of a type constrain only the values of that type
                if (!opts_.infer_types) return {"null", "boolean", "object", "array", "number", "string"};
                if (groups.size() == 1)
                {
                    warn(loc, "the schema has no `type`; it is read as " + groups[0] + " from its keywords");
                    return groups;
                }
                warn(loc, "the schema has no `type` and keywords of several types; it is read as any JSON value");
                return {};
            }

            typed type_of(const json* s0, const std::string& loc, const std::string& hint)
            {
                if (const auto it = cache_.find(s0); it != cache_.end())
                {
                    if (loose_.contains(s0)) ++warn_calls_;
                    return it->second;
                }
                const auto calls = warn_calls_;
                auto r = type_of_new(s0, loc, hint);
                if (warn_calls_ != calls) loose_.insert(s0);
                return r;
            }

            typed type_of_new(const json* s0, const std::string& loc, const std::string& hint)
            {
                if (s0->is_bool())
                {
                    // the root and $defs go through type_named(), which gives a struct or an alias the settings;
                    // `true` needs a name only at the root
                    if (!s0->b) return def_names_.contains(s0) ? type_named(s0, loc) : no_value();
                    return is_root(s0) ? type_named(s0, loc) : typed{};
                }
                if (!s0->is_object())
                {
                    warn(loc, "a schema is not an object or a boolean; it is read as any JSON value");
                    return {};
                }
                // a schema that its own type holds becomes a struct of one field with yyjson::transparent, which can
                // hold itself as other structs do
                if (busy_.contains(s0)) return typed{named_type(self_struct(s0, loc)), {}};
                if (def_names_.contains(s0)) return type_named(s0, loc);
                busy_.emplace(s0, hint);
                auto r = type_content(s0, loc, hint, false);
                busy_.erase(s0);
                if (const auto it = self_structs_.find(s0); it != self_structs_.end())
                {
                    fill_transparent(it->second, s0, std::move(r), loc, false);
                    return cache_[s0] = typed{named_type(it->second), {}};
                }
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
                busy_.emplace(s0, name);
                auto r = type_content(s0, loc, name, true);
                busy_.erase(s0);
                if (const auto it = self_structs_.find(s0); it != self_structs_.end())
                {
                    fill_transparent(it->second, s0, std::move(r), loc, true);
                    return cache_[s0] = typed{named_type(it->second), {}};
                }
                if (find_decl(name) >= 0) return cache_[s0] = r;
                if (is_root(s0) && !r.s.empty())
                {
                    // the constraints of a root that is not an object have no field to go on: a struct of one field
                    // with yyjson::transparent, which is read and written as the value of the field, takes them
                    const auto idx = add_decl(decl{.kind = decl::kind_t::structure, .name = name, .loc = loc});
                    fill_transparent(idx, s0, std::move(r), loc, true);
                    return cache_[s0] = typed{named_type(idx), {}};
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

            // the struct of a schema that its own type holds, made at the first such reference; the schema of $defs
            // or the root keeps its name
            int self_struct(const json* s0, const std::string& loc)
            {
                if (const auto it = self_structs_.find(s0); it != self_structs_.end()) return it->second;
                const auto it = def_names_.find(s0);
                const auto name = it != def_names_.end() ? it->second : unique_type_name(busy_.at(s0));
                const auto idx =
                    add_decl(decl{.kind = decl::kind_t::structure, .name = name, .loc = loc, .transparent = true});
                return self_structs_[s0] = idx;
            }

            // whether the type holds the struct `idx` with no array or object between, through which reading it
            // never ends
            bool holds_unguarded(const ctype& t, int idx, std::vector<int>& seen) const
            {
                switch (t.k)
                {
                    case tk::named:
                    {
                        if (t.decl == idx) return true;
                        if (std::ranges::find(seen, t.decl) != seen.end()) return false;
                        seen.push_back(t.decl);
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return holds_unguarded(d.target, idx, seen);
                        return d.transparent && d.filled && holds_unguarded(d.fields[0].type, idx, seen);
                    }
                    case tk::optional:
                    case tk::indirect:
                    case tk::variant:
                        return std::ranges::any_of(t.args, [&](const ctype& a) { return holds_unguarded(a, idx, seen); });
                    default:
                        return false;
                }
            }

            // a struct of one field with yyjson::transparent, which is read and written as the value of the field.
            // The metadata of a schema that is not named stays with the fields that hold it
            void fill_transparent(int idx, const json* s0, typed r, const std::string& loc, bool named)
            {
                auto& d = decls_[idx];
                if (named) d.meta = meta_of(*s0);
                // a schema that refers to an alias takes the metadata of the alias, which has no place for it
                if (const auto* t = named && is_pure_ref(*s0) ? resolve(*s0->find("$ref"), loc) : nullptr; t != nullptr)
                {
                    auto m = meta_of(*t);
                    if (!d.meta.title) d.meta.title = std::move(m.title);
                    if (!d.meta.description) d.meta.description = std::move(m.description);
                    if (!d.meta.comment) d.meta.comment = std::move(m.comment);
                    d.meta.deprecated = d.meta.deprecated || m.deprecated;
                    if (d.meta.examples.empty()) d.meta.examples = std::move(m.examples);
                }
                if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                d.transparent = true;
                auto seen = std::vector<int>();
                if (holds_unguarded(r.t, idx, seen))
                {
                    warn(loc,
                         "the schema refers to itself with no array or object between, which no value ends; it is "
                         "read as any JSON value");
                    r = {};
                }
                d.fields.push_back(field{.member = "value", .key = "value", .type = r.t, .set = r.s, .loc = loc});
                d.filled = true;
            }

            void warn_unsupported(const json& s, const std::string& loc)
            {
                for (const auto k : ignored_keys)
                {
                    const auto* v = s.find(k);
                    if (v == nullptr) continue;
                    // specialize() replaced the references that it found; resolve() tells why the others are not
                    if (dynamic_ && v->is_string() && resolve(*v, loc) == nullptr) continue;
                    warn(loc, "`" + std::string(k) + "` is not supported and is ignored");
                }
            }

            typed type_content(const json* s0, const std::string& loc, const std::string& hint, bool named)
            {
                const auto* s = normalize(s0, loc);
                auto r = type_body(s0, loc, hint, named);
                // a variant passes the conditions to its alternatives, which take them or warn
                const auto* c = s->is_object() ? s->find("$conditions") : nullptr;
                if (c != nullptr && !conditions_used_.contains(s) &&
                    !((s->has("anyOf") && !required_groups_of(*s, "anyOf")) ||
                      (s->has("oneOf") && !required_groups_of(*s, "oneOf"))))
                    warn_unused_conditions(*c, r.t);
                return r;
            }

            // The conditions of a schema whose type is not a struct, which takes them as yyjson::if_then_else and
            // yyjson::not_: those that test only the keys of objects hold for the values of the type that are not
            // objects, and the others are ignored with a warning
            void warn_unused_conditions(const json& list, const ctype& t)
            {
                const auto object_only = [&](const json* sub, const std::string& sloc) {
                    const auto* n = normalize_full(sub, sloc);
                    if (n->is_bool()) return n->b;
                    return n->is_object() && std::ranges::all_of(n->members, [&](const auto& m) {
                               return is_metadata_key(m.first) || is_object_key(m.first);
                           });
                };
                for (const auto& c : list.items)
                {
                    if (const auto* nt = c.find("not"); nt != nullptr && handled_nots_.contains(nt)) continue;
                    if (const auto* i = c.find("if"); i != nullptr && handled_ifs_.contains(i)) continue;
                    const auto* l = c.find("$loc");
                    const auto cloc = l != nullptr && l->is_string() ? l->text : std::string();
                    const auto* kw = c.has("if") ? "if" : c.has("not") ? "not" : "dependentSchemas";
                    if (!can_be_object(t))
                    {
                        if (c.has("dependentSchemas")) continue;
                        // `if` that only objects satisfy leaves `else` to a value of another kind
                        const auto* i = c.has("if") ? normalize_full(c.find("if"), cloc + "/if") : nullptr;
                        const auto types = i != nullptr && i->is_object() ? type_list(*i) : std::vector<std::string>();
                        const auto objects = !types.empty() && std::ranges::all_of(types, [](const auto& x) {
                            return x == "object";
                        });
                        if (i != nullptr && std::ranges::all_of(std::array{"then", "else"}, [&](const char* k) {
                                const auto* v = c.find(k);
                                return v == nullptr || (objects && k[0] == 't') || object_only(v, cloc + "/" + k);
                            }))
                            continue;
                    }
                    warn(cloc, "`" + std::string(kw) + "` is not supported on a value that is not read as a struct and is "
                               "ignored");
                }
            }

            // whether a value of the type may be a JSON object
            bool can_be_object(const ctype& t) const
            {
                const auto& r = resolved(t);
                switch (r.k)
                {
                    case tk::any:
                    case tk::object:
                    case tk::map:
                        return true;
                    case tk::named:
                        return is_struct(r);
                    case tk::optional:
                    case tk::indirect:
                    case tk::variant:
                        return std::ranges::any_of(r.args, [&](const ctype& a) { return can_be_object(a); });
                    default:
                        return false;
                }
            }

            // whether reading a JSON value into a C++ type succeeds, as far as the generator tells
            enum class fit : std::uint8_t
            {
                no,
                // it depends on `pattern`, `contains` or the format "regex", which the generator does not check
                unknown,
                yes
            };

            // how much of the values of one JSON kind a subschema accepts
            enum class reach : std::uint8_t
            {
                none,
                some,  // or the generator cannot tell
                all
            };

            // How much of the values of the JSON kind `kind` the subschema `sub` accepts: "null", "boolean", "object",
            // "array", "number", "integer" or "string"; "number" takes integers too
            reach reach_of(const json* sub, std::string_view kind, const std::string& loc, int depth = 0)
            {
                static const auto opaque = std::set<std::string_view>{"oneOf",
                                                                      "allOf",
                                                                      "$ref",
                                                                      "$dynamicRef",
                                                                      "$recursiveRef",
                                                                      "unevaluatedProperties",
                                                                      "unevaluatedItems",
                                                                      "if",
                                                                      "then",
                                                                      "else",
                                                                      "not",
                                                                      "dependentSchemas"};
                static const auto string_keys =
                    std::set<std::string_view>{"minLength", "maxLength", "pattern", "format"};
                static const auto number_keys = std::set<std::string_view>{"minimum", "maximum", "exclusiveMinimum",
                                                                           "exclusiveMaximum", "multipleOf"};
                static const auto array_keys =
                    std::set<std::string_view>{"items",    "prefixItems", "additionalItems", "contains",   "minItems",
                                               "maxItems", "uniqueItems", "minContains",     "maxContains"};
                static const auto object_keys = std::set<std::string_view>{
                    "properties",    "required",      "additionalProperties", "patternProperties",
                    "minProperties", "maxProperties", "propertyNames",        "dependentRequired"};
                const auto of_kind = [&](std::string_view k) {
                    if (kind == "string") return string_keys.contains(k);
                    if (kind == "number" || kind == "integer") return number_keys.contains(k);
                    if (kind == "array") return array_keys.contains(k);
                    if (kind == "object") return object_keys.contains(k);
                    return false;
                };
                const auto in_kind = [&](const json& v) {
                    if (kind == "null") return v.is_null();
                    if (kind == "boolean") return v.is_bool();
                    if (kind == "object") return v.is_object();
                    if (kind == "array") return v.is_array();
                    if (kind == "string") return v.is_string();
                    if (kind == "integer") return v.is_number() && v.is_integral();
                    return v.is_number();
                };
                const auto values = [&](std::span<const json> vs) {
                    const auto n = std::ranges::count_if(vs, in_kind);
                    if (n == 0) return reach::none;
                    // the one value of null, both values of boolean
                    if (kind == "null") return reach::all;
                    if (kind == "boolean" &&
                        std::ranges::any_of(vs, [](const json& v) { return v.is_bool() && v.b; }) &&
                        std::ranges::any_of(vs, [](const json& v) { return v.is_bool() && !v.b; }))
                        return reach::all;
                    return reach::some;
                };
                const auto* n = normalize_full(sub, loc);
                if (n->is_bool()) return n->b ? reach::all : reach::none;
                if (!n->is_object() || depth > 8) return reach::some;
                auto r = reach::all;
                for (const auto& [k, v] : n->members)
                {
                    if (is_metadata_key(k)) continue;
                    if (k == "type")
                    {
                        const auto types = type_list(*n);
                        const auto has = [&](std::string_view x) { return std::ranges::find(types, x) != types.end(); };
                        r = std::min(r, has(kind) || (kind == "integer" && has("number")) ? reach::all
                                        : kind == "number" && has("integer")              ? reach::some
                                                                                          : reach::none);
                    }
                    else if (k == "const")
                        r = std::min(r, values(std::span(&v, 1)));
                    else if (k == "enum")
                        r = std::min(r, v.is_array() ? values(v.items) : reach::some);
                    else if (k == "anyOf" && v.is_array())
                    {
                        auto any = reach::none;
                        for (std::size_t i = 0; i < v.items.size(); ++i)
                            any = std::max(any,
                                           reach_of(&v.items[i], kind, loc + "/anyOf/" + std::to_string(i), depth + 1));
                        r = std::min(r, any);
                    }
                    else if (k == "$conditions")
                    {
                        // `not` of `not`
                        const auto* c = v.is_array() && v.items.size() == 1 ? &v.items[0] : nullptr;
                        const auto* inner = c != nullptr ? c->find("not") : nullptr;
                        if (inner == nullptr || std::ranges::any_of(c->members, [](const auto& m) {
                                return m.first != "not" && m.first != "$loc";
                            }))
                            r = std::min(r, reach::some);
                        else
                        {
                            const auto* l = c->find("$loc");
                            const auto x = reach_of(inner, kind, (l != nullptr ? l->text : loc) + "/not", depth + 1);
                            r = std::min(r, x == reach::all    ? reach::none
                                            : x == reach::none ? reach::all
                                                               : reach::some);
                        }
                    }
                    else if (opaque.contains(k) || of_kind(k))
                        r = std::min(r, reach::some);
                    if (r == reach::none) break;
                }
                return r;
            }

            void expand_not(const json* sub, const std::string& nloc,
                            std::vector<std::pair<const json*, std::string>>& out, int depth = 0)
            {
                const auto* n = normalize_full(sub, nloc);
                const auto* any = n->is_object() ? n->find("anyOf") : nullptr;
                if (depth < 8 && any != nullptr && any->is_array() &&
                    std::ranges::all_of(
                        n->members, [](const auto& m) { return m.first == "anyOf" || is_metadata_key(m.first); }))
                {
                    for (std::size_t i = 0; i < any->items.size(); ++i)
                        expand_not(&any->items[i], nloc + "/anyOf/" + std::to_string(i), out, depth + 1);
                    return;
                }
                out.emplace_back(sub, nloc);
            }

            // The value-level `not` of the normalized schema `s`: the subschemas of the entries of "$conditions" that
            // hold `not`, with their locations.  `not` of anyOf is a `not` of each subschema.  The entries count as
            // handled (handled_nots_).
            std::vector<std::pair<const json*, std::string>> value_nots(const json& s)
            {
                auto out = std::vector<std::pair<const json*, std::string>>();
                const auto* list = s.find("$conditions");
                if (list == nullptr || !list->is_array()) return out;
                for (const auto& c : list->items)
                    if (const auto* nt = c.find("not"); nt != nullptr && !c.has("if"))
                    {
                        handled_nots_.insert(nt);
                        const auto* l = c.find("$loc");
                        expand_not(nt, (l != nullptr && l->is_string() ? l->text : std::string()) + "/not", out);
                    }
                return out;
            }

            // the type of a schema that accepts no value: yyjson::value with
            // yyjson::not_(yyjson::is_type<yyjson::value>), which rejects every value
            typed no_value()
            {
                auto n = settings();
                n.type_test = static_cast<int>(type_tests_.size());
                type_tests_.push_back(make(tk::any));
                auto r = typed();
                r.s.negated.push_back(std::move(n));
                return r;
            }

            // whether `t` is the type of no_value()
            bool is_no_value(const typed& t) const
            {
                if (t.t.k != tk::any || t.s.negated.size() != 1) return false;
                auto rest = t.s;
                auto n = std::move(rest.negated.front());
                rest.negated.clear();
                const auto test = std::exchange(n.type_test, -1);
                return rest.empty() && n.empty() && test >= 0 &&
                       type_tests_[static_cast<std::size_t>(test)].k == tk::any;
            }

            // yyjson::is_type<T>, with T the type of `sub` for values of `kind` (any kind when empty); nullopt when T
            // reads more than `sub` accepts
            std::optional<settings> type_test_of(const json* sub, const std::string& nloc, const std::string& hint,
                                                 std::string_view kind)
            {
                if (!kind.empty())
                    if (const auto* n = normalize_full(sub, nloc); n->is_object())
                    {
                        auto copy = *n;
                        copy.set("type", json::string_of(kind));
                        sub = store_merged(std::move(copy));
                    }
                const auto calls = warn_calls_;
                auto t = type_of(sub, nloc, hint);
                if (warn_calls_ != calls || (t.t.k == tk::any && t.s.empty())) return std::nullopt;
                auto s = settings();
                s.type_test = static_cast<int>(type_tests_.size());
                type_tests_.push_back(constrained(std::move(t)));
                return s;
            }

            // yyjson::not_(yyjson::is_type<T>) on `r`, with T the type of `sub` for values of `kind` (any kind when
            // empty); left out with a warning when T reads more than `sub` accepts
            void add_type_test(typed& r, const json* sub, const std::string& nloc, const std::string& hint,
                               std::string_view kind)
            {
                auto s = type_test_of(sub, nloc, hint + "Not", kind);
                if (!s)
                {
                    warn(nloc, "`not` of a schema that no C++ type expresses exactly is not supported and is ignored");
                    return;
                }
                r.s.negated.push_back(std::move(*s));
            }

            // the value-level `not` of a value read as `r`, of the JSON kind `kind` of the schema
            void add_nots(typed& r, std::string_view kind, const std::vector<std::pair<const json*, std::string>>& nots,
                          const std::string& hint)
            {
                for (const auto& [sub, nloc] : nots)
                {
                    if (reach_of(sub, kind, nloc) != reach::some) continue;
                    const auto drop = [&]() {
                        warn(nloc,
                             "`not` cannot be expressed on a value of type " + std::string(kind) + "; it is ignored");
                    };
                    const auto& t = r.t;
                    if (t.k == tk::object || t.k == tk::any)
                    {
                        add_type_test(r, sub, nloc, hint, kind);
                        continue;
                    }
                    // element trees are not settings of yyjson::not_; contains is, but not on a tuple
                    const auto* n = normalize_full(sub, nloc);
                    const auto any_key = [&](std::initializer_list<std::string_view> keys) {
                        return n->is_object() && std::ranges::any_of(keys, [&](auto k) { return n->has(k); });
                    };
                    if ((t.k == tk::vector || t.k == tk::tuple) && any_key({"items", "prefixItems", "uniqueItems"}))
                    {
                        drop();
                        continue;
                    }
                    if (t.k == tk::tuple && any_key({"contains", "minContains", "maxContains"}))
                    {
                        drop();
                        continue;
                    }
                    if (t.k == tk::map && any_key({"additionalProperties", "propertyNames"}))
                    {
                        drop();
                        continue;
                    }
                    const auto calls = warn_calls_;
                    auto set = settings_for(*sub, t, nloc);
                    if (!set || set->empty() || warn_calls_ != calls)
                        drop();
                    else
                        r.s.negated.push_back(std::move(*set));
                }
            }

            // the type of a schema without a type whose `not` tells the values it rejects: yyjson::value with
            // yyjson::not_(yyjson::is_type<T>)
            typed type_not_any(const std::vector<std::pair<const json*, std::string>>& nots, const std::string& hint)
            {
                static constexpr auto kinds =
                    std::array<std::string_view, 6>{"null", "boolean", "object", "array", "number", "string"};
                auto r = typed();
                for (const auto& [sub, nloc] : nots)
                {
                    auto all = true;
                    auto none = true;
                    for (const auto k : kinds)
                    {
                        const auto x = reach_of(sub, k, nloc);
                        all = all && x == reach::all;
                        none = none && x == reach::none;
                    }
                    if (none) continue;
                    if (all)
                    {
                        warn(nloc,
                             "`not` rejects every value: the schema accepts no value, and reading it always fails");
                        return no_value();
                    }
                    add_type_test(r, sub, nloc, hint, {});
                }
                return r;
            }

            // a value-level `if` of a schema that is not a struct, with its `then` and `else` (nullptr when absent)
            // and the location of the schema
            struct value_if
            {
                const json* if_ = nullptr;
                const json* then_ = nullptr;
                const json* else_ = nullptr;
                std::string loc;
            };

            // The value-level `if` of the normalized schema `s`: the entries of "$conditions" that hold `if`.  The
            // entries count as handled (handled_ifs_).
            std::vector<value_if> value_ifs(const json& s)
            {
                auto out = std::vector<value_if>();
                const auto* list = s.find("$conditions");
                if (list == nullptr || !list->is_array()) return out;
                for (const auto& c : list->items)
                    if (const auto* i = c.find("if"))
                    {
                        handled_ifs_.insert(i);
                        const auto* l = c.find("$loc");
                        out.push_back({.if_ = i,
                                       .then_ = c.find("then"),
                                       .else_ = c.find("else"),
                                       .loc = l != nullptr && l->is_string() ? l->text : std::string()});
                    }
                return out;
            }

            // how much of the values of the JSON kind `kind` the subschema `sub` accepts, of any kind when `kind` is
            // empty; all when `sub` is nullptr, an absent `then` or `else`
            reach reach_in(const json* sub, std::string_view kind, const std::string& loc)
            {
                if (sub == nullptr) return reach::all;
                if (!kind.empty()) return reach_of(sub, kind, loc);
                auto all = true;
                auto none = true;
                for (const auto* k : {"null", "boolean", "object", "array", "number", "string"})
                {
                    const auto x = reach_of(sub, k, loc);
                    all = all && x == reach::all;
                    none = none && x == reach::none;
                }
                return all ? reach::all : none ? reach::none : reach::some;
            }

            // how much of the values of the JSON kind `kind` the `if` with its `then` and `else` accepts
            reach if_reach(const value_if& c, std::string_view kind)
            {
                const auto ri = reach_in(c.if_, kind, c.loc + "/if");
                const auto rt = reach_in(c.then_, kind, c.loc + "/then");
                const auto re = reach_in(c.else_, kind, c.loc + "/else");
                if (ri == reach::all) return rt;
                if (ri == reach::none) return re;
                return rt == re && rt != reach::some ? rt : reach::some;
            }

            // whether the library takes null, which an empty value of a type that holds a value or nothing is, to
            // satisfy the settings: the settings other than `const_` and `enum_` without nullptr hold for it, and
            // yyjson::not_ and yyjson::if_then_else see it as null
            static bool settings_hold_null(const settings& s)
            {
                if (s.type_test >= 0) return false;
                for (const auto& c : s.constraints)
                    if ((c.keyword == "const" || c.keyword == "enum") && !c.null) return false;
                for (const auto& n : s.negated)
                    if (settings_hold_null(n)) return false;
                for (std::size_t i = 0; i + 2 < s.conditional.size(); i += 3)
                    if (!settings_hold_null(settings_hold_null(s.conditional[i]) ? s.conditional[i + 1] : s.conditional[i + 2]))
                        return false;
                return true;
            }

            // The value-level `if` of a value read as `r`, of the JSON kind `kind` of the schema, or of any kind when
            // `kind` is empty.  A branch that the condition always or never takes joins the settings of the value; a
            // branch that rejects every value of the kind makes yyjson::not_ of the condition or joins it; the others
            // make yyjson::if_then_else.  `nullable`: the settings also see null, which make_nullable() adds after
            // them; an `if` whose settings reject null, which the schema accepts, is left out with a warning.
            void add_ifs(typed& r, std::string_view kind, const std::vector<value_if>& ifs, const std::string& hint,
                         bool nullable)
            {
                const auto testable = [](const settings& s) {
                    return s.items.empty() && s.property_names.empty() && s.additional.empty() &&
                           !s.unique && s.negated.empty() && s.conditional.empty();
                };
                for (const auto& c : ifs)
                {
                    const auto ri = reach_in(c.if_, kind, c.loc + "/if");
                    const auto rt = reach_in(c.then_, kind, c.loc + "/then");
                    const auto re = reach_in(c.else_, kind, c.loc + "/else");
                    // the kind is left out of the type
                    if (ri == reach::some && rt == reach::none && re == reach::none) continue;
                    const auto drop = [&](const char* key, const std::string& why) {
                        warn(c.loc + "/" + key, why + "; the `if` is ignored");
                    };
                    // the settings of a branch, empty where it accepts every value of the kind
                    const auto branch = [&](const json* sub, const char* key, reach x) -> std::optional<settings> {
                        if (x == reach::all) return settings();
                        const auto calls = warn_calls_;
                        auto set = settings_for(*sub, r.t, c.loc + "/" + key);
                        if (!set || warn_calls_ != calls)
                        {
                            set.reset();
                            if (r.t.k == tk::any || r.t.k == tk::object)
                                set = type_test_of(sub, c.loc + "/" + key, hint + pascal(key), kind);
                        }
                        if (!set)
                            drop(key, std::string("`") + key + "` is not expressed by the settings of a value");
                        return set;
                    };
                    auto add = settings();
                    if (ri != reach::some)
                    {
                        auto b = ri == reach::all ? branch(c.then_, "then", rt) : branch(c.else_, "else", re);
                        if (!b) continue;
                        add = std::move(*b);
                    }
                    else
                    {
                        // the condition
                        const auto calls = warn_calls_;
                        auto test = settings_for(*c.if_, r.t, c.loc + "/if");
                        if (!test || warn_calls_ != calls || !testable(*test))
                        {
                            test.reset();
                            if ((r.t.k == tk::any || r.t.k == tk::object) && !nullable)
                                test = type_test_of(c.if_, c.loc + "/if", hint + "If", kind);
                        }
                        if (!test || test->empty())
                        {
                            drop("if", "`if` is not expressed by the settings that yyjson::if_ takes");
                            continue;
                        }
                        auto t = branch(c.then_, "then", rt == reach::none ? reach::all : rt);
                        auto e = branch(c.else_, "else", re == reach::none ? reach::all : re);
                        if (!t || !e) continue;
                        if (rt == reach::none)
                        {
                            // a value that satisfies the condition is rejected, and every other value takes `else`
                            e->negated.push_back(std::move(*test));
                            add = std::move(*e);
                        }
                        else if (re == reach::none)
                        {
                            auto j = join_settings(std::move(*test), *t);
                            if (!j)
                            {
                                drop("then", "`then` sets a keyword of `if` to another value");
                                continue;
                            }
                            add = std::move(*j);
                        }
                        else if (!t->empty() || !e->empty())
                        {
                            add.conditional.push_back(std::move(*test));
                            add.conditional.push_back(std::move(*t));
                            add.conditional.push_back(std::move(*e));
                        }
                    }
                    if (add.empty()) continue;
                    if (nullable && !settings_hold_null(add))
                    {
                        drop("if", "`if` cannot be expressed on a value that may be null");
                        continue;
                    }
                    auto j = join_settings(r.s, add);
                    if (!j)
                    {
                        drop("if", "a branch of `if` sets a keyword of the value to another value");
                        continue;
                    }
                    r.s = std::move(*j);
                }
            }

            // The keys that the conditions of the normalized object schema `s` name, with the locations that name
            // them: the keys of `properties` and `required` of `if`, `then`, `else` and `not` (and of the
            // subschemas of `anyOf` of `not`), the keys of `dependentSchemas` and those that their schemas name
            std::vector<std::pair<std::string, std::string>> condition_keys(const json& s, int depth = 0)
            {
                auto out = std::vector<std::pair<std::string, std::string>>();
                const auto* list = s.find("$conditions");
                if (list == nullptr || !list->is_array() || depth > 8) return out;
                const auto add = [&](const std::string& k, const std::string& kloc) {
                    if (std::ranges::none_of(out, [&](const auto& e) { return e.first == k; })) out.emplace_back(k, kloc);
                };
                const auto names = [&](const json* sub, const std::string& sloc) {
                    const auto* n = normalize_full(sub, sloc);
                    if (!n->is_object()) return;
                    if (const auto* p = n->find("properties"); p != nullptr && p->is_object())
                        for (const auto& [k, v] : p->members) add(k, sloc + "/properties/" + pointer_token(k));
                    if (const auto* r = n->find("required"); r != nullptr && r->is_array())
                        for (const auto& e : r->items)
                            if (e.is_string()) add(e.text, sloc + "/required");
                    for (const auto& [k, l] : condition_keys(*n, depth + 1)) add(k, l);
                };
                for (const auto& c : list->items)
                {
                    const auto* l = c.find("$loc");
                    const auto cloc = l != nullptr && l->is_string() ? l->text : std::string();
                    for (const auto* k : {"if", "then", "else"})
                        if (const auto* v = c.find(k)) names(v, cloc + "/" + k);
                    if (const auto* v = c.find("not"))
                    {
                        names(v, cloc + "/not");
                        const auto* n = normalize_full(v, cloc + "/not");
                        if (const auto* any = n->is_object() ? n->find("anyOf") : nullptr; any != nullptr && any->is_array())
                            for (std::size_t i = 0; i < any->items.size(); ++i)
                                names(&any->items[i], cloc + "/not/anyOf/" + std::to_string(i));
                    }
                    if (const auto* d = c.find("dependentSchemas"); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                        {
                            const auto dloc = cloc + "/dependentSchemas/" + pointer_token(k);
                            add(k, dloc);
                            names(&v, dloc);
                        }
                }
                return out;
            }

            typed type_body(const json* s0, const std::string& loc, const std::string& hint, bool named)
            {
                const auto* s = normalize(s0, loc);
                if (s->is_bool() && !s->b)
                {
                    // `false` as written, or as `{"not": {}}`, rejects every value on purpose; keywords that no value
                    // satisfies together are likely a mistake of the schema
                    const auto* nt = s0->is_object() ? s0->find("not") : nullptr;
                    const auto idiom =
                        nt != nullptr && adds_nothing(*nt) && std::ranges::all_of(s0->members, [](const auto& m) {
                            return m.first == "not" || is_metadata_key(m.first);
                        });
                    if (!s0->is_bool() && !idiom && !reported_false_.contains(s0))
                        warn(loc, "no value satisfies the keywords of the schema together; reading it always fails");
                    return no_value();
                }
                if (!s->is_object()) return {};
                warn_unsupported(*s, loc);
                if (const auto* ref = s->find("$ref"))
                {
                    const auto* t = resolve(*ref, loc);
                    if (t == nullptr) return {};
                    return type_of(t, ref->text, hint);
                }
                if ((s->has("anyOf") && !required_groups_of(*s, "anyOf")) ||
                    (s->has("oneOf") && !required_groups_of(*s, "oneOf")))
                    return type_variant(s0, s, loc, hint, named);
                // `not` of the value: a kind whose every value it rejects is left out, and the others take
                // yyjson::not_ (a struct takes it from conditions_of())
                const auto nots = value_nots(*s);
                // `if` of the value: a kind whose every value it rejects is left out, and the others take the
                // settings of its branches or yyjson::if_then_else (a struct takes it from conditions_of())
                const auto ifs = value_ifs(*s);
                if (s->has("const") || s->has("enum"))
                {
                    if (!nots.empty() || !ifs.empty()) return type_listed_not(s0, *s, nots, ifs, loc, hint, named);
                    return s->has("const") ? type_const(s0, *s, loc, hint, named) : type_enum(s0, *s, loc, hint, named);
                }
                auto types = types_of(*s, loc);
                auto nullable = std::erase(types, "null") > 0;
                if (!nots.empty() || !ifs.empty())
                {
                    const auto rejected = [&](std::string_view kind) {
                        return std::ranges::any_of(
                                   nots,
                                   [&](const auto& n) { return reach_of(n.first, kind, n.second) == reach::all; }) ||
                               std::ranges::any_of(ifs, [&](const auto& c) { return if_reach(c, kind) == reach::none; });
                    };
                    if (types.empty() && !nullable)
                    {
                        // a value of one kind is read as that kind, others as yyjson::value with is_type tests
                        auto left = std::vector<std::string>();
                        for (const auto* k : {"null", "boolean", "object", "array", "number", "string"})
                            if (!rejected(k)) left.emplace_back(k);
                        if (left.empty() && !ifs.empty())
                        {
                            warn(loc, "`if` and `not` reject every value: the schema accepts no value, and reading it "
                                      "always fails");
                            return no_value();
                        }
                        if (left.size() != 1)
                        {
                            auto r = nots.empty() ? typed() : type_not_any(nots, hint);
                            add_ifs(r, {}, ifs, hint, false);
                            return r;
                        }
                        if (left[0] == "null") return {make(tk::null), {}};
                        types = std::move(left);
                    }
                    if (nullable && rejected("null"))
                        nullable = false;
                    else if (nullable)
                    {
                        for (const auto& [sub, nloc] : nots)
                            if (reach_of(sub, "null", nloc) == reach::some)
                                warn(nloc, "`not` cannot be expressed on null; it is ignored");
                        for (const auto& c : ifs)
                            if (if_reach(c, "null") == reach::some)
                                warn(c.loc + "/if", "`if` cannot be expressed on null; it is ignored for null");
                    }
                    std::erase_if(types, [&](const std::string& k) { return rejected(k); });
                    if (types.empty() && !nullable)
                    {
                        warn(loc, std::string(nots.empty()  ? "`if` rejects"
                                              : ifs.empty() ? "`not` rejects"
                                                            : "`if` and `not` reject") +
                                      " every value of the type: the schema accepts no value, and reading it always "
                                      "fails");
                        return no_value();
                    }
                }
                if (types.empty()) return nullable ? typed{make(tk::null), {}} : typed{};
                if (types.size() == 1)
                    return type_kind(types[0], s0, *s, loc, hint, named, nullable, true, nots, ifs);
                auto alts = std::vector<typed>();
                if (nullable) alts.push_back({make(tk::monostate), {}});
                for (const auto& k : types)
                    alts.push_back(type_kind(k, s0, *s, loc, hint + pascal(k), false, false, false, nots, ifs));
                return make_variant(std::move(alts), loc);
            }

            // The schema `s` of the values of the JSON kind `kind` (object or array) with the first `if` of `ifs` that
            // every value or no value of the kind satisfies replaced by the branch that the values take, merged into
            // the schema as a part of `allOf`. An `if` whose `else` rejects every value of the kind is replaced by
            // `if` and `then` in the same way. nullptr when every `if` tests the values
            const json* fold_if(const json& s, const std::vector<value_if>& ifs, std::string_view kind,
                                const std::string& loc)
            {
                if (folding_ > 8) return nullptr;
                for (const auto& c : ifs)
                {
                    const auto ri = reach_in(c.if_, kind, c.loc + "/if");
                    const auto all_if = ri == reach::some && c.else_ != nullptr &&
                                        reach_in(c.else_, kind, c.loc + "/else") == reach::none &&
                                        reach_in(c.then_, kind, c.loc + "/then") != reach::none;
                    if (ri == reach::some && !all_if) continue;
                    auto rest = s;
                    for (auto& [k, v] : rest.members)
                        if (k == "$conditions")
                        {
                            const auto& list = s.find("$conditions")->items;
                            const auto at =
                                std::ranges::find_if(list, [&](const json& e) { return e.find("if") == c.if_; });
                            if (at != list.end()) v.items.erase(v.items.begin() + (at - list.begin()));
                        }
                    if (const auto* list = rest.find("$conditions"); list != nullptr && list->items.empty())
                        rest.erase("$conditions");
                    auto parts = json::array_of({std::move(rest)});
                    if (all_if) parts.items.push_back(*c.if_);
                    if (const auto* branch = ri == reach::none ? c.else_ : c.then_) parts.items.push_back(*branch);
                    if (parts.items.size() == 1) return store_merged(std::move(parts.items[0]));
                    auto j = json::object_of();
                    j.set("allOf", std::move(parts));
                    const auto* f = normalize(store_merged(std::move(j)), loc);
                    if (!f->is_object()) return nullptr;
                    const auto types = type_list(*f);
                    return types.empty() || std::ranges::find(types, kind) != types.end() ? f : nullptr;
                }
                return nullptr;
            }

            // `whole`: the type of the kind is the type of the schema, not one alternative of several kinds. `nots`:
            // the value-level `not` of the schema
            typed type_kind(const std::string& kind, const json* s0, const json& s, const std::string& loc,
                            const std::string& hint, bool named, bool nullable, bool whole = true,
                            const std::vector<std::pair<const json*, std::string>>& nots = {},
                            const std::vector<value_if>& ifs = {})
            {
                auto r = typed();
                if (kind == "object")
                {
                    if (const auto* f = fold_if(s, ifs, kind, loc))
                    {
                        // a branch that refers to a schema of objects alone is the type of that schema
                        if (is_pure_ref(*f))
                        {
                            const auto* n = normalize_full(f, loc);
                            if (n->is_object() && type_list(*n) == std::vector<std::string>{"object"})
                            {
                                conditions_used_.insert(&s);
                                r = type_of(f, loc, hint);
                                return nullable ? make_nullable(std::move(r)) : r;
                            }
                            f = n;
                        }
                        ++folding_;
                        r = type_kind(kind, s0, *f, loc, hint, named, nullable, whole, nots, value_ifs(*f));
                        --folding_;
                        if (conditions_used_.contains(f) || !f->has("$conditions")) conditions_used_.insert(&s);
                        return r;
                    }
                    const auto* dep = s.find("dependentRequired");
                    const auto dependent =
                        dep != nullptr && dep->is_object() && std::ranges::any_of(dep->members, [](const auto& m) {
                            return m.second.is_array() && !m.second.items.empty();
                        });
                    if (s.has("properties") || s.has("required") || dependent || !condition_keys(s).empty())
                        return make_struct(s0, s, loc, hint, named, nullable, whole);
                    // patterns of different schemas: a struct of a field of yyjson::pattern_properties for each
                    if (const auto* pp = s.find("patternProperties"); opts_.pattern && pp != nullptr &&
                                                                     pp->is_object() && !pp->members.empty() &&
                                                                     !pattern_map_of(s, loc))
                        return make_struct(s0, s, loc, hint, named, nullable, whole);
                    r = type_map(s, loc, hint);
                }
                else if (kind == "array")
                {
                    if (const auto* f = fold_if(s, ifs, kind, loc))
                    {
                        // a branch that refers to a schema of arrays alone is the type of that schema
                        if (is_pure_ref(*f))
                        {
                            const auto* n = normalize_full(f, loc);
                            if (n->is_object() && type_list(*n) == std::vector<std::string>{"array"})
                            {
                                r = type_of(f, loc, hint);
                                return nullable ? make_nullable(std::move(r)) : r;
                            }
                            f = n;
                        }
                        ++folding_;
                        r = type_kind(kind, s0, *f, loc, hint, named, nullable, whole, nots, value_ifs(*f));
                        --folding_;
                        return r;
                    }
                    r = type_array(s0, s, loc, hint, named, !ifs.empty());
                }
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
                if (!nots.empty()) add_nots(r, kind, nots, hint);
                if (!ifs.empty()) add_ifs(r, kind, ifs, hint, nullable);
                return nullable ? make_nullable(std::move(r)) : r;
            }

            typed make_struct(const json* s0, const json& s, const std::string& loc, const std::string& hint,
                              bool named, bool nullable, bool whole)
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
                // a reference to the schema from inside the struct is the struct only when the struct is its type; one
                // alternative of several leaves it to type_of(), which holds the whole through a transparent struct
                if (whole) cache_[s0] = r;
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
                if (std::ranges::any_of(names.constraints, [&](const auto& c) {
                        return c.keyword == "pattern" && c.value.text == pattern;
                    }))
                    return;
                names.constraints.push_back(
                    {"yyjson::pattern(" + cpp_string(pattern) + ")", "pattern", json::string_of(pattern)});
                uses_pattern_ = true;
            }

            // whether the schema `s` is `x`, or `allOf` with `x` among its schemas, as `beside` gives a property that
            // another schema of `allOf` declared
            static bool includes(const json& s, const json& x)
            {
                if (equal(s, x)) return true;
                const auto* all = s.find("allOf");
                return all != nullptr && all->is_array() &&
                       std::ranges::any_of(all->items, [&](const json& i) { return includes(i, x); });
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
                        if (own == nullptr) own = f.schema;
                        if (own == nullptr) return false;
                        // an object never has the key of a property whose schema is `false`
                        if (const auto* n = normalize_full(own, f.loc); n->is_bool() && !n->b) return true;
                        return includes(*own, sub);
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

            // The settings of the keys of an object from the schema `s` of `propertyNames`, which sees only strings:
            // a subschema of `anyOf` / `oneOf` that accepts no string is left out, one that accepts every string
            // leaves the keys unchecked under `anyOf`, subschemas of `const` / `enum` are one `enum`, and `not` is
            // yyjson::not_.  Nothing when string settings cannot express the schema.
            std::optional<settings> key_settings(const json& s, const std::string& loc, int depth = 0)
            {
                const auto* n = normalize_full(&s, loc);
                switch (reach_of(n, "string", loc))
                {
                    case reach::none:
                        return std::nullopt;
                    case reach::all:
                        return settings();
                    case reach::some:
                        break;
                }
                if (!n->is_object() || depth > 8) return std::nullopt;
                const auto both = [](settings a, const settings& b) {
                    std::ranges::copy(b.constraints, std::back_inserter(a.constraints));
                    std::ranges::copy(b.negated, std::back_inserter(a.negated));
                    std::ranges::copy(b.conditional, std::back_inserter(a.conditional));
                    return a;
                };
                const auto* any = n->find("anyOf");
                const auto* one = n->find("oneOf");
                if (any != nullptr || one != nullptr)
                {
                    const auto* list = any != nullptr ? any : one;
                    const auto* kw = any != nullptr ? "anyOf" : "oneOf";
                    if ((any != nullptr && one != nullptr) || !list->is_array()) return std::nullopt;
                    auto rest_schema = *n;
                    rest_schema.erase(kw);
                    const auto rest = key_settings(*store_merged(std::move(rest_schema)), loc, depth + 1);
                    if (!rest) return std::nullopt;
                    auto alternatives = std::vector<settings>();
                    auto values = std::vector<json>();
                    auto listed = true;
                    for (std::size_t i = 0; i < list->items.size(); ++i)
                    {
                        const auto bloc = loc + "/" + kw + "/" + std::to_string(i);
                        const auto* b = &list->items[i];
                        const auto r = reach_of(b, "string", bloc);
                        if (r == reach::none) continue;
                        auto e = r == reach::all ? std::optional<settings>(settings())
                                                 : key_settings(*b, bloc, depth + 1);
                        if (!e) return std::nullopt;
                        if (e->empty())
                        {
                            // every string, of which `oneOf` takes those that no other subschema accepts
                            if (one != nullptr) return std::nullopt;
                            return rest;
                        }
                        const auto& c = e->constraints;
                        if (e->negated.empty() && c.size() == 1 && (c[0].keyword == "const" || c[0].keyword == "enum"))
                        {
                            for (const auto& v : c[0].keyword == "const" ? std::span(&c[0].value, 1)
                                                                        : std::span(c[0].value.items))
                            {
                                if (!v.is_string()) continue;
                                if (std::ranges::any_of(values, [&](const json& x) { return equal(x, v); }))
                                {
                                    // under `oneOf`, a string that two subschemas accept is not a key
                                    if (one != nullptr) return std::nullopt;
                                    continue;
                                }
                                values.push_back(v);
                            }
                        }
                        else
                            listed = false;
                        alternatives.push_back(std::move(*e));
                    }
                    if (alternatives.empty()) return std::nullopt;
                    if (alternatives.size() == 1) return both(*rest, alternatives[0]);
                    if (!listed) return std::nullopt;
                    auto e = json::object_of();
                    e.set("type", json::string_of("string"));
                    e.set("enum", json::array_of(std::move(values)));
                    const auto u = settings_for(*store_merged(std::move(e)), make(tk::string), loc);
                    if (!u) return std::nullopt;
                    return both(*rest, *u);
                }
                if (const auto* list = n->find("$conditions"))
                {
                    if (!list->is_array()) return std::nullopt;
                    auto rest_schema = *n;
                    rest_schema.erase("$conditions");
                    auto out = key_settings(*store_merged(std::move(rest_schema)), loc, depth + 1);
                    if (!out) return std::nullopt;
                    for (const auto& c : list->items)
                    {
                        // `dependentSchemas` applies to objects only
                        if (c.has("dependentSchemas")) continue;
                        const auto* nt = c.find("not");
                        if (nt == nullptr || c.has("if")) return std::nullopt;
                        const auto* l = c.find("$loc");
                        auto nots = std::vector<std::pair<const json*, std::string>>();
                        expand_not(nt, (l != nullptr && l->is_string() ? l->text : loc) + "/not", nots);
                        for (const auto& [sub, nloc] : nots)
                        {
                            const auto r = reach_of(sub, "string", nloc);
                            if (r == reach::none) continue;
                            if (r == reach::all) return std::nullopt;
                            auto e = key_settings(*sub, nloc, depth + 1);
                            if (!e || e->empty()) return std::nullopt;
                            out->negated.push_back(std::move(*e));
                        }
                    }
                    return out;
                }
                return settings_for(*n, make(tk::string), loc);
            }

            // whether the key `k` satisfies the settings `s` of keys: unknown when a pattern that std::regex does
            // not read or a format that the generator does not check decides it
            fit key_fit(const std::string& k, const settings& s) const
            {
                auto r = fit::yes;
                auto rest = s;
                rest.negated.clear();
                std::erase_if(rest.constraints, [&](const settings::constraint& c) {
                    if (c.keyword != "pattern") return false;
                    try
                    {
                        if (!std::regex_search(k, std::regex(c.value.text, std::regex::ECMAScript))) r = fit::no;
                    }
                    catch (const std::regex_error&)
                    {
                        r = std::min(r, fit::unknown);
                    }
                    return true;
                });
                if (r == fit::no) return r;
                r = std::min(r, fits(json::string_of(k), make(tk::string), rest));
                for (const auto& e : s.negated)
                {
                    if (r == fit::no) break;
                    const auto inner = key_fit(k, e);
                    r = inner == fit::yes ? fit::no : inner == fit::unknown ? std::min(r, fit::unknown) : r;
                }
                return r;
            }

            // The type of the key `k`, which no property of the object schema `s` declares but another keyword
            // names, by what `s` applies to such a key: the schemas of the patterns of `patternProperties` that `k`
            // matches, else `additionalProperties`; any JSON value where neither says anything, and nullopt where
            // `additionalProperties` is false, which rejects the key.  `loc` is the location of `s`; `schema` is set
            // to the schema of the type.
            std::optional<typed> undeclared_key_type(const json& s, const std::string& k, const std::string& loc,
                                                     const std::string& hint, const json*& schema)
            {
                schema = nullptr;
                if (const auto* pp = s.find("patternProperties"); pp != nullptr && pp->is_object())
                {
                    auto matched = std::vector<std::pair<const std::string*, const json*>>();
                    for (const auto& [p, sub] : pp->members)
                    {
                        try
                        {
                            if (std::regex_search(k, std::regex(p, std::regex::ECMAScript)))
                                matched.emplace_back(&p, &sub);
                        }
                        catch (const std::regex_error&)
                        {
                            warn(loc + "/patternProperties/" + pointer_token(p),
                                 "the key \"" + k +
                                     "\" is read as any JSON value: std::regex does not read the pattern");
                            return typed{};
                        }
                    }
                    if (!matched.empty())
                    {
                        // a pattern whose schema is `false` rejects the key
                        if (const auto f = std::ranges::find_if(
                                matched, [](const auto& m) { return m.second->is_bool() && !m.second->b; });
                            f != matched.end())
                        {
                            schema = f->second;
                            return std::nullopt;
                        }
                        const auto same = [&](const auto& m) { return equal(*m.second, *matched[0].second); };
                        if (!std::ranges::all_of(matched, same))
                        {
                            // the fields of yyjson::pattern_properties check the value against each schema
                            if (!opts_.pattern)
                                warn(loc + "/patternProperties", "the key \"" + k + "\" matches patterns with different "
                                                                  "schemas; it is read as any JSON value");
                            return typed{};
                        }
                        schema = matched[0].second;
                        return type_of(schema, loc + "/patternProperties/" + pointer_token(*matched[0].first), hint);
                    }
                }
                const auto* ap = s.find("additionalProperties");
                if (ap == nullptr || accepts_anything(*ap)) return typed{};
                if (ap->is_bool()) return std::nullopt;
                schema = ap;
                return type_of(ap, loc + "/additionalProperties", hint);
            }

            // the type of the field of a property or a position, with its default. `required`: the key or the
            // position is always there
            void place_default(field& f, ctype t, const json* dflt, bool required) const
            {
                if (required)
                {
                    // the default of a required key is never used by reading; it initializes the member
                    if (dflt != nullptr) f.dflt = *dflt;
                    f.type = std::move(t);
                    f.required = true;
                }
                else if (dflt == nullptr || (dflt->is_null() && !holds_null(t)))
                {
                    // no default, or `null` for a type without null, which authors write for "unset": the missing
                    // key is std::nullopt
                    f.type = make(tk::optional, {std::move(t)});
                }
                else if (opts_.optional_defaults)
                {
                    // std::nullopt is the missing key, which the program reads as the default in the comment
                    f.default_note = dump(*dflt);
                    f.type = make(tk::optional, {std::move(t)});
                }
                else
                {
                    // convert_defaults() makes it the initializer, once the structs it may construct are known, or
                    // makes the field std::optional
                    f.dflt = *dflt;
                    f.type = std::move(t);
                }
            }

            // The fields of yyjson::pattern_properties of the object schema `s`, one for the patterns of each
            // schema of `patternProperties`, whose patterns it joins.  A pattern whose schema accepts any value is
            // left out where the struct accepts the keys that name no property anyway.  The properties of `fields`
            // whose keys a pattern of the schema `false` matches go to `absent`; the other keys that it matches go to
            // a field whose values accept nothing, which is left out when `additionalProperties` is false.
            template <typename IsRequired>
            std::vector<field> pattern_fields(const json& s, const std::vector<field>& fields,
                                              std::vector<condition>& absent, const IsRequired& is_required, int idx,
                                              const std::string& loc)
            {
                const auto& pp = *s.find("patternProperties");
                const auto* ap = s.find("additionalProperties");
                const auto open = ap == nullptr || accepts_anything(*ap);
                const auto closed = ap != nullptr && ap->is_bool() && !ap->b;
                const auto* props = s.find("properties");
                struct group
                {
                    std::vector<std::string> patterns;
                    const json* schema;
                    std::string loc;
                };
                auto groups = std::vector<group>();
                for (const auto& [p, sub] : pp.members)
                {
                    const auto ploc = loc + "/patternProperties/" + pointer_token(p);
                    if (const auto* target = normalize_full(&sub, ploc); target->is_bool() && !target->b)
                    {
                        try
                        {
                            const auto re = std::regex(p, std::regex::ECMAScript);
                            for (const auto& f : fields)
                            {
                                if (f.extra || props == nullptr || props->find(f.key) == nullptr ||
                                    !std::regex_search(f.key, re))
                                    continue;
                                if (std::ranges::any_of(absent, [&](const condition& c) {
                                        return c.if_.size() == 1 && !c.if_.front().counts &&
                                               c.if_.front().key == f.key;
                                    }))
                                    continue;
                                if (is_required(f.key))
                                    warn(f.loc, "the key \"" + f.key +
                                                    "\" matches a pattern of `patternProperties` whose schema is "
                                                    "`false`; no object satisfies the schema");
                                absent.push_back(condition{
                                    .negated = true, .if_ = {cond_field{.key = f.key, .required = true}}, .loc = f.loc});
                            }
                        }
                        catch (const std::regex_error&)
                        {
                            warn(ploc, "the pattern is not checked against the keys of the properties: std::regex "
                                       "does not read it");
                        }
                        // the other keys that it matches go to a field whose values accept nothing
                        if (closed) continue;
                    }
                    if (open && accepts_anything(sub)) continue;
                    if (auto it = std::ranges::find_if(groups, [&](const group& g) { return equal(*g.schema, sub); });
                        it != groups.end())
                        it->patterns.push_back(p);
                    else
                        groups.push_back(group{.patterns = {p}, .schema = &sub, .loc = ploc});
                }
                auto out = std::vector<field>();
                auto n = 0;
                for (const auto& g : groups)
                {
                    ++n;
                    auto name = std::string();
                    if (const auto* t = g.schema->find("title"); t != nullptr && t->is_string()) name = snake(t->text);
                    if (const auto* r = g.schema->find("$ref"); name.empty() && r != nullptr && r->is_string())
                        name = snake(std::string_view(r->text).substr(r->text.rfind('/') + 1));
                    if (name.empty() || !(name.front() >= 'a' && name.front() <= 'z'))
                        name = "pattern" + std::to_string(n);
                    auto t = type_of(g.schema, g.loc, decls_[idx].name + pascal(name));
                    auto pattern = std::string();
                    for (const auto& p : g.patterns)
                        pattern += g.patterns.size() == 1 ? p : (pattern.empty() ? "" : "|") + ("(?:" + p + ")");
                    auto f = field{.member = name, .type = make(tk::map, {std::move(t.t)}), .loc = g.loc};
                    f.extra = true;
                    f.pattern = std::move(pattern);
                    if (!t.s.empty()) f.set.additional.push_back(std::move(t.s));
                    out.push_back(std::move(f));
                }
                return out;
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
                // the properties whose schema is `false`: an object must not have the key, which yyjson::not_ says
                auto absent = std::vector<condition>();
                if (const auto* props = s.find("properties"); props != nullptr && props->is_object())
                {
                    for (const auto& [k, sub] : props->members)
                    {
                        const auto floc = loc + "/properties/" + pointer_token(k);
                        const auto* target = normalize_full(&sub, floc);
                        const auto never = target->is_bool() && !target->b;
                        const auto calls = warn_calls_;
                        auto ft = never ? typed{} : type_of(&sub, floc, decls_[idx].name + pascal(k));
                        // resolved again only when it resolved without a warning
                        const auto* ref = !never && warn_calls_ == calls && is_pure_ref(sub)
                                              ? resolve(*sub.find("$ref"), floc)
                                              : nullptr;
                        if (never)
                        {
                            if (is_required(k))
                                warn(floc, "the key \"" + k + "\" is required and its schema is `false`; no object "
                                                              "satisfies the schema");
                            absent.push_back(condition{
                                .negated = true, .if_ = {cond_field{.key = k, .required = true}}, .loc = floc});
                        }
                        const auto* own = is_pure_ref(sub) ? &sub : normalize(&sub, floc);
                        const auto* dflt = own->find("default");
                        if (dflt == nullptr && target->is_object()) dflt = target->find("default");
                        if (dflt == nullptr && is_required(k) && target->is_object()) dflt = target->find("const");
                        auto f = field{
                            .member = k, .key = k, .ref = ref, .prop = target, .meta = meta_of(*own), .loc = floc};
                        f.set = std::move(ft.s);
                        place_default(f, std::move(ft.t), dflt, is_required(k));
                        fields.push_back(std::move(f));
                    }
                }
                // a key that another keyword names is a field, of the type that the object gives a key that no
                // property declares; `rejected` takes the keys that `additionalProperties` false rejects instead
                const auto add_named_key = [&](const std::string& k, bool req, const std::string& kloc,
                                               std::vector<std::string>* rejected = nullptr) {
                    if (std::ranges::any_of(fields, [&](const auto& f) { return f.key == k; })) return;
                    const json* schema = nullptr;
                    auto t = undeclared_key_type(s, k, loc, decls_[idx].name + pascal(k), schema);
                    if (!t && rejected != nullptr)
                    {
                        if (std::ranges::find(*rejected, k) == rejected->end()) rejected->push_back(k);
                        return;
                    }
                    if (!t)
                    {
                        // as a property whose schema is `false`: the object must not have the key
                        if (req)
                        {
                            warn(kloc, "the key \"" + k +
                                           (schema == nullptr
                                                ? "\" is no property, and `additionalProperties` is false"
                                                : "\" matches a pattern of `patternProperties` whose schema is `false`") +
                                           "; no object satisfies the schema");
                        }
                        absent.push_back(
                            condition{.negated = true, .if_ = {cond_field{.key = k, .required = true}}, .loc = kloc});
                        t = typed{};
                    }
                    auto f = field{.member = k, .key = k, .required = req, .schema = schema, .loc = kloc};
                    f.type = req ? std::move(t->t) : make(tk::optional, {std::move(t->t)});
                    f.set = std::move(t->s);
                    fields.push_back(std::move(f));
                };
                for (const auto& k : required) add_named_key(k, true, loc + "/required");
                auto dependent = std::vector<std::pair<std::string, std::vector<std::string>>>();
                if (const auto* dep = s.find("dependentRequired"); dep != nullptr && dep->is_object())
                {
                    for (const auto& [k, v] : dep->members)
                    {
                        const auto dloc = loc + "/dependentRequired/" + pointer_token(k);
                        if (!v.is_array() || !std::ranges::all_of(v.items, [](const auto& e) { return e.is_string(); }))
                        {
                            warn(dloc, "`dependentRequired` whose value is not an array of strings is ignored");
                            continue;
                        }
                        if (v.items.empty()) continue;
                        auto keys = std::vector<std::string>();
                        add_named_key(k, false, dloc);
                        for (const auto& e : v.items)
                        {
                            add_named_key(e.text, false, dloc);
                            keys.push_back(e.text);
                        }
                        dependent.emplace_back(k, std::move(keys));
                    }
                }
                auto groups = std::vector<required_groups>();
                for (const auto* kw : {"oneOf", "anyOf"})
                    if (auto g = required_groups_of(s, kw))
                    {
                        for (const auto& group : g->groups)
                            for (const auto& k : group) add_named_key(k, false, loc + "/" + kw);
                        groups.push_back(std::move(*g));
                    }
                // the keys that the conditions name: a key that `additionalProperties` false rejects is never in an
                // object, and has no field
                auto rejected = std::vector<std::string>();
                for (const auto& [k, kloc] : condition_keys(s)) add_named_key(k, false, kloc, &rejected);
                // the keys that name no property: rejected, accepted and dropped, or kept in a field of
                // yyjson::extra_keys, whose map holds them by the schema of `additionalProperties`
                auto additional = true;
                auto extra = std::optional<typed>();
                const auto* ap = s.find("additionalProperties");
                const auto ap_any =
                    ap == nullptr || (ap->is_bool() && ap->b) || (ap->is_object() && ap->members.empty());
                auto key_pattern = std::optional<std::string>();
                const auto* pp = s.find("patternProperties");
                auto patterns = std::vector<field>();
                if (pp != nullptr && pp->is_object() && !pp->members.empty() && opts_.pattern)
                {
                    // a field of yyjson::pattern_properties for each schema of the patterns, which checks the keys
                    // of the fields and of the other fields of patterns that its pattern matches too
                    patterns = pattern_fields(s, fields, absent, is_required, idx, loc);
                    if (ap != nullptr && ap->is_bool())
                        additional = ap->b;
                    else if (!ap_any)
                        extra = type_of(ap, loc + "/additionalProperties", decls_[idx].name + "Extra");
                }
                else if (pp != nullptr && pp->is_object() && !pp->members.empty())
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
                auto min_properties = std::optional<std::uint64_t>();
                auto max_properties = std::optional<std::uint64_t>();
                if (const auto* v = s.find("minProperties"); v != nullptr && as_uint64(*v) && *as_uint64(*v) > 0)
                    min_properties = as_uint64(*v);
                if (const auto* v = s.find("maxProperties"); v != nullptr && as_uint64(*v))
                    max_properties = as_uint64(*v);
                // `propertyNames` checks the keys that the field of yyjson::extra_keys receives; an open struct gets
                // the field for it
                auto names = std::optional<settings>();
                const auto* pn = s.find("propertyNames");
                if (pn != nullptr && reach_of(pn, "string", loc + "/propertyNames") == reach::none)
                {
                    // every key fails: only the empty object satisfies the schema, and a required key is the
                    // contradiction with maxProperties that check_levels() reports
                    max_properties = 0;
                }
                else if (pn != nullptr)
                {
                    const auto pnloc = loc + "/propertyNames";
                    const auto [other, listed] = names_of_other_keys(*pn, fields);
                    names = key_settings(*other, pnloc);
                    // the keys of the properties, checked here: an object must not have a key that fails
                    auto unchecked = false;
                    for (const auto& f : fields)
                    {
                        if (f.extra || listed) continue;
                        const auto r = names ? key_fit(f.key, *names) : fit::unknown;
                        if (r == fit::unknown) unchecked = true;
                        if (r != fit::no) continue;
                        if (f.required)
                            warn(pnloc, "the key \"" + f.key +
                                            "\" is required and fails `propertyNames`; no object satisfies the schema");
                        absent.push_back(condition{
                            .negated = true, .if_ = {cond_field{.key = f.key, .required = true}}, .loc = pnloc});
                    }
                    // a closed struct without fields of patterns has no other keys
                    const auto others = extra || additional || !patterns.empty();
                    if (!names && (others || unchecked))
                        warn(pnloc, "`propertyNames` is not expressed by string constraints and is ignored");
                    else if (names)
                    {
                        if (others && !extra && additional) extra = typed{make(tk::any), {}};
                        if (unchecked && others)
                            warn(pnloc, "`propertyNames` is checked only for the keys that name no property");
                        else if (unchecked)
                            warn(pnloc, "`propertyNames` is not checked: the generator cannot tell whether the keys "
                                        "of the properties satisfy it");
                    }
                }
                if (!opts_.cpp26 && fields.size() > max_visitable_fields)
                    fail(loc, "a struct of more than " + std::to_string(max_visitable_fields) +
                                  " properties needs --cpp26: VISITABLE_STRUCT takes no more");
                if ((extra || !patterns.empty()) && !opts_.cpp26 &&
                    fields.size() + patterns.size() >= max_reflected_fields)
                {
                    // the field would make a struct that VISITABLE_STRUCT registers, which takes neither
                    // yyjson::extra_keys nor yyjson::pattern_properties
                    const auto* what = !extra            ? "yyjson::pattern_properties"
                                       : patterns.empty() ? "yyjson::extra_keys"
                                                          : "yyjson::extra_keys or yyjson::pattern_properties";
                    warn(loc, "a struct of more than " + std::to_string(max_reflected_fields) +
                                  " fields has no field of " + what +
                                  " without --cpp26; the keys that name no property are dropped" +
                                  (patterns.empty() ? ""
                                   : additional     ? ", and `patternProperties` is not checked"
                                                    : ", and neither `patternProperties` nor `additionalProperties` "
                                                      "is checked"));
                    // the keys that the patterns accept are accepted
                    if (!patterns.empty()) additional = true;
                    extra.reset();
                    patterns.clear();
                }
                for (auto& f : patterns)
                {
                    if (names && !names->empty()) f.set.property_names.push_back(*names);
                    fields.push_back(std::move(f));
                    uses_pattern_ = true;
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
                auto conditions = conditions_of(s, fields, rejected, decls_[idx].name);
                // a `then` that asks only for keys the `if` requires holds whenever the `if` does
                std::erase_if(conditions, [](const condition& c) {
                    const auto implied = [&](const cond_field& t) {
                        return t.required && t.set.empty() && !t.is_type && !t.counts &&
                               std::ranges::any_of(c.if_, [&](const cond_field& i) {
                                   return i.key == t.key && i.required && !i.counts;
                               });
                    };
                    return !c.negated && !c.if_.empty() && c.else_.empty() && !c.then_closed && !c.else_closed &&
                           std::ranges::all_of(c.then_, implied);
                });
                std::ranges::move(absent, std::back_inserter(conditions));
                if (s.has("$conditions")) conditions_used_.insert(&s);
                auto& d = decls_[idx];
                d.fields = std::move(fields);
                d.additional = additional;
                d.dependent = std::move(dependent);
                d.groups = std::move(groups);
                d.conditions = std::move(conditions);
                d.min_properties = min_properties;
                d.max_properties = max_properties;
                d.filled = true;
            }

            // conditions

            static const field* field_of(const std::vector<field>& fields, std::string_view k)
            {
                for (const auto& f : fields)
                    if (!f.extra && f.key == k) return &f;
                return nullptr;
            }

            static bool type_has(const json& type, std::string_view kind)
            {
                if (type.is_string()) return type.text == kind;
                return type.is_array() &&
                       std::ranges::any_of(type.items, [&](const json& t) { return t.is_string() && t.text == kind; });
            }

            // whether a value of the type may be JSON null, which the settings of a field let through
            bool may_be_null(const ctype& t) const
            {
                const auto& r = resolved(t);
                if (r.k == tk::optional || r.k == tk::indirect) return may_be_null(r.args[0]);
                return holds_null(r) || r.k == tk::any || r.k == tk::null || r.k == tk::monostate;
            }

            // whether the normalized schema accepts null by `const`, `enum` and `type`
            static bool accepts_null(const json& n)
            {
                if (!n.is_object()) return !n.is_bool() || n.b;
                if (const auto* c = n.find("const"); c != nullptr && !c->is_null()) return false;
                if (const auto* e = n.find("enum");
                    e != nullptr && !(e->is_array() && std::ranges::any_of(e->items, [](const json& v) {
                        return v.is_null();
                    })))
                    return false;
                if (const auto* t = n.find("type"); t != nullptr && !type_has(*t, "null")) return false;
                return true;
            }

            // what the subschema of `if` or `not` tests, as the fields of yyjson::if_ / yyjson::not_: `fields` empty
            // when every object satisfies it, `never` when none does, `ok` false with the reason in `why` when C++
            // does not express it. `absent` lists the keys whose schemas no value of their fields satisfies, which
            // the subschema allows only to be missing; `counts_never` when the counts of keys in `fields` contradict,
            // which yyjson::if_ expresses as they are
            struct cond_test
            {
                bool ok = true;
                bool never = false;
                bool counts_never = false;
                std::vector<cond_field> fields{};
                std::vector<std::string> absent{};
                std::string why{};
            };

            // the types of yyjson::is_type<T> in `contains` are named after `hint` and the key
            cond_test test_of(const json& sub, const std::string& sloc, const std::vector<field>& fields,
                              const std::vector<std::string>& rejected, const std::string& hint)
            {
                auto r = cond_test();
                const auto fail = [&](std::string why) {
                    r.ok = false;
                    r.why = std::move(why);
                    return r;
                };
                const auto entry = [&](const std::string& k) -> cond_field& {
                    for (auto& f : r.fields)
                        if (f.key == k && !f.counts) return f;
                    return r.fields.emplace_back(cond_field{.key = k});
                };
                const auto is_rejected = [&](const std::string& k) {
                    return std::ranges::find(rejected, k) != rejected.end();
                };
                const auto* n = normalize_full(&sub, sloc);
                if (n->is_bool())
                {
                    r.never = !n->b;
                    return r;
                }
                if (!n->is_object()) return fail("it is not a schema");
                for (const auto& [k, v] : n->members)
                {
                    if (is_metadata_key(k)) continue;
                    if (k == "type")
                    {
                        // the value is an object
                        if (!type_has(v, "object")) r.never = true;
                    }
                    else if (k == "required" && v.is_array())
                    {
                        for (const auto& e : v.items)
                        {
                            if (!e.is_string()) return fail("`required` is not an array of strings");
                            if (is_rejected(e.text))
                                r.never = true;
                            else
                                entry(e.text).required = true;
                        }
                    }
                    else if (k == "properties" && v.is_object())
                    {
                        for (const auto& [key, ps] : v.members)
                        {
                            // a key that is never in an object satisfies its schema
                            if (is_rejected(key)) continue;
                            const auto ploc = sloc + "/properties/" + pointer_token(key);
                            const auto* pn = normalize_full(&ps, ploc);
                            if (accepts_anything(*pn)) continue;
                            const auto* f = field_of(fields, key);
                            if (f == nullptr) return fail("the key \"" + key + "\" has no field");
                            if (pn->is_bool()) return fail("the schema of the key \"" + key + "\" is false");
                            if (value_never(*pn, f->type))
                            {
                                r.absent.push_back(key);
                                continue;
                            }
                            // the JSON value of a field of yyjson::value or yyjson::object can be tested by
                            // yyjson::is_type<T> of the type of the schema
                            const auto vt = value_type(f->type);
                            const auto dom = vt.k == tk::any || vt.k == tk::object;
                            auto by_settings = true;
                            if (pn->is_object())
                                for (const auto& [pk, pv] : pn->members)
                                    if (!is_metadata_key(pk) && pk != "type" && pk != "const" && pk != "enum" &&
                                        pk != "minimum" && pk != "maximum" && pk != "exclusiveMinimum" &&
                                        pk != "exclusiveMaximum" && pk != "multipleOf" && pk != "minLength" &&
                                        pk != "maxLength" && pk != "pattern" && pk != "format" && pk != "minItems" &&
                                        pk != "maxItems" && pk != "contains" && pk != "minContains" &&
                                        pk != "maxContains")
                                    {
                                        if (!dom)
                                            return fail("`" + pk + "` of the key \"" + key +
                                                        "\" is not tested by a field");
                                        by_settings = false;
                                    }
                            // the test must be exact: a looser `contains` would change which branch is taken
                            const auto calls = warn_calls_;
                            auto set = by_settings ? settings_for(ps, f->type, ploc, hint + pascal(key))
                                                   : std::optional<settings>();
                            if (set && warn_calls_ != calls) set.reset();
                            else if (set)
                            {
                                // the settings of a field let null through unless they list values, so a schema
                                // that rejects null only by `type` is not tested by them
                                if (may_be_null(f->type) && !accepts_null(*pn) && !pn->has("const") &&
                                    !pn->has("enum"))
                                    return fail("the value of the key \"" + key +
                                                "\" may be null, which the test of its field lets through");
                                if (!set->empty()) entry(key).set = std::move(*set);
                                continue;
                            }
                            if (dom)
                            {
                                // the type of the schema rejects null unless the schema takes it
                                const auto before = warn_calls_;
                                auto t = type_of(&ps, ploc, hint + pascal(key));
                                if (warn_calls_ == before && warn_calls_ == calls && (t.t.k != tk::any || !t.s.empty()))
                                {
                                    entry(key).is_type = constrained(std::move(t));
                                    continue;
                                }
                            }
                            return fail("the schema of the key \"" + key +
                                        "\" is not expressed by the settings of its field");
                        }
                    }
                    else if (k == "minProperties" || k == "maxProperties")
                    {
                        if (!add_key_count(r.fields, k, v)) return fail("`" + k + "` is not a non-negative integer");
                    }
                    else if (k == "$conditions")
                        return fail("`if`, `not` and `dependentSchemas` are not supported in it");
                    else if (is_keyword_key(k))
                        return fail("`" + k + "` is not supported in it");
                }
                if (counts_contradict(r.fields)) r.never = r.counts_never = true;
                // a key that the object has, by the schema of the struct or by `required`, fails the test of absence
                for (const auto& k : r.absent)
                    if (field_of(fields, k)->required ||
                        std::ranges::any_of(r.fields, [&](const cond_field& f) { return f.key == k && f.required; }))
                        r.never = true;
                std::erase_if(r.fields, [](const cond_field& f) {
                    return !f.counts && !f.required && f.set.empty() && !f.is_type;
                });
                return r;
            }

            // what the subschema of `then` or `else` requires, as the fields of yyjson::then / yyjson::else_; the
            // parts that C++ does not express are left out with a warning. The type of yyjson::is_type<T> is named
            // after `name`, the key and `suffix`. `never` when no object satisfies the subschema: it is false, or
            // requires a key that `additionalProperties` false rejects. `closed` when it has `additionalProperties`
            // false, which yyjson::additional_properties(false) expresses: the part then names every field that
            // `properties` names, with or without settings
            // `nested`, when given, takes the "$conditions" of the subschema with its location instead of a warning
            std::vector<cond_field> assert_of(const json& sub, const std::string& sloc, const std::vector<field>& fields,
                                              const std::vector<std::string>& rejected, const std::string& name,
                                              const std::string& suffix, bool& never, bool& closed,
                                              std::vector<std::pair<const json*, std::string>>& nested)
            {
                auto out = std::vector<cond_field>();
                never = false;
                closed = false;
                const auto entry = [&](const std::string& k) -> cond_field& {
                    for (auto& f : out)
                        if (f.key == k && !f.counts) return f;
                    return out.emplace_back(cond_field{.key = k});
                };
                const auto* n = normalize_full(&sub, sloc);
                if (n->is_bool()) never = !n->b;
                if (!n->is_object()) return out;
                if (const auto* r = n->find("required"); r != nullptr && r->is_array())
                    for (const auto& e : r->items)
                        if (e.is_string() && std::ranges::find(rejected, e.text) != rejected.end())
                        {
                            never = true;
                            return out;
                        }
                if (const auto* a = n->find("additionalProperties"); a != nullptr && a->is_bool() && !a->b)
                {
                    // the keys of `patternProperties` are not named by fields, so the part would reject them
                    if (n->has("patternProperties"))
                        warn(sloc + "/additionalProperties",
                             "`additionalProperties` false beside `patternProperties` is not supported here and is "
                             "ignored");
                    else
                    {
                        closed = true;
                        // a required key that `properties` does not list, or lists as `false`, is rejected
                        const auto* props = n->find("properties");
                        if (const auto* r = n->find("required"); r != nullptr && r->is_array())
                            for (const auto& e : r->items)
                            {
                                if (!e.is_string()) continue;
                                const auto* ps = props != nullptr && props->is_object() ? props->find(e.text) : nullptr;
                                if (ps == nullptr || [&] {
                                        const auto* pn = normalize_full(ps, sloc + "/properties/" +
                                                                                pointer_token(e.text));
                                        return pn->is_bool() && !pn->b;
                                    }())
                                {
                                    never = true;
                                    closed = false;
                                    return out;
                                }
                            }
                    }
                }
                if (const auto* t = n->find("type"); t != nullptr && !type_has(*t, "object"))
                {
                    // a part whose `type` excludes objects rejects every object
                    never = true;
                    return out;
                }
                for (const auto& [k, v] : n->members)
                {
                    if (is_metadata_key(k) || k == "type" || is_non_object_key(k)) continue;
                    if (k == "additionalProperties")
                    {
                        // `true` or a schema that accepts anything adds nothing
                        if (!v.is_bool() && !accepts_anything(*normalize_full(&v, sloc + "/additionalProperties")))
                            warn(sloc + "/additionalProperties",
                                 "`additionalProperties` other than false is not supported here and is ignored");
                        continue;
                    }
                    if (k == "required" && v.is_array())
                    {
                        for (const auto& e : v.items)
                            if (e.is_string()) entry(e.text).required = true;
                    }
                    else if (k == "properties" && v.is_object())
                    {
                        for (const auto& [key, ps] : v.members)
                        {
                            if (std::ranges::find(rejected, key) != rejected.end()) continue;
                            const auto ploc = sloc + "/properties/" + pointer_token(key);
                            const auto* pn = normalize_full(&ps, ploc);
                            const auto* f = field_of(fields, key);
                            if (f == nullptr) continue;
                            if (pn->is_bool() && !pn->b)
                            {
                                // a part with yyjson::additional_properties(false) rejects a key that it does not
                                // name
                                if (!closed)
                                    warn(ploc, "the schema `false` of a key that has a field is not supported here "
                                               "and is ignored");
                                continue;
                            }
                            if (closed) (void)entry(key);
                            if (accepts_anything(*pn)) continue;
                            // the schema of the property, which the field already follows, adds nothing
                            if (f->ref != nullptr && is_pure_ref(ps) && resolve(*ps.find("$ref"), ploc) == f->ref)
                                continue;
                            if (f->prop != nullptr && same_assertions(*pn, *f->prop)) continue;
                            if (auto set = settings_for(ps, f->type, ploc))
                            {
                                if (!set->empty()) entry(key).set = std::move(*set);
                                continue;
                            }
                            const auto vt = value_type(f->type);
                            if (vt.k == tk::any || vt.k == tk::object)
                            {
                                // the JSON value of the field is read as the type of the schema
                                auto t = type_of(&ps, ploc, name + pascal(key) + suffix);
                                if (t.t.k == tk::any && t.s.empty()) continue;
                                entry(key).is_type = constrained(std::move(t));
                                continue;
                            }
                            // a reference to the type of the field adds nothing
                            if (is_pure_ref(ps))
                                if (const auto* target = resolve(*ps.find("$ref"), ploc); target != nullptr)
                                    if (const auto t = type_of(target, ploc, name + pascal(key) + suffix);
                                        t.s.empty() && spell(t.t) == spell(vt))
                                        continue;
                            warn(ploc, "the schema of the key \"" + key +
                                           "\" is not expressed by the settings of its field and is ignored");
                        }
                    }
                    else if (k == "minProperties" || k == "maxProperties")
                    {
                        if (!add_key_count(out, k, v))
                            warn(sloc + "/" + k, "`" + k + "` is not a non-negative integer and is ignored");
                    }
                    else if (k == "$conditions")
                        nested.emplace_back(&v, sloc);
                    else if (is_keyword_key(k))
                        warn(sloc + "/" + k, "`" + k + "` is not supported here and is ignored");
                }
                if (counts_contradict(out))
                {
                    // no object has fewer keys than `minProperties` and more than `maxProperties` at once; the counts
                    // are kept, as yyjson::then and yyjson::else_ express them as they are
                    never = true;
                    closed = false;
                    std::erase_if(out, [](const cond_field& f) { return !f.counts; });
                    nested.clear();
                }
                return out;
            }

            // `minProperties` or `maxProperties` of a part, joined into its entry of the counts of keys; false for a
            // value that is not a count. `minProperties` of 0 adds nothing
            static bool add_key_count(std::vector<cond_field>& part, const std::string& keyword, const json& v)
            {
                const auto n = as_uint64(v);
                if (!n) return false;
                join_key_count(part, keyword == "minProperties", *n);
                return true;
            }

            static void join_key_count(std::vector<cond_field>& part, bool is_min, std::uint64_t n)
            {
                if (is_min && n == 0) return;
                auto it = std::ranges::find_if(part, &cond_field::counts);
                if (it == part.end()) it = part.insert(part.end(), cond_field{.counts = true});
                auto& bound = is_min ? it->set.min_items : it->set.max_items;
                bound = !bound ? n : is_min ? std::max(*bound, n) : std::min(*bound, n);
            }

            // whether the counts of keys of a part leave no number of keys
            static bool counts_contradict(const std::vector<cond_field>& part)
            {
                const auto it = std::ranges::find_if(part, &cond_field::counts);
                return it != part.end() && it->set.min_items && it->set.max_items &&
                       *it->set.min_items > *it->set.max_items;
            }

            // The guard of a condition nested in `then`, `else` or `dependentSchemas`: the nested condition applies to
            // an object that passes every test of `when` and, with `unless`, fails some test of `unless` (the `if`
            // that an `else` follows)
            struct guard
            {
                std::vector<cond_field> when;
                std::optional<std::vector<cond_field>> unless;
            };

            // the settings of one value that hold when both `a` and `b` hold; nullopt when a keyword has different
            // values in them
            static std::optional<settings> join_settings(settings a, const settings& b)
            {
                for (const auto& c : b.constraints)
                {
                    const auto it = std::ranges::find(a.constraints, c.keyword, &settings::constraint::keyword);
                    if (it == a.constraints.end())
                        a.constraints.push_back(c);
                    else if (it->text != c.text)
                        return std::nullopt;
                }
                const auto join_count = [](std::optional<std::uint64_t>& x, const std::optional<std::uint64_t>& y) {
                    if (x && y && *x != *y) return false;
                    if (y) x = y;
                    return true;
                };
                const auto join_level = [](std::vector<settings>& x, const std::vector<settings>& y) {
                    if (!x.empty() && !y.empty()) return false;
                    if (!y.empty()) x = y;
                    return true;
                };
                if (!join_count(a.min_items, b.min_items) || !join_count(a.max_items, b.max_items) ||
                    !join_count(a.min_contains, b.min_contains) || !join_count(a.max_contains, b.max_contains) ||
                    !join_level(a.items, b.items) || !join_level(a.contains, b.contains) ||
                    !join_level(a.property_names, b.property_names) || !join_level(a.additional, b.additional) ||
                    !join_level(a.prefix, b.prefix))
                    return std::nullopt;
                if (a.type_test >= 0 && b.type_test >= 0 && a.type_test != b.type_test) return std::nullopt;
                if (b.type_test >= 0) a.type_test = b.type_test;
                a.unique = a.unique || b.unique;
                a.negated.insert(a.negated.end(), b.negated.begin(), b.negated.end());
                a.conditional.insert(a.conditional.end(), b.conditional.begin(), b.conditional.end());
                return a;
            }

            // the tests that hold when both `a` and `b` hold, one field for each key
            std::optional<std::vector<cond_field>> join_tests(std::vector<cond_field> a,
                                                              const std::vector<cond_field>& b) const
            {
                for (const auto& f : b)
                {
                    if (f.counts)
                    {
                        // both counts hold: the larger minimum and the smaller maximum
                        if (f.set.min_items) join_key_count(a, true, *f.set.min_items);
                        if (f.set.max_items) join_key_count(a, false, *f.set.max_items);
                        continue;
                    }
                    const auto it = std::ranges::find_if(a, [&](const cond_field& x) {
                        return x.key == f.key && !x.counts;
                    });
                    if (it == a.end())
                    {
                        a.push_back(f);
                        continue;
                    }
                    it->required = it->required || f.required;
                    if (f.is_type)
                    {
                        if (it->is_type && spell(*it->is_type) != spell(*f.is_type)) return std::nullopt;
                        it->is_type = f.is_type;
                    }
                    auto s = join_settings(std::move(it->set), f.set);
                    if (!s) return std::nullopt;
                    it->set = std::move(*s);
                }
                return a;
            }

            // a subschema that no object satisfies under the guard `g`: the guard itself fails
            void add_guarded_false(const guard& g, const std::string& sloc, std::vector<condition>& out)
            {
                if (!g.unless && !g.when.empty())
                    out.push_back(condition{.negated = true, .if_ = g.when, .loc = sloc});
                else if (g.unless && !g.when.empty())
                    out.push_back(condition{.if_ = g.when, .then_ = *g.unless, .loc = sloc});
                else
                    warn(sloc, "a subschema that no object satisfies is not supported here and is ignored");
            }

            // the conditions of a subschema of `then`, `else` or `dependentSchemas` nested in another one, which
            // apply under the guard `g`. The parts other than `not` and nested `if` and `dependentSchemas` are the
            // assertions of yyjson::then or yyjson::else_ of the guard
            void add_guarded(const json& sub, const std::string& sloc, const guard& g, const std::vector<field>& fields,
                             const std::vector<std::string>& rejected, const std::string& name,
                             const std::string& suffix, std::vector<condition>& out, int depth)
            {
                if (accepts_anything(*normalize_full(&sub, sloc))) return;
                if (depth > 8)
                {
                    warn(sloc, "conditions nested more than 8 levels deep are not supported and are ignored");
                    return;
                }
                auto never = false, closed = false;
                auto nested = std::vector<std::pair<const json*, std::string>>();
                auto a = assert_of(sub, sloc, fields, rejected, name, suffix, never, closed, nested);
                if (never && g.unless && g.when.empty() && !a.empty())
                    out.push_back(condition{.if_ = *g.unless, .else_ = std::move(a), .loc = sloc});
                else if (never)
                {
                    add_guarded_false(g, sloc, out);
                    return;
                }
                if (!a.empty() || closed)
                {
                    if (!g.unless && !g.when.empty())
                        out.push_back(
                            condition{.if_ = g.when, .then_ = std::move(a), .then_closed = closed, .loc = sloc});
                    else if (g.unless && g.when.empty())
                        out.push_back(
                            condition{.if_ = *g.unless, .else_ = std::move(a), .else_closed = closed, .loc = sloc});
                    else if (g.unless)
                        warn(sloc, "a subschema that applies when one `if` holds and another fails is not supported "
                                   "and is ignored");
                    else
                        warn(sloc, "a subschema that applies to every object is not supported here and is ignored");
                }
                add_nested(nested, g, fields, rejected, name, out, depth + 1);
            }

            // the entries of "$conditions" of subschemas of `then`, `else` or `dependentSchemas`, under the guard `g`
            void add_nested(const std::vector<std::pair<const json*, std::string>>& nested, const guard& g,
                            const std::vector<field>& fields, const std::vector<std::string>& rejected,
                            const std::string& name, std::vector<condition>& out, int depth)
            {
                for (const auto& [list, lloc] : nested)
                {
                    if (!list->is_array()) continue;
                    for (const auto& c : list->items)
                    {
                        const auto* l = c.find("$loc");
                        const auto cloc = l != nullptr && l->is_string() ? l->text : lloc;
                        if (c.has("if"))
                            add_nested_if(c, cloc, g, fields, rejected, name, out, depth);
                        else if (const auto* nt = c.find("not"))
                        {
                            // a `not` of values of other kinds holds for every object
                            if (reach_of(nt, "object", cloc + "/not") != reach::none)
                                add_not(*nt, cloc + "/not", fields, rejected, name, out, g);
                        }
                        else if (const auto* d = c.find("dependentSchemas"); d != nullptr && d->is_object())
                            for (const auto& [k, v] : d->members)
                            {
                                const auto dloc = cloc + "/dependentSchemas/" + pointer_token(k);
                                if (std::ranges::find(rejected, k) != rejected.end()) continue;
                                auto when = join_tests(g.when, {cond_field{.key = k, .required = true}});
                                add_guarded(v, dloc, guard{std::move(*when), g.unless}, fields, rejected, name,
                                            "Dependent", out, depth);
                            }
                    }
                }
            }

            // `if` nested in `then`, `else` or `dependentSchemas`: its `then` applies under the guard joined with the
            // tests of `if`; its `else` applies when the tests fail, which only a guard without `unless` expresses
            void add_nested_if(const json& c, const std::string& cloc, const guard& g, const std::vector<field>& fields,
                               const std::vector<std::string>& rejected, const std::string& name,
                               std::vector<condition>& out, int depth)
            {
                const auto* tv = c.find("then");
                const auto* ev = c.find("else");
                auto tloc = cloc + "/then", eloc = cloc + "/else";
                const auto trivial = [&](const json* x, const std::string& xloc) {
                    return x == nullptr || accepts_anything(*normalize_full(x, xloc));
                };
                if (trivial(tv, tloc) && trivial(ev, eloc)) return;
                auto t = test_of(*c.find("if"), cloc + "/if", fields, rejected, name + "If");
                if (!t.ok)
                {
                    warn(cloc + "/if", "`if` is not supported here: " + t.why + "; `then` and `else` are ignored");
                    return;
                }
                if (!t.never && !t.absent.empty())
                {
                    // as in add_if: the test of the presence of the key, with `then` and `else` swapped
                    if (t.absent.size() > 1 || !t.fields.empty())
                    {
                        warn(cloc + "/if", "`if` is not supported here: it holds only when the key \"" + t.absent[0] +
                                               "\" is missing, together with other tests; `then` and `else` are "
                                               "ignored");
                        return;
                    }
                    t.fields = {cond_field{.key = t.absent[0], .required = true}};
                    std::swap(tv, ev);
                    std::swap(tloc, eloc);
                }
                const auto suffix = [](const std::string& xloc) { return xloc.ends_with("/then") ? "Then" : "Else"; };
                if (t.never || t.fields.empty())
                {
                    // the subschema that applies, applies under the guard alone
                    const auto* x = t.never ? ev : tv;
                    const auto& xloc = t.never ? eloc : tloc;
                    if (!trivial(x, xloc)) add_guarded(*x, xloc, g, fields, rejected, name, suffix(xloc), out, depth);
                    return;
                }
                if (!trivial(tv, tloc))
                {
                    if (auto when = join_tests(g.when, t.fields))
                        add_guarded(*tv, tloc, guard{std::move(*when), g.unless}, fields, rejected, name, suffix(tloc),
                                    out, depth);
                    else
                        warn(cloc + "/if", "`if` is not supported here: it tests a key that the `if` around it tests "
                                           "by other values; `then` is ignored");
                }
                if (!trivial(ev, eloc))
                {
                    if (g.unless)
                        warn(eloc, std::string("`") + (eloc.ends_with("/else") ? "else" : "then") +
                                       "` of an `if` inside `else` is not supported and is ignored");
                    else
                        add_guarded(*ev, eloc, guard{g.when, t.fields}, fields, rejected, name, suffix(eloc), out,
                                    depth);
                }
            }

            // A link of a chain of `if` and `else`: `if` tests one key by `const` or `enum`
            struct chain_link
            {
                const json* entry = nullptr;
                std::string loc, key;
                std::vector<json> values;
                bool required = false;
            };

            std::optional<chain_link> chain_link_of(const json& c, const std::string& cloc)
            {
                const auto* n = normalize_full(c.find("if"), cloc + "/if");
                if (!n->is_object()) return std::nullopt;
                chain_link link;
                link.entry = &c;
                link.loc = cloc;
                const json* props = nullptr;
                const json* req = nullptr;
                for (const auto& [k, v] : n->members)
                {
                    if (is_metadata_key(k) || (k == "type" && type_has(v, "object"))) continue;
                    if (k == "properties")
                        props = &v;
                    else if (k == "required")
                        req = &v;
                    else
                        return std::nullopt;
                }
                if (props == nullptr || !props->is_object() || props->members.size() != 1) return std::nullopt;
                link.key = props->members[0].first;
                if (req != nullptr)
                {
                    if (!req->is_array() || req->items.size() != 1 || !req->items[0].is_string() ||
                        req->items[0].text != link.key)
                        return std::nullopt;
                    link.required = true;
                }
                const auto* p = normalize_full(&props->members[0].second, cloc + "/if/properties");
                if (!p->is_object()) return std::nullopt;
                for (const auto& [k, v] : p->members)
                {
                    if (is_metadata_key(k)) continue;
                    if (k == "const")
                        link.values.push_back(v);
                    else if (k == "enum" && v.is_array() && link.values.empty())
                        link.values = v.items;
                    else
                        return std::nullopt;
                }
                if (link.values.empty()) return std::nullopt;
                return link;
            }

            // An `if` whose `else` is another `if`, and so on, each testing the same key by values that no other
            // tests: the conditions of the links do not depend on each other, and the last `else` applies when
            // the key has none of the values. Each link is then one yyjson::if_then_else, and the last `else` one
            // more whose condition is the key having any of the values
            bool add_chain(const json& c, const std::string& cloc, const std::vector<field>& fields,
                           const std::vector<std::string>& rejected, const std::string& name,
                           std::vector<condition>& out)
            {
                auto links = std::vector<chain_link>();
                const auto* entry = &c;
                auto eloc = cloc;
                const json* last = nullptr;
                auto last_loc = std::string();
                for (auto depth = 0; depth < 64; ++depth)
                {
                    auto link = chain_link_of(*entry, eloc);
                    if (!link) return false;
                    links.push_back(std::move(*link));
                    const auto* e = entry->find("else");
                    if (e == nullptr) break;
                    const auto* n = normalize_full(e, eloc + "/else");
                    const auto* list = n->is_object() ? n->find("$conditions") : nullptr;
                    if (list == nullptr || !list->is_array() || list->items.size() != 1 ||
                        !list->items[0].has("if") ||
                        std::ranges::any_of(n->members, [](const auto& m) {
                            return m.first != "$conditions" && !is_metadata_key(m.first);
                        }))
                    {
                        last = e;
                        last_loc = eloc + "/else";
                        break;
                    }
                    entry = &list->items[0];
                    const auto* l = entry->find("$loc");
                    eloc = l != nullptr && l->is_string() ? l->text : eloc + "/else";
                }
                if (links.size() < 2) return false;
                auto values = std::vector<json>();
                auto optional_links = 0;
                for (const auto& l : links)
                {
                    if (l.key != links[0].key) return false;
                    optional_links += l.required ? 0 : 1;
                    for (const auto& v : l.values)
                    {
                        if (std::ranges::any_of(values, [&](const json& x) { return equal(x, v); })) return false;
                        values.push_back(v);
                    }
                }
                // a missing key satisfies every `if` that does not require it, of which the chain takes the first
                if (optional_links > 1) return false;
                auto parts = std::vector<condition>();
                for (const auto& l : links)
                {
                    auto t = test_of(*l.entry->find("if"), l.loc + "/if", fields, rejected, name + "If");
                    if (!t.ok || t.never || t.fields.empty() || !t.absent.empty()) return false;
                    const auto* tv = l.entry->find("then");
                    if (tv == nullptr) continue;
                    auto never = false, closed = false;
                    auto nested = std::vector<std::pair<const json*, std::string>>();
                    auto th = assert_of(*tv, l.loc + "/then", fields, rejected, name, "Then", never, closed, nested);
                    if (never)
                        parts.push_back(condition{.negated = true, .if_ = t.fields, .loc = l.loc});
                    else if (!th.empty() || closed)
                        parts.push_back(
                            condition{.if_ = t.fields, .then_ = std::move(th), .then_closed = closed, .loc = l.loc});
                    add_nested(nested, guard{std::move(t.fields), std::nullopt}, fields, rejected, name, parts, 1);
                }
                if (last != nullptr)
                {
                    auto never = false, closed = false;
                    auto nested = std::vector<std::pair<const json*, std::string>>();
                    auto el = assert_of(*last, last_loc, fields, rejected, name, "Else", never, closed, nested);
                    if (never && el.empty())
                        warn(last_loc, "`else` that no object satisfies is not supported and is ignored");
                    else if (!el.empty() || closed || !nested.empty())
                    {
                        auto any = json::object_of();
                        auto props = json::object_of();
                        auto values_schema = json::object_of();
                        values_schema.set("enum", json::array_of(values));
                        props.set(links[0].key, std::move(values_schema));
                        any.set("properties", std::move(props));
                        if (optional_links == 0)
                            any.set("required", json::array_of({json::string_of(links[0].key)}));
                        auto t = test_of(*store(std::move(any)), cloc + "/if", fields, rejected, name + "If");
                        if (!t.ok || t.never || t.fields.empty() || !t.absent.empty()) return false;
                        if (!el.empty() || closed)
                            parts.push_back(condition{
                                .if_ = t.fields, .else_ = std::move(el), .else_closed = closed, .loc = last_loc});
                        add_nested(nested, guard{{}, std::move(t.fields)}, fields, rejected, name, parts, 1);
                    }
                }
                for (auto& p : parts) out.push_back(std::move(p));
                return true;
            }

            void add_if(const json& c, const std::string& cloc, const std::vector<field>& fields,
                        const std::vector<std::string>& rejected, const std::string& name, std::vector<condition>& out)
            {
                const auto* tv = c.find("then");
                const auto* ev = c.find("else");
                auto tloc = cloc + "/then", eloc = cloc + "/else";
                const auto trivial = [&](const json* x, const std::string& xloc) {
                    return x == nullptr || accepts_anything(*normalize_full(x, xloc));
                };
                if (trivial(tv, tloc) && trivial(ev, eloc)) return;
                if (add_chain(c, cloc, fields, rejected, name, out)) return;
                auto t = test_of(*c.find("if"), cloc + "/if", fields, rejected, name + "If");
                if (!t.ok)
                {
                    warn(cloc + "/if", "`if` is not supported here: " + t.why + "; `then` and `else` are ignored");
                    return;
                }
                if (!t.never && !t.absent.empty())
                {
                    // `if` holds exactly when the key is missing: the test of its presence, with `then` and `else`
                    // swapped
                    if (t.absent.size() > 1 || !t.fields.empty())
                    {
                        warn(cloc + "/if", "`if` is not supported here: it holds only when the key \"" + t.absent[0] +
                                               "\" is missing, together with other tests; `then` and `else` are "
                                               "ignored");
                        return;
                    }
                    t.fields = {cond_field{.key = t.absent[0], .required = true}};
                    std::swap(tv, ev);
                    std::swap(tloc, eloc);
                }
                if (t.counts_never && ev != nullptr && !trivial(ev, eloc))
                {
                    // the counts of yyjson::if_ never hold, so `else` applies to every object
                    t.never = false;
                    tv = nullptr;
                }
                if (t.never || t.fields.empty())
                {
                    if (trivial(t.never ? ev : tv, t.never ? eloc : tloc)) return;
                    warn(cloc + "/if", std::string("`if` that ") + (t.never ? "no object" : "every object") +
                                           " satisfies is not supported; `then` and `else` are ignored");
                    return;
                }
                auto cond = condition{.if_ = t.fields, .loc = cloc};
                auto nested_then = std::vector<std::pair<const json*, std::string>>();
                auto nested_else = std::vector<std::pair<const json*, std::string>>();
                if (tv != nullptr)
                {
                    auto never = false;
                    cond.then_ = assert_of(*tv, tloc, fields, rejected, name, tloc.ends_with("/then") ? "Then" : "Else",
                                           never, cond.then_closed, nested_then);
                    if (never)
                    {
                        out.push_back(condition{.negated = true, .if_ = t.fields, .loc = tloc});
                        cond.then_.clear();
                    }
                }
                if (ev != nullptr)
                {
                    auto never = false;
                    cond.else_ = assert_of(*ev, eloc, fields, rejected, name, eloc.ends_with("/else") ? "Else" : "Then",
                                           never, cond.else_closed, nested_else);
                    if (never && cond.else_.empty())
                        warn(eloc, std::string("`") + (eloc.ends_with("/else") ? "else" : "then") +
                                       "` that no object satisfies is not supported and is ignored");
                }
                if (!cond.then_.empty() || !cond.else_.empty() || cond.then_closed || cond.else_closed)
                    out.push_back(std::move(cond));
                add_nested(nested_then, guard{t.fields, std::nullopt}, fields, rejected, name, out, 1);
                add_nested(nested_else, guard{{}, t.fields}, fields, rejected, name, out, 1);
            }

            // `not` of the struct, or nested under the guard `g`: an object that passes the tests of the guard and of
            // the subschema is rejected
            void add_not(const json& v, const std::string& nloc, const std::vector<field>& fields,
                         const std::vector<std::string>& rejected, const std::string& name,
                         std::vector<condition>& out, const guard& g = {})
            {
                // `not` of `anyOf` rejects an object that any subschema accepts: one yyjson::not_ for each
                auto subs = std::vector<std::pair<const json*, std::string>>{{&v, nloc}};
                const auto* n = normalize_full(&v, nloc);
                if (const auto* any = n->is_object() ? n->find("anyOf") : nullptr;
                    any != nullptr && any->is_array() && std::ranges::all_of(n->members, [](const auto& m) {
                        return m.first == "anyOf" || is_metadata_key(m.first);
                    }))
                {
                    subs.clear();
                    for (std::size_t i = 0; i < any->items.size(); ++i)
                        subs.emplace_back(&any->items[i], nloc + "/anyOf/" + std::to_string(i));
                }
                for (const auto& [sub, sloc] : subs)
                {
                    auto t = test_of(*sub, sloc, fields, rejected, name + "Not");
                    if (!t.ok)
                        warn(sloc, "`not` is not supported here: " + t.why + "; it is ignored");
                    else if (t.never)
                        continue;
                    else if (!t.absent.empty())
                        warn(sloc, "`not` is not supported here: its subschema holds only when the key \"" +
                                       t.absent[0] + "\" is missing; it is ignored");
                    else if (t.fields.empty() && g.when.empty() && !g.unless)
                        warn(sloc, "`not` of a schema that every object satisfies rejects every object; it is ignored");
                    else if (t.fields.empty())
                        add_guarded_false(g, sloc, out);
                    else if (auto when = join_tests(g.when, t.fields); !when)
                        warn(sloc, "`not` is not supported here: it tests a key that the `if` around it tests by other "
                                   "values; it is ignored");
                    else if (g.unless)
                        out.push_back(condition{.if_ = std::move(*when), .then_ = *g.unless, .loc = sloc});
                    else
                        out.push_back(condition{.negated = true, .if_ = std::move(*when), .loc = sloc});
                }
            }

            // yyjson::if_then_else and yyjson::not_ of the struct `name` from "$conditions" of its schema `s`
            std::vector<condition> conditions_of(const json& s, const std::vector<field>& fields,
                                                 const std::vector<std::string>& rejected, const std::string& name)
            {
                auto out = std::vector<condition>();
                const auto* list = s.find("$conditions");
                if (list == nullptr || !list->is_array()) return out;
                for (const auto& c : list->items)
                {
                    const auto* l = c.find("$loc");
                    const auto cloc = l != nullptr && l->is_string() ? l->text : std::string();
                    if (c.has("if"))
                        add_if(c, cloc, fields, rejected, name, out);
                    else if (const auto* nt = c.find("not"))
                    {
                        // a `not` of values of other kinds holds for every object
                        if (reach_of(nt, "object", cloc + "/not") != reach::none)
                            add_not(*nt, cloc + "/not", fields, rejected, name, out);
                    }
                    else if (const auto* d = c.find("dependentSchemas"); d != nullptr && d->is_object())
                        for (const auto& [k, v] : d->members)
                        {
                            const auto dloc = cloc + "/dependentSchemas/" + pointer_token(k);
                            if (std::ranges::find(rejected, k) != rejected.end()) continue;
                            add_guarded(v, dloc, guard{{cond_field{.key = k, .required = true}}, std::nullopt}, fields,
                                        rejected, name, "Dependent", out, 1);
                        }
                }
                return out;
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
                        // a member that names indirect takes no initializer: clang instantiates the constructors of
                        // indirect there while the struct it holds is incomplete. std::variant<std::monostate, ...>
                        // holds null without one
                        const auto wrapped = holds_indirect(f.type);
                        if (const auto& t = f.type.k == tk::optional ? f.type.args[0] : f.type;
                            wrapped && j.is_null() && holds_null(t) && (f.required || !presence_matters(decls_[i], f.key)))
                        {
                            if (!f.required)
                            {
                                f.type = ctype(t);
                                f.omit_default = true;
                            }
                            continue;
                        }
                        auto init = f.type.k == tk::optional || wrapped ? std::nullopt
                                                                        : field_init(f.type, j, position_[i]);
                        if (f.required)
                        {
                            if (init)
                                f.init = std::move(*init);
                            else
                                warn(f.loc, "the default " + dump(j) + " is not converted to the C++ type and is dropped");
                        }
                        else if (presence_matters(decls_[i], f.key))
                        {
                            // writing would leave the key out when the member holds the default, while another key
                            // requires it (dependentRequired), a group of oneOf / anyOf counts it or minProperties
                            // counts it: std::nullopt is the missing key, which the program reads as the default in
                            // the comment. A default that the type does not read is reported as in the last branch
                            if (!j.is_null() && fits(j, f.type, f.set) == fit::no)
                                warn(f.loc, "the default " + dump(j) +
                                                " is not converted to the C++ type; the field is std::optional");
                            f.default_note = dump(j);
                            if (f.type.k != tk::optional) f.type = make(tk::optional, {std::move(f.type)});
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
                            // held through indirect to break a cycle. A function defined after the structs
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

            // whether the presence of the key is checked: another key requires it, a group of required keys has it,
            // or the struct has a minimum number of keys, which a key that writing leaves out would not count to
            static bool presence_matters(const decl& d, const std::string& key)
            {
                const auto has = [&](const std::vector<std::string>& keys) {
                    return std::ranges::find(keys, key) != keys.end();
                };
                // a count of keys in a condition, which may then hold on the object read and fail on the one written
                const auto counted = std::ranges::any_of(d.conditions, [](const condition& c) {
                    return std::ranges::any_of(std::array{&c.if_, &c.then_, &c.else_}, [](const auto* part) {
                        return std::ranges::any_of(*part, &cond_field::counts);
                    });
                });
                return d.min_properties.has_value() || counted ||
                       std::ranges::any_of(d.dependent, [&](const auto& e) { return has(e.second); }) ||
                       std::ranges::any_of(d.groups, [&](const required_groups& g) {
                           return std::ranges::any_of(g.groups, has);
                       });
            }

            // the function of yyjson::default_from for the default `j` of the field `f` of `d`, which returns the
            // value that the field holds through std::optional, indirect and std::variant<std::monostate, T>;
            // it is defined after all the structs, whose initializers it may construct
            std::optional<default_function> default_function_of(const decl& d, const field& f, const json& j) const
            {
                auto t = &f.type;
                while (true)
                {
                    const auto& r = resolved(*t);
                    if (r.k == tk::optional || r.k == tk::indirect)
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
                    {
                        // reading throws on a number that rounds to infinity, and reads one that rounds to zero as
                        // zero, which the literal says so that the compiler does not warn of the truncation
                        if (!j.is_number()) return std::nullopt;
                        const auto f32 = t.k == tk::f32;
                        const auto v = f32 ? static_cast<double>(static_cast<float>(j.num)) : j.num;
                        if (std::isinf(v)) return std::nullopt;
                        if (v == 0) return std::string(std::signbit(v) ? "-" : "") + (f32 ? "0.0f" : "0.0");
                        return float_literal(j, f32);
                    }
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
                    case tk::indirect:
                    {
                        // the constructor from a value is explicit
                        const auto v = initializer(t.args[0], j, before);
                        if (!v) return std::nullopt;
                        return spell(t) + "(" + typed_init(t.args[0], *v) + ")";
                    }
                }
                return std::nullopt;
            }

            // whether the pattern of ECMAScript `re` matches `key`; nullopt where std::regex does not read it
            std::optional<bool> pattern_matches(const std::string& re, std::string_view key) const
            {
                auto it = regexes_.find(re);
                if (it == regexes_.end())
                {
                    auto r = std::optional<std::regex>();
                    try
                    {
                        r.emplace(re, std::regex::ECMAScript);
                    }
                    catch (const std::regex_error&)
                    {
                    }
                    it = regexes_.emplace(re, std::move(r)).first;
                }
                if (!it->second) return std::nullopt;
                return std::regex_search(key.begin(), key.end(), *it->second);
            }

            // The field of `d` that receives the key `k` that no field names: the first field of
            // yyjson::pattern_properties whose pattern matches it, else the field of yyjson::extra_keys; -1 for none,
            // nullopt where std::regex does not read a pattern
            std::optional<int> receiver(const decl& d, std::string_view k) const
            {
                for (std::size_t i = 0; i < d.fields.size(); ++i)
                    if (const auto& f = d.fields[i]; f.extra && f.pattern)
                    {
                        const auto m = pattern_matches(*f.pattern, k);
                        if (!m) return std::nullopt;
                        if (*m) return static_cast<int>(i);
                    }
                for (std::size_t i = 0; i < d.fields.size(); ++i)
                    if (d.fields[i].extra && !d.fields[i].pattern) return static_cast<int>(i);
                return -1;
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
                if (d.as_array)
                {
                    // the positions by index; the elements after them go to the field of yyjson::extra_items
                    if (!j.is_array()) return std::nullopt;
                    auto items = std::vector<std::string>();
                    std::size_t i = 0;
                    for (const auto& f : d.fields)
                    {
                        if (f.extra)
                        {
                            if (i >= j.items.size()) continue;
                            auto rest = json();
                            rest.k = json::kind::array;
                            rest.items.assign(j.items.begin() + static_cast<std::ptrdiff_t>(i), j.items.end());
                            const auto v = initializer(f.type, rest, before);
                            if (!v) return std::nullopt;
                            items.push_back(member(f, *v));
                            i = j.items.size();
                        }
                        else if (i < j.items.size())
                        {
                            const auto x = initializer(f.type, j.items[i++], before);
                            if (!x) return std::nullopt;
                            items.push_back(member(f, *x));
                        }
                        else if (f.required || (f.init.empty() && needs_initializer(f.type)))
                            return std::nullopt;
                    }
                    // reading drops the other elements under yyjson::items(true)
                    if (i < j.items.size() && !d.additional) return std::nullopt;
                    if (items.size() < d.fields.size()) partial_init_ = true;
                    return "{" + join(items) + "}";
                }
                if (!j.is_object()) return std::nullopt;
                auto items = std::vector<std::string>();
                // the other keys by the field that receives them, and those that no field receives
                auto rests = std::vector<json>(d.fields.size());
                auto left = std::size_t{0};
                for (const auto& [k, v] : j.members)
                {
                    if (std::ranges::any_of(d.fields, [&](const field& f) { return !f.extra && f.key == k; }))
                        continue;
                    const auto r = receiver(d, k);
                    if (!r) return std::nullopt;
                    if (*r < 0)
                        ++left;
                    else
                    {
                        rests[static_cast<std::size_t>(*r)].k = json::kind::object;
                        rests[static_cast<std::size_t>(*r)].members.emplace_back(k, v);
                    }
                }
                for (std::size_t i = 0; i < d.fields.size(); ++i)
                {
                    const auto& f = d.fields[i];
                    if (f.extra)
                    {
                        if (rests[i].members.empty()) continue;
                        const auto v = initializer(f.type, rests[i], before);
                        if (!v) return std::nullopt;
                        items.push_back(member(f, *v));
                    }
                    else if (const auto* v = j.find(f.key))
                    {
                        const auto x = initializer(f.type, *v, before);
                        if (!x) return std::nullopt;
                        items.push_back(member(f, *x));
                    }
                    else if (f.init.empty() && needs_initializer(f.type))
                        return std::nullopt;
                }
                // reading drops the other keys of an open struct
                if (left != 0 && !d.additional) return std::nullopt;
                // reading counts all the keys of the object
                if ((d.min_properties && j.members.size() < *d.min_properties) ||
                    (d.max_properties && j.members.size() > *d.max_properties))
                    return std::nullopt;
                if (items.size() < d.fields.size()) partial_init_ = true;
                return "{" + join(items) + "}";
            }

            // whether a member of the type fails to compile when a braced initializer leaves it out: the member is
            // copy-initialized from `{}`, and the default constructor of indirect is explicit
            bool needs_initializer(const ctype& t) const
            {
                switch (t.k)
                {
                    case tk::indirect:
                        return true;
                    case tk::tuple:
                        return std::ranges::any_of(t.args, [&](const ctype& a) { return needs_initializer(a); });
                    case tk::named:
                    {
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return needs_initializer(d.target);
                        return d.kind == decl::kind_t::structure && std::ranges::any_of(d.fields, [&](const field& f) {
                                   return f.init.empty() && needs_initializer(f.type);
                               });
                    }
                    default:
                        return false;
                }
            }

            // the alternative of a variant that reading takes for a JSON value: the alternative that the tag names,
            // the only one that reads it in yyjson::one_of, or the first alternative that reads it, when the generator
            // tells that the ones before do not. An alternative whose settings the generator cannot judge (`pattern`,
            // a format of the program) is taken when every other alternative rejects the value: reading takes no
            // other one, and a value outside its settings is a default that is never written, as for a field
            std::optional<std::size_t> alternative(const ctype& t, const json& j) const
            {
                if (is_nullable(t)) return j.is_null() ? 0 : 1;
                const auto one = t.combined == combination::one_of;
                auto found = std::optional<std::size_t>();
                for (std::size_t i = 0; i < t.args.size(); ++i)
                {
                    const auto r = fits(j, t.args[i], i < t.sets.size() ? t.sets[i] : settings());
                    if (r == fit::no) continue;
                    if (found) return std::nullopt;
                    // a tagged variant reads the alternative of the tag, whose `const` excludes the others
                    if (r == fit::yes && !t.tagged && !one) return i;
                    found = i;
                }
                return found;
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
                    case tk::indirect:
                        return list(opts_.cpp26 ? "std::indirect" : "yyjson::indirect");
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
                    auto items = render_settings(t.sets[i], t.args[i], q);
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
                    auto kind = text.starts_with("yyjson::") && open != std::string::npos ? text.substr(8, open - 8)
                                                                                         : std::string("setting");
                    // const_, enum_ and not_ lose their underscore, which a number after them would double
                    while (kind.ends_with('_')) kind.pop_back();
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

            // a struct read from a JSON object
            bool is_struct(const ctype& t) const
            {
                const auto& r = resolved(t);
                return r.k == tk::named && decls_[r.decl].kind == decl::kind_t::structure && !decls_[r.decl].as_array;
            }

            // a struct of yyjson::as_array, read from a JSON array
            bool is_array_struct(const ctype& t) const
            {
                const auto& r = resolved(t);
                return r.k == tk::named && decls_[r.decl].kind == decl::kind_t::structure && decls_[r.decl].as_array;
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
                        return decls_[r.decl].kind == decl::kind_t::enumeration ? 4 : decls_[r.decl].as_array ? 6 : 8;
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
                    case tk::indirect:
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
                       s.property_names.empty() && s.additional.empty() && s.prefix.empty() && s.negated.empty() &&
                       s.conditional.empty() &&
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
                        if (x.transparent)
                        {
                            // a struct still being built may read anything; one that holds itself again adds no
                            // values there, as in the library
                            if (!x.filled) return d::partial_bound(d::universal_bound());
                            if (std::ranges::find(bound_seen_, t.decl) != bound_seen_.end()) return json_bound{};
                            bound_seen_.push_back(t.decl);
                            b = d::partial_bound(bound_of(x.fields[0].type, settings()));
                            bound_seen_.pop_back();
                            return b;
                        }
                        (x.as_array ? b.array : b.object).extent = bound_extent::partial;
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
                        b = bound_of(t.args[0], s);
                        b.null = bound_extent::exact;
                        return b;
                    case tk::indirect:
                        return bound_of(t.args[0], s);
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
                            b.null = admits_null(s) ? bound_extent::exact : bound_extent::none;
                            return b;
                        }
                        for (const auto& a : t.args) b = d::unite_bounds(b, bound_of(a, settings()));
                        return bound(b, !has);
                    }
                }
                return d::universal_bound();
            }

            // whether a nullable value reads null with the settings `s`: unless yyjson::const_ or
            // yyjson::enum_ leave nullptr out, as in the library
            static bool admits_null(const settings& s)
            {
                auto listed = false;
                auto null = true;
                for (const auto& c : s.constraints)
                    if (c.keyword == "const" || c.keyword == "enum")
                    {
                        listed = true;
                        null = null && c.null;
                    }
                return !listed || null;
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
                if (too_many_alternatives(out.size(), loc)) return {};
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

            // warns of the values of `const` or `enum` of the schema `s` that the other keywords beside them reject:
            // the bounds and the lengths, and `enum` beside `const`, which the generated code leaves out, since it
            // lists the values
            void check_listed_values(const json& s, const std::string& keyword, const std::string& loc)
            {
                const auto& listed = *s.find(keyword);
                const auto values = keyword == "const" ? std::span(&listed, 1) : std::span(listed.items);
                auto numbers = settings();
                auto strings = settings();
                for (const auto* k : {"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf"})
                    if (const auto* v = s.find(k); v != nullptr && v->is_number())
                        numbers.constraints.push_back({"", k, *v});
                for (const auto* k : {"minLength", "maxLength"})
                    if (const auto* v = s.find(k); v != nullptr && as_uint64(*v))
                        strings.constraints.push_back({"", k, *v});
                const auto* e = keyword == "const" ? s.find("enum") : nullptr;
                const auto rejected = std::ranges::count_if(values, [&](const json& v) {
                    if (e != nullptr && e->is_array() && std::ranges::none_of(e->items, [&](const json& x) {
                            return equal(x, v);
                        }))
                        return true;
                    if (v.is_number()) return fits_constraints(v, numbers) == fit::no;
                    if (v.is_string()) return fits_constraints(v, strings) == fit::no;
                    return false;
                });
                if (rejected == 0) return;
                if (std::cmp_equal(rejected, values.size()))
                    warn(loc, "no value of `" + keyword +
                                  "` satisfies the other keywords beside it: the schema accepts no value here, and "
                                  "the generated code reads the value" + (values.size() == 1 ? "" : "s") + " of `" +
                                  keyword + "`");
                else
                    warn(loc, (rejected == 1 ? std::string("a value of `enum` does")
                                             : std::to_string(rejected) + " values of `enum` do") +
                                  " not satisfy the other keywords beside it, which the schema rejects and the "
                                  "generated code reads");
            }

            // whether the value `v` is of one of the types `types` of the keyword `type`
            static bool of_types(const json& v, const std::vector<std::string>& types)
            {
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
            }

            // the values of `const` or `enum` that the generated code reads beside the types `types` of the keyword
            // `type`: the values of those types, or all of them when none is, since a value of `const` or `enum` tells
            // more of what the author meant than `type` does
            static std::vector<const json*> kept_values(std::span<const json> values,
                                                        const std::vector<std::string>& types)
            {
                auto out = std::vector<const json*>();
                for (const auto& v : values)
                    if (types.empty() || of_types(v, types)) out.push_back(&v);
                if (out.empty())
                    for (const auto& v : values) out.push_back(&v);
                return out;
            }

            // `kept_values()`, warning of the values that `type` rejects
            std::vector<const json*> typed_values(std::span<const json> values, const std::vector<std::string>& types,
                                                  const std::string& keyword, const std::string& loc)
            {
                auto out = kept_values(values, types);
                const auto kept =
                    std::ranges::count_if(values, [&](const json& v) { return types.empty() || of_types(v, types); });
                if (std::cmp_equal(kept, values.size())) return out;
                if (kept == 0)
                    warn(loc, std::string(keyword == "const" ? "the value of `const` is not" : "no value of `enum` is") +
                                  " of the `type` beside it: the schema accepts no value here, and the generated code "
                                  "reads the value" + (values.size() == 1 ? "" : "s") + " of `" + keyword +
                                  "` and ignores `type`");
                else
                {
                    const auto rejected = values.size() - static_cast<std::size_t>(kept);
                    warn(loc, (rejected == 1 ? std::string("a value of `enum` is")
                                             : std::to_string(rejected) + " values of `enum` are") +
                                  " not of the `type` beside it, which the schema rejects and the generated code "
                                  "leaves out");
                }
                return out;
            }

            // whether the subschema `sub` of a value-level `not` accepts the value `v`
            fit accepts_value(const json* sub, const json& v, const std::string& nloc)
            {
                const auto kind = v.is_null()       ? "null"
                                  : v.is_bool()     ? "boolean"
                                  : v.is_object()   ? "object"
                                  : v.is_array()    ? "array"
                                  : v.is_string()   ? "string"
                                  : v.is_integral() ? "integer"
                                                    : "number";
                const auto x = reach_of(sub, kind, nloc);
                if (x != reach::some) return x == reach::all ? fit::yes : fit::no;
                if (v.is_null() || v.is_object() || v.is_array()) return fit::unknown;
                const auto k = v.is_string()      ? tk::string
                               : v.is_bool()      ? tk::boolean
                               : !v.is_integral() ? tk::f64
                               : v.num < 9.2e18   ? tk::i64
                                                  : tk::u64;
                const auto calls = warn_calls_;
                const auto set = settings_for(*sub, make(k), nloc);
                if (!set || warn_calls_ != calls) return fit::unknown;
                return fits_constraints(v, *set);
            }

            // the type of `const` / `enum` beside a value-level `not` or `if`: the values that `not` rejects, or
            // that the branch of `if` that they take rejects, are left out
            typed type_listed_not(const json* s0, const json& s,
                                  const std::vector<std::pair<const json*, std::string>>& nots,
                                  const std::vector<value_if>& ifs, const std::string& loc, const std::string& hint,
                                  bool named)
            {
                const auto* c = s.find("const");
                const auto* e = s.find("enum");
                if (c == nullptr && (!e->is_array() || e->items.empty())) return type_enum(s0, s, loc, hint, named);
                const auto values = c != nullptr ? std::span(c, 1) : std::span(e->items);
                const auto* keyword = c != nullptr ? "const" : "enum";
                auto kept = std::vector<json>();
                auto by_not = false;
                auto by_if = false;
                for (const auto& v : values)
                {
                    auto rejected = false;
                    for (const auto& [sub, nloc] : nots)
                    {
                        const auto f = accepts_value(sub, v, nloc);
                        if (f == fit::yes)
                        {
                            rejected = by_not = true;
                            break;
                        }
                        if (f == fit::unknown)
                            warn(nloc, "whether `not` rejects the value " + dump(v) + " of `" + keyword +
                                           "` is not known; the value is kept");
                    }
                    for (const auto& vi : ifs)
                    {
                        if (rejected) break;
                        const auto unknown = [&](const char* key) {
                            warn(vi.loc + "/" + key, "whether `if` rejects the value " + dump(v) + " of `" +
                                                         keyword + "` is not known; the value is kept");
                        };
                        const auto f = accepts_value(vi.if_, v, vi.loc + "/if");
                        if (f == fit::unknown)
                        {
                            unknown("if");
                            continue;
                        }
                        const auto* key = f == fit::yes ? "then" : "else";
                        const auto* branch = f == fit::yes ? vi.then_ : vi.else_;
                        if (branch == nullptr) continue;
                        const auto b = accepts_value(branch, v, vi.loc + "/" + key);
                        if (b == fit::no) rejected = by_if = true;
                        if (b == fit::unknown) unknown(key);
                    }
                    if (!rejected) kept.push_back(v);
                }
                if (kept.empty())
                {
                    warn(loc, std::string(!by_not  ? "`if` rejects"
                                          : !by_if ? "`not` rejects"
                                                   : "`if` and `not` reject") +
                                  " every value of `" + keyword +
                                  "`: the schema accepts no value, and reading it always fails");
                    return no_value();
                }
                if (kept.size() == values.size())
                    return c != nullptr ? type_const(s0, s, loc, hint, named) : type_enum(s0, s, loc, hint, named);
                auto copy = s;
                copy.erase("const");
                copy.set("enum", json::array_of(std::move(kept)));
                const auto* p = store_merged(std::move(copy));
                return type_enum(s0, *p, loc, hint, named);
            }

            typed type_const(const json* s0, const json& s, const std::string& loc, const std::string& hint,
                             bool named)
            {
                const auto& c = *s.find("const");
                auto r = typed();
                check_listed_values(s, "const", loc);
                typed_values(std::span(&c, 1), type_list(s), "const", loc);
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
                        return type_json_values(s0, s, {c}, loc, hint, named);
                }
            }

            // the type of `const` / `enum` whose values `values` include an object or an array: the type of the
            // schema without them when every value other than null is an object, or every one an array, and
            // yyjson::value otherwise, with yyjson::const_ / yyjson::enum_ of the values as yyjson::json_text
            typed type_json_values(const json* s0, const json& s, std::vector<json> values, const std::string& loc,
                                   const std::string& hint, bool named)
            {
                const auto nullable = std::erase_if(values, [](const json& v) { return v.is_null(); }) > 0;
                const auto objects = std::ranges::all_of(values, [](const json& v) { return v.is_object(); });
                const auto arrays = std::ranges::all_of(values, [](const json& v) { return v.is_array(); });
                auto r = typed();
                if (objects || arrays)
                {
                    auto copy = s;
                    copy.erase("const");
                    copy.erase("enum");
                    auto kind = json();
                    kind.k = json::kind::string;
                    kind.text = objects ? "object" : "array";
                    copy.set("type", std::move(kind));
                    r = type_kind(objects ? "object" : "array", s0, *store_merged(std::move(copy)), loc, hint, named,
                                  false);
                }
                else if (nullable)
                    values.push_back(json());
                // the values that the type reads; reading a value whose fit is not known throws when the text of
                // the constant is read
                auto kept = std::vector<json>();
                auto dropped = 0;
                for (auto& v : values)
                {
                    const auto f = v.is_null() ? fit::yes : fits(v, r.t, r.s);
                    if (f == fit::no)
                        ++dropped;
                    else
                    {
                        if (f == fit::unknown)
                            warn(loc, "whether the type reads the value " + dump(v) +
                                          " is not known; if it does not, the first read of the type throws "
                                          "std::invalid_argument");
                        kept.push_back(std::move(v));
                    }
                }
                if (dropped > 0)
                    warn(loc, (dropped == 1 ? std::string("a value") : std::to_string(dropped) + " values") +
                                  " of `const` / `enum` " + (dropped == 1 ? "does" : "do") +
                                  " not satisfy the other keywords of the schema, which reject " +
                                  (dropped == 1 ? "it" : "them") + "; the generated code leaves " +
                                  (dropped == 1 ? "it" : "them") + " out");
                if (std::ranges::none_of(kept, [](const json& v) { return !v.is_null(); }))
                {
                    if (nullable) return {make(tk::null), {}};
                    warn(loc, "no value of `const` / `enum` satisfies the other keywords of the schema: the schema "
                              "accepts no value, and reading it always fails");
                    return no_value();
                }
                r.s.constraints.push_back(listed_texts(kept));
                if (!nullable || r.t.k == tk::any) return r;
                list_null(r.s);
                return make_nullable(std::move(r));
            }

            // a JSON value as yyjson::json_text
            static std::string json_text_literal(const json& v)
            {
                return "yyjson::json_text(" + detail::quoted(dump(v)) + ")";
            }

            // yyjson::const_ of one value, or yyjson::enum_ of several, written as yyjson::json_text
            static settings::constraint listed_texts(const std::vector<json>& values)
            {
                const auto null = std::ranges::any_of(values, [](const json& v) { return v.is_null(); });
                if (values.size() == 1)
                    return {"yyjson::const_(" + json_text_literal(values[0]) + ")", "const", values[0], null};
                auto text = std::string("yyjson::enum_(");
                for (auto first = true; const auto& v : values)
                    text += (std::exchange(first, false) ? "" : ", ") + json_text_literal(v);
                return {text + ")", "enum", json::array_of(values), null};
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
                    for (auto n = 2; members.contains(m); ++n) m = numbered(base, n);
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
                check_listed_values(s, "enum", loc);
                auto nullable = false;
                auto strings = std::vector<std::string>();
                auto numbers = std::vector<const json*>();
                auto bools = std::vector<bool>();
                const auto values = typed_values(e.items, type_list(s), "enum", loc);
                if (std::ranges::any_of(values, [](const json* v) { return v->is_object() || v->is_array(); }))
                {
                    auto all = std::vector<json>();
                    for (const auto* v : values)
                        if (std::ranges::none_of(all, [&](const json& x) { return equal(x, *v); })) all.push_back(*v);
                    return type_json_values(s0, s, std::move(all), loc, hint, named);
                }
                for (const auto* p : values)
                {
                    const auto& v = *p;
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
                if (alts.size() == 1)
                {
                    if (!nullable) return std::move(alts[0]);
                    list_null(alts[0].s);
                    return make_nullable(std::move(alts[0]));
                }
                if (nullable) alts.push_back({make(tk::monostate), {}});
                return make_variant(std::move(alts), loc);
            }

            // yyjson::const_ and yyjson::enum_ reject null unless they list it: a nullable value whose
            // values they fix lists nullptr beside them, or the JSON text null beside JSON texts
            static void list_null(settings& s)
            {
                for (auto& c : s.constraints)
                {
                    if (c.text.starts_with("yyjson::const_(yyjson::json_text("))
                        c.text = "yyjson::enum_(" + c.text.substr(15, c.text.size() - 16) +
                                 ", yyjson::json_text(\"null\"))";
                    else if (c.text.starts_with("yyjson::enum_(yyjson::json_text("))
                        c.text.insert(c.text.size() - 1, ", yyjson::json_text(\"null\")");
                    else if (c.text.starts_with("yyjson::const_("))
                        c.text = "yyjson::enum_(" + c.text.substr(15, c.text.size() - 16) + ", nullptr)";
                    else if (c.text.starts_with("yyjson::enum_("))
                        c.text.insert(c.text.size() - 1, ", nullptr");
                    else
                        continue;
                    c.null = true;
                }
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

            // the subschemas of `anyOf` or `oneOf` of a schema, each with the keywords beside it
            struct choice_parts
            {
                const json* list = nullptr;
                bool any = false;  // `anyOf`, else `oneOf`
                json base;         // the keywords beside the list
                std::vector<const json*> schemas;
                std::vector<std::string> locs;
            };

            std::optional<choice_parts> choices_of(const json& s, const std::string& loc)
            {
                // a keyword of required groups stays in the schema of each alternative
                const auto* any = required_groups_of(s, "anyOf") ? nullptr : s.find("anyOf");
                const auto* one = required_groups_of(s, "oneOf") ? nullptr : s.find("oneOf");
                // exactly one o_i and some a_j hold where exactly one of (o_i and anyOf a) holds, so `anyOf` beside
                // `oneOf` stays in the base of each alternative of `oneOf`
                if (any != nullptr && one != nullptr) any = nullptr;
                const auto* list = any != nullptr ? any : one;
                if (list == nullptr) return std::nullopt;
                auto r = choice_parts{
                    .list = list, .any = any != nullptr, .base = json::object_of(), .schemas = {}, .locs = {}};
                if (!list->is_array()) return r;
                for (const auto& [k, v] : s.members)
                    if (!(k == "anyOf" && any != nullptr) && !(k == "oneOf" && list == one) && !is_metadata_key(k) &&
                        !is_ignored_key(k))
                        r.base.members.emplace_back(k, v);
                const auto kw = std::string(any != nullptr ? "anyOf" : "oneOf");
                for (std::size_t i = 0; i < list->items.size(); ++i)
                {
                    r.locs.push_back(loc + "/" + kw + "/" + std::to_string(i));
                    if (r.base.members.empty() || ref_within_base(r.base, list->items[i], r.locs.back()))
                        r.schemas.push_back(&list->items[i]);
                    else
                    {
                        auto c = r.base;
                        c.set("allOf", json::array_of({list->items[i]}));
                        r.schemas.push_back(store_merged(std::move(c)));
                    }
                }
                return r;
            }

            typed type_variant(const json* s0, const json* s, const std::string& loc, const std::string& hint,
                               bool named)
            {
                auto parts = *choices_of(*s, loc);
                const auto* list = parts.list;
                const auto kw = std::string(parts.any ? "anyOf" : "oneOf");
                if (!list->is_array() || list->items.empty())
                {
                    warn(loc, "`" + kw + "` is not a non-empty array; the value is read as any JSON value");
                    return {};
                }
                const auto& base = parts.base;
                auto& schemas = parts.schemas;
                auto& locs = parts.locs;
                if (auto r = string_choices(s0, *s, base, *list, kw, loc, hint, named)) return *r;
                // an alternative that accepts no value, such as one of a type that the base excludes, reads nothing
                for (std::size_t i = schemas.size(); i-- > 0;)
                    if (const auto* n = normalize(schemas[i], locs[i]); n->is_bool() && !n->b)
                    {
                        schemas.erase(schemas.begin() + static_cast<std::ptrdiff_t>(i));
                        locs.erase(locs.begin() + static_cast<std::ptrdiff_t>(i));
                    }
                if (schemas.empty())
                {
                    warn(loc, "no subschema of `" + kw + "` accepts a value; the value is read as any JSON value");
                    return {};
                }
                if (auto r = tagged_variant(schemas, locs, loc)) return *r;
                if (auto r = nullable_tagged_variant(base, *list, kw, locs, loc)) return *r;
                auto alts = std::vector<typed>();
                auto metas = std::vector<metadata>();
                for (std::size_t i = 0; i < schemas.size(); ++i)
                {
                    const auto* n = normalize(schemas[i], locs[i]);
                    const auto* title = n->is_object() ? n->find("title") : nullptr;
                    const auto h = title != nullptr && title->is_string() && !pascal(title->text).empty()
                                       ? pascal(title->text)
                                       : hint + "Option" + std::to_string(i + 1);
                    // an alternative that the keywords of the base leave without a value, such as a string beside
                    // `not: {minimum: 3}`, is dropped with the warnings that typing it gave
                    const auto warned = warnings_.size();
                    const auto calls = warn_calls_;
                    auto a = type_of(schemas[i], locs[i], h);
                    if (is_no_value(a))
                    {
                        for (auto j = warned; j < warnings_.size(); ++j)
                            warned_.erase({warnings_[j].location, warnings_[j].message});
                        warnings_.resize(warned);
                        warn_calls_ = calls;
                        continue;
                    }
                    alts.push_back(std::move(a));
                    adopt_metadata(alts.back().t, *n);
                    // the metadata of the alternative, unless its named type has the same
                    auto m = meta_of(*n);
                    m.comment.reset();
                    const auto& t = alts.back().t;
                    if (t.k == tk::named && same_metadata(decls_[t.decl].meta, m)) m = metadata();
                    metas.push_back(std::move(m));
                }
                if (alts.empty())
                {
                    warn(loc, "no subschema of `" + kw + "` accepts a value with the keywords beside it; reading it "
                              "always fails");
                    return no_value();
                }
                return make_variant(std::move(alts), loc, parts.any ? combination::any_of : combination::one_of,
                                    std::move(metas));
            }

            // `anyOf` / `oneOf` whose subschemas are only strings by `const` or `enum` (and null) become one enum class,
            // with the metadata of each subschema as comments of its enumerators; a reference stays an alternative
            std::optional<typed> string_choices(const json* s0, const json& s, const json& base, const json& list,
                                                const std::string& kw, const std::string& loc, const std::string& hint,
                                                bool named)
            {
                auto base_null = true;
                // an `enum` beside `oneOf` / `anyOf` keeps the strings (and null) that it lists
                const auto* base_enum = base.find("enum");
                if (base_enum != nullptr &&
                    (!base_enum->is_array() || std::ranges::any_of(base_enum->items, [](const json& v) {
                        return !v.is_string() && !v.is_null();
                    })))
                    return std::nullopt;
                const auto listed = [&](const json& v) {
                    return base_enum == nullptr || std::ranges::any_of(base_enum->items, [&](const json& x) {
                               return x.k == v.k && x.text == v.text;
                           });
                };
                // a `type` beside keeps the values of its types, or all of them when none is, as `kept_values()`
                const auto base_types = type_list(base);
                if (!base.members.empty())
                {
                    if (std::ranges::any_of(base.members,
                                            [](const auto& m) {
                                                return m.first != "type" && m.first != "enum" &&
                                                       (is_type_key(m.first) || is_ignored_key(m.first) ||
                                                        is_condition_key(m.first));
                                            }) ||
                        (base.find("type") != nullptr && base_types.empty()))
                        return std::nullopt;
                    if (base.find("type") != nullptr)
                        base_null = std::ranges::find(base_types, "null") != base_types.end();
                }
                if (!listed(json())) base_null = false;
                auto values = std::vector<std::string>();
                auto metas = std::map<std::string, metadata>();
                auto nullable = false;
                auto duplicate = false;
                // the subschemas whose values `type` rejects, warned of once the enum class is made
                auto checks = std::vector<std::pair<std::size_t, std::vector<std::string>>>();
                for (std::size_t i = 0; i < list.items.size(); ++i)
                {
                    const auto& b = list.items[i];
                    if (!b.is_object() ||
                        std::ranges::any_of(b.members, [](const auto& m) {
                            return m.first != "const" && m.first != "enum" && m.first != "type" &&
                                   (is_type_key(m.first) || is_ignored_key(m.first) ||
                                    is_condition_key(m.first));
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
                    const auto all = c != nullptr     ? std::span(c, 1)
                                     : e->is_array() ? std::span<const json>(e->items)
                                                     : std::span<const json>();
                    const auto& types = t.empty() ? base_types : t;
                    const auto own = kept_values(all, types);
                    if (own.empty() || std::ranges::any_of(own, [](const json* v) { return !v->is_string(); }))
                        return std::nullopt;
                    if (!types.empty() && !std::ranges::all_of(all, [&](const json& v) { return of_types(v, types); }))
                        checks.emplace_back(i, types);
                    auto m = meta_of(b);
                    if (!m.description) m.description = m.title;
                    m.title.reset();
                    m.examples.clear();
                    m.deprecated = false;
                    for (const auto* v : own)
                    {
                        if (!listed(*v)) continue;
                        if (std::ranges::find(values, v->text) != values.end())
                        {
                            duplicate = true;
                            continue;
                        }
                        values.push_back(v->text);
                        if (!metadata_empty(m)) metas.emplace(v->text, m);
                    }
                }
                if (values.empty()) return std::nullopt;
                for (const auto& [i, types] : checks)
                {
                    const auto& b = list.items[i];
                    const auto* c = b.find("const");
                    typed_values(c != nullptr ? std::span(c, 1) : std::span(b.find("enum")->items), types,
                                 c != nullptr ? "const" : "enum", loc + "/" + kw + "/" + std::to_string(i));
                }
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

            // whether an alternative that only refers to a schema keeps its reference instead of becoming the base
            // and the referred schema: merging them would expand a recursive reference without end.
            // - The referred schema is being typed, as in `{"properties": {...}, "oneOf": [{...}, {"$ref": "#"}]}`:
            //   the alternative reads what the referred schema accepts, which every valid value meets.
            // - The base is only a `type` that the referred schema already meets:
            //   `{"type": "object", "anyOf": [{...}, {"$ref": "#"}]}`.
            bool ref_within_base(const json& base, const json& alt, const std::string& loc)
            {
                if (!is_pure_ref(alt)) return false;
                const auto* t = &alt;
                for (auto n = 0; t != nullptr && is_pure_ref(*t) && n < 64; ++n)
                {
                    t = resolve(*t->find("$ref"), loc);
                    if (t == nullptr || !busy_.contains(t)) continue;
                    if (std::ranges::any_of(base.members, [&](const auto& m) {
                            const auto* v = t->is_object() ? t->find(m.first) : nullptr;
                            return v == nullptr || !equal(*v, m.second);
                        }))
                        warn(loc, "the alternative refers to a schema that holds it; it is read without the keywords "
                                  "beside it");
                    return true;
                }
                if (base.members.size() != 1 || base.members[0].first != "type") return false;
                if (t == nullptr || !t->is_object()) return false;
                const auto have = type_list(*t);
                const auto allowed = type_list(base);
                return !have.empty() && std::ranges::all_of(have, [&](const auto& x) {
                    return std::ranges::find(allowed, x) != allowed.end() ||
                           (x == "integer" && std::ranges::find(allowed, "number") != allowed.end());
                });
            }

            // a tagged variant under a base type that also allows null, as in {"type": ["object", "null"], "anyOf":
            // [{"properties": {"kind": {"const": "a"}}, ...}, ...]}: null is a subschema of its own, valid when
            // `anyOf` has a subschema that does not constrain the type, or `oneOf` exactly one
            std::optional<typed> nullable_tagged_variant(const json& base, const json& list, const std::string& kw,
                                                         const std::vector<std::string>& locs, const std::string& loc)
            {
                auto types = type_list(base);
                if (types.size() < 2 || std::ranges::find(types, "null") == types.end()) return std::nullopt;
                auto taking_null = std::size_t();
                for (const auto& b : list.items)
                {
                    if (!b.is_object() || b.has("$ref") || b.has("const") || b.has("enum") || b.has("allOf") ||
                        b.has("anyOf") || b.has("oneOf") || b.has("not"))
                        return std::nullopt;
                    const auto t = type_list(b);
                    if (t.empty() || std::ranges::find(t, "null") != t.end()) ++taking_null;
                }
                std::erase(types, "null");
                auto stripped = base;
                stripped.set("type", type_json(types));
                auto schemas = std::vector<const json*>();
                auto all_locs = std::vector<std::string>();
                if (kw == "anyOf" ? taking_null > 0 : taking_null == 1)
                {
                    auto n = json::object_of();
                    n.set("type", json::string_of("null"));
                    schemas.push_back(store(std::move(n)));
                    all_locs.push_back(loc);
                }
                for (std::size_t i = 0; i < list.items.size(); ++i)
                {
                    auto c = stripped;
                    c.set("allOf", json::array_of({list.items[i]}));
                    schemas.push_back(store(std::move(c)));
                    all_locs.push_back(locs[i]);
                }
                return tagged_variant(schemas, all_locs, loc);
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
                    // {"allOf": [{"$ref": ...}]} beside metadata, which normalizing keeps as the reference
                    if (is_pure_ref(*n)) n = normalize_full(n, locs[i]);
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
                // yyjson::internal_tag takes neither an empty key nor an empty name of an alternative
                for (const auto& [key, unused] : first->members)
                {
                    if (key.empty()) continue;
                    auto ids = std::vector<std::string>();
                    for (const auto i : objs)
                    {
                        const auto* v = required(*norm[i], key) ? tag_value(*norm[i], key, locs[i]) : nullptr;
                        if (v == nullptr || v->text.empty() || std::ranges::find(ids, v->text) != ids.end()) break;
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
                    if (too_many_alternatives(args.size(), loc)) return typed{};
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
                    if (const auto* p = s.find("pattern"); p != nullptr && (p->is_string() || p->is_array()))
                    {
                        if (opts_.pattern)
                        {
                            for (const auto& x : p->is_array() ? p->items : std::vector<json>{*p})
                                if (x.is_string())
                                    out.constraints.push_back(
                                        {"yyjson::pattern(" + cpp_string(x.text) + ")", "pattern", x});
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
                            std::set<std::string_view>{"idn-email", "idn-hostname", "iri", "iri-reference"};
                        if (supported.contains(f->text))
                            out.constraints.push_back({"yyjson::format(" + cpp_string(f->text) + ")", "format", *f});
                        else if (f->text == "regex")
                        {
                            if (opts_.pattern)
                            {
                                out.constraints.push_back({"yyjson::format(\"regex\")", "format", *f});
                                uses_regex_format_ = true;
                            }
                            else
                                warn(loc, "the format \"regex\" is dropped (--no-pattern)");
                        }
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

            // the constraints of `const` / `enum` on a value of kind `k`, or nothing when they do not apply; null is
            // listed as nullptr when `null` is true
            std::optional<std::vector<settings::constraint>> value_constraints(const json& s, tk k, bool null = false)
            {
                auto out = std::vector<settings::constraint>();
                const auto lit = [&](const json& v) -> std::optional<std::string> {
                    if (null && v.is_null()) return "nullptr";
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
                    out.push_back({"yyjson::const_(" + *l + ")", "const", *c, c->is_null()});
                }
                if (const auto* e = s.find("enum"))
                {
                    if (!e->is_array()) return std::nullopt;
                    auto c = std::string("yyjson::enum_(");
                    auto n = 0;
                    auto listed_null = false;
                    for (const auto& v : e->items)
                    {
                        const auto l = lit(v);
                        if (!l) continue;  // a value of another type: never read into this kind
                        c += (n++ > 0 ? ", " : "") + *l;
                        listed_null = listed_null || v.is_null();
                    }
                    if (n == 0) return std::nullopt;
                    out.push_back({c + ")", "enum", *e, listed_null});
                }
                return out;
            }

            // `const` and `enum` of an enumeration by the JSON names of its enumerators, which yyjson::const_ and
            // yyjson::enum_ take; std::nullopt when they name none. Null is listed as nullptr when `null` is true
            std::optional<std::vector<settings::constraint>> enumerator_constraints(const json& s, const ctype& t,
                                                                                    bool null = false) const
            {
                const auto& names = decls_[t.decl].enumerators;
                const auto known = [&](const json& v) {
                    return (null && v.is_null()) ||
                           (v.is_string() &&
                            std::ranges::any_of(names, [&](const auto& e) { return e.second == v.text; }));
                };
                const auto lit = [](const json& v) { return v.is_null() ? std::string("nullptr") : cpp_string(v.text); };
                auto out = std::vector<settings::constraint>();
                if (const auto* c = s.find("const"))
                {
                    if (!known(*c)) return std::nullopt;
                    out.push_back({"yyjson::const_(" + lit(*c) + ")", "const", *c, c->is_null()});
                }
                if (const auto* e = s.find("enum"))
                {
                    if (!e->is_array()) return std::nullopt;
                    auto list = std::vector<std::string>();
                    auto listed_null = false;
                    for (const auto& v : e->items)
                        if (known(v))
                        {
                            list.push_back(lit(v));
                            listed_null = listed_null || v.is_null();
                        }
                    if (list.empty()) return std::nullopt;
                    out.push_back({"yyjson::enum_(" + join(list) + ")", "enum", *e, listed_null});
                }
                return out;
            }

            // the type of a value: behind aliases, std::optional, indirect and nullable variants
            const ctype& value_type(const ctype& t) const
            {
                const auto& r = resolved(t);
                if (r.k == tk::optional || r.k == tk::indirect) return value_type(r.args[0]);
                if (is_nullable(r)) return value_type(r.args[1]);
                return r;
            }

            // the JSON kinds of the values of a type, as bits; a floating type holds integers too
            static constexpr unsigned kind_null = 1, kind_boolean = 2, kind_integer = 4, kind_real = 8,
                                      kind_string = 16, kind_array = 32, kind_object = 64, kind_all = 127;
            unsigned kinds_of(const ctype& t) const
            {
                const auto& r = resolved(t);
                switch (r.k)
                {
                    case tk::any: return kind_all;
                    case tk::object:
                    case tk::map: return kind_object;
                    case tk::null:
                    case tk::monostate: return kind_null;
                    case tk::boolean: return kind_boolean;
                    case tk::i32:
                    case tk::i64:
                    case tk::u64: return kind_integer;
                    case tk::f32:
                    case tk::f64: return kind_integer | kind_real;
                    case tk::string: return kind_string;
                    case tk::vector:
                    case tk::tuple: return kind_array;
                    case tk::optional: return kind_null | kinds_of(r.args[0]);
                    case tk::indirect: return kinds_of(r.args[0]);
                    case tk::variant:
                    {
                        if (r.tagged) return kind_object;
                        auto k = 0u;
                        for (const auto& a : r.args) k |= kinds_of(a);
                        return k;
                    }
                    case tk::named:
                    {
                        const auto& d = decls_[r.decl];
                        if (d.kind == decl::kind_t::enumeration) return kind_string;
                        if (d.as_array) return kind_array;
                        if (d.transparent)
                            for (const auto& f : d.fields)
                                if (!f.extra) return kinds_of(f.type);
                        return kind_object;
                    }
                }
                return kind_all;
            }

            // whether no value of the type `target` satisfies the normalized schema `n` by its `type`, `const` or
            // `enum`; false when it is not known
            bool value_never(const json& n, const ctype& target) const
            {
                if (!n.is_object()) return false;
                const auto& t = value_type(target);
                const auto kinds = kinds_of(t) | (may_be_null(target) ? kind_null : 0u);
                if (const auto types = type_list(n); !types.empty())
                {
                    auto allowed = 0u;
                    for (const auto& x : types)
                        allowed |= x == "null"      ? kind_null
                                   : x == "boolean" ? kind_boolean
                                   : x == "integer" ? kind_integer | kind_real
                                   : x == "number"  ? kind_integer | kind_real
                                   : x == "string"  ? kind_string
                                   : x == "array"   ? kind_array
                                   : x == "object"  ? kind_object
                                                    : kind_all;
                    if ((kinds & allowed) == 0) return true;
                }
                const auto is_enum = t.k == tk::named && decls_[t.decl].kind == decl::kind_t::enumeration;
                const auto holds = [&](const json& v) {
                    if (v.is_null()) return (kinds & kind_null) != 0;
                    if (v.is_bool()) return (kinds & kind_boolean) != 0;
                    if (v.is_number()) return (kinds & (v.is_integral() ? kind_integer | kind_real : kind_real)) != 0;
                    if (v.is_string())
                        return is_enum ? std::ranges::any_of(decls_[t.decl].enumerators,
                                                             [&](const auto& e) { return e.second == v.text; })
                                       : (kinds & kind_string) != 0;
                    return (kinds & (v.is_array() ? kind_array : kind_object)) != 0;
                };
                if (const auto* c = n.find("const"); c != nullptr && !holds(*c)) return true;
                if (const auto* e = n.find("enum");
                    e != nullptr && e->is_array() && std::ranges::none_of(e->items, holds))
                    return true;
                return false;
            }

            // whether a value of the type may be an enumeration or a struct of yyjson::as_array, whose values the
            // settings of a value of several kinds do not test by text or by the number of items
            bool holds_named(const ctype& t, bool as_array) const
            {
                const auto& r = resolved(t);
                if (r.k == tk::named)
                {
                    const auto& d = decls_[r.decl];
                    if (d.transparent)
                        return std::ranges::any_of(d.fields, [&](const field& f) {
                            return !f.extra && holds_named(f.type, as_array);
                        });
                    return as_array ? d.kind == decl::kind_t::structure && d.as_array
                                    : d.kind == decl::kind_t::enumeration;
                }
                if (r.k == tk::variant && r.tagged) return false;
                if (r.k == tk::variant || r.k == tk::optional || r.k == tk::indirect)
                    return std::ranges::any_of(r.args, [&](const ctype& a) { return holds_named(a, as_array); });
                return false;
            }

            // the settings of a value of several kinds, yyjson::value or a variant: each keyword constrains the
            // values of its kind, as in JSON Schema. `type` must take every kind of the value, unless `const` or
            // `enum` list the values; keywords of the kinds that the value cannot be are left out
            std::optional<settings> multi_kind_settings(const json& n, const ctype& target, const ctype& t,
                                                        const std::string& loc)
            {
                const auto kinds = kinds_of(t) | (may_be_null(target) ? kind_null : 0u);
                auto allowed = kind_all;
                if (const auto types = type_list(n); !types.empty())
                {
                    allowed = 0;
                    for (const auto& x : types)
                        allowed |= x == "null"      ? kind_null
                                   : x == "boolean" ? kind_boolean
                                   : x == "integer" ? kind_integer
                                   : x == "number"  ? kind_integer | kind_real
                                   : x == "string"  ? kind_string
                                   : x == "array"   ? kind_array
                                   : x == "object"  ? kind_object
                                                    : 0u;
                }
                const auto listed = n.has("const") || n.has("enum");
                if ((kinds & ~allowed) != 0 && !listed) return std::nullopt;
                auto out = settings();
                const auto kind_of = [](const json& v) {
                    return v.is_null()                        ? kind_null
                           : v.is_bool()                      ? kind_boolean
                           : v.is_number() && v.is_integral() ? kind_integer
                           : v.is_number()                    ? kind_real
                           : v.is_string()                    ? kind_string
                           : v.is_array()                     ? kind_array
                                                              : kind_object;
                };
                // an object or an array among the values that the value can be: every value as yyjson::json_text
                auto texts = false;
                for (const auto* keyword : {"const", "enum"})
                    if (const auto* l = n.find(keyword); l != nullptr)
                        for (const auto& v : *keyword == 'c' || !l->is_array() ? std::span(l, 1) : std::span(l->items))
                            texts = texts || ((v.is_array() || v.is_object()) && (kinds & allowed & kind_of(v)) != 0);
                // a literal of a listed value; "" for a value that the type or `type` leaves out
                const auto lit = [&](const json& v) -> std::optional<std::string> {
                    const auto kind = kind_of(v);
                    if ((kinds & allowed & kind) == 0) return "";
                    if (texts) return json_text_literal(v);
                    if (kind == kind_null) return "nullptr";
                    if (kind == kind_boolean) return v.b ? "true" : "false";
                    if (kind == kind_string) return cpp_string(v.text);
                    if (kind == kind_integer)
                    {
                        if (const auto i = as_int64(v)) return int_literal(*i);
                        if (const auto u = as_uint64(v)) return uint_literal(*u);
                    }
                    return float_literal(v);
                };
                if (const auto* c = n.find("const"))
                {
                    const auto l = lit(*c);
                    if (!l || l->empty()) return std::nullopt;
                    out.constraints.push_back({"yyjson::const_(" + *l + ")", "const", *c, c->is_null()});
                }
                if (const auto* e = n.find("enum"))
                {
                    if (!e->is_array()) return std::nullopt;
                    auto list = std::vector<std::string>();
                    auto listed_null = false;
                    for (const auto& v : e->items)
                    {
                        const auto l = lit(v);
                        if (!l) return std::nullopt;
                        if (l->empty()) continue;
                        list.push_back(*l);
                        listed_null = listed_null || v.is_null();
                    }
                    if (list.empty()) return std::nullopt;
                    out.constraints.push_back({"yyjson::enum_(" + join(list) + ")", "enum", *e, listed_null});
                }
                const auto any_key = [&](std::initializer_list<std::string_view> keys) {
                    return std::ranges::any_of(keys, [&](auto k) { return n.has(k); });
                };
                if ((kinds & kind_string) != 0 && any_key({"minLength", "maxLength", "pattern", "format"}))
                {
                    if (holds_named(t, false)) return std::nullopt;
                    scalar_constraints(n, tk::string, out, loc);
                }
                if ((kinds & (kind_integer | kind_real)) != 0)
                {
                    const auto number = [](const json& v) {
                        if (v.is_integral())
                        {
                            if (const auto i = as_int64(v)) return int_literal(*i);
                            if (const auto u = as_uint64(v)) return uint_literal(*u);
                        }
                        return float_literal(v);
                    };
                    for (const auto& [key, name] :
                         {std::pair{"minimum", "minimum"}, std::pair{"maximum", "maximum"},
                          std::pair{"exclusiveMinimum", "exclusive_minimum"},
                          std::pair{"exclusiveMaximum", "exclusive_maximum"}})
                        if (const auto* v = n.find(key))
                        {
                            if (!v->is_number())
                                warn(loc, std::string("`") + key + "` is not a number and is ignored");
                            else
                                out.constraints.push_back(
                                    {std::string("yyjson::") + name + "(" + number(*v) + ")", key, *v});
                        }
                    if (const auto* v = n.find("multipleOf"))
                    {
                        if (!v->is_number() || v->num <= 0)
                            warn(loc, "`multipleOf` is not a positive number and is ignored");
                        else
                            out.constraints.push_back({"yyjson::multiple_of(" + number(*v) + ")", "multipleOf", *v});
                    }
                }
                if ((kinds & kind_array) != 0)
                {
                    if (any_key({"items", "prefixItems", "contains", "minContains", "maxContains"}))
                        return std::nullopt;
                    if (const auto* u = n.find("uniqueItems"); u != nullptr && !(u->is_bool() && !u->b))
                        return std::nullopt;
                    if (any_key({"minItems", "maxItems"}))
                    {
                        if (holds_named(t, true)) return std::nullopt;
                        array_counts(n, out, loc);
                    }
                }
                if ((kinds & kind_object) != 0 &&
                    any_key({"properties", "required", "additionalProperties", "patternProperties", "minProperties",
                             "maxProperties", "propertyNames", "dependentRequired"}))
                    return std::nullopt;
                return out;
            }

            // the settings of an element level from a schema, or nothing when they cannot be expressed
            std::optional<settings> settings_for(const json& schema, const ctype& target, const std::string& loc,
                                                 const std::string& hint = {})
            {
                const auto* n = normalize_full(&schema, loc);
                if (n->is_bool()) return n->b ? std::optional<settings>(settings()) : std::nullopt;
                if (!n->is_object()) return std::nullopt;
                for (const auto* k : {"anyOf", "oneOf", "$ref", "not", "if", "$conditions"})
                    if (n->has(k)) return std::nullopt;
                const auto& t = value_type(target);
                if (t.k == tk::any || (t.k == tk::variant && !t.tagged)) return multi_kind_settings(*n, target, t, loc);
                const auto is_enum = t.k == tk::named && decls_[t.decl].kind == decl::kind_t::enumeration;
                const auto is_obj = t.k == tk::map || t.k == tk::object || t.k == tk::variant || is_struct(t);
                if (const auto types = type_list(*n); !types.empty())
                {
                    const auto has = [&](std::string_view x) { return std::ranges::find(types, x) != types.end(); };
                    // only null, of a value that can be null: yyjson::const_(nullptr), which the keywords of
                    // the other kinds add nothing to
                    if (types.size() == 1 && has("null") && may_be_null(target) && !n->has("const") &&
                        !n->has("enum"))
                    {
                        auto out = settings();
                        out.constraints.push_back({"yyjson::const_(nullptr)", "const", json(), true});
                        return out;
                    }
                    const auto covered = t.k == tk::string || is_enum              ? has("string")
                                         : is_integer_kind(t.k)                    ? has("integer") || has("number")
                                         : is_float_kind(t.k)                      ? has("number")
                                         : t.k == tk::boolean                      ? has("boolean")
                                         : t.k == tk::vector || t.k == tk::tuple ||
                                                 is_array_struct(t)                ? has("array")
                                         : is_obj                                  ? has("object")
                                         : t.k == tk::null || t.k == tk::monostate ? has("null")
                                                                                   : types.size() >= 6;
                    if (!covered) return std::nullopt;
                }
                auto out = settings();
                const auto scalar =
                    t.k == tk::string || t.k == tk::boolean || is_integer_kind(t.k) || is_float_kind(t.k);
                if ((n->has("const") || n->has("enum")) && !scalar && !is_enum)
                {
                    // the values that the type reads, as JSON text
                    for (const auto* keyword : {"const", "enum"})
                    {
                        const auto* l = n->find(keyword);
                        if (l == nullptr) continue;
                        if (*keyword == 'e' && (!l->is_array() || l->items.empty())) return std::nullopt;
                        auto kept = std::vector<json>();
                        for (const auto& v : *keyword == 'c' ? std::span(l, 1) : std::span(l->items))
                            if (v.is_null() ? may_be_null(target) : fits(v, t, settings()) != fit::no)
                                kept.push_back(v);
                        if (kept.empty()) return std::nullopt;
                        out.constraints.push_back(listed_texts(kept));
                    }
                }
                else if (n->has("const") || n->has("enum"))
                {
                    // null is read into a nullable value, and the settings reject it unless they list it
                    const auto null = may_be_null(target) && accepts_null(*n);
                    const auto c = is_enum ? enumerator_constraints(*n, t, null) : value_constraints(*n, t.k, null);
                    if (!c) return std::nullopt;
                    out.constraints = *c;
                }
                const auto any_key = [&](std::initializer_list<std::string_view> keys) {
                    return std::ranges::any_of(keys, [&](auto k) { return n->has(k); });
                };
                const auto string_keys = any_key({"minLength", "maxLength", "pattern", "format"});
                const auto array_keys = any_key({"items", "prefixItems", "contains", "minItems", "maxItems",
                                                 "uniqueItems", "minContains", "maxContains"});
                const auto object_keys =
                    any_key({"properties", "required", "additionalProperties", "patternProperties", "minProperties",
                             "maxProperties", "propertyNames", "dependentRequired"});
                if (t.k == tk::string || is_integer_kind(t.k) || is_float_kind(t.k))
                    scalar_constraints(*n, t.k, out, loc);
                if (is_enum && string_keys) return std::nullopt;
                if ((t.k == tk::tuple || is_array_struct(t)) && array_keys) return std::nullopt;
                if (t.k == tk::vector && array_keys)
                {
                    const auto* prefix = n->find("prefixItems");
                    if (prefix != nullptr && !prefix->is_array()) return std::nullopt;
                    if (const auto* items = n->find("items"); items != nullptr && items->is_bool() && !items->b)
                        out.max_items = prefix != nullptr ? prefix->items.size() : 0;
                    else if (items != nullptr)
                    {
                        auto e = settings_for(*items, t.args[0], loc + "/items", hint + "Item");
                        if (!e) return std::nullopt;
                        if (!e->empty()) out.items.push_back(std::move(*e));
                    }
                    if (prefix != nullptr)
                    {
                        auto ps = std::vector<settings>();
                        for (std::size_t i = 0; i < prefix->items.size(); ++i)
                        {
                            auto p = settings_for(prefix->items[i], t.args[0],
                                                  loc + "/prefixItems/" + std::to_string(i),
                                                  hint + "Item" + std::to_string(i + 1));
                            if (!p) return std::nullopt;
                            ps.push_back(std::move(*p));
                        }
                        set_prefix(out, std::move(ps), loc);
                    }
                    if (!array_counts(*n, out, loc)) return std::nullopt;
                    if (n->has("uniqueItems") && n->find("uniqueItems")->is_bool() && n->find("uniqueItems")->b)
                        out.unique = true;
                    if (n->has("contains") && !add_contains(*n, out, t.args[0], loc, hint)) return std::nullopt;
                }
                if ((is_struct(t) || t.k == tk::object || t.k == tk::variant) && object_keys) return std::nullopt;
                if (t.k == tk::map && object_keys)
                {
                    if (any_key({"properties", "required", "patternProperties", "dependentRequired"}))
                        return std::nullopt;
                    if (const auto* ap = n->find("additionalProperties"))
                    {
                        auto e = settings_for(*ap, t.args[0], loc + "/additionalProperties", hint + "Value");
                        if (!e) return std::nullopt;
                        if (!e->empty()) out.additional.push_back(std::move(*e));
                    }
                    if (const auto* pn = n->find("propertyNames"))
                    {
                        auto e = key_settings(*pn, loc + "/propertyNames");
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

            // a type that carries its settings: the type, or yyjson::any_of of one yyjson::alt with the settings
            static ctype constrained(typed t)
            {
                if (t.s.empty()) return std::move(t.t);
                auto r = make(tk::variant, {std::move(t.t)});
                r.combined = combination::any_of;
                r.sets.push_back(std::move(t.s));
                r.metas.emplace_back();
                return r;
            }

            // contains / minContains / maxContains; false when `contains` cannot be expressed. Elements of
            // yyjson::value or yyjson::object are tested by yyjson::is_type<T> of the type of `contains`, named after
            // `hint`; when that type reads more than `contains` allows, `maxContains` is left out
            bool add_contains(const json& s, settings& out, const ctype& elem, const std::string& loc,
                              const std::string& hint)
            {
                const auto* c = s.find("contains");
                if (c == nullptr) return true;
                const auto cloc = loc + "/contains";
                const auto* mn = s.find("minContains");
                const auto* mx = s.find("maxContains");
                const auto min_c = mn != nullptr && as_uint64(*mn) ? *as_uint64(*mn) : 1;
                auto max_c = mx != nullptr ? as_uint64(*mx) : std::nullopt;
                if (const auto* cn = normalize_full(c, cloc); cn->is_bool() && !cn->b)
                {
                    // no element matches: maxContains holds, and minContains of 1 or more fails for every array
                    if (min_c > 0) warn(cloc, "`contains` false accepts no array and is ignored");
                    return true;
                }
                const auto calls = warn_calls_;
                auto cs = settings_for(*c, elem, cloc);
                if (!cs)
                {
                    const auto& v = value_type(elem);
                    if (v.k != tk::any && v.k != tk::object) return false;
                    auto t = type_of(c, cloc, (hint.empty() ? std::string("Element") : hint) + "Contains");
                    cs = settings();
                    if (t.t.k != tk::any || !t.s.empty())
                    {
                        cs->type_test = static_cast<int>(type_tests_.size());
                        type_tests_.push_back(constrained(std::move(t)));
                    }
                }
                if (max_c && warn_calls_ != calls)
                {
                    warn(loc + "/maxContains",
                         "`maxContains` is ignored, since the elements that `contains` matches are not known exactly");
                    max_c.reset();
                }
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

            static bool has_type_test(const settings& s)
            {
                if (s.type_test >= 0) return true;
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
                    for (const auto& c : *v)
                        if (has_type_test(c)) return true;
                return false;
            }

            static std::size_t node_count(const settings& s)
            {
                auto n = std::size_t(1);
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
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
                    // the text of yyjson::json_text, and null that list_null() adds
                    if (c.text.find("yyjson::json_text(") != std::string::npos)
                    {
                        for (const auto& v : c.keyword == "const" ? std::span(&c.value, 1) : std::span(c.value.items))
                            chars += dump(v).size();
                        if (c.null && !(c.keyword == "const" ? c.value.is_null()
                                                             : std::ranges::any_of(c.value.items, [](const json& v) {
                                                                   return v.is_null();
                                                               })))
                            chars += 4;
                        continue;
                    }
                    if (c.value.is_string()) chars += c.value.text.size();
                    if (c.value.is_array())
                        for (const auto& v : c.value.items)
                            if (v.is_string()) chars += v.text.size();
                }
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
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
                        auto start = i;
                        auto end = i + 1;
                        if (text.ends_with('R'))
                        {
                            // a raw string literal R"delim(...)delim"
                            text.pop_back();
                            --start;
                            const auto open = c.text.find('(', i);
                            const auto delim = c.text.substr(i + 1, open - i - 1);
                            end = c.text.find(")" + delim + "\"", open) + delim.size() + 1;
                        }
                        else
                            while (c.text[end] != '"') end += c.text[end] == '\\' ? 2 : 1;
                        const auto literal = std::string_view(c.text).substr(start, end + 1 - start);
                        if (text.ends_with("std::string_view("))
                            text += literal;
                        else
                            text += "std::string_view(" + std::string(literal) + ")";
                        i = end;
                    }
                    c.text = std::move(text);
                }
                for (auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
                    for (auto& c : *v) view_strings(c);
            }

            // prefix_items settings within the limits of the library; call it after the settings of `items` are in
            // `out`: they apply only after the positions, so a prefix without settings is kept to start them there
            void set_prefix(settings& out, std::vector<settings> prefix, const std::string& loc)
            {
                const auto empty = [](const auto& p) { return p.empty(); };
                if (out.items.empty() && std::ranges::all_of(prefix, empty)) return;
                if (prefix.size() > 16)
                {
                    if (out.items.empty())
                        warn(loc, "constraints of more than 16 positions of `prefixItems` are not supported; they are "
                                  "dropped");
                    else
                    {
                        warn(loc, "constraints of more than 16 positions of `prefixItems` are not supported; they are "
                                  "dropped with the constraints of `items`");
                        out.items.clear();
                    }
                    return;
                }
                for (auto& p : prefix)
                    if (has_type_test(p))
                    {
                        warn(loc, "`contains` with a type in a position of `prefixItems` is not supported; the "
                                  "constraints of the position are dropped");
                        p = {};
                    }
                    else if (node_count(p) > 8)
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
                if (out.items.empty() && std::ranges::all_of(prefix, empty)) return;
                out.prefix = std::move(prefix);
            }

            typed type_array(const json* s0, const json& s, const std::string& loc, const std::string& hint,
                             bool named, bool conditional = false)
            {
                const auto* prefix = s.find("prefixItems");
                const auto* items = s.find("items");
                auto items_false = false;
                // a reference is followed with the location of its target, as type_body() does, so that the
                // warnings of the target keep that location
                auto iloc = loc + "/items";
                const auto* it = items;
                for (auto k = 0; it != nullptr && is_pure_ref(*it) && k < 64; ++k)
                {
                    const auto* ref = it->find("$ref");
                    it = resolve(*ref, iloc);
                    iloc = ref->text;
                }
                if (it != nullptr && !is_pure_ref(*it))
                {
                    const auto* n = normalize(it, iloc);
                    items_false = n->is_bool() && !n->b;
                }
                auto r = typed();
                if (prefix != nullptr && prefix->is_array() && !prefix->items.empty())
                {
                    // a position that accepts no value ends the array before it, and so does `maxItems`
                    auto n = prefix->items.size();
                    for (std::size_t i = 0; i < n; ++i)
                        if (const auto* p = normalize_full(&prefix->items[i], loc + "/prefixItems/" + std::to_string(i));
                            p->is_bool() && !p->b)
                        {
                            n = i;
                            items_false = true;
                        }
                    if (const auto m = s.has("maxItems") ? as_uint64(*s.find("maxItems")) : std::nullopt; m && *m <= n)
                    {
                        n = static_cast<std::size_t>(*m);
                        items_false = true;
                    }
                    if (n == 0)
                    {
                        r.t = make(tk::vector, {make(tk::any)});
                        r.s.max_items = 0;
                        array_counts(s, r.s, loc);
                        return r;
                    }
                    auto pos = std::vector<typed>();
                    for (std::size_t i = 0; i < n; ++i)
                        pos.push_back(type_of(&prefix->items[i], loc + "/prefixItems/" + std::to_string(i),
                                              hint + "Item" + std::to_string(i + 1)));
                    const auto* mi = s.find("minItems");
                    if (items_false && mi != nullptr && as_uint64(*mi) && *as_uint64(*mi) > n)
                        warn(loc, "`minItems` is more than the positions of `prefixItems`, and no element may follow "
                                  "them; no array satisfies the schema");
                    // a tuple cannot compare its positions or test them by `contains`; a struct of yyjson::as_array can
                    const auto* u = s.find("uniqueItems");
                    const auto whole = (u != nullptr && u->is_bool() && u->b) || s.has("contains");
                    if (items_false && mi != nullptr && as_uint64(*mi) && *as_uint64(*mi) >= n && !whole)
                    {
                        r.t = make(tk::tuple);
                        auto ps = std::vector<settings>();
                        for (auto& p : pos)
                        {
                            r.t.args.push_back(std::move(p.t));
                            ps.push_back(std::move(p.s));
                        }
                        set_prefix(r.s, std::move(ps), loc);
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
                        if (!items_false && !rest.s.empty()) r.s.items.push_back(std::move(rest.s));
                        set_prefix(r.s, std::move(ps), loc);
                    }
                    // beside a value-level `if`, which tests the positions by settings of yyjson::value
                    else if (auto vs = conditional && (items_false || (rest.t.k == tk::any && rest.s.empty()))
                                           ? value_positions(*prefix, n, loc, hint)
                                           : std::nullopt)
                    {
                        r.t = make(tk::vector, {make(tk::any)});
                        set_prefix(r.s, std::move(*vs), loc);
                    }
                    else if (!opts_.cpp26 && n + (items_false ? 0 : 1) > max_reflected_fields)
                    {
                        warn(loc, "positions of `prefixItems` of different types are read as yyjson::value without "
                                  "--cpp26 when there are more than " +
                                      std::to_string(max_reflected_fields) +
                                      " of them; their constraints are dropped");
                        r.t = make(tk::vector, {make(tk::any)});
                    }
                    else
                        return make_array_struct(s0, s, std::move(pos), items_false, std::move(rest), loc, hint, named);
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
                if (!add_contains(s, r.s, r.t.args[0], loc, hint))
                    warn(loc + "/contains",
                         "`contains` with a type other than the element type is not supported and is ignored");
                return r;
            }

            // the settings of yyjson::value that test the first `n` positions of `prefix` exactly, or nullopt
            std::optional<std::vector<settings>> value_positions(const json& prefix, std::size_t n,
                                                                 const std::string& loc, const std::string& hint)
            {
                auto out = std::vector<settings>();
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto ploc = loc + "/prefixItems/" + std::to_string(i);
                    const auto& p = prefix.items[i];
                    if (const auto* x = normalize_full(&p, ploc); adds_nothing(*x))
                    {
                        out.emplace_back();
                        continue;
                    }
                    const auto calls = warn_calls_;
                    auto set = settings_for(p, make(tk::any), ploc);
                    if (!set || warn_calls_ != calls)
                        set = type_test_of(&p, ploc, hint + "Item" + std::to_string(i + 1), {});
                    if (!set) return std::nullopt;
                    out.push_back(std::move(*set));
                }
                return out;
            }

            // a struct of yyjson::as_array for an array whose positions have different types: a field for each
            // position, which is required below `minItems`, and a field of yyjson::extra_items for the elements after
            // them when `items` has a schema
            typed make_array_struct(const json* s0, const json& s, std::vector<typed> pos, bool closed, typed rest,
                                    const std::string& loc, const std::string& hint, bool named)
            {
                const auto n = pos.size();
                const auto* mi = s.find("minItems");
                const auto min = mi != nullptr ? as_uint64(*mi).value_or(0) : 0;
                const auto& prefix = *s.find("prefixItems");
                auto fields = std::vector<field>();
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto ploc = loc + "/prefixItems/" + std::to_string(i);
                    const auto& sub = prefix.items[i];
                    const auto* target = normalize_full(&sub, ploc);
                    const auto* own = is_pure_ref(sub) ? &sub : normalize(&sub, ploc);
                    const auto required = i < min;
                    const auto* dflt = own->find("default");
                    if (dflt == nullptr && target->is_object()) dflt = target->find("default");
                    if (dflt == nullptr && required && target->is_object()) dflt = target->find("const");
                    // the member takes the name of the title of the position; name_members() makes it an identifier
                    const auto* title = own->find("title");
                    auto name = title != nullptr && title->is_string() ? snake(title->text) : std::string();
                    if (name.empty()) name = "item" + std::to_string(i + 1);
                    auto f = field{.member = name, .key = name, .meta = meta_of(*own), .loc = ploc};
                    f.set = std::move(pos[i].s);
                    place_default(f, std::move(pos[i].t), dflt, required);
                    fields.push_back(std::move(f));
                }
                // the settings of the whole array: the numbers of elements beyond the positions, which a closed
                // array has warned of, and `uniqueItems` and `contains`, which see the elements as JSON values
                auto whole = settings();
                if (min > n && !closed) whole.min_items = min;
                if (const auto* mx = s.find("maxItems"); mx != nullptr && !closed)
                    if (const auto m = as_uint64(*mx)) whole.max_items = *m;
                if (const auto* u = s.find("uniqueItems"); u != nullptr && u->is_bool() && u->b) whole.unique = true;
                (void)add_contains(s, whole, make(tk::any), loc, hint);
                // the elements after the positions: rejected, accepted and dropped, or kept in the field of
                // yyjson::extra_items
                const auto any_rest = rest.t.k == tk::any && rest.s.empty();
                if (!closed && (!any_rest || opts_.keep_unknown_keys))
                {
                    auto f = field{.type = make(tk::vector, {std::move(rest.t)}), .loc = loc + "/items"};
                    f.extra = true;
                    if (!rest.s.empty()) f.set.items.push_back(std::move(rest.s));
                    fields.push_back(std::move(f));
                }
                // the struct of the schema still being typed is the transparent struct of self_struct(), which holds
                // this one
                const auto own_name = named && !self_structs_.contains(s0);
                auto d = decl{.kind = decl::kind_t::structure,
                              .name = own_name ? hint : unique_type_name(hint),
                              .loc = loc,
                              .fields = std::move(fields),
                              .additional = !closed,
                              .filled = true,
                              .as_array = true,
                              .set = std::move(whole)};
                if (own_name)
                {
                    d.meta = meta_of(s);
                    if (const auto it = def_keys_.find(s0); it != def_keys_.end()) d.schema_name = it->second;
                }
                return typed{named_type(add_decl(std::move(d))), {}};
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
                if (const auto* pn = s.find("propertyNames");
                    pn != nullptr && reach_of(pn, "string", loc + "/propertyNames") == reach::none)
                    r.s.max_items = 0;
                else if (pn != nullptr)
                {
                    if (auto e = key_settings(*pn, loc + "/propertyNames"); !e)
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
                        const auto base = f.pattern ? sanitize(f.member) : f.extra ? std::string("extra") : sanitize(f.key);
                        // a member cannot take the name of a type, which it would hide in the struct
                        auto m = base;
                        if (type_names_.contains(m) && !base.ends_with('_')) m += "_";
                        for (auto n = 2; type_names_.contains(m) || used.contains(m); ++n) m = numbered(base, n);
                        used.insert(m);
                        f.member = m;
                        // the field of a transparent struct has no key, and a position has none either, so
                        // yyjson::rename has nothing to rename
                        if (d.transparent || d.as_array) f.key = m;
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

            // holds the field through indirect. A nullable field stays std::variant<std::monostate, indirect<T>>:
            // indirect has no empty state, and the default constructor makes the variant hold null. The field of a
            // transparent struct is the value itself, which std::optional would let be null
            void wrap(field& f, bool bare)
            {
                auto t = f.type;
                if (t.k == tk::optional) t = ctype(t.args[0]);
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
                    t = make(tk::variant, {make(tk::monostate),
                                           make(tk::indirect, {v.args.size() == 1 && !v.tagged ? v.args[0] : v})});
                }
                else
                    t = make(tk::indirect, {std::move(t)});
                f.type = f.required || bare ? std::move(t) : make(tk::optional, {std::move(t)});
                f.init.clear();
                f.omit_default = false;
            }

            // breaks cycles of structs held by value with indirect: a struct that holds itself by value, directly
            // or through others, holds the field that closes the cycle through indirect
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
                            wrap(f, decls_[i].transparent);
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

            static constexpr int unending = std::numeric_limits<int>::max();

            // the depth in structs of the value that `T()` makes, `unending` when the struct holds itself through
            // indirect again. `lv` gives the depth of each struct; with `l1`, the depths of the structs with the
            // variants as they are, a variant whose first alternative does not end takes its best alternative that
            // can come first
            int level_of(const ctype& t, const std::vector<int>& lv, const std::vector<int>* l1) const
            {
                switch (t.k)
                {
                    case tk::named:
                    {
                        const auto& d = decls_[t.decl];
                        if (d.kind == decl::kind_t::alias) return level_of(d.target, lv, l1);
                        return d.kind == decl::kind_t::structure ? lv[t.decl] : 0;
                    }
                    case tk::indirect:
                        return level_of(t.args[0], lv, l1);
                    case tk::tuple:
                    {
                        auto r = 0;
                        for (const auto& a : t.args) r = std::max(r, level_of(a, lv, l1));
                        return r;
                    }
                    case tk::variant:
                    {
                        if (t.args.empty()) return 0;
                        if (l1 == nullptr || level_of(t.args[0], *l1, nullptr) != unending)
                            return level_of(t.args[0], lv, l1);
                        auto r = unending;
                        for (std::size_t k = 0; k < t.args.size(); ++k)
                            if (can_come_first(t, k)) r = std::min(r, level_of(t.args[k], lv, l1));
                        return r;
                    }
                    default:
                        return 0;
                }
            }

            // the depths of the structs, by rounds: a struct ends once all its fields do
            std::vector<int> struct_levels(const std::vector<int>* l1) const
            {
                auto lv = std::vector<int>(decls_.size(), unending);
                for (auto changed = true; changed;)
                {
                    changed = false;
                    const auto prev = lv;
                    for (std::size_t i = 0; i < decls_.size(); ++i)
                    {
                        if (decls_[i].kind != decl::kind_t::structure || prev[i] != unending) continue;
                        auto r = 0;
                        for (const auto& f : decls_[i].fields) r = std::max(r, level_of(f.type, prev, l1));
                        if (r == unending) continue;
                        lv[i] = r + 1;
                        changed = true;
                    }
                }
                return lv;
            }

            // whether the alternative `k` of the variant can come first without changing what reading takes: the
            // tag or yyjson::one_of picks the alternative, otherwise the alternatives before it read none of its values
            bool can_come_first(const ctype& t, std::size_t k) const
            {
                if (k == 0 || t.tagged || t.combined == combination::one_of) return true;
                const auto set = [&](std::size_t i) { return i < t.sets.size() ? t.sets[i] : settings(); };
                const auto b = bound_of(t.args[k], set(k));
                for (std::size_t i = 0; i < k; ++i)
                    if (!yyjson::detail::bounds_disjoint(bound_of(t.args[i], set(i)), b) &&
                        !structs_disjoint(t.args[i], t.args[k]))
                        return false;
                return true;
            }

            // whether no object reads as both structs: one has no other keys and the other requires a key that it
            // does not have, or a key that both require has values that the two fields do not share
            bool structs_disjoint(const ctype& a, const ctype& b) const
            {
                if (!is_struct(a) || !is_struct(b)) return false;
                const auto& x = decls_[resolved(a).decl];
                const auto& y = decls_[resolved(b).decl];
                if (x.transparent || y.transparent) return false;
                const auto find = [](const decl& d, const std::string& key) -> const field* {
                    for (const auto& f : d.fields)
                        if (!f.extra && f.key == key) return &f;
                    return nullptr;
                };
                const auto excludes = [&](const decl& closed, const decl& other) {
                    return !closed.additional && std::ranges::any_of(other.fields, [&](const field& f) {
                               return f.required && !f.extra && find(closed, f.key) == nullptr &&
                                      receiver(closed, f.key) == -1;
                           });
                };
                if (excludes(x, y) || excludes(y, x)) return true;
                return std::ranges::any_of(x.fields, [&](const field& f) {
                    const auto* g = f.required && !f.extra ? find(y, f.key) : nullptr;
                    return g != nullptr && g->required &&
                           yyjson::detail::bounds_disjoint(bound_of(f.type, f.set), bound_of(g->type, g->set));
                });
            }

            // makes `T()` end for every struct: a variant whose first alternative holds the variant again takes an
            // alternative that ends first, and a field held through indirect that no value ends holds null too
            void end_defaults()
            {
                for (;;)
                {
                    const auto l1 = struct_levels(nullptr);
                    const auto l2 = struct_levels(&l1);
                    field* found = nullptr;
                    for (std::size_t i = 0; i < decls_.size() && found == nullptr; ++i)
                        if (decls_[i].kind == decl::kind_t::structure && l2[i] == unending)
                            for (auto& f : decls_[i].fields)
                                if (f.type.k == tk::indirect && level_of(f.type.args[0], l2, &l1) == unending)
                                {
                                    found = &f;
                                    break;
                                }
                    if (found == nullptr)
                    {
                        reorder_alternatives(l1, l2);
                        return;
                    }
                    const auto& held = resolved(found->type.args[0]);
                    warn(found->loc,
                         "a default-constructed " + spell(found->type.args[0]) + " holds this field again without end" +
                             (held.k == tk::variant ? " (no alternative that ends can come first: an alternative "
                                                      "before it would read its values)"
                                                    : "") +
                             "; the field is std::variant<std::monostate, ...> and reads null too");
                    found->type = make(tk::variant, {make(tk::monostate), std::move(found->type)});
                }
            }

            void reorder_alternatives(const std::vector<int>& l1, const std::vector<int>& l2)
            {
                // before the alternatives inside, which the depths of the structs saw as they were
                const auto visit = [&](const auto& self, ctype& t, std::vector<std::string>* ids) -> void {
                    if (t.k == tk::named) return;
                    if (t.k == tk::variant && !t.args.empty() && level_of(t.args[0], l1, nullptr) == unending)
                    {
                        auto best = std::size_t{0};
                        auto level = unending;
                        for (std::size_t k = 0; k < t.args.size(); ++k)
                            if (const auto n = can_come_first(t, k) ? level_of(t.args[k], l2, &l1) : unending;
                                n < level)
                            {
                                best = k;
                                level = n;
                            }
                        if (best != 0)
                        {
                            const auto rotate = [&](auto& v) {
                                if (v.size() == t.args.size())
                                    std::rotate(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(best),
                                                v.begin() + static_cast<std::ptrdiff_t>(best) + 1);
                            };
                            rotate(t.args);
                            rotate(t.sets);
                            rotate(t.metas);
                            if (ids != nullptr) rotate(*ids);
                            t.reordered = true;
                        }
                    }
                    for (auto& a : t.args) self(self, a, nullptr);
                };
                for (auto& d : decls_)
                {
                    if (d.kind == decl::kind_t::alias) visit(visit, d.target, nullptr);
                    for (auto& f : d.fields) visit(visit, f.type, nullptr);
                }
                // internal_tag lists the tags in the order of the alternatives
                for (auto& n : notes_) visit(visit, n.type, &n.ids);
            }

            // the first alternative of a variant in `t` that reorder_alternatives() moved
            static const ctype* moved_first(const ctype& t)
            {
                if (t.k == tk::named) return nullptr;
                if (t.reordered) return &t.args[0];
                for (const auto& a : t.args)
                    if (const auto* m = moved_first(a)) return m;
                return nullptr;
            }

            std::string moved_note(const ctype& t, std::string_view indent) const
            {
                const auto* m = moved_first(t);
                if (m == nullptr) return {};
                return std::string(indent) + "// " + spell(*m) +
                       " comes first among the alternatives: a default-constructed value holds it,\n" +
                       std::string(indent) + "// and the alternatives before it in the schema hold the type again\n";
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
                for (const auto& s : t.sets) named_in(s, out);
            }

            // the types of yyjson::is_type<T> in the settings
            void named_in(const settings& s, std::vector<int>& out) const
            {
                if (s.type_test >= 0) named_in(type_tests_[s.type_test], out);
                for (const auto* v : {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
                    for (const auto& e : *v) named_in(e, out);
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

            // yyjson::const_ / yyjson::enum_ of a value that is not a scalar: JSON values compared as JSON
            static fit fits_listed(const json& j, const settings& s)
            {
                for (const auto& c : s.constraints)
                {
                    if (c.keyword != "const" && c.keyword != "enum") continue;
                    if (j.is_null() && c.null) continue;
                    if (c.keyword == "const" ? !equal(j, c.value)
                                             : std::ranges::none_of(c.value.items,
                                                                    [&](const json& e) { return equal(j, e); }))
                        return fit::no;
                }
                return fit::yes;
            }

            fit fits_struct(const json& j, const decl& d) const
            {
                if (d.transparent) return fits(j, d.fields[0].type, d.fields[0].set);
                if (d.as_array)
                {
                    // the positions by index, then the elements after them
                    if (!j.is_array()) return fit::no;
                    auto r = fit::yes;
                    std::size_t i = 0;
                    for (const auto& f : d.fields)
                    {
                        if (f.extra)
                        {
                            for (; i < j.items.size() && r != fit::no; ++i)
                                r = std::min(r, fits(j.items[i], f.type.args[0],
                                                     f.set.items.empty() ? settings() : f.set.items[0]));
                        }
                        else if (i < j.items.size())
                            r = std::min(r, fits(j.items[i++], f.type, f.set));
                        else if (f.required)
                            return fit::no;
                        if (r == fit::no) return r;
                    }
                    if (i < j.items.size() && !d.additional) return fit::no;
                    return d.set.empty() ? r : std::min(r, fits(j, make(tk::vector, {make(tk::any)}), d.set));
                }
                if (!j.is_object()) return fit::no;
                auto r = fit::yes;
                const field* extra = nullptr;
                const auto value_fit = [&](const json& v, const field& m) {
                    return fits(v, m.type.args[0], m.set.additional.empty() ? settings() : m.set.additional[0]);
                };
                // the value of the key `k` against every field of yyjson::pattern_properties whose pattern matches
                // `k`; whether one matches, nullopt where none does and std::regex does not read a pattern
                const auto patterns_fit = [&](std::string_view k, const json& v) {
                    auto matched = std::optional<bool>(false);
                    for (const auto& p : d.fields)
                    {
                        if (!p.pattern) continue;
                        const auto m = pattern_matches(*p.pattern, k);
                        if (!m)
                        {
                            r = std::min(r, fit::unknown);
                            if (matched == false) matched.reset();
                        }
                        else if (*m)
                        {
                            matched = true;
                            r = std::min(r, value_fit(v, p));
                        }
                    }
                    return matched;
                };
                for (const auto& f : d.fields)
                {
                    if (f.extra)
                    {
                        if (!f.pattern) extra = &f;
                        continue;
                    }
                    if (const auto* v = j.find(f.key))
                    {
                        r = std::min(r, fits(*v, f.type, f.set));
                        patterns_fit(f.key, *v);
                    }
                    else if (f.required)
                        return fit::no;
                    if (r == fit::no) return r;
                }
                for (const auto& [k, v] : j.members)
                {
                    if (std::ranges::any_of(d.fields, [&](const field& f) { return !f.extra && f.key == k; })) continue;
                    // a key that a pattern may match is checked by the pattern only
                    if (const auto m = patterns_fit(k, v); m == false)
                    {
                        if (extra != nullptr)
                            r = std::min(r, value_fit(v, *extra));
                        else if (!d.additional)
                            return fit::no;
                    }
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
                // reading counts all the keys of the object
                if ((d.min_properties && j.members.size() < *d.min_properties) ||
                    (d.max_properties && j.members.size() > *d.max_properties))
                    return fit::no;
                return r;
            }

            // whether the value `j` reads as `t` and satisfies `s`, with the `yyjson::not_` of `s`
            fit fits(const json& j, const ctype& t, const settings& s) const
            {
                auto r = fits_value(j, t, s);
                if ((s.negated.empty() && s.conditional.empty()) || r == fit::no) return r;
                if (j.is_null() && (resolved(t).k == tk::optional || is_nullable(t))) return r;
                const auto test = [&](const settings& n) {
                    return n.type_test >= 0 ? fits(j, type_tests_[n.type_test], settings()) : fits(j, t, n);
                };
                for (const auto& n : s.negated)
                {
                    const auto inner = test(n);
                    if (inner == fit::yes) return fit::no;
                    if (inner == fit::unknown) r = std::min(r, fit::unknown);
                }
                for (std::size_t i = 0; i + 2 < s.conditional.size() && r != fit::no; i += 3)
                {
                    const auto c = test(s.conditional[i]);
                    const auto a = fits(j, t, s.conditional[i + 1]);
                    const auto b = fits(j, t, s.conditional[i + 2]);
                    r = std::min(r, c == fit::yes  ? a
                                    : c == fit::no ? b
                                    : a == fit::no && b == fit::no ? fit::no
                                                                   : fit::unknown);
                }
                return r;
            }

            fit fits_value(const json& j, const ctype& t, const settings& s) const
            {
                const auto scalar = [&](bool kind) { return kind ? fits_constraints(j, s) : fit::no; };
                const auto at = [](const std::vector<settings>& v, std::size_t i) {
                    return i < v.size() ? v[i] : settings();
                };
                switch (t.k)
                {
                    case tk::any:
                        return fits_listed(j, s);
                    case tk::object:
                        return j.is_object() ? fits_listed(j, s) : fit::no;
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
                        if (d.kind == decl::kind_t::structure)
                            return fits_listed(j, s) == fit::no ? fit::no : fits_struct(j, d);
                        return scalar(j.is_string() && std::ranges::any_of(d.enumerators, [&](const auto& e) {
                                          return e.second == j.text;
                                      }));
                    }
                    case tk::vector:
                    {
                        if (!j.is_array() || fits_listed(j, s) == fit::no) return fit::no;
                        if ((s.min_items && j.items.size() < *s.min_items) ||
                            (s.max_items && j.items.size() > *s.max_items))
                            return fit::no;
                        if (s.unique)
                            for (std::size_t a = 0; a < j.items.size(); ++a)
                                for (auto b = a + 1; b < j.items.size(); ++b)
                                    if (equal(j.items[a], j.items[b])) return fit::no;
                        auto r = fit::yes;
                        for (const auto& e : j.items)
                            if ((r = std::min(r, fits(e, t.args[0], at(s.items, 0)))) == fit::no) return r;
                        if (s.contains.empty()) return r;
                        if (has_type_test(s.contains[0])) return std::min(r, fit::unknown);
                        auto matched = std::uint64_t{0};
                        auto unknown = std::uint64_t{0};
                        for (const auto& e : j.items)
                        {
                            const auto c = fits(e, t.args[0], s.contains[0]);
                            matched += c == fit::yes ? 1 : 0;
                            unknown += c == fit::unknown ? 1 : 0;
                        }
                        const auto lo = s.min_contains.value_or(1);
                        const auto hi = s.max_contains.value_or(std::numeric_limits<std::uint64_t>::max());
                        if (matched + unknown < lo || matched > hi) return fit::no;
                        if (matched < lo || matched + unknown > hi) return std::min(r, fit::unknown);
                        return r;
                    }
                    case tk::map:
                    {
                        if (!j.is_object() || fits_listed(j, s) == fit::no) return fit::no;
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
                        if (!j.is_array() || j.items.size() != t.args.size() || fits_listed(j, s) == fit::no)
                            return fit::no;
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
                            if (fits_listed(j, s) == fit::no) return fit::no;
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
                        if (!is_nullable(t) && fits_listed(j, s) == fit::no) return fit::no;
                        auto r = fit::no;
                        for (std::size_t i = 0; i < t.args.size(); ++i)
                            r = std::max(r, fits(j, t.args[i], is_nullable(t) ? s : at(t.sets, i)));
                        return r;
                    }
                    case tk::optional:
                        return j.is_null() ? fit::yes : fits(j, t.args[0], s);
                    case tk::indirect:
                        return fits(j, t.args[0], s);
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
                    warn(loc, count(unknown) + " may not read as the C++ type: the generator does not check `pattern`, "
                                               "`contains` and the format \"regex\"; the comment `example:` has " +
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

            // The settings `s` of a value of the type `t` that leave no value to read, as the library tells them at
            // compile time (`yyjson::detail::settings_read_nothing<>()`): the reason, or nothing
            std::optional<std::string> contradiction(const ctype& t, const settings& s) const
            {
                const auto& v = value_type(t);
                const auto items = v.k == tk::map ? std::pair("`minProperties`", "`maxProperties`")
                                                  : std::pair("`minItems`", "`maxItems`");
                if (s.min_items && s.max_items && *s.min_items > *s.max_items)
                    return std::string(items.first) + " is greater than " + items.second;
                const auto number = is_integer_kind(v.k) || is_float_kind(v.k);
                if (!number && v.k != tk::string && v.k != tk::boolean) return std::nullopt;
                if (v.k == tk::string)
                {
                    const auto lengths = lengths_of(s);
                    if (lengths.high < lengths.low) return "`minLength` is greater than `maxLength`";
                }
                if (!reads_nothing(bound_of(v, s))) return std::nullopt;
                if (listed_values(s, v.k)) return "no value of `const` and `enum` is within the other constraints";
                return "no number of the C++ type " + spell(v) + " is within the bounds";
            }

            // warns of the settings of a field, an element or an alternative that leave no value to read, which the
            // schema has, as in `{"minLength": 3, "maxLength": 2}`; the generated code has them as they are, which the
            // library rejects at compile time
            void check_level(const ctype& t, const settings& s, const std::string& loc)
            {
                if (s.empty()) return;
                if (const auto r = contradiction(t, s))
                    warn(loc, *r + ": the schema accepts no value here, and the generated code does not compile");
                if (s.min_contains && s.max_contains && *s.min_contains > *s.max_contains && !s.contains.empty())
                    warn(loc, "`minContains` is greater than `maxContains`: the schema accepts no array here, and "
                              "reading one always fails");
                const auto& v = value_type(t);
                const auto elem = [&]() -> ctype {
                    return v.k == tk::vector || v.k == tk::map ? v.args[0] : make(tk::any);
                };
                for (std::size_t i = 0; i < s.prefix.size(); ++i)
                    check_level(v.k == tk::tuple ? v.args[i] : elem(), s.prefix[i],
                                loc + "/prefixItems/" + std::to_string(i));
                if (!s.items.empty()) check_level(elem(), s.items[0], loc + "/items");
                if (!s.contains.empty()) check_level(elem(), s.contains[0], loc + "/contains");
                if (!s.property_names.empty())
                    check_level(make(tk::string), s.property_names[0], loc + "/propertyNames");
                if (!s.additional.empty()) check_level(elem(), s.additional[0], loc + "/additionalProperties");
            }

            // the settings of the alternatives of yyjson::any_of and yyjson::one_of within the type `t`
            void check_alternative_levels(const ctype& t, const std::string& loc)
            {
                for (std::size_t i = 0; i < t.args.size(); ++i)
                {
                    if (t.k == tk::variant && i < t.sets.size()) check_level(t.args[i], t.sets[i], loc);
                    check_alternative_levels(t.args[i], loc);
                }
            }

            void check_levels()
            {
                for (const auto& d : decls_)
                {
                    if (d.kind == decl::kind_t::alias) check_alternative_levels(d.target, d.loc);
                    if (d.kind == decl::kind_t::structure && d.max_properties)
                    {
                        const auto required = static_cast<std::uint64_t>(
                            std::ranges::count_if(d.fields, [](const field& f) { return !f.extra && f.required; }));
                        if (d.min_properties && *d.min_properties > *d.max_properties)
                            warn(d.loc, "`minProperties` is greater than `maxProperties`: the schema accepts no object "
                                        "here, and the generated code does not compile");
                        else if (required > *d.max_properties)
                            warn(d.loc, "`required` names more keys than `maxProperties` allows: the schema accepts no "
                                        "object here, and reading one always fails");
                    }
                    if (d.kind == decl::kind_t::structure && d.as_array)
                        check_level(make(tk::vector, {make(tk::any)}), d.set, d.loc);
                    for (const auto& f : d.fields)
                    {
                        check_level(f.type, f.set, f.loc);
                        check_alternative_levels(f.type, f.loc);
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

            // the settings as the arguments of an annotation; `q` qualifies the types of the generated code, which the
            // annotations of C++20 name from outside the namespace
            std::vector<std::string> render_settings(const settings& s, const ctype& target,
                                                     std::string_view q = {}) const
            {
                auto out = std::vector<std::string>();
                if (s.type_test >= 0)
                    out.push_back("yyjson::is_type<" + spell(type_tests_[s.type_test], false, q) + ">");
                for (const auto& c : s.constraints) out.push_back(c.text);
                if (s.min_items) out.push_back("yyjson::min_items(" + std::to_string(*s.min_items) + ")");
                if (s.max_items) out.push_back("yyjson::max_items(" + std::to_string(*s.max_items) + ")");
                const auto& v = value_type(target);
                const auto elem = [&]() -> ctype {
                    return v.k == tk::vector || v.k == tk::map ? v.args[0] : make(tk::any);
                };
                if (s.unique) out.emplace_back("yyjson::unique_items");
                // positions without settings are written when `items` follows them: it applies only after them
                const auto items =
                    s.items.empty() ? std::vector<std::string>() : render_settings(s.items[0], elem(), q);
                if (!s.prefix.empty())
                {
                    auto parts = std::vector<std::string>();
                    for (std::size_t i = 0; i < s.prefix.size(); ++i)
                    {
                        const auto p = render_settings(s.prefix[i], v.k == tk::tuple ? v.args[i] : elem(), q);
                        parts.push_back(p.empty() ? "{}" : p.size() == 1 ? p[0] : "{" + join(p) + "}");
                    }
                    if (!items.empty() || std::ranges::any_of(parts, [](const auto& p) { return p != "{}"; }))
                        out.push_back("yyjson::prefix_items(" + join(parts) + ")");
                }
                if (!items.empty()) out.push_back("yyjson::items(" + join(items) + ")");
                if (!s.contains.empty())
                    if (const auto p = render_settings(s.contains[0], elem(), q); !p.empty())
                    {
                        out.push_back("yyjson::contains(" + join(p) + ")");
                        if (s.min_contains)
                            out.push_back("yyjson::min_contains(" + std::to_string(*s.min_contains) + ")");
                        if (s.max_contains)
                            out.push_back("yyjson::max_contains(" + std::to_string(*s.max_contains) + ")");
                    }
                if (!s.property_names.empty())
                    if (const auto p = render_settings(s.property_names[0], make(tk::string), q); !p.empty())
                        out.push_back("yyjson::property_names(" + join(p) + ")");
                if (!s.additional.empty())
                    if (const auto p = render_settings(s.additional[0], elem(), q); !p.empty())
                        out.push_back("yyjson::additional_properties(" + join(p) + ")");
                for (const auto& n : s.negated)
                    if (const auto p = render_settings(n, target, q); !p.empty())
                        out.push_back("yyjson::not_(" + join(p) + ")");
                for (std::size_t i = 0; i + 2 < s.conditional.size(); i += 3)
                {
                    auto parts =
                        std::vector<std::string>{"yyjson::if_(" + join(render_settings(s.conditional[i], target, q)) + ")"};
                    if (const auto p = render_settings(s.conditional[i + 1], target, q); !p.empty())
                        parts.push_back("yyjson::then(" + join(p) + ")");
                    if (const auto p = render_settings(s.conditional[i + 2], target, q); !p.empty())
                        parts.push_back("yyjson::else_(" + join(p) + ")");
                    if (parts.size() > 1) out.push_back("yyjson::if_then_else(" + join(parts) + ")");
                }
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

            std::vector<std::string> field_items(const field& f, bool as_array)
            {
                auto items = std::vector<std::string>();
                // the annotations of C++20 are outside the namespace
                const auto q = opts_.cpp26 || opts_.ns.empty() ? std::string() : opts_.ns + "::";
                if (f.extra)
                {
                    // the field has no key, and takes only the settings of its values or elements
                    if (f.pattern)
                        items.push_back("yyjson::pattern_properties(" + cpp_string(*f.pattern) + ")");
                    else
                        items.emplace_back(as_array ? "yyjson::extra_items" : "yyjson::extra_keys");
                    for (auto& s : render_settings(f.set, f.type, q)) items.push_back(std::move(s));
                    return items;
                }
                if (f.member != f.key) items.push_back("yyjson::rename(" + cpp_string(f.key) + ")");
                if (f.required) items.emplace_back("yyjson::required");
                if (f.omit_default) items.emplace_back("yyjson::omit_default");
                if (!f.default_from.empty()) items.push_back("yyjson::default_from<&" + q + f.default_from + ">");
                for (auto& s : render_settings(f.set, f.type, q)) items.push_back(std::move(s));
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
                if (d.as_array)
                {
                    // explicit, so that a read under yyjson::strict accepts the elements that the schema accepts
                    items.emplace_back("yyjson::as_array");
                    if (!std::ranges::any_of(d.fields, [](const field& f) { return f.extra; }))
                        items.emplace_back(d.additional ? "yyjson::items(true)" : "yyjson::items(false)");
                    // the annotations of C++20 are outside the namespace
                    const auto q = opts_.cpp26 || opts_.ns.empty() ? std::string() : opts_.ns + "::";
                    for (auto& x : render_settings(d.set, make(tk::vector, {make(tk::any)}), q))
                        items.push_back(std::move(x));
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
                if (d.min_properties) items.push_back("yyjson::min_properties(" + std::to_string(*d.min_properties) + ")");
                if (d.max_properties) items.push_back("yyjson::max_properties(" + std::to_string(*d.max_properties) + ")");
                // the annotations of C++20 are outside the namespace
                const auto q = opts_.cpp26 || opts_.ns.empty() ? std::string() : opts_.ns + "::";
                const auto fields = [&](std::string_view part, const std::vector<cond_field>& cs, bool closed = false) {
                    auto fs = std::vector<std::string>();
                    auto counts = std::vector<std::string>();
                    for (const auto& c : cs)
                    {
                        if (c.counts)
                        {
                            if (c.set.min_items)
                                counts.push_back("yyjson::min_properties(" + std::to_string(*c.set.min_items) + ")");
                            if (c.set.max_items)
                                counts.push_back("yyjson::max_properties(" + std::to_string(*c.set.max_items) + ")");
                            continue;
                        }
                        auto spec = std::vector<std::string>{cpp_string(member(c.key))};
                        if (c.required) spec.emplace_back("yyjson::required");
                        const auto* f = field_of(d.fields, c.key);
                        if (f != nullptr)
                            for (auto& x : render_settings(c.set, f->type, q)) spec.push_back(std::move(x));
                        if (c.is_type) spec.push_back("yyjson::is_type<" + spell(*c.is_type, false, q) + ">");
                        fs.push_back("yyjson::field(" + join(spec) + ")");
                    }
                    fs.insert(fs.end(), counts.begin(), counts.end());
                    if (closed) fs.emplace_back("yyjson::additional_properties(false)");
                    return "yyjson::" + std::string(part) + "(" + join(fs) + ")";
                };
                for (const auto& c : d.conditions)
                {
                    auto text = std::string();
                    if (c.negated)
                    {
                        const auto t = fields("if_", c.if_);
                        text = "yyjson::not_" + t.substr(t.find('('));
                    }
                    else
                    {
                        auto parts = std::vector<std::string>{fields("if_", c.if_)};
                        if (!c.then_.empty() || c.then_closed) parts.push_back(fields("then", c.then_, c.then_closed));
                        if (!c.else_.empty() || c.else_closed) parts.push_back(fields("else_", c.else_, c.else_closed));
                        text = "yyjson::if_then_else(\n        " + join(parts, ",\n        ") + ")";
                    }
                    if (std::ranges::find(items, text) == items.end()) items.push_back(std::move(text));
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
                end_defaults();
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
                // the types of yyjson::is_type<T> in `contains`
                const auto spell_tests = [&](const auto& self, const settings& s) -> void {
                    if (s.type_test >= 0) (void)spell(type_tests_[s.type_test]);
                    for (const auto* v :
                         {&s.items, &s.contains, &s.property_names, &s.additional, &s.prefix, &s.negated,
                                       &s.conditional})
                        for (const auto& e : *v) self(self, e);
                };
                for (const auto i : structs)
                {
                    for (const auto& f : decls_[i].fields)
                    {
                        ctx_ = decls_[i].name + "_" + f.member;
                        (void)spell(f.type);
                        spell_tests(spell_tests, f.set);
                    }
                    ctx_ = decls_[i].name;
                    spell_tests(spell_tests, decls_[i].set);
                }
                for (const auto i : structs)
                    for (const auto& c : decls_[i].conditions)
                        for (const auto* part : {&c.if_, &c.then_, &c.else_})
                            for (const auto& f : *part)
                            {
                                ctx_ = decls_[i].name + "_cond";
                                if (f.is_type) (void)spell(*f.is_type);
                                spell_tests(spell_tests, f.set);
                            }
                ctx_.clear();
                convert_defaults(structs);
                check_examples();
                check_levels();
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
                    body += moved_note(d.target, "");
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
                        body += moved_note(f.type, "    ");
                        const auto fi = field_items(f, d.as_array);
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
                        "cannot construct\n// a struct that holds itself or a field held through indirect.\n";
                if (any_field([](const field& f) { return holds_indirect(f.type); }))
                    out += std::string("//\n// A field that closes a cycle of structs is held through ") +
                           (opts_.cpp26 ? "std::indirect" : "yyjson::indirect") +
                           ", which copies the value it\n// holds. Its default constructor is explicit: a struct that "
                           "has such a field, directly or\n// in a member, is value-initialized as `T()`, not `T{}`.\n";
                if (std::ranges::any_of(decls_, [](const decl& d) { return d.transparent; }))
                    out +=
                        "//\n// A struct with `yyjson::transparent` is read and written as the value of its only "
                        "field, whose\n// constraints are those of the schema of the struct.\n";
                const auto any_extra = [&](bool as_array) {
                    return std::ranges::any_of(decls_, [&](const decl& d) {
                        return d.as_array == as_array && std::ranges::any_of(d.fields, [](const field& f) {
                                   return f.extra && !f.pattern;
                               });
                    });
                };
                const auto any_pattern_field = any_field([](const field& f) { return f.pattern.has_value(); });
                if (any_pattern_field)
                    out += "//\n// A field with `yyjson::pattern_properties` holds the keys of the object that "
                           "name no other field\n// and match its pattern, in the first such field. A value whose "
                           "key a pattern matches\n// satisfies the constraints of that field too, whichever field "
                           "holds it. Writing puts the\n// keys after the fields.\n";
                if (any_extra(false))
                    out += any_pattern_field
                               ? "//\n// A field with `yyjson::extra_keys` holds the keys of the object that name no "
                                 "other field and\n// that no pattern matches, and writing puts them after the "
                                 "fields.\n"
                               : "//\n// A field with `yyjson::extra_keys` holds the keys of the object that name no "
                                 "other field, and\n// writing puts them after the fields.\n";
                if (std::ranges::any_of(decls_, &decl::as_array))
                    out += "//\n// A struct with `yyjson::as_array` is read and written as a JSON array whose "
                           "elements are its\n// fields in order. A std::optional field or one with a default may be "
                           "missing at the end of\n// the array, and writing leaves out the run of such fields at "
                           "the end that hold no value or\n// their defaults.\n";
                if (any_extra(true))
                    out += "//\n// A field with `yyjson::extra_items` holds the elements after the other fields, "
                           "and writing puts\n// them at the end.\n";
                if (widened_variants_)
                    out += "//\n// A variant of more than " + std::to_string(opts_.max_alternatives) +
                           " alternatives (--max-alternatives) is yyjson::value, which reads any JSON\n// value: "
                           "the structs of its alternatives are still generated, and `cast<T>()` of the value\n// "
                           "reads one of them.\n";
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
                    out += any_pattern_field
                               ? "//\n// `yyjson::pattern` and `yyjson::pattern_properties` need a specialization of\n"
                                 "// `yyjson::pattern_matcher<>` in the program.\n"
                               : "//\n// `yyjson::pattern` needs a specialization of `yyjson::pattern_matcher<>` in "
                                 "the program.\n";
                if (uses_regex_format_)
                    out += uses_pattern_
                               ? "//\n// `yyjson::format(\"regex\")` takes a string as a regular expression when the "
                                 "constructor of\n// `yyjson::pattern_matcher<>` accepts it without throwing.\n"
                               : "//\n// `yyjson::format(\"regex\")` needs a specialization of "
                                 "`yyjson::pattern_matcher<>` in the\n// program, and takes a string as a regular "
                                 "expression when its constructor accepts it\n// without throwing.\n";
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
            explicit generator(const options& opts, bool dynamic = false) : opts_(opts), dynamic_(dynamic) {}

            // whether the schema has `$dynamicRef` / `$recursiveRef`, which needs a run in the dynamic mode
            [[nodiscard]] bool dynamic_found() const { return dynamic_found_; }

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
                resources_.emplace("", &root_);
                if (!root_path_.empty()) resources_.emplace(root_path_, &root_);
                if (const auto b = base_of(root_, ""); !b.empty())
                {
                    if (is_absolute_uri(b)) uri_files_.emplace_back(root_path_, b);
                    loc_keys_.emplace(b, "");
                }
                prepare(root_, "", &root_, is_draft07(root_), "#");
                for (const auto& path : opts_.schemas)
                {
                    const auto key = file_key("", path);
                    if (resources_.contains(key)) continue;
                    auto error = std::string();
                    if (!opts_.load)
                        warn(key + "#", "the schema is not read without a loader");
                    else if (load_file(key, "", error) == nullptr)
                        warn(key + "#", error.empty() ? "the schema cannot be read"
                                                      : "the schema is not valid JSON (" + error + ")");
                }
                if (dynamic_found_ && !dynamic_) return {};
                // a root of metadata and definitions only is a file of definitions and has no type; without
                // definitions, it accepts any JSON value
                const auto root_typed =
                    root_.is_bool() ||
                    (!is_pure_ref(root_) && (std::ranges::any_of(root_.members,
                                                                 [](const auto& m) {
                                                                     return is_type_key(m.first) ||
                                                                            is_ignored_key(m.first) ||
                                                                            is_condition_key(m.first);
                                                                 }) ||
                                             (!root_.has("$defs") && !root_.has("definitions"))));
                const auto* title = root_.is_object() ? root_.find("title") : nullptr;
                const auto root_name = !opts_.root_name.empty() ? opts_.root_name
                                       : title != nullptr && title->is_string() && !pascal(title->text).empty()
                                           ? global_safe(sanitize(pascal(title->text)))
                                           : std::string("Root");
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
                if (dynamic_) specialize(root_typed, defs);
                for (const auto& [loc, v] : defs) type_of(v, loc, def_names_.at(v));
                const auto* root = root_copy_ != nullptr ? root_copy_ : &root_;
                if (root_typed)
                    type_of(root, "#", root_name);
                else if (is_pure_ref(*root))
                {
                    // a root that only refers to another schema has the type of that schema, under the root name too
                    const auto* target = resolve(*root->find("$ref"), "#");
                    const auto it = def_names_.find(target);
                    if (!is_root(target) && (it == def_names_.end() || it->second != root_name))
                    {
                        const auto name = type_names_.contains(root_name) ? unique_type_name(root_name) : root_name;
                        type_names_.insert(name);
                        def_names_[root] = name;
                        type_named(root, "#");
                    }
                }
                if (decls_.empty()) warn("#", "the schema defines no type");
                if (dynamic_found_ && !dynamic_) return {};
                auto code = render();
                if (opts_.strict && !warnings_.empty()) throw error(warnings_);
                return {std::move(code), std::move(warnings_)};
            }
        };
    }  // namespace detail

    inline result generate(std::string_view schema, const options& opts)
    {
        // a schema with `$dynamicRef` / `$recursiveRef`, which the first run finds while it reads the files, is read
        // again in the dynamic mode
        auto g = detail::generator(opts);
        try
        {
            auto r = g.run(schema);
            if (!g.dynamic_found()) return r;
        }
        catch (const error&)
        {
            if (!g.dynamic_found()) throw;
        }
        return detail::generator(opts, true).run(schema);
    }
}  // namespace cpp_yyjson_codegen

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
