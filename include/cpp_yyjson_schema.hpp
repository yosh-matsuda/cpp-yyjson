// JSON Schema (2020-12, or the dialect of OpenAPI 3.1) of the C++ types that cpp-yyjson reads and writes.

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
#include <stdexcept>
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

    // The tags of `json_schema` and `schema_collection` that choose the dialect: JSON Schema 2020-12, or the one of
    // OpenAPI 3.1, which adds `format` to the numbers and leaves `$schema` to the `jsonSchemaDialect` of the document.
    struct json_schema_2020_12_t
    {
        explicit json_schema_2020_12_t() = default;
    };
    inline constexpr json_schema_2020_12_t json_schema_2020_12{};
    struct openapi_3_1_t
    {
        explicit openapi_3_1_t() = default;
    };
    inline constexpr openapi_3_1_t openapi_3_1{};

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
        // The schemas of a type with a caster by the caster, at any value and at a field.
        template <typename T>
        inline constexpr char caster_form_tag = 0;
        template <typename T>
        inline constexpr char field_form_tag = 0;
        template <typename S, std::size_t K>
        inline constexpr char field_tag = 0;

        template <typename T>
        concept user_caster = (yyjson::detail::caster_names_from_json<T> || yyjson::detail::caster_names_to_json<T>) &&
                              !std::derived_from<caster<T>, yyjson::detail::builtin_caster>;
        // `with_caster` whose X has no `to_json`, so that its value is written as the library writes it.
        template <typename T>
        concept library_written = yyjson::detail::is_with_caster<T>::value &&
                                  !yyjson::detail::converter_writes<typename T::value_type, typename T::caster_type>;

        // The value that the caster of T reads, in the order that the library tries the forms of `from_json`: at a
        // field of a struct the JSON value form that assigns in place, then the in-place translator, and at any
        // position the returning translator unless a JSON value form returns the value.
        enum class caster_source
        {
            library,
            json,
            in_place,
            returning
        };
        template <typename T, bool Field>
        consteval caster_source caster_source_of()
        {
            namespace yd = yyjson::detail;
            using translator = yd::from_json_translator<T>;
            if constexpr (!yd::caster_names_from_json<T>)
                return caster_source::library;
            else if constexpr (Field && yd::caster_assigns_from_json<T, reader::const_value_ref>)
                return caster_source::json;
            else if constexpr (Field && translator::in_place)
                return caster_source::in_place;
            else if constexpr (translator::castable)
                return caster_source::returning;
            else
                return caster_source::json;
        }
        template <typename T>
        consteval bool field_reads_otherwise()
        {
            constexpr auto field = caster_source_of<T, true>();
            constexpr auto other = caster_source_of<T, false>();
            if constexpr (field == caster_source::in_place && other == caster_source::returning)
                return !std::same_as<yyjson::detail::from_json_source<T>, yyjson::detail::from_json_in_place_source<T>>;
            else
                return field != other;
        }
        // A caster whose field of a struct reads a value that differs from the one at any other position.
        template <typename T>
        concept two_sources = field_reads_otherwise<T>();

        template <typename T>
        concept variant_type = yyjson::detail::variant_json_kinds<T>::is_variant;

        // A variant that `yyjson::internal_tag` tags, which reads the annotations of T only when T is a variant.
        template <typename T>
        concept internal_tagged_variant =
            variant_type<T> && !yyjson::detail::union_variant<T> && yyjson::detail::internally_tagged<T>;

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
        concept primitive_type =
            (std::same_as<T, bool> || std::integral<T> || std::floating_point<T> || string_type<T>) && !user_caster<T>;

        class generator;

        // What the call of `json_schema` asks for besides the direction: its read policies, whether the write takes
        // `yyjson::emit_defaults`, and whether the numbers take the `format` of OpenAPI.
        struct schema_mode
        {
            std::uint8_t policies = 0;
            bool emit_defaults = false;
            bool number_formats = false;
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

        // The metadata of the schema of a field, a type or an alternative of a union, which views the settings.
        // `read` reads an example as the read of the value does, and gives the text that `write` gives for it.
        struct metadata
        {
            std::optional<std::string_view> title;
            std::optional<std::string_view> description;
            bool deprecated = false;
            bool read_only = false;
            bool write_only = false;
            std::vector<std::string_view> examples;
            std::string (*read)(std::string_view, std::uint8_t) = nullptr;
            const yyjson::schema* declared = nullptr;
            std::optional<std::string_view> extend;
            // what the messages name: the type, the field or the alternative
            std::string owner;

            // The metadata of `over`, and of this where `over` has none.
            void merge_under(metadata over)
            {
                if (over.title) title = over.title;
                if (over.description) description = over.description;
                deprecated = deprecated || over.deprecated;
                read_only = read_only || over.read_only;
                write_only = write_only || over.write_only;
                if (!over.examples.empty())
                {
                    examples = std::move(over.examples);
                    read = over.read;
                }
                if (over.declared != nullptr) declared = over.declared;
                if (over.extend) extend = over.extend;
                if (!over.owner.empty()) owner = std::move(over.owner);
            }
        };

        // Reads the example `text` as T under `policies`, with the keywords that `Check::check` checks, and gives
        // what `write` writes for it.
        template <typename T, typename Check>
        struct value_example
        {
            static std::string read(std::string_view text, std::uint8_t policies)
            {
                namespace yd = yyjson::detail;
                if constexpr (!yd::read_castable<T>)
                    throw std::invalid_argument("the read does not take " + std::string(yd::type_name<T>()));
                else
                {
                    const auto scope = yd::read_policy_scope(policies);
                    const auto v = yyjson::read<T>(text);
                    Check::check(v, text);
                    if constexpr (writable<T>)
                        return std::string(yyjson::write(v));
                    else
                        return std::string(text);
                }
            }
        };
        struct no_check
        {
            static void check(const auto&, std::string_view) {}
        };
        // Reads the example `text` into the field K of S as the read of S does, and gives what `write` writes for it.
        template <typename S, std::size_t K>
        struct field_example
        {
            static std::string read(std::string_view text, std::uint8_t policies)
            {
                namespace yd = yyjson::detail;
                using F = std::remove_cv_t<yd::struct_field_type<S, K>>;
                constexpr auto i = yd::reflected_field_index<S, K>;
                if constexpr (yd::adjacent_field<S, K>)
                    throw std::invalid_argument("a field with yyjson::adjacent_tag reads its tag beside the value");
                else if constexpr (!std::is_default_constructible_v<F>)
                    return value_example<F, no_check>::read(text, policies);
                else
                {
                    const auto scope = yd::read_policy_scope(policies);
                    const auto json = yyjson::read(text);
                    F f{};
                    if constexpr (yd::null_takes_default<S, i>)
                        if (json.is_null())
                        {
                            f = yd::default_field_value<S, i>();
                            return written(f, text);
                        }
                    yd::assign_reflected_field_impl(f, json, false);
                    if constexpr (yd::constrained_field<S, K>) yd::check_field_constraints<S, K>(f, json);
                    return written(f, text);
                }
            }
            static std::string written(const auto& f, std::string_view text)
            {
                if constexpr (writable<std::remove_cvref_t<decltype(f)>>)
                    return std::string(yyjson::write(f));
                else
                    return std::string(text);
            }
        };

        // Adds the metadata setting `a` to `m`, whose examples `Example::read` reads.
        template <typename Example, typename A>
        void add_metadata(metadata& m, const A& a)
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::setting_of<A, yyjson::title<>>)
                m.title = a.text();
            else if constexpr (yd::setting_of<A, yyjson::description<>>)
                m.description = a.text();
            else if constexpr (std::same_as<A, yyjson::deprecated_t>)
                m.deprecated = true;
            else if constexpr (std::same_as<A, yyjson::read_only_t>)
                m.read_only = true;
            else if constexpr (std::same_as<A, yyjson::write_only_t>)
                m.write_only = true;
            else if constexpr (yd::setting_of<A, yyjson::examples_t<0>>)
            {
                for (std::size_t i = 0; i < a.size(); ++i) m.examples.push_back(a.text(i));
                m.read = &Example::read;
            }
            else if constexpr (std::same_as<A, yyjson::schema>)
                m.declared = &a;
            else if constexpr (yd::setting_of<A, yyjson::schema_extend<>>)
                m.extend = a.text();
        }

        template <typename T>
        concept described_type = requires { yyjson::annotations<T>.metadata; };

        // The name of T in `$defs` that `yyjson::schema_name` gives, or none.
        template <typename T>
        inline constexpr std::string_view schema_name_of = [] {
            auto name = std::string_view();
            if constexpr (described_type<T>)
                yyjson::detail::for_each_annotation(
                    [&]<typename A>(const A& a) {
                        if constexpr (std::same_as<A, yyjson::schema_name>) name = a.text();
                    },
                    yyjson::annotations<T>.metadata);
            return name;
        }();

        // The fields that have a schema of their own: the fields with keys, or the positions of an array.
        template <typename S>
        inline constexpr std::size_t described_fields = [] {
            namespace yd = yyjson::detail;
            if constexpr (yd::as_array_struct<S> && !yd::transparent_struct<S>)
                return yd::array_position_count<S>;
            else
                return yd::struct_field_count<S>;
        }();
        // Whether each `yyjson::field` with metadata of T names a field that has a schema of its own.
        template <typename T>
        consteval bool fields_described()
        {
            namespace yd = yyjson::detail;
            bool all = true;
            yd::for_each_annotation(
                [&]<typename E>(const E& entry) {
                    if constexpr (yd::is_field_annotation<E>::value)
                    {
                        if constexpr (!struct_type<T>)
                            all = false;
                        else
                        {
                            bool found = false;
                            yd::all_indices<described_fields<T>>([&](auto k) {
                                found = found || entry.member() ==
                                                     yd::declared_member_name<T, yd::struct_declared_index<T, k()>>;
                                return true;
                            });
                            all = all && found;
                        }
                    }
                },
                yyjson::annotations<T>.metadata);
            return all;
        }

        // The metadata that `yyjson::annotate` gives T.
        template <typename T>
        metadata type_metadata()
        {
            namespace yd = yyjson::detail;
            auto m = metadata();
            if constexpr (described_type<T>)
            {
                static_assert(fields_described<T>(),
                              "yyjson::field(\"member\", ...) with metadata, such as yyjson::title, names a member of "
                              "the struct that is read and written by its key or by its position, not the map of "
                              "yyjson::extra_keys, yyjson::pattern_properties or yyjson::extra_items");
                yd::for_each_annotation(
                    [&]<typename A>(const A& a) {
                        if constexpr (!yd::is_field_annotation<A>::value)
                            add_metadata<value_example<T, no_check>>(m, a);
                    },
                    yyjson::annotations<T>.metadata);
                m.owner = yd::type_name<T>();
            }
            return m;
        }

        // The metadata of the field K of S, by its index among the fields that are read.
        template <typename S, std::size_t K>
        metadata field_metadata()
        {
            namespace yd = yyjson::detail;
            auto m = metadata();
            if constexpr (described_type<S>)
            {
                constexpr auto member = yd::declared_member_name<S, yd::struct_declared_index<S, K>>;
                yd::for_each_annotation(
                    [&]<typename E>(const E& entry) {
                        if constexpr (yd::is_field_annotation<E>::value)
                            if (entry.member() == member)
                                yd::for_each_annotation([&](const auto& a) { add_metadata<field_example<S, K>>(m, a); },
                                                        entry.values);
                    },
                    yyjson::annotations<S>.metadata);
                m.owner = std::string(yd::type_name<S>()) + "::" + std::string(member);
            }
            return m;
        }

        // The metadata of the alternative I of the union V, in its `yyjson::alt`.
        template <typename V, std::size_t I,
                  typename A = typename yyjson::detail::union_traits<V>::template alternative<I>>
        struct alternative_metadata
        {
            static metadata get() { return {}; }
        };
        template <typename V, std::size_t I, typename T, auto... Settings>
        struct alternative_metadata<V, I, yyjson::alt<T, Settings...>>
        {
            static metadata get()
            {
                namespace yd = yyjson::detail;
                auto m = metadata();
                (add_metadata<value_example<T, alternative_metadata>>(m, yd::alternative_setting_value<Settings>()),
                 ...);
                m.owner = "the alternative " + std::to_string(I) + " of " + std::string(yd::type_name<V>());
                return m;
            }
            static void check(const T& v, std::string_view text)
            {
                namespace yd = yyjson::detail;
                if constexpr (yd::constrained_union_alternative<V, I>)
                {
                    using Schema = yd::union_alternative_schema<V, I>;
                    const auto json = yyjson::read(text);
                    if (const auto kind = yd::constraint_violation<Schema, true>(v, yd::input_for<Schema, T>(&json));
                        kind != yd::constraint_kind::none)
                        throw std::invalid_argument(yd::keyword_failure_text(yd::constraint_facts<Schema>(kind)));
                }
            }
        };
        // Whether an alternative of the union V has metadata.
        template <typename A>
        inline constexpr bool described_alternative = false;
        template <typename T, auto... Settings>
        inline constexpr bool described_alternative<yyjson::alt<T, Settings...>> =
            (yyjson::detail::metadata_setting<yyjson::detail::alternative_setting_t<Settings>> || ...);
        template <typename V>
        consteval bool described_alternatives()
        {
            namespace yd = yyjson::detail;
            if constexpr (!yd::union_variant<V>)
                return false;
            else
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (described_alternative<typename yd::union_traits<V>::template alternative<Is>> || ...);
                }(std::make_index_sequence<std::variant_size_v<yd::variant_base_t<V>>>{});
        }

        // The types T of `yyjson::matches<T>` in the settings A.  A subschema of `yyjson::all_of`, `any_of`, `one_of`
        // and `prefix_items` keeps no type of its settings.
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
        // The types whose schema is their kind of JSON, which a check names without a setting that keeps the type.
        using kind_types = std::tuple<bool, std::nullptr_t, double, long double, std::string, std::string_view,
                                      yyjson::value, yyjson::object, yyjson::array>;
        // The index in Ts of the type whose check is `test`, or the size of Ts.
        template <typename... Ts>
        consteval std::size_t matched_index(const yyjson::detail::type_test& test, std::tuple<Ts...>*)
        {
            std::size_t i = 0;
            ((test.test == &yyjson::detail::test_type<Ts> || (++i, false)) || ...);
            return i;
        }

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
            using tag_keys = std::vector<std::string>;
            // the keys are a copy, which the registry can move while the body adds types
            using body_function = void (*)(generator&, object_ref, tag_keys);
            struct entry
            {
                const void* id;
                std::string base;
                body_function body;
                std::array<bool, 2> used = {};  // read, write
                bool shared = true;
                std::array<std::string, 2> names = {};
                // the keys of the internal tags that the object of a struct holds besides its members
                tag_keys tags = {};
                // 0 when `yyjson::schema_name` gives the base as it is, 1 when it gives the start of the base, and 2
                // otherwise; a lower tier takes a name first, so that a derived name never takes an explicit one
                std::uint8_t tier = 2;
            };
            std::vector<entry> types;
            std::map<std::pair<const void*, tag_keys>, std::size_t> index;
            // Whether the keys of internal tags change the schema of a type in a direction.
            std::map<std::tuple<const void*, tag_keys, bool>, bool> tagged;
            std::size_t unnamed_tags = 0;
            // The schemas in the order in which they are found: the type and whether it is the write direction.
            std::vector<std::pair<std::size_t, bool>> order;
            std::string ref_prefix;
            // Until the names are settled, a reference names the entry, so that the schemas can be compared.
            bool settled = false;
            // `S()` of each struct, and the JSON text of the `default` of each field, made once per generation.
            std::map<const void*, std::shared_ptr<const void>> initial;
            std::map<const void*, std::optional<std::string>> defaults;
            // The type of each `yyjson::schema_name`, and the name of that type.
            std::map<std::string, std::pair<const void*, std::string_view>, std::less<>> schema_names;

            // The schema of a type with a caster: its conversion by the library, or by the caster, at a field or at
            // any other value.
            enum class form : std::uint8_t
            {
                library,
                caster,
                caster_field,
            };
            // The form is a template argument, so that a caster type never instantiates the conversion by the library.
            template <typename T, form F = form::library>
            std::size_t find_or_add(bool writing, const tag_keys& tags = {});

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
            // another type has already gets the first free suffix "_2", "_3", ... in the order of `order`, after the
            // names of `yyjson::schema_name`.
            void assign_names()
            {
                auto taken = std::set<std::string, std::less<>>();
                auto done = std::vector<bool>(types.size());
                for (const std::uint8_t pass : {0, 1, 2})
                    for (const auto& [i, writing] : order)
                    {
                        if (done[i] || types[i].tier != pass) continue;
                        done[i] = true;
                        auto& t = types[i];
                        for (std::size_t n = 1;; ++n)
                        {
                            const auto base = n == 1 ? t.base : t.base + "_" + std::to_string(n);
                            if (split(i))
                                t.names = {base + "-Input", base + "-Output"};
                            else
                                t.names = {base, std::string()};
                            if (!taken.contains(t.names[0]) && (t.names[1].empty() || !taken.contains(t.names[1])))
                                break;
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
                  read_policies_(mode.policies),
                  emit_defaults_(writing && mode.emit_defaults),
                  number_formats_(mode.number_formats)
            {
            }

            [[nodiscard]] bool writing() const noexcept { return writing_; }

            // Writes the schema of T into `out`, by the conversion of the library if not `User`.
            template <typename T, bool User = true>
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
            static void emit_definition(generator& g, object_ref out, registry::tag_keys tags)
            {
                const auto body = [&] { g.emit_library<T>(out, std::move(tags)); };
                // the metadata of a type with a caster describe the conversion by the caster
                if constexpr (user_caster<T>)
                    body();
                else
                    g.emit_described(out, described_metadata<T>(), body);
            }
            template <typename T, bool Field>
            static void emit_caster_definition(generator& g, object_ref out, registry::tag_keys tags)
            {
                g.emit_described(out, caster_metadata<T>(Field), [&] {
                    g.tags_ = std::move(tags);
                    g.field_ = Field;
                    g.emit_user<T>(out);
                });
            }

        private:
            // Writes the schema of the nested `yyjson::matches<T>`, which the check reads as `read` does.
            template <typename T>
            void emit_matches(object_ref out)
            {
                const bool writing = writing_;
                auto tags = std::exchange(tags_, {});
                auto within = std::exchange(within_, {});
                writing_ = false;
                emit<T>(out);
                writing_ = writing;
                tags_ = std::move(tags);
                within_ = std::move(within);
            }
            template <typename T>
            void emit_library(object_ref out, registry::tag_keys tags)
            {
                if constexpr (variant_type<T>)
                {
                    tags_ = std::move(tags);
                    emit_variant<T>(out);
                }
                else if constexpr (std::is_enum_v<T>)
                    emit_enum<T>(out);
                else if constexpr (yyjson::detail::transparent_struct<T>)
                {
                    tags_ = tags;
                    within_ = {&type_identity_tag<T>};
                    emit_transparent<T>(out);
                }
                else if constexpr (yyjson::detail::as_array_struct<T>)
                    emit_array_struct<T>(out);
                else
                    emit_struct<T>(out, tags);
            }
            // The examples of a type with a caster read by the returning `from_json`, which a field need not take.
            template <typename T>
            static metadata caster_metadata(bool field)
            {
                auto m = type_metadata<T>();
                if constexpr (two_sources<T>)
                    if (field) m.examples.clear();
                return m;
            }
            // The metadata of the type T, where a transparent struct takes those of its field where it has none.
            template <typename T>
            static metadata described_metadata()
            {
                if constexpr (yyjson::detail::transparent_struct<T>)
                {
                    auto m = field_metadata<T, 0>();
                    m.merge_under(type_metadata<T>());
                    return m;
                }
                else
                    return type_metadata<T>();
            }
            // Writes the schema that `body` writes, or the one that `yyjson::schema` declares, with the metadata `m`.
            template <typename Body>
            void emit_described(object_ref out, const metadata& m, Body&& body)
            {
                if (!emit_annotations(out, m)) std::forward<Body>(body)();
                emit_extension(out, m);
            }
            // Writes the metadata that `out` has not, and gives whether a declared schema replaces the generated one.
            bool emit_annotations(object_ref out, const metadata& m);
            void emit_extension(object_ref out, const metadata& m);

            registry& names_;
            bool writing_;
            std::uint8_t policies_;
            // the policies that read the examples in both directions
            std::uint8_t read_policies_;
            bool emit_defaults_;
            bool number_formats_;
            // The keys of the internal tags that the object of the next value holds besides its own members, which
            // a variant, a nullable value and a transparent struct pass on to the value that they hold.
            registry::tag_keys tags_;
            // The keys that `additionalProperties: false` of a node at the current object takes, each with whether
            // it is the key of an internal tag.
            std::vector<std::pair<std::string, bool>> closed_keys_;
            // Whether a key of an internal tag changed the schema.
            bool tags_used_ = false;
            // The transparent structs whose schemas the next value is the schema of, which a variant, a nullable
            // value and a transparent struct pass on as they pass `tags_`.
            std::vector<const void*> within_;
            // The types with a caster of the user whose schemas the next value is inside, which have no name to refer
            // to.
            std::vector<const void*> casters_;
            // Whether the next value is a field of a struct, which a caster reads by its in-place `from_json`.
            bool field_ = false;

            // A transparent struct that holds itself at the same value never ends a read, and one inside another
            // has its schema there, so that no chain of references comes back to a schema without a value between.
            template <typename T>
            void emit_named(object_ref out, const registry::tag_keys& tags, std::vector<const void*> within)
            {
                if constexpr (yyjson::detail::transparent_struct<T>)
                {
                    if (std::ranges::find(within, &type_identity_tag<T>) != within.end())
                    {
                        out.emplace("not", empty_object);
                        return;
                    }
                    if (!within.empty())
                    {
                        within.push_back(&type_identity_tag<T>);
                        emit_described(out, described_metadata<T>(), [&] {
                            tags_ = tags;
                            within_ = std::move(within);
                            emit_transparent<T>(out);
                        });
                        return;
                    }
                }
                auto used = false;
                if constexpr (struct_type<T> && !yyjson::detail::as_array_struct<T>)
                    used = !tags.empty() && changes_schema<T>(tags);
                const auto i = names_.find_or_add<T>(writing_, used ? tags : registry::tag_keys());
                tags_used_ = tags_used_ || used;
                out.emplace("$ref", names_.ref(i, writing_), copy_string);
            }
            // Whether the keys of internal tags change the schema of the struct T, which a trial emission tells.  The
            // trial adds no schema, and a reach of T inside it takes the keys as unused.
            template <typename T>
            bool changes_schema(const registry::tag_keys& tags)
            {
                const auto [it, added] = names_.tagged.try_emplace({&type_identity_tag<T>, tags, writing_}, false);
                if (added)
                {
                    auto saved =
                        std::tuple(names_.types, names_.index, names_.order, names_.unnamed_tags, names_.schema_names);
                    auto g = *this;
                    g.closed_keys_.clear();
                    g.casters_.clear();
                    g.tags_used_ = false;
                    auto doc = value(object());
                    emit_definition<T>(g, *doc.as_object(), tags);
                    it->second = g.tags_used_;
                    std::tie(names_.types, names_.index, names_.order, names_.unnamed_tags, names_.schema_names) =
                        std::move(saved);
                }
                return it->second;
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
            void emit_number_format(object_ref out) const;
            template <typename T>
            void emit_nullable(object_ref out);
            template <typename E>
            void emit_enum(object_ref out);
            template <typename K>
            void emit_property_names(object_ref out);
            template <typename S>
            void emit_struct(object_ref out, const registry::tag_keys& tags);
            template <typename T>
            void emit_user(object_ref out);
            template <typename T>
            void emit_caster_read(object_ref out, const registry::tag_keys& tags, bool field);
            template <typename T, bool Field>
            void emit_caster_source(object_ref out);
            template <typename T>
            void emit_caster_write(object_ref out, const registry::tag_keys& tags);
            template <typename T>
            void emit_caster_keywords(object_ref out, const registry::tag_keys& tags);
            template <typename T>
            void emit_unconverted(object_ref out);
            template <typename S>
            void emit_adjacent_tags(object_ref out);
            template <typename S, std::size_t K>
            void emit_adjacent_tag(object_ref out);
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
            template <typename S>
            void emit_struct_values(object_ref out);
            template <typename S, std::size_t P>
            void emit_pattern_values(object_ref out, const std::vector<std::string_view>& tags);
            template <typename S>
            void emit_key_names(object_ref out, const std::vector<std::string_view>& tags);
            template <typename T>
            void emit_range(object_ref out, const registry::tag_keys& tags);
            template <typename T, typename E>
            void emit_array_form(object_ref out);
            template <typename T, typename K, typename V>
            void emit_object_form(object_ref out, const registry::tag_keys& tags = {});
            template <typename T, std::size_t... Is>
            void emit_tuple(object_ref out, std::index_sequence<Is...>, const registry::tag_keys& tags);
            template <typename T, std::size_t... Is>
            void emit_pair_object(object_ref out, const registry::tag_keys& tags);
            void declare_tag_keys(object_ref out, const registry::tag_keys& tags);
            template <typename... Vs>
            void emit_values(object_ref out, std::string_view combinator);
            template <typename V>
            void emit_variant(object_ref out);
            template <typename V, std::size_t I, bool Described = true>
            void emit_alternative(object_ref out);
            template <typename V>
            void emit_internal_tag(object_ref out, registry::tag_keys tags);
            template <typename V>
            void emit_untagged(object_ref out);
            template <typename V, std::size_t I>
            void emit_untagged_alternative(object_ref out, const registry::tag_keys& tags,
                                           const std::vector<const void*>& within);
            template <typename V>
            static consteval bool bare_alternatives();
            // Adds the keywords of the node N, at a value of V of a field of S, to `out`.
            template <typename N, typename V, typename S>
            void emit_node(object_ref out);
            template <typename N, typename V, typename S>
            bool emit_projections(object_ref out);
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

        // A struct whose object holds the keys of internal tags is "Name-key" in `$defs`, or "Name-tagN" when a key
        // has a character that a name does not take.  The name of a caster type names the schema of the caster, and a
        // field that reads by the in-place `from_json` has "Name-field".
        template <typename T, registry::form F>
        std::size_t registry::find_or_add(bool writing, const tag_keys& tags)
        {
            const void* id = F == form::library  ? &type_identity_tag<T>
                             : F == form::caster ? static_cast<const void*>(&caster_form_tag<T>)
                                                 : &field_form_tag<T>;
            const auto [it, added] = index.try_emplace({id, tags}, types.size());
            const auto i = it->second;
            if (added)
            {
                const auto name = user_caster<T> && F == form::library ? std::string_view() : schema_name_of<T>;
                if (!name.empty())
                {
                    const auto [at, fresh] = schema_names.try_emplace(std::string(name), &type_identity_tag<T>,
                                                                      yyjson::detail::type_name<T>());
                    if (!fresh && at->second.first != &type_identity_tag<T>)
                        throw std::invalid_argument("json_schema: " + std::string(at->second.second) + " and " +
                                                    std::string(yyjson::detail::type_name<T>()) +
                                                    " have the same yyjson::schema_name \"" + std::string(name) + "\"");
                }
                auto base = name.empty() ? def_name(yyjson::detail::type_name<T>()) : std::string(name);
                if (F == form::caster_field) base += "-field";
                if (std::ranges::all_of(tags, [](const std::string& key) { return def_name(key) == key; }))
                    for (const auto& key : tags) base += "-" + key;
                else if (!tags.empty())
                    base += "-tag" + std::to_string(++unnamed_tags);
                if constexpr (F == form::library)
                    types.push_back({id, std::move(base), &generator::emit_definition<T>});
                else
                    types.push_back(
                        {id, std::move(base), &generator::emit_caster_definition<T, F == form::caster_field>});
                types.back().tags = tags;
                types.back().tier = name.empty() ? 2 : types.back().base == name ? 0 : 1;
            }
            if (!types[i].used[writing ? 1 : 0])
            {
                types[i].used[writing ? 1 : 0] = true;
                order.emplace_back(i, writing);
            }
            return i;
        }

        template <typename T, bool User>
        void generator::emit(object_ref out)
        {
            namespace yd = yyjson::detail;
            // a variant, a nullable value and a reference pass the keys of internal tags on to the value they hold
            auto tags = std::exchange(tags_, {});
            auto within = std::exchange(within_, {});
            const auto field = std::exchange(field_, false);
            const auto pass = [&] {
                tags_ = std::move(tags);
                within_ = std::move(within);
            };
            if constexpr (!std::same_as<T, std::remove_cv_t<T>>)
            {
                pass();
                field_ = field;
                emit<std::remove_cv_t<T>>(out);
            }
            else if constexpr (std::same_as<T, json_view> || std::same_as<T, writer::value>)
            {
            }
            else if constexpr (std::same_as<T, writer::object>)
                out.emplace("type", "object");
            else if constexpr (std::same_as<T, writer::array>)
                out.emplace("type", "array");
            else if constexpr (std::same_as<T, std::nullptr_t> || std::same_as<T, std::monostate>)
                out.emplace("type", "null");
            else if constexpr (User && user_caster<T> && !schema_name_of<T>.empty())
            {
                // a field reads by the in-place `from_json`, whose schema then has a name of its own
                const auto i = !writing_ && field && two_sources<T>
                                   ? names_.find_or_add<T, registry::form::caster_field>(writing_, tags)
                                   : names_.find_or_add<T, registry::form::caster>(writing_, tags);
                tags_used_ = tags_used_ || !tags.empty();
                out.emplace("$ref", names_.ref(i, writing_), copy_string);
            }
            else if constexpr (User && user_caster<T>)
                emit_described(out, caster_metadata<T>(field && !writing_), [&] {
                    pass();
                    field_ = field;
                    emit_user<T>(out);
                });
            else if constexpr (variant_type<T>)
            {
                // a variant inside a chain of transparent structs is inline, as the struct that holds it is
                if constexpr (!user_caster<T> && !schema_name_of<T>.empty())
                    if (within.empty())
                    {
                        const auto i = names_.find_or_add<T>(writing_, tags);
                        tags_used_ = tags_used_ || !tags.empty();
                        out.emplace("$ref", names_.ref(i, writing_), copy_string);
                        return;
                    }
                const auto body = [&] {
                    pass();
                    emit_variant<T>(out);
                };
                if constexpr (user_caster<T>)
                    body();
                else
                    emit_described(out, type_metadata<T>(), body);
            }
            else if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T>)
            {
                pass();
                emit_nullable<typename T::value_type>(out);
            }
            else if constexpr (yd::owning_pointer<T>)
            {
                pass();
                emit_nullable<std::remove_cv_t<typename yd::is_owning_pointer<T>::value_type>>(out);
            }
            else if constexpr (yd::indirect_value<T>)
            {
                pass();
                emit<typename yd::is_indirect<T>::value_type>(out);
            }
            else if constexpr (named_type<T>)
                emit_named<T>(out, tags, std::move(within));
            else if constexpr (primitive_type<T>)
            {
                out.emplace("type", type_keyword<T>());
                emit_number_format<T>(out);
                emit_primitive_keywords<T>(out);
            }
            else if constexpr (std::same_as<T, std::vector<bool>>)
            {
                out.emplace("type", "array");
                out.emplace("items", empty_object).emplace("type", "boolean");
            }
            else if constexpr (std::ranges::range<T>)
                emit_range<T>(out, tags);
            else if constexpr (yd::tuple_like<T>)
                emit_tuple<T>(out, std::make_index_sequence<std::tuple_size_v<T>>{}, tags);
            else
                emit_unconverted<T>(out);
            tags_.clear();
            within_.clear();
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

        // The `format` of OpenAPI for the number T: the narrower of int32 and int64 that holds every value, or float
        // and double.  A wider number has none.
        template <typename T>
        consteval std::string_view number_format() noexcept
        {
            using L = std::numeric_limits<T>;
            if constexpr (std::same_as<T, bool> || !std::is_arithmetic_v<T>)
                return {};
            else if constexpr (std::integral<T>)
            {
                if constexpr (L::digits <= 31)
                    return "int32";
                else if constexpr (L::digits <= 63)
                    return "int64";
                else
                    return {};
            }
            else if constexpr (std::same_as<T, float>)
                return "float";
            else if constexpr (std::same_as<T, double>)
                return "double";
            else
                return {};
        }
        inline bool number_format_name(std::string_view name) noexcept
        {
            return name == "int32" || name == "int64" || name == "float" || name == "double";
        }

        template <typename T>
        void generator::emit_number_format(object_ref out) const
        {
            if constexpr (constexpr auto name = number_format<T>(); !name.empty())
                if (number_formats_) out.emplace("format", name);
        }

        template <typename U>
        void generator::emit_nullable(object_ref out)
        {
            if constexpr (primitive_type<U>)
            {
                auto types = out.emplace("type", empty_array);
                types.emplace_back(type_keyword<U>());
                types.emplace_back("null");
                emit_number_format<U>(out);
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
            auto keys = out;
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

        // Whether `key` is the key of a field of S.
        template <typename S>
        bool field_key(std::string_view key)
        {
            return [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                return ((yyjson::detail::struct_field_key<S, Ks> == key) || ...);
            }(std::make_index_sequence<yyjson::detail::struct_field_count<S>>{});
        }

        // The keys of the adjacent tags of S that are written as members of their own.
        template <typename S>
        std::vector<std::string_view> adjacent_keys()
        {
            namespace yd = yyjson::detail;
            auto keys = std::vector<std::string_view>();
            if constexpr (yd::has_adjacent_fields<S>)
                [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    (
                        [&] {
                            if constexpr (yd::writes_adjacent_tag<S, yd::struct_field_tags<S, Ks>>)
                                keys.push_back(yd::struct_field_tags<S, Ks>::key);
                        }(),
                        ...);
                }(std::make_index_sequence<yd::struct_field_count<S>>{});
            return keys;
        }

        // The object of S also holds the members of the internal tags `tags` that no field holds, which the read
        // takes as members that it knows.
        template <typename S>
        void generator::emit_struct(object_ref out, const registry::tag_keys& tags)
        {
            namespace yd = yyjson::detail;
            auto internal = std::vector<std::string_view>();
            for (const auto& key : tags)
                if (!field_key<S>(key)) internal.push_back(key);
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
            if constexpr (yd::has_adjacent_fields<S>) emit_adjacent_tags<S>(out);
            const auto declare_tags = [&] {
                for (const auto key : internal)
                    if (!properties.contains(key))
                    {
                        properties.emplace(key, empty_object, copy_string);
                        tags_used_ = true;
                    }
            };
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
                            emit_pattern_values<S, Ps>(target, internal);
                        }(),
                        ...);
                }(std::make_index_sequence<yd::pattern_count<S>>{});
            }
            // the map of `yyjson::extra_keys` takes the unknown keys under every policy; else the read rejects them as
            // `struct_key_list::closed` decides it, and a read policy has no effect on the write
            if constexpr (yd::has_extra_keys<S>)
            {
                declare_tags();
                emit_map_values<S, yd::extra_keys_index<S>>(out.emplace("additionalProperties", empty_object));
            }
            else if (yd::struct_closed<S> ||
                     (!writing_ && ((policies_ | yd::struct_policies<S>)&yd::unknown_keys_policy) != 0))
            {
                declare_tags();
                out.emplace("additionalProperties", false);
            }
            emit_key_names<S>(out, internal);
            if constexpr (yd::struct_property_counts<S>)
            {
                constexpr const auto& c = yd::struct_count_schema<S>::constraints;
                if constexpr (c.template has<yyjson::min_properties>())
                    tighten(out, "minProperties", c.min_properties, true);
                if constexpr (c.template has<yyjson::max_properties>())
                    tighten(out, "maxProperties", c.max_properties, false);
            }
            if constexpr (yd::has_struct_conditions<S>)
            {
                // a closed node of the conditions takes the members of the tags, as the read does
                for (const auto key : internal) closed_keys_.emplace_back(key, true);
                for (const auto key : adjacent_keys<S>()) closed_keys_.emplace_back(key, false);
                emit_node<yd::node_schema<yd::struct_tree<S>, 0>, S, S>(out);
                closed_keys_.clear();
                if constexpr (yd::struct_tree<S>::tree.nodes[0].properties != 0) emit_struct_values<S>(out);
            }
        }

        // `yyjson::additional_properties` on the struct S checks the members that its `yyjson::property` does not
        // name, the fields too: a conjunct of its own, whose `properties` name only those keys.
        template <typename S>
        void generator::emit_struct_values(object_ref out)
        {
            namespace yd = yyjson::detail;
            using N = yd::node_schema<yd::struct_tree<S>, 0>;
            auto own = conjunct(out);
            auto named = own.emplace("properties", empty_object);
            for (std::size_t k = 0; k < yd::struct_field_count<S>; ++k)
                if (yd::type_property_fields<S>[k])
                    named.emplace(yd::struct_field_keys<S>[k], empty_object, copy_string);
            const auto& nodes = N::tree.nodes;
            for (auto m = N::node.members; m != 0; m = nodes[m].next)
                if (nodes[m].named && !named.contains(nodes[m].key.view()))
                    named.emplace(nodes[m].key.view(), empty_object, copy_string);
            if (named.empty()) own.erase("properties");
            emit_node<typename N::template at<N::node.properties>, yyjson::value, S>(
                own.emplace("additionalProperties", empty_object));
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
        void generator::emit_pattern_values(object_ref out, const std::vector<std::string_view>& tags)
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
            const bool internal_tag = std::ranges::any_of(tags, yd::pattern_matches<S, P>);
            if (!writing_ || !(checks_others() || internal_tag))
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
                tags_used_ = tags_used_ || !checks_others();
                auto any = out.emplace("anyOf", empty_array);
                any.emplace_back(written);
                any.emplace_back(read);
            }
        }

        // The keys of S: its `yyjson::property_names` checks every key, that of the field of `yyjson::extra_keys` the
        // keys that no field names and no pattern matches, and that of a field of `yyjson::pattern_properties` the
        // keys that its pattern matches.
        template <typename S>
        void generator::emit_key_names(object_ref out, const std::vector<std::string_view>& tags)
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
                    const auto adjacent = adjacent_keys<S>();
                    if (yd::struct_field_count<S> != 0 || !tags.empty())
                    {
                        auto keys = any.emplace_back(empty_object).emplace("enum", empty_array);
                        [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                            (keys.emplace_back(yd::struct_field_key<S, Ks>), ...);
                        }(std::make_index_sequence<yd::struct_field_count<S>>{});
                        for (const auto key : adjacent) keys.emplace_back(key);
                        for (const auto key : tags) keys.emplace_back(key, copy_string);
                        tags_used_ = tags_used_ || !tags.empty();
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

        // A direction that the caster does not cover takes the conversion of the library.  The schema of a type
        // with a caster is inline, so that one which holds itself has no schema.
        template <typename T>
        void generator::emit_user(object_ref out)
        {
            namespace yd = yyjson::detail;
            if (std::ranges::find(casters_, &type_identity_tag<T>) != casters_.end())
                throw std::invalid_argument("json_schema: the caster of " + std::string(yd::type_name<T>()) +
                                            " converts a value that holds the type again, which has no name in $defs");
            const auto tags = std::exchange(tags_, {});
            const auto field = std::exchange(field_, false);
            within_.clear();
            casters_.push_back(&type_identity_tag<T>);
            if (writing_)
                emit_caster_write<T>(out, tags);
            else
                emit_caster_read<T>(out, tags, field);
            casters_.pop_back();
            emit_caster_keywords<T>(out, tags);
            // the keywords on the type check the JSON that the caster reads or writes; the library checks its own
            if constexpr (yd::caster_type_keywords<T>)
                if (writing_ ? yd::caster_names_to_json<T> : yd::caster_names_from_json<T>)
                    emit_node<yd::caster_type_schema<T>, yyjson::value, T>(conjunct(out));
        }

        // The source that the library gives to `from_json`, and `{}` for a JSON value.  A field of a struct reads by
        // the in-place form if the caster has one, and any other position by the returning form.
        template <typename T>
        void generator::emit_caster_read(object_ref out, const registry::tag_keys& tags, bool field)
        {
            tags_ = tags;
            if (field)
                emit_caster_source<T, true>(out);
            else
                emit_caster_source<T, false>(out);
            tags_.clear();
        }
        template <typename T, bool Field>
        void generator::emit_caster_source(object_ref out)
        {
            namespace yd = yyjson::detail;
            constexpr auto source = caster_source_of<T, Field>();
            if constexpr (source == caster_source::library)
                emit<T, false>(out);
            else if constexpr (source == caster_source::in_place)
                emit<yd::from_json_in_place_source<T>>(out);
            else if constexpr (source == caster_source::returning)
                emit<yd::from_json_source<T>>(out);
        }

        // What `to_json` returns or the kind of the reference or the emitter that it fills.  `with_caster` whose X
        // has no `to_json` writes its value as the library does.
        template <typename T>
        void generator::emit_caster_write(object_ref out, const registry::tag_keys& tags)
        {
            namespace wd = yyjson::writer::detail;
            tags_ = tags;
            if constexpr (library_written<T>)
                emit<typename T::value_type>(out);
            else if constexpr (!yyjson::detail::caster_names_to_json<T>)
                emit<T, false>(out);
            else if constexpr (wd::to_json_obj_usr<const T&>)
                out.emplace("type", "object");
            else if constexpr (wd::to_json_arr_usr<const T&>)
                out.emplace("type", "array");
            else if constexpr (!wd::to_json_val_usr<const T&>)
                emit<std::remove_cvref_t<decltype(wd::to_json_wrapper(std::declval<const T&>()))>>(out);
            tags_.clear();
        }

        // The keywords of the object of T, which the read checks on the JSON that the caster reads and `validate` on
        // the JSON that it writes.  Without `to_json`, the write reflects T and the keys of its fields pass.
        template <typename T>
        void generator::emit_caster_keywords(object_ref out, const registry::tag_keys& tags)
        {
            namespace yd = yyjson::detail;
            constexpr bool reads = yd::caster_names_from_json<T> && yd::caster_checks_object<T>;
            if constexpr (reads || yd::caster_writes_checked_object<T>)
            {
                if (writing_ ? !yd::caster_checks_written_object<T> : !reads) return;
                constexpr bool closed = yd::struct_closed<T>;
                constexpr bool names = yd::struct_property_names<T>;
                constexpr bool counts = yd::struct_property_counts<T>;
                const auto has = [&](std::initializer_list<std::string_view> keys) {
                    return std::ranges::any_of(keys, [&](std::string_view k) { return out.contains(k); });
                };
                const bool apart = (closed && has({"properties", "patternProperties", "additionalProperties"})) ||
                                   (names && has({"propertyNames"})) ||
                                   (counts && has({"minProperties", "maxProperties"}));
                auto target = apart ? conjunct(out) : out;
                if constexpr (closed)
                {
                    auto keys = std::vector<std::string_view>(tags.begin(), tags.end());
                    tags_used_ = tags_used_ || !tags.empty();
                    if constexpr (!yd::caster_names_to_json<T> && yd::reflectable<T>)
                    {
                        [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                            (keys.emplace_back(yd::struct_field_key<T, Ks>), ...);
                        }(std::make_index_sequence<yd::struct_field_count<T>>{});
                        std::ranges::copy(adjacent_keys<T>(), std::back_inserter(keys));
                    }
                    if constexpr (yd::caster_type_keywords<T>)
                    {
                        const auto& nodes = yd::caster_type_schema<T>::tree.nodes;
                        for (auto m = nodes[0].members; m != 0; m = nodes[m].next)
                            if (nodes[m].named) keys.emplace_back(nodes[m].key.view());
                    }
                    if (!keys.empty())
                    {
                        auto properties = target.emplace("properties", empty_object);
                        for (const auto key : keys) properties.emplace(key, empty_object, copy_string);
                    }
                    target.emplace("additionalProperties", false);
                }
                if constexpr (names)
                    emit_node<yd::struct_names_schema<T>, std::string, T>(
                        target.emplace("propertyNames", empty_object));
                if constexpr (counts)
                {
                    constexpr const auto& c = yd::struct_count_schema<T>::constraints;
                    if constexpr (c.template has<yyjson::min_properties>())
                        tighten(target, "minProperties", c.min_properties, true);
                    if constexpr (c.template has<yyjson::max_properties>())
                        tighten(target, "maxProperties", c.max_properties, false);
                }
            }
        }

        // A type that the library reads by its constructors takes the kinds of JSON that they take.  No write
        // reaches it, because `json_schema` takes only a T that `write` takes.
        template <typename T>
        void generator::emit_unconverted(object_ref out)
        {
            namespace yd = yyjson::detail;
            if (writing_) return;
            constexpr auto kinds = yd::accepted_json_kinds<T, reader::const_value_ref>();
            if constexpr (kinds == 0)
                out.emplace("not", empty_object);
            else if constexpr ((kinds & yd::json_kind_all) != yd::json_kind_all)
            {
                auto types = std::vector<std::string_view>();
                if constexpr ((kinds & yd::json_kind_null) != 0) types.emplace_back("null");
                if constexpr ((kinds & yd::json_kind_bool) != 0) types.emplace_back("boolean");
                constexpr bool real = (kinds & yd::json_kind_real) != 0;
                constexpr auto integers = kinds & (yd::json_kind_uint | yd::json_kind_sint);
                if constexpr (real || integers != 0) types.emplace_back(real ? "number" : "integer");
                if constexpr ((kinds & yd::json_kind_string) != 0) types.emplace_back("string");
                if constexpr ((kinds & yd::json_kind_array) != 0) types.emplace_back("array");
                if constexpr ((kinds & yd::json_kind_object) != 0) types.emplace_back("object");
                if (types.size() == 1)
                    out.emplace("type", types.front());
                else
                {
                    auto array = out.emplace("type", empty_array);
                    for (const auto t : types) array.emplace_back(t);
                }
                if constexpr (real)
                    emit_primitive_keywords<double>(out);
                else if constexpr (integers != 0)
                {
                    if constexpr ((kinds & yd::json_kind_sint) != 0)
                        out.emplace("minimum", std::numeric_limits<std::int64_t>::lowest());
                    else
                        out.emplace("minimum", 0);
                    if constexpr ((kinds & yd::json_kind_uint) != 0)
                        out.emplace("maximum", std::numeric_limits<std::uint64_t>::max());
                    else
                        out.emplace("maximum", -1);
                }
            }
        }

        // Whether the read schema requires the key of the field K of S: the field declares no default.  The policies
        // do not change it, so that every read schema of S tells one contract.
        template <typename S, std::size_t K>
        [[nodiscard]] constexpr bool read_requires() noexcept
        {
            return !yyjson::detail::struct_field_absence<S>[K];
        }
        // Whether every write of S gives the key of its field K: a field that `yyjson::adjacent_tag` tags, or a field
        // not omitted when empty, nor when equal to its default unless the write takes `yyjson::emit_defaults`, nor
        // when its default is a sentinel.
        template <typename S, std::size_t K>
        [[nodiscard]] bool write_requires(bool emit_defaults)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            constexpr auto r = yd::struct_declared_index<S, K>;
            if constexpr (yd::adjacent_declared_field<S, r>)
                return true;
            else if constexpr (yd::omittable<decltype(yd::writable_field(std::declval<const F&>()))>)
                return false;
            else if constexpr (yd::field_has_default<S, r>)
                return (emit_defaults || !yd::omits_default<S, r>) && !yd::sentinel_default<S, r>();
            else
                return true;
        }

        // Whether the read schema takes the written value `v` where it is narrower than the read: a tuple of pairs
        // whose values differ in type.
        template <typename T, bool User = true>
        bool read_schema_takes(const T& v);
        // Whether the read schema of a field of a struct takes `v`, which a caster reads by its in-place `from_json`.
        template <typename F>
        bool field_read_takes(const F& v);
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
                catch (const yyjson::bad_cast&)
                {
                    return false;
                }
                catch (const yyjson::read_error&)
                {
                    return false;
                }
                catch (const yyjson::write_error&)
                {
                    return false;
                }
                catch (const std::invalid_argument&)
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
        // Whether the read schema of S takes `json`, as a read of S gives it.
        template <typename S, bool User = true>
        bool read_back_takes(const yyjson::value& json)
        {
            if constexpr (!requires { json.template cast<S>(); })
                return true;
            else
            {
                try
                {
                    return read_schema_takes<S, User>(
                        json.template cast<S>(yyjson::reject_missing_keys, yyjson::reject_null_as_missing));
                }
                catch (const yyjson::bad_cast&)
                {
                    return false;
                }
                catch (const yyjson::read_error&)
                {
                    return false;
                }
                catch (const yyjson::write_error&)
                {
                    return false;
                }
                catch (const std::invalid_argument&)
                {
                    return false;
                }
            }
        }
        // The read schema of a type with a caster is that of the value that the caster reads from the written JSON.
        template <typename T, bool Field>
        bool caster_read_takes(const T& v)
        {
            namespace yd = yyjson::detail;
            if constexpr (!requires { yyjson::value(v); })
                return true;
            else
            {
                const auto json = yyjson::value(v);
                constexpr auto source = caster_source_of<T, Field>();
                if constexpr (source == caster_source::library)
                    return read_back_takes<T, false>(json);
                else if constexpr (source == caster_source::in_place)
                    return read_back_takes<yd::from_json_in_place_source<T>>(json);
                else if constexpr (source == caster_source::returning)
                    return read_back_takes<yd::from_json_source<T>>(json);
                else
                    return true;
            }
        }
        template <typename T, bool User>
        bool read_schema_takes(const T& v)
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::is_std_optional<T>::value || yd::nullable_value<T> || yd::owning_pointer<T>)
                return !v || read_schema_takes(*v);
            else if constexpr (yd::indirect_value<T>)
                return v.valueless_after_move() || read_schema_takes(*v);
            else if constexpr (User && user_caster<T>)
                return caster_read_takes<T, false>(v);
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
                    return (field_read_takes(yd::struct_field<Ks>(v)) && ...);
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
        template <typename F>
        bool field_read_takes(const F& v)
        {
            if constexpr (user_caster<F>)
                return caster_read_takes<F, true>(v);
            else
                return read_schema_takes(v);
        }

        // The JSON text that `write` with `yyjson::emit_defaults` gives for the default `d` of a field of F, which
        // every schema of F takes, if the field is written and the read of F under every policy takes it back.  An
        // alternative that an adjacent tag names is not a field.
        template <typename F, bool Field, typename D>
        std::optional<std::string> default_text(const D& d)
        {
            namespace yd = yyjson::detail;
            decltype(auto) field = yd::writable_field(d);
            if constexpr (yd::omittable<decltype(field)>)
                if (yd::should_omit_field(field)) return std::nullopt;
            if constexpr (!readable<typename yd::read_back<std::remove_cv_t<F>>::type> || !writable<decltype(field)>)
                return std::nullopt;
            else
            {
                auto text = yd::round_trip_text<F>(d);
                if constexpr (Field)
                {
                    if (text && !field_read_takes(field)) text.reset();
                }
                else if (text && !read_schema_takes(field))
                    text.reset();
                return text;
            }
        }

        // The default text of the field K of S, which writes the alternative alone when an adjacent tag names it.
        template <typename S, std::size_t K, typename D>
        std::optional<std::string> adjacent_default_text(const D& d)
        {
            namespace yd = yyjson::detail;
            using F = yd::struct_field_type<S, K>;
            if constexpr (yd::adjacent_field<S, K> && internal_tagged_variant<F>)
                return std::visit(
                    [](const auto& a) { return default_text<std::remove_cvref_t<decltype(a)>, false>(a); },
                    static_cast<const yd::variant_base_t<F>&>(d));
            else
                return default_text<F, true>(d);
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
                if (!yd::sentinel_default<S, r>()) it->second = adjacent_default_text<S, K>(yd::stored_default<S, r>());
            }
            else if (const auto* s = initial_value<S>(names); s != nullptr)
            {
                it->second = adjacent_default_text<S, K>(yd::struct_field<K>(*s));
                if (it->second)
                {
                    try
                    {
                        const auto scope = yd::emit_defaults_scope();
                        yd::validate_object_field<false, S, K>(*s, WriteFlag::NoFlag);
                    }
                    catch (const yyjson::bad_cast&)
                    {
                        it->second.reset();
                    }
                    catch (const yyjson::read_error&)
                    {
                        it->second.reset();
                    }
                    catch (const yyjson::write_error&)
                    {
                        it->second.reset();
                    }
                    catch (const std::invalid_argument&)
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
            const auto m = field_metadata<S, K>();
            const auto declared = emit_annotations(out, m);
            if (!declared)
            {
                // an adjacent tag names the alternative, which then reads and writes without the tag of its variant
                if constexpr (yd::adjacent_field<S, K> && internal_tagged_variant<F>)
                    emit_untagged<F>(out);
                else
                    emit_member<F>(out);
                if constexpr (yd::has_constraints<S>)
                    emit_node<yd::field_schema<S, yd::struct_declared_index<S, K>>, F, S>(out);
            }
            // a declared `yyjson::schema` replaces the whole derived schema, its `default` too
            if (!declared)
                if (const auto& d = field_default<S, K>(names_)) out.emplace("default", yyjson::read(*d));
            emit_extension(out, m);
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
                field_ = true;
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
                field_ = true;
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
        void generator::emit_range(object_ref out, const registry::tag_keys& tags)
        {
            namespace yd = yyjson::detail;
            using pair = std::remove_cv_t<std::ranges::range_value_t<const T>>;
            if (writing_)
            {
                if constexpr (yd::object_written_range<T>)
                    emit_object_form<T, std::remove_cv_t<std::tuple_element_t<0, pair>>,
                                     std::remove_cv_t<std::tuple_element_t<1, pair>>>(out, tags);
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
            if constexpr (yyjson::detail::json_unique_set<T>) out.emplace("uniqueItems", true);
        }

        // A range of a fixed size is written as an object of a member for each of its pairs, and read from an
        // object of as many members.  A write puts the members of the internal tags `tags` beside the pairs, but a
        // read takes such a member as a pair.
        template <typename T, typename K, typename V>
        void generator::emit_object_form(object_ref out, const registry::tag_keys& tags)
        {
            namespace yd = yyjson::detail;
            out.emplace("type", "object");
            if constexpr (yd::c_array<T> || yd::std_array<T>)
            {
                out.emplace("minProperties", fixed_size<T>());
                out.emplace("maxProperties", fixed_size<T>() + tags.size());
            }
            declare_tag_keys(out, tags);
            emit<V>(out.emplace("additionalProperties", empty_object));
            if constexpr (yd::enum_key<K>)
            {
                auto names = out.emplace("propertyNames", empty_object);
                if (tags.empty())
                    emit_property_names<K>(names);
                else
                {
                    auto any = names.emplace("anyOf", empty_array);
                    auto keys = any.emplace_back(empty_object).emplace("enum", empty_array);
                    for (const auto& key : tags) keys.emplace_back(key, copy_string);
                    emit_property_names<K>(any.emplace_back(empty_object));
                }
            }
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
                return []<typename... Us>(std::type_identity<std::variant<Us...>>) {
                    constexpr bool bare =
                        ((primitive_type<Us> || std::same_as<Us, std::monostate> || std::same_as<Us, std::nullptr_t>) &&
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
                        const auto keywords = std::array{keyword(std::type_identity<Us>())...};
                        for (std::size_t i = 0; i < keywords.size(); ++i)
                            for (std::size_t j = 0; j < i; ++j)
                                if (keywords[i] == keywords[j]) return false;
                        constexpr auto numbers = ((std::integral<Us> && !std::same_as<Us, bool> ? 1 : 0) + ... + 0) +
                                                 ((std::floating_point<Us> ? 1 : 0) + ... + 0);
                        return numbers <= 1;
                    }
                }(std::type_identity<std::variant<std::remove_cv_t<Ts>...>>{});
            }(std::type_identity<yyjson::detail::variant_base_t<V>>{});
        }

        template <typename V, std::size_t I, bool Described>
        void generator::emit_alternative(object_ref out)
        {
            namespace yd = yyjson::detail;
            using T = std::variant_alternative_t<I, yd::variant_base_t<V>>;
            if constexpr (Described && yd::union_variant<V>)
                if constexpr (described_alternative<typename yd::union_traits<V>::template alternative<I>>)
                {
                    auto tags = std::exchange(tags_, {});
                    auto within = std::exchange(within_, {});
                    emit_described(out, alternative_metadata<V, I>::get(), [&] {
                        tags_ = std::move(tags);
                        within_ = std::move(within);
                        emit_alternative<V, I, false>(out);
                    });
                    return;
                }
            if constexpr (yd::constrained_union_alternative<V, I>)
            {
                auto closed = std::vector<std::pair<std::string, bool>>();
                for (const auto& key : tags_) closed.emplace_back(key, true);
                emit<T>(out);
                closed_keys_ = std::move(closed);
                emit_node<yd::union_alternative_schema<V, I>, T, typename yd::union_traits<V>::template alternative<I>>(
                    out);
                closed_keys_.clear();
            }
            else
                emit<T>(out);
        }

        // A variant without a tag reads a value that an alternative reads, and a `yyjson::one_of_variant` one that
        // exactly one reads.  `validate` rejects a value whose JSON another alternative reads, so that `oneOf` holds
        // in the write schema too.
        template <typename V>
        void generator::emit_variant(object_ref out)
        {
            namespace yd = yyjson::detail;
            if constexpr (!yd::union_variant<V> && yd::internally_tagged<V>)
                emit_internal_tag<V>(out, std::exchange(tags_, {}));
            else
                emit_untagged<V>(out);
        }

        // An internally tagged variant reads null as its `std::monostate`, and else an object with the member of the
        // tag beside those of the alternative that the tag names.  Exactly one alternative takes a value, so that
        // `oneOf` holds in both directions.
        template <typename V>
        void generator::emit_internal_tag(object_ref out, registry::tag_keys tags)
        {
            namespace yd = yyjson::detail;
            using B = yd::variant_base_t<V>;
            using Tags = yd::variant_tags<V>;
            constexpr auto size = std::variant_size_v<B>;
            const bool inherited = !tags.empty();
            const bool used = std::exchange(tags_used_, false);
            const auto within = std::exchange(within_, {});
            // a set, so that a recursive reach of a struct finds the entry of the registry that it is in
            if (const auto at = std::ranges::lower_bound(tags, Tags::key); at == tags.end() || *at != Tags::key)
                tags.emplace(at, Tags::key);
            const auto alternative = [&]<std::size_t I>(std::integral_constant<std::size_t, I>, object_ref o) {
                if constexpr (I == Tags::monostate)
                    o.emplace("type", "null");
                else
                {
                    tags_ = tags;
                    within_ = within;
                    emit_alternative<V, I>(o);
                    tags_.clear();
                    within_.clear();
                    // a reference alone takes keywords beside it, which then apply to the same value; the schema of a
                    // struct of members says "object" already, and any other alternative reads only an object here
                    using T = std::variant_alternative_t<I, B>;
                    const bool bare = o.size() == 1 && o.contains("$ref");
                    auto target = bare ? o : conjunct(o);
                    if (!(struct_type<T> && !user_caster<T> && !yd::as_array_struct<T> && !yd::transparent_struct<T>))
                        target.emplace("type", "object");
                    target.emplace("properties", empty_object)
                        .emplace(Tags::key, empty_object)
                        .emplace("const", Tags::ids[I]);
                    target.emplace("required", empty_array).emplace_back(Tags::key);
                }
            };
            if constexpr (size == 1)
                alternative(std::integral_constant<std::size_t, 0>(), out);
            else
            {
                auto each = out.emplace("oneOf", empty_array);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (alternative(std::integral_constant<std::size_t, Is>(), each.emplace_back(empty_object)), ...);
                }(std::make_index_sequence<size>{});
            }
            // the keys of the outer tags changed the schema if a schema of an alternative changed
            tags_used_ = used || (inherited && tags_used_);
        }

        template <typename V>
        void generator::emit_untagged(object_ref out)
        {
            namespace yd = yyjson::detail;
            const auto tags = std::exchange(tags_, {});
            const auto within = std::exchange(within_, {});
            using B = yd::variant_base_t<V>;
            constexpr auto size = std::variant_size_v<B>;
            constexpr bool constrained = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return (yd::constrained_union_alternative<V, Is> || ...);
            }(std::make_index_sequence<size>{});
            if constexpr (size == 1)
            {
                tags_ = tags;
                within_ = within;
                emit_alternative<V, 0>(out);
            }
            else if constexpr (!yd::is_one_of_variant<V> && !constrained && !described_alternatives<V>() &&
                               bare_alternatives<V>())
            {
                auto types = out.emplace("type", empty_array);
                [&]<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
                    (
                        [&] {
                            using U = std::remove_cv_t<Ts>;
                            if constexpr (primitive_type<U>)
                            {
                                types.emplace_back(type_keyword<U>());
                                emit_number_format<U>(out);
                                emit_primitive_keywords<U>(out);
                            }
                            else
                                types.emplace_back("null");
                        }(),
                        ...);
                }(std::type_identity<B>{});
            }
            else
            {
                const auto keyword = yd::is_one_of_variant<V> ? "oneOf" : "anyOf";
                auto each = out.emplace(keyword, empty_array);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (emit_untagged_alternative<V, Is>(each.emplace_back(empty_object), tags, within), ...);
                }(std::make_index_sequence<size>{});
            }
        }

        // Whether `validate` may find that the walk of the alternative I of the untagged variant V fails, and then
        // passes the value if read gives another alternative from its JSON.
        template <typename V, std::size_t I>
        consteval bool walk_may_fail()
        {
            namespace yd = yyjson::detail;
            if constexpr (yd::constrained_union_alternative<V, I> ||
                          yd::needs_validation<std::variant_alternative_t<I, yd::variant_base_t<V>>>)
                return true;
            else if constexpr (yd::is_one_of_variant<V>)
                return yd::overlaps_another(yd::one_of_variant_cast<V>::overlap, I) ||
                       yd::overlaps_another(yd::one_of_variant_cast<V>::tagged_overlap, I);
            else
                return false;
        }

        // The write schema of the alternative I also takes what its read schema takes, when `validate` may pass the
        // value of another alternative because read gives I from its JSON.  A `yyjson::one_of_variant` branch takes
        // only what reads as I, so that a value of I that another branch also describes matches one branch.
        template <typename V, std::size_t I>
        void generator::emit_untagged_alternative(object_ref out, const registry::tag_keys& tags,
                                                  const std::vector<const void*>& within)
        {
            constexpr bool read_back = []<std::size_t... Js>(std::index_sequence<Js...>) {
                return ((Js != I && walk_may_fail<V, Js>()) || ...);
            }(std::make_index_sequence<std::variant_size_v<yyjson::detail::variant_base_t<V>>>{});
            if constexpr (read_back || yyjson::detail::is_one_of_variant<V>)
            {
                if (writing_)
                {
                    auto written = value(object());
                    tags_ = tags;
                    within_ = within;
                    emit_alternative<V, I>(*written.as_object());
                    auto read = value(object());
                    writing_ = false;
                    tags_ = tags;
                    within_ = within;
                    emit_alternative<V, I>(*read.as_object());
                    writing_ = true;
                    if (written.write() != read.write())
                    {
                        if constexpr (!yyjson::detail::is_one_of_variant<V>)
                        {
                            auto any = out.emplace("anyOf", empty_array);
                            any.emplace_back(written);
                            any.emplace_back(read);
                        }
                        else if constexpr (read_back)
                        {
                            const auto members = *read.as_object();
                            for (const auto& [key, v] : members) out.emplace(std::string(key), v, copy_string);
                        }
                        else
                        {
                            auto all = out.emplace("allOf", empty_array);
                            all.emplace_back(written);
                            all.emplace_back(read);
                        }
                        return;
                    }
                }
            }
            tags_ = tags;
            within_ = within;
            emit_alternative<V, I>(out);
        }

        // The examples are JSON texts that read as the value does, and the write schema gives each as written.
        inline bool generator::emit_annotations(object_ref out, const metadata& m)
        {
            const auto add = [&](std::string_view key, const auto& v) {
                if (!out.contains(key)) out.emplace(key, v, copy_string);
            };
            if (m.title) add("title", *m.title);
            if (m.description) add("description", *m.description);
            if (m.deprecated) add("deprecated", true);
            if (m.read_only) add("readOnly", true);
            if (m.write_only) add("writeOnly", true);
            if (!m.examples.empty() && !out.contains("examples"))
            {
                auto list = out.emplace("examples", empty_array);
                for (const auto text : m.examples)
                {
                    const auto rejected = [&](const std::exception& e) {
                        return std::invalid_argument("json_schema: the example " + std::string(text) + " of " +
                                                     m.owner + " is not read: " + e.what());
                    };
                    // only the errors of the library: an exception of a caster of the user passes unchanged
                    try
                    {
                        const auto written = m.read(text, read_policies_);
                        list.emplace_back(yyjson::read(writing_ ? std::string_view(written) : text));
                    }
                    catch (const read_error& e)
                    {
                        throw rejected(e);
                    }
                    catch (const bad_cast& e)
                    {
                        throw rejected(e);
                    }
                    catch (const write_error& e)
                    {
                        throw rejected(e);
                    }
                    catch (const std::invalid_argument& e)
                    {
                        throw rejected(e);
                    }
                }
            }
            if (m.declared == nullptr || !m.declared->has(writing_)) return false;
            const auto declared = [&] {
                try
                {
                    return yyjson::read(m.declared->text(writing_));
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (const std::exception& e)
                {
                    throw std::invalid_argument("json_schema: the yyjson::schema of " + m.owner +
                                                " is not JSON: " + e.what());
                }
            }();
            if (const auto b = declared.as_bool())
            {
                if (!*b) add("not", empty_object);
            }
            else if (const auto o = declared.as_object())
            {
                for (const auto& [key, v] : *o) add(key, v);
            }
            else
                throw std::invalid_argument("json_schema: the yyjson::schema of " + m.owner +
                                            " is not an object or a boolean");
            return true;
        }

        // A member of `yyjson::schema_extend` replaces the keyword of the same key.
        inline void generator::emit_extension(object_ref out, const metadata& m)
        {
            if (!m.extend) return;
            const auto extension = [&] {
                try
                {
                    return yyjson::read(*m.extend);
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (const std::exception& e)
                {
                    throw std::invalid_argument("json_schema: the yyjson::schema_extend of " + m.owner +
                                                " is not JSON: " + e.what());
                }
            }();
            const auto o = extension.as_object();
            if (!o)
                throw std::invalid_argument("json_schema: the yyjson::schema_extend of " + m.owner +
                                            " is not an object");
            for (const auto& [key, v] : *o)
            {
                out.erase(key);
                out.emplace(key, v, copy_string);
            }
        }

        // The members of the internal tags `tags` take any value beside `additionalProperties`.
        inline void generator::declare_tag_keys(object_ref out, const registry::tag_keys& tags)
        {
            if (tags.empty()) return;
            auto properties = out.emplace("properties", empty_object);
            for (const auto& key : tags) properties.emplace(key, empty_object, copy_string);
            tags_used_ = true;
        }

        // A write puts the members of the internal tags `tags` beside the pairs, but a read takes such a member as
        // a pair.
        template <typename T, std::size_t... Is>
        void generator::emit_pair_object(object_ref out, const registry::tag_keys& tags)
        {
            out.emplace("type", "object");
            if (!writing_)
            {
                out.emplace("minProperties", sizeof...(Is));
                out.emplace("maxProperties", sizeof...(Is));
            }
            else
                declare_tag_keys(out, tags);
            emit_values<std::remove_cv_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>...>(
                out.emplace("additionalProperties", empty_object), writing_ ? "anyOf" : "allOf");
        }

        // A tuple of key-value pairs with string keys is written as an object of a member for each pair, and read
        // from such an object or from an array of the pairs.  JSON Schema cannot tie a member to its position, so a
        // member that is read must satisfy the schemas of the values of all the pairs.
        template <typename T, std::size_t... Is>
        void generator::emit_tuple(object_ref out, std::index_sequence<Is...>, const registry::tag_keys& tags)
        {
            constexpr auto size = sizeof...(Is);
            if constexpr (size == 0)
            {
                // written as an empty object, and read from an empty array as well
                if (writing_)
                {
                    out.emplace("type", "object");
                    declare_tag_keys(out, tags);
                    out.emplace("maxProperties", tags.size());
                }
                else
                {
                    auto types = out.emplace("type", empty_array);
                    types.emplace_back("array");
                    types.emplace_back("object");
                    out.emplace("maxItems", 0);
                    out.emplace("maxProperties", 0);
                }
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
                        emit_pair_object<T, Is...>(out, tags);
                    else
                        array_form(out);
                }
                else if constexpr (object_read && array_read)
                {
                    auto any = out.emplace("anyOf", empty_array);
                    emit_pair_object<T, Is...>(any.emplace_back(empty_object), {});
                    array_form(any.emplace_back(empty_object));
                }
                else if constexpr (object_read)
                    emit_pair_object<T, Is...>(out, {});
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
                // the values of a caster are compared as the JSON that it reads and writes
                if constexpr (!yd::dom_checked<C> && !user_caster<C> &&
                              requires { yyjson::read<R>(std::string_view()); })
                {
                    if (writing_ && N::values[i].kind != yd::json_scalar::kind_t::null)
                    {
                        try
                        {
                            auto written = std::string();
                            yyjson::write(yyjson::read<R>(text), written);
                            text = std::move(written);
                        }
                        catch (const yyjson::bad_cast&)
                        {
                        }
                        catch (const yyjson::read_error&)
                        {
                        }
                        catch (const yyjson::write_error&)
                        {
                        }
                        catch (const std::invalid_argument&)
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

        template <typename N, typename C>
        void generator::emit_bounds(object_ref out)
        {
            constexpr const auto& c = N::constraints;
            if constexpr (c.template has<yyjson::minimum>()) tighten(out, "minimum", c.minimum, true);
            if constexpr (c.template has<yyjson::exclusive_minimum>())
                tighten(out, "exclusiveMinimum", c.exclusive_minimum, true);
            if constexpr (c.template has<yyjson::maximum>()) tighten(out, "maximum", c.maximum, false);
            if constexpr (c.template has<yyjson::exclusive_maximum>())
                tighten(out, "exclusiveMaximum", c.exclusive_maximum, false);
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

        // The `if` and the `then` of a new condition of `out`.
        inline std::pair<object_ref, object_ref> condition(object_ref out)
        {
            auto holder = place(out, "if");
            auto cond = holder.emplace("if", empty_object);
            return {cond, holder.emplace("then", empty_object)};
        }

        template <typename S>
        void generator::emit_adjacent_tags(object_ref out)
        {
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (
                    [&] {
                        if constexpr (yyjson::detail::adjacent_field<S, Ks>) emit_adjacent_tag<S, Ks>(out);
                    }(),
                    ...);
            }(std::make_index_sequence<yyjson::detail::struct_field_count<S>>{});
        }

        // The tag of the variant of the field K names its alternative, also when a field of S holds it.  A missing tag
        // reads only null, as `std::monostate`, and a missing value keeps its default, which the tag must name.  A
        // write gives no tag for `std::monostate`, and none beside a default value that it leaves out.
        template <typename S, std::size_t K>
        void generator::emit_adjacent_tag(object_ref out)
        {
            namespace yd = yyjson::detail;
            using Tags = yd::struct_field_tags<S, K>;
            using F = yd::struct_field_type<S, K>;
            constexpr auto key = Tags::key;
            constexpr auto value = yd::struct_field_key<S, K>;
            constexpr bool own = yd::writes_adjacent_tag<S, Tags>;
            constexpr bool monostate = Tags::monostate != std::variant_npos;
            constexpr auto size = std::variant_size_v<F>;
            const bool required = writing_ ? write_requires<S, K>(emit_defaults_) : read_requires<S, K>();
            auto properties = [&] {
                auto v = out.find("properties")->second;
                return *v.as_object();
            }();
            const auto names = [](object_ref o) {
                auto values = o.emplace("enum", empty_array);
                for (std::size_t i = 0; i < size; ++i)
                    if (i != Tags::monostate) values.emplace_back(Tags::ids[i]);
            };
            if constexpr (own)
                names(properties.emplace(key, empty_object));
            else
            {
                // `S()` may hold a tag that names no alternative, which no read keeps unless the variant is missing
                auto field = sub(properties, key);
                if (auto it = field.find("default"); it != field.end())
                {
                    const auto d = it->second.as_string();
                    if (!d || Tags::alternative(*d) == std::variant_npos) field.erase("default");
                }
                if (!monostate || !writing_) names(place(field, "enum"));
            }
            // a write leaves the field of the tag unchecked while the variant has no value
            const bool unchecked = !own && monostate && writing_;
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (
                    [&] {
                        if constexpr (Is != Tags::monostate)
                        {
                            auto [tagged, then] = condition(out);
                            tagged.emplace("properties", empty_object)
                                .emplace(key, empty_object)
                                .emplace("const", Tags::ids[Is]);
                            tagged.emplace("required", empty_array).emplace_back(key);
                            auto at = then.emplace("properties", empty_object).emplace(value, empty_object);
                            using T = std::variant_alternative_t<Is, F>;
                            if (!unchecked)
                                emit<T>(at);
                            else
                            {
                                auto any = at.emplace("anyOf", empty_array);
                                emit<T>(any.emplace_back(empty_object));
                                any.emplace_back(empty_object).emplace("type", "null");
                            }
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<size>{});
            // a write checks a field of the tag only against the variant, and may leave it out as any other field
            if constexpr (!own)
                if (writing_) return;
            const auto depend = [&](std::string_view from, std::string_view to) {
                auto all = sub(out, "dependentRequired");
                all.emplace(from, empty_array).emplace_back(to);
            };
            if constexpr (monostate)
            {
                auto [valued, then] = condition(out);
                valued.emplace("required", empty_array).emplace_back(value);
                valued.emplace("properties", empty_object)
                    .emplace(value, empty_object)
                    .emplace("not", empty_object)
                    .emplace("type", "null");
                then.emplace("required", empty_array).emplace_back(key);
            }
            else if (required)
                add_required(out, key);
            else
                depend(value, key);
            if (required) return;
            const auto initial = [&]() -> std::size_t {
                constexpr auto r = yd::struct_declared_index<S, K>;
                if constexpr (yd::presence_of<S, r>.from.has_value())
                    return yd::stored_default<S, r>().index();
                else if (const auto* s = initial_value<S>(names_); s != nullptr)
                    return yd::struct_field<K>(*s).index();
                return std::variant_npos;
            }();
            if (writing_ || initial == Tags::monostate)
                depend(key, value);
            else if (initial != std::variant_npos)
            {
                auto [missing, then] = condition(out);
                missing.emplace("required", empty_array).emplace_back(key);
                missing.emplace("not", empty_object).emplace("required", empty_array).emplace_back(value);
                then.emplace("properties", empty_object)
                    .emplace(key, empty_object)
                    .emplace("const", Tags::ids[initial]);
            }
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
            // a closed node at a value of an internally tagged variant takes its tag, unless it sees other keys
            if constexpr (internal_tagged_variant<C>)
                if (closed_keys_.empty())
                {
                    closed_keys_.emplace_back(yd::variant_tags<C>::key, false);
                    emit_node<N, V, S>(out);
                    closed_keys_.clear();
                    return;
                }
            if constexpr (yd::variant_value<C> && !empty_node(n))
                if (emit_projections<N, C, S>(out)) return;
            // only the nodes of the same value see the keys that a closed node takes
            const auto closed = std::exchange(closed_keys_, {});
            if constexpr (c.template has<yyjson::matches_t<void>>())
                emit_matched<N, S>(out.empty() ? out : conjunct(out));
            if constexpr (n.values_count != 0) emit_listed<N, C>(out);
            if constexpr ((c.bits & yd::numeric_constraints) != 0) emit_bounds<N, C>(out);
            if constexpr (c.template has<yyjson::min_length>()) tighten(out, "minLength", c.min_length, true);
            if constexpr (c.template has<yyjson::max_length>()) tighten(out, "maxLength", c.max_length, false);
            for (std::size_t i = 0; i < N::patterns.size(); ++i)
                (i == 0 ? place(out, "pattern") : conjunct(out)).emplace("pattern", N::patterns[i], copy_string);
            if constexpr (c.template has<yyjson::format<>>() || c.template has<yyjson::format_assertion<>>())
            {
                // a format of the node replaces the one of OpenAPI that the number has
                if (auto it = out.find("format"); number_formats_ && it != out.end())
                    if (const auto name = it->second.as_string(); name && number_format_name(*name))
                        out.erase("format");
            }
            if constexpr (c.template has<yyjson::format<>>())
                place(out, "format").emplace("format", c.format_name.view(), copy_string);
            if constexpr (c.template has<yyjson::format_assertion<>>())
                place(out, "format").emplace("format", c.assertion_name.view(), copy_string);
            constexpr auto array_bits = yd::constraint_bit<yyjson::min_items> | yd::constraint_bit<yyjson::max_items> |
                                        yd::constraint_bit<yyjson::unique_items_t>;
            if constexpr ((c.bits & array_bits) != 0 || n.items != 0 || n.prefix_count != 0 || n.contains != 0 ||
                          n.items_denied)
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
                if constexpr (n.closed)
                {
                    for (const auto& [key, internal] : closed)
                    {
                        auto properties = sub(holder, "properties");
                        if (properties.contains(key)) continue;
                        properties.emplace(key, empty_object, copy_string);
                        tags_used_ = tags_used_ || internal;
                    }
                    holder.emplace("additionalProperties", false);
                }
            }
            closed_keys_ = closed;
            if constexpr (n.combinators != 0) emit_combinators<N, n.combinators, V, S>(out);
            closed_keys_ = closed;
        }

        // The JSON types that a read of A takes, or none for all.
        template <typename A>
        std::vector<std::string_view> json_types()
        {
            namespace yd = yyjson::detail;
            constexpr auto kinds = yd::accepted_json_kinds<A, reader::const_value_ref>();
            auto types = std::vector<std::string_view>();
            if constexpr ((kinds & yd::json_kind_all) != yd::json_kind_all)
            {
                if constexpr ((kinds & yd::json_kind_null) != 0) types.emplace_back("null");
                if constexpr ((kinds & yd::json_kind_bool) != 0) types.emplace_back("boolean");
                if constexpr ((kinds & yd::json_kind_number) != 0) types.emplace_back("number");
                if constexpr ((kinds & yd::json_kind_string) != 0) types.emplace_back("string");
                if constexpr ((kinds & yd::json_kind_array) != 0) types.emplace_back("array");
                if constexpr ((kinds & yd::json_kind_object) != 0) types.emplace_back("object");
            }
            return types;
        }

        // The keywords of a node at a variant V check the alternative that it holds.  When they differ between the
        // alternatives, each applies to the JSON types of its alternative, and also to a value that another one reads.
        template <typename N, typename V, typename S>
        bool generator::emit_projections(object_ref out)
        {
            namespace yd = yyjson::detail;
            struct projection
            {
                std::vector<std::string_view> types;
                value schema;
                std::string text;
            };
            const auto closed = closed_keys_;
            auto each = std::vector<projection>();
            [&]<typename... As>(std::type_identity<std::variant<As...>>) {
                (
                    [&] {
                        auto o = value(object());
                        closed_keys_ = closed;
                        emit_node<N, As, S>(*o.as_object());
                        auto text = o.write();
                        each.push_back({json_types<yd::constrained_value_t<std::remove_cv_t<As>>>(), std::move(o),
                                        std::move(text)});
                    }(),
                    ...);
            }(std::type_identity<yd::variant_base_t<V>>{});
            closed_keys_ = closed;
            if (std::ranges::all_of(each, [&](const projection& p) { return p.text == each.front().text; }))
                return false;
            for (std::size_t i = 0; i < each.size(); ++i)
            {
                const auto& p = each[i];
                const auto repeated = [&] {
                    for (std::size_t j = 0; j < i; ++j)
                        if (each[j].text == p.text && each[j].types == p.types) return true;
                    return false;
                };
                if (p.text == "{}" || repeated()) continue;
                auto all = [&] {
                    if (auto it = out.find("allOf"); it != out.end())
                    {
                        auto v = it->second;
                        return *v.as_array();
                    }
                    return out.emplace("allOf", empty_array);
                }();
                if (p.types.empty())
                    all.emplace_back(p.schema);
                else
                {
                    auto conditional = all.emplace_back(empty_object);
                    auto condition = conditional.emplace("if", empty_object);
                    if (p.types.size() == 1)
                        condition.emplace("type", p.types.front());
                    else
                    {
                        auto types = condition.emplace("type", empty_array);
                        for (const auto t : p.types) types.emplace_back(t);
                    }
                    conditional.emplace("then", p.schema);
                }
            }
            return true;
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
            if constexpr (n.prefix_count != 0 || n.items != 0 || n.items_denied)
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
                        else if constexpr (n.items_denied)
                            target.emplace("items", false);
                    }
                    else if constexpr (n.items != 0)
                        emit_node<typename N::template at<n.items>, typename P::element, S>(sub(out, "items"));
                    else
                        (out.contains("items") || out.contains("prefixItems") ? conjunct(out) : out)
                            .emplace("items", false);
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
            // the root of the conditions of a struct beside the `properties` of its fields, which `emit_struct_values`
            // gives
            if constexpr (n.properties != 0 && !std::same_as<N, yd::node_schema<yd::struct_tree<S>, 0>>)
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
        requires (yyjson::detail::object_struct_value<C>() && K < yyjson::detail::struct_field_count<C>)
        struct member_type<C, K>
        {
            using type = yyjson::detail::struct_field_type<C, K>;
        };
        // a key of no field, of a tag or of the map of `yyjson::extra_keys` or `yyjson::pattern_properties`
        template <typename C, std::size_t K>
        requires (yyjson::detail::object_struct_value<C>() && K == yyjson::detail::struct_field_count<C>)
        struct member_type<C, K>
        {
            using type = yyjson::value;
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
                auto member = sub(sub(at, "properties"), key);
                emit_node<Mn, F, S>(member);
                // the read puts the key in the map of `yyjson::extra_keys`, whose schema `properties` hides
                if constexpr (yd::object_struct_value<C>())
                    if constexpr (yd::has_extra_keys<C> && k == yd::struct_field_count<C>)
                        if (const auto tags = adjacent_keys<C>();
                            std::ranges::find(tags, key) == tags.end() && !yd::matches_any_pattern<C>(key))
                            emit_map_values<C, yd::extra_keys_index<C>>(member.empty() ? member : conjunct(member));
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

        // `yyjson::matches<T>` keeps no T but its check, which finds T among the settings of S.
        template <typename N, typename S>
        void generator::emit_matched(object_ref out)
        {
            using Ts =
                decltype(std::tuple_cat(std::declval<typename matched_types<typename settings_of<S>::type>::type>(),
                                        std::declval<kind_types>()));
            constexpr auto i = matched_index(N::constraints.matches, static_cast<Ts*>(nullptr));
            static_assert(i < std::tuple_size_v<Ts>,
                          "json_schema takes yyjson::matches<T> in a subschema of yyjson::all_of, any_of, one_of and "
                          "prefix_items when T is a kind of JSON, such as bool or std::string, or when another setting "
                          "of the type has yyjson::matches<T>");
            if constexpr (i < std::tuple_size_v<Ts>) emit_matches<std::tuple_element_t<i, Ts>>(out);
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
                    names_->types[i].body(g, defs.emplace(names_->name(i, writing), empty_object, copy_string),
                                          names_->types[i].tags);
                }
            }
            // Calls `f(id, writing, schema)` with the schema of each root in the order of `add`.
            template <typename F>
            void emit_roots(F&& f)
            {
                settle();
                for (const auto& r : roots_)
                {
                    auto doc = value(object());
                    auto g = generator(*names_, r.writing, mode_);
                    r.emit(g, *doc.as_object());
                    f(r.id, r.writing, *std::move(doc).as_object());
                }
            }

        private:
            struct root
            {
                const void* id;
                bool writing;
                void (*emit)(generator&, object_ref);
            };
            std::string ref_prefix_;
            schema_mode mode_;
            std::vector<root> roots_;
            std::optional<registry> names_;

            [[nodiscard]] std::string definition_text(std::size_t i, bool writing)
            {
                auto doc = value(object());
                auto g = generator(*names_, writing, mode_);
                names_->types[i].body(g, *doc.as_object(), names_->types[i].tags);
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
                    names_->types[i].body(g, *doc.as_object(), names_->types[i].tags);
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
        inline constexpr bool dialect_tag = std::same_as<A, json_schema_2020_12_t> || std::same_as<A, openapi_3_1_t>;
        template <typename A>
        inline constexpr bool schema_argument =
            std::same_as<A, write_direction_t> || std::same_as<A, emit_defaults_t> || std::same_as<A, schema_options> ||
            dialect_tag<A> || yyjson::detail::read_policy<A>;

        struct collection_options
        {
            // The prefix of the references to the named types, where the program puts `result::schemas()`.
            std::string_view ref_prefix = "#/components/schemas/";
        };
    }  // namespace schema_detail

    // The JSON Schema (2020-12) of the JSON that `read<T>` takes, or with `yyjson::write_direction` of the JSON that
    // `write` gives.  The other arguments are the tags of the read policies, `yyjson::emit_defaults` and the dialect,
    // and one `schema_options`.
    template <typename T, typename... Args>
    [[nodiscard]] object json_schema(const Args&... args)
    {
        namespace sd = schema_detail;
        constexpr bool writing = (std::same_as<Args, write_direction_t> || ...);
        constexpr bool policies = (yyjson::detail::read_policy<Args> || ...);
        constexpr bool defaults = (std::same_as<Args, emit_defaults_t> || ...);
        constexpr bool openapi = (std::same_as<Args, openapi_3_1_t> || ...);
        static_assert((sd::schema_argument<Args> && ...),
                      "json_schema takes yyjson::write_direction, yyjson::emit_defaults, the read policies such as "
                      "yyjson::strict, a dialect such as yyjson::openapi_3_1, and yyjson::schema_options");
        static_assert(yyjson::detail::count_true<std::same_as<Args, write_direction_t>...> <= 1 &&
                          yyjson::detail::count_true<std::same_as<Args, emit_defaults_t>...> <= 1 &&
                          yyjson::detail::count_true<std::same_as<Args, schema_options>...> <= 1 &&
                          yyjson::detail::count_true<sd::dialect_tag<Args>...> <= 1,
                      "json_schema takes each of its tags, one dialect and yyjson::schema_options once");
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
        auto b = sd::builder(options.ref_prefix, {yyjson::detail::policies_of<Args...>, defaults, openapi});
        b.add<T>(writing);
        auto doc = value(object());
        auto root = *doc.as_object();
        if constexpr (!openapi) root.emplace("$schema", sd::dialect);
        if (!options.id.empty()) root.emplace("$id", options.id, copy_string);
        auto defs = root.emplace("$defs", empty_object);
        b.emit_definitions(defs);
        if (defs.empty()) root.erase("$defs");
        b.emit_root<T>(root, writing);
        return *std::move(doc).as_object();
    }

    // The schemas of several types, such as `components/schemas` of an OpenAPI 3.1 document.  A named type has one
    // name while its read and write schemas are the same, and "Name-Input" and "Name-Output" otherwise, which
    // `build()` settles when all the types are known.
    class schema_collection
    {
    public:
        using options = schema_detail::collection_options;

        // The schemas that `build()` generates.
        class result
        {
        public:
            // The schema of T that `add<T>()` added: a reference for a named type, without `$schema`.
            template <typename T>
            [[nodiscard]] const object& schema() const
            {
                return find(&schema_detail::type_identity_tag<T>, false, yyjson::detail::type_name<T>());
            }
            template <typename T>
            [[nodiscard]] const object& schema(write_direction_t) const
            {
                return find(&schema_detail::type_identity_tag<T>, true, yyjson::detail::type_name<T>());
            }
            // The schemas of the named types, by their names.
            [[nodiscard]] const object& schemas() const noexcept { return schemas_; }

        private:
            friend class schema_collection;
            struct root
            {
                const void* id;
                bool writing;
                object schema;
            };
            std::vector<root> roots_;
            object schemas_;

            explicit result(object schemas) : schemas_(std::move(schemas)) {}

            [[nodiscard]] const object& find(const void* id, bool writing, std::string_view name) const
            {
                for (const auto& r : roots_)
                    if (r.id == id && r.writing == writing) return r.schema;
                throw std::logic_error("yyjson::schema_collection::result::schema: " + std::string(name) +
                                       " was not added for " + (writing ? "yyjson::write_direction" : "read"));
            }
        };

        // The arguments are those of `json_schema` but the direction and `schema_options`: the read policies apply to
        // the read schemas, `yyjson::emit_defaults` to the write schemas.  The dialect is OpenAPI 3.1 by default.
        template <typename... Args>
        requires (!(sizeof...(Args) == 1 && (std::same_as<Args, schema_collection> && ...)))
        explicit schema_collection(const Args&... args) : builder_(make_builder(args...))
        {
        }

        // Adds the schema of what `read<T>` takes, or with `yyjson::write_direction` of what `write` gives.
        template <typename T>
        schema_collection& add() &
        {
            static_assert(yyjson::detail::read_castable<T>, "schema_collection::add<T>: yyjson::cast does not give T");
            builder_.add<T>(false);
            return *this;
        }
        template <typename T>
        schema_collection& add(write_direction_t) &
        {
            static_assert(writable<T>,
                          "schema_collection::add<T>(yyjson::write_direction): yyjson::write does not take T");
            builder_.add<T>(true);
            return *this;
        }
        template <typename T>
        schema_collection&& add() &&
        {
            return std::move(add<T>());
        }
        template <typename T>
        schema_collection&& add(write_direction_t w) &&
        {
            return std::move(add<T>(w));
        }

        // Generates the schemas of the types added.  Throws `std::invalid_argument` as `json_schema` does.
        [[nodiscard]] result build() const&
        {
            auto b = builder_;
            return build(b);
        }
        [[nodiscard]] result build() && { return build(builder_); }

    private:
        schema_detail::builder builder_;

        template <typename... Args>
        static schema_detail::builder make_builder(const Args&... args)
        {
            namespace sd = schema_detail;
            static_assert((((sd::schema_argument<Args> && !std::same_as<Args, write_direction_t> &&
                             !std::same_as<Args, schema_options>) ||
                            std::same_as<Args, options>) &&
                           ...),
                          "schema_collection takes yyjson::emit_defaults, the read policies such as yyjson::strict, a "
                          "dialect such as yyjson::json_schema_2020_12, and schema_collection::options");
            static_assert(yyjson::detail::count_true<std::same_as<Args, emit_defaults_t>...> <= 1 &&
                              yyjson::detail::count_true<std::same_as<Args, options>...> <= 1 &&
                              yyjson::detail::count_true<sd::dialect_tag<Args>...> <= 1,
                          "schema_collection takes each of its tags, one dialect and schema_collection::options once");
            auto opts = options();
            const auto take = [&]<typename A>(const A& a) {
                if constexpr (std::same_as<A, options>) opts = a;
            };
            (take(args), ...);
            return sd::builder(opts.ref_prefix,
                               {yyjson::detail::policies_of<Args...>, (std::same_as<Args, emit_defaults_t> || ...),
                                !(std::same_as<Args, json_schema_2020_12_t> || ...)});
        }

        static result build(schema_detail::builder& b)
        {
            auto defs = value(object());
            b.emit_definitions(*defs.as_object());
            auto r = result(*std::move(defs).as_object());
            b.emit_roots([&](const void* id, bool writing, object schema) {
                r.roots_.push_back({id, writing, std::move(schema)});
            });
            return r;
        }
    };
}  // namespace yyjson
