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

// NOLINTEND
