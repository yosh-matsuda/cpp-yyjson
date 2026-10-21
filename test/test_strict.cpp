#include <mutex>

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
// fields that `yyjson::strict` lets the object leave out, which keep the value they have in `T()`
struct StrictDefaulted
{
    int a = 0;
    int b = 7;
    std::vector<int> c = {1};
    bool operator==(const StrictDefaulted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictDefaulted> = yyjson::annotate(
    yyjson::field("b", yyjson::optional_key), yyjson::field("c", yyjson::optional_key, yyjson::max_items(1)));
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
// an alternative with a required key, tagged internally and adjacently
using StrictRequiredShape = std::variant<StrictCircle, StrictRequired>;
template <>
inline constexpr auto yyjson::annotations<StrictRequiredShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "required"}));
struct StrictRequiredBare
{
    std::variant<StrictCircle, StrictRequired> body;
    bool operator==(const StrictRequiredBare&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<StrictRequiredBare> =
    yyjson::annotate(yyjson::field("body", yyjson::adjacent_tag("t", {"circle", "required"})));

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
// an alternative read by a caster that casts its view, which takes the tag as the struct that it reads
struct StrictViewedCircle
{
    StrictCircle c;
    bool operator==(const StrictViewedCircle&) const = default;
};
template <>
struct yyjson::caster<StrictViewedCircle>
{
    static StrictViewedCircle from_json(const yyjson::json_view& v) { return {v.cast<StrictCircle>()}; }
};
using StrictViewedShape = std::variant<StrictViewedCircle, StrictRect>;
template <>
inline constexpr auto yyjson::annotations<StrictViewedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
// casts a view without `yyjson::strict` while a strict read runs on the thread
struct StrictScopedView
{
    StrictPoint p;
};
template <>
struct yyjson::caster<StrictScopedView>
{
    static StrictScopedView from_json(const yyjson::json_view& v)
    {
        const auto scope = yyjson::detail::strict_scope();
        return {v.cast<StrictPoint>()};
    }
};
#endif

// the overloads with `yyjson::strict` take the types that those without it take
template <typename T>
concept strict_reads = requires(T& t) {
    yyjson::read<T>("", yyjson::strict);
    yyjson::read<T>("", yyjson::ReadFlag::NoFlag, yyjson::strict);
};
template <typename T>
concept strict_reads_into = requires(T& t) {
    yyjson::read_into("", t, yyjson::strict);
    yyjson::read_into("", t, yyjson::ReadFlag::NoFlag, yyjson::strict);
};
template <typename T>
concept strict_merges_into = requires(T& t) {
    yyjson::merge_into("", t, yyjson::strict);
    yyjson::merge_into("", t, yyjson::ReadFlag::NoFlag, yyjson::strict);
};
static_assert(strict_reads<StrictPoint> && strict_reads_into<StrictPoint> && strict_merges_into<StrictPoint>);
static_assert(!strict_reads<std::mutex> && !strict_reads_into<std::mutex> && !strict_merges_into<std::mutex>);
static_assert(!strict_reads_into<const StrictPoint> && !strict_merges_into<const StrictPoint>);

#if FIELD_RFL_ANNOTATIONS
struct[[= yyjson::additional_properties(false)]] StrictAnnotated
{
    [[= yyjson::required]] int a = 0;
    int b = 0;
    bool operator==(const StrictAnnotated&) const = default;
};
struct StrictAnnotatedDefaulted
{
    int a = 0;
    [[= yyjson::optional_key]] int b = 7;
    bool operator==(const StrictAnnotatedDefaulted&) const = default;
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
    // a key twice after a key out of order, which comes the second time in the order of the fields
    expect_rejected<StrictPoint>(R"({"y":2,"x":1,"y":3})", R"(the object read into StrictPoint has the key "y" twice)",
                                 "/y");
    // each object of an array has its own keys
    expect_strict<std::vector<StrictPoint>>(R"([{"y":2,"x":1},{"z":3,"y":2,"x":1},{"y":2,"x":1}])",
                                            {{1, 2, {}}, {1, 2, 3}, {1, 2, {}}});
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
    expect_rejected<StrictLarge>(R"({"c":3,"a":1,"b":2,"c":4,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})",
                                 R"(the object read into StrictLarge has the key "c" twice)", "/c");
    expect_rejected<StrictLarge>(R"({"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})",
                                 R"(StrictLarge is not constructible from JSON without the key "a")");

    expect_strict<StrictVisitable>(R"({"a":1})", {1, {}});
    expect_rejected<StrictVisitable>(R"({"b":1})",
                                     R"(StrictVisitable is not constructible from JSON without the key "a")");
    expect_rejected<StrictVisitable>(R"({"a":1,"c":1})", R"(StrictVisitable has no field of the key "c")", "/c");
}

TEST(Strict, OptionalKey)
{
    expect_strict<StrictDefaulted>(R"({"a":1})", {1, 7, {1}});
    expect_strict<StrictDefaulted>(R"({"c":[2],"a":1,"b":3})", {1, 3, {2}});
    expect_strict<StrictDefaulted>(R"({"a":1,"c":[]})", {1, 7, {}});
    expect_rejected<StrictDefaulted>(R"({"b":3})",
                                     R"(StrictDefaulted is not constructible from JSON without the key "a")");
    expect_rejected<StrictDefaulted>(R"({"a":1,"x":0})", R"(StrictDefaulted has no field of the key "x")", "/x");
    // the value of the key is read and checked as without it, and null is not left out
    expect_rejected<StrictDefaulted>(R"({"a":1,"c":[1,2]})", "", "/c");
    expect_rejected<StrictDefaulted>(R"({"a":1,"b":null})", "", "/b");
    EXPECT_THROW((void)yyjson::read<StrictDefaulted>(R"({"a":1,"c":[1,2]})"), yyjson::bad_cast);
    // nothing changes without `yyjson::strict`, and the key is always written
    expect_lenient<StrictDefaulted>(R"({"b":3})", {0, 3, {1}});
    EXPECT_EQ(yyjson::write(StrictDefaulted()), R"({"a":0,"b":7,"c":[1]})");
#if FIELD_RFL_ANNOTATIONS
    expect_strict<StrictAnnotatedDefaulted>(R"({"a":1})", {1, 7});
    expect_rejected<StrictAnnotatedDefaulted>(
        R"({"b":1})", R"(StrictAnnotatedDefaulted is not constructible from JSON without the key "a")");
#endif
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
    EXPECT_THROW((void)yyjson::read<StrictRequired>("{}"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<StrictRequired>(R"({"c":0,"a":1})"), yyjson::bad_cast);
    EXPECT_EQ(yyjson::read<StrictRequired>(R"({"c":0,"b":2,"a":1})"), (StrictRequired{1, 2}));
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_FALSE(read_directly<StrictRequired>("{}", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_FALSE(read_directly<StrictRequired>(R"({"c":0,"a":1})", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_TRUE(read_directly<StrictRequired>(R"({"c":0,"b":2,"a":1})", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_FALSE(read_directly<StrictClosed>(R"({"a":1,"c":2,"b":3})", yyjson::ReadFlag::NoFlag).has_value());
#endif

    // `yyjson::additional_properties(false)` without `yyjson::strict`, and `(true)` under it
    EXPECT_EQ(yyjson::read<StrictClosed>(R"({"a":1})"), (StrictClosed{1, {}}));
    EXPECT_EQ(yyjson::read<StrictClosed>(R"({"a":1,"b":null})"), (StrictClosed{1, {}}));
    e = bad_cast_of([] { (void)yyjson::read<StrictClosed>(R"({"a":1,"c":2})"); });
    EXPECT_EQ(e.path(), "/c");
    EXPECT_STREQ(e.what(), R"(StrictClosed has no field of the key "c" at /c (line 1, column 12, pos 11))");
    EXPECT_THROW((void)yyjson::read(R"({"c":2,"a":1})").cast<StrictClosed>(), yyjson::bad_cast);
    expect_strict<StrictOpen>(R"({"a":1,"c":2})", {1});
    expect_rejected<StrictOpen>(R"({"c":2})", R"(StrictOpen is not constructible from JSON without the key "a")");
    expect_rejected<StrictOpen>(R"({"a":1,"c":2,"a":3})", R"(the object read into StrictOpen has the key "a" twice)",
                                "/a");

    // the key of a skipped field names no field
    EXPECT_EQ(yyjson::read<StrictSkipped>(R"({"a":1,"s":2})"), (StrictSkipped{1, 0}));
    expect_rejected<StrictSkipped>(R"({"a":1,"s":2})", R"(StrictSkipped has no field of the key "s")", "/s");

#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(yyjson::read<StrictAnnotated>(R"({"a":1})"), (StrictAnnotated{1, 0}));
    EXPECT_THROW((void)yyjson::read<StrictAnnotated>(R"({"b":1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<StrictAnnotated>(R"({"a":1,"c":1})"), yyjson::bad_cast);
#endif
}

namespace
{
    // what the reader without a document gives, under `yyjson::strict`, reading `json` over `t` or merging it into
    // `t`, or nothing when it leaves the text to the document
    template <typename T>
    std::optional<std::string> strictly_over(T t, std::string_view json, bool merge)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto scope = yyjson::detail::strict_scope();
        return read_root_directly(json, yyjson::ReadFlag::NoFlag, [&](auto& reader) {
            if (merge)
                reader.merge_root(t);
            else
                reader.read_root_into(t);
            return read_or_error([&]() -> const T& { return t; });
        });
#else
        static_cast<void>(t);
        static_cast<void>(json);
        static_cast<void>(merge);
        return std::nullopt;
#endif
    }

    // `read_into` and `merge_into` under `yyjson::strict` read `json` over `t` as the document does, also without it
    template <typename T>
    void expect_strict_over(const T& t, std::string_view json, bool merge, const T& expected)
    {
        auto u = t;
        if (merge)
            yyjson::merge_into(json, u, yyjson::strict);
        else
            yyjson::read_into(json, u, yyjson::strict);
        EXPECT_EQ(u, expected) << json;
        if (reads_directly)
        {
            const auto direct = strictly_over(t, json, merge);
            ASSERT_TRUE(direct.has_value()) << json;
            EXPECT_EQ(*direct, read_or_error([&]() -> const T& { return expected; })) << json;
        }
    }

    // and reject a key that `json` names twice, which the reader without a document leaves to the document
    template <typename T>
    void expect_rejected_over(const T& t, std::string_view json, bool merge, std::string_view path)
    {
        const auto e = bad_cast_of([&] {
            auto u = t;
            if (merge)
                yyjson::merge_into(json, u, yyjson::strict);
            else
                yyjson::read_into(json, u, yyjson::strict);
        });
        EXPECT_TRUE(std::string_view(e.what()).find(" twice") != std::string_view::npos) << json << "\n" << e.what();
        EXPECT_EQ(e.path(), path) << json;
        EXPECT_FALSE(strictly_over(t, json, merge).has_value()) << json;
    }
}  // namespace

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

TEST(Strict, MapsOver)
{
    // `read_into` and `merge_into` read a map in place under `yyjson::strict`, and a key named twice fails, whether
    // the map had it, the object added it or, in a merge, null erased it
    const auto check = []<typename M>(std::type_identity<M>) {
        const auto old = M{{"a", 1}, {"b", 2}};
        expect_strict_over(old, R"({"a":3,"b":4})", false, M{{"a", 3}, {"b", 4}});
        expect_strict_over(old, R"({"c":5,"b":4})", false, M{{"b", 4}, {"c", 5}});
        expect_strict_over(old, R"({"b":4,"0":1,"a":3,"z":6})", false, M{{"0", 1}, {"a", 3}, {"b", 4}, {"z", 6}});
        expect_strict_over(old, "{}", false, M());
        for (const auto& [json, path] : {std::pair{R"({"a":3,"a":4})", "/a"},
                                         {R"({"a":3,"b":4,"a":5})", "/a"},
                                         {R"({"b":4,"a":3,"b":5})", "/b"},
                                         {R"({"0":1,"0":2})", "/0"},
                                         {R"({"b":4,"z":1,"z":2})", "/z"}})
            expect_rejected_over(old, json, false, path);

        expect_strict_over(old, R"({"a":3,"c":5})", true, M{{"a", 3}, {"b", 2}, {"c", 5}});
        expect_strict_over(old, R"({"a":null,"x":null,"c":5})", true, M{{"b", 2}, {"c", 5}});
        for (const auto& [json, path] : {std::pair{R"({"a":3,"a":4})", "/a"},
                                         {R"({"c":3,"c":4})", "/c"},
                                         {R"({"a":null,"a":4})", "/a"},
                                         {R"({"a":4,"a":null})", "/a"},
                                         {R"({"x":null,"x":4})", "/x"},
                                         {R"({"x":null,"x":null})", "/x"}})
            expect_rejected_over(old, json, true, path);
    };
    check(std::type_identity<std::map<std::string, int>>());
    check(std::type_identity<std::unordered_map<std::string, int>>());
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

    // a tag key twice, of which the read without `yyjson::strict` takes the first
    expect_rejected<StrictShape>(R"({"type":"circle","type":"rect","r":1})",
                                 R"(the object read into StrictCircle has the key "type" twice)", "/type");
    expect_rejected<StrictBare>(R"({"id":1,"t":"rect","t":"circle","body":{"w":1,"h":2}})",
                                R"(the object read into StrictBare has the key "t" twice)", "/t");
    expect_rejected<StrictItem>(R"({"kind":"shape","type":"circle","r":1,"kind":"text"})",
                                R"(the object read into StrictClosedCircle has the key "kind" twice)", "/kind");
    EXPECT_EQ(yyjson::read<StrictShape>(R"({"type":"circle","type":"rect","r":1})"), StrictShape(StrictCircle{1}));
    EXPECT_EQ(yyjson::read<StrictBare>(R"({"id":1,"t":"rect","t":"circle","body":{"w":1,"h":2}})"),
              (StrictBare{1, StrictRect{1, 2}}));
    auto shape = StrictShape(StrictCircle{});
    EXPECT_THROW(yyjson::merge_into(R"({"type":"circle","type":"rect","r":1})", shape, yyjson::strict),
                 yyjson::bad_cast);

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

    // a patch that changes the alternative reads it anew, with the keys that it requires, also in the document that
    // merges the patch again after the direct read failed
    auto shape = StrictRequiredShape(StrictCircle{1});
    EXPECT_THROW(yyjson::merge_into(R"({"type":"required","a":1})", shape), yyjson::bad_cast);
    shape = StrictCircle{1};
    yyjson::merge_into(R"({"type":"required","a":1,"b":2})", shape);
    EXPECT_EQ(shape, StrictRequiredShape(StrictRequired{1, 2}));
    auto bare = StrictRequiredBare{StrictCircle{1}};
    EXPECT_THROW(yyjson::merge_into(R"({"t":"required","body":{"a":1}})", bare), yyjson::bad_cast);
    bare = StrictRequiredBare{StrictCircle{1}};
    yyjson::merge_into(R"({"body":{"a":1,"b":2},"t":"required"})", bare);
    EXPECT_EQ(bare, (StrictRequiredBare{StrictRequired{1, 2}}));
    s = StrictCircle{1};
    EXPECT_THROW(yyjson::merge_into(R"({"type":"rect","w":2})", s, yyjson::strict), yyjson::bad_cast);
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
    // the copy of a mutable value that a view is given takes the tag of its alternative
    expect_strict<StrictViewedShape>(R"({"type":"circle","r":1})", StrictViewedCircle{{1}});
    expect_rejected<StrictViewedShape>(R"({"type":"circle","r":1,"w":0})",
                                       R"(StrictCircle has no field of the key "w")", "/w");
    // a view cast without it while a strict read runs reads strictly
    EXPECT_EQ(yyjson::read<StrictScopedView>(R"({"x":1,"y":2})").p, (StrictPoint{1, 2, {}}));
    EXPECT_TRUE(read_directly<StrictScopedView>(R"({"x":1,"y":2})", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_FALSE(read_directly<StrictScopedView>(R"({"x":1,"y":2,"w":0})", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_THROW((void)yyjson::read<StrictScopedView>(R"({"x":1,"y":2,"w":0})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"y":2,"w":0})").cast<StrictScopedView>(), yyjson::bad_cast);
#endif
    EXPECT_EQ(yyjson::read<std::vector<StrictPoint>>(R"([{"x":1,"w":0}])"), (std::vector{StrictPoint{1, 0, {}}}));

    // and the reads after one read as before
    EXPECT_EQ(yyjson::read<StrictPoint>(R"({"x":1,"w":0})"), (StrictPoint{1, 0, {}}));
    EXPECT_EQ(yyjson::read(R"({"x":1,"w":0})").cast<StrictPoint>(), (StrictPoint{1, 0, {}}));
}

// NOLINTEND
