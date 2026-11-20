#include "test_constraints_common.hpp"

// NOLINTBEGIN

// the subschemas of yyjson::any_of, one_of, not_ and if_then_else that no value satisfies, also by the bounds of
// the type
struct DeadBranches
{
    unsigned char byte = 0;
    int any = 0;
    std::string text = {};
    std::vector<int> one = {};
    std::optional<unsigned char> low = {};
    bool operator==(const DeadBranches&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<DeadBranches> = yyjson::annotate(
    yyjson::field("byte", yyjson::not_(yyjson::minimum(256))),
    yyjson::field("any", yyjson::any_of(yyjson::all_of(yyjson::minimum(5), yyjson::maximum(1)), yyjson::maximum(3))),
    yyjson::field("text",
                  yyjson::if_then_else(yyjson::if_(yyjson::min_length(3), yyjson::max_length(2)),
                                       yyjson::then(yyjson::const_("a")), yyjson::else_(yyjson::max_length(2)))),
    yyjson::field("one",
                  yyjson::one_of(yyjson::min_items(2), yyjson::all_of(yyjson::min_items(3), yyjson::max_items(1)))),
    yyjson::field("low", yyjson::any_of(yyjson::maximum(-1), yyjson::minimum(10))));

// the subschemas of yyjson::not_ at each position that takes keywords
struct Negated
{
    int n = 0;
    std::string s = {};
    std::vector<int> v = {};
    int twice = 0;
    std::map<std::string, int> m = {};
    std::tuple<int, std::string> row = {};
    std::vector<int> c = {};
    std::vector<std::optional<int>> holes = {};
    std::optional<int> o = {};
    std::vector<std::vector<int>> nested = {};
    bool operator==(const Negated&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Negated> = yyjson::annotate(
    yyjson::field("n", yyjson::not_(yyjson::minimum(10), yyjson::maximum(19))),
    yyjson::field("s", yyjson::not_(yyjson::pattern("^x"))),
    yyjson::field("v", yyjson::items(yyjson::not_(yyjson::not_(yyjson::minimum(0))))),
    yyjson::field("twice", yyjson::not_(yyjson::const_(1)), yyjson::not_(yyjson::const_(2))),
    yyjson::field("m", yyjson::additional_properties(yyjson::not_(yyjson::const_(0))),
                  yyjson::property_names(yyjson::not_(yyjson::pattern("^_")))),
    yyjson::field("row", yyjson::prefix_items(yyjson::not_(yyjson::const_(0)), {yyjson::not_(yyjson::const_("x"))})),
    yyjson::field("c", yyjson::contains(yyjson::not_(yyjson::minimum(0)))),
    yyjson::field("holes", yyjson::items(yyjson::not_(yyjson::minimum(5)))),
    yyjson::field("o", yyjson::not_(yyjson::minimum(5))),
    yyjson::field("nested", yyjson::not_(yyjson::items(yyjson::min_items(1)))));

// yyjson::if_then_else with both branches and with one
struct Conditional
{
    int n = 0;
    std::vector<int> v = {};
    std::vector<int> e = {};
    std::string s = {};
    std::map<std::string, std::string> m = {};
    bool operator==(const Conditional&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Conditional> = yyjson::annotate(
    yyjson::field("n", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                            yyjson::else_(yyjson::maximum(5)))),
    yyjson::field(
        "v", yyjson::if_then_else(yyjson::if_(yyjson::min_items(2)), yyjson::then(yyjson::items(yyjson::maximum(9))))),
    yyjson::field(
        "e", yyjson::items(yyjson::if_then_else(yyjson::if_(yyjson::const_(0)), yyjson::else_(yyjson::minimum(5))))),
    yyjson::field("s", yyjson::if_then_else(yyjson::if_(yyjson::pattern("^#")), yyjson::then(yyjson::min_length(4)))),
    yyjson::field("m", yyjson::additional_properties(yyjson::if_then_else(yyjson::if_(yyjson::const_("x")),
                                                                          yyjson::else_(yyjson::enum_("y", "z"))))));

// yyjson::all_of, any_of and one_of, whose subschemas are braced lists or settings alone
struct Composed
{
    int a = 0;
    int b = 0;
    int c = 0;
    std::vector<int> v = {};
    std::string s = {};
    std::map<std::string, int> m = {};
    int deep = 0;
    std::vector<std::vector<int>> rows = {};
    bool operator==(const Composed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Composed> = yyjson::annotate(
    yyjson::field("a", yyjson::all_of(yyjson::minimum(0), {yyjson::multiple_of(2), yyjson::maximum(10)})),
    yyjson::field("b", yyjson::any_of(yyjson::maximum(0), {yyjson::minimum(10), yyjson::maximum(20)})),
    yyjson::field("c", yyjson::one_of(yyjson::multiple_of(2), yyjson::multiple_of(3))),
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::const_(0), yyjson::minimum(10)))),
    yyjson::field("s", yyjson::any_of(yyjson::const_("a"), {yyjson::enum_("b", "c")}, {yyjson::pattern("^d")})),
    yyjson::field("m", yyjson::additional_properties(yyjson::one_of({yyjson::minimum(0)}, {yyjson::maximum(9)}))),
    yyjson::field("deep", yyjson::one_of(yyjson::not_(yyjson::minimum(0)),
                                         yyjson::all_of(yyjson::minimum(100),
                                                        yyjson::any_of(yyjson::const_(100), yyjson::minimum(200))))),
    yyjson::field("rows", yyjson::any_of(yyjson::max_items(1), yyjson::items(yyjson::min_items(2)))));

// yyjson::matches at the positions of values of the DOM
struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
};
struct Typed
{
    std::vector<yyjson::value> events = {};
    std::vector<yyjson::value> ids = {};
    yyjson::value point = {};
    yyjson::value text = {};
    std::map<std::string, yyjson::value> small = {};
    bool operator==(const Typed& other) const
    {
        const auto write = [](const auto& t) {
            return std::string(yyjson::write(std::tuple(t.events, t.ids, t.point, t.text, t.small)).data());
        };
        return write(*this) == write(other);
    }
};
template <>
inline constexpr auto yyjson::annotations<Typed> = yyjson::annotate(
    yyjson::field("events", yyjson::contains(yyjson::matches<Point>), yyjson::min_contains(2)),
    yyjson::field("ids", yyjson::items(yyjson::any_of(yyjson::matches<int>, yyjson::matches<std::string>))),
    yyjson::field("point", yyjson::matches<Point>), yyjson::field("text", yyjson::not_(yyjson::matches<std::string>)),
    yyjson::field("small", yyjson::additional_properties(yyjson::matches<std::uint8_t>)));

// the combinators of a struct that VISITABLE_STRUCT describes
struct VisitedCombined
{
    int n = 0;
    std::vector<int> v = {};
    bool operator==(const VisitedCombined&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VisitedCombined> =
    yyjson::annotate(yyjson::field("n", yyjson::not_(yyjson::const_(0))),
                     yyjson::field("v", yyjson::items(yyjson::one_of(yyjson::minimum(5), yyjson::maximum(7)))));
VISITABLE_STRUCT(VisitedCombined, n, v);

TEST(Constraints, Not)
{
    expect_accepted<Negated>(
        R"({"n":9,"s":"y","v":[0,1],"twice":3,"m":{"a":1},"row":[1,"y"],"c":[1,-1],"holes":[1],"o":3,"nested":[[]]})",
        {9, "y", {0, 1}, 3, {{"a", 1}}, {1, "y"}, {1, -1}, {1}, 3, {{}}});
    expect_accepted<Negated>(R"({"n":20,"o":null})", {.n = 20});

    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Negated )", key);
    };
    expect_rejected<Negated>(R"({"n":10})",
                             R"(the value satisfies the subschema of not at /n; reading the key "n" of Negated)", "/n");
    expect_rejected<Negated>(R"({"n":19})",
                             R"(the value satisfies the subschema of not at /n; reading the key "n" of Negated)", "/n");
    expect_rejected<Negated>(R"({"s":"xy"})",
                             R"(the value satisfies the subschema of not at /s; reading the key "s" of Negated)", "/s");
    // not_(not_(minimum(0))) is minimum(0)
    expect_rejected<Negated>(
        R"({"v":[0,-1]})",
        R"(the value satisfies the subschema of not at /v/1; reading an element of the value at the key "v" of Negated)",
        "/v/1");
    // each yyjson::not_ of a field is checked
    expect_rejected<Negated>(
        R"({"twice":1})", R"(the value satisfies the subschema of not at /twice; reading the key "twice" of Negated)",
        "/twice");
    expect_rejected<Negated>(
        R"({"twice":2})", R"(the value satisfies the subschema of not at /twice; reading the key "twice" of Negated)",
        "/twice");
    expect_rejected<Negated>(
        R"({"m":{"a":0}})",
        R"(the value satisfies the subschema of not at /m/a; reading an element of the value at the key "m" of Negated)",
        "/m/a");
    expect_rejected<Negated>(
        R"({"m":{"_a":1}})",
        R"(the key satisfies the subschema of not at /m/_a; reading a key of the value at the key "m" of Negated)",
        "/m/_a");
    expect_rejected<Negated>(
        R"({"row":[0,"y"]})",
        R"(the value satisfies the subschema of not at /row/0; reading an element of the value at the key "row" of Negated)",
        "/row/0");
    expect_rejected<Negated>(
        R"({"row":[1,"x"]})",
        R"(the value satisfies the subschema of not at /row/1; reading an element of the value at the key "row" of Negated)",
        "/row/1");
    expect_rejected<Negated>(R"({"c":[1,2]})",
                             R"(the array has no item that satisfies contains at /c; reading the key "c" of Negated)",
                             "/c");
    // null satisfies minimum(5), so not_ rejects a null element, but a null field is missing
    expect_rejected<Negated>(
        R"({"holes":[1,null]})",
        R"(the value satisfies the subschema of not at /holes/1; reading an element of the value at the key "holes" of Negated)",
        "/holes/1");
    expect_rejected<Negated>(R"({"o":5})",
                             R"(the value satisfies the subschema of not at /o; reading the key "o" of Negated)", "/o");
    // not_ of a keyword of the elements
    expect_rejected<Negated>(
        R"({"nested":[[1]]})",
        R"(the value satisfies the subschema of not at /nested; reading the key "nested" of Negated)", "/nested");
    expect_rejected<Negated>(
        R"({"nested":[]})",
        R"(the value satisfies the subschema of not at /nested; reading the key "nested" of Negated)", "/nested");
}

TEST(Constraints, IfThenElse)
{
    expect_accepted<Conditional>(R"({"n":12,"v":[10],"e":[0,5],"s":"#abc","m":{"a":"x","b":"y"}})",
                                 {12, {10}, {0, 5}, "#abc", {{"a", "x"}, {"b", "y"}}});
    expect_accepted<Conditional>(R"({"n":5,"v":[1,9],"s":"ab"})", {5, {1, 9}, {}, "ab", {}});

    expect_rejected<Conditional>(
        R"({"n":11})", R"(the value satisfies if and not then at /n; reading the key "n" of Conditional)", "/n");
    expect_rejected<Conditional>(
        R"({"n":6})", R"(the value satisfies neither if nor else at /n; reading the key "n" of Conditional)", "/n");
    expect_rejected<Conditional>(
        R"({"v":[1,10]})", R"(the value satisfies if and not then at /v; reading the key "v" of Conditional)", "/v");
    expect_rejected<Conditional>(
        R"({"e":[0,4]})",
        R"(the value satisfies neither if nor else at /e/1; reading an element of the value at the key "e" of Conditional)",
        "/e/1");
    expect_rejected<Conditional>(
        R"({"s":"#ab"})", R"(the value satisfies if and not then at /s; reading the key "s" of Conditional)", "/s");
    expect_rejected<Conditional>(
        R"({"m":{"a":"w"}})",
        R"(the value satisfies neither if nor else at /m/a; reading an element of the value at the key "m" of Conditional)",
        "/m/a");
}

TEST(Constraints, AllAnyOneOf)
{
    expect_accepted<Composed>(R"({"a":10,"b":-1,"c":4,"v":[0,10],"s":"c","m":{"k":-1},"deep":-5,"rows":[[1,2],[3,4]]})",
                              {10, -1, 4, {0, 10}, "c", {{"k", -1}}, -5, {{1, 2}, {3, 4}}});
    expect_accepted<Composed>(R"({"b":15,"c":3,"s":"a","m":{"k":10},"deep":100,"rows":[[]]})",
                              {.b = 15, .c = 3, .s = "a", .m = {{"k", 10}}, .deep = 100, .rows = {{}}});
    expect_accepted<Composed>(R"({"c":9,"s":"dz","deep":300})", {.c = 9, .s = "dz", .deep = 300});

    expect_rejected<Composed>(
        R"({"a":-2})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"a":3})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"a":12})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"b":5})", R"(the value satisfies none of the subschemas of anyOf at /b; reading the key "b" of Composed)",
        "/b");
    expect_rejected<Composed>(
        R"({"b":21})", R"(the value satisfies none of the subschemas of anyOf at /b; reading the key "b" of Composed)",
        "/b");
    expect_rejected<Composed>(
        R"({"c":1})", R"(the value satisfies none of the subschemas of oneOf at /c; reading the key "c" of Composed)",
        "/c");
    expect_rejected<Composed>(
        R"({"c":6})", R"(the value satisfies more than one subschema of oneOf at /c; reading the key "c" of Composed)",
        "/c");
    expect_rejected<Composed>(
        R"({"v":[0,5]})",
        R"(the value satisfies none of the subschemas of anyOf at /v/1; reading an element of the value at the key "v" of Composed)",
        "/v/1");
    expect_rejected<Composed>(
        R"({"s":"e"})", R"(the value satisfies none of the subschemas of anyOf at /s; reading the key "s" of Composed)",
        "/s");
    // minimum(0) and maximum(9) both hold for 5
    expect_rejected<Composed>(
        R"({"m":{"k":5}})",
        R"(the value satisfies more than one subschema of oneOf at /m/k; reading an element of the value at the key "m" of Composed)",
        "/m/k");
    expect_rejected<Composed>(
        R"({"deep":0})",
        R"(the value satisfies none of the subschemas of oneOf at /deep; reading the key "deep" of Composed)", "/deep");
    expect_rejected<Composed>(
        R"({"deep":150})",
        R"(the value satisfies none of the subschemas of oneOf at /deep; reading the key "deep" of Composed)", "/deep");
    expect_rejected<Composed>(
        R"({"rows":[[1],[2]]})",
        R"(the value satisfies none of the subschemas of anyOf at /rows; reading the key "rows" of Composed)", "/rows");
}

TEST(Constraints, DeadBranches)
{
    // a subschema that no value satisfies is false, as in JSON Schema
    expect_accepted<DeadBranches>(R"({"byte":255,"any":3,"text":"ab","one":[1,2],"low":10})",
                                  {255, 3, "ab", {1, 2}, 10});
    expect_accepted<DeadBranches>(R"({"byte":0,"any":-7,"text":"","one":[1,2,3]})", {0, -7, "", {1, 2, 3}});
    expect_rejected<DeadBranches>(
        R"({"any":4})",
        R"(the value satisfies none of the subschemas of anyOf at /any; reading the key "any" of DeadBranches)",
        "/any");
    expect_rejected<DeadBranches>(
        R"({"text":"abc"})",
        R"(the value satisfies neither if nor else at /text; reading the key "text" of DeadBranches)", "/text");
    expect_rejected<DeadBranches>(
        R"({"one":[1]})",
        R"(the value satisfies none of the subschemas of oneOf at /one; reading the key "one" of DeadBranches)",
        "/one");
    expect_rejected<DeadBranches>(
        R"({"low":9})",
        R"(the value satisfies none of the subschemas of anyOf at /low; reading the key "low" of DeadBranches)",
        "/low");
    expect_rejected<DeadBranches>(R"({"byte":256})",
                                  "the number is out of range, got 256 at /byte; reading unsigned char", "/byte");
}

TEST(Constraints, Matches)
{
    expect_accepted<Typed>(
        R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},"text":1,)"
        R"("small":{"a":255}})",
        yyjson::read<Typed>(R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},)"
                            R"("text":1,"small":{"a":255}})"));

    // a point without "y" is not read under yyjson::strict
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1,"y":null}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"y":3}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"ids":[1,1.5],"point":{"x":1,"y":2}})",
        R"(the value satisfies none of the subschemas of anyOf at /ids/1; reading an element of the value at the key "ids" of Typed)",
        "/ids/1");
    expect_rejected<Typed>(R"({"point":[1,2]})",
                           R"(the value does not satisfy matches at /point; reading the key "point" of Typed)",
                           "/point");
    expect_rejected<Typed>(R"({"point":{"x":1,"y":2},"text":"a"})",
                           R"(the value satisfies the subschema of not at /text; reading the key "text" of Typed)",
                           "/text");
    expect_rejected<Typed>(
        R"({"point":{"x":1,"y":2},"small":{"a":256}})",
        R"(the value does not satisfy matches at /small/a; reading an element of the value at the key "small" of Typed)",
        "/small/a");

    // yyjson::reject_unknown_keys of the read around yyjson::matches closes T; other policies do not change it
    constexpr auto unknown = R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"z":3}],"ids":[],"point":{"x":1,"y":2,"z":3},)"
                             R"("text":null,"small":{}})";
    EXPECT_EQ(
        what_without_position(bad_cast_of([&] { (void)yyjson::read<Typed>(unknown, yyjson::reject_unknown_keys); })),
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)");
    EXPECT_NO_THROW((void)yyjson::read<Typed>(unknown, yyjson::strict));
    constexpr auto point = R"({"point":{"x":1,"y":2,"z":3}})";
    EXPECT_NO_THROW((void)yyjson::read<Typed>(point));
    EXPECT_EQ(
        what_without_position(bad_cast_of([&] { (void)yyjson::read<Typed>(point, yyjson::reject_unknown_keys); })),
        R"(the value does not satisfy matches at /point; reading the key "point" of Typed)");
}

TEST(Constraints, MatchesFunction)
{
    const auto matches = [](auto m, const char* json) { return m(yyjson::value(yyjson::read(json))); };
    EXPECT_TRUE(matches(yyjson::matches<int>, "1"));
    EXPECT_FALSE(matches(yyjson::matches<int>, "1.5"));
    EXPECT_FALSE(matches(yyjson::matches<std::uint8_t>, "300"));
    EXPECT_TRUE(matches(yyjson::matches<Point>, R"({"x":1,"y":2,"z":3})"));
    EXPECT_FALSE(matches(yyjson::matches<Point>, R"({"x":1})"));
    EXPECT_TRUE(matches(yyjson::matches<std::optional<int>>, "null"));
    EXPECT_TRUE(yyjson::matches<std::vector<int>>(yyjson::read("[1,2]")));
    EXPECT_FALSE(yyjson::matches<std::vector<int>>(*yyjson::read("[1,true]").as_array()));

    // the kind of the value decides the types that read each value of their kinds, as a read under strict does
    const auto read_strictly = []<typename T>(std::type_identity<T>, const char* json) {
        try
        {
            (void)yyjson::read<std::tuple<T>>(std::format("[{}]", json), yyjson::strict);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    };
    const auto agree = [&]<typename T>(std::type_identity<T> type) {
        for (const auto* json : {"null", "true", "0", "-1", "1.5", "1e300", R"("a")", "[1]", R"({"a":1})"})
        {
            EXPECT_EQ(yyjson::matches<T>(yyjson::value(yyjson::read(json))), read_strictly(type, json))
                << json << " " << yyjson::detail::type_name<T>();
        }
    };
    agree(std::type_identity<bool>());
    agree(std::type_identity<std::nullptr_t>());
    agree(std::type_identity<double>());
    agree(std::type_identity<std::string>());
    agree(std::type_identity<yyjson::value>());
    agree(std::type_identity<yyjson::array>());
    agree(std::type_identity<yyjson::object>());
    agree(std::type_identity<int>());
    agree(std::type_identity<float>());
}

TEST(Constraints, CombinatorsMerge)
{
    // merge_into checks the values of the patch, and a null patch resets the field without a check; the fields
    // that the patch leaves out are checked as they are written
    auto start = Negated();
    EXPECT_THROW(yyjson::merge_into(R"({"n":9})", start), yyjson::bad_cast);
    auto merged = Negated{.n = 5, .row = {1, "y"}, .c = {-1}, .o = 1, .nested = {{}}};
    yyjson::merge_into(R"({"n":9,"o":null})", merged);
    EXPECT_EQ(merged.n, 9);
    EXPECT_FALSE(merged.o.has_value());
    EXPECT_THROW(yyjson::merge_into(R"({"n":11})", merged), yyjson::bad_cast);

    expect_accepted<VisitedCombined>(R"({"n":1,"v":[4,8]})", {1, {4, 8}});
    expect_rejected<VisitedCombined>(
        R"({"n":0})", R"(the value satisfies the subschema of not at /n; reading the key "n" of VisitedCombined)",
        "/n");
    expect_rejected<VisitedCombined>(
        R"({"n":1,"v":[6]})",
        R"(the value satisfies more than one subschema of oneOf at /v/0; reading an element of the value at the key "v" of VisitedCombined)",
        "/v/0");
}

TEST(Constraints, CombinatorsDifferential)
{
    // the reader without a document reads what the document gives, and leaves a rejected value to it
    const auto negated = [](const char* json) { return expect_direct_read<Negated>(json); };
    EXPECT_TRUE(negated(R"({"n":9,"v":[1],"m":{"a":1},"row":[1,"y"],"holes":[1]})"));
    for (const auto* json : {R"({"n":10})", R"({"v":[-1]})", R"({"m":{"_":1}})", R"({"holes":[null]})"})
        (void)negated(json);
    const auto conditional = [](const char* json) { return expect_direct_read<Conditional>(json); };
    EXPECT_TRUE(conditional(R"({"n":12,"e":[0,5]})"));
    (void)conditional(R"({"e":[1]})");
    const auto composed = [](const char* json) { return expect_direct_read<Composed>(json); };
    EXPECT_TRUE(composed(R"({"c":2,"v":[0,11],"s":"b"})"));
    for (const auto* json : {R"({"c":6})", R"({"v":[1]})", R"({"m":{"k":3}})"}) (void)composed(json);
    const auto typed = [](const char* json) { return expect_direct_read<Typed>(json); };
    EXPECT_TRUE(typed(R"({"events":[{"x":1,"y":2},{"x":1,"y":2}],"point":{"x":0,"y":0}})"));
    (void)typed(R"({"events":[{"x":1,"y":2},{"x":1,"y":2}],"point":{"x":0}})");
}

// NOLINTEND
