#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::as_array` reads and writes a struct as a JSON array whose elements are its fields in the order of their
// declarations
struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Point> = yyjson::annotate(yyjson::as_array);
// positions at the end that an object could leave out, and elements after them that are dropped
struct Rule
{
    int level = 0;
    std::optional<std::string> name;
    std::optional<int> value;
    bool operator==(const Rule&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Rule> =
    yyjson::annotate(yyjson::as_array, yyjson::items(true), yyjson::field("level", yyjson::minimum(0)));
// elements after the positions kept in a field, which constrains them
struct Series
{
    std::string label;
    std::vector<double> values;
    bool operator==(const Series&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Series> = yyjson::annotate(
    yyjson::as_array, yyjson::field("values", yyjson::extra_items, yyjson::items(yyjson::minimum(0))));
// no elements after the positions, a position that is always required and one with a default
struct Closed
{
    int a = 0;
    int b = 2;
    int c = 3;
    bool operator==(const Closed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Closed> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::field("b", yyjson::required),
                     yyjson::field("c", yyjson::omit_default));
// a skipped field is no position
struct Skipping
{
    int a = 0;
    int cache = 7;
    int b = 0;
    bool operator==(const Skipping&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Skipping> =
    yyjson::annotate(yyjson::as_array, yyjson::field("cache", yyjson::skip));
// a recursive struct
struct Node
{
    std::string name;
    std::vector<Node> children;
    bool operator==(const Node&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Node> =
    yyjson::annotate(yyjson::as_array, yyjson::field("children", yyjson::extra_items));
// positions that may be left out, one of which is always required
struct Gap
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const Gap&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Gap> = yyjson::annotate(yyjson::as_array);
struct MaybeInt
{
    std::optional<int> value;
    bool operator==(const MaybeInt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MaybeInt> = yyjson::annotate(yyjson::transparent);
struct RequiredTail
{
    int a = 0;
    MaybeInt b;
    bool operator==(const RequiredTail&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RequiredTail> =
    yyjson::annotate(yyjson::as_array, yyjson::field("b", yyjson::required));
struct TransparentGap
{
    MaybeInt a;
    int b = 0;
    bool operator==(const TransparentGap&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TransparentGap> =
    yyjson::annotate(yyjson::as_array, yyjson::field("b", yyjson::required));
struct DefaultFirst
{
    int a = 1;
    int b = 0;
    bool operator==(const DefaultFirst&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<DefaultFirst> =
    yyjson::annotate(yyjson::as_array, yyjson::field("a", yyjson::omit_default));
// the struct where other values are
struct Drawing
{
    Point origin;
    std::optional<Point> end;
    std::vector<Rule> rules;
    std::map<std::string, Series> series;
    std::variant<int, Point> where;
    bool operator==(const Drawing&) const = default;
};

#define AS_ARRAY_RULE(Name, ...)                                                       \
    struct Name                                                                        \
    {                                                                                  \
        int a;                                                                         \
        std::vector<int> e;                                                            \
    };                                                                                 \
    template <>                                                                        \
    inline constexpr auto yyjson::annotations<Name> = yyjson::annotate(__VA_ARGS__);
AS_ARRAY_RULE(ArrayTwice, yyjson::as_array, yyjson::as_array)
AS_ARRAY_RULE(ArrayRenamed, yyjson::as_array, yyjson::field("a", yyjson::rename("x")))
AS_ARRAY_RULE(ArrayExtraKeys, yyjson::as_array, yyjson::field("e", yyjson::extra_keys))
AS_ARRAY_RULE(ArrayClosedObject, yyjson::as_array, yyjson::additional_properties(false))
AS_ARRAY_RULE(ArrayDependent, yyjson::as_array, yyjson::dependent_required("a", {"e"}))
AS_ARRAY_RULE(ArrayItemsAlone, yyjson::items(false))
AS_ARRAY_RULE(ExtraItemsAlone, yyjson::field("e", yyjson::extra_items))
AS_ARRAY_RULE(ExtraItemsNotLast, yyjson::as_array, yyjson::field("a", yyjson::extra_items))
AS_ARRAY_RULE(ExtraItemsDenied, yyjson::as_array, yyjson::items(false), yyjson::field("e", yyjson::extra_items))
AS_ARRAY_RULE(ExtraItemsRequired, yyjson::as_array, yyjson::field("e", yyjson::extra_items, yyjson::required))
AS_ARRAY_RULE(ExtraItemsCounted, yyjson::as_array, yyjson::field("e", yyjson::extra_items, yyjson::min_items(1)))
AS_ARRAY_RULE(ExtraItemsConstrained, yyjson::as_array, yyjson::items(true),
              yyjson::field("e", yyjson::extra_items, yyjson::items(yyjson::maximum(9))),
              yyjson::field("a", yyjson::minimum(0)))
#undef AS_ARRAY_RULE
struct NoValue
{
    NoValue(int, int) {}
};
struct ArrayUnconvertible
{
    int a;
    std::vector<NoValue> b;
};
template <>
inline constexpr auto yyjson::annotations<ArrayUnconvertible> = yyjson::annotate(yyjson::as_array);
namespace as_array_rules
{
    template <typename T>
    constexpr auto table_error =
        yyjson::detail::make_field_key_table<T, yyjson::detail::reflected_field_names<T>>().error;
    using enum yyjson::detail::field_key_error;
    static_assert(table_error<ArrayTwice> == as_array_twice);
    static_assert(table_error<ArrayRenamed> == as_array_field_setting);
    static_assert(table_error<ArrayExtraKeys> == as_array_field_setting);
    static_assert(table_error<ArrayClosedObject> == as_array_struct_setting);
    static_assert(table_error<ArrayDependent> == as_array_struct_setting);
    static_assert(table_error<ArrayItemsAlone> == items_without_as_array);
    static_assert(table_error<ExtraItemsAlone> == extra_items_without_as_array);
    static_assert(table_error<ExtraItemsNotLast> == extra_items_not_last);
    static_assert(table_error<ExtraItemsDenied> == extra_items_denied);
    static_assert(table_error<ExtraItemsRequired> == extra_items_with_setting);
    static_assert(table_error<ExtraItemsCounted> == extra_items_with_setting);
    static_assert(table_error<ExtraItemsConstrained> == none);

    // a struct of a position that cannot be read or written is neither readable nor writable
    static_assert(!yyjson::readable<ArrayUnconvertible>);
    static_assert(!yyjson::writable<ArrayUnconvertible>);
    static_assert(yyjson::readable<Node> && yyjson::writable<Node>);
}  // namespace as_array_rules

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

    // every read of `json` gives `expected`, with a document and without one, into a value and merged into one;
    // `strict` tells whether `yyjson::strict` reads it too
    template <typename T>
    void expect_read(std::string_view json, const T& expected, bool strict = true)
    {
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        EXPECT_TRUE(expect_direct_read<T>(json)) << json;
        EXPECT_TRUE(expect_read_into(T(), json)) << json;
        static_cast<void>(expect_merge_into(T(), json));
        if (strict)
        {
            EXPECT_EQ(yyjson::read<T>(json, yyjson::strict), expected) << json;
            EXPECT_EQ(yyjson::read(json).cast<T>(yyjson::strict), expected) << json;
            auto into = T();
            yyjson::read_into(json, into, yyjson::strict);
            EXPECT_EQ(into, expected) << json;
        }
        else
        {
            EXPECT_THROW((void)yyjson::read<T>(json, yyjson::strict), yyjson::bad_cast) << json;
            EXPECT_THROW((void)yyjson::read(json).cast<T>(yyjson::strict), yyjson::bad_cast) << json;
        }
    }

    // every read of `json` throws `bad_cast` with `message` about the value at `path`; the reader without a
    // document leaves it to the document
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path,
                         bool strict_only = false)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_EQ(message_of(e).substr(0, message.size()), message) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::strict); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(yyjson::strict); }));
        if (strict_only) return;
        check(bad_cast_of([&] { (void)yyjson::read<T>(json); }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::read_into(json, t);
        }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(); }));
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
    }

    // `yyjson::validate(t)` and `yyjson::write(t, yyjson::strict)` throw `message` at `path`, and so does the strict
    // read of what `t` is written as
    template <typename T>
    void expect_invalid(const T& t, std::string_view message, std::string_view path)
    {
        const auto e = bad_cast_of([&] { yyjson::validate(t); });
        EXPECT_EQ(message_of(e), std::string(message) + (path.empty() ? "" : " at " + std::string(path)));
        EXPECT_EQ(e.path(), path);
        const auto w = bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); });
        EXPECT_EQ(std::string_view(w.what()), std::string_view(e.what()));
        const auto json = std::string(yyjson::write(t));
        const auto r = bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::strict); });
        EXPECT_TRUE(message_of(r).starts_with(message)) << json << "\n" << r.what();
        EXPECT_EQ(r.path(), path) << json;
    }

    template <typename T>
    void expect_valid(const T& t)
    {
        EXPECT_NO_THROW(yyjson::validate(t));
        EXPECT_EQ(std::string(yyjson::write(t, yyjson::strict)), std::string(yyjson::write(t)));
        EXPECT_EQ(yyjson::read<T>(std::string(yyjson::write(t)), yyjson::strict), t);
    }

    // writing gives `json` with and without a document
    template <typename T>
    void expect_written(const T& t, std::string_view json)
    {
        EXPECT_EQ(yyjson::write(t), json);
        EXPECT_EQ(yyjson::value(t).write(), json);
        expect_direct_write(t);
    }
}  // namespace

TEST(AsArray, Positions)
{
    expect_read<Point>("[1,2]", {1, 2});
    expect_written(Point{1, 2}, "[1,2]");
    expect_read<Skipping>("[1,2]", {1, 7, 2});
    expect_written(Skipping{1, 9, 2}, "[1,2]");

    // a missing position keeps its initializer, which `yyjson::strict` rejects unless an object could leave its key
    // out, and elements after the positions are dropped, which `yyjson::strict` rejects without `yyjson::items`
    expect_read<Point>("[1]", {1, 0}, false);
    expect_read<Point>("[]", {0, 0}, false);
    expect_read<Point>("[1,2,3]", {1, 2}, false);
    expect_rejected<Point>("[1]", "Point is not constructible from JSON without the element 1", "", true);
    expect_rejected<Point>("[1,2,3]", "Point has no field of the element 2", "/2", true);
    expect_read<Rule>("[1]", {1, std::nullopt, std::nullopt});
    expect_read<Rule>(R"([1,"a"])", {1, "a", std::nullopt});
    expect_read<Rule>(R"([1,"a",3,4,"x"])", {1, "a", 3});
    expect_rejected<Rule>("[]", "Rule is not constructible from JSON without the element 0", "", true);

    // `yyjson::required` rejects a missing position always, a position with a default may be missing, and
    // `yyjson::items(false)` rejects elements after the positions
    expect_read<Closed>("[1,4]", {1, 4, 3});
    expect_read<Closed>("[1,4,5]", {1, 4, 5});
    expect_rejected<Closed>("[1]", "Closed is not constructible from JSON without the element 1", "");
    expect_rejected<Closed>("[1,4,5,6]", "Closed has no field of the element 3", "/3");

    // the elements after the positions in a field
    expect_read<Series>(R"(["s"])", {"s", {}});
    expect_read<Series>(R"(["s",1,2.5])", {"s", {1, 2.5}});
    expect_written(Series{"s", {1, 2.5}}, R"(["s",1.0,2.5])");
    expect_written(Series{"s", {}}, R"(["s"])");

    // a JSON value other than an array, and a position of another type
    expect_rejected<Point>(R"({"x":1,"y":2})", "Point is not constructible from JSON", "");
    expect_rejected<Point>(R"([1,"2"])", "int is not constructible from JSON", "/1");
    expect_rejected<Rule>(R"([1,"a",null])", "std::optional<int> is not constructible from JSON null", "/2");
}

TEST(AsArray, Constraints)
{
    // a message names the field of a position, and the path its element
    expect_rejected<Rule>("[-1]", "the value of the field \"level\" of Rule is less than the minimum 0", "/0");
    expect_rejected<Series>(R"(["s",1,-2])", "an element of the value of the field \"values\" of Series is less than the minimum 0", "/2");
    expect_rejected<Drawing>(R"({"series":{"k":["s",-1]}})", "an element of the value of the field \"values\" of Series",
                             "/series/k/1");
    expect_rejected<Drawing>(R"({"rules":[[0],[-1]]})", "the value of the field \"level\" of Rule is less than the minimum 0",
                             "/rules/1/0");
}

TEST(AsArray, Writing)
{
    // only the omittable values at the end are left out; one before a value that is written is written as null
    expect_written(Rule{1, std::nullopt, std::nullopt}, "[1]");
    expect_written(Rule{1, "a", std::nullopt}, R"([1,"a"])");
    expect_written(Rule{1, std::nullopt, 3}, "[1,null,3]");
    expect_written(Closed{1, 4, 3}, "[1,4]");
    expect_written(Closed{1, 4, 5}, "[1,4,5]");
    expect_written(Point{}, "[0,0]");
}

TEST(AsArray, Nesting)
{
    const auto drawing = Drawing{{1, 2}, Point{3, 4}, {{0, "a", 1}, {1, {}, {}}}, {{"k", {"s", {1}}}}, Point{5, 6}};
    const auto json =
        R"({"origin":[1,2],"end":[3,4],"rules":[[0,"a",1],[1]],"series":{"k":["s",1.0]},"where":[5,6]})";
    expect_read<Drawing>(json, drawing);
    expect_written(drawing, json);
    auto where = Drawing();
    where.where = 3;
    expect_read<Drawing>(R"({"where":3})", where, false);

    // `read_into` and `merge_into` replace the value
    auto into = drawing;
    yyjson::read_into(R"({"origin":[7],"end":[8,9]})", into);
    EXPECT_EQ(into.origin, (Point{7, 0}));
    EXPECT_EQ(into.end, (Point{8, 9}));
    auto merged = drawing;
    yyjson::merge_into(R"({"origin":[7]})", merged);
    EXPECT_EQ(merged.origin, (Point{7, 0}));
    EXPECT_EQ(merged.end, (Point{3, 4}));
    static_cast<void>(expect_merge_into(drawing, R"({"origin":[7],"rules":[[2]]})"));

    // a recursive struct
    const auto tree = Node{"a", {{"b", {}}, {"c", {{"d", {}}}}}};
    expect_read<Node>(R"(["a",["b"],["c",["d"]]])", tree);
    expect_written(tree, R"(["a",["b"],["c",["d"]]])");
    expect_rejected<Node>(R"(["a",["b"],["c",[1]]])", "", "/2/1/0");
}

TEST(AsArray, Validate)
{
    expect_valid(Rule{1, std::nullopt, std::nullopt});
    expect_valid(Rule{1, "a", 3});
    expect_valid(Closed{1, 4, 3});
    expect_valid(Series{"s", {1, 2.5}});
    expect_valid(Gap{std::nullopt, std::nullopt});
    expect_valid(Gap{1, std::nullopt});
    expect_valid(RequiredTail{1, {2}});
    expect_valid(Node{"a", {{"b", {}}}});
    expect_valid(Drawing{{1, 2}, Point{3, 4}, {{0, "a", 1}}, {{"k", {"s", {1}}}}, Point{5, 6}});

    // an empty `std::optional` before a written value is written as null, which reading rejects; a value equal to
    // the default is written as it is
    expect_invalid(Gap{std::nullopt, 2}, "std::optional<int> is not constructible from JSON null", "/0");
    expect_invalid(TransparentGap{{std::nullopt}, 2}, "std::optional<int> is not constructible from JSON null", "/0");
    expect_valid(TransparentGap{{1}, 2});
    expect_valid(DefaultFirst{1, 5});
    EXPECT_EQ(yyjson::write(DefaultFirst{1, 5}), "[1,5]");

    // a value that `yyjson::required` keeps in the array, and the constraints of a position and of the elements
    // after the positions
    expect_invalid(RequiredTail{1, {}},
                   "RequiredTail is not constructible from JSON without the element 1 of the field \"b\"", "");
    expect_invalid(Rule{-1, std::nullopt, std::nullopt},
                   "the value of the field \"level\" of Rule is less than the minimum 0", "/0");
    expect_invalid(Series{"s", {1, -2}},
                   "an element of the value of the field \"values\" of Series is less than the minimum 0", "/2");
    expect_invalid(Drawing{{}, {}, {{0, {}, {}}, {-1, {}, {}}}, {}, 0},
                   "the value of the field \"level\" of Rule is less than the minimum 0", "/rules/1/0");
    expect_invalid(Drawing{{}, {}, {}, {{"k", {"s", {-1}}}}, 0},
                   "an element of the value of the field \"values\" of Series is less than the minimum 0",
                   "/series/k/1");
}
