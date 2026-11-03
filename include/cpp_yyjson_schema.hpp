// JSON Schema output of the C++ types that cpp-yyjson reads and writes.  See docs/json-schema.md.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cpp_yyjson.hpp"

namespace yyjson
{
    // Which conversion the schema describes: the JSON that `read<T>` takes, or the JSON that `write` gives.  The two
    // differ where a type is read from more than it is written as, such as a caster whose `from_json` and `to_json`
    // use different types.
    enum class schema_direction : std::uint8_t
    {
        read,
        write,
    };

    struct schema_options
    {
        schema_direction direction = schema_direction::read;
        // The value of `$id`, none if empty.
        std::string_view id = {};
        // The prefix of the references to named types.  Their schemas are always in `$defs` of the result; another
        // prefix is for a program that moves them elsewhere in a larger document.
        std::string_view ref_prefix = "#/$defs/";
    };

    namespace schema_detail
    {
        struct collection_options
        {
            // The prefix of the references to named types, where the program puts `result::schemas()`.
            std::string_view ref_prefix = "#/components/schemas/";
        };
    }  // namespace schema_detail

    namespace schema_detail
    {
        using yyjson::detail::schema_node;
        using writer::array_ref;
        using writer::object_ref;

        inline constexpr std::string_view dialect = "https://json-schema.org/draft/2020-12/schema";

        // The name of T in `$defs`: its qualified name with the characters other than `[A-Za-z0-9_.]` replaced.
        template <typename T>
        std::string def_name()
        {
            auto name = std::string(yyjson::detail::type_name<T>());
            for (const auto prefix : {std::string_view("struct "), std::string_view("class "),
                                      std::string_view("enum "), std::string_view("union ")})
                if (name.starts_with(prefix)) name.erase(0, prefix.size());
            auto result = std::string();
            for (std::size_t i = 0; i < name.size(); ++i)
            {
                const auto c = name[i];
                if (c == ':' && i + 1 < name.size() && name[i + 1] == ':')
                {
                    result += '.';
                    ++i;
                }
                else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
                         c == '.')
                    result += c;
                else if (c != ' ')
                    result += '_';
            }
            return result;
        }

        template <typename T>
        concept string_type = std::same_as<T, std::string> || std::same_as<T, std::string_view> ||
                              std::same_as<T, std::filesystem::path> || std::same_as<T, const char*> ||
                              std::same_as<T, char*>;

        template <typename T>
        concept struct_type = (yyjson::detail::reflectable<T> && !yyjson::detail::std_array<T>) ||
                              yyjson::detail::visitable<T>;

        template <typename T>
        concept object_range =
            std::ranges::range<T> && yyjson::detail::key_value_like<std::ranges::range_value_t<T>> &&
            (yyjson::detail::object_emplaceable_range<T, reader::const_object_ref> ||
             yyjson::detail::object_back_emplaceable_range<T, reader::const_object_ref>);

        // A pair of a key and a value, which a tuple of them writes as a member of an object.
        template <typename P>
        concept key_value_element =
            yyjson::detail::pair_like<P> && yyjson::detail::key_type<std::tuple_element_t<0, P>>;

        template <typename T>
        struct is_variant : std::false_type
        {
        };
        template <typename... Ts>
        struct is_variant<std::variant<Ts...>> : std::true_type
        {
        };

        // The alternative I of `yyjson::any_of` or `yyjson::one_of`, which is `yyjson::alt<T, settings...>` or T.
        template <std::size_t I, typename A, typename... As>
        struct nth_alternative : nth_alternative<I - 1, As...>
        {
        };
        template <typename A, typename... As>
        struct nth_alternative<0, A, As...>
        {
            using type = A;
        };
        template <typename V, std::size_t I>
        struct combined_alternative;
        template <template <typename...> class C, typename... As, std::size_t I>
        struct combined_alternative<C<As...>, I>
        {
            using type = typename nth_alternative<I, As...>::type;
        };

        // Whether the alternative A has `yyjson::examples`.
        template <typename A>
        inline constexpr bool alternative_has_examples = false;
        template <typename T, auto... Settings>
        inline constexpr bool alternative_has_examples<yyjson::alt<T, Settings...>> =
            (yyjson::detail::is_examples<yyjson::detail::alternative_setting_t<Settings>>::value || ...);

        template <typename T>
        inline constexpr char type_identity_tag = 0;

        // The tag that `yyjson::internal_tag` writes in the object of an alternative: its key and the name of the
        // alternative.
        struct variant_tag
        {
            std::string_view key;
            std::string_view id;
        };

        class generator;

        // The structs and the enumerations that the schemas refer to by name.  A type has one schema per direction,
        // or one for both directions when they are the same.
        struct registry
        {
            using body_function = void (*)(generator&, object_ref);
            struct entry
            {
                const void* id;
                std::string base;
                std::string_view type_name;
                body_function body;
                std::array<bool, 2> used = {};  // read, write
                bool shared = true;
            };
            std::vector<entry> types;
            // The schemas in the order in which they are found: the type and whether it is the write direction.
            std::vector<std::pair<std::size_t, bool>> order;
            std::string ref_prefix;
            // Whether the names are settled; until then a reference names the entry, which compares the schemas.
            bool settled = false;
            // `S()` of each struct whose read schema has been written, constructed once for all its schemas.
            std::vector<std::pair<const void*, std::shared_ptr<const void>>> defaults;

            template <typename T>
            std::size_t find_or_add(bool writing);

            [[nodiscard]] bool split(std::size_t i) const noexcept
            {
                return !types[i].shared && types[i].used[0] && types[i].used[1];
            }
            [[nodiscard]] std::string name(std::size_t i, bool writing) const
            {
                if (!settled)
                    return "#" + std::to_string(i) + (split(i) ? (writing ? ":write" : ":read") : "");
                if (!split(i)) return types[i].base;
                return types[i].base + (writing ? "-Output" : "-Input");
            }
            [[nodiscard]] std::string ref(std::size_t i, bool writing) const
            {
                return settled ? ref_prefix + name(i, writing) : name(i, writing);
            }
        };

        // The settings of `yyjson::title`, `yyjson::description`, `yyjson::deprecated` and `yyjson::examples` of a
        // field or a type.
        struct metadata
        {
            std::string_view title;
            std::string_view description;
            bool deprecated = false;
            std::vector<std::string_view> examples;
            // The schema that `yyjson::schema` declares, which is not metadata but is given in the same places.
            std::optional<yyjson::schema> declared;

            template <typename A>
            void add(const A& a)
            {
                if constexpr (yyjson::detail::setting_of<A, yyjson::title<>>)
                    title = a.text();
                else if constexpr (yyjson::detail::setting_of<A, yyjson::description<>>)
                    description = a.text();
                else if constexpr (std::same_as<A, deprecated_t>)
                    deprecated = true;
                else if constexpr (yyjson::detail::is_examples<A>::value)
                    for (std::size_t i = 0; i < A::size(); ++i) examples.push_back(a.text(i));
                else if constexpr (std::same_as<A, yyjson::schema>)
                    declared = a;
            }
            // The declared schema of a direction, or null if it is derived.
            [[nodiscard]] const yyjson::schema* declared_for(bool writing) const noexcept
            {
                return declared && declared->text_data(writing) != nullptr ? &*declared : nullptr;
            }
            [[nodiscard]] bool empty() const noexcept
            {
                return title.empty() && description.empty() && !deprecated && examples.empty();
            }
        };

#if FIELD_RFL_ANNOTATIONS
        // The annotations `yyjson::examples` of the entity R, which is a template and not looked up by its type.
        template <std::meta::info R>
        inline constexpr std::span<const std::meta::info> examples_annotations =
            std::define_static_array(yyjson::detail::template_annotations_of<R, false, ^^yyjson::examples_t>());
        template <std::meta::info R, std::size_t K>
        using examples_annotation_type =
            typename[:std::meta::remove_cvref(std::meta::type_of(examples_annotations<R>[K])):];

        // Adds the metadata in the annotations of the member or the type R.
        template <std::meta::info R>
        void add_annotated_metadata(metadata& m)
        {
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::title<>>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::description<>>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, deprecated_t>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::schema>) m.add(a);
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (m.add(std::meta::extract<examples_annotation_type<R, Ks>>(examples_annotations<R>[Ks])), ...);
            }(std::make_index_sequence<examples_annotations<R>.size()>{});
        }
#endif

        // The member that each element of `annotations<S>` names with `yyjson::field`, or an empty name.  It is
        // computed once for a struct, so that a field looks up its settings without instantiating code for every
        // element: a struct of a few hundred annotated fields would otherwise take tens of gigabytes to compile.
        template <typename S>
        inline constexpr auto field_annotation_members = yyjson::detail::apply_annotations(
            [](const auto&... as) {
                return std::array<std::string_view, sizeof...(as)>{[]<typename A>(const A& a) -> std::string_view {
                    if constexpr (yyjson::detail::is_field_annotation<A>::value)
                        return a.member;
                    else
                        return {};
                }(as)...};
            },
            annotations<S>.values);

        // The indices in `annotations<S>` of the `yyjson::field` settings of the field R, by its index among all
        // the fields.
        template <typename S, std::size_t R>
        consteval std::size_t field_annotation_count()
        {
            return static_cast<std::size_t>(std::ranges::count(field_annotation_members<S>,
                                                               yyjson::detail::declared_member_name<S, R>));
        }
        template <typename S, std::size_t R>
        inline constexpr auto field_annotation_indices = [] {
            auto result = std::array<std::size_t, field_annotation_count<S, R>()>{};
            auto k = std::size_t{0};
            for (std::size_t i = 0; i < field_annotation_members<S>.size(); ++i)
                if (field_annotation_members<S>[i] == yyjson::detail::declared_member_name<S, R>) result[k++] = i;
            return result;
        }();

        // f(a) with each `yyjson::field` a of the field R of S in `annotations<S>`.
        template <typename S, std::size_t R, typename F>
        constexpr void for_each_field_annotation(F&& f)
        {
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (f(yyjson::detail::annotation_at<field_annotation_indices<S, R>[Ks]>(annotations<S>.values)), ...);
            }(std::make_index_sequence<field_annotation_indices<S, R>.size()>{});
        }

        // The metadata of the field R of the struct S, by its index among all the fields.  The settings are checked
        // with the keys of the fields, where a field has each of them once at most.
        template <typename S, std::size_t R>
        metadata field_metadata()
        {
            auto m = metadata();
            for_each_field_annotation<S, R>(
                [&](const auto& a) { std::apply([&](const auto&... vs) { (m.add(vs), ...); }, a.values); });
#if FIELD_RFL_ANNOTATIONS
            if constexpr (!yyjson::detail::visitable<S>)
            {
                // Metadata alone does not build the table of the keys, which checks the settings of a member.
                constexpr auto member = field_reflection::detail::member_at<S, R>;
                static_assert(field_reflection::detail::annotations_v<member, yyjson::title<>>.size() <= 1,
                              "a field is given yyjson::title twice");
                static_assert(field_reflection::detail::annotations_v<member, yyjson::description<>>.size() <= 1,
                              "a field is given yyjson::description twice");
                static_assert(field_reflection::detail::annotations_v<member, deprecated_t>.size() <= 1,
                              "a field is given yyjson::deprecated twice");
                static_assert(examples_annotations<member>.size() <= 1, "a field is given yyjson::examples twice");
                static_assert(field_reflection::detail::annotations_v<member, yyjson::schema>.size() <= 1,
                              "a field is given yyjson::schema twice");
                add_annotated_metadata<member>(m);
            }
#endif
            return m;
        }

        // Whether the field R of S has `yyjson::examples`, which instantiates reading them.
        template <typename S, std::size_t R>
        consteval bool field_has_examples()
        {
            auto result = false;
            for_each_field_annotation<S, R>([&](const auto& a) {
                result = result || std::apply(
                                       []<typename... Vs>(const Vs&...) {
                                           return (yyjson::detail::is_examples<Vs>::value || ...);
                                       },
                                       a.values);
            });
#if FIELD_RFL_ANNOTATIONS
            if constexpr (!yyjson::detail::visitable<S>)
                result = result || !examples_annotations<field_reflection::detail::member_at<S, R>>.empty();
#endif
            return result;
        }

        // Whether the field R of S has `yyjson::schema`.
        template <typename S, std::size_t R>
        consteval bool field_has_schema()
        {
            auto result = false;
            for_each_field_annotation<S, R>([&](const auto& a) {
                result = result || std::apply(
                                       []<typename... Vs>(const Vs&...) {
                                           return (std::same_as<Vs, yyjson::schema> || ...);
                                       },
                                       a.values);
            });
#if FIELD_RFL_ANNOTATIONS
            if constexpr (!yyjson::detail::visitable<S>)
                result = result || !field_reflection::detail::annotations_v<field_reflection::detail::member_at<S, R>,
                                                                            yyjson::schema>
                                        .empty();
#endif
            return result;
        }

        // The number of the settings of the type A that `annotations<T>` and the annotations of the type T have.
        template <typename T, typename A>
        consteval std::size_t type_setting_count()
        {
            auto count = []<typename... As>(std::type_identity<std::tuple<As...>>) {
                return (std::size_t{0} + ... + std::size_t{yyjson::detail::setting_of<As, A>});
            }(std::type_identity<yyjson::detail::annotation_values_of<T>>{});
#if FIELD_RFL_ANNOTATIONS
            if constexpr (std::is_class_v<T> || std::is_enum_v<T>)
                count += field_reflection::type_annotations<T, A>.size();
#endif
            return count;
        }
        template <typename T>
        consteval std::size_t type_examples_count()
        {
            auto count = []<typename... As>(std::type_identity<std::tuple<As...>>) {
                return (std::size_t{0} + ... + std::size_t{yyjson::detail::is_examples<As>::value});
            }(std::type_identity<yyjson::detail::annotation_values_of<T>>{});
#if FIELD_RFL_ANNOTATIONS
            if constexpr (std::is_class_v<T> || std::is_enum_v<T>)
                count += examples_annotations<std::meta::dealias(^^T)>.size();
#endif
            return count;
        }

        // The metadata of the type T.
        template <typename T>
        metadata type_metadata()
        {
            static_assert(type_setting_count<T, yyjson::title<>>() <= 1, "a type is given yyjson::title twice");
            static_assert(type_setting_count<T, yyjson::description<>>() <= 1,
                          "a type is given yyjson::description twice");
            static_assert(type_setting_count<T, deprecated_t>() <= 1, "a type is given yyjson::deprecated twice");
            static_assert(type_examples_count<T>() <= 1, "a type is given yyjson::examples twice");
            static_assert(type_setting_count<T, yyjson::schema>() <= 1, "a type is given yyjson::schema twice");
            auto m = metadata();
            yyjson::detail::apply_annotations(
                [&](const auto&... as) {
                    (
                        [&]<typename A>(const A& a) {
                            if constexpr (yyjson::detail::metadata_setting<A>) m.add(a);
                        }(as),
                        ...);
                },
                annotations<T>.values);
#if FIELD_RFL_ANNOTATIONS
            if constexpr (std::is_class_v<T> || std::is_enum_v<T>) add_annotated_metadata<std::meta::dealias(^^T)>(m);
#endif
            return m;
        }

        // The name of the struct or the enumeration T in `$defs`: its `yyjson::schema_name`, or else its qualified
        // name.
        template <typename T>
        std::string schema_name_of()
        {
            static_assert(type_setting_count<T, yyjson::schema_name>() <= 1,
                          "a type is given yyjson::schema_name twice");
            if constexpr (type_setting_count<T, yyjson::schema_name>() == 0)
                return def_name<T>();
            else
            {
                auto name = std::string_view();
                yyjson::detail::apply_annotations(
                    [&](const auto&... as) {
                        (
                            [&]<typename A>(const A& a) {
                                if constexpr (std::same_as<A, yyjson::schema_name>) name = a.text();
                            }(as),
                            ...);
                    },
                    annotations<T>.values);
#if FIELD_RFL_ANNOTATIONS
                for (const yyjson::schema_name& a : field_reflection::type_annotations<T, yyjson::schema_name>)
                    name = a.text();
#endif
                return std::string(name);
            }
        }

        template <typename T>
        struct optional_value
        {
            using type = T;
        };
        template <typename T>
        struct optional_value<std::optional<T>>
        {
            using type = T;
        };

        // A variant of std::monostate and a value, which is null or the value, and not tagged by `internal_tag`.
        template <typename T>
        concept nullable_variant = is_variant<T>::value && yyjson::detail::nullable<T> &&
                                   (yyjson::detail::variant_tags<T>::tagging != detail::variant_tagging::internal);

        template <typename T>
        concept user_read = !yyjson::detail::reads_by_default<T>;
        // T is written by a `to_json` of the user, which returns a value or writes into one.
        template <typename T>
        concept user_write = (!std::derived_from<caster<T>, yyjson::detail::builtin_caster>) &&
                             (writer::detail::to_json_inp_usr_defined<T> ||
                              writer::detail::to_json_usr_translator<const T&>);

        // The types T of the settings `yyjson::is_type<T>` within the setting A, as a `type_list` that may repeat
        // them.  A node of `yyjson::contains(yyjson::is_type<T>)` keeps only the test of T, so the generator finds T
        // here, in the types of the annotations of the type that has the node.
        template <typename... Ls>
        struct concat_types
        {
            using type = yyjson::detail::type_list<>;
        };
        template <typename... As>
        struct concat_types<yyjson::detail::type_list<As...>>
        {
            using type = yyjson::detail::type_list<As...>;
        };
        template <typename... As, typename... Bs, typename... Rest>
        struct concat_types<yyjson::detail::type_list<As...>, yyjson::detail::type_list<Bs...>, Rest...>
            : concat_types<yyjson::detail::type_list<As..., Bs...>, Rest...>
        {
        };
        // Eight lists at a time, so that a struct of many fields does not exceed the depth of instantiation.
        template <typename... As, typename... Bs, typename... Cs, typename... Ds, typename... Es, typename... Fs,
                  typename... Gs, typename... Hs, typename... Rest>
        struct concat_types<yyjson::detail::type_list<As...>, yyjson::detail::type_list<Bs...>,
                            yyjson::detail::type_list<Cs...>, yyjson::detail::type_list<Ds...>,
                            yyjson::detail::type_list<Es...>, yyjson::detail::type_list<Fs...>,
                            yyjson::detail::type_list<Gs...>, yyjson::detail::type_list<Hs...>, Rest...>
            : concat_types<yyjson::detail::type_list<As..., Bs..., Cs..., Ds..., Es..., Fs..., Gs..., Hs...>, Rest...>
        {
        };
        template <typename A>
        struct type_tests_in
        {
            using type = yyjson::detail::type_list<>;
        };
        template <typename T>
        struct type_tests_in<yyjson::is_type_t<T>>
        {
            using type = yyjson::detail::type_list<T>;
        };
        template <template <typename...> class Tmpl, typename... As>
        struct type_tests_in<Tmpl<As...>> : concat_types<typename type_tests_in<As>::type...>
        {
        };
        // The settings of an alternative are its template arguments, as values or as pointers to them.
        template <typename T, auto... Ss>
        struct type_tests_in<yyjson::alt<T, Ss...>>
            : concat_types<typename type_tests_in<
                  std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<decltype(Ss)>>>>::type...>
        {
        };
#if FIELD_RFL_ANNOTATIONS
        template <typename S, std::size_t I, std::size_t... Ks>
        auto element_type_tests(std::index_sequence<Ks...>)
            -> concat_types<typename type_tests_in<yyjson::detail::element_annotation_type<S, I, Ks>>::type...>;
        template <typename S, std::size_t... Ks>
        auto condition_type_tests(std::index_sequence<Ks...>)
            -> concat_types<typename type_tests_in<yyjson::detail::condition_annotation_type<S, Ks>>::type...>;
        template <typename S, std::size_t... Is>
        auto annotated_type_tests(std::index_sequence<Is...>) -> concat_types<
            typename decltype(condition_type_tests<S>(
                std::make_index_sequence<yyjson::detail::condition_annotations<S>.size()>{}))::type,
            typename decltype(element_type_tests<S, Is>(
                std::make_index_sequence<yyjson::detail::element_annotations<S, Is>.size()>{}))::type...>;
#endif
        // The types of `yyjson::is_type<T>` in the annotations of T: those of `yyjson::annotations<T>`, and, in
        // C++26, those of the members and of the struct itself.
        template <typename T>
        struct type_tests_of
        {
            using type = typename type_tests_in<yyjson::detail::annotation_values_of<T>>::type;
        };
        // `yyjson::any_of` and `yyjson::one_of` also hold them in the settings of their alternatives.
        template <yyjson::detail::combined_variant V>
        struct type_tests_of<V>
            : concat_types<typename type_tests_in<yyjson::detail::annotation_values_of<V>>::type,
                           typename type_tests_in<V>::type>
        {
        };
#if FIELD_RFL_ANNOTATIONS
        template <typename T>
        requires yyjson::detail::reflectable<T>
        struct type_tests_of<T>
            : concat_types<typename type_tests_in<yyjson::detail::annotation_values_of<T>>::type,
                           typename decltype(annotated_type_tests<T>(
                               std::make_index_sequence<field_reflection::field_count<T>>{}))::type>
        {
        };
#endif

        class generator
        {
        public:
            generator(registry& names, bool writing, bool strict) : names_(names), writing_(writing), strict_(strict)
            {
            }

            // Writes the schema of T into `out`, with the settings of the node `node` of `nodes` (none if null).
            template <typename T>
            void emit(object_ref out, const schema_node* nodes = nullptr, std::size_t node = 0);
            template <typename T>
            static void emit_root(generator& g, object_ref out)
            {
                g.emit<T>(out);
            }
            // Writes the schema of the struct, the enumeration or the named variant T, which `$defs` has.
            template <typename T>
            static void emit_definition(generator& g, object_ref out)
            {
                if (g.emit_declared_type<T>(out))
                    ;
                else if (g.writing() ? user_write<T> : user_read<T>)
                {
                    if constexpr (user_write<T>)
                        if (g.writing()) g.emit_user_write<T>(out);
                    if constexpr (user_read<T>)
                        if (!g.writing()) g.emit_user_read<T>(out);
                }
                else if constexpr (std::is_enum_v<T>)
                    g.emit_enum<T>(out);
                else if constexpr (is_variant<T>::value)
                    g.emit_variant<T>(out);
                else if constexpr (yyjson::detail::combined_variant<T>)
                    g.emit_combined<T>(out);
                else if constexpr (yyjson::detail::transparent_struct<T>)
                    g.emit_transparent<T>(out, {});
                else if constexpr (yyjson::detail::as_array_struct<T>)
                    g.emit_array_struct<T>(out);
                else if constexpr (struct_type<T>)
                    g.emit_struct<T>(out);
                g.emit_type_metadata<T>(out);
            }

        private:
            registry& names_;
            bool writing_;
            bool strict_;
            // The values of `yyjson::const_` and `yyjson::enum_` of the struct whose fields are written,
            // and the nodes of its tree, where the patterns of `yyjson::pattern` after the first of a value are.
            const yyjson::detail::json_scalar* values_ = nullptr;
            const schema_node* nodes_ = nullptr;
            // The variants tagged by `internal_tag` whose alternatives are being written in place.
            std::vector<const void*> inlined_;
            // The schemas of the types of `yyjson::is_type<T>` in `yyjson::contains` of the type whose settings are
            // written, which a node keeps as its test only.
            struct type_test_schema
            {
                const yyjson::detail::type_test_info* test;
                void (generator::*emit)(object_ref);
            };
            std::span<const type_test_schema> type_tests_;

            template <typename... Ts>
            static std::span<const type_test_schema> type_tests_from(yyjson::detail::type_list<Ts...>)
            {
                if constexpr (sizeof...(Ts) == 0)
                    return {};
                else
                {
                    static constexpr type_test_schema table[] = {
                        {&yyjson::detail::type_test_info_of<Ts>, &generator::emit_read<Ts>}...};
                    return table;
                }
            }
            // Makes the types of `yyjson::is_type<T>` in the annotations of T those that the nodes find, until the
            // returned object is destroyed.
            template <typename T>
            [[nodiscard]] auto type_tests_for()
            {
                struct restore
                {
                    std::span<const type_test_schema>& tests;
                    std::span<const type_test_schema> value;
                    ~restore() { tests = value; }
                };
                return restore{type_tests_,
                               std::exchange(type_tests_, type_tests_from(typename type_tests_of<T>::type{}))};
            }
            // Writes the read schema of the type of the test into `out`, beside what `out` already has.
            void emit_type_test(object_ref out, const yyjson::detail::type_test_info& test)
            {
                const auto it =
                    std::ranges::find_if(type_tests_, [&](const type_test_schema& e) { return e.test == &test; });
                if (it == type_tests_.end())
                    throw std::logic_error(std::string("yyjson::json_schema found no schema of yyjson::is_type<")
                                               .append(test.name)
                                               .append(">"));
                if (out.size() == 0)
                    (this->*it->emit)(out);
                else
                    (this->*it->emit)(all_of(out).emplace_back(empty_object));
            }
            // The `allOf` of `out`, which is added if `out` has none.
            static array_ref all_of(object_ref out)
            {
                return out.contains("allOf") ? *out.find("allOf")->second.as_array()
                                             : out.emplace("allOf", empty_array);
            }

            // Whether the settings of a node hold for a value of T itself: a `std::optional`, a pointer and a variant
            // of `std::monostate` and a value pass them to the value that they hold, null has none, and the
            // settings of a type that a caster of the user converts are not written for the JSON value.
            template <typename T>
            [[nodiscard]] bool settles() const noexcept
            {
                if constexpr (yyjson::detail::is_std_optional<T>::value || yyjson::detail::owning_pointer<T> ||
                              nullable_variant<T> || std::same_as<T, std::nullptr_t> ||
                              std::same_as<T, std::monostate>)
                    return false;
                else if constexpr (yyjson::detail::indirect_value<T> &&
                                   std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                    return false;
                else if constexpr (user_read<T> || user_write<T>)
                    return writing() ? !user_write<T> : !user_read<T>;
                else
                    return true;
            }
            // Writes the settings of the node k of a `yyjson::not_` or a `yyjson::if_` of a value of T into `o`.
            template <typename T>
            void emit_test_settings(object_ref o, const schema_node* nodes, const schema_node& k)
            {
                if constexpr (std::floating_point<T> && sizeof(T) < sizeof(double))
                    emit_float<T>(o, &k);
                else if constexpr (object_range<T> || std::same_as<T, writer::object> ||
                                   std::same_as<T, reader::object>)
                {
                    // min_items and max_items count the properties of an object.
                    auto rest = k.constraints;
                    if (rest.has<yyjson::min_items>()) o.emplace("minProperties", rest.min_items);
                    if (rest.has<yyjson::max_items>()) o.emplace("maxProperties", rest.max_items);
                    rest.set &= ~(yyjson::detail::constraint_bit<yyjson::min_items> |
                                  yyjson::detail::constraint_bit<yyjson::max_items>);
                    emit_constraints(o, rest);
                }
                else
                    emit_constraints(o, k.constraints, yyjson::detail::multi_kind<T>);
                emit_written_texts<T>(o, k.constraints);
                if (k.type_test != nullptr) emit_type_test(o, *k.type_test);
                if constexpr (std::same_as<T, writer::array> || std::same_as<T, reader::array>)
                {
                    if (k.contains != 0)
                        emit<writer::value>(o.emplace("contains", empty_object), nodes, k.contains);
                }
                else if constexpr (yyjson::detail::constraint_item_range<T>)
                {
                    if (k.contains != 0)
                        emit<yyjson::detail::array_element_t<T>>(o.emplace("contains", empty_object), nodes,
                                                                 k.contains);
                }
                if constexpr (yyjson::detail::constraint_prefix_target<T> || std::same_as<T, writer::array> ||
                              std::same_as<T, reader::array>)
                    if (k.prefix_count != 0) emit_test_prefix<T>(o, nodes, k);
            }
            // Writes the positions of `yyjson::prefix_items` of the node `k` of a test on a value of T as
            // `prefixItems`, each the schema of the element at it with its settings.
            template <typename T>
            void emit_test_prefix(object_ref o, const schema_node* nodes, const schema_node& k)
            {
                auto prefix = o.emplace("prefixItems", empty_array);
                if constexpr (yyjson::detail::constraint_tuple<T>)
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        ((Is < k.prefix_count ? emit<std::tuple_element_t<Is, T>>(prefix.emplace_back(empty_object),
                                                                                  nodes, k.prefix + Is)
                                              : void()),
                         ...);
                    }(std::make_index_sequence<std::tuple_size_v<T>>{});
                else
                {
                    using E = std::conditional_t<std::same_as<T, writer::array> || std::same_as<T, reader::array>,
                                                 writer::value, yyjson::detail::array_element_t<T>>;
                    for (std::size_t i = 0; i < k.prefix_count; ++i)
                        emit<E>(prefix.emplace_back(empty_object), nodes, k.prefix + i);
                }
            }
            // Writes the `yyjson::not_` of a value of T from the node `first` on as `not`: of the schema of the
            // settings of one, or of `anyOf` those of several.  A value that has `not` already takes it in `allOf`.
            template <typename T>
            void emit_negated(object_ref out, const schema_node* nodes, std::size_t first)
            {
                auto target = out;
                if (out.contains("not")) target = all_of(out).emplace_back(empty_object);
                auto negated = target.emplace("not", empty_object);
                if (nodes[first].next == 0) return emit_test_settings<T>(negated, nodes, nodes[first]);
                auto any = negated.emplace("anyOf", empty_array);
                for (auto k = first; k != 0; k = nodes[k].next)
                    emit_test_settings<T>(any.emplace_back(empty_object), nodes, nodes[k]);
            }
            // Writes the `yyjson::if_then_else` of a value of T from the node `first` on as `if`, `then` and
            // `else`: `if` has the settings of `yyjson::if_`, `then` and `else` the schema of T with those of
            // `yyjson::then` and `yyjson::else_`.  A value that has `if` already takes the others in `allOf`.
            template <typename T>
            void emit_conditional(object_ref out, const schema_node* nodes, std::size_t first)
            {
                for (auto c = first; c != 0; c = nodes[c].next)
                {
                    auto target = out.contains("if") ? all_of(out).emplace_back(empty_object) : out;
                    emit_test_settings<T>(target.emplace("if", empty_object), nodes, nodes[c]);
                    if (nodes[c].then_branch != 0)
                        emit<T>(target.emplace("then", empty_object), nodes, nodes[c].then_branch);
                    if (nodes[c].else_branch != 0)
                        emit<T>(target.emplace("else", empty_object), nodes, nodes[c].else_branch);
                }
            }

            // Writes the settings of `yyjson::property_names` at the node `keys`: the constraints of a string, its
            // `yyjson::not_` and its `yyjson::if_then_else`.
            void emit_key_settings(object_ref out, const schema_node* nodes, std::size_t keys)
            {
                emit_constraints(out, nodes[keys].constraints);
                if (nodes[keys].negated != 0) emit_negated<std::string>(out, nodes, nodes[keys].negated);
                if (nodes[keys].conditional != 0)
                    emit_conditional<std::string>(out, nodes, nodes[keys].conditional);
            }

            // `yyjson::strict` applies to reading only: what `write` gives has every key that is not left out.
            [[nodiscard]] bool strict() const noexcept { return strict_ && !writing(); }
            [[nodiscard]] bool writing() const noexcept { return writing_; }
            // Writes the read schema of T in both directions: a value that `yyjson::is_type<T>` checks is written
            // as it is, so the write schema takes whatever the read accepts.
            template <typename T>
            void emit_read(object_ref out)
            {
                struct restore
                {
                    bool& writing;
                    bool value;
                    ~restore() { writing = value; }
                } const guard{writing_, std::exchange(writing_, false)};
                emit<T>(out);
            }

            // `S()`, from which the read schema takes the defaults of the fields, or null for the write schema.
            template <typename S>
            [[nodiscard]] const S* defaults_of()
            {
                if constexpr (std::default_initializable<S>)
                {
                    if (writing()) return nullptr;
                    const void* const id = &type_identity_tag<S>;
                    auto& all = names_.defaults;
                    const auto it = std::ranges::find_if(all, [&](const auto& d) { return d.first == id; });
                    if (it != all.end()) return static_cast<const S*>(it->second.get());
                    return static_cast<const S*>(all.emplace_back(id, std::make_shared<const S>()).second.get());
                }
                else
                    return nullptr;
            }

            template <typename F>
            [[nodiscard]] F read_value(std::string_view text) const
            {
                if (strict()) return yyjson::read<F>(text, yyjson::strict);
                return yyjson::read<F>(text);
            }

            // Whether the field K of S takes the value `field` as `read<S>` does: it satisfies the constraints of
            // the field.
            template <typename S, std::size_t K, typename F>
            [[nodiscard]] static bool satisfies(const F& field)
            {
                if constexpr (yyjson::detail::constrained_field<S, K>)
                    return yyjson::detail::constraint_violation<S, K>(field) == yyjson::detail::constraint_kind::none;
                else
                    return true;
            }

            // Whether the field K of S takes back what `write` gives for its value `d`: a caster may write what it
            // does not read, and a value in `T()` may not satisfy the constraints, also those of the alternative
            // of a `yyjson::one_of` that it holds where another alternative reads the text, and neither is a
            // default of the schema that is read.
            template <typename S, std::size_t K, typename F>
            [[nodiscard]] bool reads_back(const F& d) const
            {
                if constexpr (readable<F>)
                {
                    if (!yyjson::detail::settings_hold(d)) return false;
                    try
                    {
                        const auto text = yyjson::write(d);
                        return satisfies<S, K>(read_value<F>(std::string_view(text)));
                    }
                    catch (const std::exception&)
                    {
                        return false;
                    }
                }
                else
                    return false;
            }

            // Writes the metadata `m` of a field or a type into `out`, where it replaces that of the type of the
            // field.  Each example is read into E and checked by `check`, which takes the `std::optional<E>` that
            // holds it; the schema of reading has it as it is written, and the schema of `write` has what `write`
            // gives for the value read.
            template <typename E, bool Examples, typename Check>
            void emit_metadata(object_ref out, const metadata& m, std::string_view of, Check&& check) const
            {
                const auto put = [&](std::string_view key, const auto& value) {
                    out.erase(key);
                    out.emplace(key, value, copy_string);
                };
                if (!m.title.empty()) put("title", m.title);
                if (!m.description.empty()) put("description", m.description);
                if (m.deprecated) put("deprecated", true);
                if constexpr (Examples) emit_examples<E>(out, m, of, check);
            }
            template <typename E, typename Check>
            void emit_examples(object_ref out, const metadata& m, std::string_view of, Check&& check) const
            {
                out.erase("examples");
                auto list = out.emplace("examples", empty_array);
                for (const auto text : m.examples)
                {
                    const auto fail = [&](std::string_view why) {
                        throw std::invalid_argument(std::string("yyjson::examples of ").append(of).append(" has ")
                                                        .append(text).append(", which ").append(why));
                    };
                    if constexpr (readable<E>)
                    {
                        auto value = std::optional<E>();
                        try
                        {
                            value.emplace(read_value<E>(text));
                        }
                        catch (const std::exception& e)
                        {
                            fail(std::string("read<T> does not read: ").append(e.what()));
                        }
                        if (!check(value)) fail("does not satisfy the constraints");
                        if (!writing())
                            list.emplace_back(yyjson::read(text), copy_string);
                        else if constexpr (writable<E>)
                            list.emplace_back(*value, copy_string);
                    }
                    else
                        fail("read<T> cannot read");
                }
            }
            // Writes the schema that `yyjson::schema` declares for `of`, and returns whether it is `false`.
            static bool emit_declared(object_ref out, std::string_view text, std::string_view of)
            {
                const auto fail = [&](std::string_view why) {
                    throw std::invalid_argument(std::string("yyjson::schema of ").append(of).append(" has ")
                                                    .append(text).append(", which ").append(why));
                };
                auto doc = std::optional<reader::value>();
                try
                {
                    doc.emplace(yyjson::read(text));
                }
                catch (const std::exception& e)
                {
                    fail(std::string("is not JSON: ").append(e.what()));
                }
                if (const auto b = doc->as_bool())
                {
                    if (*b) return false;
                    out.emplace("not", empty_object);
                    return true;
                }
                const auto o = doc->as_object();
                if (!o) fail("is neither an object nor a boolean");
                for (const auto& [k, v] : *o) out.emplace(k, v, copy_string);
                return false;
            }
            // Writes the schema that `yyjson::schema` declares for T in this direction, and returns whether there
            // is one.
            template <typename T>
            bool emit_declared_type(object_ref out) const
            {
                if constexpr (type_setting_count<T, yyjson::schema>() != 0)
                {
                    if (const auto m = type_metadata<T>(); const auto* d = m.declared_for(writing()))
                    {
                        emit_declared(out, d->text(writing()), yyjson::detail::type_name<T>());
                        return true;
                    }
                }
                return false;
            }
            template <typename T>
            void emit_type_metadata(object_ref out) const
            {
                if (const auto m = type_metadata<T>(); !m.empty())
                    emit_metadata<T, type_examples_count<T>() != 0>(out, m, yyjson::detail::type_name<T>(),
                                                                    [](const std::optional<T>&) { return true; });
            }
            // The name of the field R of S in messages.
            template <typename S, std::size_t R>
            [[nodiscard]] static std::string member_of()
            {
                return std::string(yyjson::detail::type_name<S>()).append("::").append(
                    yyjson::detail::declared_member_name<S, R>);
            }
            // Whether T is put in `$defs` and has its metadata there: a struct, an enumeration, or a variant that
            // `yyjson::schema_name` names, and in a direction in which a caster of the user converts T, a type that
            // `yyjson::schema_name` names.
            template <typename T>
            [[nodiscard]] bool named() const noexcept
            {
                constexpr bool has_name = type_setting_count<T, yyjson::schema_name>() != 0;
                if (writing() ? user_write<T> : user_read<T>) return has_name;
                return std::is_enum_v<T> || struct_type<T> ||
                       ((is_variant<T>::value || yyjson::detail::combined_variant<T>) && has_name);
            }
            // Whether `yyjson::schema` declares the schema of T in this direction.
            template <typename T>
            [[nodiscard]] bool declares() const
            {
                if constexpr (type_setting_count<T, yyjson::schema>() != 0)
                {
                    const auto m = type_metadata<T>();
                    return m.declared_for(writing()) != nullptr;
                }
                else
                    return false;
            }

            template <typename T>
            void emit_user_read(object_ref out);
            template <typename T>
            void emit_user_write(object_ref out);
            template <typename T>
            void emit_builtin(object_ref out, const schema_node* nodes, std::size_t node);

            static void emit_bound(object_ref out, std::string_view key, const yyjson::detail::bound_number& b)
            {
                using kind = yyjson::detail::bound_number::kind_t;
                switch (b.kind)
                {
                    case kind::sint: out.emplace(key, b.sint); break;
                    case kind::uint: out.emplace(key, b.uint); break;
                    case kind::real: out.emplace(key, b.real); break;
                }
            }
            // Calls f with the C++ value of the JSON scalar s.
            template <typename F>
            static void visit_scalar(const yyjson::detail::json_scalar& s, F&& f)
            {
                using kind = yyjson::detail::json_scalar::kind_t;
                using number = yyjson::detail::bound_number::kind_t;
                if (s.kind == kind::string)
                    f(s.string());
                else if (s.kind == kind::boolean)
                    f(s.boolean);
                else if (s.number.kind == number::sint)
                    f(s.number.sint);
                else if (s.number.kind == number::uint)
                    f(s.number.uint);
                else
                    f(s.number.real);
            }
            // Whether a value that holds null or nothing, with the settings of the node n, may be null: unless
            // `yyjson::const_` or `yyjson::enum_` leave `nullptr` out.
            bool admits_null(const schema_node* n) const
            {
                using kind = yyjson::detail::json_scalar::kind_t;
                if (n == nullptr) return true;
                const auto& c = n->constraints;
                if (c.has<yyjson::const_<>>() && values_[c.constant].kind != kind::null) return false;
                if (c.has<yyjson::enum_<0>>())
                {
                    for (std::size_t i = 0; i < c.enum_count; ++i)
                        if (values_[c.enum_first + i].kind == kind::null) return true;
                    return false;
                }
                return true;
            }
            // Whether null satisfies the settings of the node k of a `yyjson::not_` or a `yyjson::if_`, as reading
            // tests it: unless `yyjson::const_` or `yyjson::enum_` leave `nullptr` out, or the type of
            // `yyjson::is_type<T>` does not read null.
            bool null_satisfies(const schema_node& k) const
            {
                return admits_null(&k) &&
                       (k.type_test == nullptr || k.type_test->test(nullptr, yyjson::detail::dom_kind::none));
            }
            // Whether null passes the settings of the node n, as reading tests the null of a `yyjson::then` or a
            // `yyjson::else_`.
            bool null_passes(const schema_node* nodes, const schema_node& n) const
            {
                if (!null_satisfies(n)) return false;
                for (auto k = n.negated; k != 0; k = nodes[k].next)
                    if (null_satisfies(nodes[k])) return false;
                return null_meets_conditions(nodes, &n);
            }
            // Whether null passes the `yyjson::if_then_else` of the node n: the settings of `yyjson::then` when it
            // satisfies those of `yyjson::if_`, and those of `yyjson::else_` otherwise.
            bool null_meets_conditions(const schema_node* nodes, const schema_node* n) const
            {
                if (n == nullptr) return true;
                for (auto c = n->conditional; c != 0; c = nodes[c].next)
                {
                    const auto branch = null_satisfies(nodes[c]) ? nodes[c].then_branch : nodes[c].else_branch;
                    if (branch != 0 && !null_passes(nodes, nodes[branch])) return false;
                }
                return true;
            }
            // What the `yyjson::not_` of the node n do to null: a `not_` whose settings all hold for null rejects it,
            // unless it has a `yyjson::is_type<T>`, which tests it.  A `yyjson::if_then_else` that null does not
            // pass rejects it too.
            enum class negated_null
            {
                kept,
                rejected,
                tested
            };
            negated_null null_in_not(const schema_node* nodes, const schema_node* n) const
            {
                if (!null_meets_conditions(nodes, n)) return negated_null::rejected;
                if (n == nullptr) return negated_null::kept;
                auto result = negated_null::kept;
                for (auto k = n->negated; k != 0; k = nodes[k].next)
                {
                    if (!admits_null(&nodes[k])) continue;
                    if (nodes[k].type_test == nullptr) return negated_null::rejected;
                    result = negated_null::tested;
                }
                return result;
            }
            // Writes the schema of null beside a value with the settings of the node n: `"type": "null"`, and the
            // `not` of the `yyjson::is_type<T>` of each `yyjson::not_` whose other settings hold for null.
            void emit_null(object_ref out, const schema_node* nodes, const schema_node* n)
            {
                out.emplace("type", "null");
                if (null_in_not(nodes, n) != negated_null::tested) return;
                std::size_t count = 0;
                for (auto k = n->negated; k != 0; k = nodes[k].next)
                    count += admits_null(&nodes[k]) ? 1 : 0;
                auto negated = out.emplace("not", empty_object);
                for (auto k = n->negated; k != 0; k = nodes[k].next)
                {
                    if (!admits_null(&nodes[k])) continue;
                    if (count == 1) return emit_type_test(negated, *nodes[k].type_test);
                    auto any = negated.contains("anyOf") ? *negated.find("anyOf")->second.as_array()
                                                         : negated.emplace("anyOf", empty_array);
                    emit_type_test(any.emplace_back(empty_object), *nodes[k].type_test);
                }
            }
            // Whether `yyjson::const_` or `yyjson::enum_` of the node n list only `nullptr`.
            bool only_null(const schema_node* n) const
            {
                using kind = yyjson::detail::json_scalar::kind_t;
                if (n == nullptr) return false;
                const auto& c = n->constraints;
                if (c.has<yyjson::const_<>>()) return values_[c.constant].kind == kind::null;
                if (!c.has<yyjson::enum_<0>>()) return false;
                for (std::size_t i = 0; i < c.enum_count; ++i)
                    if (values_[c.enum_first + i].kind != kind::null) return false;
                return true;
            }
            // Writes the constraints of a node.  The bounds of a number type are written by `emit_number`.
            // A listed `nullptr` belongs to the null branch beside the value and is left out here, unless the value
            // itself can be null (`with_null`): a DOM value or a variant of several JSON kinds.
            void emit_constraints(object_ref out, const yyjson::detail::field_constraints& c,
                                  bool with_null = false) const
            {
                if (c.has<yyjson::minimum>()) emit_bound(out, "minimum", c.minimum);
                if (c.has<yyjson::maximum>()) emit_bound(out, "maximum", c.maximum);
                if (c.has<yyjson::exclusive_minimum>()) emit_bound(out, "exclusiveMinimum", c.exclusive_minimum);
                if (c.has<yyjson::exclusive_maximum>()) emit_bound(out, "exclusiveMaximum", c.exclusive_maximum);
                if (c.has<yyjson::multiple_of>()) emit_bound(out, "multipleOf", c.multiple_of);
                if (c.has<yyjson::min_length>()) out.emplace("minLength", c.min_length);
                if (c.has<yyjson::max_length>()) out.emplace("maxLength", c.max_length);
                if (c.has<yyjson::min_items>()) out.emplace("minItems", c.min_items);
                if (c.has<yyjson::max_items>()) out.emplace("maxItems", c.max_items);
                if (c.has<yyjson::pattern<>>())
                {
                    out.emplace("pattern", c.pattern());
                    // The patterns after the first, which `allOf` gives.
                    for (auto k = c.next_pattern; k != 0; k = nodes_[k].constraints.next_pattern)
                        all_of(out).emplace_back(empty_object).emplace("pattern", nodes_[k].constraints.pattern());
                }
                if (c.has<yyjson::format<>>()) out.emplace("format", c.format());
                using kind = yyjson::detail::json_scalar::kind_t;
                // A `yyjson::json_text` is written as it is; `emit` gives the schema of `write` what `write` gives
                // for it.
                const auto put = [&](const yyjson::detail::json_scalar& s, auto&& emplace) {
                    if (s.kind == kind::json)
                        emplace(yyjson::read(s.string()), copy_string);
                    else if (s.kind != kind::null)
                        visit_scalar(s, emplace);
                    else if (with_null)
                        emplace(nullptr);
                };
                if (c.has<yyjson::const_<>>())
                    put(values_[c.constant], [&](const auto&... v) { out.emplace("const", v...); });
                if (c.has<yyjson::enum_<0>>())
                {
                    auto values = out.emplace("enum", empty_array);
                    for (std::size_t i = 0; i < c.enum_count; ++i)
                        put(values_[c.enum_first + i], [&](const auto&... v) { values.emplace_back(v...); });
                }
                if (c.has<yyjson::unique_items_t>()) out.emplace("uniqueItems", true);
                if (c.has<yyjson::min_contains>()) out.emplace("minContains", c.min_contains);
                if (c.has<yyjson::max_contains>()) out.emplace("maxContains", c.max_contains);
                // `satisfies<f>` has no keyword.
            }

            // Calls `put` with what `write` gives for each value that the text of a `yyjson::json_text` reads into as
            // the check of a value of T does: a DOM value takes the text as it is, a variant reads it into the
            // alternative that it holds, and other types read it into T, which must read it.
            template <typename T, typename F>
            static void written_json_text(std::string_view text, F&& put)
            {
                using U = std::remove_cv_t<T>;
                if constexpr (yyjson::detail::dom_kind_of<U> != yyjson::detail::dom_kind::none)
                    put(std::string(std::string_view(yyjson::read(text).write())));
                else if constexpr (yyjson::detail::multi_kind_variant<U>)
                {
                    using V = std::remove_cvref_t<decltype(yyjson::detail::as_std_variant(std::declval<const U&>()))>;
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        (written_json_alternative<std::variant_alternative_t<Is, V>>(text, put), ...);
                    }(std::make_index_sequence<std::variant_size_v<V>>{});
                }
                else
                    written_json_value<yyjson::detail::constrained_value_t<U>, false>(text, put);
            }
            template <typename A, typename F>
            static void written_json_alternative(std::string_view text, F&& put)
            {
                if constexpr (std::same_as<A, std::monostate> || std::same_as<A, std::nullptr_t>)
                    ;
                else if constexpr (yyjson::detail::nullable<A>)
                    written_json_alternative<yyjson::detail::constrained_value_t<A>>(text, put);
                else if constexpr (yyjson::detail::multi_kind<A>)
                    written_json_text<A>(text, put);
                else if constexpr (yyjson::detail::transparent_struct<A>)
                    written_json_alternative<
                        std::remove_cvref_t<decltype(yyjson::detail::struct_field<0>(std::declval<const A&>()))>>(
                        text, put);
                else
                    written_json_value<A, true>(text, put);
            }
            template <typename T, bool Multi, typename F>
            static void written_json_value(std::string_view text, F&& put)
            {
                using R = std::conditional_t<yyjson::detail::constraint_string<T>, std::string, T>;
                if constexpr (readable<R> && writable<R>)
                    if (const auto v = yyjson::detail::read_json_text<R, Multi>(text))
                        put(std::string(std::string_view(yyjson::write(*v))));
            }
            // Gives the schema of `write` what `write` gives for the values of `yyjson::const_` and `yyjson::enum_`
            // that are `yyjson::json_text`, which the check of a value of T reads.  A constant that reads into
            // values written differently, as an alternative of a variant each, lists them.
            template <typename T>
            void emit_written_texts(object_ref out, const yyjson::detail::field_constraints& c) const
            {
                using kind = yyjson::detail::json_scalar::kind_t;
                const auto json = [&](std::size_t i) { return values_[i].kind == kind::json; };
                const bool constant = c.has<yyjson::const_<>>() && json(c.constant);
                const bool listed = c.has<yyjson::enum_<0>>() &&
                                    std::ranges::any_of(std::views::iota(c.enum_first, c.enum_first + c.enum_count),
                                                        json);
                if (!writing() || (!constant && !listed)) return;
                const auto texts = [&](std::size_t first, std::size_t count) {
                    auto result = std::vector<std::string>();
                    const auto add = [&](std::string t) {
                        if (std::ranges::find(result, t) == result.end()) result.push_back(std::move(t));
                    };
                    for (auto i = first; i < first + count; ++i)
                    {
                        if (json(i))
                            written_json_text<T>(values_[i].string(), add);
                        else if (values_[i].kind == kind::null && yyjson::detail::multi_kind<T>)
                            add("null");
                    }
                    return result;
                };
                const auto list = [](object_ref o, const std::vector<std::string>& ts) {
                    auto values = o.emplace("enum", empty_array);
                    for (const auto& t : ts) values.emplace_back(yyjson::read(t), copy_string);
                };
                if (listed)
                {
                    out.erase("enum");
                    list(out, texts(c.enum_first, c.enum_count));
                }
                if (constant)
                {
                    out.erase("const");
                    const auto ts = texts(c.constant, 1);
                    if (ts.size() == 1)
                        out.emplace("const", yyjson::read(ts.front()), copy_string);
                    else
                        list(out.contains("enum") ? all_of(out).emplace_back(empty_object) : out, ts);
                }
            }

            // The double whose shortest text is what `yyjson::write` writes of f.
            template <std::floating_point F>
            static double written_real(F f)
            {
                return yyjson::read(yyjson::write(f)).template cast<double>();
            }

            // A float is compared with the bounds and the values of `const_` and `enum_` rounded to a
            // float, and is written in the shortest text that reads back as it, which may lie past the double of
            // the float: 3.4028235e38 for the largest one.  The schema gives those floats as they are written, an
            // exclusive bound as the inclusive bound of the next float inward.
            template <std::floating_point I>
            void emit_float(object_ref out, const schema_node* n) const
            {
                using L = std::numeric_limits<I>;
                using kind = yyjson::detail::bound_number::kind_t;
                const auto rounded = [](const yyjson::detail::bound_number& b) {
                    return b.kind == kind::sint   ? static_cast<I>(b.sint)
                           : b.kind == kind::uint ? static_cast<I>(b.uint)
                                                  : static_cast<I>(b.real);
                };
                out.emplace("type", "number");
                auto lower = L::lowest();
                auto upper = L::max();
                auto rest = yyjson::detail::field_constraints();
                if (n != nullptr)
                {
                    const auto& c = n->constraints;
                    if (c.has<yyjson::minimum>()) lower = std::max(lower, rounded(c.minimum));
                    if (c.has<yyjson::exclusive_minimum>())
                        lower = std::max(lower, std::nextafter(rounded(c.exclusive_minimum), L::infinity()));
                    if (c.has<yyjson::maximum>()) upper = std::min(upper, rounded(c.maximum));
                    if (c.has<yyjson::exclusive_maximum>())
                        upper = std::min(upper, std::nextafter(rounded(c.exclusive_maximum), -L::infinity()));
                    rest = c;
                    rest.set &= ~(yyjson::detail::constraint_bit<yyjson::minimum> |
                                  yyjson::detail::constraint_bit<yyjson::exclusive_minimum> |
                                  yyjson::detail::constraint_bit<yyjson::maximum> |
                                  yyjson::detail::constraint_bit<yyjson::exclusive_maximum> |
                                  yyjson::detail::constraint_bit<yyjson::const_<>> |
                                  yyjson::detail::constraint_bit<yyjson::enum_<0>>);
                }
                // A bound past the largest float leaves no float to read.
                if (lower <= L::max() && upper >= L::lowest())
                {
                    out.emplace("minimum", written_real(lower));
                    out.emplace("maximum", written_real(upper));
                }
                else
                    out.emplace("not", empty_object);
                // A listed `nullptr` belongs to the null branch beside the value.  A `yyjson::json_text` is written
                // as it is, as `emit_constraints` does.
                using value_kind = yyjson::detail::json_scalar::kind_t;
                const auto put = [&](std::size_t i, auto&& emplace) {
                    if (values_[i].kind == value_kind::json)
                        emplace(yyjson::read(values_[i].string()), copy_string);
                    else if (values_[i].kind != value_kind::null)
                        emplace(written_real(rounded(values_[i].number)));
                };
                if (n != nullptr && n->constraints.has<yyjson::const_<>>())
                    put(n->constraints.constant, [&](const auto&... v) { out.emplace("const", v...); });
                if (n != nullptr && n->constraints.has<yyjson::enum_<0>>())
                {
                    auto values = out.emplace("enum", empty_array);
                    const auto first = n->constraints.enum_first;
                    for (std::size_t i = first; i < first + n->constraints.enum_count; ++i)
                        put(i, [&](const auto&... v) { values.emplace_back(v...); });
                }
                emit_constraints(out, rest);
            }

            template <typename I>
            void emit_number(object_ref out, const schema_node* n) const
            {
                if constexpr (std::floating_point<I> && sizeof(I) < sizeof(double))
                    return emit_float<I>(out, n);
                out.emplace("type", std::integral<I> ? "integer" : "number");
                using L = std::numeric_limits<I>;
                // A double holds every finite JSON number that the reader accepts.
                constexpr bool bounded = std::integral<I>;
                const auto* c = n != nullptr ? &n->constraints : nullptr;
                // The bounds of the type, unless a constraint of the same keyword is at least as tight.
                const auto tighter = [](const yyjson::detail::bound_number& b, auto limit, bool lower) {
                    using kind = yyjson::detail::bound_number::kind_t;
                    const auto v = b.kind == kind::sint   ? static_cast<long double>(b.sint)
                                   : b.kind == kind::uint ? static_cast<long double>(b.uint)
                                                          : static_cast<long double>(b.real);
                    return lower ? v >= static_cast<long double>(limit) : v <= static_cast<long double>(limit);
                };
                const auto limit = [&](std::string_view key, I value) {
                    if constexpr (std::floating_point<I>)
                        out.emplace(key, static_cast<double>(value));
                    else if constexpr (std::is_signed_v<I>)
                        out.emplace(key, static_cast<std::int64_t>(value));
                    else
                        out.emplace(key, static_cast<std::uint64_t>(value));
                };
                if constexpr (bounded)
                {
                    if (c == nullptr || !c->has<yyjson::minimum>() || !tighter(c->minimum, L::lowest(), true))
                        limit("minimum", L::lowest());
                    if (c == nullptr || !c->has<yyjson::maximum>() || !tighter(c->maximum, L::max(), false))
                        limit("maximum", L::max());
                }
                if (c != nullptr)
                {
                    auto rest = *c;
                    if (bounded && rest.has<yyjson::minimum>() && !tighter(rest.minimum, L::lowest(), true))
                        rest.set &= ~yyjson::detail::constraint_bit<yyjson::minimum>;
                    if (bounded && rest.has<yyjson::maximum>() && !tighter(rest.maximum, L::max(), false))
                        rest.set &= ~yyjson::detail::constraint_bit<yyjson::maximum>;
                    emit_constraints(out, rest);
                }
            }

            // The key of an enumerator in an object: its name, or the integer as `enum_key_text` writes it.
            template <typename E>
            static void emit_enum_key(object_ref out)
            {
                using conv = yyjson::detail::enum_conversion<E>;
                const auto names = [&](object_ref o) {
                    auto arr = o.emplace("enum", empty_array);
                    for (const auto name : conv::names) arr.emplace_back(name);
                };
                // The range of the underlying type is not checked by the pattern.
                constexpr auto integer =
                    std::is_signed_v<std::underlying_type_t<E>> ? "^(0|-?[1-9][0-9]*)$" : "^(0|[1-9][0-9]*)$";
                if constexpr (conv::format == enum_format::name)
                    names(out);
                else if constexpr (conv::format == enum_format::integer)
                    out.emplace("pattern", integer);
                else
                {
                    auto any = out.emplace("anyOf", empty_array);
                    names(any.emplace_back(empty_object));
                    any.emplace_back(empty_object).emplace("pattern", integer);
                }
            }

            template <typename E>
            void emit_enum(object_ref out)
            {
                using conv = yyjson::detail::enum_conversion<E>;
                const auto names = [&](object_ref o) {
                    auto arr = o.emplace("enum", empty_array);
                    for (const auto name : conv::names) arr.emplace_back(name);
                };
                if constexpr (conv::format == enum_format::name)
                    names(out);
                else if constexpr (conv::format == enum_format::integer)
                    emit_number<std::underlying_type_t<E>>(out, nullptr);
                else
                {
                    auto any = out.emplace("anyOf", empty_array);
                    names(any.emplace_back(empty_object));
                    emit_number<std::underlying_type_t<E>>(any.emplace_back(empty_object), nullptr);
                }
            }

            // Writes `{"$ref": ...}` for a named type, whose schema `$defs` has.
            template <typename T>
            void emit_named(object_ref out)
            {
                out.emplace("$ref", names_.ref(names_.find_or_add<T>(writing()), writing()), copy_string);
            }

            template <typename S>
            void emit_struct(object_ref out, std::span<const variant_tag> tags = {});
            template <typename S>
            void emit_array_struct(object_ref out);
            template <typename S, std::size_t K>
            bool emit_field_value(object_ref p, const schema_node* nodes, const metadata& m);
            template <typename S, std::size_t K>
            [[nodiscard]] bool field_required() const;
            template <typename S, std::size_t K>
            void emit_field_default(object_ref p, const S* defaults, bool rejects);
            template <typename S, std::size_t K>
            void emit_field_metadata(object_ref p, const metadata& m);
            template <typename S>
            void emit_conditions(object_ref out, std::optional<array_ref>& all_of, std::span<const variant_tag> tags);
            template <typename S>
            void emit_pattern_fields(object_ref out, const schema_node* nodes);
            template <typename S>
            void emit_property_names(object_ref out, object_ref properties, const schema_node* nodes);
            template <typename S, std::size_t... Es>
            void emit_condition_part(object_ref out, std::size_t begin, std::size_t end, std::index_sequence<Es...>,
                                     std::span<const std::string_view> closed = {});
            template <typename S>
            void emit_transparent(object_ref out, std::span<const variant_tag> tags);
            template <typename F, typename Tags, std::size_t... Is>
            void emit_adjacent(array_ref one, std::string_view key, std::optional<std::size_t> held,
                               std::index_sequence<Is...>);

            template <typename V>
            void emit_variant(object_ref out);
            template <typename V>
            void emit_combined(object_ref out);
            template <typename V, std::size_t I>
            void emit_alternative(object_ref out);
            template <typename V>
            void emit_tagged_variant(object_ref out, std::span<const variant_tag> outer);
            template <typename A>
            void emit_tagged(object_ref out, std::span<const variant_tag> tags, const schema_node* nodes = nullptr,
                             std::size_t node = 0);
            // Writes the tags into `properties` and their keys into `required`.  A key that two tags have takes the
            // id of the first; it fails when the ids differ, as no string is both.
            static void emit_tags(object_ref properties, std::vector<std::string_view>& required,
                                  std::span<const variant_tag> tags)
            {
                for (std::size_t i = 0; i < tags.size(); ++i)
                {
                    const auto first = std::ranges::find(tags, tags[i].key, &variant_tag::key);
                    if (first == tags.begin() + static_cast<std::ptrdiff_t>(i))
                    {
                        properties.emplace(tags[i].key, empty_object).emplace("const", tags[i].id);
                        required.push_back(tags[i].key);
                    }
                    else if (first->id != tags[i].id)
                    {
                        auto p = *properties.find(tags[i].key)->second.as_object();
                        if (!p.contains("not")) p.emplace("not", empty_object);
                    }
                }
            }

            template <typename T>
            void emit_range(object_ref out, const schema_node* nodes, std::size_t node);

            template <typename T, std::size_t... Is>
            void emit_tuple(object_ref out, const schema_node* nodes, std::size_t node, std::index_sequence<Is...>);
        };

        template <typename T>
        std::size_t registry::find_or_add(bool writing)
        {
            std::size_t i = 0;
            while (i < types.size() && types[i].id != &type_identity_tag<T>) ++i;
            if (i == types.size())
                types.push_back({&type_identity_tag<T>, schema_name_of<T>(), yyjson::detail::type_name<T>(),
                                 &generator::emit_definition<T>});
            if (!types[i].used[writing])
            {
                types[i].used[writing] = true;
                order.emplace_back(i, writing);
            }
            return i;
        }

        template <typename T>
        void generator::emit(object_ref out, const schema_node* nodes, std::size_t node)
        {
            if constexpr (std::same_as<T, std::remove_cvref_t<T>>)
                if (!named<T>() && emit_declared_type<T>(out))
                {
                    // The declared schema replaces the derived one with the constraints of the field.
                    emit_type_metadata<T>(out);
                    return;
                }
            const schema_node* n = nodes != nullptr ? &nodes[node] : nullptr;
            const auto constraints = [&] {
                if (n != nullptr) emit_constraints(out, n->constraints, yyjson::detail::multi_kind<T>);
            };
            if constexpr (!std::same_as<T, std::remove_cvref_t<T>>)
                emit<std::remove_cvref_t<T>>(out, nodes, node);
            else if constexpr (std::same_as<T, json_view> || std::same_as<T, writer::value> ||
                               std::same_as<T, reader::value>)
                constraints();
            // The settings of a DOM object or array are those of a map or a range of DOM values.
            else if constexpr (std::same_as<T, writer::object> || std::same_as<T, reader::object>)
            {
                if (n != nullptr && (n->constraints.set != 0 || n->structured()))
                    emit_range<std::map<std::string, writer::value>>(out, nodes, node);
                else
                    out.emplace("type", "object");
            }
            else if constexpr (std::same_as<T, writer::array> || std::same_as<T, reader::array>)
            {
                if (n != nullptr && (n->constraints.set != 0 || n->structured()))
                    emit_range<std::vector<writer::value>>(out, nodes, node);
                else
                    out.emplace("type", "array");
            }
            else if constexpr (std::same_as<T, std::nullptr_t> || std::same_as<T, std::monostate>)
                out.emplace("type", "null");
            else if constexpr (user_read<T> || user_write<T>)
            {
                // A type may have a caster for one direction and be converted by the library in the other.
                if (writing() ? !user_write<T> : !user_read<T>)
                    emit_builtin<T>(out, nodes, node);
                else if (named<T>())
                    emit_named<T>(out);
                else if (writing())
                {
                    if constexpr (user_write<T>) emit_user_write<T>(out);
                }
                else
                {
                    if constexpr (user_read<T>) emit_user_read<T>(out);
                }
            }
            else
                emit_builtin<T>(out, nodes, node);
            if constexpr (std::same_as<T, std::remove_cvref_t<T>>)
            {
                if (n != nullptr && settles<T>()) emit_written_texts<T>(out, n->constraints);
                if (n != nullptr && n->type_test != nullptr) emit_type_test(out, *n->type_test);
                if (n != nullptr && n->negated != 0 && settles<T>()) emit_negated<T>(out, nodes, n->negated);
                if (n != nullptr && n->conditional != 0 && settles<T>())
                    emit_conditional<T>(out, nodes, n->conditional);
                static_assert(std::is_enum_v<T> || struct_type<T> || is_variant<T>::value ||
                                  yyjson::detail::combined_variant<T> || user_read<T> || user_write<T> ||
                                  type_setting_count<T, yyjson::schema_name>() == 0,
                              "yyjson::schema_name names a struct, an enumeration, a variant or a type with a caster");
                if (!named<T>()) emit_type_metadata<T>(out);
            }
        }

        // The constraints of a field hold for the C++ value that a user caster makes, and the caster's conversion
        // is unknown, so they are not emitted for the JSON value on either side.  A type whose caster takes
        // `yyjson::json_view` or an emitter gives only `{}` or its "type" here; `yyjson::schema` on the type or the
        // field declares the rest, and `emit` takes it before coming here.
        template <typename T>
        void generator::emit_user_read(object_ref out)
        {
            if constexpr (yyjson::detail::from_json_translator<T>::castable)
                emit<yyjson::detail::from_json_source<T>>(out, nullptr, 0);
        }

        template <typename T>
        void generator::emit_user_write(object_ref out)
        {
            if constexpr (writer::detail::to_json_inp_usr_defined<T>)
            {
                using ref = writer::detail::to_json_input_ref<T>;
                if constexpr (std::same_as<ref, object_ref>)
                    out.emplace("type", "object");
                else if constexpr (std::same_as<ref, array_ref>)
                    out.emplace("type", "array");
            }
            else
            {
                using W = std::remove_cvref_t<decltype(writer::detail::to_json_wrapper(std::declval<const T&>()))>;
                emit<W>(out, nullptr, 0);
            }
        }

        template <typename T>
        void generator::emit_builtin(object_ref out, const schema_node* nodes, std::size_t node)
        {
            const schema_node* n = nodes != nullptr ? &nodes[node] : nullptr;
            const auto constraints = [&] {
                if (n != nullptr) emit_constraints(out, n->constraints, yyjson::detail::multi_kind<T>);
            };
            if constexpr (std::same_as<T, bool>)
            {
                out.emplace("type", "boolean");
                constraints();
            }
            else if constexpr (std::is_enum_v<T>)
            {
                emit_named<T>(out);
                constraints();
            }
            else if constexpr (std::integral<T> || std::floating_point<T>)
                emit_number<T>(out, n);
            else if constexpr (string_type<T>)
            {
                out.emplace("type", "string");
                constraints();
            }
            else if constexpr (yyjson::detail::is_std_optional<T>::value || yyjson::detail::owning_pointer<T>)
            {
                using U = std::remove_cvref_t<decltype(*std::declval<T&>())>;
                if (!admits_null(n) || null_in_not(nodes, n) == negated_null::rejected)
                    emit<U>(out, nodes, node);
                else if (only_null(n))
                    emit_null(out, nodes, n);
                else
                {
                    auto any = out.emplace("anyOf", empty_array);
                    emit<U>(any.emplace_back(empty_object), nodes, node);
                    emit_null(any.emplace_back(empty_object), nodes, n);
                }
            }
            else if constexpr (yyjson::detail::indirect_value<T> &&
                               std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                emit<typename T::value_type>(out, nodes, node);
            else if constexpr (yyjson::detail::combined_variant<T>)
            {
                if (named<T>())
                    emit_named<T>(out);
                else
                    emit_combined<T>(out);
                constraints();
            }
            else if constexpr (nullable_variant<T>)
            {
                // The constraints hold for the value that is not null, as through a pointer; a named variant keeps
                // its reference and has them beside it.
                const auto alternatives = [&](auto any) {
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        const auto alternative = [&]<typename A>(object_ref o) {
                            if constexpr (std::same_as<A, std::monostate>)
                                emit_null(o, nodes, n);
                            else
                                emit<A>(o, nodes, node);
                        };
                        (alternative.template operator()<std::variant_alternative_t<Is, T>>(
                             any.emplace_back(empty_object)),
                         ...);
                    }(std::make_index_sequence<2>{});
                };
                if (!admits_null(n) || null_in_not(nodes, n) == negated_null::rejected)
                {
                    // `yyjson::const_`, `yyjson::enum_` or a `yyjson::not_` reject null; only the value is
                    // read.
                    using V = std::variant_alternative_t<std::same_as<std::variant_alternative_t<0, T>, std::monostate>
                                                             ? 1
                                                             : 0,
                                                         T>;
                    emit<V>(out, nodes, node);
                }
                else if (only_null(n))
                    emit_null(out, nodes, n);
                else if (!named<T>())
                    alternatives(out.emplace("anyOf", empty_array));
                else
                {
                    emit_named<T>(out);
                    if (n != nullptr && (n->constraints.set != 0 || n->structured()))
                        alternatives(out.emplace("anyOf", empty_array));
                }
            }
            else if constexpr (is_variant<T>::value)
            {
                if (named<T>())
                    emit_named<T>(out);
                else
                    emit_variant<T>(out);
                constraints();
            }
            else if constexpr (yyjson::detail::transparent_over_optional<T>)
            {
                // `$defs` has the value type of the field; null empties it here, as it empties a `std::optional`.
                auto any = out.emplace("anyOf", empty_array);
                emit_named<T>(any.emplace_back(empty_object));
                any.emplace_back(empty_object).emplace("type", "null");
                constraints();
            }
            else if constexpr (struct_type<T>)
            {
                emit_named<T>(out);
                constraints();
            }
            else if constexpr (yyjson::detail::tuple_like<T> && !std::ranges::range<T>)
                emit_tuple<T>(out, nodes, node, std::make_index_sequence<std::tuple_size_v<T>>{});
            else if constexpr (std::ranges::range<T>)
                emit_range<T>(out, nodes, node);
            else
                out.emplace("$comment", "no schema for this C++ type", copy_string);
        }

        template <typename T>
        void generator::emit_range(object_ref out, const schema_node* nodes, std::size_t node)
        {
            const schema_node* n = nodes != nullptr ? &nodes[node] : nullptr;
            const auto child = [&](std::size_t index) { return index != 0 ? nodes : nullptr; };
            if constexpr (object_range<T>)
            {
                using K = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>;
                using V = yyjson::detail::key_value_mapped_t<T>;
                const auto key_schema = [&](object_ref keys) {
                    if constexpr (std::is_enum_v<K>)
                        emit_enum_key<K>(keys);
                    else if (n != nullptr && n->keys != 0)
                        emit_key_settings(keys, nodes, n->keys);
                };
                const auto value_schema = [&](object_ref values) {
                    emit<V>(values, child(n != nullptr ? n->values : 0), n != nullptr ? n->values : 0);
                };
                const auto object_form = [&](object_ref o) {
                    o.emplace("type", "object");
                    if (n != nullptr)
                    {
                        // min_items and max_items count the entries, which are the properties of the object.
                        auto rest = n->constraints;
                        if (rest.has<yyjson::min_items>()) o.emplace("minProperties", rest.min_items);
                        if (rest.has<yyjson::max_items>()) o.emplace("maxProperties", rest.max_items);
                        rest.set &= ~(yyjson::detail::constraint_bit<yyjson::min_items> |
                                      yyjson::detail::constraint_bit<yyjson::max_items>);
                        emit_constraints(o, rest);
                    }
                    if (std::is_enum_v<K> || (n != nullptr && n->keys != 0))
                        key_schema(o.emplace("propertyNames", empty_object));
                    value_schema(o.emplace("additionalProperties", empty_object));
                };
                // A sequence of key-value pairs is also read from an array of [key, value], which is not declared.
                object_form(out);
            }
            else
            {
                using E = yyjson::detail::array_element_t<T>;
                out.emplace("type", "array");
                constexpr bool fixed = yyjson::detail::std_array<T> || yyjson::detail::c_array<T>;
                if (n != nullptr)
                {
                    auto rest = n->constraints;
                    if (fixed)
                        rest.set &= ~(yyjson::detail::constraint_bit<yyjson::min_items> |
                                      yyjson::detail::constraint_bit<yyjson::max_items>);
                    emit_constraints(out, rest);
                }
                // A multiset keeps repeated elements: only a container whose `insert` can refuse one has unique items.
                if constexpr (yyjson::detail::unique_keys<T>)
                    if (!(n != nullptr && n->constraints.has<yyjson::unique_items_t>()))
                        out.emplace("uniqueItems", true);
                if constexpr (fixed)
                {
                    // The array holds exactly its size, which `min_items` and `max_items` can only narrow further.
                    constexpr std::size_t size = [] {
                        if constexpr (yyjson::detail::c_array<T>)
                            return std::extent_v<T>;
                        else
                            return std::tuple_size_v<T>;
                    }();
                    auto min = size;
                    auto max = size;
                    if (n != nullptr && n->constraints.has<yyjson::min_items>())
                        min = std::max(min, n->constraints.min_items);
                    if (n != nullptr && n->constraints.has<yyjson::max_items>())
                        max = std::min(max, n->constraints.max_items);
                    out.emplace("minItems", min);
                    out.emplace("maxItems", max);
                }
                if (n != nullptr && n->prefix_count != 0)
                {
                    auto prefix = out.emplace("prefixItems", empty_array);
                    for (std::size_t i = 0; i < n->prefix_count; ++i)
                        emit<E>(prefix.emplace_back(empty_object), nodes, n->prefix + i);
                }
                auto items = out.emplace("items", empty_object);
                emit<E>(items, child(n != nullptr ? n->items : 0), n != nullptr ? n->items : 0);
                if (n != nullptr && n->contains != 0)
                    emit<E>(out.emplace("contains", empty_object), nodes, n->contains);
            }
        }

        template <typename T, std::size_t... Is>
        void generator::emit_tuple(object_ref out, const schema_node* nodes, std::size_t node,
                                   std::index_sequence<Is...>)
        {
            const schema_node* n = nodes != nullptr ? &nodes[node] : nullptr;
            if constexpr (sizeof...(Is) == 0)
            {
                // An empty tuple is a tuple of key-value pairs, too: it is written as an empty object, and read from
                // an empty array as well.
                out.emplace("type", "object");
                out.emplace("maxProperties", 0);
                return;
            }
            else if constexpr ((key_value_element<std::tuple_element_t<Is, T>> && ...))
            {
                // An object of a member for each pair, in order.  JSON Schema cannot tie a member to its position, so
                // a member written satisfies the schema of one of the positions, and a member read has to satisfy
                // those of all of them.  When the values differ in type, no value satisfies them all, and the read
                // schema takes any of them instead, which accepts members that the read rejects.
                // The node of the key or the value of the pair at position i, or 0 when it has no constraints.
                const auto part = [&](std::size_t i, std::size_t k) -> std::size_t {
                    if (n == nullptr || i >= n->prefix_count) return 0;
                    const auto& pair = nodes[n->prefix + i];
                    return k < pair.prefix_count ? pair.prefix + k : 0;
                };
                using V0 = std::remove_cvref_t<std::tuple_element_t<1, std::tuple_element_t<0, T>>>;
                constexpr bool same_values =
                    (std::same_as<std::remove_cvref_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>, V0> &&
                     ...);
                const bool keys_constrained = ((part(Is, 0) != 0) || ...);
                const bool values_constrained = ((part(Is, 1) != 0) || ...);
                out.emplace("type", "object");
                out.emplace("minProperties", sizeof...(Is));
                out.emplace("maxProperties", sizeof...(Is));
                if (keys_constrained)
                {
                    auto each =
                        out.emplace("propertyNames", empty_object).emplace(writing() ? "anyOf" : "allOf", empty_array);
                    (emit<std::string>(each.emplace_back(empty_object), part(Is, 0) != 0 ? nodes : nullptr,
                                       part(Is, 0)),
                     ...);
                }
                auto values = out.emplace("additionalProperties", empty_object);
                if (!values_constrained && same_values)
                    emit<V0>(values);
                else
                {
                    auto each = values.emplace(writing() || !same_values ? "anyOf" : "allOf", empty_array);
                    (emit<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>(
                         each.emplace_back(empty_object), part(Is, 1) != 0 ? nodes : nullptr, part(Is, 1)),
                     ...);
                }
            }
            else
            {
                out.emplace("type", "array");
                if (n != nullptr) emit_constraints(out, n->constraints);
                auto prefix = out.emplace("prefixItems", empty_array);
                (
                    [&] {
                        const bool constrained = n != nullptr && Is < n->prefix_count;
                        emit<std::tuple_element_t<Is, T>>(prefix.emplace_back(empty_object),
                                                          constrained ? nodes : nullptr,
                                                          constrained ? n->prefix + Is : 0);
                    }(),
                    ...);
                out.emplace("minItems", sizeof...(Is));
                out.emplace("items", false);
            }
        }

        template <typename V>
        void generator::emit_variant(object_ref out)
        {
            using tags = yyjson::detail::variant_tags<V>;
            if constexpr (tags::tagging == detail::variant_tagging::internal)
                emit_tagged_variant<V>(out, {});
            else
            {
                // An adjacent tag is written by the struct that holds the variant.
                auto any = out.emplace("anyOf", empty_array);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (emit<std::variant_alternative_t<Is, V>>(any.emplace_back(empty_object)), ...);
                }(std::make_index_sequence<std::variant_size_v<V>>{});
            }
        }

        // `yyjson::any_of` as `anyOf` and `yyjson::one_of` as `oneOf`: each alternative is the schema of its type
        // with its settings and its metadata, in both directions.
        template <typename V>
        void generator::emit_combined(object_ref out)
        {
            using table = typename yyjson::detail::combined_alternatives<V>::table;
            auto list = out.emplace(yyjson::detail::combined_alternatives<V>::exactly_one ? "oneOf" : "anyOf",
                                    empty_array);
            const auto* const outer_values = std::exchange(values_, table::tree.values.data());
            const auto* const outer_nodes = std::exchange(nodes_, table::tree.nodes.data());
            const auto tests = type_tests_for<V>();
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (emit_alternative<V, Is>(list.emplace_back(empty_object)), ...);
            }(std::make_index_sequence<std::variant_size_v<V>>{});
            values_ = outer_values;
            nodes_ = outer_nodes;
        }

        template <typename V, std::size_t I>
        void generator::emit_alternative(object_ref out)
        {
            using A = typename combined_alternative<V, I>::type;
            using T = std::variant_alternative_t<I, V>;
            if constexpr (yyjson::detail::constrained_alternative<V, I>)
                emit<T>(out, yyjson::detail::combined_alternatives<V>::table::tree.nodes.data(), I);
            else
                emit<T>(out);
            auto m = metadata();
            yyjson::detail::alternative_traits<A>::for_each([&]<typename S>(const S& setting) {
                if constexpr (yyjson::detail::metadata_setting<S>) m.add(setting);
            });
            if (m.empty()) return;
            const auto check = [](const std::optional<T>& v) {
                if constexpr (yyjson::detail::constrained_alternative<V, I>)
                    return yyjson::detail::level_violation<yyjson::detail::alternative_level<V, I>>(*v) ==
                           yyjson::detail::constraint_kind::none;
                else
                    return v.has_value();
            };
            emit_metadata<T, alternative_has_examples<A>>(
                out, m,
                std::string("the alternative ").append(std::to_string(I)).append(" of ").append(
                    yyjson::detail::type_name<V>()),
                check);
        }

        // A variant tagged by `internal_tag`, in an object that the variants in `outer` tag too when it is an
        // alternative of theirs.  The alternatives are written in place, since a closed struct has to know the keys
        // of the tags, so a variant that is reached again within them would be written without end: it needs a
        // name, which puts it in `$defs`.
        template <typename V>
        void generator::emit_tagged_variant(object_ref out, std::span<const variant_tag> outer)
        {
            using tags = yyjson::detail::variant_tags<V>;
            if (std::ranges::find(inlined_, &type_identity_tag<V>) != inlined_.end())
                throw std::invalid_argument(
                    std::string("the schema of ")
                        .append(yyjson::detail::type_name<V>())
                        .append(" refers to itself through an alternative that yyjson::internal_tag tags; give the "
                                "variant a yyjson::schema_name, which puts it in $defs"));
            inlined_.push_back(&type_identity_tag<V>);
            struct pop
            {
                std::vector<const void*>& inlined;
                ~pop() { inlined.pop_back(); }
            } const guard{inlined_};

            auto list = std::vector<variant_tag>(outer.begin(), outer.end());
            list.push_back({tags::key, {}});
            auto one = out.emplace("oneOf", empty_array);
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (
                    [&] {
                        using A = std::variant_alternative_t<Is, V>;
                        auto branch = one.emplace_back(empty_object);
                        if constexpr (Is == tags::monostate)
                        {
                            // Null, without a tag; an object that an outer variant tags is never read as it.
                            if (outer.empty())
                                branch.emplace("type", "null");
                            else
                                branch.emplace("not", empty_object);
                        }
                        else
                        {
                            list.back().id = tags::ids[Is];
                            emit_tagged<A>(branch, list);
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<std::variant_size_v<V>>{});
        }

        // An alternative A in the object that `tags` tag.  The read takes the alternative from the whole object, and
        // the write adds the tags to what A writes, so a struct is written in place with the tags among its
        // properties, through a pointer, a translator or another variant tagged in the same object.  Anything else
        // takes the tags beside its own schema, which has to allow their keys.
        //
        // `nodes` and `node` are the settings of the field of a struct annotated with `yyjson::transparent`, which
        // A is the type of.  A struct and a variant take none that a schema has.
        template <typename A>
        void generator::emit_tagged(object_ref out, std::span<const variant_tag> tags, const schema_node* nodes,
                                    std::size_t node)
        {
            using T = std::remove_cvref_t<A>;
            const auto beside = [&] {
                auto all = out.emplace("allOf", empty_array);
                emit<T>(all.emplace_back(empty_object), nodes, node);
                auto tag = all.emplace_back(empty_object);
                auto required = std::vector<std::string_view>();
                emit_tags(tag.emplace("properties", empty_object), required, tags);
                auto r = tag.emplace("required", empty_array);
                for (const auto k : required) r.emplace_back(k);
            };
            if (declares<T>()) return beside();
            if constexpr (user_read<T> || user_write<T>)
            {
                if (writing())
                {
                    if constexpr (user_write<T> && !writer::detail::to_json_inp_usr_defined<T>)
                    {
                        emit_tagged<decltype(writer::detail::to_json_wrapper(std::declval<const T&>()))>(out, tags);
                        return emit_type_metadata<T>(out);
                    }
                    else if constexpr (user_write<T>)
                        return beside();
                }
                else
                {
                    if constexpr (user_read<T> && yyjson::detail::from_json_translator<T>::castable)
                    {
                        emit_tagged<yyjson::detail::from_json_source<T>>(out, tags);
                        return emit_type_metadata<T>(out);
                    }
                    else if constexpr (user_read<T>)
                        return beside();
                }
            }
            if constexpr (yyjson::detail::is_std_optional<T>::value || yyjson::detail::owning_pointer<T> ||
                          yyjson::detail::indirect_value<T>)
                // Null is not an object, which the tags need.
                emit_tagged<decltype(*std::declval<T&>())>(out, tags, nodes, node);
            else if constexpr (yyjson::detail::transparent_struct<T>)
            {
                emit_transparent<T>(out, tags);
                emit_type_metadata<T>(out);
            }
            else if constexpr (yyjson::detail::as_array_struct<T>)
                // An array, which the tags cannot be in.
                beside();
            else if constexpr (struct_type<T>)
            {
                emit_struct<T>(out, tags);
                emit_type_metadata<T>(out);
            }
            else if constexpr (yyjson::detail::internally_tagged<T>)
            {
                emit_tagged_variant<T>(out, tags);
                emit_type_metadata<T>(out);
            }
            else
                beside();
        }

        // The branches of a variant with an adjacent tag, which are exclusive: one for each alternative, and when
        // reading, the object without the tag and the member, and the tag of `held`, the alternative in `S()`, without
        // the member.  The field keeps its value in `S()` in these two.
        template <typename F, typename Tags, std::size_t... Is>
        void generator::emit_adjacent(array_ref one, std::string_view key, std::optional<std::size_t> held,
                                      std::index_sequence<Is...>)
        {
            (
                [&] {
                    using A = std::variant_alternative_t<Is, F>;
                    auto branch = one.emplace_back(empty_object);
                    auto bp = branch.emplace("properties", empty_object);
                    if constexpr (Is == Tags::monostate)
                    {
                        bp.emplace(key, empty_object).emplace("type", "null");
                        branch.emplace("required", empty_array).emplace_back(key);
                        branch.emplace("not", empty_object).emplace("required", empty_array).emplace_back(Tags::key);
                    }
                    else
                    {
                        bp.emplace(Tags::key, empty_object).emplace("const", Tags::ids[Is]);
                        emit<A>(bp.emplace(key, empty_object));
                        auto r = branch.emplace("required", empty_array);
                        r.emplace_back(Tags::key);
                        r.emplace_back(key);
                    }
                }(),
                ...);
            if (writing() || !held) return;
            if (*held != Tags::monostate)
            {
                auto branch = one.emplace_back(empty_object);
                branch.emplace("properties", empty_object)
                    .emplace(Tags::key, empty_object)
                    .emplace("const", Tags::ids[*held]);
                branch.emplace("required", empty_array).emplace_back(Tags::key);
                branch.emplace("not", empty_object).emplace("required", empty_array).emplace_back(key);
            }
            auto absent = one.emplace_back(empty_object).emplace("not", empty_object).emplace("anyOf", empty_array);
            absent.emplace_back(empty_object).emplace("required", empty_array).emplace_back(Tags::key);
            absent.emplace_back(empty_object).emplace("required", empty_array).emplace_back(key);
        }

        // A struct annotated with `yyjson::transparent` is the value of its field, with the settings of the field.  An
        // optional field gives its value type, as a `std::optional` field does: the struct takes null beside it as an
        // element, a value of a map or the root, where null empties the field.  A tagged alternative is the object
        // only.
        template <typename S>
        void generator::emit_transparent(object_ref out, std::span<const variant_tag> tags)
        {
            using F = yyjson::detail::struct_field_type<S, 0>;
            using V = std::conditional_t<yyjson::detail::is_std_optional<F>::value, typename optional_value<F>::type, F>;
            constexpr auto node = yyjson::detail::struct_declared_index<S, 0>;
            const schema_node* nodes = nullptr;
            const yyjson::detail::json_scalar* values = nullptr;
            if constexpr (yyjson::detail::constrained_field<S, 0>)
            {
                nodes = yyjson::detail::field_table_of<S>::table.schemas.nodes.data();
                values = yyjson::detail::field_table_of<S>::table.schemas.values.data();
            }
            const auto* const outer_values = std::exchange(values_, values);
            const auto* const outer_nodes = std::exchange(nodes_, nodes);
            const auto tests = type_tests_for<S>();
            if (tags.empty())
                emit<std::conditional_t<yyjson::detail::transparent_over_optional<S>, V, F>>(out, nodes, node);
            else
                emit_tagged<V>(out, tags, nodes, node);
            values_ = outer_values;
            nodes_ = outer_nodes;
        }

        template <typename S>
        void generator::emit_struct(object_ref out, std::span<const variant_tag> tags)
        {
            using rules = yyjson::detail::struct_key_rules<S>;
            using additional = yyjson::detail::additional_keys;
            constexpr auto count = yyjson::detail::struct_field_count<S>;
            constexpr const schema_node* nodes = [] {
                if constexpr (yyjson::detail::field_table_of<S>::exists)
                    return yyjson::detail::field_table_of<S>::table.schemas.nodes.data();
                else
                    return nullptr;
            }();

            const auto* const outer_values = std::exchange(values_, [] {
                if constexpr (yyjson::detail::field_table_of<S>::exists)
                    return yyjson::detail::field_table_of<S>::table.schemas.values.data();
                else
                    return static_cast<const yyjson::detail::json_scalar*>(nullptr);
            }());
            const auto* const outer_nodes = std::exchange(nodes_, nodes);
            const auto tests = type_tests_for<S>();
            out.emplace("type", "object");
            auto properties = out.emplace("properties", empty_object);
            auto required = std::vector<std::string_view>();
            emit_tags(properties, required, tags);
            auto adjacent = std::optional<array_ref>();
            [[maybe_unused]] const S* const defaults = defaults_of<S>();

            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                [[maybe_unused]] const bool done[] = {
                    true,
                    (
                        [&] {
                            using F = yyjson::detail::struct_field_type<S, Ks>;
                            constexpr auto key = yyjson::detail::struct_field_key<S, Ks>;
                            constexpr auto declared = yyjson::detail::struct_declared_index<S, Ks>;
                            const auto m = field_metadata<S, declared>();
                            // A field that has the key of a tag is read from the tag, whose id it must accept, and is
                            // written only when it holds the id.
                            const bool tagged = std::ranges::find(tags, key, &variant_tag::key) != tags.end();
                            auto p = tagged ? properties.find(key)
                                                  ->second.as_object()
                                                  ->emplace("allOf", empty_array)
                                                  .emplace_back(empty_object)
                                            : properties.emplace(key, empty_object);
                            [[maybe_unused]] bool rejects = false;
                            if constexpr (yyjson::detail::adjacent_field<S, Ks>)
                            {
                                static_assert(!field_has_schema<S, declared>(),
                                              "yyjson::schema is not for a field with yyjson::adjacent_tag");
                                // The variant and its tag: one branch for each alternative.
                                using tags = yyjson::detail::struct_field_tags<S, Ks>;
                                if constexpr (yyjson::detail::writes_adjacent_tag<S, tags>)
                                {
                                    auto ids = properties.emplace(tags::key, empty_object).emplace("enum", empty_array);
                                    for (const auto id : tags::ids)
                                        if (!id.empty()) ids.emplace_back(id);
                                }
                                if (!adjacent) adjacent = out.emplace("allOf", empty_array);
                                const auto held = [&]() -> std::optional<std::size_t> {
                                    if (defaults == nullptr) return std::nullopt;
                                    return yyjson::detail::struct_field<Ks>(*defaults).index();
                                }();
                                emit_adjacent<F, tags>(
                                    adjacent->emplace_back(empty_object).emplace("oneOf", empty_array), key, held,
                                    std::make_index_sequence<std::variant_size_v<F>>{});
                            }
                            else
                                rejects = emit_field_value<S, Ks>(p, nodes, m);

                            if (tagged)
                                ;
                            else if (field_required<S, Ks>())
                                required.push_back(key);
                            else if constexpr (!yyjson::detail::adjacent_field<S, Ks>)
                                emit_field_default<S, Ks>(p, defaults, rejects);
                            if (!m.empty()) emit_field_metadata<S, Ks>(p, m);
                        }(),
                        true)...};
            }(std::make_index_sequence<count>{});

            if (!required.empty())
            {
                auto r = out.emplace("required", empty_array);
                for (const auto k : required) r.emplace_back(k);
            }
            if constexpr (!rules::dependencies.empty())
            {
                auto dependent = out.emplace("dependentRequired", empty_object);
                for (const auto& [k, j] : rules::dependencies)
                {
                    auto keys = dependent.find(rules::keys[k]);
                    auto list = keys != dependent.end() ? *keys->second.as_array()
                                                        : dependent.emplace(rules::keys[k], empty_array);
                    list.emplace_back(rules::keys[j]);
                }
            }
            // The keys of the object, unknown keys and tags too, in both directions.
            if constexpr (rules::min_properties != 0) out.emplace("minProperties", rules::min_properties);
            if constexpr (rules::max_properties != std::numeric_limits<std::size_t>::max())
                out.emplace("maxProperties", rules::max_properties);
            if constexpr (!rules::groups.empty())
            {
                // `oneOf` or `anyOf` of the groups, in `allOf` when the struct has one already
                for (std::size_t g = 0; g < rules::groups.size();)
                {
                    const auto first = g;
                    const std::string_view keyword = rules::groups[first].exactly_one ? "oneOf" : "anyOf";
                    auto branches = [&] {
                        if (out.find(keyword) == out.end()) return out.emplace(keyword, empty_array);
                        if (!adjacent) adjacent = out.emplace("allOf", empty_array);
                        return adjacent->emplace_back(empty_object).emplace(keyword, empty_array);
                    }();
                    for (; g < rules::groups.size() && rules::groups[g].choice == rules::groups[first].choice; ++g)
                    {
                        auto keys = branches.emplace_back(empty_object).emplace("required", empty_array);
                        for (auto f = rules::groups[g].begin; f < rules::groups[g].end; ++f)
                            keys.emplace_back(rules::keys[rules::group_fields[f]]);
                    }
                }
            }
            if constexpr (yyjson::detail::has_conditions<S>) emit_conditions<S>(out, adjacent, tags);
            if constexpr (yyjson::detail::has_pattern_fields<S>) emit_pattern_fields<S>(out, nodes);
            if constexpr (yyjson::detail::has_pattern_fields<S> || yyjson::detail::has_extra_keys<S>)
                emit_property_names<S>(out, properties, nodes);
            if constexpr (yyjson::detail::has_extra_keys<S>)
            {
                // The keys that no field has and no pattern matches go to the field of `yyjson::extra_keys`, in
                // both directions and with `yyjson::strict` too, so the other keys take the schema of its values.
                constexpr auto e = yyjson::detail::extra_keys_index<S>;
                using M = std::remove_cvref_t<decltype(yyjson::detail::extra_keys_field(std::declval<S&>()))>;
                const auto v = nodes != nullptr ? nodes[e].values : 0;
                emit<typename M::mapped_type>(out.emplace("additionalProperties", empty_object),
                                              v != 0 ? nodes : nullptr, v);
                values_ = outer_values;
                nodes_ = outer_nodes;
                return;
            }
            values_ = outer_values;
            nodes_ = outer_nodes;
            const bool closed = writing() || rules::additional == additional::denied ||
                                (strict() && rules::additional != additional::allowed);
            // The keys of the branches of `allOf` are in `properties` too, so `additionalProperties` knows them.
            if (closed) out.emplace("additionalProperties", false);
        }

        // The fields of S annotated with `yyjson::pattern_properties` as `patternProperties`: every key that a
        // pattern matches takes the schema of the values of its field, a key of a field too, as reading checks
        // them.  Two fields of the same pattern give the schemas of both in `allOf`.
        template <typename S>
        void generator::emit_pattern_fields(object_ref out, const schema_node* nodes)
        {
            auto patterns = out.emplace("patternProperties", empty_object);
            [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                (
                    [&] {
                        using M = yyjson::detail::pattern_map_t<S, Ps>;
                        constexpr auto key = yyjson::detail::pattern_of<S, Ps>;
                        const auto v = yyjson::detail::pattern_node<S, Ps>.values;
                        auto target = [&] {
                            const auto found = patterns.find(key);
                            if (found == patterns.end()) return patterns.emplace(key, empty_object);
                            auto earlier = *found->second.as_object();
                            const auto all = earlier.find("allOf");
                            auto list = all != earlier.end() ? *all->second.as_array()
                                                             : earlier.emplace("allOf", empty_array);
                            return list.emplace_back(empty_object);
                        }();
                        emit<typename M::mapped_type>(target, v != 0 ? nodes : nullptr, v);
                    }(),
                    ...);
            }(std::make_index_sequence<yyjson::detail::pattern_count<S>>{});
        }

        // The settings of `yyjson::property_names` of the fields of `yyjson::pattern_properties` and
        // `yyjson::extra_keys` of S as `propertyNames`, which checks every key: those of the fields and the tags,
        // in `properties`, pass it as they are, as reading does not check them.  The keys of a field of a pattern
        // are those that the pattern matches, wherever they go, and those of `yyjson::extra_keys` are those that
        // no pattern matches.  Several of them go to `allOf`.
        template <typename S>
        void generator::emit_property_names(object_ref out, object_ref properties, const schema_node* nodes)
        {
            constexpr auto patterns = yyjson::detail::pattern_count<S>;
            const auto extra_keys = [&]() -> std::size_t {
                if constexpr (yyjson::detail::has_extra_keys<S>)
                    return nodes != nullptr ? nodes[yyjson::detail::extra_keys_index<S>].keys : 0;
                else
                    return 0;
            }();
            const auto rules = [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                return (std::size_t{extra_keys != 0} + ... +
                        std::size_t{yyjson::detail::constrained_pattern_keys<S, Ps>});
            }(std::make_index_sequence<patterns>{});
            if (rules == 0) return;
            auto names = out.emplace("propertyNames", empty_object);
            auto all = rules > 1 ? std::optional(names.emplace("allOf", empty_array)) : std::nullopt;
            // The branches of one of them, after the keys of the fields and the tags.
            const auto branches = [&] {
                auto rule = all ? all->emplace_back(empty_object) : names;
                auto list = rule.emplace("anyOf", empty_array);
                if (properties.size() != 0)
                {
                    auto known = list.emplace_back(empty_object).emplace("enum", empty_array);
                    for (const auto& [key, _] : properties) known.emplace_back(std::string_view(key));
                }
                return list;
            };
            [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                (
                    [&] {
                        if constexpr (yyjson::detail::constrained_pattern_keys<S, Ps>)
                        {
                            auto list = branches();
                            list.emplace_back(empty_object)
                                .emplace("not", empty_object)
                                .emplace("pattern", yyjson::detail::pattern_of<S, Ps>);
                            emit_key_settings(list.emplace_back(empty_object), nodes,
                                              yyjson::detail::pattern_node<S, Ps>.keys);
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<patterns>{});
            if (extra_keys != 0 && patterns == 0 && properties.size() == 0)
                emit_key_settings(names, nodes, extra_keys);
            else if (extra_keys != 0)
            {
                auto list = branches();
                [&]<std::size_t... Ps>(std::index_sequence<Ps...>) {
                    constexpr std::array<std::string_view, patterns> keys{yyjson::detail::pattern_of<S, Ps>...};
                    for (auto p = keys.begin(); p != keys.end(); ++p)
                        if (std::find(keys.begin(), p, *p) == p) list.emplace_back(empty_object).emplace("pattern", *p);
                }(std::make_index_sequence<patterns>{});
                emit_key_settings(list.emplace_back(empty_object), nodes, extra_keys);
            }
        }

        // Writes the fields of the entries from `begin` to `end` of the conditions of S as `properties`, with the
        // keys that the entries require as `required`.  A field that the part names again goes to `allOf`.  A part
        // with `yyjson::additional_properties(false)` has the keys of its tags in `closed`; it names every field
        // that it has and those keys in `properties`, and rejects the other keys.
        template <typename S, std::size_t... Es>
        void generator::emit_condition_part(object_ref out, std::size_t begin, std::size_t end,
                                            std::index_sequence<Es...>, std::span<const std::string_view> closed)
        {
            using table = yyjson::detail::condition_table<S>;
            auto properties = std::optional<object_ref>();
            auto required = std::vector<std::string_view>();
            auto again = std::optional<array_ref>();
            const bool closes = closed.data() != nullptr;
            const auto name = [&](std::string_view key) {
                if (!properties) properties = out.emplace("properties", empty_object);
                if (!properties->contains(key)) properties->emplace(key, empty_object);
            };
            (
                [&] {
                    if (Es < begin || Es >= end) return;
                    using entry = yyjson::detail::condition_entry_of<S, Es>;
                    using F = typename entry::type;
                    using V = typename optional_value<F>::type;
                    using R = typename entry::type_test;
                    constexpr auto key = yyjson::detail::struct_field_key<S, entry::field>;
                    if (entry::entry.required && std::ranges::find(required, key) == required.end())
                        required.push_back(key);
                    // A field that only `yyjson::required` names has no schema of its own.
                    if constexpr (std::is_void_v<R> && !entry::constrained)
                    {
                        if (closes) name(key);
                        return;
                    }
                    auto p = [&] {
                        if (!properties) properties = out.emplace("properties", empty_object);
                        if (!properties->contains(key)) return properties->emplace(key, empty_object);
                        if (!again) again = out.emplace("allOf", empty_array);
                        return again->emplace_back(empty_object)
                            .emplace("properties", empty_object)
                            .emplace(key, empty_object);
                    }();
                    const auto* const nodes = table::tree.nodes.data();
                    if constexpr (!std::is_void_v<R>)
                    {
                        if constexpr (entry::constrained)
                        {
                            auto all = p.emplace("allOf", empty_array);
                            emit<V>(all.emplace_back(empty_object), nodes, Es);
                            emit_read<R>(all.emplace_back(empty_object));
                        }
                        else
                            emit_read<R>(p);
                    }
                    else
                        emit<V>(p, nodes, Es);
                }(),
                ...);
            if (closes)
            {
                for (const auto k : closed) name(k);
                out.emplace("additionalProperties", false);
            }
            if (!required.empty())
            {
                auto r = out.emplace("required", empty_array);
                for (const auto k : required) r.emplace_back(k);
            }
        }

        // Writes the schema of the value of the field K of S into `p`, by its index among the fields that are read,
        // unless it has `yyjson::adjacent_tag`.  Returns whether the schema that `yyjson::schema` declares for it
        // rejects every value.
        template <typename S, std::size_t K>
        bool generator::emit_field_value(object_ref p, const schema_node* nodes, const metadata& m)
        {
            using F = yyjson::detail::struct_field_type<S, K>;
            constexpr auto node = yyjson::detail::struct_declared_index<S, K>;
            const auto* field_nodes = yyjson::detail::constrained_field<S, K> ? nodes : nullptr;
            if (const auto* d = m.declared_for(writing()))
                return emit_declared(p, d->text(writing()), member_of<S, node>());
            if constexpr (yyjson::detail::is_std_optional<F>::value)
                // A field without a value is left out of the object, which is how the type declares that it may be
                // absent.  Null is read as the value type reads it.
                emit<typename F::value_type>(p, field_nodes, node);
            else if constexpr (yyjson::detail::transparent_over_optional<F>)
            {
                // The same for a struct whose field is such a `std::optional`, which `$defs` defines by the value
                // type.
                emit_named<F>(p);
                if (field_nodes != nullptr)
                {
                    emit_constraints(p, field_nodes[node].constraints);
                    if (field_nodes[node].negated != 0) emit_negated<F>(p, field_nodes, field_nodes[node].negated);
                    if (field_nodes[node].conditional != 0)
                        emit_conditional<F>(p, field_nodes, field_nodes[node].conditional);
                }
            }
            else
                emit<F>(p, field_nodes, node);
            return false;
        }

        // Whether the field K of S has to be in the object: the write of S never leaves it out, and the read
        // requires it.
        template <typename S, std::size_t K>
        bool generator::field_required() const
        {
            using F = yyjson::detail::struct_field_type<S, K>;
            using rules = yyjson::detail::struct_key_rules<S>;
            if (writing())
                return !yyjson::detail::omittable<F> &&
                       !yyjson::detail::omits_default<S, yyjson::detail::struct_declared_index<S, K>>;
            return rules::required[K] || (strict() && !rules::optional[K]);
        }

        // Writes the value that reading gives the field K of S when the object leaves out its key, from `defaults`,
        // which is `S()` or null, as the `default` of `p`, unless `rejects` or the read does not take it back.
        template <typename S, std::size_t K>
        void generator::emit_field_default(object_ref p, [[maybe_unused]] const S* defaults,
                                           [[maybe_unused]] bool rejects)
        {
            using F = yyjson::detail::struct_field_type<S, K>;
            constexpr auto declared = yyjson::detail::struct_declared_index<S, K>;
            if constexpr (std::default_initializable<S> && writable<F>)
            {
                const auto emit_default = [&](const F& d) {
                    const bool omitted = [&] {
                        if constexpr (yyjson::detail::omittable<F>)
                            return yyjson::detail::should_omit_field(d);
                        else
                            return false;
                    }();
                    if (!omitted && !rejects && reads_back<S, K>(d)) p.emplace("default", d, copy_string);
                };
                if (defaults == nullptr)
                    ;
                else if constexpr (yyjson::detail::has_default_from<S, declared>)
                    emit_default(yyjson::detail::default_field_value<S, declared, F>());
                else
                    emit_default(yyjson::detail::struct_field<K>(*defaults));
            }
        }

        // Writes the metadata `m` of the field K of S into `p`.
        template <typename S, std::size_t K>
        void generator::emit_field_metadata(object_ref p, const metadata& m)
        {
            using F = yyjson::detail::struct_field_type<S, K>;
            constexpr auto declared = yyjson::detail::struct_declared_index<S, K>;
            using E = std::conditional_t<yyjson::detail::is_std_optional<F>::value &&
                                             !yyjson::detail::adjacent_field<S, K>,
                                         typename optional_value<F>::type, F>;
            // F is E, or the std::optional<E> that holds the example
            const auto check = [](const std::optional<E>& v) {
                if constexpr (std::same_as<E, F>)
                    return satisfies<S, K>(*v);
                else
                    return satisfies<S, K>(v);
            };
            emit_metadata<E, field_has_examples<S, declared>()>(p, m, member_of<S, declared>(), check);
        }

        // A struct annotated with `yyjson::as_array` is an array of its fields in the order of their declarations,
        // in `prefixItems`.  The fields that the read requires, or that the write never leaves out, are at least as
        // many as `minItems` says: those before the last of them are written too, holding a value or null.  The
        // elements after them are those of the field annotated with `yyjson::extra_items`; a struct without it takes
        // none by `items`, unless `yyjson::items(true)` or a read without `yyjson::strict` takes and drops them.
        // The settings of the whole array, such as `yyjson::min_items` or `yyjson::contains`, follow.
        template <typename S>
        void generator::emit_array_struct(object_ref out)
        {
            using additional = yyjson::detail::additional_keys;
            constexpr auto count = yyjson::detail::struct_field_count<S>;
            constexpr const schema_node* nodes = yyjson::detail::field_table_of<S>::table.schemas.nodes.data();
            const auto* const outer_values =
                std::exchange(values_, yyjson::detail::field_table_of<S>::table.schemas.values.data());
            const auto* const outer_nodes = std::exchange(nodes_, nodes);
            const auto tests = type_tests_for<S>();
            out.emplace("type", "array");
            [[maybe_unused]] const S* const defaults = defaults_of<S>();
            std::size_t min_items = 0;
            if constexpr (count != 0)
            {
                auto prefix = out.emplace("prefixItems", empty_array);
                [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    [[maybe_unused]] const bool done[] = {
                        true, (
                                  [&] {
                                      const auto m =
                                          field_metadata<S, yyjson::detail::struct_declared_index<S, Ks>>();
                                      auto p = prefix.emplace_back(empty_object);
                                      const bool rejects = emit_field_value<S, Ks>(p, nodes, m);
                                      if (field_required<S, Ks>())
                                          min_items = Ks + 1;
                                      else
                                          emit_field_default<S, Ks>(p, defaults, rejects);
                                      if (!m.empty()) emit_field_metadata<S, Ks>(p, m);
                                  }(),
                                  true)...};
                }(std::make_index_sequence<count>{});
            }
            if constexpr (yyjson::detail::constrained_array<S>)
            {
                // The settings of the whole array, such as `yyjson::unique_items`, which hold for its JSON elements.
                const auto& a = nodes[yyjson::detail::field_table_of<S>::table.array_node];
                auto rest = a.constraints;
                if (rest.template has<yyjson::min_items>())
                {
                    min_items = std::max(min_items, rest.min_items);
                    rest.set &= ~yyjson::detail::constraint_bit<yyjson::min_items>;
                }
                emit_constraints(out, rest);
                if (a.contains != 0) emit<writer::value>(out.emplace("contains", empty_object), nodes, a.contains);
            }
            if (min_items != 0) out.emplace("minItems", min_items);
            if constexpr (yyjson::detail::has_extra_items<S>)
            {
                const auto v = nodes[yyjson::detail::extra_items_index<S>].items;
                emit<yyjson::detail::extra_item_type<S>>(out.emplace("items", empty_object), v != 0 ? nodes : nullptr,
                                                         v);
            }
            else if (writing() || yyjson::detail::struct_items<S> == additional::denied ||
                     (strict() && yyjson::detail::struct_items<S> != additional::allowed))
                out.emplace("items", false);
            values_ = outer_values;
            nodes_ = outer_nodes;
        }

        // Adds the key of the adjacent tag that field K of S writes, if any.  A function rather than a lambda in the
        // fold of the caller: clang 18 and 19 crash on that lambda.
        template <typename S, std::size_t K>
        void add_adjacent_tag_key(std::vector<std::string_view>& keys)
        {
            using T = yyjson::detail::struct_field_tags<S, K>;
            if constexpr (yyjson::detail::writes_adjacent_tag<S, T>) keys.push_back(T::key);
        }

        // Writes the conditions of S, `yyjson::if_then_else` as `if`, `then` and `else`, and `yyjson::not_` as
        // `not`, each in a branch of `allOf` when S has more than one.  `tags` are the tags of `yyjson::internal_tag`
        // that the object of S has.
        template <typename S>
        void generator::emit_conditions(object_ref out, std::optional<array_ref>& all_of,
                                        std::span<const variant_tag> tags)
        {
            using table = yyjson::detail::condition_table<S>;
            constexpr auto entries = std::make_index_sequence<table::entries.size()>{};
            const auto* const outer_values = std::exchange(values_, table::tree.values.data());
            const auto* const outer_nodes = std::exchange(nodes_, table::tree.nodes.data());
            // The keys of the tags, which a part with `yyjson::additional_properties(false)` does not reject.  The
            // span is not empty for such a part even when S has no tags.
            auto tag_keys = std::vector<std::string_view>();
            if constexpr (table::closes)
            {
                tag_keys.reserve(tags.size() + 1);
                for (const auto& tag : tags) tag_keys.push_back(tag.key);
                if constexpr (yyjson::detail::has_adjacent_fields<S>)
                    [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                        (add_adjacent_tag_key<S, Ks>(tag_keys), ...);
                    }(std::make_index_sequence<yyjson::detail::struct_field_count<S>>{});
            }
            const auto closed = [&](bool c) {
                static constexpr std::string_view none[1] = {};
                if (!c) return std::span<const std::string_view>();
                return tag_keys.empty() ? std::span<const std::string_view>(none, 0)
                                        : std::span<const std::string_view>(tag_keys);
            };
            using bounds = yyjson::detail::condition_rule::key_bounds;
            // `yyjson::min_properties` and `yyjson::max_properties` of a part
            const auto count = [](object_ref part, const bounds& keys) {
                if (keys.min != 0) part.emplace("minProperties", keys.min);
                if (keys.max != bounds().max) part.emplace("maxProperties", keys.max);
            };
            // `oneOf` or `anyOf` of the groups of `yyjson::one_of_required` and `yyjson::any_of_required` of a
            // part, in `allOf` when the part has the keyword already
            const auto groups = [](object_ref part, std::size_t begin, std::size_t end) {
                for (auto g = begin; g < end;)
                {
                    const auto first = g;
                    const std::string_view keyword = table::groups[first].exactly_one ? "oneOf" : "anyOf";
                    auto branches = [&] {
                        if (part.find(keyword) == part.end()) return part.emplace(keyword, empty_array);
                        const auto all = part.find("allOf");
                        auto list = all != part.end() ? *all->second.as_array() : part.emplace("allOf", empty_array);
                        return list.emplace_back(empty_object).emplace(keyword, empty_array);
                    }();
                    for (; g < end && table::groups[g].choice == table::groups[first].choice; ++g)
                    {
                        auto keys = branches.emplace_back(empty_object).emplace("required", empty_array);
                        for (auto f = table::groups[g].begin; f < table::groups[g].end; ++f)
                            keys.emplace_back(yyjson::detail::struct_key_rules<S>::keys[table::group_fields[f]]);
                    }
                }
            };
            for (const auto& rule : table::rules)
            {
                auto target = [&] {
                    if (table::rules.size() == 1) return out;
                    if (!all_of) all_of = out.emplace("allOf", empty_array);
                    return all_of->emplace_back(empty_object);
                }();
                if (rule.negated)
                {
                    auto part = target.emplace("not", empty_object);
                    emit_condition_part<S>(part, rule.begin, rule.then_begin, entries);
                    count(part, rule.if_keys);
                    continue;
                }
                auto if_part = target.emplace("if", empty_object);
                emit_condition_part<S>(if_part, rule.begin, rule.then_begin, entries);
                count(if_part, rule.if_keys);
                if (rule.then_begin != rule.else_begin || rule.then_closed || rule.then_keys.bounded() ||
                    rule.then_groups != rule.else_groups)
                {
                    auto part = target.emplace("then", empty_object);
                    emit_condition_part<S>(part, rule.then_begin, rule.else_begin, entries, closed(rule.then_closed));
                    count(part, rule.then_keys);
                    groups(part, rule.then_groups, rule.else_groups);
                }
                if (rule.else_begin != rule.end || rule.else_closed || rule.else_keys.bounded() ||
                    rule.else_groups != rule.groups_end)
                {
                    auto part = target.emplace("else", empty_object);
                    emit_condition_part<S>(part, rule.else_begin, rule.end, entries, closed(rule.else_closed));
                    count(part, rule.else_keys);
                    groups(part, rule.else_groups, rule.groups_end);
                }
            }
            values_ = outer_values;
            nodes_ = outer_nodes;
        }
    }  // namespace schema_detail

    namespace schema_detail
    {
        // The schemas of several roots and of the named types that they refer to.  The names are settled when all
        // the roots are known, because a type whose read and write schemas differ has two names.
        class builder
        {
        public:
            builder(std::string_view ref_prefix, bool strict) : ref_prefix_(ref_prefix), strict_(strict) {}

            template <typename T>
            void add(bool writing)
            {
                if (find_root<T>(writing) == roots_.size())
                {
                    roots_.push_back({&type_identity_tag<T>, writing, &generator::emit_root<T>});
                    names_.reset();
                }
            }
            template <typename T>
            void emit_root(object_ref out, bool writing)
            {
                settle();
                auto g = generator(*names_, writing, strict_);
                generator::emit_root<T>(g, out);
            }
            // Calls `f(id, writing, schema)` with the schema of each root, where `id` tells its type.
            template <typename F>
            void emit_roots(F&& f)
            {
                settle();
                for (const auto& r : roots_)
                {
                    auto doc = value(object());
                    auto g = generator(*names_, r.writing, strict_);
                    r.emit(g, *doc.as_object());
                    f(r.id, r.writing, *std::move(doc).as_object());
                }
            }
            // Writes the schemas of the named types into `defs`.
            void emit_definitions(object_ref defs)
            {
                settle();
                for (const auto& [i, writing] : names_->order)
                {
                    if (!names_->split(i) && names_->types[i].used[0] && writing) continue;
                    auto g = generator(*names_, writing, strict_);
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
            bool strict_;
            std::vector<root> roots_;
            std::optional<registry> names_;

            template <typename T>
            [[nodiscard]] std::size_t find_root(bool writing) const noexcept
            {
                std::size_t i = 0;
                while (i < roots_.size() && (roots_[i].id != &type_identity_tag<T> || roots_[i].writing != writing))
                    ++i;
                return i;
            }

            [[nodiscard]] std::string definition_text(std::size_t i, bool writing)
            {
                auto doc = value(object());
                auto g = generator(*names_, writing, strict_);
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
                // Finds the named types: the schema of each adds the types that it refers to.
                for (const auto& r : roots_)
                {
                    auto doc = value(object());
                    auto g = generator(*names_, r.writing, strict_);
                    r.emit(g, *doc.as_object());
                }
                for (std::size_t k = 0; k < names_->order.size(); ++k)
                {
                    const auto [i, writing] = names_->order[k];
                    (void)definition_text(i, writing);
                }
                // A type shares one name for both directions while its two schemas are the same, where the types
                // that they refer to are taken as the same if they share a name.  Splitting a type changes the
                // schemas that refer to it, so this repeats until nothing is split.
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
                names_->settled = true;
                auto owners = std::map<std::string, std::size_t, std::less<>>();
                for (const auto& [i, writing] : names_->order)
                {
                    const auto [it, added] = owners.emplace(names_->name(i, writing), i);
                    if (!added && it->second != i)
                        throw std::invalid_argument(
                            std::string("the JSON Schema has the name \"")
                                .append(it->first)
                                .append("\" for both ")
                                .append(names_->types[it->second].type_name)
                                .append(" and ")
                                .append(names_->types[i].type_name)
                                .append("; give one of them another name with yyjson::schema_name"));
                }
            }
        };

        template <typename T>
        object json_schema(const schema_options& options, bool strict)
        {
            const auto writing = options.direction == schema_direction::write;
            auto b = builder(options.ref_prefix, strict && !writing);
            b.add<T>(writing);
            auto doc = value(object());
            auto root = *doc.as_object();
            root.emplace("$schema", dialect);
            if (!options.id.empty()) root.emplace("$id", options.id, copy_string);
            auto defs = root.emplace("$defs", empty_object);
            b.emit_definitions(defs);
            if (defs.empty()) root.erase("$defs");
            b.emit_root<T>(root, writing);
            return *std::move(doc).as_object();
        }
    }  // namespace schema_detail

    // The JSON Schema (2020-12) of T: the JSON that `read<T>` takes, or with `schema_direction::write` the JSON that
    // `write` gives, as the types and the annotations declare it.
    template <typename T>
    [[nodiscard]] object json_schema(const schema_options& options = {})
    {
        return schema_detail::json_schema<T>(options, false);
    }
    // The schema of what `read<T>` takes with `yyjson::strict`: the key of a field is required unless the field may
    // be absent by its type, by `optional_key` or by `omit_default`, and a struct has no other keys unless
    // `additional_properties` says otherwise.  The schema of `write` is the same with or without it.
    template <typename T>
    [[nodiscard]] object json_schema(strict_t, const schema_options& options = {})
    {
        return schema_detail::json_schema<T>(options, true);
    }

    // The schemas of several types, such as `components/schemas` of an OpenAPI 3.1 document, without `$schema`.  A
    // named type has one name when its read and write schemas are the same, and otherwise the two names "Name-Input"
    // and "Name-Output", so the names are settled by `build()`, when all the types are known.
    //
    //     const auto schemas = yyjson::schema_collection(yyjson::strict)
    //                              .add<Order>()
    //                              .add<Receipt>(yyjson::schema_direction::write)
    //                              .build();
    //     const auto& request = schemas.schema<Order>();  // {"$ref": "#/components/schemas/Order"}
    //     const auto& components = schemas.schemas();
    class schema_collection
    {
    public:
        using options = schema_detail::collection_options;

        // The schemas that `build()` generates.
        class result
        {
        public:
            // The schema of T that `add<T>(direction)` added: a reference for a named type.
            template <typename T>
            [[nodiscard]] const object& schema(schema_direction direction = schema_direction::read) const
            {
                const auto writing = direction == schema_direction::write;
                for (const auto& r : roots_)
                    if (r.id == &schema_detail::type_identity_tag<T> && r.writing == writing) return r.schema;
                throw std::logic_error(std::string("yyjson::schema_collection::result::schema of ")
                                           .append(yyjson::detail::type_name<T>())
                                           .append(" without add in the same direction"));
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
        };

        explicit schema_collection(const options& opts = {}) : builder_(opts.ref_prefix, false) {}
        // The read schemas are those of `read<T>` with `yyjson::strict`.
        explicit schema_collection(strict_t, const options& opts = {}) : builder_(opts.ref_prefix, true) {}

        template <typename T>
        schema_collection& add(schema_direction direction = schema_direction::read) &
        {
            builder_.add<T>(direction == schema_direction::write);
            return *this;
        }
        template <typename T>
        schema_collection&& add(schema_direction direction = schema_direction::read) &&
        {
            return std::move(add<T>(direction));
        }

        // Generates the schemas of the types added.  Throws `std::invalid_argument` as `json_schema<T>` does.
        [[nodiscard]] result build() const&
        {
            auto b = builder_;
            return build(b);
        }
        [[nodiscard]] result build() && { return build(builder_); }

    private:
        schema_detail::builder builder_;

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
