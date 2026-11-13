// JSON Schema (2020-12) of the C++ types that cpp-yyjson reads and writes.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cpp_yyjson.hpp"

namespace yyjson
{
    // The tag of `json_schema` that asks for the schema of the JSON that `write` gives, instead of the JSON that
    // `read` takes.
    struct write_direction_t
    {
        explicit write_direction_t() = default;
    };
    inline constexpr write_direction_t write_direction{};

    struct schema_options
    {
        // The value of `$id`, none if empty.
        std::string_view id = {};
        // The prefix of the references to the named types, whose schemas are in `$defs` of the result.
        std::string_view ref_prefix = "#/$defs/";
    };

    namespace schema_detail
    {
        using writer::object_ref;

        inline constexpr std::string_view dialect = "https://json-schema.org/draft/2020-12/schema";

        // The name in `$defs` of the type named `type_name`: the qualified name without "struct ", "class " or
        // "enum ", with "::" as "." and each character other than `[A-Za-z0-9._-]` as "_".
        inline std::string def_name(std::string_view type_name)
        {
            for (const auto prefix :
                 {std::string_view("struct "), std::string_view("class "), std::string_view("enum ")})
                if (type_name.starts_with(prefix)) type_name.remove_prefix(prefix.size());
            auto result = std::string();
            for (std::size_t i = 0; i < type_name.size(); ++i)
            {
                const auto c = type_name[i];
                if (c == ':' && i + 1 < type_name.size() && type_name[i + 1] == ':')
                {
                    result += '.';
                    ++i;
                }
                else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' ||
                         c == '_' || c == '-')
                    result += c;
                else
                    result += '_';
            }
            return result;
        }

        template <typename T>
        inline constexpr char type_identity_tag = 0;
        template <typename S, std::size_t K>
        inline constexpr char field_tag = 0;

        template <typename T>
        concept user_caster = (yyjson::detail::caster_names_from_json<T> || yyjson::detail::caster_names_to_json<T>) &&
                              !std::derived_from<caster<T>, yyjson::detail::builtin_caster>;

        template <typename T>
        concept variant_type = yyjson::detail::variant_json_kinds<T>::is_variant || std::is_union_v<T>;

        // The types read from a JSON string and written as one.
        template <typename T>
        concept string_type =
            std::same_as<T, const char*> || std::same_as<T, char*> ||
            (std::is_class_v<T> && yyjson::detail::json_string_constructible<T> &&
             (std::convertible_to<const T&, std::string_view> || std::same_as<T, std::filesystem::path>));

        template <typename T>
        concept struct_type =
            yyjson::detail::visitable<T> || (yyjson::detail::reflectable<T> && !yyjson::detail::std_array<T> &&
                                             (!std::ranges::range<T> || yyjson::detail::reflected_range_aggregate<T>));

        // The types whose schemas `$defs` holds.
        template <typename T>
        concept named_type = std::is_enum_v<T> || struct_type<T>;

        // The types whose schemas are a single `type` with keywords that the type `null` ignores.
        template <typename T>
        concept primitive_type = std::same_as<T, bool> || std::integral<T> || std::floating_point<T> || string_type<T>;

        // A set whose elements are equal exactly when their JSON values are: scalars that a standard comparison
        // orders, so that a JSON array of the set has unique items.
        template <typename T>
        concept json_unique_set = yyjson::detail::insertable_set<T> && yyjson::detail::unique_set<T> && [] {
            using K = typename T::key_type;
            if constexpr (!std::is_arithmetic_v<K> && !std::is_enum_v<K> && !std::same_as<K, std::string> &&
                          !std::same_as<K, std::string_view>)
                return false;
            // the writer rounds a wider float to a double, which can give two elements one text
            else if constexpr (std::floating_point<K> &&
                               std::numeric_limits<K>::digits > std::numeric_limits<double>::digits)
                return false;
            else if constexpr (requires { typename T::key_compare; })
                return std::same_as<typename T::key_compare, std::less<K>> ||
                       std::same_as<typename T::key_compare, std::less<>> ||
                       std::same_as<typename T::key_compare, std::greater<K>> ||
                       std::same_as<typename T::key_compare, std::greater<>>;
            else if constexpr (requires { typename T::key_equal; })
                return std::same_as<typename T::key_equal, std::equal_to<K>> ||
                       std::same_as<typename T::key_equal, std::equal_to<>>;
            else
                return false;
        }();

        class generator;

        // What the call of `json_schema` asks for besides the direction: its read policies, and whether the write
        // takes `yyjson::emit_defaults`.
        struct schema_mode
        {
            std::uint8_t policies = 0;
            bool emit_defaults = false;
        };

        // The named types that the schemas refer to.  A type has one schema per direction, or one for both
        // directions while the two are the same.
        struct registry
        {
            using body_function = void (*)(generator&, object_ref);
            struct entry
            {
                const void* id;
                std::string base;
                body_function body;
                std::array<bool, 2> used = {};  // read, write
                bool shared = true;
                std::array<std::string, 2> names = {};
            };
            std::vector<entry> types;
            std::map<const void*, std::size_t> index;
            // The schemas in the order in which they are found: the type and whether it is the write direction.
            std::vector<std::pair<std::size_t, bool>> order;
            std::string ref_prefix;
            // Until the names are settled, a reference names the entry, so that the schemas can be compared.
            bool settled = false;
            // `S()` of each struct, and the JSON text of the `default` of each field, made once per generation.
            std::map<const void*, std::shared_ptr<const void>> initial;
            std::map<const void*, std::optional<std::string>> defaults;

            template <typename T>
            std::size_t find_or_add(bool writing);

            [[nodiscard]] bool split(std::size_t i) const noexcept
            {
                return !types[i].shared && types[i].used[0] && types[i].used[1];
            }
            [[nodiscard]] std::string name(std::size_t i, bool writing) const
            {
                if (!settled) return "#" + std::to_string(i) + (split(i) ? (writing ? ":write" : ":read") : "");
                return types[i].names[split(i) && writing ? 1 : 0];
            }
            [[nodiscard]] std::string ref(std::size_t i, bool writing) const
            {
                return settled ? ref_prefix + name(i, writing) : name(i, writing);
            }

            // Gives each type its names: "Name", or "Name-Input" and "Name-Output" when it is split.  A name that
            // another type has already gets the first free suffix "_2", "_3", ... in the order of `order`.
            void assign_names()
            {
                auto taken = std::set<std::string, std::less<>>();
                auto done = std::vector<bool>(types.size());
                for (const auto& [i, writing] : order)
                {
                    if (done[i]) continue;
                    done[i] = true;
                    auto& t = types[i];
                    for (std::size_t n = 1;; ++n)
                    {
                        const auto base = n == 1 ? t.base : t.base + "_" + std::to_string(n);
                        if (split(i))
                            t.names = {base + "-Input", base + "-Output"};
                        else
                            t.names = {base, std::string()};
                        if (!taken.contains(t.names[0]) && (t.names[1].empty() || !taken.contains(t.names[1]))) break;
                    }
                    taken.insert(t.names[0]);
                    if (!t.names[1].empty()) taken.insert(t.names[1]);
                }
                settled = true;
            }
        };

        class generator
        {
        public:
            generator(registry& names, bool writing, schema_mode mode)
                : names_(names),
                  writing_(writing),
                  policies_(writing ? 0 : mode.policies),
                  emit_defaults_(writing && mode.emit_defaults)
            {
            }

            [[nodiscard]] bool writing() const noexcept { return writing_; }

            // Writes the schema of T into `out`.
            template <typename T>
            void emit(object_ref out);

            // `write` gives a C array of `char` at the root as a string, but one inside a value as integers.
            template <typename T>
            static void emit_root(generator& g, object_ref out)
            {
                if constexpr (yyjson::detail::char_string_array<T>)
                {
                    if (g.writing_)
                    {
                        out.emplace("type", "string");
                        out.emplace("maxLength", std::extent_v<T>);
                        return;
                    }
                }
                g.emit<T>(out);
            }
            // Writes the schema of the named type T, which `$defs` holds.
            template <typename T>
            static void emit_definition(generator& g, object_ref out)
            {
                if constexpr (std::is_enum_v<T>)
                    g.emit_enum<T>(out);
                else
                    g.emit_struct<T>(out);
            }

        private:
            registry& names_;
            bool writing_;
            std::uint8_t policies_;
            bool emit_defaults_;

            template <typename T>
            void emit_named(object_ref out)
            {
                const auto i = names_.find_or_add<T>(writing_);
                out.emplace("$ref", names_.ref(i, writing_), copy_string);
            }

            template <typename T>
            static consteval std::size_t fixed_size() noexcept
            {
                if constexpr (yyjson::detail::c_array<T>)
                    return std::extent_v<T>;
                else
                    return std::tuple_size_v<T>;
            }

            template <typename T>
            static constexpr std::string_view type_keyword() noexcept
            {
                if constexpr (std::same_as<T, bool>)
                    return "boolean";
                else if constexpr (std::integral<T>)
                    return "integer";
                else if constexpr (std::floating_point<T>)
                    return "number";
                else
                    return "string";
            }
            // Writes the keywords of the primitive T other than `type`.
            template <typename T>
            static void emit_primitive_keywords(object_ref out);
            template <typename T>
            void emit_nullable(object_ref out);
            template <typename E>
            void emit_enum(object_ref out);
            template <typename K>
            void emit_property_names(object_ref out);
            template <typename S>
            void emit_struct(object_ref out);
            template <typename S, std::size_t K>
            void emit_field(object_ref properties, std::vector<std::string_view>& required);
            template <typename T>
            void emit_range(object_ref out);
            template <typename T, typename E>
            void emit_array_form(object_ref out);
            template <typename T, typename K, typename V>
            void emit_object_form(object_ref out);
            template <typename T, std::size_t... Is>
            void emit_tuple(object_ref out, std::index_sequence<Is...>);
            template <typename T, std::size_t... Is>
            void emit_pair_object(object_ref out);
            template <typename... Vs>
            void emit_values(object_ref out, std::string_view combinator);
        };

        template <typename T>
        std::size_t registry::find_or_add(bool writing)
        {
            const auto [it, added] = index.try_emplace(&type_identity_tag<T>, types.size());
            const auto i = it->second;
            if (added)
                types.push_back({&type_identity_tag<T>, def_name(yyjson::detail::type_name<T>()),
                                 &generator::emit_definition<T>});
            if (!types[i].used[writing ? 1 : 0])
            {
                types[i].used[writing ? 1 : 0] = true;
                order.emplace_back(i, writing);
            }
            return i;
        }

        template <typename T>
        void generator::emit(object_ref out)
        {
            namespace yd = yyjson::detail;
            if constexpr (!std::same_as<T, std::remove_cv_t<T>>)
                emit<std::remove_cv_t<T>>(out);
            else if constexpr (std::same_as<T, json_view> || std::same_as<T, writer::value>)
            {
            }
            else if constexpr (std::same_as<T, writer::object>)
                out.emplace("type", "object");
            else if constexpr (std::same_as<T, writer::array>)
                out.emplace("type", "array");
            else if constexpr (std::same_as<T, std::nullptr_t> || std::same_as<T, std::monostate>)
                out.emplace("type", "null");
            else if constexpr (yd::is_with_caster<T>::value)
                static_assert(yd::false_v<T>, "C7g: yyjson::with_caster is not emitted yet");
            else if constexpr (user_caster<T>)
                static_assert(yd::false_v<T>, "C7g: a type with a caster of the user is not emitted yet");
            else if constexpr (variant_type<T>)
                static_assert(yd::false_v<T>, "C7f: variants and unions are not emitted yet");
            else if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T>)
                emit_nullable<typename T::value_type>(out);
            else if constexpr (yd::owning_pointer<T>)
                emit_nullable<std::remove_cv_t<typename yd::is_owning_pointer<T>::value_type>>(out);
            else if constexpr (yd::indirect_value<T>)
                emit<typename yd::is_indirect<T>::value_type>(out);
            else if constexpr (yd::transparent_struct<T>)
                static_assert(yd::false_v<T>, "C7e: yyjson::transparent is not emitted yet");
            else if constexpr (yd::as_array_struct<T>)
                static_assert(yd::false_v<T>, "C7e: yyjson::as_array is not emitted yet");
            else if constexpr (named_type<T>)
                emit_named<T>(out);
            else if constexpr (primitive_type<T>)
            {
                out.emplace("type", type_keyword<T>());
                emit_primitive_keywords<T>(out);
            }
            else if constexpr (std::same_as<T, std::vector<bool>>)
            {
                out.emplace("type", "array");
                out.emplace("items", empty_object).emplace("type", "boolean");
            }
            else if constexpr (std::ranges::range<T>)
                emit_range<T>(out);
            else if constexpr (yd::tuple_like<T>)
                emit_tuple<T>(out, std::make_index_sequence<std::tuple_size_v<T>>{});
            else
                static_assert(yd::false_v<T>,
                              "C7g: a type that is converted by its constructors or its conversions is not emitted "
                              "yet");
        }

        // The written text of a float is the shortest one that reads back as it, which can be past its double:
        // 3.4028235e38 for the largest float.  The bounds are those texts, or those of a double for a wider type,
        // because the reader takes no number past a double.
        template <typename T>
        void generator::emit_primitive_keywords(object_ref out)
        {
            using L = std::numeric_limits<T>;
            if constexpr (std::same_as<T, bool> || string_type<T>)
            {
            }
            else if constexpr (std::integral<T> && std::is_signed_v<T>)
            {
                out.emplace("minimum", static_cast<std::int64_t>(L::lowest()));
                out.emplace("maximum", static_cast<std::int64_t>(L::max()));
            }
            else if constexpr (std::integral<T>)
            {
                out.emplace("minimum", 0);
                out.emplace("maximum", static_cast<std::uint64_t>(L::max()));
            }
            else
            {
                using F = std::conditional_t<yyjson::detail::narrower_than_double<T>, T, double>;
                static const auto bounds = [] {
                    const auto written = [](F f) { return yyjson::read(yyjson::write(f)).template cast<double>(); };
                    return std::array{written(std::numeric_limits<F>::lowest()), written(std::numeric_limits<F>::max())};
                }();
                out.emplace("minimum", bounds[0]);
                out.emplace("maximum", bounds[1]);
            }
        }

        template <typename U>
        void generator::emit_nullable(object_ref out)
        {
            if constexpr (primitive_type<U>)
            {
                auto types = out.emplace("type", empty_array);
                types.emplace_back(type_keyword<U>());
                types.emplace_back("null");
                emit_primitive_keywords<U>(out);
            }
            else
            {
                auto any = out.emplace("anyOf", empty_array);
                emit<U>(any.emplace_back(empty_object));
                any.emplace_back(empty_object).emplace("type", "null");
            }
        }

        // An enumeration of `enum_format::name_or_integer` writes an enumerator without a name as its integer, so that
        // both directions take the names and the integers.
        template <typename E>
        void generator::emit_enum(object_ref out)
        {
            using conv = yyjson::detail::enum_conversion<E>;
            using U = std::underlying_type_t<E>;
            const auto names = [](object_ref o) {
                o.emplace("type", "string");
                auto values = o.emplace("enum", empty_array);
                for (const auto name : conv::names) values.emplace_back(name);
            };
            const auto integer = [](object_ref o) {
                o.emplace("type", "integer");
                if constexpr (std::same_as<U, bool>)
                {
                    o.emplace("minimum", 0);
                    o.emplace("maximum", 1);
                }
                else
                    emit_primitive_keywords<U>(o);
            };
            if constexpr (conv::format == enum_format::name)
                names(out);
            else if constexpr (conv::format == enum_format::integer)
                integer(out);
            else
            {
                auto any = out.emplace("anyOf", empty_array);
                names(any.emplace_back(empty_object));
                integer(any.emplace_back(empty_object));
            }
        }

        // The decimal texts of the integers in [1, n], without a leading zero, as alternatives of a regular expression.
        inline std::string positive_range_pattern(std::uint64_t n)
        {
            const auto s = std::to_string(n);
            const auto digit_class = [](char lo, char hi) {
                return lo == hi ? std::string(1, lo) : std::string{'[', lo, '-', hi, ']'};
            };
            const auto any_digits = [](std::size_t count) {
                return count == 0   ? std::string()
                       : count == 1 ? std::string("[0-9]")
                                    : "[0-9]{" + std::to_string(count) + "}";
            };
            std::string out;
            const auto add = [&](const std::string& alternative) {
                if (!out.empty()) out += '|';
                out += alternative;
            };
            if (n == 0) return out;
            if (s.size() == 2) add("[1-9]");
            if (s.size() > 2) add("[1-9][0-9]{0," + std::to_string(s.size() - 2) + "}");
            for (std::size_t i = 0; i < s.size(); ++i)
            {
                const char lo = i == 0 ? '1' : '0';
                if (s[i] > lo)
                    add(s.substr(0, i) + digit_class(lo, static_cast<char>(s[i] - 1)) + any_digits(s.size() - i - 1));
            }
            add(s);
            return out;
        }

        // The keys of a map of the enumeration K: the names, or the integers of the underlying type in decimal without
        // a leading zero, which the read takes and the write gives.
        template <typename K>
        void generator::emit_property_names(object_ref out)
        {
            using conv = yyjson::detail::enum_conversion<K>;
            using U = std::underlying_type_t<K>;
            const auto names = [](object_ref o) {
                auto values = o.emplace("enum", empty_array);
                for (const auto name : conv::names) values.emplace_back(name);
            };
            const auto integer = [](object_ref o) {
                if constexpr (std::same_as<U, bool>)
                    o.emplace("pattern", "^[01]$");
                else
                {
                    auto pattern = "^(0|" + positive_range_pattern(std::numeric_limits<U>::max());
                    if constexpr (std::is_signed_v<U>)
                    {
                        const auto magnitude = static_cast<std::uint64_t>(-(std::numeric_limits<U>::min() + 1)) + 1;
                        pattern += "|-(" + positive_range_pattern(magnitude) + ")";
                    }
                    pattern += ")$";
                    o.emplace("pattern", pattern, copy_string);
                }
            };
            auto keys = out.emplace("propertyNames", empty_object);
            if constexpr (conv::format == enum_format::name)
                names(keys);
            else if constexpr (conv::format == enum_format::integer)
                integer(keys);
            else
            {
                auto any = keys.emplace("anyOf", empty_array);
                names(any.emplace_back(empty_object));
                integer(any.emplace_back(empty_object));
            }
        }

        template <typename S>
        void generator::emit_struct(object_ref out)
        {
            namespace yd = yyjson::detail;
            static_assert(!yd::has_constraints<S>, "C7c: the keywords of the fields are not emitted yet");
            static_assert(!yd::has_struct_conditions<S>, "C7c: the conditions of a struct are not emitted yet");
            static_assert(!yd::has_key_maps<S>,
                          "C7d: yyjson::extra_keys and yyjson::pattern_properties are not emitted yet");
            static_assert(!yd::struct_property_counts<S> && !yd::struct_property_names<S>,
                          "C7d: the keywords of the keys of a struct are not emitted yet");
            static_assert(!yd::has_adjacent_fields<S>, "C7f: yyjson::adjacent_tag is not emitted yet");
            out.emplace("type", "object");
            auto properties = out.emplace("properties", empty_object);
            auto required = std::vector<std::string_view>();
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (emit_field<S, Ks>(properties, required), ...);
            }(std::make_index_sequence<yd::struct_field_count<S>>{});
            if (!required.empty())
            {
                auto keys = out.emplace("required", empty_array);
                for (const auto key : required) keys.emplace_back(key);
            }
            // the read rejects an unknown key as `struct_key_list::closed` decides it, and a read policy has no effect
            // on the write
            if (yd::struct_closed<S> ||
                (!writing_ && ((policies_ | yd::struct_policies<S>)&yd::unknown_keys_policy) != 0))
                out.emplace("additionalProperties", false);
        }

        // Whether the read schema requires the key of the field K of S: the field declares no default.  The policies
        // do not change it, so that every read schema of S tells one contract.
        template <typename S, std::size_t K>
        [[nodiscard]] constexpr bool read_requires() noexcept
        {
            return !yyjson::detail::struct_field_absence<S>[K];
        }
        // Whether every write of S gives the key of its field K: the field is not omitted when empty, nor when equal
        // to its default unless the write takes `yyjson::emit_defaults`.
        template <typename S, std::size_t K>
        [[nodiscard]] constexpr bool write_requires(bool emit_defaults) noexcept
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            return (emit_defaults || !yd::omits_default<S, yd::struct_declared_index<S, K>>) &&
                   !yd::omittable<decltype(yd::writable_field(std::declval<const F&>()))>;
        }

        // A C array is read as the `std::array` of as many elements.
        template <typename F>
        struct read_back
        {
            using type = F;
        };
        template <typename E, std::size_t N>
        struct read_back<E[N]>
        {
            using type = std::array<typename read_back<E>::type, N>;
        };

        // Whether the read schema takes the written value `v` where it is narrower than the read: a tuple of pairs
        // whose values differ in type.
        template <typename T>
        bool read_schema_takes(const T& v)
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T> || yd::owning_pointer<T>)
                return !v || read_schema_takes(*v);
            else if constexpr (yd::indirect_value<T>)
                return v.valueless_after_move() || read_schema_takes(*v);
            else if constexpr (struct_type<T>)
                return [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return (read_schema_takes(yd::struct_field<Ks>(v)) && ...);
                }(std::make_index_sequence<yd::struct_field_count<T>>{});
            else if constexpr (primitive_type<T> || std::is_enum_v<T> || std::same_as<T, std::vector<bool>>)
                return true;
            else if constexpr (std::ranges::range<T>)
            {
                for (const auto& e : v)
                    if (!read_schema_takes(e)) return false;
                return true;
            }
            else if constexpr (yd::tuple_like<T>)
                return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    if constexpr (writer::detail::to_json_obj_defined<const T&>)
                    {
                        using first = std::remove_cv_t<std::tuple_element_t<1, std::tuple_element_t<0, T>>>;
                        return (std::same_as<std::remove_cv_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>,
                                             first> &&
                                ...) &&
                               (read_schema_takes(std::get<1>(std::get<Is>(v))) && ...);
                    }
                    else
                        return (read_schema_takes(std::get<Is>(v)) && ...);
                }(std::make_index_sequence<std::tuple_size_v<T>>{});
            else
                return true;
        }

        // The JSON text that `write` with `yyjson::emit_defaults` gives for the default `d` of a field of F, which
        // every schema of F takes, if the field is written and the read of F under every policy takes it back.
        template <typename F, typename D>
        std::optional<std::string> default_text(const D& d)
        {
            namespace yd = yyjson::detail;
            decltype(auto) field = yd::writable_field(d);
            if constexpr (yd::omittable<decltype(field)>)
                if (yd::should_omit_field(field)) return std::nullopt;
            using R = typename read_back<std::remove_cv_t<F>>::type;
            if constexpr (!readable<R>)
                return std::nullopt;
            else
            {
                try
                {
                    auto text = std::string(yyjson::write(field, yyjson::emit_defaults));
                    (void)yyjson::read<R>(text, yyjson::strict, yyjson::reject_unknown_keys);
                    if (!read_schema_takes(field)) return std::nullopt;
                    return text;
                }
                catch (const std::exception&)
                {
                    return std::nullopt;
                }
            }
        }

        template <typename S>
        const S* initial_value(registry& names)
        {
            if constexpr (std::is_default_constructible_v<S>)
            {
                auto& value = names.initial[&type_identity_tag<S>];
                if (!value) value = std::make_shared<const S>();
                return static_cast<const S*>(value.get());
            }
            else
                return nullptr;
        }

        // The `default` of the field K of S: the value of its `yyjson::default_from<f>`, or else the field of `S()`
        // if it satisfies the keywords of the field as `validate` checks them.  A required field has none.
        template <typename S, std::size_t K>
        const std::optional<std::string>& field_default(registry& names)
        {
            namespace yd = yyjson::detail;
            constexpr auto r = yd::struct_declared_index<S, K>;
            using F = yd::struct_field_type<S, K>;
            const auto [it, added] = names.defaults.try_emplace(&field_tag<S, K>);
            if (!added) return it->second;
            if constexpr (yd::presence_of<S, r>.required)
                return it->second;
            else if constexpr (yd::presence_of<S, r>.from.has_value())
                it->second = default_text<F>(yd::default_field_value<S, r>());
            else if (const auto* s = initial_value<S>(names); s != nullptr)
            {
                it->second = default_text<F>(yd::struct_field<K>(*s));
                if (it->second)
                {
                    try
                    {
                        const auto scope = yd::emit_defaults_scope();
                        yd::validate_object_field<false, S, K>(*s, WriteFlag::NoFlag);
                    }
                    catch (const std::exception&)
                    {
                        it->second.reset();
                    }
                }
            }
            return it->second;
        }

        // The read schema requires the keys that the read requires, and the write schema the keys that every write
        // gives.  The `default` is the same in both.
        template <typename S, std::size_t K>
        void generator::emit_field(object_ref properties, std::vector<std::string_view>& required)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            constexpr auto key = yd::struct_field_key<S, K>;
            auto p = properties.emplace(key, empty_object);
            if constexpr (yd::is_std_optional<F>::value)
                emit<typename F::value_type>(p);
            else
                emit<F>(p);
            if (const auto& d = field_default<S, K>(names_)) p.emplace("default", yyjson::read(*d));
            if (writing_ ? write_requires<S, K>(emit_defaults_) : read_requires<S, K>()) required.push_back(key);
        }

        // A range is written as an object when its elements are key-value pairs, and read from the forms that
        // `nested_casts` gives: an array of its elements, an object of its values, or either.
        template <typename T>
        void generator::emit_range(object_ref out)
        {
            namespace yd = yyjson::detail;
            using pair = std::remove_cv_t<std::ranges::range_value_t<const T>>;
            if (writing_)
            {
                if constexpr (yd::object_written_range<T>)
                    emit_object_form<T, std::remove_cv_t<std::tuple_element_t<0, pair>>,
                                     std::remove_cv_t<std::tuple_element_t<1, pair>>>(out);
                else
                    emit_array_form<T, pair>(out);
                return;
            }
            // `nested_casts` sees no element of a C array of arrays, which is read row by row
            if constexpr (std::rank_v<T> >= 2)
            {
                emit_array_form<T, std::remove_extent_t<T>>(out);
                return;
            }
            using n = yd::nested_casts<T>;
            const auto object_form = [&](object_ref o) {
                if constexpr (n::values)
                    emit_object_form<T, std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>,
                                     typename n::value_type>(o);
            };
            if constexpr (n::elements && n::values)
            {
                auto any = out.emplace("anyOf", empty_array);
                emit_array_form<T, typename n::element_type>(any.emplace_back(empty_object));
                object_form(any.emplace_back(empty_object));
            }
            else if constexpr (n::elements)
                emit_array_form<T, typename n::element_type>(out);
            else if constexpr (n::values)
                object_form(out);
            else
                out.emplace("not", empty_object);
        }

        template <typename T, typename E>
        void generator::emit_array_form(object_ref out)
        {
            namespace yd = yyjson::detail;
            out.emplace("type", "array");
            emit<std::remove_cv_t<E>>(out.emplace("items", empty_object));
            if constexpr (yd::c_array<T> || yd::std_array<T>)
            {
                out.emplace("minItems", fixed_size<T>());
                out.emplace("maxItems", fixed_size<T>());
            }
            if constexpr (json_unique_set<T>) out.emplace("uniqueItems", true);
        }

        // A range of a fixed size is written as an object of a member for each of its pairs, and read from an
        // object of as many members.
        template <typename T, typename K, typename V>
        void generator::emit_object_form(object_ref out)
        {
            namespace yd = yyjson::detail;
            out.emplace("type", "object");
            if constexpr (yd::c_array<T> || yd::std_array<T>)
            {
                out.emplace("minProperties", fixed_size<T>());
                out.emplace("maxProperties", fixed_size<T>());
            }
            emit<V>(out.emplace("additionalProperties", empty_object));
            if constexpr (yd::enum_key<K>) emit_property_names<K>(out);
        }

        // Whether the I-th of Vs is the first of its type.
        template <std::size_t I, typename... Vs>
        consteval bool first_of_type()
        {
            using V = std::tuple_element_t<I, std::tuple<Vs...>>;
            return []<std::size_t... Js>(std::index_sequence<Js...>) {
                return ((Js >= I || !std::same_as<std::tuple_element_t<Js, std::tuple<Vs...>>, V>) && ...);
            }(std::index_sequence_for<Vs...>{});
        }

        // The schema of a value that is one of Vs: of each of them under `combinator`, or of V when all are V.
        template <typename... Vs>
        void generator::emit_values(object_ref out, std::string_view combinator)
        {
            using first = std::tuple_element_t<0, std::tuple<Vs...>>;
            if constexpr ((std::same_as<Vs, first> && ...))
                emit<first>(out);
            else
            {
                auto each = out.emplace(combinator, empty_array);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (
                        [&] {
                            if constexpr (first_of_type<Is, Vs...>())
                                emit<std::tuple_element_t<Is, std::tuple<Vs...>>>(each.emplace_back(empty_object));
                        }(),
                        ...);
                }(std::index_sequence_for<Vs...>{});
            }
        }

        template <typename T, std::size_t... Is>
        void generator::emit_pair_object(object_ref out)
        {
            out.emplace("type", "object");
            if (!writing_)
            {
                out.emplace("minProperties", sizeof...(Is));
                out.emplace("maxProperties", sizeof...(Is));
            }
            emit_values<std::remove_cv_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>...>(
                out.emplace("additionalProperties", empty_object), writing_ ? "anyOf" : "allOf");
        }

        // A tuple of key-value pairs with string keys is written as an object of a member for each pair, and read
        // from such an object or from an array of the pairs.  JSON Schema cannot tie a member to its position, so a
        // member that is read must satisfy the schemas of the values of all the pairs.
        template <typename T, std::size_t... Is>
        void generator::emit_tuple(object_ref out, std::index_sequence<Is...>)
        {
            constexpr auto size = sizeof...(Is);
            if constexpr (size == 0)
            {
                // written as an empty object, and read from an empty array as well
                if (writing_)
                    out.emplace("type", "object");
                else
                {
                    auto types = out.emplace("type", empty_array);
                    types.emplace_back("array");
                    types.emplace_back("object");
                    out.emplace("maxItems", 0);
                }
                out.emplace("maxProperties", 0);
            }
            else
            {
                const auto array_form = [&](object_ref o) {
                    o.emplace("type", "array");
                    auto prefix = o.emplace("prefixItems", empty_array);
                    (emit<std::tuple_element_t<Is, T>>(prefix.emplace_back(empty_object)), ...);
                    o.emplace("minItems", size);
                    o.emplace("items", false);
                };
                constexpr bool object_read = reader::detail::tuple_from_object<T>;
                constexpr bool array_read = reader::detail::tuple_from_array<T>;
                if (writing_)
                {
                    if constexpr (writer::detail::to_json_obj_defined<const T&>)
                        emit_pair_object<T, Is...>(out);
                    else
                        array_form(out);
                }
                else if constexpr (object_read && array_read)
                {
                    auto any = out.emplace("anyOf", empty_array);
                    emit_pair_object<T, Is...>(any.emplace_back(empty_object));
                    array_form(any.emplace_back(empty_object));
                }
                else if constexpr (object_read)
                    emit_pair_object<T, Is...>(out);
                else if constexpr (array_read)
                    array_form(out);
                else
                    out.emplace("not", empty_object);
            }
        }

        // The schemas of the roots and of the named types that they refer to.  The names are settled when all the
        // roots are known, because a type whose read and write schemas differ has two names.
        class builder
        {
        public:
            explicit builder(std::string_view ref_prefix, schema_mode mode = {}) : ref_prefix_(ref_prefix), mode_(mode)
            {
            }

            template <typename T>
            void add(bool writing)
            {
                for (const auto& r : roots_)
                    if (r.id == &type_identity_tag<T> && r.writing == writing) return;
                roots_.push_back({&type_identity_tag<T>, writing, &generator::emit_root<T>});
                names_.reset();
            }
            template <typename T>
            void emit_root(object_ref out, bool writing)
            {
                settle();
                auto g = generator(*names_, writing, mode_);
                generator::emit_root<T>(g, out);
            }
            // Writes the schemas of the named types into `defs`.
            void emit_definitions(object_ref defs)
            {
                settle();
                const auto order = names_->order;
                for (const auto& [i, writing] : order)
                {
                    if (!names_->split(i) && names_->types[i].used[0] && writing) continue;
                    auto g = generator(*names_, writing, mode_);
                    names_->types[i].body(g, defs.emplace(names_->name(i, writing), empty_object, copy_string));
                }
            }

        private:
            struct root
            {
                const void* id;
                bool writing;
                registry::body_function emit;
            };
            std::string ref_prefix_;
            schema_mode mode_;
            std::vector<root> roots_;
            std::optional<registry> names_;

            [[nodiscard]] std::string definition_text(std::size_t i, bool writing)
            {
                auto doc = value(object());
                auto g = generator(*names_, writing, mode_);
                names_->types[i].body(g, *doc.as_object());
                return std::string(doc.write());
            }

            void settle()
            {
                if (names_) return;
                try
                {
                    find_names();
                }
                catch (...)
                {
                    names_.reset();
                    throw;
                }
            }
            void find_names()
            {
                names_.emplace();
                names_->ref_prefix = ref_prefix_;
                for (const auto& r : roots_)
                {
                    auto doc = value(object());
                    auto g = generator(*names_, r.writing, mode_);
                    r.emit(g, *doc.as_object());
                }
                // The schema of each named type adds the types that it refers to.
                for (std::size_t k = 0; k < names_->order.size(); ++k)
                {
                    const auto [i, writing] = names_->order[k];
                    auto doc = value(object());
                    auto g = generator(*names_, writing, mode_);
                    names_->types[i].body(g, *doc.as_object());
                }
                // A type has one name while its two schemas are the same, where the types that they refer to are the
                // same if they share a name.  Splitting a type changes the schemas that refer to it, so this repeats.
                for (auto changed = true; changed;)
                {
                    changed = false;
                    for (std::size_t i = 0; i < names_->types.size(); ++i)
                        if (names_->types[i].shared && names_->types[i].used[0] && names_->types[i].used[1] &&
                            definition_text(i, false) != definition_text(i, true))
                        {
                            names_->types[i].shared = false;
                            changed = true;
                        }
                }
                names_->assign_names();
            }
        };

        template <typename A>
        inline constexpr bool schema_argument =
            std::same_as<A, write_direction_t> || std::same_as<A, emit_defaults_t> || std::same_as<A, schema_options> ||
            yyjson::detail::read_policy<A>;
    }  // namespace schema_detail

    // The JSON Schema (2020-12) of the JSON that `read<T>` takes, or with `yyjson::write_direction` of the JSON that
    // `write` gives.  The other arguments are the tags of the read policies or `yyjson::emit_defaults`, and one
    // `schema_options`.
    template <typename T, typename... Args>
    [[nodiscard]] object json_schema(const Args&... args)
    {
        namespace sd = schema_detail;
        constexpr bool writing = (std::same_as<Args, write_direction_t> || ...);
        constexpr bool policies = (yyjson::detail::read_policy<Args> || ...);
        constexpr bool defaults = (std::same_as<Args, emit_defaults_t> || ...);
        static_assert((sd::schema_argument<Args> && ...),
                      "json_schema takes yyjson::write_direction, yyjson::emit_defaults, the read policies such as "
                      "yyjson::strict, and yyjson::schema_options");
        static_assert(yyjson::detail::count_true<std::same_as<Args, write_direction_t>...> <= 1 &&
                          yyjson::detail::count_true<std::same_as<Args, emit_defaults_t>...> <= 1 &&
                          yyjson::detail::count_true<std::same_as<Args, schema_options>...> <= 1,
                      "json_schema takes each of its tags and yyjson::schema_options once");
        static_assert(!(writing && policies),
                      "json_schema: a read policy, such as yyjson::strict, applies to the read schema, not to the "
                      "schema of yyjson::write_direction");
        static_assert(writing || !defaults,
                      "json_schema: yyjson::emit_defaults applies to the write schema; give yyjson::write_direction");
        if constexpr (writing)
            static_assert(writable<T>, "json_schema<T>(yyjson::write_direction): yyjson::write does not take T");
        else
            static_assert(yyjson::detail::read_castable<T>, "json_schema<T>: yyjson::cast does not give T");
        auto options = schema_options();
        const auto take = [&]<typename A>(const A& a) {
            if constexpr (std::same_as<A, schema_options>) options = a;
        };
        (take(args), ...);
        auto b = sd::builder(options.ref_prefix, {yyjson::detail::policies_of<Args...>, defaults});
        b.add<T>(writing);
        auto doc = value(object());
        auto root = *doc.as_object();
        root.emplace("$schema", sd::dialect);
        if (!options.id.empty()) root.emplace("$id", options.id, copy_string);
        auto defs = root.emplace("$defs", empty_object);
        b.emit_definitions(defs);
        if (defs.empty()) root.erase("$defs");
        b.emit_root<T>(root, writing);
        return *std::move(doc).as_object();
    }
}  // namespace yyjson
