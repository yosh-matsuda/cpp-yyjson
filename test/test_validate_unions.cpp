#include "test_validate_common.hpp"

// NOLINTBEGIN

// a union: the settings of the alternative that it holds, the count of a one_of, and the read of its JSON when the
// alternative fails, which passes if another alternative reads it
struct VdUnionA
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionA> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
struct VdUnionB
{
    int x = 0;
};
struct VdUnionC
{
    int x = 0;
    std::optional<int> y;
};
struct VdUnionD
{
    int y = 0;
};
struct VdUnionE
{
    std::string msg;
};
using VdLowOrInt = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, int>;
using VdLowOrHigh =
    yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdLowOrShort =
    yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<std::string, yyjson::max_length(3)>>;
using VdItemsOrText =
    yyjson::any_of_variant<yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(0))>, std::string>;
using VdSignedOrUnsigned = yyjson::one_of_variant<std::int64_t, std::uint64_t>;
// some alternatives other than the last one may read its values and some may not
using VdPartlyOverlapping =
    yyjson::one_of_variant<yyjson::alt<double, yyjson::maximum(0.5)>, std::string,
                           yyjson::alt<std::int64_t, yyjson::minimum(-100), yyjson::maximum(10)>, std::uint64_t>;
using VdAB = std::variant<VdUnionA, VdUnionB>;
using VdAnyAB = yyjson::any_of_variant<VdUnionA, VdUnionB>;
using VdOneCD = yyjson::one_of_variant<VdUnionC, VdUnionD>;
using VdOneAD = yyjson::one_of_variant<VdUnionA, VdUnionD>;
struct VdUnionFields
{
    VdOneCD u = VdUnionD{};
    VdLowOrShort n = 0;
};
struct VdCounted
{
    int a = 0;
    int b = 0;
};
using VdCountedOrMap =
    yyjson::any_of_variant<yyjson::alt<VdCounted, yyjson::max_properties(1)>, std::map<std::string, int>>;
struct VdDefaulted
{
    int a = 0;
    int b = 5;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaulted> = yyjson::annotate(yyjson::field("b", yyjson::default_));
using VdDefaultedOrInt = yyjson::any_of_variant<yyjson::alt<VdDefaulted, yyjson::max_properties(1)>, int>;
struct VdDisjointField
{
    int a = 0;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdDisjointField> = yyjson::annotate(yyjson::field("a", yyjson::minimum(0)));

namespace
{
    template <typename V>
    std::string vd_name()
    {
        return std::string(yyjson::detail::variant_name<V>());
    }
}  // namespace

TEST(Validate, Unions)
{
    static_assert(yyjson::detail::needs_validation<VdLowOrInt>);
    static_assert(yyjson::detail::needs_validation<VdOneCD>);
    static_assert(!yyjson::detail::needs_validation<yyjson::any_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<yyjson::one_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<std::variant<VdUnionB, int>>);

    // the settings of the alternative, where another alternative may read the JSON
    expect_valid(VdLowOrInt(std::in_place_index<0>, 5));
    expect_valid(VdLowOrInt(std::in_place_index<0>, -5));
    expect_valid(VdLowOrHigh(std::in_place_index<0>, 3));
    expect_valid(VdLowOrHigh(std::in_place_index<1>, -20));
    expect_invalid(
        VdLowOrHigh(std::in_place_index<0>, -5),
        "no alternative accepts the value; reading yyjson::one_of_variant<int, int>: alternative 0 (int): the number "
        "is less than the minimum 0; alternative 1 (int): the number is greater than the maximum -10",
        "");
    expect_valid(VdLowOrShort(std::string("abc")));
    expect_invalid(VdLowOrShort(-1),
                   std::format("no alternative accepts the value; reading {}: alternative 0 (int): the number is less "
                               "than the minimum 0; alternative 1 ({}): expected a string, got a number",
                               yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                               yyjson::detail::type_name<std::string>()),
                   "");
    expect_invalid(
        VdLowOrShort(std::string("abcd")),
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got a "
                    "string; alternative 1 ({}): the string is longer than the maximum length 3",
                    yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                    yyjson::detail::type_name<std::string>()),
        "");
    expect_invalid(
        VdItemsOrText(std::vector<int>{1, -2}),
        std::format("no alternative accepts the value; reading {}: alternative 0 ({}): the array has an item that does "
                    "not satisfy items; alternative 1 ({}): expected a string, got an array",
                    yyjson::detail::variant_name<yyjson::any_of_variant<std::vector<int>, std::string>>(),
                    yyjson::detail::type_name<std::vector<int>>(), yyjson::detail::type_name<std::string>()),
        "");
    // a keyword that counts the JSON of the alternative
    expect_valid(VdCountedOrMap(VdCounted{1, 2}));
    expect_valid(VdDefaultedOrInt(VdDefaulted{1, 5}));
    const auto defaulted = "no alternative accepts the value; reading " + vd_name<VdDefaultedOrInt>() +
                           ": alternative 0 (VdDefaulted): the object has more properties than the maximum 1; "
                           "alternative 1 (int): expected an integer, got an object";
    EXPECT_EQ(std::string(validation_error([] {
                              yyjson::validate(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults);
                          }).what()),
              defaulted);
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults,
                                                  yyjson::with_validation);
                          }).what()),
              defaulted);

    // a value whose alternative fails passes if another reads its JSON
    expect_valid(VdAB(VdUnionA{-1}));
    expect_valid(VdAnyAB(VdUnionA{-1}));
    // another alternative that reads the JSON only without its required keys
    expect_invalid(
        std::variant<VdUnionA, VdUnionE>(VdUnionA{-1}),
        R"(no alternative accepts the value; reading std::variant<VdUnionA, VdUnionE>: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 (VdUnionE): a required key is missing (reading the key "msg" of VdUnionE))",
        "", read_leg::skipped);
    expect_invalid(
        VdOneAD(VdUnionA{-1}),
        R"(no alternative accepts the value; reading yyjson::one_of_variant<VdUnionA, VdUnionD>: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 (VdUnionD): a required key is missing (reading the key "y" of VdUnionD))",
        "");
    // a walk that fails for the alternative that read gives too, which read does not check
    expect_invalid(
        std::variant<VdReadBack, int>(VdReadBack{3, {}, std::nullopt}),
        R"(the value is not valid at /z; validating VdReadBack: the field "z" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/z", read_leg::skipped);

    // the count of a one_of, which a disjoint alternative skips
    expect_invalid(VdOneCD(VdUnionC{1, 2}),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdUnionC, VdUnionD>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_valid(VdOneCD(VdUnionC{1, {}}));
    expect_valid(VdOneCD(VdUnionD{1}));
    expect_valid(yyjson::one_of_variant<int, std::string>(1));
    expect_valid(VdSignedOrUnsigned(std::int64_t{-5}));
    expect_invalid(
        VdSignedOrUnsigned(std::int64_t{5}),
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "");
    expect_invalid(
        std::vector<VdSignedOrUnsigned>{std::int64_t{-5}, std::uint64_t{5}},
        std::format("the value matches more than one alternative at /1; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "/1");
    expect_valid(VdPartlyOverlapping(-300.0));
    expect_valid(VdPartlyOverlapping(std::uint64_t{20}));
    expect_invalid(
        VdPartlyOverlapping(std::int64_t{0}),
        std::format(
            "the value matches more than one alternative; reading {}: the alternatives 0 and 2 match",
            yyjson::detail::variant_name<yyjson::one_of_variant<double, std::string, std::int64_t, std::uint64_t>>()),
        "");
    expect_invalid(
        VdPartlyOverlapping(std::uint64_t{5}),
        std::format(
            "the value matches more than one alternative; reading {}: the alternatives 2 and 3 match",
            yyjson::detail::variant_name<yyjson::one_of_variant<double, std::string, std::int64_t, std::uint64_t>>()),
        "");

    // nested unions, and a union in an array, a map and a struct
    expect_invalid(yyjson::any_of_variant<VdOneCD, int>(VdOneCD(VdUnionC{1, 2})),
                   "no alternative accepts the value; reading yyjson::any_of_variant<yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>, int>: alternative 0 (yyjson::one_of_variant<VdUnionC, VdUnionD>): the value matches "
                   "more than one alternative (reading yyjson::one_of_variant<VdUnionC, VdUnionD>: the alternatives 0 "
                   "and 1 match); alternative 1 (int): expected an integer, got an object",
                   "");
    expect_valid(yyjson::any_of_variant<VdOneAD, VdUnionB>(VdOneAD(VdUnionA{-1})));
    expect_valid(yyjson::one_of_variant<VdAB, VdUnionD>(VdAB(VdUnionA{-1})));
    expect_invalid(std::vector<VdOneCD>{VdUnionD{1}, VdUnionD{2}, VdUnionD{3}, VdUnionC{1, 2}},
                   "the value matches more than one alternative at /3; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/3");
    expect_invalid(std::map<std::string, VdOneCD>{{"k", VdUnionC{1, 2}}},
                   "the value matches more than one alternative at /k; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/k");
    expect_invalid(VdUnionFields{VdUnionC{1, 2}, 1},
                   "the value matches more than one alternative at /u; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/u");
    expect_valid(VdUnionFields{VdUnionC{1, {}}, std::string("ab")});
    // the settings of an alternative that is a variant check the alternative that the inner variant holds
    using VdNested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<int, std::string>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    using VdInner = std::variant<std::variant<int, std::string>, bool>;
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(7))));
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(std::string("ab")))));
    expect_valid(VdNested(VdInner(true)));
    expect_invalid(
        VdNested(VdInner(std::variant<int, std::string>(1))),
        std::format(
            "no alternative accepts the value; reading {}: alternative 0 ({}): the number is less than the minimum 5",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");
    expect_invalid(
        VdNested(VdInner(std::variant<int, std::string>(std::string("a")))),
        std::format(
            "no alternative accepts the value; reading {}: alternative 0 ({}): the string is shorter than the minimum "
            "length 2",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");

    // a passing check of a disjoint union, or of an alternative that satisfies its settings, allocates nothing
    EXPECT_EQ(validate_allocations(std::vector<VdLowOrInt>(100, VdLowOrInt(std::in_place_index<0>, 1))), 0U);
    EXPECT_EQ(validate_allocations(yyjson::one_of_variant<int, std::string>(1)), 0U);
    EXPECT_EQ(validate_allocations(VdDisjointField{1, std::string("x")}), 0U);
}

// a union in a tagged variant, and a variant that is tagged
struct VdClosedA
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedA> = yyjson::annotate(yyjson::additional_properties(false));
struct VdTypedA
{
    std::string type;
    int a = 0;
};
using VdOneClosed = yyjson::one_of_variant<VdClosedA, VdTypedA>;
struct VdOtherZ
{
    int z = 0;
};
using VdTaggedOne = std::variant<VdOneClosed, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedOne> = yyjson::annotate(yyjson::internal_tag("type", {"one", "z"}));
using VdTaggedAD = std::variant<VdUnionA, VdUnionD>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedAD> = yyjson::annotate(yyjson::internal_tag("kind", {"a", "d"}));
// a union that a wrapper holds in a tagged variant
struct VdOneWrapped
{
    VdOneClosed u;
};
template <>
inline constexpr auto yyjson::annotations<VdOneWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedWrapped = std::variant<VdOneWrapped, std::unique_ptr<VdOneClosed>, std::optional<VdOneClosed>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedWrapped> =
    yyjson::annotate(yyjson::internal_tag("type", {"w", "p", "o", "z"}));
// a variant that is not tagged, whose map alternative reads the written object only without the tag
using VdUnionOrMap = std::variant<VdUnionA, std::map<std::string, int>>;
struct VdMapWrapped
{
    VdUnionOrMap u;
};
template <>
inline constexpr auto yyjson::annotations<VdMapWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedMap =
    std::variant<VdUnionOrMap, VdMapWrapped, std::unique_ptr<VdUnionOrMap>, std::optional<VdUnionOrMap>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedMap> =
    yyjson::annotate(yyjson::internal_tag("type", {"u", "w", "p", "o", "z"}));
struct VdAdjacentAB
{
    std::string kind = "a";
    std::variant<VdUnionA, VdUnionB> data;
};
template <>
inline constexpr auto yyjson::annotations<VdAdjacentAB> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"a", "b"})));

TEST(Validate, TaggedUnions)
{
    // the key of the tag lets the alternatives of the one_of read the same object
    expect_valid(VdOneClosed(VdClosedA{0}));
    expect_invalid(VdTaggedOne(VdOneClosed(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedOne(VdOneClosed(VdTypedA{"one", 0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(std::vector<VdTaggedOne>{VdOtherZ{}, VdOneClosed(VdClosedA{0})},
                   "the value matches more than one alternative at /1; reading yyjson::one_of_variant<VdClosedA, "
                   "VdTypedA>: the alternatives 0 and 1 match",
                   "/1");
    expect_valid(VdTaggedOne(VdOtherZ{1}));

    // the tag reaches a union through a transparent struct, a pointer and an optional
    expect_invalid(VdTaggedWrapped(VdOneWrapped{VdClosedA{0}}),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedWrapped(std::make_unique<VdOneClosed>(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedWrapped(std::optional<VdOneClosed>(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_valid(VdTaggedWrapped(VdOtherZ{1}));
    expect_invalid(
        VdTaggedMap(VdUnionOrMap(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(VdMapWrapped{VdUnionA{-1}}),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(std::make_unique<VdUnionOrMap>(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(std::optional<VdUnionOrMap>(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_valid(VdTaggedMap(VdUnionOrMap(VdUnionA{1})));
    expect_valid(VdTaggedMap(VdMapWrapped{VdUnionA{1}}));

    // a tagged variant reads only the alternative of its tag
    expect_invalid(VdTaggedAD(VdUnionA{-1}),
                   R"(the number is less than the minimum 0 at /x; reading the key "x" of VdUnionA)", "/x");
    expect_invalid(VdAdjacentAB{"a", VdUnionA{-1}},
                   R"(the number is less than the minimum 0 at /data/x; reading the key "x" of VdUnionA)", "/data/x");
    expect_valid(VdAdjacentAB{"b", VdUnionB{-1}});
}

// a union whose alternative write gives a repeated key or null for a NaN
using VdRepeatedOrMap = std::variant<std::multimap<std::string, int>, std::map<std::string, int>>;
using VdLowOrNull = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, std::nullptr_t>;
using VdLowDouble = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, int>;
// a tree whose depth the check limits through a union
struct VdUnionTree
{
    int v = 0;
    std::vector<std::variant<VdUnionTree, int>> kids;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionTree> = yyjson::annotate(yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, UnionWrittenForms)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    // the other alternative does not read the key twice either, as a strict read
    const auto repeated = VdRepeatedOrMap(std::multimap<std::string, int>{{"a", 1}, {"a", 2}});
    const auto twice = [](int i, std::string_view type) {
        return std::format("alternative {} ({}) at /a: the object has a duplicate key (reading {})", i, type, type);
    };
    const auto message = std::format("no alternative accepts the value; reading {}: {}; {}", vd_name<VdRepeatedOrMap>(),
                                     twice(0, yyjson::detail::type_name<std::multimap<std::string, int>>()),
                                     twice(1, yyjson::detail::type_name<std::map<std::string, int>>()));
    expect_invalid(repeated, message, "", read_leg::skipped);
    EXPECT_NO_THROW((void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated)));
    EXPECT_EQ(what_without_position(validation_error(
                  [&] { (void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated), yyjson::strict); })),
              message);

    // a NaN that write gives as null, which another alternative reads
    const auto nan = std::nan("");
    EXPECT_EQ(std::string(yyjson::write(VdLowOrNull(nan), WriteFlag::InfAndNanAsNull, with_validation)), "null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::InfAndNanAsNull, with_validation);
                          }).what()),
              "no alternative accepts the value; reading " + vd_name<VdLowDouble>() +
                  ": alternative 0 (double): expected a number, got null; alternative 1 (int): expected an integer, "
                  "got null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              "no alternative accepts the value; reading " + vd_name<VdLowDouble>() +
                  ": alternative 0 (double): the number is less than the minimum 0; alternative 1 (int): expected an "
                  "integer, got a number");

    // the depth of a walk and of the read of a failing union
    expect_invalid(
        VdUnionTree{0, {VdUnionTree{-1, {}}}},
        R"(no alternative accepts the value at /kids/0; reading std::variant<VdUnionTree, int>: alternative 0 (VdUnionTree) at /v: the number is less than the minimum 0 (reading the key "v" of VdUnionTree); alternative 1 (int): expected an integer, got an object)",
        "/kids/0");
    auto tree = VdUnionTree();
    for (auto i = 0; i < 300; ++i) tree = VdUnionTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    auto failing = VdUnionTree{-1, {}};
    for (auto i = 0; i < 100; ++i) failing = VdUnionTree{0, {std::move(failing)}};
    EXPECT_THROW(yyjson::validate(failing), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
}

// NOLINTEND
