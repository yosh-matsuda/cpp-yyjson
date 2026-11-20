#include "test_constraints_common.hpp"

// NOLINTBEGIN

// the tree of the keywords of the field `v` of a struct, by its error
#define CONSTRAINT_RULE(NAME, TYPE, ...) \
    struct NAME                          \
    {                                    \
        TYPE v;                          \
    };                                   \
    template <>                          \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(yyjson::field("v", __VA_ARGS__));
// `yyjson::property` on the struct adds keywords to the value of the field that `yyjson::field` constrains, each
// once
struct PropertyMerged
{
    int v = 0;
    bool operator==(const PropertyMerged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PropertyMerged> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(0)), yyjson::property("v", yyjson::maximum(9)));
struct PropertyTwice
{
    int v = 0;
};
template <>
inline constexpr auto yyjson::annotations<PropertyTwice> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(0)), yyjson::property("v", yyjson::minimum(1)));
// yyjson::additional_properties(false) allows the keys of yyjson::property only, and not those of yyjson::required
struct ClosedKeys
{
    std::optional<int> zip;
    std::optional<int> city;
};
CONSTRAINT_RULE(ClosedRequiredOnly, ClosedKeys, yyjson::additional_properties(false), yyjson::property("city"),
                yyjson::required("zip"))
CONSTRAINT_RULE(ClosedRequiredNamed, ClosedKeys, yyjson::additional_properties(false), yyjson::property("zip"),
                yyjson::required("zip"))
CONSTRAINT_RULE(ClosedRequiredAllOf, ClosedKeys, yyjson::additional_properties(false),
                yyjson::all_of(yyjson::required("zip")))
CONSTRAINT_RULE(ClosedRequiredOneOf, ClosedKeys, yyjson::one_of(yyjson::required("zip"), yyjson::required("city")),
                yyjson::additional_properties(false), yyjson::property("zip"), yyjson::property("city"))
#if !defined(__cpp_lib_define_static)
CONSTRAINT_RULE(RequiredKeyTwice, ClosedKeys, yyjson::any_of(yyjson::required("zip", "zip")))
#endif
CONSTRAINT_RULE(MinimumTwice, int, yyjson::minimum(0), yyjson::minimum(1))
CONSTRAINT_RULE(ConstTwice, int, yyjson::const_(1), yyjson::const_(1))
CONSTRAINT_RULE(EnumTwice, int, yyjson::enum_(1), yyjson::enum_(2))
CONSTRAINT_RULE(FormatTwice, std::string, yyjson::format("date"), yyjson::format("time"))
CONSTRAINT_RULE(SatisfiesTwice, int, yyjson::satisfies<is_even>, yyjson::satisfies<is_even>)
CONSTRAINT_RULE(PatternsTwice, std::string, yyjson::pattern("^a"), yyjson::pattern("^a"))
CONSTRAINT_RULE(MultipleOfZero, int, yyjson::multiple_of(0))
CONSTRAINT_RULE(MultipleOfNegative, double, yyjson::multiple_of(-0.5))
CONSTRAINT_RULE(BoundsInverted, int, yyjson::minimum(2), yyjson::maximum(1))
CONSTRAINT_RULE(BoundsExclusive, int, yyjson::exclusive_minimum(1), yyjson::maximum(1))
CONSTRAINT_RULE(BoundsEqual, int, yyjson::minimum(1), yyjson::maximum(1))
CONSTRAINT_RULE(LengthsInverted, std::string, yyjson::min_length(3), yyjson::max_length(2))
CONSTRAINT_RULE(ConstOutside, int, yyjson::const_(5), yyjson::maximum(4))
CONSTRAINT_RULE(EnumOutside, std::string, yyjson::enum_("abc", "de"), yyjson::max_length(1))
CONSTRAINT_RULE(EnumPartlyInside, std::string, yyjson::enum_("abc", "d"), yyjson::max_length(1))
CONSTRAINT_RULE(ItemsInverted, std::vector<int>, yyjson::min_items(3), yyjson::max_items(2))
CONSTRAINT_RULE(ContainsMissing, std::vector<int>, yyjson::min_contains(1))
CONSTRAINT_RULE(ContainsInverted, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(2),
                yyjson::max_contains(1))
using StringMap = std::map<std::string, int>;
using ScalarVariant = std::variant<int, std::string>;
CONSTRAINT_RULE(PropertiesInverted, StringMap, yyjson::min_properties(2), yyjson::max_properties(1))
CONSTRAINT_RULE(ItemsTwice, std::vector<int>, yyjson::items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(1)))
CONSTRAINT_RULE(ItemsInside, std::vector<int>, yyjson::items(yyjson::minimum(2), yyjson::maximum(1)))
// A position holds at most eight nodes. Count in breadth to avoid MSVC's initializer depth limit.
constexpr auto nine_nodes = yyjson::detail::prefix_schema{
    yyjson::items(yyjson::items(yyjson::items(yyjson::minimum(0))), yyjson::contains(yyjson::minimum(0))),
    yyjson::contains(yyjson::items(yyjson::items(yyjson::minimum(0))), yyjson::contains(yyjson::minimum(0)))};
CONSTRAINT_RULE(PrefixTooLong, yyjson::value, yyjson::prefix_items(nine_nodes))
// a subschema of a combinator follows the rules of a field, and a field can take a combinator more than once
CONSTRAINT_RULE(NotTwice, int, yyjson::not_(yyjson::minimum(0)), yyjson::not_(yyjson::maximum(1)))
CONSTRAINT_RULE(NotInverted, int, yyjson::not_(yyjson::minimum(2), yyjson::maximum(1)))
CONSTRAINT_RULE(BranchMissingContains, std::vector<int>,
                yyjson::if_then_else(yyjson::if_(yyjson::min_items(1)), yyjson::then(yyjson::min_contains(1))))
CONSTRAINT_RULE(MatchesTwice, yyjson::value, yyjson::matches<int>, yyjson::matches<int>)
// the keywords of a position and of the subschemas of its yyjson::all_of constrain one value
CONSTRAINT_RULE(AllOfInverted, int, yyjson::minimum(1), yyjson::all_of(yyjson::maximum(0)))
CONSTRAINT_RULE(AllOfTwiceInverted, int, yyjson::all_of(yyjson::minimum(1)), yyjson::all_of(yyjson::maximum(0)))
CONSTRAINT_RULE(AllOfNestedInverted, int,
                yyjson::all_of(yyjson::all_of(yyjson::exclusive_minimum(0)), yyjson::not_(yyjson::const_(7))),
                yyjson::maximum(0))
CONSTRAINT_RULE(AllOfStricter, int, yyjson::minimum(0), yyjson::all_of(yyjson::minimum(1)))
CONSTRAINT_RULE(AllOfConstOutside, int, yyjson::const_(5), yyjson::all_of(yyjson::maximum(4)))
CONSTRAINT_RULE(AllOfItemsInverted, std::vector<int>, yyjson::min_items(3), yyjson::all_of(yyjson::max_items(2)))
CONSTRAINT_RULE(AllOfLengthsInverted, std::string, yyjson::all_of(yyjson::min_length(3), yyjson::max_length(2)))
CONSTRAINT_RULE(ContainsNone, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::max_contains(0))
// yyjson::contains of two subschemas count different elements
CONSTRAINT_RULE(ContainsAcrossAllOf, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(2),
                yyjson::all_of(yyjson::contains(yyjson::maximum(0)), yyjson::max_contains(1)))
// a subschema of yyjson::any_of, one_of, not_ and if_then_else may hold a branch that no value satisfies
CONSTRAINT_RULE(AnyOfDead, int,
                yyjson::any_of(yyjson::all_of(yyjson::minimum(5), yyjson::maximum(1)), yyjson::maximum(3)))
CONSTRAINT_RULE(OneOfDead, std::vector<int>,
                yyjson::one_of(yyjson::min_items(2), yyjson::all_of(yyjson::min_items(3), yyjson::max_items(1))))
CONSTRAINT_RULE(BranchesDead, std::string,
                yyjson::if_then_else(yyjson::if_(yyjson::min_length(3), yyjson::max_length(2)),
                                     yyjson::then(yyjson::const_("a"), yyjson::min_length(2)),
                                     yyjson::else_(yyjson::enum_("ab", "cd"), yyjson::max_length(1))))
CONSTRAINT_RULE(ContainsDead, std::vector<int>,
                yyjson::not_(yyjson::contains(yyjson::minimum(0)), yyjson::max_contains(0)))
CONSTRAINT_RULE(ByteDead, unsigned char, yyjson::not_(yyjson::minimum(256)))
CONSTRAINT_RULE(ByteOutside, unsigned char, yyjson::minimum(256))
CONSTRAINT_RULE(ByteAllOfOutside, unsigned char, yyjson::all_of(yyjson::maximum(-1)))
// a contradiction leaves no value of its kind, and a position of more kinds reads the others
CONSTRAINT_RULE(ValueBoundsInverted, yyjson::value, yyjson::minimum(1), yyjson::maximum(0))
CONSTRAINT_RULE(VariantBoundsInverted, ScalarVariant, yyjson::minimum(1), yyjson::maximum(0))
CONSTRAINT_RULE(ValueConstOutside, yyjson::value, yyjson::const_(5), yyjson::maximum(4))
CONSTRAINT_RULE(AnyOfTooLong, yyjson::value, yyjson::any_of(nine_nodes))
#undef CONSTRAINT_RULE
// the counts of the members of a struct follow the rules of those of a map
struct StructCountsInverted
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsInverted> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::max_properties(1));
struct StructCountsTwice
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsTwice> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::max_properties(1));
struct StructCountsEqual
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsEqual> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(1));
// the counts of the members of a struct and those of the subschemas of its yyjson::all_of constrain one object
struct StructAllOfCounts
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructAllOfCounts> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::all_of(yyjson::max_properties(1)));
struct StructAnyOfCounts
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructAnyOfCounts> = yyjson::annotate(
    yyjson::min_properties(1), yyjson::any_of(yyjson::max_properties(0), yyjson::property("v", yyjson::minimum(0))));
// the names of the members of a struct follow the rules of those of a map
struct StructNamesInverted
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructNamesInverted> =
    yyjson::annotate(yyjson::property_names(yyjson::min_length(3), yyjson::max_length(2)));
struct StructNamesTwice
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructNamesTwice> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::property_names(yyjson::max_length(1)));
namespace constraint_tree_rules
{
    template <typename T>
    constexpr auto names_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_names_tree<T>>().error;
    template <typename T>
    constexpr auto count_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>().error;
    template <typename T>
    constexpr auto tree_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_field_tree<T, 0>>().error;
    // whether the keywords of the position of the field `v` leave no value of its JSON kinds, or of its integer type
    template <typename T>
    constexpr auto tree_of = yyjson::detail::make_schema_tree<&yyjson::detail::build_field_tree<T, 0>>();
    template <typename T>
    constexpr bool empty_position =
        yyjson::detail::position_reads_nothing<yyjson::detail::constrained_value_t<decltype(T::v)>>(tree_of<T>, 0);
    template <typename T>
    constexpr bool out_of_type = yyjson::detail::position_out_of_type<decltype(T::v)>(tree_of<T>, 0);
    template <typename T>
    constexpr bool counts_empty =
        (yyjson::detail::position_empty_kinds(
             yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>(), 0) &
         yyjson::detail::object_kind) != 0;
    template <typename T>
    constexpr bool struct_empty = [] {
        constexpr auto counts =
            yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>().nodes[0].constraints;
        return (yyjson::detail::position_empty_kinds(
                    yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_tree<T>>(), 0, {&counts, 1}) &
                yyjson::detail::object_kind) != 0;
    }();
    template <typename T>
    constexpr bool names_empty = [] {
        constexpr auto tree = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_names_tree<T>>();
        return yyjson::detail::position_reads_nothing<std::string>(tree, tree.nodes[0].keys);
    }();
    using enum yyjson::detail::constraint_error;
    static_assert(tree_error<MinimumTwice> == keyword_twice && tree_error<ConstTwice> == keyword_twice &&
                  tree_error<EnumTwice> == keyword_twice && tree_error<FormatTwice> == keyword_twice &&
                  tree_error<SatisfiesTwice> == keyword_twice);
    static_assert(tree_error<PatternsTwice> == none);
    static_assert(tree_error<PropertyMerged> == none && tree_error<PropertyTwice> == keyword_twice);
    static_assert(tree_error<ClosedRequiredOnly> == closed_required &&
                  tree_error<ClosedRequiredAllOf> == closed_required);
    static_assert(tree_error<ClosedRequiredNamed> == none && tree_error<ClosedRequiredOneOf> == none);
#if !defined(__cpp_lib_define_static)
    static_assert(tree_error<RequiredKeyTwice> == required_key_twice);
#endif
    static_assert(tree_error<MultipleOfZero> == multiple_of_not_positive &&
                  tree_error<MultipleOfNegative> == multiple_of_not_positive);
    static_assert(empty_position<BoundsInverted> && empty_position<BoundsExclusive> &&
                  empty_position<LengthsInverted> && empty_position<ConstOutside> && empty_position<EnumOutside>);
    static_assert(!empty_position<BoundsEqual> && !empty_position<EnumPartlyInside>);
    static_assert(empty_position<ItemsInverted> && empty_position<PropertiesInverted> &&
                  empty_position<ContainsInverted>);
    // the subschema of yyjson::items is a position of its own
    static_assert(tree_error<ItemsInside> == none && !empty_position<ItemsInside> &&
                  yyjson::detail::position_reads_nothing<int>(tree_of<ItemsInside>,
                                                              tree_of<ItemsInside>.nodes[0].items));
    static_assert(tree_error<ContainsMissing> == contains_missing && tree_error<ItemsTwice> == keyword_twice);
    static_assert(tree_error<PrefixTooLong> == prefix_position_too_large);
    static_assert(tree_error<NotTwice> == none && tree_error<NotInverted> == none && !empty_position<NotInverted> &&
                  tree_error<BranchMissingContains> == contains_missing && tree_error<MatchesTwice> == keyword_twice &&
                  tree_error<AnyOfTooLong> == prefix_position_too_large);
    static_assert(counts_empty<StructCountsInverted> && count_error<StructCountsTwice> == keyword_twice &&
                  count_error<StructCountsEqual> == none && !counts_empty<StructCountsEqual>);
    static_assert(names_empty<StructNamesInverted> && names_error<StructNamesTwice> == keyword_twice);
    static_assert(struct_empty<StructAllOfCounts> && !struct_empty<StructAnyOfCounts>);
    static_assert(empty_position<AllOfInverted> && empty_position<AllOfTwiceInverted> &&
                  empty_position<AllOfNestedInverted> && empty_position<AllOfConstOutside> &&
                  empty_position<AllOfItemsInverted> && empty_position<AllOfLengthsInverted>);
    static_assert(!empty_position<AllOfStricter> && tree_error<AllOfStricter> == none);
    static_assert(empty_position<ContainsNone> && !empty_position<ContainsAcrossAllOf>);
    static_assert(tree_error<AnyOfDead> == none && !empty_position<AnyOfDead> && !empty_position<OneOfDead> &&
                  !empty_position<BranchesDead> && !empty_position<ContainsDead>);
    static_assert(!out_of_type<ByteDead> && out_of_type<ByteOutside> && out_of_type<ByteAllOfOutside>);
    static_assert(!empty_position<ValueBoundsInverted> && !empty_position<VariantBoundsInverted> &&
                  empty_position<BoundsInverted> && empty_position<ValueConstOutside>);
    // the JSON kinds of a variant are those of its alternatives
    using yyjson::detail::json_kinds_of, yyjson::detail::misplaced_keywords, yyjson::detail::constraint_bit;
    static_assert(misplaced_keywords(constraint_bit<yyjson::min_items>, json_kinds_of<ScalarVariant>()) != 0 &&
                  misplaced_keywords(constraint_bit<yyjson::min_items>,
                                     json_kinds_of<std::variant<int, std::vector<int>>>()) == 0);

    // the keywords that a field without them leaves unchecked, and those that are no check
    static_assert(!yyjson::detail::has_constraints<Spot> && yyjson::detail::has_constraints<Bounded>);
    static_assert(yyjson::detail::constrained_field<Bounded, 0> && !yyjson::detail::constrained_field<Described, 0>);
}  // namespace constraint_tree_rules

TEST(Constraints, StructConditions)
{
    using S = ConditionShape;
    expect_conditions<S>(R"({"kind":"circle","radius":1})", "");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1},"sides":[1,2,3]})", "");
    // another kind takes else_, whose keys are absent, and leaves the radius unchecked
    expect_conditions<S>(R"({"kind":"square","radius":-1})", "");
    expect_conditions<S>(
        R"({"kind":"circle"})",
        R"(a required key is missing; reading the key "radius" of ConditionShape, which yyjson::if_then_else requires)");
    expect_conditions<S>(
        R"({"kind":"circle","radius":-1})",
        R"(the value satisfies if and not then at /radius; reading the key "radius" of ConditionShape)", "/radius");
    expect_conditions<S>(R"({"kind":"square","p":{"x":"a"}})",
                         R"(the value satisfies neither if nor else at /p; reading the key "p" of ConditionShape)",
                         "/p");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1,"y":2}})", "");
    expect_conditions<S>(
        R"({"kind":"square","sides":[1,0,2]})",
        R"(the value satisfies neither if nor else at /sides/1; reading an element of the value at the key "sides" of ConditionShape)",
        "/sides/1");
    expect_conditions<S>(
        R"({"kind":"square","sides":[1,2]})",
        R"(the value satisfies neither if nor else at /sides; reading the key "sides" of ConditionShape)", "/sides");
    // not_ of a key holds for an object without the key, as JSON Schema's `not` of `properties`
    expect_conditions<S>(R"({"kind":"xy"})",
                         "the value satisfies the subschema of not; reading ConditionShape in yyjson::not_");
    expect_conditions<S>(R"({})", "the value satisfies the subschema of not; reading ConditionShape in yyjson::not_");
    // a null that the read takes as a missing key is no key
    expect_conditions<S>(
        R"({"kind":"circle","radius":null})",
        R"(a required key is missing; reading the key "radius" of ConditionShape, which yyjson::if_then_else requires)");

    expect_conditions<Leveled>(R"({"level":4})", "");
    expect_conditions<Leveled>(R"({"level":5,"note":"n"})", "");
    expect_conditions<Leveled>(
        R"({"level":5})",
        R"(a required key is missing; reading the key "note" of Leveled, which yyjson::if_then_else requires)");
    expect_conditions<Leveled>(R"({"level":5,"note":""})",
                               R"(the value satisfies neither if nor else at /note; reading the key "note" of Leveled)",
                               "/note");

    expect_conditions<ConditionVisitable>(R"({"a":1,"b":2})", "");
    expect_conditions<ConditionVisitable>(R"({"b":2})", "");
    expect_conditions<ConditionVisitable>(
        R"({"a":1})",
        R"(a required key is missing; reading the key "b" of ConditionVisitable, which yyjson::if_then_else requires)");
    expect_conditions<ConditionReordered>(R"({"a":1,"b":1,"c":5})", "");
    expect_conditions<ConditionReordered>(
        R"({"a":9,"b":9,"c":1})", "the value satisfies none of the subschemas of anyOf; reading ConditionReordered");
    expect_conditions<ConditionConstructed>(R"({"a_":0})", "");
    expect_conditions<ConditionConstructed>(R"({"a_":1,"b_":0})", "");
    expect_conditions<ConditionConstructed>(
        R"({"a_":1})",
        R"(a required key is missing; reading the key "b_" of ConditionConstructed, which yyjson::if_then_else requires)");

    // the keys of the object tell which fields it has, whatever values the fields have in T()
    expect_conditions<ConditionPresence>(R"({})", "");
    expect_conditions<ConditionPresence>(R"({"c":7})", "");
    expect_conditions<ConditionPresence>(R"({"a":5,"b":0,"c":7,"d":1})", "");
    expect_conditions<ConditionPresence>(R"({"d":1,"a":5})", "");
    expect_conditions<ConditionPresence>(
        R"({"a":5})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"b":0})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"c":7,"b":0})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"a":5,"b":0,"c":7,"e":0,"d":1})",
        "the value satisfies the subschema of not; reading ConditionPresence in yyjson::not_");
    expect_conditions<ConditionPresence>(
        R"({"e":0,"c":7})", "the value satisfies the subschema of not; reading ConditionPresence in yyjson::not_");

    expect_conditions<Contact>(R"({"name":"a","email":"e"})", "");
    expect_conditions<Contact>(R"({"id":1,"phone":"p","age":0})", "");
    // a yyjson::one_of or any_of of yyjson::required("key", ...) names the keys in its message
    expect_conditions<Contact>(
        R"({"email":"e"})",
        R"(the value satisfies none of the subschemas of anyOf; reading Contact without all the keys of one of ["name"], ["id"])");
    expect_conditions<Contact>(
        R"({"name":"a"})",
        R"(the value satisfies none of the subschemas of oneOf; reading Contact without all the keys of one of ["email"], ["phone"])");
    expect_conditions<Contact>(
        R"({"name":"a","email":"e","phone":"p"})",
        R"(the value satisfies more than one subschema of oneOf; reading Contact with all the keys of more than one of ["email"], ["phone"])");
    expect_conditions<Contact>(
        R"({"name":"a","email":"e","age":-1})",
        R"(the value does not satisfy all the subschemas of allOf at /age; reading the key "age" of Contact)", "/age");
    expect_conditions<Contact>(
        R"({"id":0,"email":"e"})",
        "the value does not satisfy all the subschemas of allOf; reading Contact in yyjson::not_");
}

TEST(Constraints, PropertyAndField)
{
    expect_accepted<PropertyMerged>(R"({"v":9})", {9});
    expect_rejected<PropertyMerged>(
        R"({"v":-1})", R"(the number is less than the minimum 0 at /v; reading the key "v" of PropertyMerged)", "/v");
    expect_rejected<PropertyMerged>(
        R"({"v":10})", R"(the number is greater than the maximum 9 at /v; reading the key "v" of PropertyMerged)",
        "/v");

    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1})",
        "");
    expect_conditions<ManyKeys>(R"({"z":1})", "");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1})",
        R"(the value satisfies none of the subschemas of oneOf; reading ManyKeys without all the keys of one of ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1,"z":1})",
        R"(the value satisfies more than one subschema of oneOf; reading ManyKeys with all the keys of more than one of ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");

    expect_conditions<NestedAllOfProperty>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfProperty>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfProperty)",
        "/b");
    expect_conditions<NestedAllOfMixed>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfMixed>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfMixed)",
        "/b");
    expect_conditions<NestedAllOfDeep>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfDeep>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfDeep)",
        "/b");
    expect_conditions<NestedAllOfObject>(R"({"a":1,"b":{"c":1}})", "");
    expect_conditions<NestedAllOfObject>(
        R"({"a":1,"b":{}})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfObject)",
        "/b");

    expect_conditions<PresenceNestedKnown>(R"({"v":{"bb":1,"a":0}})", "");
    expect_conditions<PresenceNestedKnown>(
        R"({"v":{"a":0}})",
        R"(the object does not satisfy properties at /v; reading the key "v" of PresenceNestedKnown)", "/v");
    expect_conditions<PresenceNestedKnown>(
        R"({"v":{"bb":1,"a":-1}})",
        R"(the object does not satisfy properties at /v; reading the key "v" of PresenceNestedKnown)", "/v");
}

TEST(Constraints, StructConditionsOfNull)
{
    // a null that the read takes as a missing key is no key, and the conditions do not check the value that the
    // field keeps
    expect_conditions<NullKeys>(R"({"a":null,"b":1})", "");
    expect_conditions<NullKeys>(
        R"({"a":null})",
        R"(the value satisfies none of the subschemas of oneOf; reading NullKeys without all the keys of one of ["a"], ["b"])");
    expect_conditions<NullKeys>(
        R"({"a":4})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of NullKeys)", "/a");
    // a null in a field that holds null is a present key, which a merge takes as a removal
    expect_conditions<NullKeys>(R"({"b":1,"c":null,"d":1})", "");
    expect_conditions<NullKeys>(
        R"({"b":1,"c":null})",
        R"(a required key is missing; reading the key "d" of NullKeys, which yyjson::if_then_else requires)", "",
        false);
}

TEST(Constraints, StructConditionsMerged)
{
    // merge_into checks the object after the merge: the members of the patch that are not null, and the fields that
    // the patch does not name and that the write gives, with the values that the merge gives
    auto c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"a","email":"e","age":3})", c);
    EXPECT_EQ(c.age, 3);
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"name":"a","email":"e","age":-1})", c); });
    EXPECT_EQ(message_of(e),
              R"(the value does not satisfy all the subschemas of allOf at /age; reading the key "age" of Contact)")
        << e.what();
    EXPECT_EQ(e.path(), "/age");
    yyjson::merge_into(R"({"age":4})", c);
    EXPECT_EQ(c, (Contact{.name = "a", .email = "e", .age = 4}));
    e = bad_cast_of([&] { yyjson::merge_into(R"({"name":null})", c); });
    EXPECT_EQ(
        message_of(e),
        R"(the value satisfies none of the subschemas of anyOf; reading Contact without all the keys of one of ["name"], ["id"])")
        << e.what();
    c = Contact{.name = "a", .email = "e"};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"phone":"p"})", c); });
    EXPECT_EQ(
        message_of(e),
        R"(the value satisfies more than one subschema of oneOf; reading Contact with all the keys of more than one of ["email"], ["phone"])")
        << e.what();
    c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"b","email":null,"phone":"p"})", c);
    EXPECT_EQ(c, (Contact{.name = "b", .phone = "p"}));
#if YYJSON_VERSION_HEX >= 0x000C00
    auto merged_directly = false;
    static_cast<void>(expect_merge_into(Contact{.name = "a", .email = "e"}, R"({"name":"b","email":"e","phone":"p"})",
                                        &merged_directly));
    EXPECT_FALSE(merged_directly);
    static_cast<void>(expect_merge_into(Contact{.name = "a", .email = "e"}, R"({"name":"b","email":null,"phone":"p"})",
                                        &merged_directly));
    EXPECT_TRUE(merged_directly);
#endif
    // read_into checks the keys of the text that it reads, not the values that `t` held before
    auto p = ConditionPresence{};
    yyjson::read_into(R"({"d":1})", p);
    EXPECT_THROW(yyjson::read_into(R"({"a":1})", p), yyjson::bad_cast);
}

TEST(Constraints, MergedObject)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    // a field equal to its default is not in the object that the write gives, and a field with
    // yyjson::emit_defaults is
    const auto server = yyjson::read<MergedServer>(R"({"host":"a"})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b"})"), R"({"host":"b","port":8080})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b","cert":"x"})"), R"({"host":"b","port":8080,"cert":"x"})");
    rejected(
        expect_merge_rule(server, R"({"port":8080})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedServer, which yyjson::dependent_required requires)");
    const auto secure = yyjson::read<MergedServer>(R"({"host":"a","port":9000,"cert":"x"})");
    EXPECT_EQ(expect_merge_rule(secure, R"({"host":"b"})"), R"({"host":"b","port":9000,"cert":"x"})");
    rejected(
        expect_merge_rule(secure, R"({"cert":null})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedServer, which yyjson::dependent_required requires)");
    EXPECT_EQ(expect_merge_rule(secure, R"({"cert":null,"port":null})"), R"({"host":"a","port":8080})");
    EXPECT_EQ(expect_merge_rule(MergedServer{"a", 8080, "x"}, R"({"cert":null})"), R"({"host":"a","port":8080})");
    rejected(
        expect_merge_rule(MergedEmitted{"a", 8080, "x"}, R"({"cert":null})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedEmitted, which yyjson::dependent_required requires)");

    // the keys that the patch does not name stay in the object
    const auto choice = yyjson::read<MergedChoice>(R"({"a":1})");
    rejected(
        expect_merge_rule(choice, R"({"b":2})"),
        R"(the value satisfies more than one subschema of oneOf; reading MergedChoice with all the keys of more than one of ["a"], ["b"])");
    EXPECT_EQ(expect_merge_rule(choice, R"({"x":1})"), R"({"a":1})");
    EXPECT_EQ(expect_merge_rule(choice, R"({"a":null,"b":2})"), R"({"b":2})");
    rejected(
        expect_merge_rule(choice, R"({"a":null})"),
        R"(the value satisfies none of the subschemas of oneOf; reading MergedChoice without all the keys of one of ["a"], ["b"])");

    // a null removes the key that the object needs
    EXPECT_EQ(expect_merge_rule(MergedRequired{1, {}}, R"({"n":2})"), R"({"id":1,"n":2})");
    rejected(expect_merge_rule(MergedRequired{1, {}}, R"({"id":null})"),
             R"(a required key is missing; reading the key "id" of MergedRequired)");
    EXPECT_THROW(yyjson::merge_into(R"({"id":null})", *std::make_unique<MergedRequired>()), yyjson::bad_cast);

    // the counts take the members of the object after the merge
    const auto counted = [](int a, std::optional<int> b, int z) {
        auto t = MergedCounted{a, b, {}};
        t.z = z;
        return t;
    };
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"b":2})"), R"({"a":1,"b":2})");
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"z":3})"), R"({"a":1,"z":3})");
    rejected(expect_merge_rule(counted(1, {}, 3), R"({"b":2})"),
             "the object has more properties than the maximum 2; reading MergedCounted");
    rejected(expect_merge_rule(counted(1, {}, 0), R"({"b":2,"y":1})"),
             "the object has more properties than the maximum 2; reading MergedCounted");
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 3), R"({"b":2,"z":null})"), R"({"a":1,"b":2})");
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"c":1})"), R"({"a":0,"b":1})");
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":2})"), R"({"a":0,"b":2})");
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":null})"),
             "the object has fewer properties than the minimum 2; reading MergedMinimum");
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"a":null,"b":null,"c":1})"),
             "the object has fewer properties than the minimum 2; reading MergedMinimum");

    // a closed subschema sees the keys of the object after the merge
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"x":2})"), R"({"x":2})");
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"z":1})"), R"({"x":1,"z":1})");
    rejected(expect_merge_rule(MergedClosed{1, {}}, R"({"y":1})"),
             "the value satisfies none of the subschemas of oneOf; reading MergedClosed");
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, 1}, R"({"z":null})"), R"({"x":1})");
    rejected(expect_merge_rule(MergedClosed{1, 1}, R"({"x":null})"),
             "the value satisfies none of the subschemas of oneOf; reading MergedClosed");
}

TEST(Constraints, MergedObjectAtField)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    const auto merge = [](const MergedPath& t, std::string_view patch) {
        return expect_merge_rule<MergedPath, true>(t, patch);
    };
    const auto path = MergedPath{{1, 3}, {}};
    EXPECT_EQ(merge(path, R"({"start":{"x":2}})"), R"({"start":{"x":2,"y":3}})");
    rejected(merge(path, R"({"start":{"y":null}})"),
             R"(the object does not satisfy properties at /start; reading the key "start" of MergedPath)");
    rejected(merge(MergedPath{{1, {}}, {}}, R"({"start":{"x":2}})"),
             R"(the object does not satisfy properties at /start; reading the key "start" of MergedPath)");
    rejected(merge(path, R"({"end":{"x":2}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    rejected(merge(path, R"({"end":{"y":4}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    rejected(merge(path, R"({"end":{"x":2,"y":null}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    EXPECT_EQ(merge(path, R"({"end":{"x":2,"y":4}})"), R"({"start":{"x":1,"y":3},"end":{"x":2,"y":4}})");
    const auto ended = MergedPath{{1, 3}, MergedPoint{2, 4}};
    EXPECT_EQ(merge(ended, R"({"end":{"x":5}})"), R"({"start":{"x":1,"y":3},"end":{"x":5,"y":4}})");
    rejected(merge(ended, R"({"end":{"y":null}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
}

TEST(Constraints, MergedAbsent)
{
    // a value that is absent, as an empty optional or a new key of a map, reads the patch without the null members
    // of its objects
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    const auto merge = [](std::string_view patch) {
        return expect_merge_rule<MergedAbsent, true>(MergedAbsent(), patch);
    };
    rejected(merge(R"({"req":{"n":1}})"),
             R"(a required key is missing at /req; reading the key "id" of MergedRequired)");
    rejected(merge(R"({"req":{"id":null,"n":1}})"),
             R"(a required key is missing at /req; reading the key "id" of MergedRequired)");
    EXPECT_EQ(merge(R"({"req":{"id":1,"n":null}})"), R"({"req":{"id":1},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"init":{"x":null,"y":4}})"), R"({"init":{"x":5,"y":4},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"point":{"x":null,"y":4}})"), R"({"point":{"x":0,"y":4},"reqs":{},"mins":{}})");
    rejected(merge(R"({"reqs":{"k":{"n":1}}})"),
             R"(a required key is missing at /reqs/k; reading the key "id" of MergedRequired)");
    rejected(merge(R"({"reqs":{"k":{"id":null}}})"),
             R"(a required key is missing at /reqs/k; reading the key "id" of MergedRequired)");
    EXPECT_EQ(merge(R"({"reqs":{"k":{"id":1,"n":null}}})"), R"({"reqs":{"k":{"id":1}},"mins":{}})");
    rejected(merge(R"({"mins":{"k":{"b":1}}})"),
             "the object has fewer properties than the minimum 2 at /mins/k; reading MergedMinimum");
    rejected(merge(R"({"mins":{"k":{"b":1,"c":null}}})"),
             "the object has fewer properties than the minimum 2 at /mins/k; reading MergedMinimum");
    EXPECT_EQ(merge(R"({"mins":{"k":{"a":1,"b":null,"c":2}}})"), R"({"reqs":{},"mins":{"k":{"a":1}}})");
    // a null in an array stays, and so does a null member of an object in an array
    EXPECT_EQ(merge(R"({"arrays":{"v":[1,null],"w":null}})"),
              R"({"arrays":{"items":[],"v":[1,null]},"reqs":{},"mins":{}})");
    EXPECT_TRUE(merge(R"({"arrays":{"items":[{"x":null}],"w":null}})").starts_with("bad cast"));
    // a value that is present takes the null members of the patch
    auto present = MergedAbsent();
    present.point = MergedPoint{1, 2};
    EXPECT_EQ((expect_merge_rule<MergedAbsent, true>(present, R"({"point":{"x":null,"y":4}})")),
              R"({"point":{"x":0,"y":4},"reqs":{},"mins":{}})");
}

TEST(Constraints, MergedAbsentField)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    // a field that the write omits is absent
    const auto outer = OmittedOuter{.name = "a", .in = {}, .min = {}};
    const auto merge = [](const OmittedOuter& t, std::string_view patch) {
        return expect_merge_rule<OmittedOuter, true>(t, patch);
    };
    rejected(
        merge(outer, R"({"in":{"y":1}})"),
        R"(the object does not have all the keys that dependentRequired requires at /in; reading the key "z" of OmittedInner, which yyjson::dependent_required requires)");
    EXPECT_EQ(merge(outer, R"({"in":{"y":1,"z":2}})"), R"({"name":"a","in":{"y":1,"z":2}})");
    rejected(merge(outer, R"({"min":{"b":1}})"),
             "the object has fewer properties than the minimum 2 at /min; reading MergedMinimum");
    EXPECT_EQ(merge(outer, R"({"min":{"a":1,"b":null,"c":2}})"), R"({"name":"a","in":{"z":0},"min":{"a":1}})");
    auto written = outer;
    written.in.z = 1;
    EXPECT_EQ(merge(written, R"({"in":{"y":1}})"), R"({"name":"a","in":{"y":1,"z":1}})");

    // the counts in a condition count a field that the patch does not name
    rejected(expect_merge_rule(KeptGate{{}, {1}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
             R"(the value satisfies if and not then at /s; reading the key "s" of KeptGate)");
    rejected(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1})"),
             R"(the value satisfies if and not then at /m; reading the key "m" of KeptGate)");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1,"m":{"l":2}})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");

    // a new key of a map of the keys that no field names is absent
    const auto rest = [](std::string_view patch) {
        return expect_merge_rule<MergedRest, true>(MergedRest{.a = 1, .ks = {}, .rest = {{"r", {1, {}}}}}, patch);
    };
    rejected(rest(R"({"x":{"n":1}})"), R"(a required key is missing at /x; reading the key "id" of MergedRequired)");
    rejected(rest(R"({"k":{"n":1}})"), R"(a required key is missing at /k; reading the key "id" of MergedRequired)");
    EXPECT_EQ(rest(R"({"x":{"id":2,"n":null},"k":{"id":3,"n":null}})"),
              R"({"a":1,"k":{"id":3},"r":{"id":1},"x":{"id":2}})");
    EXPECT_EQ(rest(R"({"r":{"n":2}})"), R"({"a":1,"r":{"id":1,"n":2}})");
    rejected(rest(R"({"r":{"id":null}})"),
             R"(a required key is missing at /r; reading the key "id" of MergedRequired)");

    // a field that the write wrote before a merge without a document changed it to its default is not absent in
    // the merge through a document that follows, which the null of the adjacent tag starts
    const auto fallback = [](const WrittenOuter& t, std::string_view patch) {
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(t, patch, &merged_directly));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(merged_directly) << patch;
#endif
        return expect_merge_rule<WrittenOuter, true>(t, patch);
    };
    EXPECT_EQ(fallback(WrittenOuter{{1, 0}, {}, {}}, R"({"in":{"y":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    EXPECT_EQ(fallback(WrittenOuter{{}, {{"a", 1}}, {}}, R"({"m":{"a":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    rejected(fallback(WrittenOuter(), R"({"in":{"y":1},"body":null,"t":null})"),
             R"(a required key is missing at /in; reading the key "z" of WrittenInner)");
}

// NOLINTEND
