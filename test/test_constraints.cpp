#include "test_constraints_common.hpp"

// NOLINTBEGIN

// The keywords of the values of the fields: the bounds of a number, the lengths and the patterns of a string,
// `yyjson::format_assertion`, `yyjson::const_`, `yyjson::enum_` and `yyjson::satisfies`

// bounds of another kind than the field, near the limits of the field
struct MixedBounds
{
    std::int64_t big = 0;
    std::uint64_t ubig = 0;
    double real = 0;
    std::int64_t negative = 0;
    float quarter = 0;
    double cents = 0;
    std::int64_t tenths = 0;
    bool operator==(const MixedBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MixedBounds> = yyjson::annotate(
    yyjson::field("big", yyjson::maximum(9007199254740992.0)),
    yyjson::field("ubig", yyjson::minimum(-1), yyjson::multiple_of(9223372036854775808ULL)),
    yyjson::field("real", yyjson::maximum(9007199254740993LL)),
    yyjson::field("negative", yyjson::exclusive_maximum(-2.5), yyjson::multiple_of(7)),
    yyjson::field("quarter", yyjson::multiple_of(0.25)), yyjson::field("cents", yyjson::multiple_of(0.01)),
    yyjson::field("tenths", yyjson::multiple_of(0.1)));

// bounds of a float field, which compares the JSON number that the field is read from
struct FloatBounds
{
    float tenth = 0;
    float under = 0;
    float big = 16777216.0F;
    float step = 0;
    bool operator==(const FloatBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatBounds> = yyjson::annotate(
    yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("under", yyjson::exclusive_maximum(0.1)),
    yyjson::field("big", yyjson::minimum(16777217)), yyjson::field("step", yyjson::multiple_of(0.1)));

// floats in elements, in values of a map and in an alternative of a variant
struct FloatElements
{
    std::vector<float> v;
    std::map<std::string, float> m;
    std::variant<float, std::string> u = 0.0F;
    bool operator==(const FloatElements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatElements> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::maximum(0.1))),
    yyjson::field("m", yyjson::additional_properties(yyjson::maximum(0.1))), yyjson::field("u", yyjson::maximum(0.1)));

// bounds of a double field, which compares the JSON number that the field is read from
struct RealBounds
{
    double top = 0;
    double tenth = 0;
    double cent = 0;
    double half = 0;
    double third = 0;
    double three = 0;
    long double wide = 0;
    bool operator==(const RealBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RealBounds> = yyjson::annotate(
    yyjson::field("top", yyjson::maximum(9007199254740992LL)), yyjson::field("tenth", yyjson::multiple_of(0.1)),
    yyjson::field("cent", yyjson::multiple_of(0.01)), yyjson::field("half", yyjson::multiple_of(0.5)),
    yyjson::field("third", yyjson::multiple_of(1.5)), yyjson::field("three", yyjson::multiple_of(3)),
    yyjson::field("wide", yyjson::exclusive_minimum(-9007199254740992LL)));

// doubles in elements and in an alternative of a variant
struct RealElements
{
    std::vector<double> v;
    std::variant<double, std::string> u = 0.0;
    bool operator==(const RealElements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RealElements> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::maximum(9007199254740992LL))),
                     yyjson::field("u", yyjson::multiple_of(0.1)));

// divisors that no fixed number of digits holds: integers up to 2^64, and reals by their shortest decimal
struct WideDivisors
{
    double big = 0;
    std::uint64_t ubig = 0;
    double tiny = 0;
    double odd = 0;
    std::int64_t huge = 0;
    bool operator==(const WideDivisors&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WideDivisors> = yyjson::annotate(
    yyjson::field("big", yyjson::multiple_of(9007199254740992ULL)),
    yyjson::field("ubig", yyjson::multiple_of(9007199254740992ULL)), yyjson::field("tiny", yyjson::multiple_of(1e-18)),
    yyjson::field("odd", yyjson::multiple_of(900719925474098.8)), yyjson::field("huge", yyjson::multiple_of(1e20)));

// several patterns at one level, which a value must all match, between other keywords too
struct Patterns
{
    std::string s;
    std::optional<std::string> o;
    std::string v;
    bool operator==(const Patterns&) const = default;
};
// a pattern made from a std::string_view, which the setting points to
inline constexpr std::string_view digits = "^[0-9]*$";
template <>
inline constexpr auto yyjson::annotations<Patterns> = yyjson::annotate(
    yyjson::field("s", yyjson::pattern("^a"), yyjson::pattern("z$")),
    yyjson::field("o", yyjson::pattern("^o"), yyjson::min_length(2), yyjson::pattern("o$"), yyjson::pattern("x")),
    yyjson::field("v", yyjson::pattern(digits)));

struct ConstrainedVisitable
{
    int a = 0;
    std::string b;
    bool operator==(const ConstrainedVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedVisitable> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("b", yyjson::max_length(1)));
VISITABLE_STRUCT(ConstrainedVisitable, a, b);

// a renamed field is constrained under its key, and the direct reader compares more fields than in their order
struct ConstrainedLarge
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    int renamed = 0;
    bool operator==(const ConstrainedLarge&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedLarge> = yyjson::annotate(
    yyjson::field("i", yyjson::maximum(1)), yyjson::field("renamed", yyjson::rename("r"), yyjson::minimum(0)));

// a skipped field before the constrained ones, a character type and a std::optional field with a default value
struct ConstrainedSkipped
{
    int skipped = 0;
    int a = 0;
    std::string s = "xy";
    char c = 1;
    std::optional<int> o = 5;
    bool operator==(const ConstrainedSkipped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedSkipped> =
    yyjson::annotate(yyjson::field("skipped", yyjson::skip), yyjson::field("a", yyjson::maximum(5)),
                     yyjson::field("s", yyjson::min_length(2)), yyjson::field("c", yyjson::minimum(1)),
                     yyjson::field("o", yyjson::maximum(3)));
// the members of VISITABLE_STRUCT in another order than they are declared, and a std::optional field with a default
// value
struct ConstrainedReordered
{
    int a = 0;
    std::string b;
    std::optional<int> o = 5;
    bool operator==(const ConstrainedReordered&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedReordered> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("b", yyjson::max_length(1)),
                     yyjson::field("o", yyjson::maximum(3)));
VISITABLE_STRUCT(ConstrainedReordered, b, o, a);

// a field that keeps its default value, which is out of the bounds, when the key is missing or null
struct ConstrainedDefaults
{
    int a = -1;
    std::optional<int> o = -1;
    bool operator==(const ConstrainedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedDefaults> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(0), yyjson::default_), yyjson::field("o", yyjson::minimum(0)));

// The rules that the static assertions enforce, tested on the conditions that they assert
namespace constraint_rules
{
    // the table of the keys of a struct of the members `v` and `w`
    template <typename... As>
    consteval auto table_error(const As&... annotations)
    {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, false};
        (builder.apply_annotation(annotations), ...);
        return builder.result().error;
    }
    using enum yyjson::detail::field_key_error;
    using yyjson::field, yyjson::minimum, yyjson::maximum;
    static_assert(table_error(field("v", minimum(0)), field("w", yyjson::pattern("^a"))) == none);
    static_assert(table_error(minimum(0)) == keyword_kind);
    static_assert(table_error(yyjson::not_(yyjson::min_items(1))) == keyword_kind);
    static_assert(table_error(yyjson::any_of(yyjson::const_(1))) == struct_value_keyword);
    static_assert(table_error(field("v", yyjson::skip, minimum(0))) == skipped_with_setting);
    static_assert(table_error(field("v", minimum(0)), field("v", yyjson::skip)) == skipped_with_setting);
    static_assert(table_error(field("w", minimum(0), yyjson::adjacent_tag("t", {"x"}))) == tagged_constraint);
    static_assert(table_error(field("v", minimum(0)), field("w", yyjson::adjacent_tag("t", {"x"}))) == none);
    // the keywords of an object that a struct takes: `yyjson::additional_properties(false)` and the counts
    using yyjson::additional_properties;
    static_assert(table_error(additional_properties(false)) == none);
    static_assert(table_error(additional_properties(false), additional_properties(false)) == closed_twice);
    static_assert(table_error(additional_properties(minimum(0))) == none);
    static_assert(table_error(yyjson::min_properties(1)) == none);
    static_assert(table_error(yyjson::max_properties(1), additional_properties(false)) == none);
    static_assert(table_error(yyjson::property_names(yyjson::min_length(1))) == none);
    static_assert(table_error(yyjson::max_properties(1), yyjson::property_names(yyjson::min_length(1))) == none);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), additional_properties(false)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::min_properties(1)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::max_properties(1), yyjson::transparent, field("w", yyjson::skip)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip),
                              yyjson::property_names(yyjson::min_length(1))) == transparent_object_keyword);
    static_assert(table_error(yyjson::dependent_required("v", {"w"})) == none);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::dependent_required("v", {"w"})) ==
                  transparent_object_keyword);
    // the field of `yyjson::extra_keys`, which takes only the keywords of its values and keys
    using yyjson::extra_keys;
    static_assert(table_error(field("w", extra_keys)) == none);
    static_assert(table_error(field("w", extra_keys, additional_properties(minimum(0)),
                                    yyjson::property_names(yyjson::max_length(1)))) == none);
    static_assert(table_error(field("v", extra_keys), field("w", extra_keys)) == extra_keys_twice);
    static_assert(table_error(field("w", extra_keys, yyjson::rename("x"))) == extra_keys_with_setting);
    static_assert(table_error(field("w", yyjson::required), field("w", extra_keys)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::default_)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::skip)) == extra_keys_with_setting);
    static_assert(table_error(field("w", yyjson::skip), field("w", extra_keys)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::min_properties(1))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::min_items(1))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, minimum(0))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::adjacent_tag("t", {"x"}))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys), additional_properties(false)) == extra_keys_denied);
    static_assert(table_error(additional_properties(false), field("w", extra_keys)) == extra_keys_denied);
    static_assert(table_error(field("w", extra_keys), additional_properties(minimum(0))) == none);
    static_assert(table_error(yyjson::transparent, field("w", extra_keys)) == transparent_object_keyword);
    static_assert(table_error(field("w", extra_keys), yyjson::transparent) == transparent_object_keyword);
    static_assert([] {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, true};
        builder.apply_annotation(field("w", extra_keys));
        return builder.result().error;
    }() == extra_keys_visitable);
    // the fields of `yyjson::pattern_properties`, which take the same keywords, and a struct that is closed
    using yyjson::pattern_properties;
    static_assert(table_error(field("v", pattern_properties("^a")), field("w", pattern_properties(""))) == none);
    static_assert(table_error(field("w", pattern_properties("^a"), additional_properties(minimum(0)),
                                    yyjson::property_names(yyjson::max_length(1)))) == none);
    static_assert(table_error(field("v", pattern_properties("^a")), field("w", extra_keys)) == none);
    static_assert(table_error(additional_properties(false), field("w", pattern_properties("^a"))) == none);
    static_assert(table_error(field("w", pattern_properties("^a"), pattern_properties("^b"))) == pattern_twice);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::rename("x"))) == pattern_with_setting);
    static_assert(table_error(field("w", yyjson::required), field("w", pattern_properties("^a"))) ==
                  pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::default_)) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::skip)) == pattern_with_setting);
    static_assert(table_error(field("w", yyjson::skip), field("w", pattern_properties("^a"))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::min_properties(1))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::min_items(1))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), extra_keys)) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::adjacent_tag("t", {"x"}))) ==
                  pattern_with_setting);
    static_assert(table_error(yyjson::transparent, field("w", pattern_properties("^a"))) == transparent_object_keyword);
    static_assert([] {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, true};
        builder.apply_annotation(field("w", pattern_properties("^a")));
        return builder.result().error;
    }() == pattern_visitable);
    // 65 members of the names of one letter
    inline constexpr char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$";
    static_assert([] {
        auto members = std::array<std::string_view, 65>{};
        for (std::size_t i = 0; i < 65; ++i) members[i] = std::string_view(letters + i, 1);
        auto builder = yyjson::detail::field_key_builder<65>{members, {members}, false};
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (builder.apply_annotation(field(std::string_view(letters + I, 1), pattern_properties("^a"))), ...);
        }(std::make_index_sequence<65>());
        return builder.result().error;
    }() == too_many_patterns);
    using yyjson::detail::extra_keys_map;
    static_assert(extra_keys_map<std::map<std::string, int>> && extra_keys_map<std::unordered_map<std::string, int>> &&
                  extra_keys_map<std::map<std::string, yyjson::value>>);
    static_assert(!extra_keys_map<std::map<int, int>> && !extra_keys_map<std::multimap<std::string, int>> &&
                  !extra_keys_map<std::vector<std::pair<std::string, int>>> && !extra_keys_map<yyjson::object>);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::one_of(yyjson::required("v"))) ==
                  transparent_conditions);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::required("v")) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::property("v", yyjson::minimum(0)), yyjson::transparent,
                              field("w", yyjson::skip)) == transparent_object_keyword);
}  // namespace constraint_rules

// a bound is compared as a double, so a long double that it would round does not compile; Clang 17 and older
// take no floating-point template argument
#if !defined(__clang__) || __clang_major__ >= 18
template <auto V>
concept bound_accepted = requires { typename std::integral_constant<bool, (yyjson::minimum(V), true)>; };
static_assert(bound_accepted<1.5L> && bound_accepted<9007199254740992.0L>);
static_assert(!bound_accepted<9007199254740993.0L> || sizeof(long double) == sizeof(double));
#endif

namespace struct_condition_rules
{
    // the settings that a condition of a struct takes, which the types alone tell
    template <typename... As>
    concept conditions_accepted = requires { yyjson::not_(std::declval<As>()...); };
    static_assert(conditions_accepted<decltype(yyjson::field("k", yyjson::required, yyjson::minimum(0)))>);
    static_assert(yyjson::detail::has_struct_conditions<ConditionShape> &&
                  yyjson::detail::has_struct_conditions<ConditionVisitable> &&
                  !yyjson::detail::has_struct_conditions<Bounded> && !yyjson::detail::has_constraints<Contact>);

    // `yyjson::dependent_required` and `yyjson::required("key", ...)` name the keys of the fields, each once
    template <int K>
    consteval auto make_presence()
    {
        if constexpr (K == 0)
            return yyjson::dependent_required("a", {"b", "c"});
        else if constexpr (K == 1)
            return yyjson::dependent_required("a", {"b", "b"});
        else if constexpr (K == 2)
            return yyjson::dependent_required("a", {"b", "a"});
        else if constexpr (K == 3)
            return yyjson::dependent_required("a", {});
        else if constexpr (K == 4)
            return yyjson::one_of(yyjson::required("a"), yyjson::required("a", "b"));
        else if constexpr (K == 5)
            return yyjson::one_of(yyjson::required("a"), yyjson::required("b", "c", "b"));
        else
            return yyjson::any_of(yyjson::required("a", "a"));
    }
    template <int K>
    concept presence_made = requires { typename std::integral_constant<bool, (make_presence<K>(), true)>; };
    static_assert(presence_made<0> && !presence_made<1> && !presence_made<2> && !presence_made<3> && presence_made<4>);
    // a string literal given twice is an error of the tree before C++26, and a std::string_view of the call
#if defined(__cpp_lib_define_static)
    static_assert(!presence_made<5> && !presence_made<6>);
#else
    static_assert(presence_made<5> && presence_made<6>);
#endif
    template <typename S>
    concept keys_made = requires(S s) { yyjson::one_of(yyjson::required(s), yyjson::required(s, s)); };
    static_assert(keys_made<std::string_view> && keys_made<std::string>);
    // a key that no field has, which a renamed or skipped member does not
    template <typename T>
    constexpr auto presence_error =
        yyjson::detail::check_struct_tree<T>(yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_tree<T>>());
    using enum yyjson::detail::struct_condition_error;
    static_assert(presence_error<PresenceRenamed> == unknown_key && presence_error<PresenceSkipped> == unknown_key &&
                  presence_error<PresenceKnown> == none);
    static_assert(presence_error<PresenceExtra> == none && presence_error<ConditionExtra> == none);
    static_assert(presence_error<PresencePattern> == none && presence_error<ConditionPattern> == none);
    static_assert(presence_error<PresenceNestedKnown> == none);
    static_assert(presence_error<ClosedAllOfKeys> == closed_required && presence_error<ClosedAllOfField> == none);
    static_assert(!yyjson::detail::has_struct_conditions<NestedAllOf> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfProperty> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfMixed> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfDeep> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfObject>);
    static_assert(yyjson::detail::has_struct_conditions<PresenceKnown> &&
                  !yyjson::detail::has_constraints<PresenceKnown>);
    // the keywords of an object at a position of a struct, which a map, a struct written as an array and a value
    // of another type do not take as a struct
    using yyjson::detail::holds_object_struct, yyjson::detail::object_struct_value;
    static_assert(object_struct_value<Positioned>() && holds_object_struct<PositionedAlternatives>() &&
                  holds_object_struct<ClosedAlternatives>());
    static_assert(!object_struct_value<int>() && !object_struct_value<std::map<std::string, int>>() &&
                  !object_struct_value<std::vector<Positioned>>() && !object_struct_value<Scores>() &&
                  !holds_object_struct<std::variant<int, std::string>>() && !object_struct_value<yyjson::object>());
    using yyjson::detail::field_schema, yyjson::detail::struct_object_node;
    static_assert(field_schema<Positions, 0>::node.closed && field_schema<Positions, 3>::node.closed &&
                  !struct_object_node<field_schema<Positions, 1>> && !struct_object_node<field_schema<Positions, 4>>);
    // they are settings of a subschema and of a field too, as a struct at any position takes them
    static_assert(
        yyjson::detail::subschema_setting<decltype(yyjson::one_of(yyjson::required("a"), yyjson::required("b")))> &&
        yyjson::detail::subschema_setting<decltype(yyjson::dependent_required("a", {"b"}))> &&
        yyjson::detail::field_setting<decltype(yyjson::any_of(yyjson::required("a")))>);
}  // namespace struct_condition_rules

TEST(Constraints, Numbers)
{
    expect_accepted<Bounded>(R"({"i":0,"d":0.5,"u":9,"o":-5})", {0, 0.5, 9, -5});
    expect_accepted<Bounded>(R"({"i":10,"d":1.25,"u":0})", {10, 1.25, 0, {}});
    expect_accepted<Bounded>(R"({"o":null})", {});

    expect_rejected<Bounded>(R"({"i":-1})",
                             R"(the number is less than the minimum 0 at /i; reading the key "i" of Bounded)", "/i");
    expect_rejected<Bounded>(
        R"({"i":11})", R"(the number is greater than the maximum 10 at /i; reading the key "i" of Bounded)", "/i");
    expect_rejected<Bounded>(
        R"({"d":0})", R"(the number is not greater than the exclusive minimum 0 at /d; reading the key "d" of Bounded)",
        "/d");
    expect_rejected<Bounded>(
        R"({"d":1.5})",
        R"(the number is not less than the exclusive maximum 1.5 at /d; reading the key "d" of Bounded)", "/d");
    expect_rejected<Bounded>(R"({"u":4})", R"(the number is not a multiple of 3 at /u; reading the key "u" of Bounded)",
                             "/u");
    expect_rejected<Bounded>(R"({"i":1,"o":-6})",
                             R"(the number is less than the minimum -5 at /o; reading the key "o" of Bounded)", "/o");
    // the key that the object leaves out is not checked, and the field keeps a value out of the bounds
    EXPECT_EQ(yyjson::read<Bounded>(R"({"i":1})").d, 1);
    // `yyjson::strict` checks the keys besides
    EXPECT_EQ(yyjson::read<Bounded>(R"({"i":1,"d":1,"u":3})", yyjson::strict), (Bounded{1, 1, 3, {}}));
    EXPECT_THROW((void)yyjson::read<Bounded>(R"({"i":1,"d":1,"u":2})", yyjson::strict), yyjson::bad_cast);

    expect_accepted<MixedBounds>(R"({"big":9007199254740992,"ubig":9223372036854775808,"real":9007199254740992,)"
                                 R"("negative":-7,"quarter":0.75})",
                                 {9007199254740992, 9223372036854775808ULL, 9007199254740992.0, -7, 0.75F});
    expect_accepted<MixedBounds>(R"({"negative":-9223372036854775807})", {0, 0, 0, -9223372036854775807LL, 0});
    expect_rejected<MixedBounds>(
        R"({"big":9007199254740993})",
        R"(the number is greater than the maximum 9007199254740992 at /big; reading the key "big" of MixedBounds)",
        "/big");
    expect_rejected<MixedBounds>(
        R"({"ubig":1})",
        R"(the number is not a multiple of 9223372036854775808 at /ubig; reading the key "ubig" of MixedBounds)",
        "/ubig");
    expect_rejected<MixedBounds>(
        R"({"real":9007199254740994})",
        R"(the number is greater than the maximum 9007199254740993 at /real; reading the key "real" of MixedBounds)",
        "/real");
    expect_rejected<MixedBounds>(
        R"({"negative":-2})",
        R"(the number is not less than the exclusive maximum -2.5 at /negative; reading the key "negative" of MixedBounds)",
        "/negative");
    expect_rejected<MixedBounds>(
        R"({"negative":-6})",
        R"(the number is not a multiple of 7 at /negative; reading the key "negative" of MixedBounds)", "/negative");
    expect_rejected<MixedBounds>(
        R"({"quarter":0.3})",
        R"(the number is not a multiple of 0.25 at /quarter; reading the key "quarter" of MixedBounds)", "/quarter");

    // a multiple of a decimal is decided in decimal, which the quotient in binary is not
    for (const auto* json : {R"({"cents":19.99})", R"({"cents":0.07})", R"({"cents":-1234567.89})", R"({"cents":1e-2})",
                             R"({"tenths":-30})", R"({"tenths":0})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<MixedBounds>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).cast<MixedBounds>()) << json;
    }
    expect_rejected<MixedBounds>(
        R"({"cents":0.001})",
        R"(the number is not a multiple of 0.01 at /cents; reading the key "cents" of MixedBounds)", "/cents");
    expect_rejected<MixedBounds>(
        R"({"cents":19.995})",
        R"(the number is not a multiple of 0.01 at /cents; reading the key "cents" of MixedBounds)", "/cents");
    // an integer field is a multiple of 0.1 whatever it holds
    EXPECT_EQ(yyjson::read<MixedBounds>(R"({"tenths":7})").tenths, 7);
}

TEST(Constraints, FloatBounds)
{
    // the JSON number is compared before the read rounds it to a float
    expect_accepted<FloatBounds>(R"({"tenth":0.1,"under":0.09999999,"big":16777217,"step":0.3})",
                                 {0.1F, 0.09999999F, 16777216.0F, 0.3F});
    for (const auto* json : {R"({"tenth":0.10000001})", R"({"tenth":0.10000000149011612})"})
        expect_rejected<FloatBounds>(
            json, R"(the number is greater than the maximum 0.1 at /tenth; reading the key "tenth" of FloatBounds)",
            "/tenth");
    expect_rejected<FloatBounds>(
        R"({"under":0.1})",
        R"(the number is not less than the exclusive maximum 0.1 at /under; reading the key "under" of FloatBounds)",
        "/under");
    expect_rejected<FloatBounds>(
        R"({"big":16777216})",
        R"(the number is less than the minimum 16777217 at /big; reading the key "big" of FloatBounds)", "/big");
    expect_rejected<FloatBounds>(
        R"({"step":0.30000001})",
        R"(the number is not a multiple of 0.1 at /step; reading the key "step" of FloatBounds)", "/step");

    expect_accepted<FloatElements>(R"({"v":[0.1,0.05],"m":{"a":0.1},"u":0.1})", {{0.1F, 0.05F}, {{"a", 0.1F}}, 0.1F});
    expect_rejected<FloatElements>(
        R"({"v":[0.1,0.10000001]})",
        R"(the number is greater than the maximum 0.1 at /v/1; reading an element of the value at the key "v" of FloatElements)",
        "/v/1");
    expect_rejected<FloatElements>(
        R"({"m":{"a":0.1,"b":0.10000001}})",
        R"(the number is greater than the maximum 0.1 at /m/b; reading an element of the value at the key "m" of FloatElements)",
        "/m/b");
    expect_rejected<FloatElements>(
        R"({"u":0.10000001})",
        R"(the number is greater than the maximum 0.1 at /u; reading the key "u" of FloatElements)", "/u");
}

TEST(Constraints, RealBounds)
{
    // the JSON integer is compared before the read rounds it to a double
    expect_rejected<RealBounds>(
        R"({"top":9007199254740993})",
        R"(the number is greater than the maximum 9007199254740992 at /top; reading the key "top" of RealBounds)",
        "/top");
    expect_rejected<RealBounds>(
        R"({"wide":-9007199254740993})",
        R"(the number is not greater than the exclusive minimum -9007199254740992 at /wide; reading the key "wide" of RealBounds)",
        "/wide");
    // the text of a real is read as a double before the comparison
    expect_accepted<RealBounds>(R"({"top":9007199254740992,"wide":-9007199254740991})",
                                {9007199254740992.0, 0, 0, 0, 0, 0, -9007199254740991.0L});
    expect_accepted<RealBounds>(R"({"top":9007199254740993.0})", {9007199254740992.0});

    // a decimal multiple is decided on the shortest decimal of the double
    expect_accepted<RealBounds>(R"({"tenth":0.3,"cent":-1234567.89,"half":5,"third":4.5,"three":9007199254740993})",
                                {0, 0.3, -1234567.89, 5, 4.5, 9007199254740992.0});
    for (const auto* json : {R"({"tenth":-0.3})", R"({"tenth":1e300})", R"({"tenth":0.1e-0})"})
        EXPECT_NO_THROW((void)yyjson::read<RealBounds>(json)) << json;
    for (const auto* json : {R"({"tenth":0.30000000000000004})", R"({"tenth":0.35})", R"({"tenth":1e-300})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 0.1 at /tenth; reading the key "tenth" of RealBounds)", "/tenth");
    expect_rejected<RealBounds>(
        R"({"cent":0.001})", R"(the number is not a multiple of 0.01 at /cent; reading the key "cent" of RealBounds)",
        "/cent");
    for (const auto* json : {R"({"third":4.0})", R"({"third":4})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 1.5 at /third; reading the key "third" of RealBounds)", "/third");
    for (const auto* json : {R"({"three":9007199254740992})", R"({"three":1e300})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 3 at /three; reading the key "three" of RealBounds)", "/three");

    expect_accepted<RealElements>(R"({"v":[9007199254740992],"u":0.7})", {{9007199254740992.0}, 0.7});
    expect_rejected<RealElements>(
        R"({"v":[0,9007199254740993]})",
        R"(the number is greater than the maximum 9007199254740992 at /v/1; reading an element of the value at the key "v" of RealElements)",
        "/v/1");
    expect_rejected<RealElements>(R"({"u":0.7000000000000001})",
                                  R"(the number is not a multiple of 0.1 at /u; reading the key "u" of RealElements)",
                                  "/u");
}

TEST(Constraints, WideDivisors)
{
    // an integer divisor divides the JSON integer exactly
    for (const auto* json : {R"({"big":9007199254740993})", R"({"big":-9007199254740993})"})
        expect_rejected<WideDivisors>(
            json, R"(the number is not a multiple of 9007199254740992 at /big; reading the key "big" of WideDivisors)",
            "/big");
    expect_rejected<WideDivisors>(
        R"({"ubig":9007199254740993})",
        R"(the number is not a multiple of 9007199254740992 at /ubig; reading the key "ubig" of WideDivisors)",
        "/ubig");
    expect_accepted<WideDivisors>(R"({"big":18014398509481984,"ubig":18014398509481984})",
                                  {18014398509481984.0, 18014398509481984U});
    // a real divisor takes its shortest decimal, with any exponent
    expect_accepted<WideDivisors>(R"({"tiny":3e-18,"odd":900719925474098.8})", {0, 0, 3e-18, 900719925474098.8});
    expect_accepted<WideDivisors>(R"({"tiny":1e300})", {0, 0, 1e300});
    expect_rejected<WideDivisors>(
        R"({"tiny":3.5e-18})",
        R"(the number is not a multiple of 1e-18 at /tiny; reading the key "tiny" of WideDivisors)", "/tiny");
    expect_rejected<WideDivisors>(
        R"({"odd":900719925474098.9})",
        R"(the number is not a multiple of 900719925474098.8 at /odd; reading the key "odd" of WideDivisors)", "/odd");
    // no integer of 64 bits but 0 is a multiple of 1e20
    expect_accepted<WideDivisors>(R"({"huge":0})", {});
    expect_rejected<WideDivisors>(
        R"({"huge":9223372036854775807})",
        R"(the number is not a multiple of 1e+20 at /huge; reading the key "huge" of WideDivisors)", "/huge");
}

TEST(Constraints, NaN)
{
    // NaN is in no bounds
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    EXPECT_THROW((void)yyjson::read<Bounded>(R"({"d":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":Infinity})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"cents":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"d":NaN})", flag).cast<Bounded>(), yyjson::bad_cast);
}

TEST(Constraints, Texts)
{
    expect_accepted<Texts>(R"({"name":"ab","code":"XYZ","note":"nb"})", {"ab", "XYZ", "nb"});
    // the length is in code points
    expect_accepted<Texts>(R"({"name":"日本語"})", {"日本語", "ABC", {}});

    expect_rejected<Texts>(
        R"({"name":"a"})",
        R"(the string is shorter than the minimum length 2 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"日本語x"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"ab","code":"abc"})",
        R"(the string does not match the pattern "^[A-Z]{3}$" at /code; reading the key "code" of Texts)", "/code");
    expect_rejected<Texts>(R"({"name":"ab","note":"x"})",
                           R"(the string does not match the pattern "^n" at /note; reading the key "note" of Texts)",
                           "/note");
}

TEST(Constraints, Patterns)
{
    expect_accepted<Patterns>(R"({"s":"abz","o":"oxo","v":"123"})", {"abz", "oxo", "123"});
    expect_accepted<Patterns>(R"({"s":"az"})", {"az", {}, {}});

    expect_rejected<Patterns>(
        R"({"s":"ab"})",
        R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)", "/s");
    expect_rejected<Patterns>(
        R"({"s":"bz"})",
        R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)", "/s");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"oo"})",
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"xo"})",
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"o"})",
        R"(the string is shorter than the minimum length 2 at /o; reading the key "o" of Patterns)", "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","v":"1a"})",
        R"(the string does not match the pattern "^[0-9]*$" at /v; reading the key "v" of Patterns)", "/v");

    // validate and the checked write check them as well
    expect_valid(Patterns{"abz", "oxo", "123"});
    expect_invalid(Patterns{"ab", {}, {}},
                   R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)",
                   "/s");
    expect_invalid(
        Patterns{"az", "oxx", {}},
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_invalid(Patterns{"az", "o", {}},
                   R"(the string is shorter than the minimum length 2 at /o; reading the key "o" of Patterns)", "/o");
    expect_invalid(Patterns{"az", {}, "1a"},
                   R"(the string does not match the pattern "^[0-9]*$" at /v; reading the key "v" of Patterns)", "/v");
}

TEST(Constraints, Satisfies)
{
    expect_accepted<Validated>(R"({"even":2,"text":"ok","nonzero":null})", {2, "ok", {}});
    expect_accepted<Validated>(R"({"nonzero":1})", {0, {}, 1});

    expect_rejected<Validated>(
        R"({"even":1})", R"(the value does not satisfy the predicate at /even; reading the key "even" of Validated)",
        "/even");
    expect_rejected<Validated>(
        R"({"text":"bad"})",
        R"(the value does not satisfy the predicate at /text; reading the key "text" of Validated)", "/text");
    expect_rejected<Validated>(
        R"({"nonzero":0})",
        R"(the value does not satisfy the predicate at /nonzero; reading the key "nonzero" of Validated)", "/nonzero");
}

TEST(Constraints, Structs)
{
    expect_accepted<ConstrainedVisitable>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedVisitable>(
        R"({"a":0})", R"(the number is less than the minimum 1 at /a; reading the key "a" of ConstrainedVisitable)",
        "/a");
    expect_rejected<ConstrainedVisitable>(
        R"({"b":"xy"})",
        R"(the string is longer than the maximum length 1 at /b; reading the key "b" of ConstrainedVisitable)", "/b");

    expect_accepted<ConstrainedLarge>(R"({"i":1,"r":0,"a":5})", {5, 0, 0, 0, 0, 0, 0, 0, 1, 0});
    expect_rejected<ConstrainedLarge>(
        R"({"a":1,"i":2})",
        R"(the number is greater than the maximum 1 at /i; reading the key "i" of ConstrainedLarge)", "/i");
    expect_rejected<ConstrainedLarge>(
        R"({"r":-1})", R"(the number is less than the minimum 0 at /r; reading the key "r" of ConstrainedLarge)", "/r");

    expect_rejected<ConstrainedOuter>(
        R"({"items":[{"i":1},{"i":12}]})",
        R"(the number is greater than the maximum 10 at /items/1/i; reading the key "i" of Bounded)", "/items/1/i");
    expect_rejected<ConstrainedOuter>(
        R"({"texts":{"name":"x"}})",
        R"(the string is shorter than the minimum length 2 at /texts/name; reading the key "name" of Texts)",
        "/texts/name");
    // the position of the value is added
    const auto e = bad_cast_of([] { (void)yyjson::read<ConstrainedOuter>(R"({"items":[{"i":-3}]})"); });
    ASSERT_TRUE(e.position().has_value());
    EXPECT_EQ(e.position()->pos, 15);

    // the fields are checked by their own constraints whatever their places in the struct
    expect_accepted<ConstrainedSkipped>(R"({"a":5,"s":"xy","c":1})", {0, 5, "xy", 1, 5});
    expect_rejected<ConstrainedSkipped>(
        R"({"a":6})", R"(the number is greater than the maximum 5 at /a; reading the key "a" of ConstrainedSkipped)",
        "/a");
    expect_rejected<ConstrainedSkipped>(
        R"({"a":1,"s":"x"})",
        R"(the string is shorter than the minimum length 2 at /s; reading the key "s" of ConstrainedSkipped)", "/s");
    expect_rejected<ConstrainedSkipped>(
        R"({"c":0})", R"(the number is less than the minimum 1 at /c; reading the key "c" of ConstrainedSkipped)",
        "/c");
    expect_rejected<ConstrainedSkipped>(
        R"({"o":4})", R"(the number is greater than the maximum 3 at /o; reading the key "o" of ConstrainedSkipped)",
        "/o");
    expect_accepted<ConstrainedReordered>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedReordered>(
        R"({"b":"xy","a":1})",
        R"(the string is longer than the maximum length 1 at /b; reading the key "b" of ConstrainedReordered)", "/b");
    expect_rejected<ConstrainedReordered>(
        R"({"b":"x","a":0})",
        R"(the number is less than the minimum 1 at /a; reading the key "a" of ConstrainedReordered)", "/a");

    // null leaves a field at its value in `T()`, which is not checked
    expect_accepted<ConstrainedSkipped>(R"({"o":null})", {});
    expect_accepted<ConstrainedReordered>(R"({"o":null})", {});
    expect_rejected<ConstrainedReordered>(
        R"({"o":4})", R"(the number is greater than the maximum 3 at /o; reading the key "o" of ConstrainedReordered)",
        "/o");
    EXPECT_THROW((void)yyjson::read<ConstrainedSkipped>(R"({"o":null})", yyjson::strict), yyjson::bad_cast);

    // a default value that the key leaves is not checked, and a value that the key gives is
    expect_accepted<ConstrainedDefaults>("{}", {});
    expect_accepted<ConstrainedDefaults>(R"({"o":null})", {});
    expect_accepted<ConstrainedDefaults>(R"({"a":0,"o":0})", {0, 0});
    expect_rejected<ConstrainedDefaults>(
        R"({"a":-2})", R"(the number is less than the minimum 0 at /a; reading the key "a" of ConstrainedDefaults)",
        "/a");
    expect_rejected<ConstrainedDefaults>(
        R"({"o":-2})", R"(the number is less than the minimum 0 at /o; reading the key "o" of ConstrainedDefaults)",
        "/o");
}

TEST(Constraints, Transparent)
{
    expect_accepted<Percent>("50", {50});
    expect_rejected<Percent>("101", "the number is greater than the maximum 100; reading Percent", "");
    expect_accepted<Progress>(R"({"done":100,"steps":[0,100]})", {{100}, {{0}, {100}}});
    expect_rejected<Progress>(R"({"done":-1})", "the number is less than the minimum 0 at /done; reading Percent",
                              "/done");
    expect_rejected<Progress>(R"({"steps":[1,200]})",
                              "the number is greater than the maximum 100 at /steps/1; reading Percent", "/steps/1");
}

TEST(Constraints, Merge)
{
    // null resets a field without a check, and a struct merged into a field is checked as a whole
    auto t = Bounded{5, 1, 3, 2};
    yyjson::merge_into(R"({"o":null,"i":null})", t);
    EXPECT_EQ(t, (Bounded{0, 1, 3, {}}));
    EXPECT_THROW(yyjson::merge_into(R"({"u":5})", t), yyjson::bad_cast);

    auto outer = ConstrainedOuter{{}, {"ab", "ABC", {}}};
    EXPECT_EQ(expect_merge_into(outer, R"({"texts":{"note":"n"}})"),
              R"({"items":[],"texts":{"name":"ab","code":"ABC","note":"n"}})");
    EXPECT_THROW(yyjson::merge_into(R"({"texts":{"code":"x"}})", outer), yyjson::bad_cast);

    auto progress = Progress{{5}, {}};
    EXPECT_EQ(expect_merge_into(progress, R"({"done":7})"), R"({"done":7,"steps":[]})");
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"done":101})", progress); });
    EXPECT_EQ(e.path(), "/done");
}

TEST(Constraints, Write)
{
    // a value out of the bounds is written as it is
    EXPECT_EQ(written(Bounded{-1, 0, 1, {}}), R"({"i":-1,"d":0.0,"u":1})");
    EXPECT_EQ(written(Percent{200}), "200");
}

// NOLINTEND
