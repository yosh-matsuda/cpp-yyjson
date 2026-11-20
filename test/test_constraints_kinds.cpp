#include "test_constraints_common.hpp"

// NOLINTBEGIN

// the keywords check the JSON value at their position, and those of another JSON kind check nothing there
enum class KjColor
{
    red,
    green
};
struct KjRect
{
    int w = 0;
    int h = 0;
    bool operator==(const KjRect&) const = default;
};
struct KjCircle
{
    double r = 0;
    bool operator==(const KjCircle&) const = default;
};
using KjShape = std::variant<KjCircle, KjRect>;
template <>
inline constexpr auto yyjson::annotations<KjShape> = yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct KjObjects
{
    KjRect r;
    KjShape s;
    bool operator==(const KjObjects&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjObjects> =
    yyjson::annotate(yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::max_properties(2)));
struct KjScalars
{
    std::variant<KjColor, std::string> v;
    KjColor c{};
    std::variant<int, std::string> w = std::string();
    bool operator==(const KjScalars&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjScalars> =
    yyjson::annotate(yyjson::field("v", yyjson::max_length(3)), yyjson::field("c", yyjson::max_length(3)),
                     yyjson::field("w", yyjson::not_(yyjson::min_length(1))));
struct KjArrays
{
    std::array<char, 3> a{};
    std::tuple<int, double, std::string> t = {0, 1, ""};
    std::set<int> s;
    bool operator==(const KjArrays&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjArrays> =
    yyjson::annotate(yyjson::field("a", yyjson::items(yyjson::maximum(100))),
                     yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::unique_items));
struct KjExtra
{
    int a = 0;
    std::map<std::string, int> rest;
    bool operator==(const KjExtra&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjExtra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
struct KjEvent
{
    int id = 0;
    std::variant<KjRect, int> data;
    bool operator==(const KjEvent&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));
// written as a string, and read as an object by its field
struct KjText
{
    std::string s;
    bool operator==(const KjText&) const = default;
};
template <>
struct yyjson::caster<KjText>
{
    static std::string to_json(const KjText& t) { return t.s; }
};
struct KjCasters
{
    std::vector<CasterName> names;
    std::map<std::string, CasterName> map;
    KjText text;
    bool operator==(const KjCasters&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjCasters> =
    yyjson::annotate(yyjson::field("names", yyjson::items(yyjson::max_length(2))),
                     yyjson::field("map", yyjson::additional_properties(yyjson::max_length(2))),
                     yyjson::field("text", yyjson::max_length(1)));

TEST(Constraints, JsonKindObjects)
{
    expect_accepted<KjObjects>(R"({"r":{"w":1,"h":2},"s":{"type":"circle","r":1}})", {{1, 2}, KjCircle{1}});
    // every member of the object of the struct satisfies the subschema, and the tag counts as a member
    expect_rejected<KjObjects>(
        R"({"r":{"w":1,"h":-2}})",
        R"(the object has a value that does not satisfy additionalProperties at /r; reading the key "r" of KjObjects)",
        "/r");
    expect_rejected<KjObjects>(
        R"({"s":{"type":"rect","w":1,"h":2}})",
        R"(the object has more properties than the maximum 2 at /s; reading the key "s" of KjObjects)", "/s");
}

TEST(Constraints, JsonKindScalars)
{
    expect_accepted<KjScalars>(R"({"v":"red","c":"red","w":""})", {KjColor::red, KjColor::red, std::string()});
    expect_rejected<KjScalars>(
        R"({"v":"green"})", R"(the string is longer than the maximum length 3 at /v; reading the key "v" of KjScalars)",
        "/v");
    expect_rejected<KjScalars>(
        R"({"c":"green"})", R"(the string is longer than the maximum length 3 at /c; reading the key "c" of KjScalars)",
        "/c");
    // `yyjson::min_length` holds for a number, so that `yyjson::not_` rejects it
    expect_rejected<KjScalars>(
        R"({"w":1})", R"(the value satisfies the subschema of not at /w; reading the key "w" of KjScalars)", "/w");
}

TEST(Constraints, JsonKindArrays)
{
    // the unique elements of a tuple of several types compare their JSON values
    expect_accepted<KjArrays>(R"({"a":[1,2,3],"t":[1,2,"x"],"s":[1,2]})", {{1, 2, 3}, {1, 2.0, "x"}, {1, 2}}, false);
    expect_rejected<KjArrays>(
        R"({"a":[1,2,101]})",
        R"(the number is greater than the maximum 100 at /a/2; reading an element of the value at the key "a" of KjArrays)",
        "/a/2");
    expect_rejected<KjArrays>(
        R"({"t":[-1,2,"x"]})",
        R"(the number is less than the minimum 0 at /t/0; reading an element of the value at the key "t" of KjArrays)",
        "/t/0");
    expect_rejected<KjArrays>(
        R"({"t":[1,1,"x"]})",
        R"(the item is equal to an earlier item at /t/1; reading an element of the value at the key "t" of KjArrays)",
        "/t/1");
    // the set drops a duplicate, but the keyword checks the JSON value
    expect_rejected<KjArrays>(
        R"({"s":[1,1]})",
        R"(the item is equal to an earlier item at /s/1; reading an element of the value at the key "s" of KjArrays)",
        "/s/1");
}

TEST(Constraints, JsonKindKeys)
{
    // the struct writes any key with the map of `yyjson::extra_keys`, and the key of an adjacent tag
    expect_accepted<KjExtra>(R"({"a":1,"b":5})", {1, {{"b", 5}}});
    expect_rejected<KjExtra>(R"({"a":1,"b":-5})",
                             R"(the number is less than the minimum 0 at /b; reading the key "b" of KjExtra)", "/b");
    expect_rejected<KjExtra>(R"({"a":1})", R"(a required key is missing; reading the key "b" of KjExtra)", "");
    expect_accepted<KjEvent>(R"({"id":1,"data":5,"kind":"count"})", {1, 5});
    expect_rejected<KjEvent>(
        R"({"id":1,"data":{"w":1,"h":2},"kind":"rect"})",
        R"(the value is not equal to the constant "count" at /kind; reading the key "kind" of KjEvent)", "/kind");
}

TEST(Constraints, JsonKindCasters)
{
    using N = CasterName;
    expect_accepted<KjCasters>(R"({"names":["a","bc"],"map":{"k":"ab"},"text":{"s":"long"}})",
                               {{N("a"), N("bc")}, {{"k", N("ab")}}, {"long"}});
    expect_rejected<KjCasters>(
        R"({"names":["a","bcd"]})",
        R"(the string is longer than the maximum length 2 at /names/1; reading an element of the value at the key "names" of KjCasters)",
        "/names/1");
    expect_rejected<KjCasters>(
        R"({"map":{"k":"abc"}})",
        R"(the string is longer than the maximum length 2 at /map/k; reading an element of the value at the key "map" of KjCasters)",
        "/map/k");
}

// `yyjson::additional_properties` on the type of a struct checks each member that `yyjson::property` does not name
struct KjTypeRest
{
    int a = 0;
    std::string b = "x";
    std::map<std::string, int> rest;
    bool operator==(const KjTypeRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjTypeRest> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::min_length(1)),
                     yyjson::additional_properties(yyjson::minimum(0)));
// the keywords at the position of an internally tagged variant name its tag
struct KjTagged
{
    KjShape s;
    bool operator==(const KjTagged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjTagged> =
    yyjson::annotate(yyjson::field("s", yyjson::property("type", yyjson::enum_("circle")), yyjson::required("type"),
                                   yyjson::if_then_else(yyjson::if_(yyjson::property("type", yyjson::const_("circle"))),
                                                        yyjson::then(yyjson::max_properties(2)))));
// a value that the extra keys read is written in another form
enum class KjLevel
{
    zero,
    one
};
template <>
inline constexpr auto yyjson::annotations<KjLevel> = yyjson::annotate(yyjson::enum_format::name_or_integer);
struct KjLevels
{
    int a = 0;
    std::map<std::string, KjLevel> rest;
    bool operator==(const KjLevels&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjLevels> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("z", yyjson::maximum(0)));
struct KjDomPrefix
{
    yyjson::value v;
};
template <>
inline constexpr auto yyjson::annotations<KjDomPrefix> =
    yyjson::annotate(yyjson::field("v", yyjson::prefix_items(yyjson::minimum(0))));

TEST(Constraints, JsonKindTypeRest)
{
    expect_accepted<KjTypeRest>(R"({"a":1,"b":"x","z":3})", {1, "x", {{"z", 3}}});
    expect_rejected<KjTypeRest>(
        R"({"a":-1,"b":"x"})",
        "the number is less than the minimum 0 at /a; reading a value of KjTypeRest in yyjson::additional_properties",
        "/a");
    expect_rejected<KjTypeRest>(
        R"({"a":1,"b":"x","z":-3})",
        "the number is less than the minimum 0 at /z; reading a value of KjTypeRest in yyjson::additional_properties",
        "/z");
    expect_rejected<KjTypeRest>(
        R"({"a":1,"b":""})",
        R"(the string is shorter than the minimum length 1 at /b; reading the key "b" of KjTypeRest)", "/b");
    EXPECT_NO_THROW(yyjson::validate(KjTypeRest{1, "x", {{"q", 2}}}));
    EXPECT_THROW(yyjson::validate(KjTypeRest{-1, "x", {}}), yyjson::bad_cast);
    EXPECT_THROW(yyjson::validate(KjTypeRest{1, "x", {{"q", -2}}}), yyjson::bad_cast);
}

TEST(Constraints, JsonKindTag)
{
    expect_accepted<KjTagged>(R"({"s":{"type":"circle","r":1}})", {KjCircle{1}});
    expect_rejected<KjTagged>(R"({"s":{"type":"rect","w":1}})",
                              R"(the object does not satisfy properties at /s; reading the key "s" of KjTagged)", "/s");
    // the read checks the input object, which can hold a key that the struct does not write
    expect_rejected<KjTagged>(R"({"s":{"type":"circle","r":1,"x":0}})",
                              R"(the value satisfies if and not then at /s; reading the key "s" of KjTagged)", "/s");
    EXPECT_NO_THROW(yyjson::validate(KjTagged{KjCircle{1}}));
    EXPECT_THROW(yyjson::validate(KjTagged{KjRect{}}), yyjson::bad_cast);
    // a merge checks the object as merged, with the tag of the alternative that it holds
    auto t = KjTagged{KjCircle{1}};
    yyjson::merge_into(R"({"s":{"r":2}})", t);
    EXPECT_EQ(t, KjTagged{KjCircle{2}});
}

TEST(Constraints, JsonKindInputValues)
{
    // the read checks the value of the input, and `validate` the value that the struct writes
    expect_accepted<KjLevels>(R"({"a":0,"z":"one"})", {0, {{"z", KjLevel::one}}});
    expect_accepted<KjLevels>(R"({"a":0,"z":0})", {0, {{"z", KjLevel::zero}}});
    expect_rejected<KjLevels>(
        R"({"a":0,"z":1})", R"(the number is greater than the maximum 0 at /z; reading the key "z" of KjLevels)", "/z");
    EXPECT_NO_THROW(yyjson::validate(KjLevels{0, {{"z", KjLevel::one}}}));
    // a value of the DOM checks its elements by their positions
    EXPECT_NO_THROW((void)yyjson::read<KjDomPrefix>(R"({"v":[0,-1]})"));
    const auto e = bad_cast_of([] { (void)yyjson::read<KjDomPrefix>(R"({"v":[-1]})"); });
    EXPECT_EQ(e.path(), "/v/0");
}

TEST(Constraints, ExtraKeyProperty)
{
    expect_accepted<ScExtra>(R"({"a":1,"b":2,"c":1})", {1, {{"b", 2}, {"c", 1}}});
    expect_rejected<ScExtra>(
        R"({"a":1,"b":2,"c":-1})",
        "the number is less than the minimum 0 at /c; reading a value of ScExtra in yyjson::additional_properties",
        "/c");
    expect_rejected<ScExtra>(R"({"a":1,"b":-2})",
                             R"(the number is less than the minimum 0 at /b; reading the key "b" of ScExtra)", "/b");
    EXPECT_NO_THROW(yyjson::validate(ScExtra{0, {{"b", 2}, {"c", 1}}}));
    EXPECT_THROW(yyjson::validate(ScExtra{0, {{"b", 2}, {"c", -1}}}), yyjson::bad_cast);
}

TEST(Constraints, WrittenCasterElements)
{
    // `validate` checks the JSON that the caster writes, and does not read it back
    EXPECT_NO_THROW(yyjson::validate(ScWrittenVec{{{"ok"}}, {{"k", {"ok"}}}}));
    const auto v = bad_cast_of([] { yyjson::validate(ScWrittenVec{{{"bad"}}, {}}); });
    EXPECT_EQ(v.path(), "/v/0");
    const auto m = bad_cast_of([] { yyjson::validate(ScWrittenVec{{}, {{"k", {"bad"}}}}); });
    EXPECT_EQ(m.path(), "/m/k");
    EXPECT_EQ(yyjson::write(ScWrittenVec{{{"ok"}}, {}}, yyjson::with_validation), R"({"v":["ok"],"m":{}})");
    EXPECT_THROW((void)yyjson::write(ScWrittenVec{{{"bad"}}, {}}, yyjson::with_validation), yyjson::bad_cast);
}

TEST(Constraints, MergeChecksPatch)
{
    // a merge into a value that a caster replaces checks the JSON of the patch
    auto counts = ScCounts{{{"k", ScCount{0}}}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"k":{"n":1}}})", counts); });
    EXPECT_EQ(e.path(), "/m/k");
    yyjson::merge_into(R"({"m":{"k":{"n":0}}})", counts);
    // a patched member is checked by the value of the patch, not by the value that it writes
    auto rest = ScLevelRest{};
    const auto r = bad_cast_of([&] { yyjson::merge_into(R"({"e":1})", rest); });
    EXPECT_EQ(message_of(r),
              "the number is greater than the maximum 0 at /e; reading a value of ScLevelRest in "
              "yyjson::additional_properties")
        << r.what();
    yyjson::merge_into(R"({"e":0})", rest);
    EXPECT_EQ(rest, ScLevelRest{});
}

TEST(Constraints, InPlaceCasterKind)
{
    expect_accepted<ScInPlaceField>(R"({"x":1})", {{1}});
    expect_rejected<ScInPlaceField>(
        R"({"x":-1})", R"(the number is less than the minimum 0 at /x; reading the key "x" of ScInPlaceField)", "/x");
}

TEST(Constraints, TupleCombinatorKinds)
{
    expect_accepted<ScPair>(R"({"t":[0,"a"]})", {{0, "a"}});
    expect_accepted<ScPair>(R"({"t":[1,"ab"]})", {{1, "ab"}});
    expect_rejected<ScPair>(R"({"t":[1,"a"]})",
                            R"(the array has no item that satisfies contains at /t; reading the key "t" of ScPair)",
                            "/t");
    EXPECT_NO_THROW(yyjson::validate(ScPair{{1, "ab"}}));
    EXPECT_THROW(yyjson::validate(ScPair{{1, "a"}}), yyjson::bad_cast);
}

TEST(Constraints, TagStringKeywords)
{
    expect_accepted<ScTagged>(R"({"kind":"ii","v":1})", {1});
    EXPECT_THROW((void)yyjson::read<ScTagged>(R"({"kind":"s","v":"x"})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"kind":"s","v":"x"})").cast<ScTagged>(), yyjson::bad_cast);
    EXPECT_THROW(yyjson::validate(ScTagged{std::string("x")}), yyjson::bad_cast);
}

// the keywords on a type check the JSON that its caster reads and writes
struct CtList
{
    std::vector<int> v;
    bool operator==(const CtList&) const = default;
};
template <>
struct yyjson::caster<CtList>
{
    static CtList from_json(const std::vector<int>& v) { return {v}; }
    static std::vector<int> to_json(const CtList& l) { return l.v; }
};
template <>
inline constexpr auto yyjson::annotations<CtList> = yyjson::annotate(
    yyjson::not_(yyjson::contains(yyjson::minimum(5))), yyjson::prefix_items(yyjson::maximum(3)), yyjson::unique_items);
struct CtMap
{
    std::map<std::string, int> m;
    bool operator==(const CtMap&) const = default;
};
template <>
struct yyjson::caster<CtMap>
{
    static CtMap from_json(const std::map<std::string, int>& m) { return {m}; }
    static std::map<std::string, int> to_json(const CtMap& o) { return o.m; }
};
template <>
inline constexpr auto yyjson::annotations<CtMap> =
    yyjson::annotate(yyjson::property("a", yyjson::minimum(0)), yyjson::additional_properties(yyjson::maximum(9)),
                     yyjson::dependent_required("a", {"b"}), yyjson::additional_properties(false),
                     yyjson::property("b", yyjson::minimum(0)));
struct CtNumber
{
    int n = 0;
    bool operator==(const CtNumber&) const = default;
};
template <>
struct yyjson::caster<CtNumber>
{
    static CtNumber from_json(int n) { return {n}; }
    static int to_json(const CtNumber& n) { return n.n; }
};
template <>
inline constexpr auto yyjson::annotations<CtNumber> =
    yyjson::annotate(yyjson::not_(yyjson::any_of(yyjson::minimum(5), yyjson::multiple_of(2))));
// the read reflects the struct, and the caster writes an array
struct CtWritten
{
    int a = 0;
    bool operator==(const CtWritten&) const = default;
};
template <>
struct yyjson::caster<CtWritten>
{
    static std::vector<int> to_json(const CtWritten& w) { return {w.a}; }
};
template <>
inline constexpr auto yyjson::annotations<CtWritten> =
    yyjson::annotate(yyjson::items(yyjson::minimum(0)), yyjson::property("a", yyjson::maximum(5)));
struct CtHolder
{
    CtList l;
    std::vector<CtMap> ms;
    bool operator==(const CtHolder&) const = default;
};

TEST(Constraints, CasterTypeKeywords)
{
    // the unique items of the JSON of a caster leave the type to the document
    expect_accepted<CtList>("[1,2]", {{1, 2}}, false);
    expect_rejected<CtList>("[1,5]", "the value satisfies the subschema of not; reading CtList", "");
    expect_rejected<CtList>("[4]", "the array has an item that does not satisfy prefixItems; reading CtList", "");
    expect_rejected<CtList>("[1,1]", "the array has duplicate items; reading CtList", "");
    EXPECT_NO_THROW(yyjson::validate(CtList{{1, 2}}));
    EXPECT_THROW(yyjson::validate(CtList{{6}}), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(CtList{{1, 1}}, yyjson::with_validation), yyjson::bad_cast);

    expect_accepted<CtMap>(R"({"a":1,"b":2})", {{{"a", 1}, {"b", 2}}});
    expect_rejected<CtMap>(R"({"a":1})",
                           "the object does not have all the keys that dependentRequired requires; reading CtMap", "");
    expect_rejected<CtMap>(R"({"a":-1,"b":1})", "the object does not satisfy properties; reading CtMap", "");
    expect_rejected<CtMap>(R"({"c":1})",
                           "the object has a key that is not allowed at /c; reading CtMap, which "
                           "yyjson::additional_properties(false) closes",
                           "/c");
    EXPECT_NO_THROW(yyjson::validate(CtMap{{{"a", 1}, {"b", 1}}}));
    EXPECT_THROW(yyjson::validate(CtMap{{{"b", -1}}}), yyjson::bad_cast);

    expect_accepted<CtNumber>("1", {1});
    expect_rejected<CtNumber>("2", "the value satisfies the subschema of not; reading CtNumber", "");
    expect_rejected<CtNumber>("7", "the value satisfies the subschema of not; reading CtNumber", "");
    EXPECT_THROW(yyjson::validate(CtNumber{2}), yyjson::bad_cast);

    // the read checks the object that it reads, and `validate` the array that the caster writes
    EXPECT_EQ(yyjson::read<CtWritten>(R"({"a":1})"), CtWritten{1});
    EXPECT_THROW((void)yyjson::read<CtWritten>(R"({"a":6})"), yyjson::bad_cast);
    EXPECT_NO_THROW(yyjson::validate(CtWritten{6}));
    EXPECT_THROW(yyjson::validate(CtWritten{-1}), yyjson::bad_cast);

    // a value at a position checks its caster type too, and a merge the JSON of the patch
    const auto e = bad_cast_of([] { (void)yyjson::read<CtHolder>(R"({"l":[1],"ms":[{"c":1}]})"); });
    EXPECT_EQ(e.path(), "/ms/0/c");
    auto holder = CtHolder{};
    const auto m = bad_cast_of([&] { yyjson::merge_into(R"({"l":[1,1]})", holder); });
    EXPECT_EQ(m.path(), "/l");
}

// a typed predicate on an element that a caster writes takes the C++ element, and the other keywords its JSON
struct C4Word
{
    std::string s;
    bool operator==(const C4Word&) const = default;
};
template <>
struct yyjson::caster<C4Word>
{
    static C4Word from_json(std::string s) { return {std::move(s)}; }
    static std::string to_json(const C4Word& w) { return w.s; }
};
inline bool c4_allowed(const C4Word& w) { return w.s != "no"; }
struct C4Words
{
    std::vector<C4Word> v;
    std::map<std::string, C4Word> m;
    bool operator==(const C4Words&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C4Words> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(3), yyjson::satisfies<c4_allowed>)),
                     yyjson::field("m", yyjson::additional_properties(yyjson::max_length(3),
                                                                      yyjson::all_of(yyjson::satisfies<c4_allowed>))));
// a merge checks a member that the patch merges into on the patch at each depth
struct C4Inner
{
    ScLevel e = ScLevel::zero;
    int k = 3;
    bool operator==(const C4Inner&) const = default;
};
struct C4Outer
{
    C4Inner in;
    bool operator==(const C4Outer&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C4Outer> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
// an enumeration with a caster takes the keywords of its JSON
enum class C4Code
{
    a,
    b
};
template <>
struct yyjson::caster<C4Code>
{
    static C4Code from_json(int i) { return static_cast<C4Code>(i); }
    static int to_json(const C4Code& c) { return static_cast<int>(c); }
};
template <>
inline constexpr auto yyjson::annotations<C4Code> = yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
// a class that is not an aggregate
class C4Number
{
    int n_ = 0;

public:
    C4Number(int n = 0) : n_(n) {}  // NOLINT(google-explicit-constructor)
    [[nodiscard]] int n() const { return n_; }
    bool operator==(const C4Number&) const = default;
};
template <>
struct yyjson::caster<C4Number>
{
    static C4Number from_json(int i) { return {i}; }
    static int to_json(const C4Number& x) { return x.n(); }
};
template <>
inline constexpr auto yyjson::annotations<C4Number> = yyjson::annotate(yyjson::all_of(yyjson::maximum(1)));
// `items(false)` on the array of a caster
struct C4Empty
{
    std::vector<int> v;
    bool operator==(const C4Empty&) const = default;
};
template <>
struct yyjson::caster<C4Empty>
{
    static C4Empty from_json(const std::vector<int>& v) { return {v}; }
    static std::vector<int> to_json(const C4Empty& x) { return x.v; }
};
template <>
inline constexpr auto yyjson::annotations<C4Empty> = yyjson::annotate(yyjson::items(false));
struct C4EmptyHolder
{
    C4Empty x;
    bool operator==(const C4EmptyHolder&) const = default;
};

TEST(Constraints, CasterElementPredicates)
{
    expect_accepted<C4Words>(R"({"v":["yes"],"m":{"k":"yes"}})", {{{"yes"}}, {{"k", {"yes"}}}});
    expect_rejected<C4Words>(
        R"({"v":["no"],"m":{}})",
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C4Words)",
        "/v/0");
    expect_rejected<C4Words>(
        R"({"v":["long"],"m":{}})",
        R"(the string is longer than the maximum length 3 at /v/0; reading an element of the value at the key "v" of C4Words)",
        "/v/0");
    expect_rejected<C4Words>(
        R"({"v":[],"m":{"k":"no"}})",
        R"(the value does not satisfy all the subschemas of allOf at /m/k; reading an element of the value at the key "m" of C4Words)",
        "/m/k");
    EXPECT_NO_THROW(yyjson::validate(C4Words{{{"yes"}}, {{"k", {"yes"}}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{{"no"}}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{}, {{"k", {"no"}}}}); }).path(), "/m/k");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{}, {{"k", {"long"}}}}); }).path(), "/m/k");
}

TEST(Constraints, NestedMergeObject)
{
    expect_rejected<C4Outer>(R"({"in":{"e":1,"k":0}})",
                             "the object has a value that does not satisfy additionalProperties at /in; reading a "
                             "value of C4Outer in yyjson::additional_properties",
                             "/in", false);
    auto outer = C4Outer{};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"in":{"e":1,"k":0}})", outer); });
    EXPECT_EQ(e.path(), "/in");
    // the kept member k = 3 is checked on its written value
    outer = C4Outer{};
    EXPECT_THROW(yyjson::merge_into(R"({"in":{"e":0}})", outer), yyjson::bad_cast);
    outer = C4Outer{};
    yyjson::merge_into(R"({"in":{"e":0,"k":0}})", outer);
    EXPECT_EQ(outer, (C4Outer{{ScLevel::zero, 0}}));
}

TEST(Constraints, CasterTypeKeywordsOfOtherTypes)
{
    expect_accepted<C4Code>("0", C4Code::a);
    expect_rejected<C4Code>("1", "the value does not satisfy all the subschemas of allOf; reading C4Code", "");
    EXPECT_NO_THROW(yyjson::validate(C4Code::a));
    EXPECT_THROW(yyjson::validate(C4Code::b), yyjson::bad_cast);

    expect_accepted<C4Number>("1", C4Number{1});
    for (const auto& e : {bad_cast_of([] { (void)yyjson::read<C4Number>("2"); }),
                          bad_cast_of([] { (void)yyjson::read("2").cast<C4Number>(); }), bad_cast_of([] {
                              auto x = C4Number();
                              yyjson::merge_into("2", x);
                          })})
        EXPECT_EQ(what_without_position(e), "the value does not satisfy all the subschemas of allOf; reading C4Number");
    EXPECT_THROW(yyjson::validate(C4Number{2}), yyjson::bad_cast);

    expect_accepted<C4Empty>("[]", C4Empty{});
    expect_rejected<C4Empty>("[1]", "the array has an item that does not satisfy items; reading C4Empty", "");
    EXPECT_THROW(yyjson::validate(C4Empty{{1}}), yyjson::bad_cast);
    auto holder = C4EmptyHolder{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"x":[1]})", holder); }).path(), "/x");
}

// NOLINTEND
