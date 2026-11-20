#include "test_constraints_common.hpp"

// NOLINTBEGIN

// a merge into a variant with `internal_tag` checks the members of the patch at each depth
struct C5Named
{
    ScLevel e = ScLevel::zero;
    bool operator==(const C5Named&) const = default;
};
struct C5Counted
{
    int k = 0;
    ScLevel f = ScLevel::zero;
    bool operator==(const C5Counted&) const = default;
};
using C5Tagged = std::variant<C5Named, C5Counted>;
template <>
inline constexpr auto yyjson::annotations<C5Tagged> = yyjson::annotate(yyjson::internal_tag("type", {"i", "j"}));
struct C5TaggedHolder
{
    C5Tagged in = C5Counted{};
    bool operator==(const C5TaggedHolder&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C5TaggedHolder> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
// an enumeration with a caster that reads and writes a JSON object
enum class C5Shape
{
    a,
    b
};
template <>
struct yyjson::caster<C5Shape>
{
    static C5Shape from_json(const std::map<std::string, int>& m) { return m.size() > 1 ? C5Shape::b : C5Shape::a; }
    static std::map<std::string, int> to_json(const C5Shape& s)
    {
        return s == C5Shape::a ? std::map<std::string, int>{{"a", 1}} : std::map<std::string, int>{{"a", 1}, {"b", 1}};
    }
};
template <>
inline constexpr auto yyjson::annotations<C5Shape> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::property_names(yyjson::max_length(1)));
enum class C5Closed
{
    a
};
template <>
struct yyjson::caster<C5Closed>
{
    static C5Closed from_json(const std::map<std::string, int>&) { return C5Closed::a; }
    static std::map<std::string, int> to_json(const C5Closed&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<C5Closed> = yyjson::annotate(yyjson::additional_properties(false));
// the elements that a caster only writes, and a set of elements that do not read back as written
struct C5Text
{
    std::string s;
    auto operator<=>(const C5Text&) const = default;
};
template <>
struct yyjson::caster<C5Text>
{
    static std::string to_json(const C5Text& t) { return "!" + t.s; }
};
template <>
struct std::hash<C5Text>
{
    std::size_t operator()(const C5Text& t) const noexcept { return std::hash<std::string>()(t.s); }
};
inline bool c5_allowed(const C5Text& t) { return t.s != "no"; }
struct C5Texts
{
    std::vector<C5Text> v;
    std::set<C5Text> s;
    std::unordered_set<C5Text> u;
};
template <>
inline constexpr auto yyjson::annotations<C5Texts> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)),
                     yyjson::field("s", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)),
                     yyjson::field("u", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)));
struct C5Word
{
    std::string s;
    auto operator<=>(const C5Word&) const = default;
};
template <>
struct yyjson::caster<C5Word>
{
    static C5Word from_json(const std::string& s)
    {
        if (s.starts_with("!")) throw std::runtime_error("not readable");
        return {s};
    }
    static std::string to_json(const C5Word& w) { return "!" + w.s; }
};
inline bool c5_word_allowed(const C5Word& w) { return w.s != "no"; }
struct C5Words
{
    std::set<C5Word> v;
    bool operator==(const C5Words&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C5Words> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_word_allowed>)));

TEST(Constraints, MergeTaggedVariant)
{
    const auto merged = [](std::string_view patch, C5TaggedHolder h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_THROW((void)yyjson::read<C5TaggedHolder>(R"({"in":{"type":"i","e":1}})"), yyjson::bad_cast);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"i","e":1}})", {}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"i"}})", {}), C5TaggedHolder{C5Named{}});
    // the alternative that the holder keeps takes the patch at its members
    EXPECT_THROW((void)merged(R"({"in":{"f":1}})", {}), yyjson::bad_cast);
    const auto kept = C5TaggedHolder{C5Counted{0, ScLevel::one}};
    EXPECT_EQ(merged(R"({"in":{"k":0}})", kept), kept);
    EXPECT_THROW((void)merged(R"({"in":{"type":"j","f":1}})", {}), yyjson::bad_cast);
}

TEST(Constraints, CasterEnumObjectKeywords)
{
    expect_accepted<C5Shape>(R"({"a":1,"b":1})", C5Shape::b);
    for (const auto& e : {bad_cast_of([] { (void)yyjson::read<C5Shape>(R"({"a":1})"); }),
                          bad_cast_of([] { (void)yyjson::read(R"({"a":1})").cast<C5Shape>(); }), bad_cast_of([] {
                              auto x = C5Shape::b;
                              yyjson::merge_into(R"({"a":1})", x);
                          })})
        EXPECT_EQ(what_without_position(e), "the object has fewer properties than the minimum 2; reading C5Shape");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C5Shape>(R"({"a":1,"bb":1})"); }).path(), "/bb");
    EXPECT_NO_THROW(yyjson::validate(C5Shape::b));
    EXPECT_THROW(yyjson::validate(C5Shape::a), yyjson::bad_cast);

    EXPECT_NO_THROW((void)yyjson::read<C5Closed>("{}"));
    EXPECT_THROW((void)yyjson::read<C5Closed>(R"({"a":1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"a":1})").cast<C5Closed>(), yyjson::bad_cast);
}

TEST(Constraints, WrittenCasterElementsOfSets)
{
    EXPECT_NO_THROW(yyjson::validate(C5Texts{{{"yes"}}, {{"yes"}}, {{"a"}, {"b"}, {"yes"}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{{"no"}}, {}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{}, {{"a"}, {"no"}}, {}}); }).path(), "/s/1");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{}, {{"a"}, {"long"}}, {}}); }).path(), "/s/1");
    // the path is the position of the element in the written array
    const auto u = C5Texts{{}, {}, {{"a"}, {"no"}, {"b"}}};
    const auto at = std::ranges::distance(u.u.begin(), std::ranges::find(u.u, C5Text{"no"}));
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(u); }).path(), "/u/" + std::to_string(at));
    EXPECT_THROW((void)yyjson::write(u, yyjson::with_validation), yyjson::bad_cast);

    expect_accepted<C5Words>(R"({"v":["yes"]})", {{{"yes"}}});
    expect_rejected<C5Words>(
        R"({"v":["no"]})",
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C5Words)",
        "/v/0");
    EXPECT_NO_THROW(yyjson::validate(C5Words{{{"yes"}}}));
    EXPECT_EQ(
        message_of(bad_cast_of([] { yyjson::validate(C5Words{{{"no"}}}); })),
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C5Words)");
    EXPECT_EQ(
        message_of(bad_cast_of([] { yyjson::validate(C5Words{{{"long"}}}); })),
        R"(the string is longer than the maximum length 4 at /v/0; reading an element of the value at the key "v" of C5Words)");
}

// the elements of a vector<bool> and the values of a map, which the check copies
struct C6Flags
{
    std::vector<bool> v;
    std::map<std::string, bool> m;
};
template <>
inline constexpr auto yyjson::annotations<C6Flags> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::const_(true))),
                     yyjson::field("m", yyjson::additional_properties(yyjson::const_(true))));
// a read inside validate checks its own input
struct C6Letter
{
    std::string s;
    auto operator<=>(const C6Letter&) const = default;
};
template <>
struct yyjson::caster<C6Letter>
{
    static C6Letter from_json(const std::string& s) { return {s}; }
    static std::string to_json(const C6Letter& c) { return c.s; }
};
inline bool c6_is_a(const C6Letter& c) { return c.s == "a"; }
struct C6Letters
{
    std::set<C6Letter> v;
};
template <>
inline constexpr auto yyjson::annotations<C6Letters> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::max_length(1), yyjson::satisfies<c6_is_a>))));
inline bool c6_reads_inside(int)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read(R"({"v":["long","a"]})").cast<C6Letters>(); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C6Letters>(R"({"v":["long","a"]})"); }).path(), "/v/0");
    return true;
}
struct C6Outer
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<C6Outer> =
    yyjson::annotate(yyjson::field("x", yyjson::satisfies<c6_reads_inside>));
// casters whose written value differs from the value that they read
struct C6Step
{
    int n = 0;
    bool operator==(const C6Step&) const = default;
};
template <>
struct yyjson::caster<C6Step>
{
    static C6Step from_json(int n) { return {n}; }
    static int to_json(const C6Step& c) { return c.n + 1; }
};
template <>
inline constexpr auto yyjson::annotations<C6Step> = yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
enum class C6Level
{
    a,
    b
};
template <>
struct yyjson::caster<C6Level>
{
    static C6Level from_json(int n) { return static_cast<C6Level>(n); }
    static int to_json(C6Level x) { return static_cast<int>(x) + 1; }
};
template <>
inline constexpr auto yyjson::annotations<C6Level> = yyjson::annotate(yyjson::all_of(yyjson::maximum(1)));
struct C6Kept
{
    C6Step c;
    C6Level x = C6Level::a;
    int n = 0;
    bool operator==(const C6Kept&) const = default;
};
// a patch that switches the alternative gives a new one, read from the patch alone
struct C6First
{
    int a = 0;
    bool operator==(const C6First&) const = default;
};
struct C6Second
{
    std::optional<int> k = 1;
    bool operator==(const C6Second&) const = default;
};
using C6Choice = std::variant<C6First, C6Second>;
template <>
inline constexpr auto yyjson::annotations<C6Choice> = yyjson::annotate(yyjson::internal_tag("type", {"a", "b"}));
struct C6Switch
{
    C6Choice in;
    bool operator==(const C6Switch&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C6Switch> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
struct C6Counted
{
    C6Choice in;
    bool operator==(const C6Counted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C6Counted> =
    yyjson::annotate(yyjson::additional_properties(yyjson::min_properties(2)));

TEST(Constraints, WrittenProxyElements)
{
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{false}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{true, false}, {}}); }).path(), "/v/1");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{}, {{"a", false}}}); }).path(), "/m/a");
    EXPECT_NO_THROW(yyjson::validate(C6Flags{{true, true}, {{"a", true}}}));
}

TEST(Constraints, ReadInsideValidate)
{
    EXPECT_NO_THROW(yyjson::validate(C6Outer{}));
    EXPECT_NO_THROW((void)yyjson::write(C6Outer{}, yyjson::with_validation));
}

TEST(Constraints, MergeKeepsCasterValues)
{
    const auto merged = [](std::string_view patch, C6Kept k) {
        yyjson::merge_into(patch, k);
        return k;
    };
    const auto kept = yyjson::read<C6Kept>(R"({"c":0,"x":0,"n":0})");
    EXPECT_EQ(yyjson::write(kept), R"({"c":1,"x":1,"n":0})");
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(kept); }).path(), "/c");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", kept); }).path(), "/c");
    // the patch gives the value that the check takes
    EXPECT_EQ(merged(R"({"c":0,"n":1})", kept).n, 1);
    EXPECT_EQ(merged(R"({"c":-1,"x":0})", {}).n, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"c":1})", {}); }).path(), "/c");
    const auto level = C6Kept{{-1}, C6Level::b, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", level); }).path(), "/x");
    EXPECT_EQ(merged(R"({"x":1})", level).x, C6Level::b);
}

TEST(Constraints, MergeSwitchesAlternative)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_EQ(yyjson::write(yyjson::read<C6Switch>(R"({"in":{"type":"b"}})")), R"({"in":{"type":"b","k":1}})");
    EXPECT_EQ(merged(R"({"in":{"type":"b"}})", C6Switch{}), C6Switch{C6Second{}});
    EXPECT_EQ(merged(R"({"in":{"type":"b","k":0}})", C6Switch{}), C6Switch{C6Second{0}});
    EXPECT_THROW((void)merged(R"({"in":{"type":"b","k":1}})", C6Switch{}), yyjson::bad_cast);
    // the alternative that the patch names is kept, and its written members count
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"b"}})", C6Switch{C6Second{}}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"a"}})", C6Switch{C6Second{}}), C6Switch{});

    EXPECT_THROW((void)yyjson::read<C6Counted>(R"({"in":{"type":"b"}})"), yyjson::bad_cast);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"b"}})", C6Counted{}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"b"}})", C6Counted{C6Second{}}), C6Counted{C6Second{}});
}

struct C7Pair
{
    int x = 0;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Pair> = yyjson::annotate(yyjson::field("x", yyjson::maximum(0)));
struct C7Nested
{
    C7Pair in;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::map<std::string, C7Pair> pairs;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Nested> = yyjson::annotate(
    yyjson::field("v", yyjson::max_items(1)), yyjson::field("m", yyjson::additional_properties(yyjson::maximum(0))));
struct C7Other
{
    std::optional<int> k = 1;
    bool operator==(const C7Other&) const = default;
};
struct C7Adjacent
{
    std::variant<C7Pair, C7Other> v;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Adjacent> =
    yyjson::annotate(yyjson::field("v", yyjson::adjacent_tag("kind", {"a", "b"})));
enum class C7Key
{
    x,
    y
};
struct C7EnumKeys
{
    std::map<C7Key, C7Pair> m;
    std::map<C7Key, int> counts;
};
template <>
inline constexpr auto yyjson::annotations<C7EnumKeys> =
    yyjson::annotate(yyjson::field("counts", yyjson::additional_properties(yyjson::maximum(0))));

TEST(Constraints, MergeChecksKeptFields)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C7Pair>(R"({"x":1,"n":1})"); }).path(), "/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", C7Pair{1, 0}); }).path(), "/x");
    EXPECT_EQ(merged(R"({"x":0})", C7Pair{1, 0}).x, 0);

    const auto bad_in = C7Nested{{1, 0}, {}, {}, {}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", bad_in); }).path(), "/in/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"n":1}})", bad_in); }).path(), "/in/x");
    EXPECT_EQ(merged(R"({"in":{"x":0}})", bad_in).in.x, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", C7Nested{{}, {1, 2}, {}, {}, 0}); }).path(), "/v");
    const auto bad_m = C7Nested{{}, {}, {{"a", 1}}, {}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"b":0}})", bad_m); }).path(), "/m/a");
    EXPECT_EQ(merged(R"({"m":{"a":0}})", bad_m).m.at("a"), 0);
    const auto bad_pairs = C7Nested{{}, {}, {}, {{"a", {1, 0}}}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"pairs":{"b":{}}})", bad_pairs); }).path(), "/pairs/a/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"pairs":{"a":{"n":1}}})", bad_pairs); }).path(), "/pairs/a/x");
}

TEST(Constraints, MergeChecksKeptAdjacentAlternative)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto bad = C7Adjacent{C7Pair{1, 0}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", bad); }).path(), "/v/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"v":{"n":1}})", bad); }).path(), "/v/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"kind":"a","v":{"n":1}})", bad); }).path(), "/v/x");
    EXPECT_EQ(std::get<C7Pair>(merged(R"({"v":{"x":0}})", bad).v).x, 0);
    // a tag that names another alternative reads it from the patch alone
    EXPECT_EQ(std::get<C7Other>(merged(R"({"kind":"b","v":{}})", bad).v), C7Other{});
}

TEST(Constraints, MergeChecksKeptEnumKeys)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto bad = C7EnumKeys{{{C7Key::x, {1, 0}}}, {}};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"y":{}}})", bad); }).path(), "/m/x/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"x":{"n":1}}})", bad); }).path(), "/m/x/x");
    EXPECT_EQ(merged(R"({"m":{"x":{"x":0}}})", bad).m.at(C7Key::x).x, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"counts":{"y":0}})", C7EnumKeys{{}, {{C7Key::x, 1}}}); }).path(),
              "/counts/x");
}

// a struct whose map of the other keys merges objects into its values
struct C7Spot
{
    int x = 0;
    std::optional<int> y;
};
struct C7Extra
{
    int a = 0;
    std::map<std::string, C7Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C7Extra> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("a"),
                     yyjson::additional_properties(yyjson::min_properties(2)));

TEST(Constraints, MergeChecksExtraKeyObjects)
{
    // the object that a patch merges into a value of the map is checked as merged
    auto t = C7Extra{0, {{"r", {1, 2}}}};
    yyjson::merge_into(R"({"r":{"x":3}})", t);
    EXPECT_EQ(written(t), R"({"a":0,"r":{"x":3,"y":2}})");
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"r":{"y":null}})", t); }).path(), "/r");
}

// a field that yyjson::adjacent_tag tags, whose content a property of the struct around it checks
struct C7Two
{
    std::optional<int> a;
    std::optional<int> b;
};
struct C7AdjacentTwo
{
    std::variant<C7Two, C7Other> v;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7AdjacentTwo> =
    yyjson::annotate(yyjson::field("v", yyjson::adjacent_tag("kind", {"a", "b"})));
struct C7AdjacentOuter
{
    C7AdjacentTwo h;
};
template <>
inline constexpr auto yyjson::annotations<C7AdjacentOuter> =
    yyjson::annotate(yyjson::field("h", yyjson::property("v", yyjson::max_properties(1))));

TEST(Constraints, MergeChecksMergedAdjacentContent)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto t = C7AdjacentOuter{{C7Two{1, {}}, 0}};
    EXPECT_EQ(
        bad_cast_of([] { (void)yyjson::read<C7AdjacentOuter>(R"({"h":{"kind":"a","v":{"a":1,"b":2}}})"); }).path(),
        "/h");
    // the content is checked as merged: {"a":1,"b":2}, then {"b":2}
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"h":{"v":{"b":2}}})", t); }).path(), "/h");
    EXPECT_EQ(written(merged(R"({"h":{"v":{"a":null,"b":2}}})", t)), R"({"h":{"kind":"a","v":{"b":2},"n":0}})");
    EXPECT_EQ(written(merged(R"({"h":{"kind":"b","v":{"k":2}}})", t)), R"({"h":{"kind":"b","v":{"k":2},"n":0}})");
}

// a field whose type has no JSON to write: the merge does not check the value that it keeps
struct C7ReadOnly
{
    int n = 0;
};
template <>
struct yyjson::caster<C7ReadOnly>
{
    static C7ReadOnly from_json(int n) { return {n}; }
};
struct C7Unwritable
{
    C7ReadOnly r;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Unwritable> = yyjson::annotate(yyjson::field("r", yyjson::minimum(0)));

TEST(Constraints, MergeKeepsUnwritableValues)
{
    EXPECT_THROW((void)yyjson::read<C7Unwritable>(R"({"r":-1,"n":0})"), yyjson::bad_cast);
    auto t = C7Unwritable{{-1}, 0};
    yyjson::merge_into(R"({"n":1})", t);
    EXPECT_EQ(t.r.n, -1);
    EXPECT_EQ(t.n, 1);
    EXPECT_THROW(yyjson::merge_into(R"({"r":-2})", t), yyjson::bad_cast);
}

// a struct with keywords on the keys of its object only, inside another struct
struct C9Pairless
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Pairless> = yyjson::annotate(yyjson::min_properties(2));
struct C9HoldsPairless
{
    C9Pairless in;
    int n = 0;
};
// a struct whose keys `yyjson::property_names` checks, and a keyword on another field
struct C9Names
{
    int longname = 0;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Names> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(1)), yyjson::field("n", yyjson::maximum(0)));
// maps of the other keys whose values the patch creates
struct C9Spot
{
    int x = 0;
};
struct C9Rest
{
    std::map<std::string, C9Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C9Rest> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::additional_properties(yyjson::min_properties(1)));
struct C9Pattern
{
    std::map<std::string, C9Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C9Pattern> = yyjson::annotate(
    yyjson::field("rest", yyjson::pattern_properties("^r")), yyjson::additional_properties(yyjson::min_properties(1)));
// a struct whose other keys must hold objects with a member
struct C9Closed
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Closed> =
    yyjson::annotate(yyjson::additional_properties(yyjson::min_properties(1)));
// the types of the differential test
struct C9Inner
{
    int x = 0;
    std::optional<int> y;
};
template <>
inline constexpr auto yyjson::annotations<C9Inner> =
    yyjson::annotate(yyjson::field("x", yyjson::maximum(5)), yyjson::max_properties(2));
struct C9Other
{
    int k = 0;
};
using C9Internal = std::variant<C9Inner, C9Other>;
template <>
inline constexpr auto yyjson::annotations<C9Internal> = yyjson::annotate(yyjson::internal_tag("type", {"i", "o"}));
struct C9Target
{
    C9Inner in;
    std::map<std::string, int> m;
    std::vector<int> v;
    std::optional<int> o;
    C9Internal u;
    std::variant<C9Inner, C9Other> a;
    std::map<std::string, C9Inner> rest;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Target> = yyjson::annotate(
    yyjson::field("m", yyjson::max_properties(2), yyjson::additional_properties(yyjson::maximum(3))),
    yyjson::field("v", yyjson::max_items(2)), yyjson::field("o", yyjson::maximum(1)),
    yyjson::field("u", yyjson::max_properties(2)), yyjson::field("a", yyjson::adjacent_tag("kind", {"i", "o"})),
    yyjson::field("rest", yyjson::extra_keys), yyjson::dependent_required("o", {"r"}), yyjson::max_properties(9));

TEST(Constraints, MergeChecksObjectKeywordsOfKeptStructs)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9HoldsPairless>(R"({"in":{"x":0},"n":1})"); }).path(), "/in");
    auto h = C9HoldsPairless{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":1})", h); }).path(), "/in");
    EXPECT_EQ(h.n, 0);
}

TEST(Constraints, MergeChecksAllKeyNames)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Names>(R"({"longname":0,"n":0})"); }).path(), "/longname");
    auto t = C9Names{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":0})", t); }).path(), "/longname");
}

TEST(Constraints, MergeChecksNewMapValues)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Rest>(R"({"a":{}})"); }).path(), "/a");
    auto rest = C9Rest{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"a":{}})", rest); }).path(), "/a");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Pattern>(R"({"r":{}})"); }).path(), "/r");
    auto pattern = C9Pattern{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"r":{}})", pattern); }).path(), "/r");
    EXPECT_TRUE(pattern.rest.empty());
}

TEST(Constraints, MergeRemovesNullMembersOfOtherKeys)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Closed>(R"({"n":0,"extra":{}})"); }).path(), "/extra");
    auto t = C9Closed{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"extra":{"x":null}})", t); }).path(), "/extra");
}

// merge_into rejects a patch if and only if read rejects the object that the patch merges into the written target
TEST(Constraints, MergeChecksAsReadOfMergedObject)
{
    const auto path_of = [](auto&& f) -> std::optional<std::string> {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::string(e.path());
        }
        return std::nullopt;
    };
    const auto expect_as_read = [&](const C9Target& target, std::string_view patch, std::string_view merged,
                                    bool kept = true) {
        const auto read = path_of([&] { (void)yyjson::read<C9Target>(merged); });
        auto t = target;
        const auto merge = path_of([&] { yyjson::merge_into(patch, t); });
        EXPECT_EQ(merge, read) << patch;
        if (merge.has_value() && kept)
        {
            EXPECT_EQ(written(t), written(target)) << patch;
        }
    };
    auto t = C9Target{};
    t.m = {{"a", 1}};
    t.v = {1};
    t.o = 1;
    t.rest = {{"r", {}}};
    ASSERT_EQ(written(t), R"({"in":{"x":0},"m":{"a":1},"v":[1],"o":1,"u":{"type":"i","x":0},"kind":"i","a":{"x":0},)"
                          R"("n":0,"r":{"x":0}})");
    const auto with = [](std::string_view in, std::string_view m, std::string_view v, std::string_view o,
                         std::string_view u, std::string_view a, std::string_view n, std::string_view rest) {
        return std::format(R"({{"in":{},"m":{},"v":{}{},"u":{},{},"n":{}{}}})", in, m, v, o, u, a, n, rest);
    };
    const auto base = [&](std::string_view n) {
        return with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                    R"("kind":"i","a":{"x":0})", n, R"(,"r":{"x":0})");
    };
    // nested structs and optional fields
    expect_as_read(t, R"({"n":1})", base("1"));
    expect_as_read(t, R"({"in":{"x":6}})",
                   with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"in":{"y":1}})",
                   with(R"({"x":0,"y":1})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    auto kept_y = t;
    kept_y.in.y = 1;
    const auto with_y = with(R"({"x":0,"y":1})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                             R"("kind":"i","a":{"x":0})", "1", R"(,"r":{"x":0})");
    expect_as_read(kept_y, R"({"n":1})", with_y);
    expect_as_read(kept_y, R"({"in":{"y":null}})", base("0"));
    expect_as_read(t, R"({"o":2})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":2)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"o":null})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", "", R"({"type":"i","x":0})", R"("kind":"i","a":{"x":0})",
                        "0", R"(,"r":{"x":0})"));
    // maps, arrays, and null removal
    expect_as_read(t, R"({"m":{"b":2}})",
                   with(R"({"x":0})", R"({"a":1,"b":2})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":2,"c":3}})",
                   with(R"({"x":0})", R"({"a":1,"b":2,"c":3})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"a":null,"b":2,"c":3}})",
                   with(R"({"x":0})", R"({"b":2,"c":3})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":4}})",
                   with(R"({"x":0})", R"({"a":1,"b":4})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"v":[1,2,3]})",
                   with(R"({"x":0})", R"({"a":1})", "[1,2,3]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"v":[5]})",
                   with(R"({"x":0})", R"({"a":1})", "[5]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    // the members of `yyjson::extra_keys`, and the dependent key that a null removes
    expect_as_read(t, R"({"s":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0},"s":{"x":6})"));
    expect_as_read(t, R"({"r":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0,"y":1})"));
    expect_as_read(t, R"({"r":null})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", ""));
    expect_as_read(t, R"({"zz":null})", base("0"));
    // the internal tag and the adjacent tag: a tag that names another alternative takes the patch alone
    expect_as_read(t, R"({"u":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":6})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"u":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0,"y":1})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"u":{"type":"o","k":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"o","k":1})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"a":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":6})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"a":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0,"y":1})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"kind":"o","a":{"k":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"o","a":{"k":1})", "0", R"(,"r":{"x":0})"));
    // repeated keys apply in their order, and the last one gives the value
    expect_as_read(t, R"({"n":1,"n":2})", base("2"));
    expect_as_read(t, R"({"n":2,"n":1})", base("1"));
    expect_as_read(t, R"({"m":{"b":1,"b":5}})",
                   with(R"({"x":0})", R"({"a":1,"b":5})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":5,"b":1}})",
                   with(R"({"x":0})", R"({"a":1,"b":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    // the merge checks each value of a repeated key as it applies it, as read checks each, after the values before
    const auto first_in = with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                               R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})");
    expect_as_read(t, R"({"in":{"x":6},"in":{"x":0}})", first_in, false);
    expect_as_read(t, R"({"in":{"x":6,"x":0}})", first_in, false);
    expect_as_read(t, R"({"o":2,"o":1})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":2)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"),
                   false);
    expect_as_read(t, R"({"in":{"x":0,"x":6}})",
                   with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
}

struct C10Count
{
    int n = 0;
};
inline int c10_reads = 0;
template <>
struct yyjson::caster<C10Count>
{
    static C10Count from_json(int n)
    {
        ++c10_reads;
        return {n};
    }
    static int to_json(const C10Count& c) { return c.n + 1; }
};
inline bool c10_is_five(const C10Count& c) { return c.n == 5; }
struct C10Holder
{
    C10Count c;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C10Holder> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<c10_is_five>), yyjson::field("n", yyjson::maximum(1)));

struct C10Outer
{
    C10Holder in;
    int m = 0;
};

// the merge checks a kept value on the C++ value, and reads only the values of the patch
TEST(Constraints, MergeChecksKeptValuesWithoutReadingThem)
{
    auto h = C10Holder{{5}, 0};
    c10_reads = 0;
    yyjson::merge_into(R"({"n":1})", h);
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(h.c.n, 5);
    EXPECT_EQ(h.n, 1);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":2})", h); }).path(), "/n");
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C10Holder>(R"({"c":7,"n":0})"); }).path(), "/c");
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"c":7})", h); }).path(), "/c");
    EXPECT_EQ(h.c.n, 5);
    yyjson::merge_into(R"({"c":5,"n":0})", h);
    EXPECT_EQ(h.c.n, 5);
    EXPECT_EQ(h.n, 0);
    auto outer = C10Outer{{{5}, 0}, 0};
    c10_reads = 0;
    yyjson::merge_into(R"({"in":{"n":1}})", outer);
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(outer.in.n, 1);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"in":{"n":2}})", outer); }).path(), "/in/n");
}

struct C10Pair
{
    int a = 0;
    int b = 0;
};
inline bool c10_sum_small(const C10Pair& p) { return p.a + p.b <= 1; }
struct C10Typed
{
    C10Pair in;
    std::map<std::string, C10Pair> rest;
};
template <>
inline constexpr auto yyjson::annotations<C10Typed> = yyjson::annotate(
    yyjson::field("in", yyjson::satisfies<c10_sum_small>),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::satisfies<c10_sum_small>)));

// a predicate that takes a C++ value checks a value that the patch merges into after the merge
TEST(Constraints, MergeChecksTypedPredicatesAfterTheMerge)
{
    const auto target = C10Typed{{1, 0}, {{"k", {1, 0}}}};
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C10Typed>(R"({"in":{"a":1,"b":1}})"); }).path(), "/in");
    auto t = target;
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"in":{"b":1}})", t); }).path(), "/in");
    t = target;
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"k":{"b":1}})", t); }).path(), "/k");
    t = target;
    yyjson::merge_into(R"({"in":{"a":0,"b":1},"k":{"a":0}})", t);
    EXPECT_EQ(t.in.b, 1);
    EXPECT_EQ(t.rest.at("k").a, 0);
}

// a merge walks a member that no field takes without recursion, at any depth of the patch
struct R11Deep
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Deep> = yyjson::annotate(yyjson::field("n", yyjson::maximum(1)));

TEST(Constraints, MergeWalksADeepPatchWithoutRecursion)
{
    constexpr auto depth = 200000;
    auto patch = std::string(R"({"n":0,"x":null,"extra":)");
    for (auto i = 0; i < depth; ++i) patch += R"({"a":null,"b":)";
    patch += '0';
    patch.append(depth, '}');
    patch += '}';
    auto t = R11Deep{1};
    yyjson::merge_into(patch, t);
    EXPECT_EQ(t.n, 0);
}

TEST(Constraints, MergeChecksAFieldPropertyOnTheMergedMember)
{
    expect_rejected<R11Property>(R"({"f":{"x":1}})",
                                 R"(the object does not satisfy properties at /f; reading the key "f" of R11Property)",
                                 "/f", false);
    auto t = R11Property{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", t); }).path(), "/f");
    t = R11Property{{1, 0}};
    yyjson::merge_into(R"({"f":{"x":0}})", t);
    EXPECT_EQ(t.f.x, 0);
}

TEST(Constraints, MergeChecksEveryDeferredPredicate)
{
    auto c = R11Condition{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", c); }).path(), "/f");
    auto k = R11Kept{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":0}})", k); }).path(), "/k");
    auto w = R11Wrapped{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"w":{"x":1}})", w); }).path(), "/w");
    yyjson::merge_into(R"({"f":{"x":-1}})", c);
    EXPECT_EQ(c.f.x, -1);
}

// NOLINTEND
