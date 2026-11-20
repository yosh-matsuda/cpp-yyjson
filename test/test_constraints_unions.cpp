#include "test_constraints_common.hpp"

// NOLINTBEGIN

TEST(Constraints, UnionTypes)
{
    // the alternatives of one type are told apart by the index
    auto a = UnionValues(std::in_place_index<1>, 200);
    EXPECT_EQ(a.index(), 1);
    EXPECT_EQ(std::get<1>(a), 200);
    a = std::string("abc");
    EXPECT_TRUE(std::holds_alternative<std::string>(a));
    EXPECT_EQ(std::visit([](const auto& v) { return sizeof(v); }, a), sizeof(std::string));
    const auto b = a;
    EXPECT_EQ(a, b);
    a.emplace<0>(4);
    EXPECT_NE(a, b);
    EXPECT_EQ(*std::get_if<0>(&a), 4);
    static_assert(std::is_same_v<yyjson::any_of_variant<yyjson::alt<int>>::variant_type, std::variant<int>>);
}

TEST(Constraints, UnionReading)
{
    // the first alternative whose type reads the value and whose settings it satisfies
    expect_accepted<UnionValues>("4", UnionValues(std::in_place_index<0>, 4));
    expect_accepted<UnionValues>("200", UnionValues(std::in_place_index<1>, 200));
    expect_accepted<UnionValues>(R"("abc")", UnionValues(std::string("abc")));
    expect_accepted<UnionValues>("[3,1]", UnionValues(std::vector{3, 1}));
    expect_accepted<UnionValues>(R"([5,"a"])", UnionValues(std::tuple(5, std::string("a"))));
    // the reasons of the alternatives for a value of each kind but an array
    const auto kind_reasons = [](std::string_view kind, std::string_view text) {
        const auto got = [&](std::string_view expected) { return std::format("expected {}, got {}", expected, kind); };
        return read_by_none<UnionValues>({got("an integer"), got("an integer"),
                                          text.empty() ? got("a string") : std::string(text), got("an array"),
                                          got("an array")});
    };
    const auto number_reasons = [](std::string_view first, std::string_view path = "") {
        return read_by_none<UnionValues>({first, "the value is not one of 100, 200", "expected a string, got a number",
                                          "expected an array, got a number", "expected an array, got a number"},
                                         path);
    };
    expect_rejected<UnionValues>("5", number_reasons("the value does not satisfy the predicate"), "");
    expect_rejected<UnionValues>("150", number_reasons("the number is greater than the maximum 9"), "");
    expect_rejected<UnionValues>(R"("ABC")",
                                 kind_reasons("a string", R"(the string does not match the pattern "^[a-z]+$")"), "");
    expect_rejected<UnionValues>(R"("abcdefghi")",
                                 kind_reasons("a string", "the string is longer than the maximum length 8"), "");
    expect_rejected<UnionValues>("[5,6]",
                                 read_by_none<UnionValues>({"expected an integer, got an array",
                                                            "expected an integer, got an array",
                                                            "expected a string, got an array",
                                                            "the array has no item that satisfies contains",
                                                            {std::format("expected a string, got a number (reading {})",
                                                                         yyjson::detail::type_name<std::string>()),
                                                             "/1"}}),
                                 "");
    expect_rejected<UnionValues>("1.5", kind_reasons("a number", ""), "");
    expect_rejected<UnionValues>("null", kind_reasons("null", ""), "");
    expect_rejected<UnionFields>(R"({"v":5})", number_reasons("the value does not satisfy the predicate", "/v"), "/v");

    // an earlier alternative of the same type may read the value that a later one held
    using Ints = yyjson::any_of_variant<int, yyjson::alt<int, yyjson::minimum(0)>>;
    EXPECT_EQ(yyjson::read<Ints>(yyjson::write(Ints(std::in_place_index<1>, 5))).index(), 0);

    // the keys of an object rank the structs as those of a std::variant
    using PointOrLabel = yyjson::any_of_variant<UnionPoint, UnionLabel>;
    expect_alternative<PointOrLabel>(R"({"label":"a"})", 1);
    expect_alternative<PointOrLabel>(R"({"x":1})", 0);
    using ClosedOrPoint = yyjson::any_of_variant<UnionClosed, UnionPoint>;
    expect_alternative<ClosedOrPoint>(R"({"x":1})", 0);
    expect_alternative<ClosedOrPoint>(R"({"x":1,"y":2})", 1);
    using PointOrMap = yyjson::any_of_variant<UnionPoint, UnionMap>;
    expect_alternative<PointOrMap>(R"({"x":1,"k":2})", 0);
    expect_alternative<PointOrMap>(R"({"x":1,"k":2})", 1, yyjson::reject_unknown_keys);
    // the settings of a struct alternative check the keys of the object after the struct reads it
    using CountedOrMap = yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::min_properties(2)>, UnionMap>;
    expect_alternative<CountedOrMap>(R"({"x":1,"y":2})", 0);
    expect_alternative<CountedOrMap>(R"({"x":1})", 1);
    // in an alternative, additional_properties(false) accepts only the keys of the tags, as at a field
    using ClosedAltOrMap =
        yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::additional_properties(false)>, UnionMap>;
    expect_alternative<ClosedAltOrMap>("{}", 0);
    expect_alternative<ClosedAltOrMap>(R"({"x":1})", 1);
    expect_alternative<ClosedAltOrMap>(R"({"k":1})", 1);

    // yyjson::field in an alternative constrains a member, as in a field that holds a struct
    using Shapes = yyjson::any_of_variant<yyjson::alt<UnionShape, yyjson::property("kind", yyjson::const_("circle"))>,
                                          yyjson::alt<UnionShape, yyjson::property("kind", yyjson::const_("square"))>>;
    expect_alternative<Shapes>(R"({"kind":"square","size":2})", 1);
    expect_alternative<Shapes>(R"({"kind":"circle"})", 0);
    expect_rejected<Shapes>(R"({"kind":"hexagon"})",
                            "no alternative accepts the value; reading yyjson::any_of_variant<UnionShape, UnionShape>: "
                            "alternative 0 (UnionShape): the object does not satisfy properties; alternative 1 "
                            "(UnionShape): the object does not satisfy properties",
                            "");
    using RequiredX = yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::required("x")>, UnionPoint>;
    expect_alternative<RequiredX>(R"({"x":1})", 0);
    expect_alternative<RequiredX>(R"({"y":1})", 1);
    // additional_properties(false) takes only the keys that yyjson::field names, also when it ranks the structs
    using NamedX = yyjson::any_of_variant<
        yyjson::alt<UnionPoint, yyjson::property("x", yyjson::minimum(0)), yyjson::additional_properties(false)>,
        UnionMap>;
    expect_alternative<NamedX>(R"({"x":1})", 0);
    expect_alternative<NamedX>(R"({"x":1,"y":2})", 1);
    expect_alternative<NamedX>(R"({"x":-1})", 1);
    // a setting that holds pointers to strings is given through a pointer before C++26
    using YNeedsX = yyjson::any_of_variant<yyjson::alt<UnionPoint, &union_y_needs_x>, UnionMap>;
    expect_alternative<YNeedsX>(R"({"x":1,"y":2})", 0);
    expect_alternative<YNeedsX>(R"({"y":2})", 1);
    // additional_properties(false) also accepts the tag of the variant that holds the union
    expect_alternative<UnionTagged>(R"({"kind":"p","x":1})", 0);
    expect_alternative<UnionTagged>(R"({"x":1,"kind":"p"})", 0);
    expect_rejected<UnionTagged>(
        R"({"kind":"p","x":1,"z":1})",
        "no alternative accepts the value; reading yyjson::any_of_variant<UnionPoint>: alternative 0 (UnionPoint): the "
        "object has a key that additionalProperties does not allow",
        "");

    // the settings of an alternative reach the value that a variant held in a variant holds, at any depth
    using IntOrText = std::variant<int, std::string>;
    using Nested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<IntOrText, double>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    for (const auto json : {"1", "1.5"})
        expect_rejected<Nested>(json, read_by_none<Nested>({"the number is less than the minimum 5"}), "");
    expect_rejected<Nested>(R"("a")", read_by_none<Nested>({"the string is shorter than the minimum length 2"}), "");
    for (const auto json : {"7", "7.5", R"("ab")", "true"}) EXPECT_NO_THROW((void)yyjson::read<Nested>(json)) << json;
    using NestedListed = yyjson::any_of_variant<yyjson::alt<std::variant<IntOrText, bool>, yyjson::enum_(5, "ab")>>;
    expect_alternative<NestedListed>("5", 0);
    expect_alternative<NestedListed>(R"("ab")", 0);
    expect_rejected<NestedListed>(
        "1",
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 ({}): the value is not one of 5, "ab")",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");
    // read_into and merge_into replace the value
    auto fields = UnionFields{UnionValues(std::in_place_index<0>, 2), UnionPoint{1, 2}};
    yyjson::merge_into(R"({"p":{"x":5}})", fields);
    EXPECT_EQ(fields.p, (yyjson::any_of_variant<UnionPoint, std::string>(UnionPoint{5, 0})));
    yyjson::read_into(R"({"v":"id","p":{"y":3}})", fields);
    EXPECT_EQ(fields, (UnionFields{UnionValues(std::string("id")), UnionPoint{0, 3}}));

    // written as the alternative held, in both writers
    fields = UnionFields{UnionValues(std::in_place_index<1>, 100), std::string("s")};
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"v":100,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::object(fields).write()), R"({"v":100,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::value(UnionValues(std::in_place_index<0>, 99)).write()), "99");
}

TEST(Constraints, UnionDomValues)
{
    // yyjson::matches is a setting of an alternative of a value of the DOM
    using ListOrText =
        yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::matches<std::vector<int>>>, std::string>;
    EXPECT_EQ(yyjson::read("[1,2]").cast<ListOrText>().index(), 0);
    EXPECT_EQ(yyjson::read(R"("a")").cast<ListOrText>().index(), 1);
    EXPECT_THROW((void)yyjson::read(R"([1,"a"])").cast<ListOrText>(), yyjson::bad_cast);

    // the union reads any value, though the std::variant that stores it reads only an object
    static_assert(std::same_as<yyjson::any_of_variant<yyjson::value>::variant_type, UnionStorage>);
    using ValueOrText = std::variant<yyjson::any_of_variant<yyjson::value>, std::string>;
    expect_alternative<ValueOrText>("1", 0);
    expect_alternative<ValueOrText>(R"("a")", 0);
}

// NOLINTEND
