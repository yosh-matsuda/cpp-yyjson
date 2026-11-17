#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::nullable<T>`, which reads JSON null as an empty value and writes an empty value as null

struct NulPoint
{
    int x = 1;
    int y = 2;
    bool operator==(const NulPoint&) const = default;
};

struct NulFields
{
    yyjson::nullable<int> n;
    yyjson::nullable<std::string> s = "s";
    yyjson::nullable<NulPoint> p;
    yyjson::nullable<NulPoint> q = NulPoint{5, 6};
    std::vector<yyjson::nullable<double>> v;
    std::map<std::string, yyjson::nullable<int>> m;
    bool operator==(const NulFields&) const = default;
};

struct NulFieldsV
{
    yyjson::nullable<int> n;
    yyjson::nullable<NulPoint> p;
    bool operator==(const NulFieldsV&) const = default;
};
VISITABLE_STRUCT(NulFieldsV, n, p);

// a key that may be absent, null, or a value
struct NulThreeStates
{
    std::optional<yyjson::nullable<int>> t;
    bool operator==(const NulThreeStates&) const = default;
};

namespace
{
    template <typename T>
    void expect_nullable(std::string_view json)
    {
        EXPECT_TRUE(expect_direct_read<T>(json)) << json;
        EXPECT_TRUE(expect_read_into(T(), json)) << json;
        const auto t = yyjson::read<T>(json);
        expect_direct_write(t);
        EXPECT_EQ(yyjson::read<T>(yyjson::write(t)), t) << json;
    }
}  // namespace

TEST(Nullable, Values)
{
    EXPECT_EQ(written(yyjson::read<yyjson::nullable<int>>("null")), "null");
    EXPECT_EQ(written(yyjson::read<yyjson::nullable<int>>("3")), "3");
    EXPECT_EQ(written(yyjson::read<std::vector<yyjson::nullable<std::string>>>(R"(["a",null])")), R"(["a",null])");
    EXPECT_EQ(written(std::tuple<yyjson::nullable<int>, int>(yyjson::nullable<int>(), 1)), "[null,1]");
    EXPECT_EQ(written(yyjson::nullable<NulPoint>()), "null");
    expect_nullable<yyjson::nullable<int>>("null");
    expect_nullable<yyjson::nullable<int>>("7");
    expect_nullable<yyjson::nullable<NulPoint>>("null");
    expect_nullable<yyjson::nullable<NulPoint>>(R"({"y":3})");
    expect_nullable<std::vector<yyjson::nullable<int>>>("[1,null,3]");
    expect_nullable<std::map<std::string, yyjson::nullable<NulPoint>>>(R"({"a":{"x":3},"b":null})");
    expect_nullable<std::tuple<yyjson::nullable<int>, yyjson::nullable<std::string>>>(R"([null,"a"])");
    // a DOM value reads and writes it too
    EXPECT_FALSE(yyjson::read("null").cast<yyjson::nullable<int>>().has_value());
    EXPECT_EQ(yyjson::value(yyjson::nullable<int>(4)).write(), "4");
    EXPECT_EQ(yyjson::value(yyjson::nullable<int>()).write(), "null");
    // a value of another kind is not read as an empty value
    EXPECT_EQ(read_or_error([] { return yyjson::read<yyjson::nullable<int>>(R"("a")"); }),
              read_or_error([] { return yyjson::read<int>(R"("a")"); }));
}

TEST(Nullable, Fields)
{
    expect_nullable<NulFields>(R"({"n":1,"s":"a","p":{"x":3},"q":{"y":4},"v":[1.5,null],"m":{"a":null,"b":2}})");
    expect_nullable<NulFields>(R"({"n":null,"s":null,"p":null,"q":null,"v":[null],"m":{"a":null}})");
    expect_nullable<NulFieldsV>(R"({"n":null,"p":{"x":3}})");
    expect_nullable<NulFieldsV>(R"({"n":2,"p":null})");
    // an empty field is written as null, and its key stays
    auto empty = NulFields();
    empty.s.reset();
    empty.q.reset();
    EXPECT_EQ(written(empty), R"({"n":null,"s":null,"p":null,"q":null,"v":[],"m":{}})");
    EXPECT_EQ(written(NulFieldsV{}), R"({"n":null,"p":null})");
    // an object is read over a value that the field holds, as for a field of the type of that value
    const auto t = yyjson::read<NulFields>(R"({"q":{"x":4}})");
    EXPECT_EQ(*t.q, (NulPoint{4, 2}));
    EXPECT_EQ(*t.s, "s");
}

TEST(Nullable, Merge)
{
    auto t = NulFields();
    t.n = 1;
    t.p = NulPoint{3, 4};
    EXPECT_EQ(expect_merge_into(t, R"({"p":{"x":7}})"),
              R"({"n":1,"s":"s","p":{"x":7,"y":4},"q":{"x":5,"y":6},"v":[],"m":{}})");
    // null resets a field, as RFC 7396 removes the member
    EXPECT_EQ(expect_merge_into(t, R"({"n":null,"p":null,"q":null})"),
              R"({"n":null,"s":"s","p":null,"q":{"x":5,"y":6},"v":[],"m":{}})");
    // an empty field takes the merged object as a new value
    EXPECT_EQ(expect_merge_into(NulFields{}, R"({"p":{"y":9}})"),
              R"({"n":null,"s":"s","p":{"x":1,"y":9},"q":{"x":5,"y":6},"v":[],"m":{}})");
}

TEST(Nullable, ThreeStates)
{
    expect_nullable<NulThreeStates>("{}");
    expect_nullable<NulThreeStates>(R"({"t":null})");
    expect_nullable<NulThreeStates>(R"({"t":5})");
    EXPECT_FALSE(yyjson::read<NulThreeStates>("{}").t.has_value());
    EXPECT_TRUE(yyjson::read<NulThreeStates>(R"({"t":null})").t.has_value());
    EXPECT_FALSE(yyjson::read<NulThreeStates>(R"({"t":null})").t->has_value());
    EXPECT_EQ(written(NulThreeStates{}), "{}");
    EXPECT_EQ(written(NulThreeStates{yyjson::nullable<int>()}), R"({"t":null})");
    EXPECT_EQ(written(NulThreeStates{yyjson::nullable<int>(5)}), R"({"t":5})");
    // the key of a field of nullable may not be left out under strict, and null is not a missing key
    EXPECT_EQ(read_or_error([] { return yyjson::read<NulFieldsV>(R"({"p":null})", yyjson::strict); }),
              read_or_error([] { return yyjson::read(R"({"p":null})").cast<NulFieldsV>(yyjson::strict); }));
    EXPECT_EQ(read_or_error([] { return yyjson::read<NulFieldsV>(R"({"p":null})", yyjson::strict); }),
              "bad cast: a required key is missing; reading the key \"n\" of NulFieldsV");
    EXPECT_EQ(read_or_error([] { return yyjson::read<NulFieldsV>(R"({"n":null,"p":null})", yyjson::strict); }),
              R"({"n":null,"p":null})");
    EXPECT_EQ(read_or_error([] {
                  return yyjson::read<NulThreeStates>(R"({"t":null})", yyjson::reject_null_as_missing);
              }),
              R"({"t":null})");
}

TEST(Nullable, Comparisons)
{
    const auto e = yyjson::nullable<int>();
    const auto one = yyjson::nullable<int>(1);
    EXPECT_TRUE(e == std::nullopt);
    EXPECT_FALSE(one == std::nullopt);
    EXPECT_TRUE(one == 1);
    EXPECT_TRUE(1 == one);
    EXPECT_FALSE(e == 1);
    EXPECT_TRUE(e < one);
    EXPECT_TRUE(one == yyjson::nullable<long>(1));
    EXPECT_TRUE(yyjson::nullable(NulPoint{}) == NulPoint{});
    static_assert(std::same_as<decltype(yyjson::nullable(1)), yyjson::nullable<int>>);
    static_assert(!std::is_convertible_v<yyjson::nullable<int>, bool>);
    static_assert(yyjson::readable<yyjson::nullable<NulPoint>> && yyjson::writable<yyjson::nullable<NulPoint>>);
}

// NOLINTEND
