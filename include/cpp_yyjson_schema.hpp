// JSON Schema output of the C++ types that cpp-yyjson reads and writes.
//
// PROTOTYPE: the interface and the coverage are not settled.  See docs/design/json-schema.md.

#pragma once

#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>

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
        // The prefix of the references to named types, which are put in `$defs` unless it is changed.
        std::string_view ref_prefix = "#/$defs/";
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

        template <typename T>
        struct is_variant : std::false_type
        {
        };
        template <typename... Ts>
        struct is_variant<std::variant<Ts...>> : std::true_type
        {
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
            generator(const schema_options& options, bool strict) : options_(options), strict_(strict) {}

            // Writes the schema of T into `out`, with the settings of the node `node` of `nodes` (none if null).
            template <typename T>
            void emit(object_ref out, const schema_node* nodes = nullptr, std::size_t node = 0);

            void set_defs(object_ref defs) { defs_ = defs; }
            [[nodiscard]] bool has_defs() const noexcept { return !names_.empty(); }

        private:
            schema_options options_;
            bool strict_;
            std::optional<object_ref> defs_;
            std::set<std::string, std::less<>> names_;
            // The values of `yyjson::const_` and `yyjson::enum_` of the struct whose fields are written.
            const yyjson::detail::json_scalar* values_ = nullptr;

            // `yyjson::strict` applies to reading only: what `write` gives has every key that is not left out.
            [[nodiscard]] bool strict() const noexcept { return strict_ && !writing(); }
            [[nodiscard]] bool writing() const noexcept { return options_.direction == schema_direction::write; }

            // Whether `read<F>` takes back what `write` gives for the value: a caster may write what it does not
            // read, and such a value is no default of the schema that is read.
            template <typename F>
            [[nodiscard]] bool reads_back(const F& value) const
            {
                if constexpr (requires(std::string_view s) { yyjson::read<F>(s); })
                {
                    try
                    {
                        const auto text = yyjson::write(value);
                        if (strict())
                            (void)yyjson::read<F>(std::string_view(text), yyjson::strict);
                        else
                            (void)yyjson::read<F>(std::string_view(text));
                        return true;
                    }
                    catch (const std::exception&)
                    {
                        return false;
                    }
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
                        rest.set &= static_cast<std::uint16_t>(~yyjson::detail::constraint_bit<yyjson::minimum>);
                    if (bounded && rest.has<yyjson::maximum>() && !tighter(rest.maximum, L::max(), false))
                        rest.set &= static_cast<std::uint16_t>(~yyjson::detail::constraint_bit<yyjson::maximum>);
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

            // Writes `{"$ref": ...}` for a named type, and its schema into `$defs` the first time.
            template <typename T, typename Body>
            void emit_named(object_ref out, Body&& body)
            {
                auto name = def_name<T>();
                out.emplace("$ref", std::string(options_.ref_prefix) + name, copy_string);
                if (names_.contains(name)) return;
                names_.insert(name);
                auto def = defs_->emplace(name, empty_object, copy_string);
                body(def);
            }

            template <typename S>
            void emit_struct(object_ref out, std::string_view tag_key = {}, std::string_view tag_id = {});
            template <typename F, typename Tags, std::size_t... Is>
            void emit_adjacent(array_ref one, std::string_view key, std::index_sequence<Is...>);

            template <typename V>
            void emit_variant(object_ref out);

            template <typename T>
            void emit_range(object_ref out, const schema_node* nodes, std::size_t node);

            template <typename T, std::size_t... Is>
            void emit_tuple(object_ref out, const schema_node* nodes, std::size_t node, std::index_sequence<Is...>);
        };

        template <typename T>
        void generator::emit(object_ref out, const schema_node* nodes, std::size_t node)
        {
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
        }

        // The constraints of a field hold for the C++ value that a user caster makes, and the caster's conversion
        // is unknown, so they are not emitted for the JSON value on either side.
        template <typename T>
        void generator::emit_user_read(object_ref out)
        {
            if constexpr (yyjson::detail::from_json_translator<T>::castable)
                emit<yyjson::detail::from_json_source<T>>(out, nullptr, 0);
            // TODO: a schema that the type declares
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
                // TODO: a schema that the type declares
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
                emit_named<T>(out, [&](object_ref def) { emit_enum<T>(def); });
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
                emit_named<T>(out, [&](object_ref def) { emit_struct<T>(def); });
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
                    if (n != nullptr) emit_constraints(o, n->constraints);
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
                if (n != nullptr) emit_constraints(out, n->constraints);
                if constexpr (yyjson::detail::insertable_set<T> || yyjson::detail::unique_keys<T>)
                    if (!(n != nullptr && n->constraints.has<yyjson::unique_items_t>()))
                        out.emplace("uniqueItems", true);
                if constexpr (yyjson::detail::std_array<T> || yyjson::detail::c_array<T>)
                {
                    constexpr auto size = sizeof(T) / sizeof(E);
                    if (!(n != nullptr && n->constraints.has<yyjson::min_items>())) out.emplace("minItems", size);
                    if (!(n != nullptr && n->constraints.has<yyjson::max_items>())) out.emplace("maxItems", size);
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
            // TODO: a tuple of key-value pairs is read from an object as well
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

        template <typename F, typename Tags, std::size_t... Is>
        void generator::emit_adjacent(array_ref one, std::string_view key, std::index_sequence<Is...>)
        {
            (
                [&] {
                    using A = std::variant_alternative_t<Is, F>;
                    auto branch = one.emplace_back(empty_object);
                    auto bp = branch.emplace("properties", empty_object);
                    if constexpr (std::same_as<A, std::monostate>)
                    {
                        bp.emplace(key, empty_object).emplace("type", "null");
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
            if (!tag_key.empty())
            {
                properties.emplace(tag_key, empty_object).emplace("const", tag_id);
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
                        auto p = properties.emplace(key, empty_object);
                        if constexpr (yyjson::detail::adjacent_field<S, Ks>)
                        {
                            // The variant and its tag: one branch for each alternative.
                            using tags = yyjson::detail::struct_field_tags<S, Ks>;
                            if constexpr (yyjson::detail::writes_adjacent_tag<S, tags>)
                            {
                                auto ids = properties.emplace(tags::key, empty_object).emplace("enum", empty_array);
                                for (const auto id : tags::ids)
                                    if (!id.empty()) ids.emplace_back(id);
                            }
                            if (!adjacent) adjacent = out.emplace("allOf", empty_array);
                            emit_adjacent<F, tags>(adjacent->emplace_back(empty_object).emplace("oneOf", empty_array),
                                                   key, std::make_index_sequence<std::variant_size_v<F>>{});
                            // TODO: an object without the tag and the value, which keeps the value in S()
                        }
                        else if constexpr (yyjson::detail::is_std_optional<F>::value)
                        {
                            // A field without a value is left out of the object, which is how the type declares
                            // that it may be absent.  Null is not part of the declaration, although a read without
                            // `yyjson::strict` takes it, unless the value type reads null itself.
                            emit<typename F::value_type>(p, field_nodes, node);
                        }
                        else
                            emit<F>(p, field_nodes, node);

                        const bool is_required = [&] {
                            if (writing())
                                return !yyjson::detail::omittable<F>;
                            return rules::required[Ks] || (strict() && !rules::optional[Ks]);
                        }();
                        if (is_required)
                            required.push_back(key);
                        else if constexpr (std::default_initializable<S> && writable<F>)
                        {
                            const auto& d = yyjson::detail::struct_field<Ks>(defaults);
                            const bool omitted = [&] {
                                if constexpr (yyjson::detail::omittable<F>)
                                    return yyjson::detail::should_omit_field(d);
                                else
                                    return false;
                            }();
                            if (!writing() && !omitted && reads_back(d)) p.emplace("default", d, copy_string);
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
            // TODO: unevaluatedProperties when the struct has an adjacent tag in allOf
            if (closed) out.emplace("additionalProperties", false);
        }
    }  // namespace schema_detail

    namespace schema_detail
    {
        template <typename T>
        object json_schema(const schema_options& options, bool strict)
        {
            auto doc = value(object());
            auto root = *doc.as_object();
            root.emplace("$schema", dialect);
            if (!options.id.empty()) root.emplace("$id", options.id, copy_string);
            auto gen = generator(options, strict);
            gen.set_defs(root.emplace("$defs", empty_object));
            gen.template emit<T>(root);
            if (!gen.has_defs()) root.erase("$defs");
            return *std::move(doc).as_object();
        }
    }  // namespace schema_detail

    // The JSON Schema (2020-12) of T: the JSON that `read<T>` takes, or with `schema_direction::write` the JSON that
    // `write` gives, as the types and the annotations declare it.  PROTOTYPE.
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
}  // namespace yyjson
