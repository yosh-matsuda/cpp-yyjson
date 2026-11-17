#include "test_common.hpp"

// NOLINTBEGIN

// `bad_cast` names the JSON Pointer of the value that fails, and `yyjson::read<T>` its position in the text
struct PathItem
{
    int x = 0;
};
struct PathDoc
{
    std::vector<PathItem> items;
    std::map<std::string, int> m;
    std::variant<int, std::string> v;
    std::optional<PathItem> o;
};

// read by a caster that casts the members itself
struct PathPoint
{
    int x = 0;
    int y = 0;
};
// read in place by a caster
struct PathScale
{
    double value = 0;
};
struct PathUser
{
    PathPoint p;
    PathScale s;
};
// read by a caster that takes the value of a mutable document, in both forms
struct PathMutablePoint
{
    int x = 0;
};
struct PathMutableScale
{
    int value = 0;
};
struct PathMutableUser
{
    PathMutablePoint p;
    PathMutableScale s;
};

// the values of nested casts that fail, which name themselves as they throw
struct PathCArray
{
    int c[2] = {};
};
struct PathCircle
{
    int r = 0;
};
struct PathRect
{
    int w = 0;
};
using PathShape = std::variant<PathCircle, PathRect>;
template <>
inline constexpr auto yyjson::annotations<PathShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct PathAdjacent
{
    std::variant<PathCircle, PathRect> shape;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<PathAdjacent> =
    yyjson::annotate(yyjson::field("shape", yyjson::adjacent_tag("kind", {"circle", "rect"})));

#if YYJSON_VERSION_HEX >= 0x000C00
// read by a caster that casts its view, of a copy of a mutable value
struct PathViewed
{
    PathItem item;
};
template <>
struct yyjson::caster<PathViewed>
{
    static PathViewed from_json(const yyjson::json_view& v) { return {v.cast<PathItem>()}; }
};
#endif

template <>
struct yyjson::caster<PathPoint>
{
    static PathPoint from_json(const yyjson::reader::const_value_ref& v)
    {
        const auto o = v.as_object();
        if (!o.has_value()) throw yyjson::bad_cast("not an object");
        return {(*o)["x"].cast<int>(), (*o)["y"].cast<int>()};
    }
};
template <>
struct yyjson::caster<PathScale>
{
    static void from_json(PathScale& s, double v) noexcept { s.value = v; }
};
template <>
struct yyjson::caster<PathMutablePoint>
{
    static PathMutablePoint from_json(const yyjson::writer::const_value_ref& v)
    {
        return {(*v.as_object())["x"].cast<int>()};
    }
};
template <>
struct yyjson::caster<PathMutableScale>
{
    static void from_json(PathMutableScale& s, const yyjson::writer::const_value_ref& v) { s.value = v.cast<int>(); }
};

// the kinds of the failures that the library throws
struct KindPoint
{
    int x = 0;
    std::optional<int> y;
};
struct KindLimits
{
    int n = 0;
    std::string s;
    int odd = 0;
    int big = 0;
    yyjson::value flag;
    int m = 0;
};
inline bool kind_is_odd(const int& v) { return v % 2 != 0; }
template <>
inline constexpr auto yyjson::annotations<KindLimits> = yyjson::annotate(
    yyjson::field("n", yyjson::minimum(0)), yyjson::field("s", yyjson::max_length(2)),
    yyjson::field("odd", yyjson::satisfies<kind_is_odd>), yyjson::field("big", yyjson::not_(yyjson::minimum(10))),
    yyjson::field("flag", yyjson::matches<bool>),
    yyjson::field("m", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                            yyjson::else_(yyjson::minimum(0)))));
struct KindRequired
{
    std::vector<int> s;
    std::optional<int> k;
};
template <>
inline constexpr auto yyjson::annotations<KindRequired> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("s", yyjson::min_items(2))), yyjson::then(yyjson::required("k"))));
using KindOneOf = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
struct KindNested
{
    std::vector<KindNested> v;
};
// combinators of a struct, which report their own keyword in the place of that of an operand
struct KindAll
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<KindAll> = yyjson::annotate(
    yyjson::all_of(yyjson::property("x", yyjson::minimum(0)), yyjson::property("y", yyjson::maximum(5))));
struct KindIf
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<KindIf> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("x", yyjson::minimum(10))), yyjson::then(yyjson::property("y", yyjson::minimum(0))),
    yyjson::else_(yyjson::property("y", yyjson::maximum(0)))));
struct KindAllIf
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<KindAllIf> = yyjson::annotate(yyjson::all_of(yyjson::if_then_else(
    yyjson::if_(yyjson::property("x", yyjson::minimum(10))), yyjson::then(yyjson::property("y", yyjson::minimum(0))))));
struct KindAllRequired
{
    std::vector<int> s;
    std::optional<int> k;
};
template <>
inline constexpr auto yyjson::annotations<KindAllRequired> = yyjson::annotate(yyjson::all_of(yyjson::if_then_else(
    yyjson::if_(yyjson::property("s", yyjson::min_items(2))), yyjson::then(yyjson::required("k")))));
struct KindThenRequired
{
    std::vector<int> s;
    std::optional<int> k;
};
template <>
inline constexpr auto yyjson::annotations<KindThenRequired> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("s", yyjson::min_items(2))), yyjson::then(yyjson::required("k"))));
struct KindAllDependent
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<KindAllDependent> =
    yyjson::annotate(yyjson::all_of(yyjson::dependent_required("a", {"b"})));
struct KindDependent
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<KindDependent> = yyjson::annotate(yyjson::dependent_required("a", {"b"}));
struct KindFieldAll
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<KindFieldAll> =
    yyjson::annotate(yyjson::field("x", yyjson::all_of(yyjson::minimum(0), yyjson::maximum(5))));
struct KindOneKeys
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<KindOneKeys> =
    yyjson::annotate(yyjson::one_of(yyjson::required("a"), yyjson::required("b")));
struct KindAnyKeys
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<KindAnyKeys> =
    yyjson::annotate(yyjson::any_of(yyjson::required("a"), yyjson::required("b")));

// a caster that throws exceptions of other types than `bad_cast` for some values
struct Foreign
{
    int v = 0;
    bool operator==(const Foreign&) const = default;
};
template <>
struct yyjson::caster<Foreign>
{
    static Foreign from_json(int v)
    {
        if (v == 1) throw std::logic_error("logic");
        if (v == 2) throw std::runtime_error("runtime");
        if (v == 3) throw 3;
        if (v < 0) throw yyjson::bad_cast("negative");
        return {v};
    }
    template <typename... Ts>
    static int to_json(const Foreign& f, Ts...)
    {
        return f.v;
    }
};

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

    // the value at `path` fails, which starts at the first `marker` in `json`, in the first line
    template <typename T>
    void expect_path(std::string_view json, std::string_view path, std::string_view marker)
    {
        const auto pos = json.find(marker);
        ASSERT_NE(pos, std::string_view::npos);
        const auto position = yyjson::text_position{1, pos + 1, pos};
        const auto check = [&](const yyjson::bad_cast& e, bool has_position) {
            EXPECT_EQ(e.path(), path) << json;
            if (has_position)
            {
                EXPECT_EQ(e.position(), position) << json;
            }
            else
            {
                EXPECT_FALSE(e.position().has_value()) << json;
            }
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json); }), true);
        check(bad_cast_of([&] {
                  auto t = T();
                  yyjson::read_into(json, t);
              }),
              true);
        // a document knows no text
        check(bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); }), false);
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).template cast<T>(); }), false);
    }
}  // namespace

TEST(ErrorPath, NamesThePath)
{
    expect_path<PathDoc>(R"({"items":[{"x":1},{"x":"a"}]})", "/items/1/x", R"("a")");
    expect_path<PathDoc>(R"({"items":[{"x":1},2]})", "/items/1", "2");
    expect_path<PathDoc>(R"({"m":{"k":1,"l":[]}})", "/m/l", "[]");
    expect_path<PathDoc>(R"({"v":[1]})", "/v", "[1]");
    expect_path<PathDoc>(R"({"o":{"x":true}})", "/o/x", "true");
    expect_path<std::map<std::string, int>>(R"({"a/b~c":null})", "/a~1b~0c", "null");
    expect_path<std::vector<std::vector<int>>>("[[],[0,1,{}]]", "/1/2", "{}");
    expect_path<int>(R"(  "a")", "", R"("a")");

    // the paths into a value that a caster casts by itself, and to one that a caster reads in place
    expect_path<PathUser>(R"({"p":{"x":1,"y":"a"},"s":1})", "/p/y", R"("a")");
    expect_path<PathUser>(R"({"p":{"x":1,"y":2},"s":"a"})", "/s", R"("a")");
    expect_path<PathUser>(R"({"p":[],"s":1})", "/p", "[]");
    // the same through the copy that a caster of the other DOM API is given
    expect_path<PathMutableUser>(R"({"p":{"x":"a"},"s":1})", "/p/x", R"("a")");
    expect_path<PathMutableUser>(R"({"p":{"x":1},"s":"a"})", "/s", R"("a")");
#if YYJSON_VERSION_HEX >= 0x000C00
    expect_path<std::vector<PathViewed>>(R"([{"x":1},{"x":"a"}])", "/1/x", R"("a")");
#endif
}

TEST(ErrorPath, NamesTheValueThatFails)
{
    expect_path<std::vector<std::uint8_t>>("[1,300]", "/1", "300");
    expect_path<std::vector<std::int8_t>>("[1,-300]", "/1", "-300");
    expect_path<std::vector<int>>("[1,1.5]", "/1", "1.5");
    expect_path<std::vector<float>>("[1,1e39]", "/1", "1e39");
    expect_path<std::vector<std::string>>(R"(["a",true])", "/1", "true");
    expect_path<std::vector<int>>(R"([1,"a"])", "/1", R"("a")");
    expect_path<PathItem>(R"({"x":null})", "/x", "null");
    expect_path<std::vector<Color>>(R"(["red","pink"])", "/1", R"("pink")");
    expect_path<std::vector<Color>>(R"(["red",1])", "/1", "1");
    // a key names its member
    expect_path<std::map<Color, int>>(R"({"red":1,"pink":2})", "/pink", "2");
    expect_path<std::vector<std::array<int, 2>>>("[[1,2],[3]]", "/1", "[3]");
    expect_path<std::vector<std::tuple<int, int>>>("[[1,2],[3]]", "/1", "[3]");
    expect_path<std::vector<std::tuple<int, int>>>("[[1,2],{}]", "/1", "{}");
    expect_path<std::vector<PathCArray>>(R"([{"c":[1,2]},{"c":[1]}])", "/1/c", "[1]}");
    expect_path<std::vector<PathCArray>>(R"([{"c":{}}])", "/0/c", "{}");
    expect_path<std::vector<std::variant<int, std::string>>>("[1,[]]", "/1", "[]");
    expect_path<std::vector<PathShape>>(R"([{"type":"rect","w":1},{"type":"oval"}])", "/1/type", R"("oval")");
    expect_path<std::vector<PathShape>>(R"([{"type":1}])", "/0/type", "1");
    expect_path<std::vector<PathShape>>(R"([{"type":"rect"},{"w":1}])", "/1", R"({"w")");
    expect_path<std::vector<PathShape>>("[[]]", "/0", "[]]");
}

TEST(ErrorPath, What)
{
    const auto json = std::string_view(R"({"items":[{"x":"a"}]})");
    auto e = bad_cast_of([&] { (void)yyjson::read<PathDoc>(json); });
    EXPECT_STREQ(e.what(), "int is not constructible from JSON string at /items/0/x (line 1, column 16, pos 15)");
    e = bad_cast_of([&] { (void)yyjson::read(json).cast<PathDoc>(); });
    EXPECT_STREQ(e.what(), "int is not constructible from JSON string at /items/0/x");
    e = bad_cast_of([&] { (void)yyjson::read<int>(R"("a")"); });
    EXPECT_STREQ(e.what(), "int is not constructible from JSON string (line 1, column 1, pos 0)");
    EXPECT_EQ(e.path(), "");

    // a null character ends the C string of `what()`, so control characters are written escaped there
    e = bad_cast_of([&] { (void)yyjson::read(R"({"a\u0000\nb":"x"})").cast<std::map<std::string, int>>(); });
    EXPECT_EQ(e.path(), std::string_view("/a\0\nb", 5));
    EXPECT_STREQ(e.what(), R"(int is not constructible from JSON string at /a\u0000\u000ab)");

    // a copy shares the path
    const auto copy = e;
    EXPECT_EQ(copy.path(), e.path());
    EXPECT_STREQ(copy.what(), e.what());

    // an exception that the user throws has none
    EXPECT_EQ(yyjson::bad_cast("x").path(), "");
    EXPECT_FALSE(yyjson::bad_cast("x").position().has_value());
    EXPECT_STREQ(yyjson::bad_cast("x").what(), "x");
}

TEST(ErrorPath, Position)
{
    // lines and columns count characters, `pos` bytes
    const auto json = std::string_view("{\"m\":{},\n \"\xC3\xA9\": {\"a/b~c\": \"x\"}}");
    const auto e = bad_cast_of([&] { (void)yyjson::read<std::map<std::string, std::map<std::string, int>>>(json); });
    EXPECT_EQ(e.path(), "/\xC3\xA9/a~1b~0c");
    EXPECT_EQ(e.position(), (yyjson::text_position{2, 17, json.find("\"x\"")}));

    // merging names the path from the root
    auto doc = PathDoc();
    auto m = bad_cast_of([&] { yyjson::merge_into("{\"m\":\n{\"k\":\"a\"}}", doc); });
    EXPECT_EQ(m.path(), "/m/k");
    EXPECT_EQ(m.position(), (yyjson::text_position{2, 6, 11}));
    m = bad_cast_of([&] { yyjson::merge_into(R"({"items":[{"x":1},{"x":[]}]})", doc); });
    EXPECT_EQ(m.path(), "/items/1/x");
    EXPECT_EQ(m.position(), (yyjson::text_position{1, 24, 23}));

    // the null members that a merge removes from a new value are still in the text
    for (const auto text : {std::string_view(R"({"x":{"z":null,"a":{"z":null,"k":"no"}}})"),
                            std::string_view(R"({"x":{"a":{"k":1,"k":null,"k":2,"k":"no"}}})")})
    {
        auto maps = std::map<std::string, std::map<std::string, std::map<std::string, int>>>();
        const auto n = bad_cast_of([&] { yyjson::merge_into(text, maps); });
        EXPECT_EQ(n.path(), "/x/a/k");
        EXPECT_EQ(n.position(), (yyjson::text_position{1, text.find("\"no\"") + 1, text.find("\"no\"")}));
    }
    // a direct merge leaves such a value to the document
    auto pairs = std::map<std::string, std::vector<std::pair<std::string, std::optional<int>>>>();
    const auto pairs_text = std::string_view(R"({"x":{"z":null,"k":"no"}})");
    const auto p = bad_cast_of([&] { yyjson::merge_into(pairs_text, pairs); });
    EXPECT_EQ(p.path(), "/x/k");
    const auto at = pairs_text.find("\"no\"");
    EXPECT_EQ(p.position(), (yyjson::text_position{1, at + 1, at}));

    // a comment before the value leaves its position unknown, one after it does not
    auto c = bad_cast_of(
        [] { (void)yyjson::read<std::vector<int>>(R"([1, /* c */ "a"])", yyjson::ReadFlag::AllowComments); });
    EXPECT_EQ(c.path(), "/1");
    EXPECT_FALSE(c.position().has_value());
    c = bad_cast_of([] { (void)yyjson::read<std::vector<int>>(R"(["a"] // c)", yyjson::ReadFlag::AllowComments); });
    EXPECT_EQ(c.path(), "/0");
    EXPECT_EQ(c.position(), (yyjson::text_position{1, 2, 1}));

#if YYJSON_VERSION_HEX >= 0x000B00
    // a byte order mark is not a character
    const auto bom =
        bad_cast_of([] { (void)yyjson::read<std::vector<int>>("\xEF\xBB\xBF[1,\"a\"]", yyjson::ReadFlag::AllowBom); });
    EXPECT_EQ(bom.path(), "/1");
    EXPECT_EQ(bom.position(), (yyjson::text_position{1, 4, 6}));
#endif
#if YYJSON_VERSION_HEX >= 0x000C00
    // whitespace that JSON does not have is not looked for
    const auto ws =
        bad_cast_of([] { (void)yyjson::read<std::vector<int>>(R"(["a"])", yyjson::ReadFlag::AllowExtWhitespace); });
    EXPECT_EQ(ws.path(), "/0");
    EXPECT_FALSE(ws.position().has_value());
#endif
}

TEST(ErrorPath, AdjacentTag)
{
    // the value may come before its tag, and the tag after the value
    expect_path<std::vector<PathAdjacent>>(R"([{"kind":"rect","shape":{"w":"a"}}])", "/0/shape/w", R"("a")");
    expect_path<std::vector<PathAdjacent>>(R"([{"shape":{"w":"a"},"kind":"rect"}])", "/0/shape/w", R"("a")");
    expect_path<std::vector<PathAdjacent>>(R"([{"shape":{"w":1},"kind":"oval"}])", "/0/kind", R"("oval")");
    expect_path<std::vector<PathAdjacent>>(R"([{"kind":"rect","shape":[]}])", "/0/shape", "[]");

    // the lines of a value deep in the text
    const auto json = std::string_view("[\n{\"kind\": \"circle\",\n \"shape\": {\"r\": 1.5}}]");
    const auto e = bad_cast_of([&] { (void)yyjson::read<std::vector<PathAdjacent>>(json); });
    EXPECT_EQ(e.path(), "/0/shape/r");
    EXPECT_EQ(e.position(), (yyjson::text_position{3, 17, json.find("1.5")}));

    auto v = std::vector<PathAdjacent>();
    const auto m = bad_cast_of([&] { yyjson::merge_into(R"([{"kind":"circle","shape":{"r":true}}])", v); });
    EXPECT_EQ(m.path(), "/0/shape/r");
    EXPECT_EQ(m.position(), (yyjson::text_position{1, 32, 31}));
}

TEST(ErrorPath, Kind)
{
    using enum yyjson::error_kind;
    // each way of reading gives the same kind
    const auto expect_kind = [](yyjson::error_kind kind, auto read) {
        EXPECT_EQ(bad_cast_of([&] { read(std::false_type()); }).kind(), kind);
        EXPECT_EQ(bad_cast_of([&] { read(std::true_type()); }).kind(), kind);
    };
    const auto kind_of = [&]<typename T>(std::type_identity<T>, std::string_view json, yyjson::error_kind kind,
                                         auto... policies) {
        expect_kind(kind, [&](auto dom) {
            if constexpr (dom)
                (void)yyjson::read(json).template cast<T>(policies...);
            else
                (void)yyjson::read<T>(json, policies...);
        });
    };
    kind_of(std::type_identity<int>(), R"("a")", type);
    kind_of(std::type_identity<int>(), "1.5", type);
    kind_of(std::type_identity<std::vector<int>>(), "{}", type);
    kind_of(std::type_identity<KindPoint>(), "[]", type);
    kind_of(std::type_identity<std::uint8_t>(), "300", out_of_range);
    kind_of(std::type_identity<float>(), "1e39", out_of_range);
    kind_of(std::type_identity<std::array<int, 2>>(), "[1]", size);
    kind_of(std::type_identity<std::tuple<int, int>>(), "[1,2,3]", size);
    kind_of(std::type_identity<KindPoint>(), R"({"x":1,"z":2})", unknown_key, yyjson::reject_unknown_keys);
    kind_of(std::type_identity<KindPoint>(), R"({"y":1})", missing_key, yyjson::reject_missing_keys);
    kind_of(std::type_identity<KindPoint>(), R"({"x":1,"x":2})", duplicate, yyjson::reject_duplicates);
    kind_of(std::type_identity<std::set<int>>(), "[1,1]", duplicate, yyjson::reject_duplicates);
    kind_of(std::type_identity<KindPoint>(), R"({"x":1,"y":null})", null, yyjson::reject_null_as_missing);
    kind_of(std::type_identity<Color>(), R"("pink")", unknown_enum);
    kind_of(std::type_identity<PathShape>(), R"({"type":"oval"})", tag);
    kind_of(std::type_identity<PathShape>(), R"({"r":1})", tag);
    kind_of(std::type_identity<std::variant<int, std::string>>(), "[]", no_alternative);
    kind_of(std::type_identity<KindOneOf>(), "5", ambiguous_alternative);
    kind_of(std::type_identity<KindLimits>(), R"({"n":-1})", keyword);
    // a cast that a user caster throws by itself
    kind_of(std::type_identity<Foreign>(), "-1", other);
    EXPECT_EQ(yyjson::bad_cast("x").kind(), other);
    EXPECT_EQ(yyjson::bad_cast("x").keyword(), "");

    auto deep = std::string();
    for (auto i = 0; i < 300; ++i) deep += R"({"v":[)";
    for (auto i = 0; i < 300; ++i) deep += "]}";
    kind_of(std::type_identity<KindNested>(), deep, recursion_limit);
    EXPECT_THROW((void)yyjson::read<KindNested>(deep), yyjson::detail::recursion_limit_error);

    // a copy keeps the kind
    const auto e = bad_cast_of([] { (void)yyjson::read<std::uint8_t>("300"); });
    const auto copy = e;
    EXPECT_EQ(copy.kind(), out_of_range);
}

TEST(ErrorPath, Keyword)
{
    // the keyword of the setting that fails, as each way of reading gives it
    const auto expect_keyword = [](std::string_view json, std::string_view keyword, auto type) {
        using T = typename decltype(type)::type;
        for (const auto& e : {bad_cast_of([&] { (void)yyjson::read<T>(json); }),
                              bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); })})
        {
            EXPECT_EQ(e.kind(), yyjson::error_kind::keyword) << json;
            EXPECT_EQ(e.keyword(), keyword) << json;
        }
    };
    const auto limits = std::type_identity<KindLimits>();
    expect_keyword(R"({"n":-1})", "minimum", limits);
    expect_keyword(R"({"s":"abc"})", "maxLength", limits);
    expect_keyword(R"({"odd":2})", "satisfies", limits);
    expect_keyword(R"({"flag":1})", "matches", limits);
    // a combinator reports itself, and a condition the branch that fails
    expect_keyword(R"({"big":10})", "not", limits);
    expect_keyword(R"({"m":11})", "then", limits);
    expect_keyword(R"({"m":-1})", "else", limits);
    expect_keyword(R"({"s":[1,2]})", "required", std::type_identity<KindRequired>());

    // a key that the read does not take is no keyword
    const auto e = bad_cast_of([] { (void)yyjson::read<KindPoint>(R"({"x":1,"z":2})", yyjson::reject_unknown_keys); });
    EXPECT_EQ(e.keyword(), "");
}

TEST(ErrorPath, OtherExceptionsPassThroughVariants)
{
    // an exception of another type than `bad_cast` from an alternative ends the read of the variant
    const auto expect_passed = []<typename V>(std::type_identity<V>) {
        const auto read_both = [](std::string_view json) {
            (void)yyjson::read<V>(json);
            return yyjson::read(json).template cast<V>();
        };
        EXPECT_THROW((void)yyjson::read<V>("1"), std::logic_error);
        EXPECT_THROW((void)yyjson::read("1").template cast<V>(), std::logic_error);
        EXPECT_THROW((void)yyjson::read<V>("2"), std::runtime_error);
        EXPECT_THROW((void)yyjson::read("2").template cast<V>(), std::runtime_error);
        EXPECT_THROW((void)yyjson::read<V>("3"), int);
        EXPECT_THROW((void)yyjson::read("3").template cast<V>(), int);
        EXPECT_NO_THROW((void)read_both("4"));
    };
    expect_passed(std::type_identity<std::variant<Foreign, double>>());
    expect_passed(std::type_identity<yyjson::any_of_variant<Foreign, double>>());
    expect_passed(std::type_identity<yyjson::one_of_variant<Foreign, std::string>>());
    // a `bad_cast` still lets the next alternative read the value
    using Plain = std::variant<Foreign, double>;
    EXPECT_EQ(yyjson::read<Plain>("-1").index(), 1);
    EXPECT_EQ(yyjson::read("-1").cast<Plain>().index(), 1);
    EXPECT_THROW((void)yyjson::read<std::vector<Plain>>("[0,1]"), std::logic_error);
    using Named = std::map<std::string, std::variant<Foreign, std::string>>;
    EXPECT_THROW((void)yyjson::read<Named>(R"({"a":3})"), int);

    // the check of the written value reads it back into the other alternatives
    using Both = yyjson::one_of_variant<long long, Foreign>;
    EXPECT_THROW(yyjson::validate(Both(1LL)), std::logic_error);
    EXPECT_THROW((void)yyjson::write(Both(2LL), yyjson::with_validation), std::runtime_error);
    EXPECT_THROW(yyjson::validate(Both(3LL)), int);
}

namespace
{
    // the keyword of the `bad_cast` of `json` for T and of the value `t`, from each way of reading and checking
    template <typename T>
    void expect_keyword_everywhere(std::string_view json, const T& t, std::string_view keyword)
    {
        for (const auto& e : {bad_cast_of([&] { (void)yyjson::read<T>(json); }),
                              bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); }), bad_cast_of([&] {
                                  auto merged = T();
                                  yyjson::merge_into(json, merged);
                              }),
                              bad_cast_of([&] { yyjson::validate(t); })})
        {
            EXPECT_EQ(e.kind(), yyjson::error_kind::keyword) << json;
            EXPECT_EQ(e.keyword(), keyword) << json;
        }
    }
}  // namespace

TEST(ErrorPath, CombinatorKeyword)
{
    // a combinator reports its own keyword, and a condition the branch that it takes
    expect_keyword_everywhere(R"({"x":-1,"y":0})", KindAll{-1, 0}, "allOf");
    expect_keyword_everywhere(R"({"x":0,"y":6})", KindAll{0, 6}, "allOf");
    expect_keyword_everywhere(R"({"x":10,"y":-1})", KindIf{10, -1}, "then");
    expect_keyword_everywhere(R"({"x":0,"y":1})", KindIf{0, 1}, "else");
    expect_keyword_everywhere(R"({"x":-1})", KindFieldAll{-1}, "allOf");
    // the outermost combinator
    expect_keyword_everywhere(R"({"x":10,"y":-1})", KindAllIf{10, -1}, "allOf");
    // a key that `required` requires inside a combinator, and a key that `dependent_required` requires
    expect_keyword_everywhere(R"({"s":[1,2]})", KindAllRequired{{1, 2}, {}}, "required");
    expect_keyword_everywhere(R"({"s":[1,2]})", KindThenRequired{{1, 2}, {}}, "required");
    expect_keyword_everywhere(R"({"a":1})", KindDependent{1, {}}, "dependentRequired");
    expect_keyword_everywhere(R"({"a":1})", KindAllDependent{1, {}}, "allOf");
}

TEST(ErrorPath, RequiredGroupsKeyword)
{
    // no group or more than one for `one_of`, and no group for `any_of`
    expect_keyword_everywhere("{}", KindOneKeys(), "oneOf");
    expect_keyword_everywhere(R"({"a":1,"b":2})", KindOneKeys{1, 2}, "oneOf");
    expect_keyword_everywhere("{}", KindAnyKeys(), "anyOf");
    EXPECT_NO_THROW((void)yyjson::read<KindAnyKeys>(R"({"a":1,"b":2})"));
    EXPECT_NO_THROW(yyjson::validate(KindAnyKeys{1, 2}));
}

TEST(ErrorPath, KeyOutlivesTheDocument)
{
    // the record names the key after the text and the document of the read are gone
    auto copy = yyjson::bad_cast("none");
    {
        auto json = std::string(R"({"x":null})");
        auto p = KindPoint();
        copy = bad_cast_of([&] { yyjson::merge_into(json, p, yyjson::reject_missing_keys); });
        json.assign(json.size(), ' ');
    }
    EXPECT_EQ(copy.kind(), yyjson::error_kind::null);
    EXPECT_EQ(yyjson::detail::bad_cast_access::failure(copy).key, "x");
    EXPECT_EQ(yyjson::detail::bad_cast_access::failure(copy).owner, yyjson::detail::type_name<KindPoint>());
}

TEST(ErrorPath, DirectReadKeepsTheExceptionOfTheDocument)
{
    // a read without a document gives the exception that the cast of a document gives for the same text
    const auto same = []<typename T, typename E>(std::type_identity<T>, std::type_identity<E>, std::string_view json) {
        EXPECT_THROW((void)yyjson::read<T>(json), E) << json;
        EXPECT_THROW((void)yyjson::read(json).template cast<T>(), E) << json;
    };
    using Single = std::tuple<Foreign>;
    using Pair = std::tuple<Foreign, int>;
    // the size of the array is checked before the element is read
    same(std::type_identity<Single>(), std::type_identity<yyjson::bad_cast>(), "[1,2]");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<Single>("[1,2]"); }).kind(), yyjson::error_kind::size);
    same(std::type_identity<Pair>(), std::type_identity<std::logic_error>(), "[1,2]");
    same(std::type_identity<Pair>(), std::type_identity<std::runtime_error>(), "[2,2]");
    same(std::type_identity<Pair>(), std::type_identity<int>(), "[3,2]");
    using Sized = std::variant<Single, std::string>;
    same(std::type_identity<Sized>(), std::type_identity<yyjson::bad_cast>(), "[1,2]");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<Sized>("[1,2]"); }).kind(), yyjson::error_kind::no_alternative);
    using Passed = std::variant<Pair, std::string>;
    same(std::type_identity<Passed>(), std::type_identity<std::logic_error>(), "[1,2]");
    same(std::type_identity<Passed>(), std::type_identity<std::runtime_error>(), "[2,2]");
    same(std::type_identity<Passed>(), std::type_identity<int>(), "[3,2]");
}

TEST(ErrorPath, DirectReadRecordsTheFacts)
{
    // the record of a read without a document holds what the cast of a document records, also in a copy
    const auto same = []<typename T>(std::type_identity<T>, std::string_view json,
                                     yyjson::ReadFlag flg = yyjson::ReadFlag::NoFlag) {
        const auto direct = bad_cast_of([&] { (void)yyjson::read<T>(json, flg); });
        const auto copy = direct;
        const auto dom = bad_cast_of([&] { (void)yyjson::read(json, flg).template cast<T>(); });
        const auto& expected = yyjson::detail::bad_cast_access::failure(dom);
        for (const auto* e : {&direct, &copy})
        {
            const auto& got = yyjson::detail::bad_cast_access::failure(*e);
            EXPECT_EQ(got.kind, expected.kind) << json;
            EXPECT_EQ(got.got, expected.got) << json;
            EXPECT_NE(got.got, 0) << json;
            EXPECT_EQ(got.input, expected.input) << json;
            EXPECT_EQ(got.text, expected.text) << json;
        }
    };
    same(std::type_identity<int>(), R"("x")");
    same(std::type_identity<std::string>(), "12");
    same(std::type_identity<bool>(), "1.5");
    same(std::type_identity<std::uint8_t>(), "300");
    // a number as the document parses it
    same(std::type_identity<bool>(), "1.0");
    same(std::type_identity<bool>(), "1e2");
    same(std::type_identity<bool>(), "-0.0");
    same(std::type_identity<bool>(), "123456789012345678901234567890");
    same(std::type_identity<std::string>(), "[1.0]");
    same(std::type_identity<std::vector<bool>>(), "[true, 1.0]");
    same(std::type_identity<bool>(), "NaN", yyjson::ReadFlag::AllowInfAndNan);
    same(std::type_identity<bool>(), "-Infinity", yyjson::ReadFlag::AllowInfAndNan);
    same(std::type_identity<bool>(), "1e400", yyjson::ReadFlag::AllowInfAndNan);
    same(std::type_identity<std::vector<int>>(), R"([1,"x"])");
    same(std::type_identity<KindPoint>(), R"({"x":-1.5})");
    same(std::type_identity<PathShape>(), R"({"type":"oval","r":1})");
    same(std::type_identity<std::vector<PathShape>>(), R"([{"type":"oval"}])");
    const auto tag = bad_cast_of([] { (void)yyjson::read<PathShape>(R"({"type":"oval"})"); });
    EXPECT_EQ(yyjson::detail::bad_cast_access::failure(tag).input, "oval");
}

// NOLINTEND
