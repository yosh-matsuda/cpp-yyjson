// JSON Schema output of the C++ types that cpp-yyjson reads and writes.  See docs/json-schema.md.

#pragma once

#include <array>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
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

    struct schema_collection_options
    {
        // The prefix of the references to named types, where the program puts `schemas()`.
        std::string_view ref_prefix = "#/components/schemas/";
    };

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

        template <typename T>
        inline constexpr char type_identity_tag = 0;

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
                if constexpr (std::same_as<A, yyjson::title>)
                    title = a.text();
                else if constexpr (std::same_as<A, yyjson::description>)
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
            std::define_static_array(yyjson::detail::template_annotations_of<R, ^^yyjson::examples_t>());
        template <std::meta::info R, std::size_t K>
        using examples_annotation_type =
            typename[:std::meta::remove_cvref(std::meta::type_of(examples_annotations<R>[K])):];

        // Adds the metadata in the annotations of the member or the type R.
        template <std::meta::info R>
        void add_annotated_metadata(metadata& m)
        {
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::title>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::description>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, deprecated_t>) m.add(a);
            for (const auto& a : field_reflection::detail::annotations_v<R, yyjson::schema>) m.add(a);
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (m.add(std::meta::extract<examples_annotation_type<R, Ks>>(examples_annotations<R>[Ks])), ...);
            }(std::make_index_sequence<examples_annotations<R>.size()>{});
        }
#endif

        // The metadata of the field R of the struct S, by its index among all the fields.  The settings are checked
        // with the keys of the fields, where a field has each of them once at most.
        template <typename S, std::size_t R>
        metadata field_metadata()
        {
            auto m = metadata();
            std::apply(
                [&](const auto&... as) {
                    (
                        [&]<typename A>(const A& a) {
                            if constexpr (yyjson::detail::is_field_annotation<A>::value)
                            {
                                if (a.member == yyjson::detail::declared_member_name<S, R>)
                                    std::apply([&](const auto&... vs) { (m.add(vs), ...); }, a.values);
                            }
                        }(as),
                        ...);
                },
                annotations<S>.values);
#if FIELD_RFL_ANNOTATIONS
            if constexpr (!yyjson::detail::visitable<S>)
            {
                // Metadata alone does not build the table of the keys, which checks the settings of a member.
                constexpr auto member = field_reflection::detail::member_at<S, R>;
                static_assert(field_reflection::detail::annotations_v<member, yyjson::title>.size() <= 1,
                              "a field is given yyjson::title twice");
                static_assert(field_reflection::detail::annotations_v<member, yyjson::description>.size() <= 1,
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
            auto result = std::apply(
                [](const auto&... as) {
                    return (
                        [&]<typename A>(const A& a) {
                            if constexpr (yyjson::detail::is_field_annotation<A>::value)
                                return a.member == yyjson::detail::declared_member_name<S, R> &&
                                       std::apply(
                                           []<typename... Vs>(const Vs&...) {
                                               return (yyjson::detail::is_examples<Vs>::value || ...);
                                           },
                                           a.values);
                            else
                                return false;
                        }(as) ||
                        ...);
                },
                annotations<S>.values);
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
            auto result = std::apply(
                [](const auto&... as) {
                    return (
                        [&]<typename A>(const A& a) {
                            if constexpr (yyjson::detail::is_field_annotation<A>::value)
                                return a.member == yyjson::detail::declared_member_name<S, R> &&
                                       std::apply(
                                           []<typename... Vs>(const Vs&...) {
                                               return (std::same_as<Vs, yyjson::schema> || ...);
                                           },
                                           a.values);
                            else
                                return false;
                        }(as) ||
                        ...);
                },
                annotations<S>.values);
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
                return (std::size_t{0} + ... + std::size_t{std::same_as<As, A>});
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
            static_assert(type_setting_count<T, yyjson::title>() <= 1, "a type is given yyjson::title twice");
            static_assert(type_setting_count<T, yyjson::description>() <= 1,
                          "a type is given yyjson::description twice");
            static_assert(type_setting_count<T, deprecated_t>() <= 1, "a type is given yyjson::deprecated twice");
            static_assert(type_examples_count<T>() <= 1, "a type is given yyjson::examples twice");
            static_assert(type_setting_count<T, yyjson::schema>() <= 1, "a type is given yyjson::schema twice");
            auto m = metadata();
            std::apply(
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
                std::apply(
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

        template <typename T>
        concept user_read = !yyjson::detail::reads_by_default<T>;
        // T is written by a `to_json` of the user, which returns a value or writes into one.
        template <typename T>
        concept user_write = (!std::derived_from<caster<T>, yyjson::detail::builtin_caster>) &&
                             (writer::detail::to_json_inp_usr_defined<T> ||
                              writer::detail::to_json_usr_translator<const T&>);

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
            // Writes the schema of the struct or the enumeration T, which `$defs` has.
            template <typename T>
            static void emit_definition(generator& g, object_ref out)
            {
                if (!g.emit_declared_type<T>(out))
                {
                    if constexpr (std::is_enum_v<T>)
                        g.emit_enum<T>(out);
                    else
                        g.emit_struct<T>(out);
                }
                g.emit_type_metadata<T>(out);
            }

        private:
            registry& names_;
            bool writing_;
            bool strict_;
            // The values of `yyjson::const_` and `yyjson::enum_` of the struct whose fields are written.
            const yyjson::detail::json_scalar* values_ = nullptr;

            // `yyjson::strict` applies to reading only: what `write` gives has every key that is not left out.
            [[nodiscard]] bool strict() const noexcept { return strict_ && !writing(); }
            [[nodiscard]] bool writing() const noexcept { return writing_; }

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
            // does not read, and a value in `T()` may not satisfy the constraints, and neither is a default of the
            // schema that is read.
            template <typename S, std::size_t K, typename F>
            [[nodiscard]] bool reads_back(const F& d) const
            {
                if constexpr (readable<F>)
                {
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
            // field.  Each example is read into E and checked by `check`; the schema of reading has it as it is
            // written, and the schema of `write` has what `write` gives for the value read.
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
                        if (!check(*value)) fail("does not satisfy the constraints");
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
                // A key of `doc` is copied as a std::string: the reference to a value of a document does not copy it.
                for (const auto& [k, v] : *o) out.emplace(std::string(k), v);
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
                                                                    [](const T&) { return true; });
            }
            // The name of the field R of S in messages.
            template <typename S, std::size_t R>
            [[nodiscard]] static std::string member_of()
            {
                return std::string(yyjson::detail::type_name<S>()).append("::").append(
                    yyjson::detail::declared_member_name<S, R>);
            }
            // Whether T is put in `$defs` and has its metadata there.
            template <typename T>
            [[nodiscard]] bool named() const noexcept
            {
                if constexpr (std::is_enum_v<T> || struct_type<T>)
                    return writing() ? !user_write<T> : !user_read<T>;
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
            // Writes the constraints of a node.  The bounds of a number type are written by `emit_number`.
            void emit_constraints(object_ref out, const yyjson::detail::field_constraints& c) const
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
                if (c.has<yyjson::pattern>()) out.emplace("pattern", c.pattern());
                if (c.has<yyjson::format>()) out.emplace("format", c.format());
                if (c.has<yyjson::const_>())
                    visit_scalar(values_[c.constant], [&](auto v) { out.emplace("const", v); });
                if (c.has<yyjson::enum_<0>>())
                {
                    auto values = out.emplace("enum", empty_array);
                    for (std::size_t i = 0; i < c.enum_count; ++i)
                        visit_scalar(values_[c.enum_first + i], [&](auto v) { values.emplace_back(v); });
                }
                if (c.has<yyjson::unique_items_t>()) out.emplace("uniqueItems", true);
                if (c.has<yyjson::min_contains>()) out.emplace("minContains", c.min_contains);
                if (c.has<yyjson::max_contains>()) out.emplace("maxContains", c.max_contains);
                // `validate<f>` has no keyword.
            }

            template <typename I>
            void emit_number(object_ref out, const schema_node* n) const
            {
                out.emplace("type", std::integral<I> ? "integer" : "number");
                using L = std::numeric_limits<I>;
                // A double holds every finite JSON number that the reader accepts.
                constexpr bool bounded = std::integral<I> || sizeof(I) < sizeof(double);
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
            void emit_struct(object_ref out, std::string_view tag_key = {}, std::string_view tag_id = {});
            template <typename F, typename Tags, std::size_t... Is>
            void emit_adjacent(array_ref one, std::string_view key, std::optional<std::size_t> held,
                               std::index_sequence<Is...>);

            template <typename V>
            void emit_variant(object_ref out);

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
                if (n != nullptr) emit_constraints(out, n->constraints);
            };
            if constexpr (!std::same_as<T, std::remove_cvref_t<T>>)
                emit<std::remove_cvref_t<T>>(out, nodes, node);
            else if constexpr (std::same_as<T, json_view> || std::same_as<T, writer::value> ||
                               std::same_as<T, reader::value>)
                constraints();
            else if constexpr (std::same_as<T, writer::object> || std::same_as<T, reader::object>)
            {
                out.emplace("type", "object");
                constraints();
            }
            else if constexpr (std::same_as<T, writer::array> || std::same_as<T, reader::array>)
            {
                out.emplace("type", "array");
                constraints();
            }
            else if constexpr (std::same_as<T, std::nullptr_t> || std::same_as<T, std::monostate>)
                out.emplace("type", "null");
            else if constexpr (user_read<T> || user_write<T>)
            {
                // A type may have a caster for one direction and be converted by the library in the other.
                if (writing())
                {
                    if constexpr (user_write<T>)
                        emit_user_write<T>(out);
                    else
                        emit_builtin<T>(out, nodes, node);
                }
                else
                {
                    if constexpr (user_read<T>)
                        emit_user_read<T>(out);
                    else
                        emit_builtin<T>(out, nodes, node);
                }
            }
            else
                emit_builtin<T>(out, nodes, node);
            if constexpr (std::same_as<T, std::remove_cvref_t<T>>)
            {
                static_assert(std::is_enum_v<T> || struct_type<T> || type_setting_count<T, yyjson::schema_name>() == 0,
                              "yyjson::schema_name names a struct or an enumeration");
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
                if (n != nullptr) emit_constraints(out, n->constraints);
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
                auto any = out.emplace("anyOf", empty_array);
                emit<U>(any.emplace_back(empty_object), nodes, node);
                any.emplace_back(empty_object).emplace("type", "null");
            }
            else if constexpr (is_variant<T>::value)
            {
                emit_variant<T>(out);
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
                        emit_constraints(keys, nodes[n->keys].constraints);
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
                // each member read has to satisfy the schema of every value and key, and a member written one of them.
                // The node of the key or the value of the pair at position i, or 0 when it has no constraints.
                const auto part = [&](std::size_t i, std::size_t k) -> std::size_t {
                    if (n == nullptr || i >= n->prefix_count) return 0;
                    const auto& pair = nodes[n->prefix + i];
                    return k < pair.prefix_count ? pair.prefix + k : 0;
                };
                const bool keys_constrained = ((part(Is, 0) != 0) || ...);
                const bool values_constrained = ((part(Is, 1) != 0) || ...);
                const char* const combine = writing() ? "anyOf" : "allOf";
                out.emplace("type", "object");
                out.emplace("minProperties", sizeof...(Is));
                out.emplace("maxProperties", sizeof...(Is));
                if (keys_constrained)
                {
                    auto each = out.emplace("propertyNames", empty_object).emplace(combine, empty_array);
                    (emit<std::string>(each.emplace_back(empty_object), part(Is, 0) != 0 ? nodes : nullptr,
                                       part(Is, 0)),
                     ...);
                }
                using V0 = std::remove_cvref_t<std::tuple_element_t<1, std::tuple_element_t<0, T>>>;
                auto values = out.emplace("additionalProperties", empty_object);
                if (!values_constrained &&
                    (std::same_as<std::remove_cvref_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>, V0> &&
                     ...))
                    emit<V0>(values);
                else
                {
                    auto each = values.emplace(combine, empty_array);
                    (emit<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>(
                         each.emplace_back(empty_object), part(Is, 1) != 0 ? nodes : nullptr, part(Is, 1)),
                     ...);
                }
                return;
            }
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

        template <typename V>
        void generator::emit_variant(object_ref out)
        {
            using tags = yyjson::detail::variant_tags<V>;
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                if constexpr (tags::tagging == detail::variant_tagging::internal)
                {
                    auto one = out.emplace("oneOf", empty_array);
                    (
                        [&] {
                            using A = std::variant_alternative_t<Is, V>;
                            auto branch = one.emplace_back(empty_object);
                            if constexpr (std::same_as<A, std::monostate>)
                                branch.emplace("type", "null");
                            else if constexpr (struct_type<A>)
                                emit_struct<A>(branch, tags::key, tags::ids[Is]);
                            else
                            {
                                auto all = branch.emplace("allOf", empty_array);
                                emit<A>(all.emplace_back(empty_object));
                                auto tag = all.emplace_back(empty_object);
                                tag.emplace("properties", empty_object)
                                    .emplace(tags::key, empty_object)
                                    .emplace("const", tags::ids[Is]);
                                tag.emplace("required", empty_array).emplace_back(tags::key);
                            }
                        }(),
                        ...);
                }
                else
                {
                    // An adjacent tag is written by the struct that holds the variant.
                    auto any = out.emplace("anyOf", empty_array);
                    (emit<std::variant_alternative_t<Is, V>>(any.emplace_back(empty_object)), ...);
                }
            }(std::make_index_sequence<std::variant_size_v<V>>{});
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
                    if constexpr (std::same_as<A, std::monostate>)
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
            constexpr auto monostate = std::array{std::same_as<std::variant_alternative_t<Is, F>, std::monostate>...};
            if (!monostate[*held])
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

        template <typename S>
        void generator::emit_struct(object_ref out, std::string_view tag_key, std::string_view tag_id)
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
            out.emplace("type", "object");
            auto properties = out.emplace("properties", empty_object);
            auto required = std::vector<std::string_view>();
            auto tag = std::optional<object_ref>();
            if (!tag_key.empty())
            {
                tag = properties.emplace(tag_key, empty_object);
                tag->emplace("const", tag_id);
                required.push_back(tag_key);
            }
            auto adjacent = std::optional<array_ref>();
            [[maybe_unused]] const auto defaults = [] {
                if constexpr (std::default_initializable<S>)
                    return S();
                else
                    return 0;
            }();

            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (
                    [&] {
                        using F = yyjson::detail::struct_field_type<S, Ks>;
                        constexpr auto key = yyjson::detail::struct_field_key<S, Ks>;
                        constexpr auto node = yyjson::detail::struct_declared_index<S, Ks>;
                        const auto* field_nodes = yyjson::detail::constrained_field<S, Ks> ? nodes : nullptr;
                        constexpr auto declared = yyjson::detail::struct_declared_index<S, Ks>;
                        const auto m = field_metadata<S, declared>();
                        // A field that has the key of the tag is read from the tag, whose id it must accept, and is
                        // written only when it holds the id.
                        const bool tagged = tag && key == tag_key;
                        auto p = tagged ? tag->emplace("allOf", empty_array).emplace_back(empty_object)
                                        : properties.emplace(key, empty_object);
                        bool rejects = false;
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
                                if constexpr (std::default_initializable<S>)
                                    return yyjson::detail::struct_field<Ks>(defaults).index();
                                else
                                    return std::nullopt;
                            }();
                            emit_adjacent<F, tags>(adjacent->emplace_back(empty_object).emplace("oneOf", empty_array),
                                                   key, held, std::make_index_sequence<std::variant_size_v<F>>{});
                        }
                        else if constexpr (yyjson::detail::is_std_optional<F>::value)
                        {
                            // A field without a value is left out of the object, which is how the type declares
                            // that it may be absent.  Null is not part of the declaration, although a read without
                            // `yyjson::strict` takes it, unless the value type reads null itself.
                            if (const auto* d = m.declared_for(writing()))
                                rejects = emit_declared(p, d->text(writing()), member_of<S, declared>());
                            else
                                emit<typename F::value_type>(p, field_nodes, node);
                        }
                        else if (const auto* d = m.declared_for(writing()))
                            rejects = emit_declared(p, d->text(writing()), member_of<S, declared>());
                        else
                            emit<F>(p, field_nodes, node);

                        const bool is_required = [&] {
                            if (writing())
                                return !yyjson::detail::omittable<F>;
                            return rules::required[Ks] || (strict() && !rules::optional[Ks]);
                        }();
                        if (tagged)
                            ;
                        else if (is_required)
                            required.push_back(key);
                        else if constexpr (std::default_initializable<S> && writable<F> &&
                                           !yyjson::detail::adjacent_field<S, Ks>)
                        {
                            const auto& d = yyjson::detail::struct_field<Ks>(defaults);
                            const bool omitted = [&] {
                                if constexpr (yyjson::detail::omittable<F>)
                                    return yyjson::detail::should_omit_field(d);
                                else
                                    return false;
                            }();
                            if (!writing() && !omitted && !rejects && reads_back<S, Ks>(d))
                                p.emplace("default", d, copy_string);
                        }
                        if (!m.empty())
                        {
                            using E = std::conditional_t<yyjson::detail::is_std_optional<F>::value &&
                                                             !yyjson::detail::adjacent_field<S, Ks>,
                                                         typename optional_value<F>::type, F>;
                            const auto check = [](const E& v) {
                                if constexpr (std::same_as<E, F>)
                                    return satisfies<S, Ks>(v);
                                else
                                    return satisfies<S, Ks>(F(v));
                            };
                            emit_metadata<E, field_has_examples<S, declared>()>(p, m, member_of<S, declared>(), check);
                        }
                    }(),
                    ...);
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
            values_ = outer_values;
            const bool closed = writing() || rules::additional == additional::denied ||
                                (strict() && rules::additional != additional::allowed);
            // The keys of the branches of `allOf` are in `properties` too, so `additionalProperties` knows them.
            if (closed) out.emplace("additionalProperties", false);
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
            [[nodiscard]] bool contains(bool writing) const noexcept
            {
                return find_root<T>(writing) != roots_.size();
            }
            template <typename T>
            void emit_root(object_ref out, bool writing)
            {
                settle();
                auto g = generator(*names_, writing, strict_);
                generator::emit_root<T>(g, out);
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
    // be absent by its type or by `optional_key`, and a struct has no other keys unless `additional_properties`
    // says otherwise.  The schema of `write` is the same with or without it.
    template <typename T>
    [[nodiscard]] object json_schema(strict_t, const schema_options& options = {})
    {
        return schema_detail::json_schema<T>(options, true);
    }

    // The schemas of several types, such as `components/schemas` of an OpenAPI 3.1 document, without `$schema`.  A
    // struct or an enumeration has one name when its read and write schemas are the same, and otherwise the two
    // names "Name-Input" and "Name-Output".
    //
    //     auto schemas = yyjson::schema_collection();
    //     schemas.add<Order>().add<Receipt>(yyjson::schema_direction::write);
    //     auto request = schemas.schema<Order>();  // {"$ref": "#/components/schemas/Order"}
    //     auto components = schemas.schemas();
    class schema_collection
    {
    public:
        explicit schema_collection(const schema_collection_options& options = {}) : builder_(options.ref_prefix, false)
        {
        }
        // The read schemas are those of `read<T>` with `yyjson::strict`.
        explicit schema_collection(strict_t, const schema_collection_options& options = {})
            : builder_(options.ref_prefix, true)
        {
        }

        template <typename T>
        schema_collection& add(schema_direction direction = schema_direction::read)
        {
            builder_.add<T>(direction == schema_direction::write);
            return *this;
        }
        // The schema of T that `add<T>(direction)` added: a reference for a struct or an enumeration.
        template <typename T>
        [[nodiscard]] object schema(schema_direction direction = schema_direction::read)
        {
            const auto writing = direction == schema_direction::write;
            if (!builder_.contains<T>(writing))
                throw std::logic_error(std::string("yyjson::schema_collection::schema of ")
                                           .append(yyjson::detail::type_name<T>())
                                           .append(" without add in the same direction"));
            auto doc = value(object());
            builder_.emit_root<T>(*doc.as_object(), writing);
            return *std::move(doc).as_object();
        }
        // The schemas of the structs and the enumerations, by their names.
        [[nodiscard]] object schemas()
        {
            auto doc = value(object());
            builder_.emit_definitions(*doc.as_object());
            return *std::move(doc).as_object();
        }

    private:
        schema_detail::builder builder_;
    };
}  // namespace yyjson
