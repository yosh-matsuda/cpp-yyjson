#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::strict` rejects JSON that has more or less than the type takes, and `yyjson::required` and
// `yyjson::additional_properties` check the keys of a struct without it
struct StrictPoint
{
    int x = 0;
    int y = 0;
    std::optional<int> z;
    bool operator==(const StrictPoint&) const = default;
};
struct StrictRequired
{
    int a = 0;
    int b = 0;
    bool operator==(const StrictRequired&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictRequired> = yyjson::annotate(yyjson::field("b", yyjson::required));
struct StrictClosed
{
    int a = 0;
    std::optional<int> b;
    bool operator==(const StrictClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictClosed> = yyjson::annotate(yyjson::additional_properties(false));
struct StrictOpen
{
    int a = 0;
    bool operator==(const StrictOpen&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictOpen> = yyjson::annotate(yyjson::additional_properties(true));
struct StrictSkipped
{
    int a = 0;
    int s = 0;
    bool operator==(const StrictSkipped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictSkipped> = yyjson::annotate(yyjson::field("s", yyjson::skip));
// more fields than the reader compares in their order
struct StrictLarge
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const StrictLarge&) const = default;
};
struct StrictVisitable
{
    int a = 0;
    std::optional<int> b;
    bool operator==(const StrictVisitable&) const = default;
};
VISITABLE_STRUCT(StrictVisitable, a, b);
struct StrictContainers
{
    std::vector<StrictPoint> points;
    std::map<std::string, int> map;
    std::set<int> set;
    bool operator==(const StrictContainers&) const = default;
};

struct StrictCircle
{
    int r = 0;
    bool operator==(const StrictCircle&) const = default;
};
struct StrictRect
{
    int w = 0;
    int h = 0;
    bool operator==(const StrictRect&) const = default;
};
struct StrictClosedCircle
{
    int r = 0;
    bool operator==(const StrictClosedCircle&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictClosedCircle> = yyjson::annotate(yyjson::additional_properties(false));
using StrictShape = std::variant<StrictCircle, StrictRect>;
template <>
inline constexpr auto yyjson::annotations<StrictShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
using StrictClosedShape = std::variant<StrictClosedCircle, StrictRect>;
template <>
inline constexpr auto yyjson::annotations<StrictClosedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
// an alternative read through a translator from a closed struct takes the tag as well
struct StrictTranslatedCircle
{
    int radius = 0;
    bool operator==(const StrictTranslatedCircle&) const = default;
};
template <>
struct yyjson::caster<StrictTranslatedCircle>
{
    static StrictTranslatedCircle from_json(const StrictClosedCircle& c) { return {c.r}; }
    static StrictClosedCircle to_json(const StrictTranslatedCircle& c) { return {c.radius}; }
};
using StrictTranslatedShape = std::variant<StrictTranslatedCircle, StrictRect>;
template <>
inline constexpr auto yyjson::annotations<StrictTranslatedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
// an internally tagged variant as an alternative of another, tagged in the same object
struct StrictText
{
    std::string s;
    bool operator==(const StrictText&) const = default;
};
using StrictItem = std::variant<StrictClosedShape, StrictText>;
template <>
inline constexpr auto yyjson::annotations<StrictItem> =
    yyjson::annotate(yyjson::internal_tag("kind", {"shape", "text"}));
// the tag of an adjacently tagged field is a field of the struct, or written by the library
using StrictBody = std::variant<StrictCircle, StrictRect, int>;
struct StrictEnvelope
{
    std::string kind;
    StrictBody data;
    bool operator==(const StrictEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictEnvelope> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "number"})));
struct StrictBare
{
    int id = 0;
    StrictBody body;
    bool operator==(const StrictBare&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictBare> =
    yyjson::annotate(yyjson::field("body", yyjson::adjacent_tag("t", {"circle", "rect", "number"})));

// read by a caster that casts its member itself
struct StrictWrapped
{
    StrictPoint p;
};
template <>
struct yyjson::caster<StrictWrapped>
{
    static StrictWrapped from_json(const yyjson::reader::const_value_ref& v) { return {v.cast<StrictPoint>()}; }
};
#if YYJSON_VERSION_HEX >= 0x000C00
// read by a caster that casts a view strictly
struct StrictViewed
{
    StrictPoint p;
};
template <>
struct yyjson::caster<StrictViewed>
{
    static StrictViewed from_json(const yyjson::json_view& v) { return {v.cast<StrictPoint>(yyjson::strict)}; }
};
// casts a view strictly into a type whose caster reads a document
struct StrictViewedWrapped
{
    StrictPoint p;
};
template <>
struct yyjson::caster<StrictViewedWrapped>
{
    static StrictViewedWrapped from_json(const yyjson::json_view& v)
    {
        return {v.cast<StrictWrapped>(yyjson::strict).p};
    }
};
// casts the views of an array and an object strictly
struct StrictViewedParts
{
    StrictPoint p;
    std::vector<StrictPoint> list;
};
template <>
struct yyjson::caster<StrictViewedParts>
{
    static StrictViewedParts from_json(const yyjson::json_view& v)
    {
        const auto parts = v.as_array().value();
        return {parts[0].as_object().value().cast<StrictPoint>(yyjson::strict),
                parts[1].as_array().value().cast<std::vector<StrictPoint>>(yyjson::strict)};
    }
};
#endif

#if FIELD_RFL_ANNOTATIONS
struct[[= yyjson::additional_properties(false)]] StrictAnnotated
{
    [[= yyjson::required]] int a = 0;
    int b = 0;
    bool operator==(const StrictAnnotated&) const = default;
};
#endif

namespace
{
    template <typename F>
    yyjson::bad_cast bad_cast_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return e;
        }
        ADD_FAILURE() << "no exception";
        return yyjson::bad_cast("no exception");
    }

    // every strict read of `json` gives `expected`, and the reader without a document reads it
    template <typename T>
    void expect_strict(std::string_view json, const T& expected)
    {
        EXPECT_EQ(yyjson::read<T>(json, yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::read<T>(json, yyjson::ReadFlag::AllowComments, yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::cast<T>(yyjson::value(yyjson::read(json)), yyjson::strict), expected) << json;
        auto into = T();
        yyjson::read_into(json, into, yyjson::strict);
        EXPECT_EQ(into, expected) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto scope = yyjson::detail::strict_scope();
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_TRUE(direct.has_value()) << json;
        EXPECT_EQ(*direct, read_or_error([&]() -> const T& { return expected; })) << json;
#endif
    }

    // every read of `json` without `yyjson::strict` gives `expected`, also through the reader without a document
    // and by merging with and without a document
    template <typename T>
    void expect_lenient(std::string_view json, const T& expected)
    {
        const auto result = read_or_error([&]() -> const T& { return expected; });
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        auto into = T();
        yyjson::read_into(json, into);
        EXPECT_EQ(into, expected) << json;
        auto merged_directly = false;
        EXPECT_EQ(expect_merge_into(T(), json, &merged_directly), result) << json;
        EXPECT_TRUE(merged_directly) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_TRUE(direct.has_value()) << json;
        EXPECT_EQ(*direct, result) << json;
#endif
    }

    // every strict read of `json` throws `bad_cast` with `message` about the value at `path`, and the reader without
    // a document leaves it to the document
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path = "")
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(std::string_view(e.what()).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::strict); }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::read_into(json, t, yyjson::strict);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(yyjson::strict); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(yyjson::strict); }));
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto scope = yyjson::detail::strict_scope();
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
#endif
    }
}  // namespace

TEST(Strict, Structs)
{
    expect_strict<StrictPoint>(R"({"x":1,"y":2})", {1, 2, {}});
    expect_strict<StrictPoint>(R"({"x":1,"y":2,"z":3})", {1, 2, 3});
    expect_strict<StrictPoint>(R"({"z":3,"y":2,"x":1})", {1, 2, 3});
    expect_strict<StrictPoint>(R"({"y":2,"x":1})", {1, 2, {}});

    expect_rejected<StrictPoint>(R"({"x":1})", R"(StrictPoint is not constructible from JSON without the key "y")");
    expect_rejected<StrictPoint>(R"({"z":1})", R"(StrictPoint is not constructible from JSON without the key "x")");
    expect_rejected<StrictPoint>("{}", R"(StrictPoint is not constructible from JSON without the key "x")");
    expect_rejected<StrictPoint>(R"({"x":1,"y":2,"w":3})", R"(StrictPoint has no field of the key "w")", "/w");
    expect_rejected<StrictPoint>(R"({"w":3,"x":1,"y":2})", R"(StrictPoint has no field of the key "w")", "/w");
    expect_rejected<StrictPoint>(R"({"x":1,"y":2,"x":3})", R"(the object read into StrictPoint has the key "x" twice)",
                                 "/x");
    expect_rejected<StrictPoint>(R"({"x":1,"y":2,"z":null})",
                                 "std::optional<int> is not constructible from null under yyjson::strict", "/z");

    // none of them without `yyjson::strict`
    EXPECT_EQ(yyjson::read<StrictPoint>(R"({"x":1})"), (StrictPoint{1, 0, {}}));
    EXPECT_EQ(yyjson::read<StrictPoint>(R"({"x":1,"y":2,"w":3})"), (StrictPoint{1, 2, {}}));
    EXPECT_EQ(yyjson::read<StrictPoint>(R"({"x":1,"y":2,"z":null})"), (StrictPoint{1, 2, {}}));
    EXPECT_EQ(yyjson::read(R"({"x":1,"y":2,"w":3})").cast<StrictPoint>(), (StrictPoint{1, 2, {}}));

    // the position of the object that misses a key, and of the value of a key that is not a field
    auto e = bad_cast_of(
        [] { (void)yyjson::read<std::vector<StrictPoint>>(R"([{"x":1,"y":2}, {"x":1}])", yyjson::strict); });
    EXPECT_EQ(e.path(), "/1");
    EXPECT_EQ(e.position(), (yyjson::text_position{1, 17, 16}));
    e = bad_cast_of([] { (void)yyjson::read<StrictPoint>(R"({"x":1,"y":2,"w":3})", yyjson::strict); });
    EXPECT_STREQ(e.what(), R"(StrictPoint has no field of the key "w" at /w (line 1, column 18, pos 17))");

    expect_strict<StrictLarge>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})",
                               {1, 2, 3, 4, 5, 6, 7, 8, 9, {}});
    expect_strict<StrictLarge>(R"({"i":9,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"a":1,"j":0})",
                               {1, 2, 3, 4, 5, 6, 7, 8, 9, 0});
    expect_rejected<StrictLarge>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})",
                                 R"(StrictLarge is not constructible from JSON without the key "i")");
    expect_rejected<StrictLarge>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"k":0})",
                                 R"(StrictLarge has no field of the key "k")", "/k");

    expect_strict<StrictVisitable>(R"({"a":1})", {1, {}});
    expect_rejected<StrictVisitable>(R"({"b":1})",
                                     R"(StrictVisitable is not constructible from JSON without the key "a")");
    expect_rejected<StrictVisitable>(R"({"a":1,"c":1})", R"(StrictVisitable has no field of the key "c")", "/c");
}

TEST(Strict, Annotations)
{
    // `yyjson::required` without `yyjson::strict`, and the other fields under it
    EXPECT_EQ(yyjson::read<StrictRequired>(R"({"b":2})"), (StrictRequired{0, 2}));
    EXPECT_EQ(yyjson::read(R"({"b":2,"c":3})").cast<StrictRequired>(), (StrictRequired{0, 2}));
    auto e = bad_cast_of([] { (void)yyjson::read<StrictRequired>(R"({"a":1})"); });
    EXPECT_STREQ(e.what(),
                 R"(StrictRequired is not constructible from JSON without the key "b" (line 1, column 1, pos 0))");
    e = bad_cast_of([] { (void)yyjson::read(R"({"a":1})").cast<StrictRequired>(); });
    EXPECT_STREQ(e.what(), R"(StrictRequired is not constructible from JSON without the key "b")");
    expect_rejected<StrictRequired>(R"({"b":2})",
                                    R"(StrictRequired is not constructible from JSON without the key "a")");

    // `yyjson::additional_properties(false)` without `yyjson::strict`, and `(true)` under it
    EXPECT_EQ(yyjson::read<StrictClosed>(R"({"a":1})"), (StrictClosed{1, {}}));
    EXPECT_EQ(yyjson::read<StrictClosed>(R"({"a":1,"b":null})"), (StrictClosed{1, {}}));
    e = bad_cast_of([] { (void)yyjson::read<StrictClosed>(R"({"a":1,"c":2})"); });
    EXPECT_EQ(e.path(), "/c");
    EXPECT_STREQ(e.what(), R"(StrictClosed has no field of the key "c" at /c (line 1, column 12, pos 11))");
    EXPECT_THROW((void)yyjson::read(R"({"c":2,"a":1})").cast<StrictClosed>(), yyjson::bad_cast);
    expect_strict<StrictOpen>(R"({"a":1,"c":2})", {1});
    expect_rejected<StrictOpen>(R"({"c":2})", R"(StrictOpen is not constructible from JSON without the key "a")");

    // the key of a skipped field names no field
    EXPECT_EQ(yyjson::read<StrictSkipped>(R"({"a":1,"s":2})"), (StrictSkipped{1, 0}));
    expect_rejected<StrictSkipped>(R"({"a":1,"s":2})", R"(StrictSkipped has no field of the key "s")", "/s");

#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(yyjson::read<StrictAnnotated>(R"({"a":1})"), (StrictAnnotated{1, 0}));
    EXPECT_THROW((void)yyjson::read<StrictAnnotated>(R"({"b":1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<StrictAnnotated>(R"({"a":1,"c":1})"), yyjson::bad_cast);
#endif
}

TEST(Strict, Containers)
{
    expect_strict<std::set<int>>("[1,2,3]", {1, 2, 3});
    expect_rejected<std::set<int>>("[1,2,1]", "the array read into std::set<int> has an element twice", "/2");
    expect_rejected<std::unordered_set<int>>("[1,2,1]", "the array read into", "/2");
    expect_strict<std::multiset<int>>("[1,2,1]", {1, 1, 2});
    EXPECT_EQ(yyjson::read<std::set<int>>("[1,2,1]"), (std::set<int>{1, 2}));

    expect_strict<std::map<std::string, int>>(R"({"a":1,"b":2})", {{"a", 1}, {"b", 2}});
    expect_rejected<std::map<std::string, int>>(R"({"a":1,"b":2,"a":3})", "the object read into", "/a");
    expect_rejected<std::unordered_map<std::string, int>>(R"({"a":1,"a":3})", "the object read into", "/a");
    using map_type = std::map<std::string, int>;
    EXPECT_EQ(yyjson::read<map_type>(R"({"a":1,"a":3})"), (map_type{{"a", 1}}));
    expect_rejected<std::map<int, int>>("[[1,2],[1,3]]", "the array read into", "/1");

    expect_strict<StrictContainers>(R"({"points":[{"x":1,"y":2}],"map":{"a":1},"set":[1]})",
                                    {{{1, 2, {}}}, {{"a", 1}}, {1}});
    expect_rejected<StrictContainers>(R"({"points":[{"x":1,"y":2},{"x":1,"y":2,"w":0}],"map":{},"set":[]})",
                                      R"(StrictPoint has no field of the key "w")", "/points/1/w");
    // `read_into` reads over a map that has the keys
    auto c = StrictContainers{{}, {{"a", 0}}, {}};
    EXPECT_THROW(yyjson::read_into(R"({"points":[],"map":{"a":1,"a":2},"set":[]})", c, yyjson::strict),
                 yyjson::bad_cast);
}

TEST(Strict, Variants)
{
    // the tag of an internally tagged variant is a key of its alternative, and adjacent tags of the struct
    expect_strict<StrictShape>(R"({"type":"circle","r":1})", StrictCircle{1});
    expect_strict<StrictShape>(R"({"w":1,"type":"rect","h":2})", StrictRect{1, 2});
    expect_rejected<StrictShape>(R"({"type":"circle","r":1,"w":2})", R"(StrictCircle has no field of the key "w")",
                                 "/w");
    expect_rejected<StrictShape>(R"({"type":"rect","w":1})",
                                 R"(StrictRect is not constructible from JSON without the key "h")");
    expect_strict<std::vector<StrictShape>>(R"([{"type":"circle","r":1},{"type":"rect","w":1,"h":2}])",
                                            {StrictCircle{1}, StrictRect{1, 2}});
    expect_strict<StrictEnvelope>(R"({"kind":"circle","data":{"r":1}})", {"circle", StrictCircle{1}});
    expect_strict<StrictBare>(R"({"id":1,"t":"rect","body":{"w":1,"h":2}})", {1, StrictRect{1, 2}});
    expect_rejected<StrictBare>(R"({"id":1,"t":"rect","body":{"w":1,"h":2},"u":0})",
                                R"(StrictBare has no field of the key "u")", "/u");

    // a closed alternative takes its tag without `yyjson::strict`
    EXPECT_EQ(yyjson::read<StrictClosedShape>(R"({"type":"circle","r":1})"), StrictClosedShape(StrictClosedCircle{1}));
    EXPECT_EQ(yyjson::read(R"({"r":1,"type":"circle"})").cast<StrictClosedShape>(),
              StrictClosedShape(StrictClosedCircle{1}));
    EXPECT_THROW((void)yyjson::read<StrictClosedShape>(R"({"type":"circle","r":1,"w":1})"), yyjson::bad_cast);
    // but not the key of another variant's tag
    EXPECT_THROW((void)yyjson::read<StrictClosedCircle>(R"({"type":"circle","r":1})"), yyjson::bad_cast);

    // nor does a closed struct read through a translator, which reads back what `write` writes
    const auto translated = StrictTranslatedShape(StrictTranslatedCircle{2});
    const auto written = std::string(yyjson::write(translated));
    EXPECT_EQ(written, R"({"type":"circle","r":2})");
    expect_lenient<StrictTranslatedShape>(written, translated);
    expect_strict<StrictTranslatedShape>(written, translated);
    auto merged = StrictTranslatedShape(StrictRect{});
    yyjson::merge_into(written, merged);
    EXPECT_EQ(merged, translated);

    // the alternative of a variant tagged in the same object takes both tags
    const auto item = StrictItem(StrictClosedShape(StrictClosedCircle{1}));
    expect_lenient<StrictItem>(R"({"kind":"shape","type":"circle","r":1})", item);
    EXPECT_EQ(yyjson::read(R"({"r":1,"type":"circle","kind":"shape"})").cast<StrictItem>(), item);
    expect_strict<StrictItem>(R"({"kind":"shape","type":"circle","r":1})", item);
    expect_strict<StrictItem>(R"({"kind":"text","s":"a"})", StrictItem(StrictText{"a"}));
    EXPECT_THROW((void)yyjson::read<StrictItem>(R"({"kind":"shape","type":"circle","r":1,"w":1})"), yyjson::bad_cast);

    // an untagged variant takes the first alternative that the strict read accepts
    using Untagged = std::variant<StrictCircle, StrictRect, std::string>;
    EXPECT_EQ(yyjson::read<Untagged>(R"({"w":1,"h":2})", yyjson::strict), Untagged(StrictRect{1, 2}));
    EXPECT_EQ(yyjson::read<Untagged>(R"({"w":1,"h":2})"), Untagged(StrictCircle{0}));
}

TEST(Strict, Merge)
{
    // the keys that the patch leaves out are kept, and null resets a field
    auto p = StrictPoint{1, 2, 3};
    yyjson::merge_into(R"({"y":5,"z":null})", p, yyjson::strict);
    EXPECT_EQ(p, (StrictPoint{1, 5, {}}));
    EXPECT_THROW(yyjson::merge_into(R"({"y":5,"w":0})", p, yyjson::strict), yyjson::bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"y":5,"y":6})", p, yyjson::strict), yyjson::bad_cast);
    auto r = StrictRequired{1, 2};
    yyjson::merge_into(R"({"a":3})", r);
    EXPECT_EQ(r, (StrictRequired{3, 2}));
    yyjson::merge_into(R"({"a":4})", r, yyjson::strict);
    EXPECT_EQ(r, (StrictRequired{4, 2}));

    auto m = std::map<std::string, int>{{"a", 1}};
    EXPECT_THROW(yyjson::merge_into(R"({"b":1,"b":2})", m, yyjson::strict), yyjson::bad_cast);
    yyjson::merge_into(R"({"b":1,"b":2})", m);
    EXPECT_EQ(m, (std::map<std::string, int>{{"a", 1}, {"b", 2}}));

    auto s = StrictShape(StrictCircle{1});
    yyjson::merge_into(R"({"type":"circle","r":2})", s, yyjson::strict);
    EXPECT_EQ(s, StrictShape(StrictCircle{2}));
    EXPECT_THROW(yyjson::merge_into(R"({"type":"circle","w":2})", s, yyjson::strict), yyjson::bad_cast);
}

TEST(Strict, Scope)
{
    // a caster's casts read strictly within a strict read
    EXPECT_EQ(yyjson::read<StrictWrapped>(R"({"x":1,"w":0})").p, (StrictPoint{1, 0, {}}));
    EXPECT_THROW((void)yyjson::read<StrictWrapped>(R"({"x":1,"w":0})", yyjson::strict), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1})").cast<StrictWrapped>(yyjson::strict), yyjson::bad_cast);

#if YYJSON_VERSION_HEX >= 0x000C00
    // a caster casts a view strictly within a read that is not strict
    EXPECT_EQ(yyjson::read<StrictViewed>(R"({"x":1,"y":2})").p, (StrictPoint{1, 2, {}}));
    EXPECT_EQ(yyjson::read(R"({"x":1,"y":2})").cast<StrictViewed>().p, (StrictPoint{1, 2, {}}));
    EXPECT_THROW((void)yyjson::read<StrictViewed>(R"({"x":1,"y":2,"w":0})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"y":2,"w":0})").cast<StrictViewed>(), yyjson::bad_cast);
    EXPECT_TRUE(read_directly<StrictViewed>(R"({"x":1,"y":2})", yyjson::ReadFlag::NoFlag).has_value());
    // also the casts of the document that the caster of the type reads
    EXPECT_EQ(yyjson::read<StrictViewedWrapped>(R"({"x":1,"y":2})").p, (StrictPoint{1, 2, {}}));
    EXPECT_THROW((void)yyjson::read<StrictViewedWrapped>(R"({"x":1,"y":2,"w":0})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"y":2,"w":0})").cast<StrictViewedWrapped>(), yyjson::bad_cast);
    // and the views of an array and an object
    constexpr auto parts = R"([{"x":1,"y":2},[{"x":3,"y":4}]])";
    EXPECT_EQ(yyjson::read<StrictViewedParts>(parts).list, (std::vector{StrictPoint{3, 4, {}}}));
    EXPECT_TRUE(read_directly<StrictViewedParts>(parts, yyjson::ReadFlag::NoFlag).has_value());
    for (const auto json : {R"([{"x":1,"y":2,"w":0},[]])", R"([{"x":1,"y":2},[{"x":3,"y":4,"w":0}]])"})
    {
        EXPECT_THROW((void)yyjson::read<StrictViewedParts>(json), yyjson::bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<StrictViewedParts>(), yyjson::bad_cast) << json;
    }
#endif
    EXPECT_EQ(yyjson::read<std::vector<StrictPoint>>(R"([{"x":1,"w":0}])"), (std::vector{StrictPoint{1, 0, {}}}));

    // and the reads after one read as before
    EXPECT_EQ(yyjson::read<StrictPoint>(R"({"x":1,"w":0})"), (StrictPoint{1, 0, {}}));
    EXPECT_EQ(yyjson::read(R"({"x":1,"w":0})").cast<StrictPoint>(), (StrictPoint{1, 0, {}}));
}

// NOLINTEND
