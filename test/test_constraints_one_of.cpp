#include "test_constraints_common.hpp"

// NOLINTBEGIN

// The alternatives of `yyjson::one_of_variant`
struct OneOfX
{
    int x = 0;
    bool operator==(const OneOfX&) const = default;
};
struct OneOfXZ
{
    int x = 0;
    int z = 0;
    bool operator==(const OneOfXZ&) const = default;
};
struct OneOfMaybeX
{
    std::optional<int> x;
    bool operator==(const OneOfMaybeX&) const = default;
};
struct OneOfMaybeXY
{
    std::optional<int> x;
    int y = 0;
    bool operator==(const OneOfMaybeXY&) const = default;
};
struct OneOfY
{
    int y = 0;
    bool operator==(const OneOfY&) const = default;
};
struct OneOfClosedX
{
    int x = 0;
    bool operator==(const OneOfClosedX&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OneOfClosedX> = yyjson::annotate(yyjson::additional_properties(false));
// a caster of the user that takes only a value of the mutable document, which the read gives a copy of
struct OneOfOtherCode
{
    int n = 0;
};
template <>
struct yyjson::caster<OneOfOtherCode>
{
    static OneOfOtherCode from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        return {1};
    }
};
// a range and a map of this file that such a caster reads in place of their elements and values
struct OtherCodeRange : std::vector<int>
{
};
template <>
struct yyjson::caster<OtherCodeRange>
{
    static OtherCodeRange from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_array()) throw yyjson::bad_cast("not an array");
        auto result = OtherCodeRange();
        result.push_back(7);
        return result;
    }
};
struct OtherCodeMap : std::map<std::string, int>
{
};
template <>
struct yyjson::caster<OtherCodeMap>
{
    static OtherCodeMap from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_object()) throw yyjson::bad_cast("not an object");
        auto result = OtherCodeMap();
        result.emplace("k", 7);
        return result;
    }
};
struct OneOfZ
{
    int z = 0;
    bool operator==(const OneOfZ&) const = default;
};
struct OneOfTextX
{
    std::string x;
    bool operator==(const OneOfTextX&) const = default;
};
struct OneOfNullX
{
    std::nullptr_t x;
    bool operator==(const OneOfNullX&) const = default;
};
struct OneOfXMaybeY
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const OneOfXMaybeY&) const = default;
};
struct OneOfXMaybeZ
{
    int x = 0;
    std::optional<int> z;
    bool operator==(const OneOfXMaybeZ&) const = default;
};
struct OneOfXRest
{
    int x = 0;
    std::map<std::string, int> rest;
    bool operator==(const OneOfXRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OneOfXRest> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct OneOfXAny
{
    int x = 0;
    std::map<std::string, yyjson::value> rest;
};
template <>
inline constexpr auto yyjson::annotations<OneOfXAny> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct OneOfSignedX
{
    std::int64_t x = 0;
    bool operator==(const OneOfSignedX&) const = default;
};
struct OneOfUnsignedX
{
    std::uint64_t x = 0;
    bool operator==(const OneOfUnsignedX&) const = default;
};
template <typename T>
struct OneOfWrapped
{
    T inner;
    bool operator==(const OneOfWrapped&) const = default;
};
template <typename T>
inline constexpr auto yyjson::annotations<OneOfWrapped<T>> = yyjson::annotate(yyjson::transparent);
struct OneOfMany16
{
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
};
struct OneOfMany17
{
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q;
};
// a field of OneOfCode reads a string in place, but `cast<OneOfCode>` reads no value
struct OneOfCode
{
    int n = 0;
};
template <>
struct yyjson::caster<OneOfCode>
{
    template <typename Json>
    static void from_json(OneOfCode& c, const Json& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        c.n = 1;
    }
};
struct OneOfCodeX
{
    OneOfCode x;
};
struct OneOfOtherCodeX
{
    OneOfOtherCode x;
};
// a std::optional of a caster of the user, which reads a string
struct OneOfNumber
{
    int n = 0;
};
template <>
struct yyjson::caster<std::optional<OneOfNumber>>
{
    template <typename Json>
    static std::optional<OneOfNumber> from_json(const Json& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        return OneOfNumber{1};
    }
};
struct OneOfMaybeNumberX
{
    std::optional<OneOfNumber> x;
};
// a closed struct takes the key of the tag of an enclosing variant
struct OneOfXKind
{
    int x = 0;
    std::string kind;
};
struct OneOfClosedXMaybeKind
{
    int x = 0;
    std::optional<int> kind;
};
template <>
inline constexpr auto yyjson::annotations<OneOfClosedXMaybeKind> =
    yyjson::annotate(yyjson::additional_properties(false));
using OneOfTaggedXKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfXKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedXKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
using OneOfTaggedMaybeKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfClosedXMaybeKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedMaybeKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
struct OneOfXMaybeTextKind
{
    int x = 0;
    std::optional<std::string> kind;
};
using OneOfTaggedTextKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfXMaybeTextKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedTextKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
// a field of an adjacent tag reads the value of the alternative, not the object of its own internal tag
using OneOfPayload = std::variant<long, std::string>;
template <>
inline constexpr auto yyjson::annotations<OneOfPayload> = yyjson::annotate(yyjson::internal_tag("inner", {"i", "s"}));
struct OneOfAdjacentX
{
    OneOfPayload x;
};
template <>
inline constexpr auto yyjson::annotations<OneOfAdjacentX> =
    yyjson::annotate(yyjson::field("x", yyjson::adjacent_tag("kind", {"i", "s"})));
// a transparent struct between two others with a setting of the field of the inner one
struct OneOfMidX
{
    OneOfX b;
};
template <>
inline constexpr auto yyjson::annotations<OneOfMidX> =
    yyjson::annotate(yyjson::transparent, yyjson::field("b", yyjson::property("x", yyjson::minimum(0))));
using OneOfInts = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
inline bool one_of_has_value(const yyjson::nullable<int>& v) { return v.has_value(); }
inline constexpr auto one_of_present = yyjson::satisfies<one_of_has_value>;
inline constexpr auto one_of_object = yyjson::matches<yyjson::object>;
struct OneOfOrder
{
    bool operator()(int a, int b) const { return a < b; }
};
struct OneOfFields
{
    OneOfInts n;
    yyjson::one_of_variant<OneOfX, std::string> p;
    bool operator==(const OneOfFields&) const = default;
};

namespace
{
    // the message of a value that the alternatives i and j of V both read
    template <typename V>
    std::string read_by_two(std::size_t i, std::size_t j)
    {
        return std::format("the value matches the alternatives {} and {} of {}", i, j,
                           yyjson::detail::variant_name<V>());
    }

    // reading `json` into V under `policies` throws `bad_cast` with `message`, with and without a document
    template <typename V, typename... Policies>
    void expect_rejected_under(std::string_view json, const expected_text& message, Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) { expect_text(message_of(e), message, json); };
        check(bad_cast_of([&] { (void)yyjson::read<V>(json, policies...); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<V>(policies...); }));
    }

    // whether an alternative of the `yyjson::one_of_variant` V reads only values that another one reads
    template <typename V>
    constexpr bool one_of_contained =
        yyjson::detail::one_of_relations<V>::contained.contained != std::variant_size_v<V>;
    // whether the alternatives I and J of V may both read a value
    template <typename V, std::size_t I, std::size_t J>
    constexpr bool one_of_overlapping = yyjson::detail::one_of_relations<V>::template overlapping<J>[I];
}  // namespace

TEST(Constraints, OneOfReading)
{
    // the struct that misses a key without a default under yyjson::reject_missing_keys does not match
    using XOrZ = yyjson::one_of_variant<OneOfX, OneOfZ>;
    expect_accepted<XOrZ>(R"({"x":1})", XOrZ(OneOfX{1}));
    // both match an object of both keys, since each one takes other keys, unless yyjson::reject_unknown_keys of
    // the call closes them
    expect_rejected<XOrZ>(R"({"x":1,"z":2,"w":0})",
                          "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                          "OneOfZ>: the alternatives 0 and 1 match",
                          "");
    expect_rejected_under<XOrZ>(R"({"x":1,"z":2})",
                                "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                                "OneOfZ>: the alternatives 0 and 1 match");
    expect_rejected_under<XOrZ>(
        R"({"x":1,"z":2})",
        "no alternative accepts the value; reading yyjson::one_of_variant<OneOfX, OneOfZ>: alternative 0 (OneOfX) at "
        "/z: the object has a key that is not allowed (reading OneOfX); alternative 1 (OneOfZ) at /x: the object has a "
        "key that is not allowed (reading OneOfZ)",
        yyjson::reject_unknown_keys);
    // an alternative may match with none of its keys
    using MaybeXOrY = yyjson::one_of_variant<OneOfMaybeX, OneOfY>;
    expect_rejected<MaybeXOrY>(R"({"y":1})",
                               "the value matches more than one alternative; reading "
                               "yyjson::one_of_variant<OneOfMaybeX, OneOfY>: the alternatives 0 and 1 match",
                               "");
    expect_accepted<MaybeXOrY>(R"({"x":1})", MaybeXOrY(OneOfMaybeX{1}));
    expect_accepted<MaybeXOrY>("{}", MaybeXOrY(OneOfMaybeX{}));
    // a closed struct does not match an object of other keys
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_accepted<ClosedOrXZ>(R"({"x":1,"z":2})", ClosedOrXZ(OneOfXZ{1, 2}));
    expect_accepted<ClosedOrXZ>(R"({"x":1})", ClosedOrXZ(OneOfClosedX{1}));
    expect_rejected<ClosedOrXZ>(
        R"({"z":1})",
        R"(no alternative accepts the value; reading yyjson::one_of_variant<OneOfClosedX, OneOfXZ>: alternative 0 (OneOfClosedX) at /z: the object has a key that is not allowed (reading OneOfClosedX, which yyjson::additional_properties(false) closes); alternative 1 (OneOfXZ): a required key is missing (reading the key "x" of OneOfXZ))",
        "");

    // yyjson::reject_unknown_keys of the call closes the struct alternatives
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_alternative<XOrText>(R"({"x":1,"k":2})", 0);
    EXPECT_THROW((void)yyjson::read<XOrText>(R"({"x":1,"k":2})", yyjson::reject_unknown_keys), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"k":2})").cast<XOrText>(yyjson::reject_unknown_keys), yyjson::bad_cast);
    expect_alternative<XOrText>(R"({"x":1})", 0, yyjson::reject_unknown_keys);
    expect_alternative<XOrText>(R"("a")", 1, yyjson::reject_unknown_keys);
    // so an object of a key that only one of them declares matches that one
    using XOrMaybeXY = yyjson::one_of_variant<OneOfX, OneOfMaybeXY>;
    expect_rejected_under<XOrMaybeXY>(R"({"x":1,"y":2})",
                                      "the value matches more than one alternative; reading "
                                      "yyjson::one_of_variant<OneOfX, OneOfMaybeXY>: the alternatives 0 and 1 match");
    expect_accepted<XOrMaybeXY>(R"({"x":1})", XOrMaybeXY(OneOfX{1}));
    expect_alternative<XOrMaybeXY>(R"({"x":1,"y":2})", 1, yyjson::reject_unknown_keys);
    EXPECT_EQ(std::get<1>(yyjson::read<XOrMaybeXY>(R"({"x":1,"y":2})", yyjson::reject_unknown_keys)),
              (OneOfMaybeXY{1, 2}));
    // a set does not match a repeated element, which `uniqueItems` rejects, under any policies
    using SetOrList = yyjson::one_of_variant<std::set<int>, std::vector<int>>;
    expect_alternative<SetOrList>("[1,1]", 1);
    expect_alternative<SetOrList>("[1,1]", 1, yyjson::reject_duplicates);
    expect_rejected_under<SetOrList>(
        "[1,2]", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                             yyjson::detail::variant_name<SetOrList>()));
    using CountedSet = yyjson::one_of_variant<yyjson::alt<std::set<int>, yyjson::min_items(1)>,
                                              yyjson::alt<std::vector<int>, yyjson::max_items(2)>>;
    expect_alternative<CountedSet>("[1,1]", 1, yyjson::strict);
    // a set of another order has no `uniqueItems`, and no schema rejects a repeated key
    using OrderedOrList = yyjson::one_of_variant<std::set<int, OneOfOrder>, std::vector<int>>;
    expect_rejected_under<OrderedOrList>(
        "[1,1]", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                             yyjson::detail::variant_name<OrderedOrList>()));
    EXPECT_THROW((void)yyjson::read<XOrText>(R"({"x":1,"x":2})", yyjson::reject_duplicates), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"x":2})").cast<XOrText>(yyjson::reject_duplicates), yyjson::bad_cast);

    // the settings of an alternative decide whether it matches
    using SignedX = yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>,
                                           yyjson::alt<OneOfX, yyjson::property("x", yyjson::maximum(-1))>>;
    expect_alternative<SignedX>(R"({"x":1})", 0);
    expect_alternative<SignedX>(R"({"x":-1})", 1);
    expect_accepted<OneOfInts>("-1", OneOfInts(std::in_place_index<1>, -1));
    // the reader without a document leaves a value to it when a later alternative may read it too
    expect_accepted<OneOfInts>("11", OneOfInts(std::in_place_index<0>, 11), false);
    expect_rejected<OneOfInts>("5",
                               "the value matches more than one alternative; reading yyjson::one_of_variant<int, int>: "
                               "the alternatives 0 and 1 match",
                               "");
    expect_rejected<OneOfInts>(
        "1.5",
        "no alternative accepts the value; reading yyjson::one_of_variant<int, int>: alternative 0 (int): expected an "
        "integer, got a number; alternative 1 (int): expected an integer, got a number",
        "");
    expect_rejected<OneOfFields>(R"({"n":5,"p":"a"})",
                                 "the value matches more than one alternative at /n; reading "
                                 "yyjson::one_of_variant<int, int>: the alternatives 0 and 1 match",
                                 "/n");

    // read_into and merge_into replace the value
    auto fields = OneOfFields{OneOfInts(std::in_place_index<0>, 20), OneOfX{1}};
    yyjson::merge_into(R"({"p":{"x":5}})", fields);
    EXPECT_EQ(fields.p, (yyjson::one_of_variant<OneOfX, std::string>(OneOfX{5})));
    yyjson::read_into(R"({"n":-2,"p":"s"})", fields);
    EXPECT_EQ(fields, (OneOfFields{OneOfInts(std::in_place_index<1>, -2), std::string("s")}));

    // written as the alternative held, in both writers
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"n":-2,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::object(fields).write()), R"({"n":-2,"p":"s"})");
    fields.p = OneOfX{3};
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"n":-2,"p":{"x":3}})");
    EXPECT_EQ(std::string(yyjson::value(OneOfInts(std::in_place_index<0>, 5)).write()), "5");
}

struct UnionReasonField
{
    std::variant<int, UnionPoint> v;
};
struct UnionReasonLine
{
    UnionPoint from;
    std::vector<UnionPoint> to;
};
struct UnionReasonNode
{
    std::variant<int, OneOfX> v;
    yyjson::one_of_variant<std::nullptr_t, yyjson::indirect<UnionReasonNode>> next;
};
struct UnionReasonCount
{
    int x = 0;
    int y = 0;
};
struct UnionReasonText
{
    std::string x;
    int y = 0;
};

namespace
{
    // reading `json` into T under `policies` throws `bad_cast` with exactly `message` about the value at `path`,
    // with and without a document, and from a mutable document
    template <typename T, typename... Policies>
    void expect_reasons(std::string_view json, const expected_text& message, std::string_view path,
                        Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(what_without_position(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, policies...); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(policies...); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(policies...); }));
    }
}  // namespace

TEST(Constraints, UnionFailureReasons)
{
    // a message names the union by the types of its alternatives, without their settings
    EXPECT_EQ((yyjson::detail::variant_name<std::variant<int, bool>>()), "std::variant<int, bool>");
    EXPECT_EQ((yyjson::detail::variant_name<yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>>()),
              "yyjson::any_of_variant<int, bool>");
    EXPECT_EQ((yyjson::detail::variant_name<const yyjson::one_of_variant<int, bool>>()),
              "yyjson::one_of_variant<int, bool>");
    expect_reasons<yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>>(
        "0",
        "no alternative accepts the value; reading yyjson::one_of_variant<int, bool>: alternative 0 (int): the number "
        "is less than the minimum 1; alternative 1 (bool): expected a boolean, got a number",
        "");
    // a union inside a union has the same name in the label of its alternative and in its reason
    expect_reasons<yyjson::any_of_variant<yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>, bool>>(
        "0",
        "no alternative accepts the value; reading yyjson::any_of_variant<yyjson::any_of_variant<int, bool>, bool>: "
        "alternative 0 (yyjson::any_of_variant<int, bool>): no alternative accepts the value (reading "
        "yyjson::any_of_variant<int, bool>); alternative 1 (bool): expected a boolean, got a number",
        "");
    expect_reasons<std::variant<std::variant<int, bool>, bool>>(
        "null",
        "no alternative accepts the value; reading std::variant<std::variant<int, bool>, bool>: alternative 0 "
        "(std::variant<int, bool>): expected a boolean or an integer, got null; alternative 1 (bool): expected a "
        "boolean, got null",
        "");

    // each alternative that does not read the kind of the value
    using Kinds = std::variant<int, std::string, UnionPoint>;
    expect_reasons<Kinds>("true",
                          std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an "
                                      "integer, got a boolean; alternative 1 ({}): expected a string, got a boolean; "
                                      "alternative 2 (UnionPoint): expected an object, got a boolean",
                                      yyjson::detail::variant_name<std::variant<int, std::string, UnionPoint>>(),
                                      yyjson::detail::type_name<std::string>()),
                          "");
    expect_reasons<std::vector<Kinds>>(
        R"([1,"a",null])",
        std::format(
            "no alternative accepts the value at /2; reading {}: alternative 0 (int): expected an integer, got null; "
            "alternative 1 ({}): expected a string, got null; alternative 2 (UnionPoint): expected an object, got null",
            yyjson::detail::variant_name<std::variant<int, std::string, UnionPoint>>(),
            yyjson::detail::type_name<std::string>()),
        "/2");
    // the reason of a struct gives the path of the value within it, and the path of the variant ends the message once
    using IntOrPoint = std::variant<int, UnionPoint>;
    expect_reasons<IntOrPoint>(
        R"({"x":"a"})",
        "no alternative accepts the value; reading std::variant<int, UnionPoint>: alternative 0 (int): expected an "
        "integer, got an object; alternative 1 (UnionPoint) at /x: expected an integer, got a string (reading int)",
        "");
    expect_reasons<UnionReasonField>(
        R"({"v":{"x":"a"}})",
        "no alternative accepts the value at /v; reading std::variant<int, UnionPoint>: alternative 0 (int): expected "
        "an integer, got an object; alternative 1 (UnionPoint) at /x: expected an integer, got a string (reading int)",
        "/v");
    try
    {
        (void)yyjson::read<UnionReasonField>(R"({"v":{"x":"a"}})");
        ADD_FAILURE();
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(std::string(e.what()),
                  "no alternative accepts the value at /v (line 1, column 6, pos 5); reading std::variant<int, "
                  "UnionPoint>: alternative 0 (int): expected an integer, got an object; alternative 1 (UnionPoint) at "
                  "/x: expected an integer, got a string (reading int)");
    }

    // the setting of an alternative that the value does not satisfy
    using NonNegative = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, std::string>;
    expect_reasons<NonNegative>(
        "-1",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): the number is less than the "
                    "minimum 0; alternative 1 ({}): expected a string, got a number",
                    yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                    yyjson::detail::type_name<std::string>()),
        "");

    // the alternatives of yyjson::one_of_variant under the policies that count the matches, the one that the keys
    // of the object prove to fail too
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_reasons<ClosedOrXZ>(
        R"({"z":1})",
        R"(no alternative accepts the value; reading yyjson::one_of_variant<OneOfClosedX, OneOfXZ>: alternative 0 (OneOfClosedX) at /z: the object has a key that is not allowed (reading OneOfClosedX, which yyjson::additional_properties(false) closes); alternative 1 (OneOfXZ): a required key is missing (reading the key "x" of OneOfXZ))",
        "");
    // the one that matches under those policies and fails under the policies of the call gives its own failure
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_reasons<XOrText>(R"({"x":1,"x":2})", "the object has a duplicate key at /x; reading OneOfX", "/x",
                            yyjson::reject_duplicates);
    // also a variant within it, which gives the reasons of its alternatives
    expect_reasons<UnionReasonNode>(
        R"({"v":1,"next":{"v":{"x":1,"x":2},"next":null}})",
        "no alternative accepts the value at /next/v; reading std::variant<int, OneOfX>: alternative 0 (int): expected "
        "an integer, got an object; alternative 1 (OneOfX) at /x: the object has a duplicate key (reading OneOfX)",
        "/next/v", yyjson::reject_duplicates);
    // yyjson::reject_unknown_keys of the call closes the struct alternatives before the count
    expect_reasons<XOrText>(
        R"({"x":1,"k":2})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (OneOfX) at /k: the object has a key "
                    "that is not allowed (reading OneOfX); alternative 1 ({}): expected a string, got an object",
                    yyjson::detail::variant_name<XOrText>(), yyjson::detail::type_name<std::string>()),
        "", yyjson::reject_unknown_keys);

    // the path within an alternative goes through its members and elements, and a failure of the whole value has none
    using IntOrLine = std::variant<int, UnionReasonLine>;
    expect_reasons<IntOrLine>(R"({"from":{"x":1,"y":2},"to":[{"x":3,"y":4},{"x":5,"y":true}]})",
                              "no alternative accepts the value; reading std::variant<int, UnionReasonLine>: "
                              "alternative 0 (int): expected an integer, got an object; alternative 1 "
                              "(UnionReasonLine) at /to/1/y: expected an integer, got a boolean (reading int)",
                              "");
    expect_reasons<IntOrLine>(
        "[]",
        "no alternative accepts the value; reading std::variant<int, UnionReasonLine>: alternative 0 (int): expected "
        "an integer, got an array; alternative 1 (UnionReasonLine): expected an object, got an array",
        "");
    using IntOrMap = std::variant<int, std::map<std::string, int>>;
    expect_reasons<IntOrMap>(
        R"({"a/b~c":"x"})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got an "
                    "object; alternative 1 ({}) at /a~1b~0c: expected an integer, got a string (reading int)",
                    yyjson::detail::variant_name<IntOrMap>(), yyjson::detail::type_name<std::map<std::string, int>>()),
        "");

    // a struct alternative that a member of the object fails without a read still gives the reason of that read
    using CountOrText = std::variant<UnionReasonCount, UnionReasonText>;
    for (const auto& [json, first] : {std::pair{R"({"x":"a","y":"b"})", "x"}, std::pair{R"({"y":"b","x":"a"})", "y"}})
    {
        expect_reasons<CountOrText>(
            json,
            std::format("no alternative accepts the value; reading std::variant<UnionReasonCount, UnionReasonText>: "
                        "alternative 0 (UnionReasonCount) at /{}: expected an integer, got a string (reading int); "
                        "alternative 1 (UnionReasonText) at /y: expected an integer, got a string (reading int)",
                        first),
            "");
    }
    expect_reasons<CountOrText>(
        R"({"x":null})",
        std::format("no alternative accepts the value; reading std::variant<UnionReasonCount, UnionReasonText>: "
                    "alternative 0 (UnionReasonCount) at /x: expected an integer, got null (reading int); alternative "
                    "1 (UnionReasonText) at /x: expected a string, got null (reading {})",
                    yyjson::detail::type_name<std::string>()),
        "");
    EXPECT_EQ(yyjson::read<CountOrText>(R"({"x":"a","y":1})").index(), 1);
    EXPECT_EQ(yyjson::read(R"({"x":"a","y":1})").cast<CountOrText>().index(), 1);

    // a variant within an alternative gives no reasons of its own alternatives, which bounds the work
    using Nested = std::variant<int, std::variant<std::string, std::variant<bool, UnionPoint>>>;
    using Inner = std::variant_alternative_t<1, Nested>;
    expect_reasons<Nested>(
        R"({"x":"a"})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got an "
                    "object; alternative 1 ({}): no alternative accepts the value (reading {})",
                    yyjson::detail::variant_name<Nested>(), yyjson::detail::variant_name<Inner>(),
                    yyjson::detail::variant_name<Inner>()),
        "");
}

TEST(Constraints, OneOfBounds)
{
    // alternatives that read no value in common are not both read
    using Signs = yyjson::one_of_variant<yyjson::alt<int, yyjson::maximum(0)>, yyjson::alt<int, yyjson::minimum(1)>>;
    static_assert(!one_of_overlapping<Signs, 0, 1> && !one_of_contained<Signs>);
    expect_accepted<Signs>("-3", Signs(std::in_place_index<0>, -3));
    expect_accepted<Signs>("5", Signs(std::in_place_index<1>, 5));
    using Kinds = yyjson::one_of_variant<int, std::string, bool, std::vector<int>, std::nullptr_t>;
    static_assert(!one_of_overlapping<Kinds, 0, 1> && !one_of_overlapping<Kinds, 1, 3> && !one_of_contained<Kinds>);
    expect_accepted<Kinds>("[1]", Kinds(std::vector{1}));
    expect_accepted<Kinds>("true", Kinds(true));
    using Words = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::enum_("a", "b")>,
                                         yyjson::alt<std::string, yyjson::min_length(2)>>;
    static_assert(!one_of_overlapping<Words, 0, 1>);
    expect_accepted<Words>(R"("ab")", Words(std::in_place_index<1>, "ab"));
    using Lists = yyjson::one_of_variant<yyjson::alt<std::vector<int>, yyjson::max_items(2)>,
                                         yyjson::alt<std::vector<int>, yyjson::min_items(3)>>;
    static_assert(!one_of_overlapping<Lists, 0, 1>);
    expect_accepted<Lists>("[1,2,3]", Lists(std::in_place_index<1>, std::vector{1, 2, 3}));
    // a combinator may reject the empty value of a nullable alternative
    using NonNull = yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, yyjson::not_(yyjson::const_(nullptr))>,
                                           std::nullptr_t>;
    static_assert(!one_of_contained<NonNull>);
    static_assert(
        !one_of_contained<yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, &one_of_present>, std::nullptr_t>>);
    static_assert(
        !one_of_contained<
            yyjson::one_of_variant<yyjson::alt<yyjson::nullable<yyjson::object>, &one_of_object>, std::nullptr_t>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, yyjson::minimum(0)>, std::nullptr_t>>);
    expect_alternative<NonNull>("null", 1);
    expect_alternative<NonNull>("1", 0);
    using Owned = yyjson::one_of_variant<std::unique_ptr<int>, std::string>;
    expect_alternative<Owned>("1", 0);
    expect_alternative<Owned>(R"("a")", 1);

    // alternatives that may read a value in common are counted
    static_assert(one_of_overlapping<OneOfInts, 0, 1> && !one_of_contained<OneOfInts>);
    using Wide = yyjson::one_of_variant<std::int64_t, std::uint64_t>;
    static_assert(one_of_overlapping<Wide, 0, 1> && !one_of_contained<Wide>);
    expect_accepted<Wide>("-1", Wide(std::int64_t{-1}), false);
    expect_accepted<Wide>("18446744073709551615", Wide(std::uint64_t{18446744073709551615u}));
    expect_rejected<Wide>(
        "1",
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "");
    using Steps =
        yyjson::one_of_variant<yyjson::alt<int, yyjson::multiple_of(2)>, yyjson::alt<int, yyjson::multiple_of(3)>>;
    static_assert(one_of_overlapping<Steps, 0, 1> && !one_of_contained<Steps>);
    expect_accepted<Steps>("4", Steps(std::in_place_index<0>, 4), false);
    expect_rejected<Steps>("6",
                           "the value matches more than one alternative; reading yyjson::one_of_variant<int, int>: the "
                           "alternatives 0 and 1 match",
                           "");
    // a float compares a JSON integer exactly, also from 2^53, where a double rounds it
    using BigFloat = yyjson::one_of_variant<yyjson::alt<float, yyjson::exclusive_minimum(9007199254740992)>,
                                            yyjson::alt<std::uint64_t, yyjson::maximum(9007199254740993U)>>;
    static_assert(one_of_overlapping<BigFloat, 0, 1>);
    expect_rejected<BigFloat>(
        "9007199254740993",
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<float, std::uint64_t>>()),
        "");
    using FloatOrBig =
        yyjson::one_of_variant<yyjson::alt<float, yyjson::maximum(9007199254740992)>,
                               yyjson::alt<std::uint64_t, yyjson::const_(std::uint64_t{9007199254740993U})>>;
    static_assert(!one_of_contained<FloatOrBig>);
    expect_alternative<FloatOrBig>("9007199254740993", 1);
    expect_alternative<FloatOrBig>("9007199254740992", 0);

    // an alternative whose values another reads all of is a compile error
    static_assert(one_of_contained<yyjson::one_of_variant<int, double>>);
    static_assert(one_of_contained<yyjson::one_of_variant<int, int>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfX>>);
    static_assert(one_of_contained<yyjson::one_of_variant<std::nullptr_t, std::optional<int>>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>,
                                         yyjson::alt<int, yyjson::minimum(0), yyjson::maximum(5)>>>);
    static_assert(
        one_of_contained<yyjson::one_of_variant<yyjson::alt<std::string, yyjson::enum_("a", "b")>, std::string>>);
    static_assert(one_of_contained<yyjson::one_of_variant<yyjson::alt<std::string, &union_id_pattern>, std::string>>);
    using Numbers = yyjson::detail::one_of_relations<yyjson::one_of_variant<std::string, int, double>>;
    static_assert(Numbers::contained.contained == 1 && Numbers::contained.container == 2);
    // the analysis does not prove what a setting that it does not know reads
    static_assert(!one_of_contained<
                  yyjson::one_of_variant<std::vector<int>, yyjson::alt<std::vector<int>, yyjson::max_items(2)>>>);
    // a caster of the user that takes a value of the other document reads any value
    using OtherOrString = yyjson::one_of_variant<OneOfOtherCode, std::string>;
    static_assert(one_of_overlapping<OtherOrString, 0, 1>);
    expect_rejected_under<OtherOrString>(
        R"("a")", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                              yyjson::detail::variant_name<yyjson::one_of_variant<OneOfOtherCode, std::string>>()));
    expect_alternative<std::variant<OneOfOtherCode, std::string>>(R"("a")", 0);
    // such a caster of a range or a map reads values that the elements or the values cannot
    expect_alternative<std::variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<std::variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);

    // a struct reads each object of another one that requires each key that it requires and takes no key that it
    // does not name, with values that it reads, with and without yyjson::reject_unknown_keys of the call
    using XAndClosedX = yyjson::detail::one_of_relations<yyjson::one_of_variant<OneOfX, OneOfClosedX>>;
    static_assert(XAndClosedX::contained.contained == 1 && XAndClosedX::contained.container == 0);
    using ClosedByAlt =
        yyjson::alt<OneOfXZ, yyjson::property("x"), yyjson::property("z"), yyjson::additional_properties(false)>;
    static_assert(one_of_contained<yyjson::one_of_variant<ClosedByAlt, OneOfXZ>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>, OneOfX>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfWrapped<OneOfClosedX>>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfWrapped<OneOfX>, OneOfClosedX>>);
    // the call's yyjson::reject_unknown_keys closes a struct that names fewer keys, so that it reads no object
    // with a key that only the other one names
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, ClosedByAlt>>);
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, OneOfXMaybeY>>);
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, OneOfWrapped<OneOfXZ>>>);
    using XOrXZ = yyjson::one_of_variant<OneOfX, OneOfXZ>;
    static_assert(!one_of_contained<XOrXZ> && one_of_overlapping<XOrXZ, 0, 1>);
    expect_rejected_under<XOrXZ>(R"({"x":1,"z":2})",
                                 "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                                 "OneOfXZ>: the alternatives 0 and 1 match");
    expect_alternative<XOrXZ>(R"({"x":1,"z":2})", 1, yyjson::reject_unknown_keys);
    expect_alternative<XOrXZ>(R"({"x":1})", 0, yyjson::reject_unknown_keys);
    // a map of other keys keeps a struct open under the call's yyjson::reject_unknown_keys
    using XOrRest = yyjson::one_of_variant<OneOfX, OneOfXRest>;
    static_assert(!one_of_contained<XOrRest>);
    expect_alternative<XOrRest>(R"({"x":1,"a":2})", 1, yyjson::reject_unknown_keys);

    // a struct reads no object of another one that requires a key that it rejects, or a value there that it does
    // not read
    using ClosedXOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    static_assert(!one_of_overlapping<ClosedXOrXZ, 0, 1>);
    expect_alternative<ClosedXOrXZ>(R"({"x":1,"z":2})", 1);
    expect_alternative<ClosedXOrXZ>(R"({"x":1})", 0);
    using IntOrText = yyjson::one_of_variant<OneOfX, OneOfTextX>;
    static_assert(!one_of_overlapping<IntOrText, 0, 1>);
    expect_alternative<IntOrText>(R"({"x":"a"})", 1);
    expect_alternative<IntOrText>(R"({"x":1})", 0);
    using Letters = yyjson::one_of_variant<yyjson::alt<OneOfTextX, yyjson::property("x", yyjson::enum_("a", "b"))>,
                                           yyjson::alt<OneOfTextX, yyjson::property("x", yyjson::enum_("c"))>>;
    static_assert(!one_of_overlapping<Letters, 0, 1>);
    expect_alternative<Letters>(R"({"x":"c"})", 1);
    expect_alternative<Letters>(R"({"x":"a"})", 0);
    // a std::optional does not take null as a missing key while the alternatives are counted
    using IntOrNull = yyjson::one_of_variant<OneOfMaybeX, OneOfNullX>;
    static_assert(!one_of_overlapping<IntOrNull, 0, 1> && !one_of_contained<IntOrNull>);
    expect_alternative<IntOrNull>(R"({"x":null})", 1);
    expect_alternative<IntOrNull>(R"({"x":1})", 0);
    expect_alternative<IntOrNull>("{}", 0);

    // two structs that read some values of each other at a key are counted
    using SignedOrUnsigned = yyjson::one_of_variant<OneOfSignedX, OneOfUnsignedX>;
    static_assert(!one_of_contained<SignedOrUnsigned> && one_of_overlapping<SignedOrUnsigned, 0, 1>);
    expect_rejected<SignedOrUnsigned>(
        R"({"x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfSignedX, OneOfUnsignedX>: "
        "the alternatives 0 and 1 match",
        "");
    expect_alternative<SignedOrUnsigned>(R"({"x":-1})", 0);

    // the analysis does not prove that a struct reads each value at a key that it does not name
    using MaybeYOrZ = yyjson::one_of_variant<OneOfXMaybeY, OneOfXMaybeZ>;
    static_assert(!one_of_contained<MaybeYOrZ> && one_of_overlapping<MaybeYOrZ, 0, 1>);
    expect_rejected<MaybeYOrZ>(R"({"x":1})",
                               "the value matches more than one alternative; reading "
                               "yyjson::one_of_variant<OneOfXMaybeY, OneOfXMaybeZ>: the alternatives 0 and 1 match",
                               "");
    expect_alternative<MaybeYOrZ>(R"({"x":1,"y":"a"})", 1);
    // nor what a struct reads with a setting other than the keys that it takes
    using MinimumX = yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>,
                                            yyjson::alt<OneOfXZ, yyjson::property("x", yyjson::minimum(0))>>;
    static_assert(!one_of_contained<MinimumX> && one_of_overlapping<MinimumX, 0, 1>);
    expect_rejected<MinimumX>(R"({"x":1,"z":2})",
                              "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                              "OneOfXZ>: the alternatives 0 and 1 match",
                              "");
    // nor what a struct with a map of other keys reads at those keys
    using XZOrAny = yyjson::one_of_variant<OneOfXZ, OneOfXAny>;
    static_assert(!one_of_contained<XZOrAny> && one_of_overlapping<XZOrAny, 0, 1>);
    expect_rejected<XZOrAny>(R"({"x":1,"z":2})",
                             "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfXZ, "
                             "OneOfXAny>: the alternatives 0 and 1 match",
                             "");
    // nor compares the keys of two structs beyond 256 pairs of them
    using Many = yyjson::one_of_variant<OneOfMany16, OneOfMany17>;
    static_assert(!one_of_contained<Many> && one_of_overlapping<Many, 0, 1>);

    // a field that is read otherwise than its type reads a value of its own may read any value: in place by a
    // caster of the user, by a caster of the user of a std::optional, or as the value of an adjacent tag
    using CodeOrText = yyjson::one_of_variant<OneOfCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<CodeOrText, 0, 1>);
    expect_rejected_under<CodeOrText>(R"({"x":"a"})",
                                      "the value matches more than one alternative; reading "
                                      "yyjson::one_of_variant<OneOfCodeX, OneOfTextX>: the alternatives 0 and 1 match");
    // a caster of the user of the other document reads any value, also in a field
    using OtherCodeOrText = yyjson::one_of_variant<OneOfOtherCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<OtherCodeOrText, 0, 1>);
    expect_rejected_under<OtherCodeOrText>(
        R"({"x":"a"})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfOtherCodeX, OneOfTextX>: the "
        "alternatives 0 and 1 match");
    using NumberOrText = yyjson::one_of_variant<OneOfMaybeNumberX, OneOfTextX>;
    static_assert(one_of_overlapping<NumberOrText, 0, 1>);
    expect_rejected_under<NumberOrText>(
        R"({"x":"a"})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfMaybeNumberX, OneOfTextX>: "
        "the alternatives 0 and 1 match");
    using AdjacentOrX = yyjson::one_of_variant<OneOfAdjacentX, OneOfX>;
    static_assert(one_of_overlapping<AdjacentOrX, 0, 1>);
    expect_rejected_under<AdjacentOrX>(
        R"({"kind":"i","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfAdjacentX, OneOfX>: the "
        "alternatives 0 and 1 match");
    // the key of the tag of an enclosing variant tells no closed struct apart, and a closed struct does not read
    // only what another one reads that names that key
    using XKind = std::variant_alternative_t<0, OneOfTaggedXKind>;
    static_assert(!one_of_overlapping<XKind, 0, 1> && yyjson::detail::one_of_variant_cast<XKind>::tagged_overlap[1][0]);
    expect_alternative<XKind>(R"({"x":1,"kind":"p"})", 1);
    expect_rejected_under<OneOfTaggedXKind>(
        R"({"kind":"p","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfClosedX, OneOfXKind>: the "
        "alternatives 0 and 1 match");
    using MaybeKind = std::variant_alternative_t<0, OneOfTaggedMaybeKind>;
    static_assert(!one_of_contained<MaybeKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedMaybeKind>(R"({"kind":"p","x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","x":1})").cast<OneOfTaggedMaybeKind>()).index(), 0);
    // the closed struct also takes the key of the tag again with a value that the other one does not read
    using TextKind = std::variant_alternative_t<0, OneOfTaggedTextKind>;
    static_assert(!one_of_contained<TextKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedTextKind>(R"({"kind":"p","kind":1,"x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","kind":1,"x":1})").cast<OneOfTaggedTextKind>()).index(), 0);
    expect_rejected_under<OneOfTaggedTextKind>(
        R"({"kind":"p","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfClosedX, "
        "OneOfXMaybeTextKind>: the alternatives 0 and 1 match");
    // each transparent struct between an alternative and its keyed struct may have settings
    using LayeredOrXZ = yyjson::one_of_variant<OneOfWrapped<OneOfMidX>, OneOfXZ>;
    static_assert(!one_of_contained<LayeredOrXZ>);
    expect_alternative<LayeredOrXZ>(R"({"x":-1,"z":1})", 1);
    expect_rejected_under<LayeredOrXZ>(
        R"({"x":1,"z":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfWrapped<OneOfMidX>, "
        "OneOfXZ>: the alternatives 0 and 1 match");
}

#if defined(__cpp_lib_define_static)
TEST(Constraints, UnionStringViewSettings)
{
    // a setting made from a std::string_view is a template argument
    using Id = yyjson::any_of_variant<yyjson::alt<std::string, yyjson::pattern(union_id_regex)>, int>;
    expect_alternative<Id>(R"("abc")", 0);
    EXPECT_THROW((void)yyjson::read<Id>(R"("ABC")"), yyjson::bad_cast);
    using OneId = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::pattern(union_id_regex)>,
                                         yyjson::alt<std::string, yyjson::max_length(2)>>;
    expect_alternative<OneId>(R"("abc")", 0);
    expect_alternative<OneId>(R"("AB")", 1);
    EXPECT_THROW((void)yyjson::read<OneId>(R"("ab")"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"("ab")").cast<OneId>(), yyjson::bad_cast);
}
#endif

// NOLINTEND
