// JSON Schema (2020-12) of the C++ types that cpp-yyjson reads and writes.

#pragma once

#include <array>
#include <cmath>
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
        concept variant_type = yyjson::detail::variant_json_kinds<T>::is_variant;

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

        // A new subschema in `allOf` of `out`, for a keyword that `out` already has with another value.
        inline object_ref conjunct(object_ref out)
        {
            if (auto it = out.find("allOf"); it != out.end())
            {
                auto all = it->second;
                return all.as_array()->emplace_back(empty_object);
            }
            return out.emplace("allOf", empty_array).emplace_back(empty_object);
        }
        // `out`, or a conjunct of it if it has `key`.
        inline object_ref place(object_ref out, std::string_view key)
        {
            return out.contains(key) ? conjunct(out) : out;
        }
        // The object at `key` of `out`, which a conjunct holds if `key` has another value, such as `false`.
        inline object_ref sub(object_ref out, std::string_view key)
        {
            if (auto it = out.find(key); it != out.end())
            {
                auto v = it->second;
                if (auto o = v.as_object(); o.has_value()) return *o;
                return conjunct(out).emplace(key, empty_object, copy_string);
            }
            return out.emplace(key, empty_object, copy_string);
        }
        // The object I of the array at `key` of `out`, which has one.
        inline object_ref sub_at(object_ref out, std::string_view key, std::size_t i)
        {
            auto array = out.find(key)->second;
            auto element = (*array.as_array())[i];
            return *element.as_object();
        }

        inline void emplace_bound(object_ref out, std::string_view key, const yyjson::detail::bound_number& b)
        {
            using kind = yyjson::detail::bound_number::kind_t;
            if (b.kind == kind::sint)
                out.emplace(key, b.sint);
            else if (b.kind == kind::uint)
                out.emplace(key, b.uint);
            else
                out.emplace(key, b.real);
        }
        // Adds the bound `b` at `key` of `out`, or keeps the tighter of it and the one there, which both hold: the
        // greater for a lower bound.
        inline void tighten(object_ref out, std::string_view key, const yyjson::detail::bound_number& b, bool lower)
        {
            namespace yd = yyjson::detail;
            const auto it = out.find(key);
            if (it == out.end())
            {
                emplace_bound(out, key, b);
                return;
            }
            auto v = it->second;
            auto held = yd::bound_number();
            if (v.is_sint())
                held.sint = *v.as_sint();
            else if (v.is_uint())
            {
                held.kind = yd::bound_number::kind_t::uint;
                held.uint = *v.as_uint();
            }
            else
            {
                held.kind = yd::bound_number::kind_t::real;
                held.real = *v.as_real();
            }
            const auto order = yd::compare_bounds(b, held);
            if (!(lower ? std::is_gt(order) : std::is_lt(order))) return;
            if (b.kind == yd::bound_number::kind_t::sint)
                v = b.sint;
            else if (b.kind == yd::bound_number::kind_t::uint)
                v = b.uint;
            else
                v = b.real;
        }
        inline void tighten(object_ref out, std::string_view key, std::size_t n, bool lower)
        {
            auto b = yyjson::detail::bound_number();
            b.kind = yyjson::detail::bound_number::kind_t::uint;
            b.uint = n;
            tighten(out, key, b, lower);
        }
        inline yyjson::detail::bound_number real_bound(double d)
        {
            auto b = yyjson::detail::bound_number();
            b.kind = yyjson::detail::bound_number::kind_t::real;
            b.real = d;
            return b;
        }

        // The types T of `yyjson::matches<T>` in the settings A.
        template <typename A>
        struct matched_types
        {
            using type = std::tuple<>;
        };
        template <typename T>
        struct matched_types<matches_t<T>>
        {
            using type = std::tuple<T>;
        };
        template <template <typename...> class X, typename... As>
        struct matched_types<X<As...>>
        {
            using type = decltype(std::tuple_cat(std::declval<typename matched_types<As>::type>()...));
        };
        template <template <std::size_t, typename...> class X, std::size_t N, typename... As>
        struct matched_types<X<N, As...>>
        {
            using type = decltype(std::tuple_cat(std::declval<typename matched_types<As>::type>()...));
        };
        // The settings that hold the `yyjson::matches` of a node of S: its annotations, or the settings of the
        // alternative S of a union.
        template <typename S>
        struct settings_of
        {
            using type = yyjson::detail::annotation_values_of<S>;
        };
        template <typename T, auto... Settings>
        struct settings_of<yyjson::alt<T, Settings...>>
        {
            using type = std::tuple<yyjson::detail::alternative_setting_t<Settings>...>;
        };
        // The index in the tuple Ts of the type whose name is `name`, or the size of Ts.
        template <typename Ts>
        struct matched_index;
        template <typename... Ts>
        struct matched_index<std::tuple<Ts...>>
        {
            static consteval std::size_t of(std::string_view name)
            {
                constexpr auto names = std::array<std::string_view, sizeof...(Ts)>{yyjson::detail::type_name<Ts>()...};
                for (std::size_t i = 0; i < names.size(); ++i)
                    if (names[i] == name) return i;
                return sizeof...(Ts);
            }
        };

        // Whether the node holds nothing, as an omitted `yyjson::then` or `yyjson::else_` does.
        constexpr bool empty_node(const yyjson::detail::schema_node& n) noexcept
        {
            return n.constraints.bits == 0 && n.values_count == 0 && n.patterns_count == 0 && n.items == 0 &&
                   n.properties == 0 && n.keys == 0 && n.contains == 0 && n.prefix_count == 0 && n.combinators == 0 &&
                   n.members == 0 && !n.closed;
        }

        // The I-th element type of the tuple E, or D if it has none.
        template <typename E, std::size_t I, typename D>
        struct element_or
        {
            using type = D;
        };
        template <typename E, std::size_t I, typename D>
        requires yyjson::detail::tuple_like<E> && (std::tuple_size_v<E> > I)
        struct element_or<E, I, D>
        {
            using type = std::remove_cv_t<std::tuple_element_t<I, E>>;
        };

        // The C++ types of the subschemas of a value of V: of its elements, its J-th element, the keys and the values
        // of its members.  A value of the DOM has those of the DOM.
        template <typename V>
        struct position_types
        {
            using element = writer::value;
            template <std::size_t J>
            using prefix = writer::value;
            using key = std::string;
            using value = writer::value;
        };
        template <typename V>
        requires std::ranges::range<V> && (!std::same_as<V, writer::value>) && (!std::same_as<V, writer::array>) &&
                 (!std::same_as<V, writer::object>) && (!std::same_as<V, json_view>)
        struct position_types<V>
        {
            using element = std::remove_cv_t<std::ranges::range_value_t<const V>>;
            template <std::size_t J>
            using prefix = element;
            using key = typename element_or<element, 0, std::string>::type;
            using value = typename element_or<element, 1, writer::value>::type;
        };
        template <typename V>
        requires yyjson::detail::tuple_like<V> && (!std::ranges::range<V>)
        struct position_types<V>
        {
            using element = typename element_or<V, 0, writer::value>::type;
            template <std::size_t J>
            using prefix = typename element_or<V, J, writer::value>::type;
            using key = std::string;
            using value = writer::value;
        };
        // The elements of a struct annotated with `yyjson::as_array` have one C++ type only when all of its positions
        // and its `yyjson::extra_items` share it.
        template <typename S, bool = yyjson::detail::has_extra_items<S>>
        struct array_struct_first
        {
            using type = std::remove_cv_t<std::ranges::range_value_t<yyjson::detail::extra_items_type<S>>>;
        };
        template <typename S>
        struct array_struct_first<S, false>
        {
            using type = std::remove_cv_t<yyjson::detail::struct_field_type<S, 0>>;
        };
        template <typename S>
        consteval auto array_struct_element()
        {
            namespace yd = yyjson::detail;
            constexpr auto n = yd::array_position_count<S>;
            if constexpr (n == 0 && !yd::has_extra_items<S>)
                return std::type_identity<writer::value>{};
            else
            {
                using T = typename array_struct_first<S>::type;
                constexpr bool same = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return (std::same_as<std::remove_cv_t<yd::struct_field_type<S, Ks>>, T> && ...);
                }(std::make_index_sequence<n>{});
                if constexpr (same)
                    return std::type_identity<T>{};
                else
                    return std::type_identity<writer::value>{};
            }
        }
        template <typename V>
        requires yyjson::detail::validated_struct<V> && yyjson::detail::as_array_struct<V> &&
                 (!yyjson::detail::transparent_struct<V>)
        struct position_types<V>
        {
            using element = typename decltype(array_struct_element<V>())::type;
            template <std::size_t J>
            using prefix = element;
            using key = std::string;
            using value = writer::value;
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
                else if constexpr (yyjson::detail::transparent_struct<T>)
                    g.emit_transparent<T>(out);
                else if constexpr (yyjson::detail::as_array_struct<T>)
                    g.emit_array_struct<T>(out);
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
            template <typename S, std::size_t K>
            void emit_field_value(object_ref out);
            template <typename F>
            void emit_member(object_ref out);
            template <typename S>
            void emit_transparent(object_ref out);
            template <typename S>
            void emit_array_struct(object_ref out);
            template <typename S, std::size_t R>
            void emit_map_values(object_ref out);
            template <typename S, std::size_t P>
            void emit_pattern_values(object_ref out);
            template <typename S>
            void emit_key_names(object_ref out);
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
            template <typename V>
            void emit_variant(object_ref out);
            template <typename V, std::size_t I>
            void emit_alternative(object_ref out);
            template <typename V>
            static consteval bool bare_alternatives();
            // Adds the keywords of the node N, at a value of V of a field of S, to `out`.
            template <typename N, typename V, typename S>
            void emit_node(object_ref out);
            template <typename N, typename C>
            void emit_listed(object_ref out);
            template <typename N, typename C>
            static void emit_bounds(object_ref out);
            template <typename N, typename C, typename S>
            void emit_array_keywords(object_ref out);
            template <typename N, typename C, typename S>
            void emit_object_keywords(object_ref out);
            template <typename N, std::size_t M, typename C, typename S>
            void emit_members(object_ref properties, object_ref out);
            template <typename N, std::size_t R, typename V, typename S>
            void emit_combinators(object_ref out);
            template <typename N, typename S>
            void emit_matched(object_ref out);
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
                emit_variant<T>(out);
            else if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T>)
                emit_nullable<typename T::value_type>(out);
            else if constexpr (yd::owning_pointer<T>)
                emit_nullable<std::remove_cv_t<typename yd::is_owning_pointer<T>::value_type>>(out);
            else if constexpr (yd::indirect_value<T>)
                emit<typename yd::is_indirect<T>::value_type>(out);
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
            static_assert(!yd::has_adjacent_fields<S>, "C7f2: yyjson::adjacent_tag is not emitted yet");
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
            if constexpr (yd::has_pattern_fields<S>)
            {
                auto patterns = out.emplace("patternProperties", empty_object);
                [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                    (
                        [&] {
                            // both fields check the value of a key that their pattern matches
                            constexpr auto pattern = yd::pattern_of<S, Ps>;
                            auto target = [&] {
                                if (auto it = patterns.find(pattern); it != patterns.end())
                                {
                                    auto v = it->second;
                                    return conjunct(*v.as_object());
                                }
                                return patterns.emplace(pattern, empty_object, copy_string);
                            }();
                            emit_pattern_values<S, Ps>(target);
                        }(),
                        ...);
                }(std::make_index_sequence<yd::pattern_count<S>>{});
            }
            // the map of `yyjson::extra_keys` takes the unknown keys under every policy; else the read rejects them as
            // `struct_key_list::closed` decides it, and a read policy has no effect on the write
            if constexpr (yd::has_extra_keys<S>)
                emit_map_values<S, yd::extra_keys_index<S>>(out.emplace("additionalProperties", empty_object));
            else if (yd::struct_closed<S> ||
                     (!writing_ && ((policies_ | yd::struct_policies<S>)&yd::unknown_keys_policy) != 0))
                out.emplace("additionalProperties", false);
            emit_key_names<S>(out);
            if constexpr (yd::struct_property_counts<S>)
            {
                constexpr const auto& c = yd::struct_count_schema<S>::constraints;
                if constexpr (c.template has<yyjson::min_properties>())
                    tighten(out, "minProperties", c.min_properties, true);
                if constexpr (c.template has<yyjson::max_properties>())
                    tighten(out, "maxProperties", c.max_properties, false);
            }
            if constexpr (yd::has_struct_conditions<S>) emit_node<yd::node_schema<yd::struct_tree<S>, 0>, S, S>(out);
        }

        // The values of the map of the field R of S, by its index among all the fields, that receives keys of no
        // field: of its mapped type, with its `yyjson::additional_properties`.
        template <typename S, std::size_t R>
        void generator::emit_map_values(object_ref out)
        {
            namespace yd = yyjson::detail;
            using schema = yd::extra_keys_schema<S, R>;
            using E = typename yd::key_map_type<S, R>::mapped_type;
            emit<E>(out);
            if constexpr (schema::node.properties != 0)
                emit_node<typename schema::template at<schema::node.properties>, E, S>(out);
        }

        // The values that the pattern P of S checks.  The pattern reads a value that it does not keep as `read` does,
        // and a write gives such values from a field, a tag or an earlier pattern of another type that matches.
        template <typename S, std::size_t P>
        void generator::emit_pattern_values(object_ref out)
        {
            namespace yd = yyjson::detail;
            using E = typename yd::pattern_type<S, P>::mapped_type;
            constexpr auto r = yd::pattern_field_indices<S>[P];
            constexpr bool after_other = []<std::size_t... Qs>(std::index_sequence<Qs...>) {
                return (!std::same_as<typename yd::pattern_type<S, Qs>::mapped_type, E> || ...);
            }(std::make_index_sequence<P>{});
            const auto checks_others = [] {
                const auto& patterns = yd::key_patterns_of<S>();
                const auto has = [](const auto& masks) {
                    return std::ranges::any_of(masks, [](std::uint64_t m) { return (m >> P & 1) != 0; });
                };
                return after_other || has(patterns.fields) || has(patterns.tags);
            };
            if (!writing_ || !checks_others())
            {
                emit_map_values<S, r>(out);
                return;
            }
            auto written = value(object());
            emit_map_values<S, r>(*written.as_object());
            auto read = value(object());
            writing_ = false;
            emit_map_values<S, r>(*read.as_object());
            writing_ = true;
            if (written.write() == read.write())
                emit_map_values<S, r>(out);
            else
            {
                auto any = out.emplace("anyOf", empty_array);
                any.emplace_back(written);
                any.emplace_back(read);
            }
        }

        // The keys of S: its `yyjson::property_names` checks every key, that of the field of `yyjson::extra_keys` the
        // keys that no field names and no pattern matches, and that of a field of `yyjson::pattern_properties` the
        // keys that its pattern matches.
        template <typename S>
        void generator::emit_key_names(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr auto names_of = []<std::size_t R>(std::integral_constant<std::size_t, R>) {
                return yd::extra_keys_schema<S, R>::node.keys != 0;
            };
            constexpr bool extra_names = [&] {
                if constexpr (yd::has_extra_keys<S>)
                    return names_of(std::integral_constant<std::size_t, yd::extra_keys_index<S>>());
                else
                    return false;
            }();
            constexpr bool pattern_names = [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                return (names_of(std::integral_constant<std::size_t, yd::pattern_field_indices<S>[Ps]>()) || ...);
            }(std::make_index_sequence<yd::pattern_count<S>>{});
            if constexpr (yd::struct_property_names<S> || extra_names || pattern_names)
            {
                const auto emit_keys = [&]<std::size_t R>(std::integral_constant<std::size_t, R>, object_ref o) {
                    using schema = yd::extra_keys_schema<S, R>;
                    emit_node<typename schema::template at<schema::node.keys>,
                              typename yd::key_map_type<S, R>::key_type, S>(o);
                };
                auto names = out.emplace("propertyNames", empty_object);
                if constexpr (yd::struct_property_names<S>)
                    emit_node<yd::struct_names_schema<S>, std::string, S>(names);
                if constexpr (extra_names)
                {
                    // a key passes if it does not go into the map
                    auto any = place(names, "anyOf").emplace("anyOf", empty_array);
                    if constexpr (yd::struct_field_count<S> != 0)
                    {
                        auto keys = any.emplace_back(empty_object).emplace("enum", empty_array);
                        [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                            (keys.emplace_back(yd::struct_field_key<S, Ks>), ...);
                        }(std::make_index_sequence<yd::struct_field_count<S>>{});
                    }
                    [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                        (any.emplace_back(empty_object).emplace("pattern", yd::pattern_of<S, Ps>, copy_string), ...);
                    }(std::make_index_sequence<yd::pattern_count<S>>{});
                    emit_keys(std::integral_constant<std::size_t, yd::extra_keys_index<S>>(),
                              any.emplace_back(empty_object));
                }
                [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                    (
                        [&] {
                            constexpr auto r = yd::pattern_field_indices<S>[Ps];
                            if constexpr (names_of(std::integral_constant<std::size_t, r>()))
                            {
                                auto holder = place(names, "if");
                                holder.emplace("if", empty_object)
                                    .emplace("pattern", yd::pattern_of<S, Ps>, copy_string);
                                emit_keys(std::integral_constant<std::size_t, r>(),
                                          holder.emplace("then", empty_object));
                            }
                        }(),
                        ...);
                }(std::make_index_sequence<yd::pattern_count<S>>{});
            }
        }

        // Whether the read schema requires the key of the field K of S: the field declares no default.  The policies
        // do not change it, so that every read schema of S tells one contract.
        template <typename S, std::size_t K>
        [[nodiscard]] constexpr bool read_requires() noexcept
        {
            return !yyjson::detail::struct_field_absence<S>[K];
        }
        // Whether every write of S gives the key of its field K: the field is not omitted when empty, nor when equal
        // to its default unless the write takes `yyjson::emit_defaults`, nor when its default is a sentinel.
        template <typename S, std::size_t K>
        [[nodiscard]] bool write_requires(bool emit_defaults)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            constexpr auto r = yd::struct_declared_index<S, K>;
            if constexpr (yd::omittable<decltype(yd::writable_field(std::declval<const F&>()))>)
                return false;
            else if constexpr (yd::field_has_default<S, r>)
                return (emit_defaults || !yd::omits_default<S, r>) && !yd::sentinel_default<S, r>();
            else
                return true;
        }

        // Whether the read schema takes the written value `v` where it is narrower than the read: a tuple of pairs
        // whose values differ in type.
        template <typename T>
        bool read_schema_takes(const T& v);
        // Whether the read schema of E, the type of a pattern that does not keep the value, takes the written `v`.
        template <typename E, typename V>
        bool pattern_value_takes(const V& v)
        {
            namespace yd = yyjson::detail;
            if constexpr (std::same_as<std::remove_cvref_t<decltype(yd::writable_field(v))>, E>)
                return true;
            else
            {
                try
                {
                    const auto json = yyjson::value(yd::writable_field(v));
                    return read_schema_takes(
                        json.template cast<E>(yyjson::reject_missing_keys, yyjson::reject_null_as_missing));
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (const std::exception&)
                {
                    return false;
                }
            }
        }
        // Whether the read schema of each pattern of S takes the written values of the keys that it matches and that
        // a field or an earlier pattern keeps.
        template <typename S>
        bool patterns_take(const S& s)
        {
            namespace yd = yyjson::detail;
            const auto check = [&]<typename V>(std::uint64_t mask, const V& v) {
                return [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                    return ((((mask >> Ps & 1) == 0) ||
                             pattern_value_takes<typename yd::pattern_type<S, Ps>::mapped_type>(v)) &&
                            ...);
                }(std::make_index_sequence<yd::pattern_count<S>>{});
            };
            const auto& fields = yd::key_patterns_of<S>().fields;
            const bool fields_taken = [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                return ([&] {
                    decltype(auto) field = yd::writable_field(yd::struct_field<Ks>(s));
                    if constexpr (yd::omittable<decltype(field)>)
                        if (yd::should_omit_field(field)) return true;
                    return check(fields[Ks], yd::struct_field<Ks>(s));
                }() && ...);
            }(std::make_index_sequence<yd::struct_field_count<S>>{});
            return fields_taken && [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                return ([&] {
                    for (const auto& kv : yd::pattern_field<Ps>(s))
                        if (!check(yd::pattern_mask<S>(std::string_view(std::get<0>(kv)), Ps + 1), std::get<1>(kv)))
                            return false;
                    return true;
                }() && ...);
            }(std::make_index_sequence<yd::pattern_count<S>>{});
        }
        template <typename T>
        bool read_schema_takes(const T& v)
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T> || yd::owning_pointer<T>)
                return !v || read_schema_takes(*v);
            else if constexpr (yd::indirect_value<T>)
                return v.valueless_after_move() || read_schema_takes(*v);
            else if constexpr (struct_type<T>)
            {
                const auto map_takes = [](const auto& map) {
                    for (const auto& kv : map)
                        if (!read_schema_takes(std::get<1>(kv))) return false;
                    return true;
                };
                const bool patterns = [&] {
                    if constexpr (yd::has_pattern_fields<T>)
                        return patterns_take(v);
                    else
                        return true;
                }();
                return [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return (read_schema_takes(yd::struct_field<Ks>(v)) && ...);
                }(std::make_index_sequence<yd::struct_field_count<T>>{}) &&
                       std::apply([&](const auto&... maps) { return (map_takes(maps) && ...); }, yd::key_maps_of(v)) &&
                       patterns;
            }
            else if constexpr (primitive_type<T> || std::is_enum_v<T> || std::same_as<T, std::vector<bool>>)
                return true;
            else if constexpr (variant_type<T>)
            {
                const auto& base = yd::as_std_variant(v);
                return !base.valueless_by_exception() &&
                       std::visit([](const auto& a) { return read_schema_takes(a); }, base);
            }
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
            if constexpr (!readable<typename yd::read_back<std::remove_cv_t<F>>::type>)
                return std::nullopt;
            else
            {
                auto text = yd::round_trip_text<F>(d);
                if (text && !read_schema_takes(field)) text.reset();
                return text;
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

        // The `default` of the field K of S: its declared default unless it is a sentinel, or else the field of
        // `S()` if it satisfies the keywords of the field as `validate` checks them.  A required field has none.
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
            else if constexpr (yd::field_has_default<S, r>)
            {
                if (!yd::sentinel_default<S, r>()) it->second = default_text<F>(yd::stored_default<S, r>());
            }
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
            constexpr auto key = yyjson::detail::struct_field_key<S, K>;
            emit_field_value<S, K>(properties.emplace(key, empty_object));
            if (writing_ ? write_requires<S, K>(emit_defaults_) : read_requires<S, K>()) required.push_back(key);
        }

        // The schema of the field K of S at its key or at its element: its type, its keywords, and its `default`.
        template <typename S, std::size_t K>
        void generator::emit_field_value(object_ref out)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            emit_member<F>(out);
            if constexpr (yd::has_constraints<S>)
                emit_node<yd::field_schema<S, yd::struct_declared_index<S, K>>, F, S>(out);
            if (const auto& d = field_default<S, K>(names_)) out.emplace("default", yyjson::read(*d));
        }

        // A field of F leaves out its key or its element when it has no value, and reads null as a missing one, or
        // rejects it at an element: a `std::optional`, or a transparent struct over one.
        template <typename F>
        void generator::emit_member(object_ref out)
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::is_std_optional<F>::value)
                emit<typename F::value_type>(out);
            else
            {
                emit<F>(out);
                if constexpr (yd::transparent_struct<F> && yd::null_missing_field<F>)
                    place(out, "not").emplace("not", empty_object).emplace("type", "null");
            }
        }

        // The struct reads and writes as the value of its only field, with the keywords of that field.  A
        // `std::optional` field allows null here, because only a field of another struct leaves it out.
        template <typename S>
        void generator::emit_transparent(object_ref out)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, 0>;
            if constexpr (yd::has_constraints<S> && yd::is_std_optional<F>::value)
            {
                // the keywords of a `std::optional` field check only the value that it holds
                auto any = out.emplace("anyOf", empty_array);
                auto held = any.emplace_back(empty_object);
                emit<typename F::value_type>(held);
                emit_node<yd::field_schema<S, yd::struct_declared_index<S, 0>>, F, S>(held);
                any.emplace_back(empty_object).emplace("type", "null");
            }
            else
            {
                emit<F>(out);
                if constexpr (yd::has_constraints<S>)
                    emit_node<yd::field_schema<S, yd::struct_declared_index<S, 0>>, F, S>(out);
            }
        }

        // The fields are the elements at their positions, and the field of `yyjson::extra_items` takes the elements
        // after them.  A read takes an array that ends after the last required field and ignores the elements after
        // the fields; a write leaves out the empty optional fields at the end and writes every default.
        template <typename S>
        void generator::emit_array_struct(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr auto n = yd::array_position_count<S>;
            out.emplace("type", "array");
            auto least = std::size_t{0};
            if constexpr (n != 0)
            {
                auto prefix = out.emplace("prefixItems", empty_array);
                [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    (
                        [&] {
                            emit_field_value<S, Ks>(prefix.emplace_back(empty_object));
                            if (writing_ ? !yd::omittable<yd::struct_field_type<S, Ks>> : read_requires<S, Ks>())
                                least = Ks + 1;
                        }(),
                        ...);
                }(std::make_index_sequence<n>{});
            }
            if (least != 0) out.emplace("minItems", least);
            if constexpr (yd::has_extra_items<S>)
            {
                using E = std::remove_cv_t<std::ranges::range_value_t<yd::extra_items_type<S>>>;
                auto items = out.emplace("items", empty_object);
                emit<E>(items);
                if constexpr (yd::has_constraints<S>)
                {
                    using schema = yd::field_schema<S, yd::struct_declared_index<S, n>>;
                    if constexpr (schema::node.items != 0)
                        emit_node<typename schema::template at<schema::node.items>, E, S>(items);
                }
            }
            else if (yd::array_struct_table<S>.second ||
                     (!writing_ && ((policies_ | yd::struct_policies<S>)&yd::unknown_keys_policy) != 0))
                out.emplace("items", false);
            if constexpr (yd::has_array_struct_keywords<S>) emit_node<yd::array_struct_schema<S>, S, S>(out);
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

        // Whether a `type` array of the alternatives of V takes exactly what each of them takes: primitives of
        // distinct types without settings, of which at most one has the bounds of a number.
        template <typename V>
        consteval bool generator::bare_alternatives()
        {
            return []<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
                constexpr bool bare =
                    ((primitive_type<Ts> || std::same_as<Ts, std::monostate> || std::same_as<Ts, std::nullptr_t>) &&
                     ...);
                if constexpr (!bare)
                    return false;
                else
                {
                    const auto keyword = []<typename T>(std::type_identity<T>) -> std::string_view {
                        if constexpr (primitive_type<T>)
                            return type_keyword<T>();
                        else
                            return "null";
                    };
                    const auto keywords = std::array{keyword(std::type_identity<Ts>())...};
                    for (std::size_t i = 0; i < keywords.size(); ++i)
                        for (std::size_t j = 0; j < i; ++j)
                            if (keywords[i] == keywords[j]) return false;
                    constexpr auto numbers = ((std::integral<Ts> && !std::same_as<Ts, bool> ? 1 : 0) + ... + 0) +
                                             ((std::floating_point<Ts> ? 1 : 0) + ... + 0);
                    return numbers <= 1;
                }
            }(std::type_identity<yyjson::detail::variant_base_t<V>>{});
        }

        template <typename V, std::size_t I>
        void generator::emit_alternative(object_ref out)
        {
            namespace yd = yyjson::detail;
            using T = std::variant_alternative_t<I, yd::variant_base_t<V>>;
            emit<T>(out);
            if constexpr (yd::constrained_union_alternative<V, I>)
                emit_node<yd::union_alternative_schema<V, I>, T, typename yd::union_traits<V>::template alternative<I>>(
                    out);
        }

        // A variant without a tag reads a value that an alternative reads, and a `yyjson::one_of_variant` one that
        // exactly one reads.  `validate` checks that only against the read schemas, so that a written value can
        // match two write schemas.
        template <typename V>
        void generator::emit_variant(object_ref out)
        {
            namespace yd = yyjson::detail;
            static_assert(yd::union_variant<V> || !yd::internally_tagged<V>,
                          "C7f2: yyjson::internal_tag is not emitted yet");
            using B = yd::variant_base_t<V>;
            constexpr auto size = std::variant_size_v<B>;
            constexpr bool constrained = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return (yd::constrained_union_alternative<V, Is> || ...);
            }(std::make_index_sequence<size>{});
            if constexpr (size == 1)
                emit_alternative<V, 0>(out);
            else if constexpr (!constrained && bare_alternatives<V>())
            {
                auto types = out.emplace("type", empty_array);
                [&]<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
                    (
                        [&] {
                            if constexpr (primitive_type<Ts>)
                            {
                                types.emplace_back(type_keyword<Ts>());
                                emit_primitive_keywords<Ts>(out);
                            }
                            else
                                types.emplace_back("null");
                        }(),
                        ...);
                }(std::type_identity<B>{});
            }
            else
            {
                const auto keyword = yd::is_one_of_variant<V> && !writing_ ? "oneOf" : "anyOf";
                auto each = out.emplace(keyword, empty_array);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (emit_alternative<V, Is>(each.emplace_back(empty_object)), ...);
                }(std::make_index_sequence<size>{});
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

        // The JSON text of the value I of `yyjson::const_` and `yyjson::enum_` of N.
        template <typename N>
        std::string listed_text(std::size_t i)
        {
            using kind = yyjson::detail::json_scalar::kind_t;
            using number = yyjson::detail::bound_number::kind_t;
            const auto& s = N::values[i];
            auto text = std::string();
            switch (s.kind)
            {
                case kind::string:
                    yyjson::write(std::string(s.string()), text);
                    break;
                case kind::number:
                    if (s.number.kind == number::sint)
                        text = std::to_string(s.number.sint);
                    else if (s.number.kind == number::uint)
                        text = std::to_string(s.number.uint);
                    else
                        yyjson::write(s.number.real, text);
                    break;
                case kind::boolean:
                    text = s.boolean ? "true" : "false";
                    break;
                case kind::null:
                    text = "null";
                    break;
                case kind::json:
                    text = s.string();
                    break;
            }
            return text;
        }

        // The write schema takes a value as `write` gives what `read` takes of it, such as the shortest text of a
        // float.  A value that a read of C rejects is kept as declared.
        template <typename N, typename C>
        void generator::emit_listed(object_ref out)
        {
            namespace yd = yyjson::detail;
            using R = std::conditional_t<yd::constraint_string<C>, std::string, C>;
            const auto value = [this](std::size_t i) {
                auto text = listed_text<N>(i);
                if constexpr (!yd::dom_checked<C> && requires { yyjson::read<R>(std::string_view()); })
                {
                    if (writing_ && N::values[i].kind != yd::json_scalar::kind_t::null)
                    {
                        try
                        {
                            auto written = std::string();
                            yyjson::write(yyjson::read<R>(text), written);
                            text = std::move(written);
                        }
                        catch (const std::bad_alloc&)
                        {
                            throw;
                        }
                        catch (const std::exception&)
                        {
                        }
                    }
                }
                return yyjson::read(text);
            };
            constexpr bool constant = N::constraints.template has<yyjson::const_<>>();
            if constexpr (constant) place(out, "const").emplace("const", value(0));
            if constexpr (N::node.values_count > (constant ? 1 : 0))
            {
                auto values = place(out, "enum").emplace("enum", empty_array);
                for (std::size_t i = constant ? 1 : 0; i < N::node.values_count; ++i) values.emplace_back(value(i));
            }
        }

        // The bound `b` of a float F as the checker compares it, rounded to F, and an exclusive one as the adjacent
        // float inside, both written as their shortest text; none if F cannot hold it.
        template <typename F>
        std::optional<double> float_bound(const yyjson::detail::bound_number& b, bool lower, bool exclusive)
        {
            using kind = yyjson::detail::bound_number::kind_t;
            auto f = b.kind == kind::sint   ? static_cast<F>(b.sint)
                     : b.kind == kind::uint ? static_cast<F>(b.uint)
                                            : static_cast<F>(b.real);
            if (!std::isfinite(f)) return std::nullopt;
            if (exclusive)
                f = std::nextafter(f, lower ? std::numeric_limits<F>::infinity() : -std::numeric_limits<F>::infinity());
            if (!std::isfinite(f)) return std::nullopt;
            return yyjson::read(yyjson::write(f)).template cast<double>();
        }

        template <typename N, typename C>
        void generator::emit_bounds(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr const auto& c = N::constraints;
            using Num = typename std::conditional_t<yd::enum_integer_only<C>, std::underlying_type<C>,
                                                    std::type_identity<C>>::type;
            const auto bound = [&](std::string_view key, const yd::bound_number& b, bool lower, bool exclusive) {
                if constexpr (std::floating_point<Num> && yd::narrower_than_double<Num>)
                {
                    if (const auto f = float_bound<Num>(b, lower, exclusive); f.has_value())
                    {
                        tighten(out, lower ? "minimum" : "maximum", real_bound(*f), lower);
                        return;
                    }
                }
                tighten(out, key, b, lower);
            };
            if constexpr (c.template has<yyjson::minimum>()) bound("minimum", c.minimum, true, false);
            if constexpr (c.template has<yyjson::exclusive_minimum>())
                bound("exclusiveMinimum", c.exclusive_minimum, true, true);
            if constexpr (c.template has<yyjson::maximum>()) bound("maximum", c.maximum, false, false);
            if constexpr (c.template has<yyjson::exclusive_maximum>())
                bound("exclusiveMaximum", c.exclusive_maximum, false, true);
            if constexpr (c.template has<yyjson::multiple_of>())
                emplace_bound(place(out, "multipleOf"), "multipleOf", c.multiple_of);
        }

        // Appends `key` to the `required` of `out`, unless it is there.
        inline void add_required(object_ref out, std::string_view key)
        {
            if (auto it = out.find("required"); it != out.end())
            {
                auto keys = *it->second.as_array();
                for (const auto k : keys)
                    if (k.as_string() == key) return;
                keys.emplace_back(key, copy_string);
                return;
            }
            out.emplace("required", empty_array).emplace_back(key, copy_string);
        }

        // The keywords of a node go into the schema that the type already gives, as the checker applies them to the
        // same value.  The schemas of the subschemas get keywords alone, besides the type of their position.
        template <typename N, typename V, typename S>
        void generator::emit_node(object_ref out)
        {
            namespace yd = yyjson::detail;
            using C = yd::constrained_value_t<std::remove_cv_t<V>>;
            constexpr const auto& n = N::node;
            constexpr const auto& c = N::constraints;
            if constexpr (c.template has<yyjson::matches_t<void>>())
                emit_matched<N, S>(out.empty() ? out : conjunct(out));
            if constexpr (n.values_count != 0) emit_listed<N, C>(out);
            if constexpr ((c.bits & yd::numeric_constraints) != 0) emit_bounds<N, C>(out);
            if constexpr (c.template has<yyjson::min_length>()) tighten(out, "minLength", c.min_length, true);
            if constexpr (c.template has<yyjson::max_length>()) tighten(out, "maxLength", c.max_length, false);
            for (std::size_t i = 0; i < N::patterns.size(); ++i)
                (i == 0 ? place(out, "pattern") : conjunct(out)).emplace("pattern", N::patterns[i], copy_string);
            if constexpr (c.template has<yyjson::format<>>())
                place(out, "format").emplace("format", c.format_name.view(), copy_string);
            if constexpr (c.template has<yyjson::format_assertion<>>())
                place(out, "format").emplace("format", c.assertion_name.view(), copy_string);
            constexpr auto array_bits = yd::constraint_bit<yyjson::min_items> | yd::constraint_bit<yyjson::max_items> |
                                        yd::constraint_bit<yyjson::unique_items_t>;
            if constexpr ((c.bits & array_bits) != 0 || n.items != 0 || n.prefix_count != 0 || n.contains != 0)
                emit_array_keywords<N, C, S>(out);
            constexpr auto object_bits =
                yd::constraint_bit<yyjson::min_properties> | yd::constraint_bit<yyjson::max_properties>;
            if constexpr ((c.bits & object_bits) != 0 || n.properties != 0 || n.keys != 0)
                emit_object_keywords<N, C, S>(out);
            if constexpr (n.members != 0 || n.closed)
            {
                // `additionalProperties: false` sees only the `properties` of its own object, which must name only the
                // members of `yyjson::property`
                const bool own = n.closed && (out.contains("properties") || out.contains("additionalProperties") ||
                                              out.contains("patternProperties"));
                auto holder = own ? conjunct(out) : out;
                if constexpr (n.members != 0) emit_members<N, n.members, C, S>(holder, out);
                if constexpr (n.closed) holder.emplace("additionalProperties", false);
            }
            if constexpr (n.combinators != 0) emit_combinators<N, n.combinators, V, S>(out);
        }

        template <typename N, typename C, typename S>
        void generator::emit_array_keywords(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr const auto& n = N::node;
            constexpr const auto& c = N::constraints;
            using P = position_types<C>;
            if constexpr (c.template has<yyjson::min_items>()) tighten(out, "minItems", c.min_items, true);
            if constexpr (c.template has<yyjson::max_items>()) tighten(out, "maxItems", c.max_items, false);
            if constexpr (c.template has<yyjson::unique_items_t>())
                if (!out.contains("uniqueItems")) out.emplace("uniqueItems", true);
            if constexpr (n.prefix_count != 0 || n.items != 0)
            {
                constexpr bool tuple = yd::tuple_like<C> && !std::ranges::range<C>;
                bool done = false;
                if constexpr (tuple)
                {
                    // the array form of a tuple has a subschema at each position and no `items`
                    if (out.contains("prefixItems"))
                    {
                        [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                            (
                                [&] {
                                    auto at = sub_at(out, "prefixItems", Js);
                                    if constexpr (Js < n.prefix_count)
                                        emit_node<typename N::template at<n.prefix_first + Js>,
                                                  typename P::template prefix<Js>, S>(at);
                                    else if constexpr (n.items != 0)
                                        emit_node<typename N::template at<n.items>, typename P::template prefix<Js>, S>(
                                            at);
                                }(),
                                ...);
                        }(std::make_index_sequence<std::tuple_size_v<C>>{});
                        done = true;
                    }
                }
                if (!done)
                {
                    if constexpr (n.prefix_count != 0)
                    {
                        // `prefixItems` beside `items` would exempt its positions from the `items` of the type
                        auto target = out.contains("items") || out.contains("prefixItems") ? conjunct(out) : out;
                        auto prefix = target.emplace("prefixItems", empty_array);
                        [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                            (emit_node<typename N::template at<n.prefix_first + Js>, typename P::template prefix<Js>,
                                       S>(prefix.emplace_back(empty_object)),
                             ...);
                        }(std::make_index_sequence<n.prefix_count>{});
                        if constexpr (n.items != 0)
                            emit_node<typename N::template at<n.items>, typename P::element, S>(
                                target.emplace("items", empty_object));
                    }
                    else
                        emit_node<typename N::template at<n.items>, typename P::element, S>(sub(out, "items"));
                }
            }
            if constexpr (n.contains != 0)
            {
                auto target = place(out, "contains");
                auto contained = target.emplace("contains", empty_object);
                if constexpr (!(yd::tuple_like<C> && !std::ranges::range<C>) && !yd::as_array_struct<C>)
                    emit<typename P::element>(contained);
                emit_node<typename N::template at<n.contains>, typename P::element, S>(contained);
                if constexpr (c.template has<yyjson::min_contains>()) target.emplace("minContains", c.min_contains);
                if constexpr (c.template has<yyjson::max_contains>()) target.emplace("maxContains", c.max_contains);
            }
        }

        template <typename N, typename C, typename S>
        void generator::emit_object_keywords(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr const auto& n = N::node;
            constexpr const auto& c = N::constraints;
            using P = position_types<C>;
            if constexpr (c.template has<yyjson::min_properties>())
                tighten(out, "minProperties", c.min_properties, true);
            if constexpr (c.template has<yyjson::max_properties>())
                tighten(out, "maxProperties", c.max_properties, false);
            if constexpr (n.properties != 0)
                emit_node<typename N::template at<n.properties>, typename P::value, S>(
                    sub(out, "additionalProperties"));
            if constexpr (n.keys != 0)
                emit_node<typename N::template at<n.keys>, typename P::key, S>(sub(out, "propertyNames"));
            // a map read from an array of its pairs is checked as the object of those pairs
            if constexpr (yd::object_written_range<C>)
                if constexpr (yd::nested_casts<C>::elements)
                    if (!writing_)
                    {
                        if constexpr (c.template has<yyjson::min_properties>())
                            tighten(out, "minItems", c.min_properties, true);
                        if constexpr (c.template has<yyjson::max_properties>())
                            tighten(out, "maxItems", c.max_properties, false);
                        if constexpr (n.properties != 0 || n.keys != 0)
                        {
                            auto pair = sub(out, "items");
                            if (!pair.contains("prefixItems"))
                            {
                                auto prefix = pair.emplace("prefixItems", empty_array);
                                prefix.emplace_back(empty_object);
                                prefix.emplace_back(empty_object);
                            }
                            if constexpr (n.keys != 0)
                                emit_node<typename N::template at<n.keys>, typename P::key, S>(
                                    sub_at(pair, "prefixItems", 0));
                            if constexpr (n.properties != 0)
                                emit_node<typename N::template at<n.properties>, typename P::value, S>(
                                    sub_at(pair, "prefixItems", 1));
                        }
                    }
        }

        // The type of the value of the key `key` of a value of C.
        template <typename C, std::size_t K>
        struct member_type
        {
            using type = typename position_types<C>::value;
        };
        template <typename C, std::size_t K>
        requires (yyjson::detail::object_struct_value<C>())
        struct member_type<C, K>
        {
            using type = yyjson::detail::struct_field_type<C, K>;
        };

        template <typename N, std::size_t M, typename C, typename S>
        void generator::emit_members(object_ref holder, object_ref out)
        {
            namespace yd = yyjson::detail;
            using Mn = typename N::template at<M>;
            constexpr auto key = Mn::node.key.view();
            constexpr auto k = [] {
                if constexpr (yd::object_struct_value<C>())
                    return yd::struct_field_of_key<C>(Mn::node.key.view());
                else
                    return std::size_t{0};
            }();
            using F = typename member_type<C, k>::type;
            if constexpr (Mn::node.named || !empty_node(Mn::node))
            {
                // a key of `yyjson::required` alone is not one that `additionalProperties: false` allows
                auto at = Mn::node.named || !N::node.closed ? holder : conjunct(out);
                emit_node<Mn, F, S>(sub(sub(at, "properties"), key));
            }
            if constexpr (Mn::node.required) add_required(holder, key);
            if constexpr (Mn::node.next != 0) emit_members<N, Mn::node.next, C, S>(holder, out);
        }

        template <typename N, std::size_t R, typename V, typename S>
        void generator::emit_combinators(object_ref out)
        {
            namespace yd = yyjson::detail;
            using kind = yd::combinator_kind;
            using Rn = typename N::template at<R>;
            constexpr auto combinator = Rn::node.combinator;
            constexpr auto first = Rn::node.operands_first;
            const auto operand = [&]<std::size_t J>(std::integral_constant<std::size_t, J>, object_ref o) {
                emit_node<typename N::template at<first + J>, V, S>(o);
            };
            using zero = std::integral_constant<std::size_t, 0>;
            using one = std::integral_constant<std::size_t, 1>;
            using two = std::integral_constant<std::size_t, 2>;
            if constexpr (combinator == kind::not_)
                operand(zero(), place(out, "not").emplace("not", empty_object));
            else if constexpr (combinator == kind::if_then_else)
            {
                auto holder = place(out, "if");
                operand(zero(), holder.emplace("if", empty_object));
                if constexpr (!empty_node(N::template at<first + 1>::node))
                    operand(one(), holder.emplace("then", empty_object));
                if constexpr (!empty_node(N::template at<first + 2>::node))
                    operand(two(), holder.emplace("else", empty_object));
            }
            else if constexpr (combinator == kind::dependent_required)
            {
                // the node tree holds it as an `if` of the key and a `then` of the dependents, which the keyword
                // gives back in one place
                using If = typename N::template at<first>;
                constexpr auto key = N::template at<If::node.members>::node.key.view();
                auto dependents = [&] {
                    auto all = sub(out, "dependentRequired");
                    if (auto it = all.find(key); it != all.end())
                    {
                        auto v = it->second;
                        return *v.as_array();
                    }
                    return all.emplace(key, empty_array, copy_string);
                }();
                const auto add = [&]<std::size_t M>(auto& self, std::integral_constant<std::size_t, M>) -> void {
                    using Mn = typename N::template at<M>;
                    const auto dependent = Mn::node.key.view();
                    bool listed = false;
                    for (const auto d : dependents) listed = listed || d.as_string() == dependent;
                    if (!listed) dependents.emplace_back(dependent, copy_string);
                    if constexpr (Mn::node.next != 0) self(self, std::integral_constant<std::size_t, Mn::node.next>());
                };
                add(add, std::integral_constant<std::size_t, N::template at<first + 1>::node.members>());
            }
            else
            {
                constexpr std::string_view keyword = combinator == kind::all_of ? "allOf"
                                                     : combinator == kind::any_of || combinator == kind::any_of_keys
                                                         ? "anyOf"
                                                         : "oneOf";
                auto operands = place(out, keyword).emplace(keyword, empty_array);
                [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                    (operand(std::integral_constant<std::size_t, Js>(), operands.emplace_back(empty_object)), ...);
                }(std::make_index_sequence<Rn::node.operands_count>{});
            }
            if constexpr (Rn::node.next != 0) emit_combinators<N, Rn::node.next, V, S>(out);
        }

        // `yyjson::matches<T>` keeps no T but its check, whose name finds T among the settings of S.
        template <typename N, typename S>
        void generator::emit_matched(object_ref out)
        {
            using Ts = typename matched_types<typename settings_of<S>::type>::type;
            constexpr auto i = matched_index<Ts>::of(N::constraints.matches.name());
            static_assert(i < std::tuple_size_v<Ts>,
                          "C7h: yyjson::matches inside yyjson::prefix_items, all_of, any_of or one_of is not emitted "
                          "yet");
            if constexpr (i < std::tuple_size_v<Ts>)
            {
                // the check reads T as `read` does, whatever the direction of the schema
                const bool writing = writing_;
                writing_ = false;
                emit<std::tuple_element_t<i, Ts>>(out);
                writing_ = writing;
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
