#include <cmath>
#include <cstdlib>
#include <memory>
#include <new>
#include "test_common.hpp"
#include "test_pattern_matcher.hpp"

// NOLINTBEGIN

// `yyjson::validate(t)` and `yyjson::write(t, ..., yyjson::with_validation)`, which check a value against the
// keywords of its type before it is written

namespace
{
    template <typename F>
    yyjson::bad_cast validation_error(F&& f)
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

    std::string with_path(std::string_view message, std::string_view path)
    {
        return std::string(message) + (path.empty() ? "" : " at " + std::string(path));
    }

    // Each form of the write with `yyjson::with_validation`, by the text that it writes.
    template <typename T>
    std::vector<std::string> checked_writes(const T& t)
    {
        using yyjson::with_validation, yyjson::WriteFlag;
        auto texts = std::vector<std::string>();
        texts.emplace_back(yyjson::write(t, with_validation));
        texts.emplace_back(yyjson::write(t, WriteFlag::NoFlag, with_validation));
        auto out = std::string("previous");
        yyjson::write(t, out, with_validation);
        texts.push_back(out);
        out = "previous";
        yyjson::write(t, out, WriteFlag::NoFlag, with_validation);
        texts.push_back(out);
        return texts;
    }

    // `t` passes the check, each checked write gives the text of the plain write, and the read of that text
    // gives a value that is written the same.
    template <typename T>
    void expect_valid(const T& t)
    {
        EXPECT_NO_THROW(yyjson::validate(t));
        const auto json = std::string(yyjson::write(t));
        for (const auto& text : checked_writes(t)) EXPECT_EQ(text, json);
        if constexpr (yyjson::readable<T>)
        {
            const auto back = yyjson::read<T>(json);
            EXPECT_EQ(std::string(yyjson::write(back)), json);
        }
    }

    enum class read_leg
    {
        checked,
        skipped
    };

    // `yyjson::validate(t)` throws `message` at `path`, each checked write throws the same and leaves `out` as it
    // was, and a read of the text of the plain write throws the same too, unless `leg` skips it.
    template <typename T>
    void expect_invalid(const T& t, std::string_view message, std::string_view path, read_leg leg = read_leg::checked)
    {
        using yyjson::with_validation, yyjson::WriteFlag;
        const auto e = validation_error([&] { yyjson::validate(t); });
        EXPECT_EQ(std::string_view(e.what()), with_path(message, path));
        EXPECT_EQ(e.path(), path);
        EXPECT_FALSE(e.position().has_value());

        const auto expect_same = [&](auto&& f) {
            const auto w = validation_error(f);
            EXPECT_EQ(std::string_view(w.what()), std::string_view(e.what()));
            EXPECT_EQ(w.path(), path);
        };
        expect_same([&] { (void)yyjson::write(t, with_validation); });
        expect_same([&] { (void)yyjson::write(t, WriteFlag::NoFlag, with_validation); });
        auto out = std::string("previous");
        expect_same([&] { yyjson::write(t, out, with_validation); });
        EXPECT_EQ(out, "previous");
        expect_same([&] { yyjson::write(t, out, WriteFlag::NoFlag, with_validation); });
        EXPECT_EQ(out, "previous");

        if constexpr (yyjson::readable<T>)
        {
            if (leg == read_leg::checked)
            {
                const auto json = std::string(yyjson::write(t));
                const auto r = validation_error([&] { (void)yyjson::read<T>(json); });
                EXPECT_TRUE(std::string_view(r.what()).starts_with(message)) << json << "\n" << r.what();
                EXPECT_EQ(r.path(), path) << json;
            }
        }
    }

    template <typename T, typename F>
    T with(F&& f)
    {
        auto t = T();
        f(t);
        return t;
    }
}  // namespace

// the keywords of the values of the fields
enum class VdColor
{
    red,
    green
};
struct VdFields
{
    int a = 0;
    double d = 0;
    std::string s = "ab";
    unsigned u = 0;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::tuple<int, std::string> t = {0, "x"};
    std::string e = "mm";
    int c = 1;
    std::string day = "2024-01-01";
};
template <>
inline constexpr auto yyjson::annotations<VdFields> = yyjson::annotate(
    yyjson::field("a", yyjson::minimum(0)), yyjson::field("d", yyjson::exclusive_maximum(10)),
    yyjson::field("s", yyjson::min_length(2), yyjson::max_length(4)), yyjson::field("u", yyjson::multiple_of(3)),
    yyjson::field("v", yyjson::items(yyjson::maximum(5)), yyjson::max_items(3), yyjson::unique_items),
    yyjson::field("m", yyjson::additional_properties(yyjson::minimum(0)),
                  yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("t", yyjson::prefix_items(yyjson::minimum(0), yyjson::min_length(1))),
    yyjson::field("e", yyjson::enum_("mm", "cm")), yyjson::field("c", yyjson::const_(1)),
    yyjson::field("day", yyjson::format_assertion("date")));

TEST(Validate, ValueKeywordsOfFields)
{
    expect_valid(VdFields{});
    expect_valid(VdFields{5, 9.5, "abcd", 6, {1, 2, 5}, {{"abc", 0}}, {3, "y"}, "cm", 1, "2000-02-29"});
    expect_invalid(with<VdFields>([](auto& t) { t.a = -1; }),
                   R"(the value of the key "a" of VdFields is less than the minimum 0)", "/a");
    expect_invalid(with<VdFields>([](auto& t) { t.d = 10; }),
                   R"(the value of the key "d" of VdFields is not less than the exclusive maximum 10)", "/d");
    expect_invalid(with<VdFields>([](auto& t) { t.s = "a"; }),
                   R"(the value of the key "s" of VdFields is shorter than the minimum length 2)", "/s");
    expect_invalid(with<VdFields>([](auto& t) { t.u = 4; }),
                   R"(the value of the key "u" of VdFields is not a multiple of 3)", "/u");
    expect_invalid(with<VdFields>([](auto& t) { t.v = {1, 9}; }),
                   R"(an element of the value of the key "v" of VdFields is greater than the maximum 5)", "/v/1");
    expect_invalid(with<VdFields>([](auto& t) { t.v = {1, 2, 3, 4}; }),
                   R"(the value of the key "v" of VdFields has more items than the maximum 3)", "/v");
    expect_invalid(with<VdFields>([](auto& t) { t.v = {1, 1}; }),
                   R"(an element of the value of the key "v" of VdFields is equal to an earlier element)", "/v/1");
    expect_invalid(with<VdFields>([](auto& t) { t.m = {{"a", 0}, {"b", -1}}; }),
                   R"(an element of the value of the key "m" of VdFields is less than the minimum 0)", "/m/b");
    expect_invalid(with<VdFields>([](auto& t) { t.m = {{"abcd", 0}}; }),
                   R"(a key of the value of the key "m" of VdFields is longer than the maximum length 3)", "/m/abcd");
    expect_invalid(with<VdFields>([](auto& t) { t.t = {-1, "x"}; }),
                   R"(an element of the value of the key "t" of VdFields is less than the minimum 0)", "/t/0");
    expect_invalid(with<VdFields>([](auto& t) { t.t = {0, ""}; }),
                   R"(an element of the value of the key "t" of VdFields is shorter than the minimum length 1)",
                   "/t/1");
    expect_invalid(with<VdFields>([](auto& t) { t.e = "km"; }),
                   R"(the value of the key "e" of VdFields is not one of "mm", "cm")", "/e");
    expect_invalid(with<VdFields>([](auto& t) { t.c = 2; }),
                   R"(the value of the key "c" of VdFields is not equal to the constant 1)", "/c");
    expect_invalid(with<VdFields>([](auto& t) { t.day = "2023-02-29"; }),
                   R"(the value of the key "day" of VdFields does not have the format "date")", "/day");
}

// the values in containers, in tuples and in other structs, with the path to each
struct VdInner
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdInner> = yyjson::annotate(yyjson::field("n", yyjson::minimum(0)));
struct VdNested
{
    std::vector<VdInner> list;
    std::map<std::string, VdInner> by_name;
    std::map<int, VdInner> by_id;
    std::map<VdColor, VdInner> by_color;
    std::tuple<int, VdInner> pair;
    VdInner inner;
    std::array<VdInner, 2> fixed;
};

TEST(Validate, NestedValues)
{
    const auto bad = VdInner{-1};
    const auto message = R"(the value of the key "n" of VdInner is less than the minimum 0)";
    expect_valid(VdNested{{{1}}, {{"a/b", {2}}}, {{3, {3}}}, {{VdColor::green, {4}}}, {1, {5}}, {6}, {}});
    expect_invalid(with<VdNested>([&](auto& t) { t.list = {{}, bad}; }), message, "/list/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"a/b", bad}}; }), message, "/by_name/a~1b/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"m~n", bad}}; }), message, "/by_name/m~0n/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_id = {{7, bad}}; }), message, "/by_id/0/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_color = {{VdColor::green, bad}}; }), message,
                   "/by_color/green/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.pair = {0, bad}; }), message, "/pair/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.inner = bad; }), message, "/inner/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.fixed = {VdInner{}, bad}; }), message, "/fixed/1/n");
    expect_invalid(std::vector<VdInner>{{}, {}, bad}, message, "/2/n");
    expect_invalid(bad, message, "/n");
}

// the values that pointers, `std::optional`, `yyjson::nullable` and `yyjson::indirect` hold, and the null that
// they are written as when they hold none
struct VdHolders
{
    std::optional<int> o;
    yyjson::nullable<int> n;
    std::shared_ptr<VdInner> p;
    std::unique_ptr<int> u;
    yyjson::indirect<VdInner> i = yyjson::indirect<VdInner>();
};
template <>
inline constexpr auto yyjson::annotations<VdHolders> =
    yyjson::annotate(yyjson::field("o", yyjson::minimum(0)), yyjson::field("n", yyjson::minimum(0)),
                     yyjson::field("u", yyjson::maximum(5)));
// a null passes `yyjson::minimum` and fails its negation
struct VdNotMinimum
{
    std::shared_ptr<int> p;
};
template <>
inline constexpr auto yyjson::annotations<VdNotMinimum> =
    yyjson::annotate(yyjson::field("p", yyjson::not_(yyjson::minimum(0))));
// a null that a keyword rejects
struct VdConstNull
{
    yyjson::nullable<int> n;
};
template <>
inline constexpr auto yyjson::annotations<VdConstNull> = yyjson::annotate(yyjson::field("n", yyjson::const_(1)));

TEST(Validate, Holders)
{
    expect_valid(VdHolders{});
    expect_valid(with<VdHolders>([](auto& t) {
        t.o = 1;
        t.n = 2;
        t.p = std::make_shared<VdInner>();
        t.u = std::make_unique<int>(5);
    }));
    expect_invalid(with<VdHolders>([](auto& t) { t.o = -1; }),
                   R"(the value of the key "o" of VdHolders is less than the minimum 0)", "/o");
    expect_invalid(with<VdHolders>([](auto& t) { t.n = -1; }),
                   R"(the value of the key "n" of VdHolders is less than the minimum 0)", "/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.p = std::make_shared<VdInner>(-1); }),
                   R"(the value of the key "n" of VdInner is less than the minimum 0)", "/p/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.u = std::make_unique<int>(6); }),
                   R"(the value of the key "u" of VdHolders is greater than the maximum 5)", "/u");
    expect_invalid(with<VdHolders>([](auto& t) { t.i = yyjson::indirect<VdInner>(VdInner{-1}); }),
                   R"(the value of the key "n" of VdInner is less than the minimum 0)", "/i/n");

    expect_valid(VdNotMinimum{std::make_shared<int>(-1)});
    expect_invalid(VdNotMinimum{}, R"(the value of the key "p" of VdNotMinimum satisfies the settings of yyjson::not_)",
                   "/p");
    expect_invalid(VdNotMinimum{std::make_shared<int>(0)},
                   R"(the value of the key "p" of VdNotMinimum satisfies the settings of yyjson::not_)", "/p");
    expect_valid(VdConstNull{1});
    expect_invalid(VdConstNull{}, R"(the value of the key "n" of VdConstNull is not equal to the constant 1)", "/n");
}

// `yyjson::satisfies` and the combinators
bool vd_is_even(int v) { return v % 2 == 0; }
struct VdLogic
{
    int even = 0;
    int any = 0;
    int one = 6;
    int all = 1;
    int cond = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdLogic> = yyjson::annotate(
    yyjson::field("even", yyjson::satisfies<vd_is_even>),
    yyjson::field("any", yyjson::any_of(yyjson::maximum(0), yyjson::minimum(10))),
    yyjson::field("one", yyjson::one_of(yyjson::maximum(5), yyjson::minimum(0))),
    yyjson::field("all", yyjson::all_of(yyjson::minimum(1), yyjson::maximum(3))),
    yyjson::field("cond", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                               yyjson::else_(yyjson::maximum(5)))));

TEST(Validate, SatisfiesAndCombinators)
{
    expect_valid(VdLogic{});
    expect_valid(VdLogic{2, 10, 6, 3, 12});
    expect_invalid(with<VdLogic>([](auto& t) { t.even = 1; }),
                   R"(the value of the key "even" of VdLogic is rejected by yyjson::satisfies)", "/even");
    expect_invalid(with<VdLogic>([](auto& t) { t.any = 5; }),
                   R"(the value of the key "any" of VdLogic satisfies none of the subschemas of yyjson::any_of)",
                   "/any");
    expect_invalid(with<VdLogic>([](auto& t) { t.one = 3; }),
                   R"(the value of the key "one" of VdLogic satisfies more than one subschema of yyjson::one_of)",
                   "/one");
    expect_invalid(with<VdLogic>([](auto& t) { t.all = 4; }),
                   R"(the value of the key "all" of VdLogic does not satisfy all the subschemas of yyjson::all_of)",
                   "/all");
    expect_invalid(
        with<VdLogic>([](auto& t) { t.cond = 11; }),
        R"(the value of the key "cond" of VdLogic satisfies the settings of yyjson::if_ and not those of yyjson::then)",
        "/cond");
    expect_invalid(
        with<VdLogic>([](auto& t) { t.cond = 6; }),
        R"(the value of the key "cond" of VdLogic satisfies neither the settings of yyjson::if_ nor those of yyjson::else_)",
        "/cond");
}

// a transparent struct, which is written as its field, and the fields of DOM values
struct VdPercent
{
    int value = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPercent> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::minimum(0), yyjson::maximum(100)));
struct VdDom
{
    VdPercent pct;
    yyjson::value any;
};
template <>
inline constexpr auto yyjson::annotations<VdDom> =
    yyjson::annotate(yyjson::field("any", yyjson::min_items(2), yyjson::items(yyjson::minimum(0))));

TEST(Validate, TransparentAndDom)
{
    expect_valid(VdPercent{100});
    expect_invalid(VdPercent{101}, "the value of VdPercent is greater than the maximum 100", "");
    expect_valid(VdDom{{5}, yyjson::read("[1,2]")});
    expect_invalid(VdDom{{-1}, yyjson::read("[1,2]")}, "the value of VdPercent is less than the minimum 0", "/pct");
    expect_invalid(VdDom{{1}, yyjson::read("[1]")},
                   R"(the value of the key "any" of VdDom has fewer items than the minimum 2)", "/any");
    expect_invalid(VdDom{{1}, yyjson::read("[1,-1]")},
                   R"(an element of the value of the key "any" of VdDom is less than the minimum 0)", "/any/1");
}

// `yyjson::as_array`, whose positions and extra items are checked as they are written
struct VdRow
{
    int id = 0;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdRow> =
    yyjson::annotate(yyjson::as_array, yyjson::field("id", yyjson::minimum(0)),
                     yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::maximum(5))));

TEST(Validate, AsArray)
{
    expect_valid(VdRow{1, {2, 5}});
    expect_invalid(VdRow{-1, {}}, "an element of the value of VdRow is less than the minimum 0", "/0");
    expect_invalid(VdRow{1, {1, 6}}, "an element of the value of VdRow is greater than the maximum 5", "/2");
}

// the keywords of the whole array of `yyjson::as_array`, counted on the array that write gives: the positions up
// to the first that it leaves out, then the extra items
struct VdArrayKeys
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
    std::vector<std::optional<int>> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayKeys> =
    yyjson::annotate(yyjson::as_array, yyjson::min_items(3), yyjson::max_items(4), yyjson::unique_items,
                     yyjson::contains(yyjson::const_(7)), yyjson::field("rest", yyjson::extra_items));
struct VdArrayShort
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayShort> = yyjson::annotate(yyjson::as_array, yyjson::min_items(2));
struct VdArrayAny
{
    int a = 0;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayAny> =
    yyjson::annotate(yyjson::as_array, yyjson::any_of(yyjson::contains(yyjson::const_(7)), yyjson::min_items(3)),
                     yyjson::field("rest", yyjson::extra_items));
struct VdArrayCell
{
    int x = 0;
    std::optional<int> y;
};
// `yyjson::max_properties` in `yyjson::contains` counts the members of the JSON object of an element
struct VdArrayCells
{
    VdArrayCell a;
    VdArrayCell b;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayCells> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::not_(yyjson::contains(yyjson::max_properties(1))));
struct VdArrayHolder
{
    VdArrayKeys row;
    VdArrayCells cells;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayHolder> = yyjson::annotate(yyjson::additional_properties(false));

TEST(Validate, AsArrayWholeArray)
{
    static_assert(yyjson::detail::needs_validation<VdArrayShort>);
    const auto keys = VdArrayKeys{7, 2, 3, {}};
    expect_valid(keys);
    expect_invalid(VdArrayKeys{7, 2, std::nullopt, {}}, "the value of VdArrayKeys has fewer items than the minimum 3",
                   "");
    expect_invalid(VdArrayKeys{1, 1, 7, {}}, "an element of the value of VdArrayKeys is equal to an earlier element",
                   "/1");
    expect_invalid(VdArrayKeys{1, 2, 3, {}},
                   "the value of VdArrayKeys has no item that satisfies the constraints of yyjson::contains", "");
    expect_invalid(VdArrayKeys{1, 2, 7, {8, 9}}, "the value of VdArrayKeys has more items than the maximum 4", "");
    expect_valid(VdArrayKeys{1, 2, 3, {7}});
    expect_invalid(VdArrayKeys{7, 2, 3, {2}}, "an element of the value of VdArrayKeys is equal to an earlier element",
                   "/3");
    expect_valid(VdArrayShort{1, 2});
    expect_invalid(VdArrayShort{1, std::nullopt}, "the value of VdArrayShort has fewer items than the minimum 2", "");

    // write throws for a position that it leaves out before an extra item, and the keywords are not checked
    const auto gap = VdArrayKeys{7, 2, std::nullopt, {3}};
    EXPECT_NO_THROW(yyjson::validate(gap));
    EXPECT_THROW((void)yyjson::write(gap, yyjson::with_validation), yyjson::write_error);

    expect_valid(VdArrayAny{7, {}});
    expect_valid(VdArrayAny{1, {2, 3}});
    expect_invalid(VdArrayAny{1, {2}}, "the value of VdArrayAny satisfies none of the subschemas of yyjson::any_of",
                   "");

    const auto cells = VdArrayCells{{1, 2}, {1, 2}};
    expect_valid(cells);
    expect_invalid(VdArrayCells{{1, 2}, {}}, "the value of VdArrayCells satisfies the settings of yyjson::not_", "");

    expect_valid(VdArrayHolder{keys, cells});
    expect_invalid(VdArrayHolder{{1, 1, 7, {}}, cells},
                   "an element of the value of VdArrayKeys is equal to an earlier element", "/row/1");
    expect_invalid(VdArrayHolder{keys, {}}, "the value of VdArrayCells satisfies the settings of yyjson::not_",
                   "/cells");
    expect_invalid(std::vector<VdArrayKeys>{keys, {7, 2, std::nullopt, {}}},
                   "the value of VdArrayKeys has fewer items than the minimum 3", "/1");
    expect_invalid(std::vector<VdArrayCells>{cells, {}},
                   "the value of VdArrayCells satisfies the settings of yyjson::not_", "/1");
}

// a field that write leaves out is not checked, and a field that a read gives another value is a violation
struct VdWeight
{
    int a = 0;
    int note = 7;
};
template <>
inline constexpr auto yyjson::annotations<VdWeight> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("note", yyjson::skip));
struct VdDefaults
{
    int d = 0;
    int e = 0;
    VdWeight w;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaults> =
    yyjson::annotate(yyjson::field("d", yyjson::minimum(1), yyjson::default_),
                     yyjson::field("e", yyjson::minimum(1), yyjson::default_, yyjson::emit_defaults),
                     yyjson::field("w", yyjson::default_));
int vd_three() { return 3; }
struct VdReadBack
{
    std::optional<int> x = 3;
    std::optional<int> y;
    std::optional<int> z = 3;
};
template <>
inline constexpr auto yyjson::annotations<VdReadBack> =
    yyjson::annotate(yyjson::field("z", yyjson::default_from<vd_three>));
struct VdOptionalPercent
{
    std::optional<int> value;
};
template <>
inline constexpr auto yyjson::annotations<VdOptionalPercent> = yyjson::annotate(yyjson::transparent);
struct VdReadBackTransparent
{
    VdOptionalPercent p = {4};
};

TEST(Validate, Presence)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    // the defaults of "d" and "w" are left out, and "e" is written
    const auto e1 = VdDefaults{0, 1, {0, 3}};
    expect_valid(e1);
    expect_invalid(VdDefaults{0, 0, {0, 7}}, R"(the value of the key "e" of VdDefaults is less than the minimum 1)",
                   "/e");
    // `yyjson::emit_defaults` of the call writes and checks them
    const auto d_message = std::string(R"(the value of the key "d" of VdDefaults is less than the minimum 1 at /d)");
    EXPECT_EQ(validation_error([&] { yyjson::validate(e1, emit_defaults); }).what(), d_message);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(e1, emit_defaults, with_validation); }).what(), d_message);
    auto out = std::string("previous");
    EXPECT_EQ(validation_error([&] { yyjson::write(e1, out, emit_defaults, with_validation); }).what(), d_message);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(validation_error([&] {
                  (void)yyjson::write(e1, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              d_message);
    EXPECT_EQ(validation_error([&] {
                  yyjson::write(e1, out, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              d_message);
    EXPECT_EQ(out, "previous");
    // the field "w" equals its default as JSON, and the skipped field is not compared
    const auto e2 = VdDefaults{1, 1, {0, 3}};
    EXPECT_NO_THROW(yyjson::validate(e2));
    const auto w_message = R"(the value of the key "a" of VdWeight is less than the minimum 1 at /w/a)";
    EXPECT_EQ(validation_error([&] { yyjson::validate(e2, emit_defaults); }).what(), std::string(w_message));
    EXPECT_EQ(std::string(yyjson::write(VdDefaults{1, 1, {2, 3}}, emit_defaults, with_validation)),
              std::string(yyjson::write(VdDefaults{1, 1, {2, 3}}, emit_defaults)));

    // an empty `std::optional`, whose default holds a value, is read back as that value
    expect_valid(VdReadBack{});
    expect_valid(VdReadBack{4, std::nullopt, 5});
    expect_invalid(VdReadBack{std::nullopt, {}, 1},
                   "the key \"x\" of VdReadBack holds an empty std::optional, which is not written and reads back "
                   "as the default value",
                   "/x", read_leg::skipped);
    expect_invalid(VdReadBack{3, {}, std::nullopt},
                   "the key \"z\" of VdReadBack holds an empty std::optional, which is not written and reads back "
                   "as the default value",
                   "/z", read_leg::skipped);
    expect_valid(VdReadBackTransparent{});
    expect_invalid(VdReadBackTransparent{VdOptionalPercent{}},
                   "the key \"p\" of VdReadBackTransparent holds an empty std::optional, which is not written and "
                   "reads back as the default value",
                   "/p", read_leg::skipped);
}

// an error of write itself takes the place of a violation in a checked write
struct VdCircle
{
    int r = 0;
};
struct VdEnvelope
{
    std::string kind = "circle";
    std::variant<VdCircle, int> data;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "num"})), yyjson::field("n", yyjson::minimum(0)));
struct VdReal
{
    double d = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdReal> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0)));

TEST(Validate, Precedence)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto violation_and_mismatch = VdEnvelope{"num", VdCircle{}, -1};
    EXPECT_THROW(yyjson::validate(violation_and_mismatch), yyjson::bad_cast);
    const auto expect_write_error = [](const auto& t) {
        try
        {
            (void)yyjson::write(t, with_validation);
            ADD_FAILURE() << "no exception";
        }
        catch (const yyjson::bad_cast&)
        {
            ADD_FAILURE() << "the violation is thrown in the place of the error of write";
        }
        catch (const std::invalid_argument&)
        {
        }
    };
    expect_write_error(violation_and_mismatch);
    const auto mismatch = VdEnvelope{"num", VdCircle{}, 0};
    EXPECT_NO_THROW(yyjson::validate(mismatch));
    expect_write_error(mismatch);

    // a NaN that the write rejects, and one that it writes
    const auto nan = VdReal{std::nan("")};
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(nan); }).what()),
              R"(the value of the key "d" of VdReal is less than the minimum 0 at /d)");
    EXPECT_THROW((void)yyjson::write(nan, with_validation), yyjson::write_error);
    auto out = std::string("previous");
    EXPECT_THROW(yyjson::write(nan, out, with_validation), yyjson::write_error);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(nan, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the value of the key "d" of VdReal is less than the minimum 0 at /d)");
    EXPECT_EQ(std::string(yyjson::write(std::vector{std::nan("")}, WriteFlag::AllowInfAndNan, with_validation)),
              "[NaN]");
    const auto negative = VdReal{-INFINITY};
    EXPECT_THROW((void)yyjson::write(negative, with_validation), yyjson::write_error);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(negative, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the value of the key "d" of VdReal is less than the minimum 0 at /d)");
    // a non-finite number written as null is checked as the null that a read gets
    const auto e = validation_error([&] { (void)yyjson::write(nan, WriteFlag::InfAndNanAsNull, with_validation); });
    EXPECT_EQ(std::string(e.what()), "double is not constructible from JSON null at /d");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_FALSE(e.position().has_value());
    out = "previous";
    EXPECT_THROW(yyjson::write(nan, out, WriteFlag::InfAndNanAsNull, with_validation), yyjson::bad_cast);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(yyjson::write(VdReal{1}, WriteFlag::InfAndNanAsNull, with_validation)), R"({"d":1.0})");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(std::vector{std::nan("")}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "double is not constructible from JSON null at /0");
    const auto optional_nan = std::vector<std::optional<double>>{std::nan("")};
    EXPECT_EQ(std::string(yyjson::write(optional_nan, WriteFlag::InfAndNanAsNull, with_validation)), "[null]");
}

// a recursive type, whose depth the check limits as a read does
struct VdTree
{
    int v = 0;
    std::vector<VdTree> kids;
};
template <>
inline constexpr auto yyjson::annotations<VdTree> = yyjson::annotate(yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, DepthLimit)
{
    expect_invalid(VdTree{0, {VdTree{}, VdTree{0, {VdTree{-1, {}}}}}},
                   R"(the value of the key "v" of VdTree is less than the minimum 0)", "/kids/1/kids/0/v");
    auto tree = VdTree();
    for (auto i = 0; i < 300; ++i) tree = VdTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(e.path(), "");
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    EXPECT_THROW((void)yyjson::write(tree, yyjson::with_validation), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
}

// the keywords of a string, of the items of an array and of the keys of a map
struct VdMore
{
    std::string code = "ab";
    std::string word = "é";
    std::vector<int> v = {10, 11};
    std::vector<int> pair = {1, 2};
    std::map<VdColor, int> colors;
    std::shared_ptr<std::string> unit = std::make_shared<std::string>("mm");
};
template <>
inline constexpr auto yyjson::annotations<VdMore> = yyjson::annotate(
    yyjson::field("code", yyjson::pattern("^a")), yyjson::field("word", yyjson::max_length(1)),
    yyjson::field("v", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2), yyjson::max_contains(3)),
    yyjson::field("pair", yyjson::const_(yyjson::json_text("[1, 2]"))),
    yyjson::field("colors", yyjson::property_names(yyjson::enum_("red"))),
    yyjson::field("unit", yyjson::enum_("mm", "cm")));

TEST(Validate, MoreValueKeywords)
{
    expect_valid(VdMore{});
    expect_valid(with<VdMore>([](auto& t) {
        t.colors = {{VdColor::red, 0}};
        t.v = {10, 1, 11, 12};
    }));
    expect_invalid(with<VdMore>([](auto& t) { t.code = "ba"; }),
                   R"(the value of the key "code" of VdMore does not match the pattern "^a")", "/code");
    expect_invalid(with<VdMore>([](auto& t) { t.word = "日本"; }),
                   R"(the value of the key "word" of VdMore is longer than the maximum length 1)", "/word");
    expect_invalid(
        with<VdMore>([](auto& t) { t.v = {10, 1}; }),
        R"(the value of the key "v" of VdMore has fewer than 2 items that satisfy the constraints of yyjson::contains)",
        "/v");
    expect_invalid(
        with<VdMore>([](auto& t) { t.v = {10, 11, 12, 13}; }),
        R"(the value of the key "v" of VdMore has more than 3 items that satisfy the constraints of yyjson::contains)",
        "/v");
    expect_invalid(with<VdMore>([](auto& t) { t.pair = {2, 1}; }),
                   R"(the value of the key "pair" of VdMore is not equal to the constant [1, 2])", "/pair");
    expect_invalid(with<VdMore>([](auto& t) { t.colors = {{VdColor::green, 0}}; }),
                   R"(a key of the value of the key "colors" of VdMore is not one of "red")", "/colors/green");
    expect_invalid(with<VdMore>([](auto& t) { t.unit = std::make_shared<std::string>("km"); }),
                   R"(the value of the key "unit" of VdMore is not one of "mm", "cm")", "/unit");
}

// a non-finite number that the write gives as null is checked as a read under yyjson::reject_null_as_missing
// reads that null at its position
struct VdOptionalReal
{
    std::optional<double> o = 1.0;
};
struct VdNullableReals
{
    std::optional<double> o;
    std::vector<std::optional<double>> v;
    yyjson::nullable<double> n;
};
struct VdUnreadable
{
    std::string_view s = "x";
    double d = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnreadable> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0)));

TEST(Validate, NonFiniteAsNull)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    const auto optional_null =
        "std::optional<double> is not constructible from JSON null under "
        "yyjson::reject_null_as_missing at /o";
    auto e = validation_error([&] { (void)yyjson::write(VdOptionalReal{nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    EXPECT_EQ(e.path(), "/o");
    e = validation_error([&] { (void)yyjson::write(VdNullableReals{nan, {}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    const auto nulls = VdNullableReals{std::nullopt, {nan, 1}, nan};
    EXPECT_EQ(std::string(yyjson::write(nulls, as_null, with_validation)), R"({"v":[null,1.0],"n":null})");

    static_assert(!yyjson::readable<VdUnreadable>);
    e = validation_error([&] { (void)yyjson::write(VdUnreadable{"x", nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "double is not constructible from JSON null at /d");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(VdUnreadable{"x", nan}); }).what()),
              R"(the value of the key "d" of VdUnreadable is less than the minimum 0 at /d)");
}

// a field that the write rejects alone is still checked against its keywords
struct VdUnwritable
{
    std::vector<double> v;
    std::string s;
};
template <>
inline constexpr auto yyjson::annotations<VdUnwritable> =
    yyjson::annotate(yyjson::field("v", yyjson::max_items(1)), yyjson::field("s", yyjson::max_length(1)));

TEST(Validate, FieldThatWriteRejects)
{
    using yyjson::with_validation;
    const auto many = VdUnwritable{{1, std::nan("")}, ""};
    auto e = validation_error([&] { yyjson::validate(many); });
    EXPECT_EQ(std::string(e.what()),
              R"(the value of the key "v" of VdUnwritable has more items than the maximum 1 at /v)");
    EXPECT_EQ(e.path(), "/v");
    EXPECT_THROW((void)yyjson::write(many, with_validation), yyjson::write_error);
    const auto invalid = VdUnwritable{{}, "\xff\xfe"};
    e = validation_error([&] { yyjson::validate(invalid); });
    EXPECT_EQ(std::string(e.what()),
              R"(the value of the key "s" of VdUnwritable is longer than the maximum length 1 at /s)");
    EXPECT_THROW((void)yyjson::write(invalid, with_validation), yyjson::write_error);
    EXPECT_NO_THROW(yyjson::validate(VdUnwritable{{std::nan("")}, "\xff"}));
}

// an empty std::optional under transparent structs in a chain
struct VdChainIn
{
    std::optional<int> v = 3;
};
template <>
inline constexpr auto yyjson::annotations<VdChainIn> = yyjson::annotate(yyjson::transparent);
struct VdChainOut
{
    VdChainIn in;
};
template <>
inline constexpr auto yyjson::annotations<VdChainOut> = yyjson::annotate(yyjson::transparent);
struct VdChainTop
{
    VdChainOut out;
};

TEST(Validate, TransparentChain)
{
    expect_valid(VdChainTop{});
    expect_invalid(VdChainTop{VdChainOut{VdChainIn{std::nullopt}}},
                   "the key \"out\" of VdChainTop holds an empty std::optional, which is not written and reads back "
                   "as the default value",
                   "/out", read_leg::skipped);
}

// an enumerator without a name is an error of write, which the check does not report
struct VdNamedColor
{
    VdColor c = VdColor::red;
};
template <>
inline constexpr auto yyjson::annotations<VdNamedColor> = yyjson::annotate(yyjson::field("c", yyjson::enum_("red")));

TEST(Validate, UnnamedEnumerator)
{
    const auto unnamed = VdNamedColor{static_cast<VdColor>(7)};
    EXPECT_NO_THROW(yyjson::validate(unnamed));
    const auto plain = validation_error([&] { (void)yyjson::write(unnamed); });
    const auto e = validation_error([&] { (void)yyjson::write(unnamed, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), std::string_view(plain.what()));
    EXPECT_EQ(e.path(), plain.path());
}

// the keywords of each kind let the types be checked and written
struct VdClosed
{
    int a = 0;
    std::optional<int> b = 1;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdClosed> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::max_properties(3), yyjson::dependent_required("a", {"b"}));
struct VdMatches
{
    yyjson::value v;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdMatches> =
    yyjson::annotate(yyjson::field("v", yyjson::contains(yyjson::matches<VdInner>)));

struct VdClosedStruct
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedStruct> = yyjson::annotate(yyjson::additional_properties(false));
struct VdPatterned
{
    int a = 0;
    std::map<std::string, int> xs = {{"x1", 1}};
};
template <>
inline constexpr auto yyjson::annotations<VdPatterned> =
    yyjson::annotate(yyjson::field("xs", yyjson::pattern_properties("^x")));
struct VdShortKeys
{
    int a = 0;
    int bb = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdShortKeys> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
struct VdSquare
{
    int side = 1;
};
using VdShape = std::variant<VdCircle, VdSquare>;
template <>
inline constexpr auto yyjson::annotations<VdShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square"}));
using VdOneOf = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdAnyOf = yyjson::any_of_variant<int, std::string>;
struct VdPoint
{
    int x = 0;
    int y = 0;
};
template <>
struct yyjson::caster<VdPoint>
{
    static VdPoint from_json(std::array<int, 2> a) { return {a[0], a[1]}; }
    template <typename... Ts>
    static std::array<int, 2> to_json(const VdPoint& p, Ts...)
    {
        return {p.x, p.y};
    }
};
struct VdPair
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPair> = yyjson::annotate(yyjson::as_array, yyjson::min_items(2));
struct VdKinds
{
    VdShape shape = VdSquare{};
    VdOneOf one;
    VdAnyOf any = std::string("x");
    std::vector<VdPoint> points = {{1, 2}};
    std::multimap<int, int> pairs = {{1, 2}, {1, 3}};
    VdPair row;
};
template <>
inline constexpr auto yyjson::annotations<VdKinds> =
    yyjson::annotate(yyjson::field("points", yyjson::max_items(3)), yyjson::field("pairs", yyjson::max_items(2)));

TEST(Validate, OtherKeywordsCompile)
{
    EXPECT_NO_THROW(yyjson::validate(VdClosed{}));
    EXPECT_EQ(std::string(yyjson::write(VdClosed{}, yyjson::with_validation)), std::string(yyjson::write(VdClosed{})));
    EXPECT_NO_THROW(yyjson::validate(VdMatches{yyjson::read(R"([{"n":1}])"), 1}));
    EXPECT_NO_THROW(yyjson::validate(VdEnvelope{}));
    expect_valid(VdClosedStruct{});
    expect_valid(VdPatterned{});
    expect_valid(VdShortKeys{});
    expect_valid(VdKinds{});
}

// the object that the write gives for a struct, with the keys of its maps and of the tag that holds it
struct VdPatternOnly
{
    int a = 0;
    std::map<std::string, int> ps;
};
template <>
inline constexpr auto yyjson::annotations<VdPatternOnly> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("ps", yyjson::pattern_properties("^p")));
struct VdTagCircle
{
    int r = 0;
    std::optional<int> label;
};
template <>
inline constexpr auto yyjson::annotations<VdTagCircle> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(2));
struct VdTagSquare
{
    int side = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagSquare> = yyjson::annotate(yyjson::min_properties(2));
struct VdTagLine
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagLine> =
    yyjson::annotate(yyjson::property_names(yyjson::pattern("^[a-n]")));
struct VdTagNamed
{
    std::string type = "named";
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagNamed> = yyjson::annotate(yyjson::additional_properties(false));
using VdTagShape = std::variant<VdTagCircle, VdTagSquare, VdTagLine, VdTagNamed>;
template <>
inline constexpr auto yyjson::annotations<VdTagShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square", "line", "named"}));
struct VdTagHolder
{
    std::vector<VdTagShape> shapes;
};
struct VdShortNames
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdShortNames> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::field("rest", yyjson::extra_keys));
struct VdLongField
{
    int abc = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdLongField> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));

// the number of members, with the defaults that the write leaves out
struct VdCount
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCount> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::default_, yyjson::field("rest", yyjson::extra_keys));
struct VdCountEmit
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountEmit> = yyjson::annotate(
    yyjson::max_properties(2), yyjson::default_, yyjson::field("a", yyjson::default_, yyjson::emit_defaults),
    yyjson::field("rest", yyjson::extra_keys));
struct VdCountMin
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountMin> =
    yyjson::annotate(yyjson::min_properties(3), yyjson::field("rest", yyjson::extra_keys));

// the presence rules and the conditions, on the keys that the write gives
struct VdPresence
{
    std::optional<int> a;
    std::optional<int> b;
    int v = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPresence> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}), yyjson::property("v", yyjson::minimum(0)));
struct VdDroppedDefault
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdDroppedDefault> =
    yyjson::annotate(yyjson::field("a", yyjson::default_), yyjson::dependent_required("b", {"a"}));
struct VdConditions
{
    int kind = 0;
    std::optional<int> x;
    std::optional<int> y;
    std::optional<int> z;
};
template <>
inline constexpr auto yyjson::annotations<VdConditions> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::property("kind", yyjson::const_(1))), yyjson::then(yyjson::required("x")),
                         yyjson::else_(yyjson::not_(yyjson::required("x")))),
    yyjson::one_of(yyjson::required("y"), yyjson::required("z")),
    yyjson::any_of(yyjson::max_properties(2), yyjson::required("z")),
    yyjson::all_of(yyjson::property("kind", yyjson::maximum(1))));

// object keywords at a field of struct type and in a condition of the struct
struct VdInnerObj
{
    int a = 0;
    std::optional<int> b;
};
struct VdOuterObj
{
    VdInnerObj in;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<VdOuterObj> =
    yyjson::annotate(yyjson::field("in", yyjson::max_properties(1)), yyjson::all_of(yyjson::max_properties(1)));

// the values of the maps of the keys
struct VdKeyMaps
{
    std::map<std::string, int> ab;
    std::map<std::string, int> a;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdKeyMaps> = yyjson::annotate(
    yyjson::field("ab", yyjson::pattern_properties("^ab"), yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("a", yyjson::pattern_properties("^a"), yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(1))));

// a non-finite number written as null beside a struct
struct VdNanInner
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdNanInner> = yyjson::annotate(yyjson::max_properties(1));
struct VdNanBeside
{
    std::tuple<double, VdNanInner> t;
    std::vector<std::pair<VdNanInner, yyjson::nullable<double>>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNanBeside> = yyjson::annotate(yyjson::field("v", yyjson::max_items(1)));

TEST(Validate, ObjectKeys)
{
    expect_valid(VdPatternOnly{0, {{"p1", 1}}});
    expect_invalid(VdPatternOnly{0, {{"q", 1}}},
                   R"(VdPatternOnly has the key "q", which yyjson::additional_properties(false) rejects)", "/q");
    // the key of the internal tag is a key of the object of the alternative
    expect_valid(VdTagShape{VdTagCircle{1, std::nullopt}});
    expect_valid(VdTagShape{VdTagSquare{}});
    expect_valid(VdTagShape{VdTagNamed{}});
    expect_invalid(VdTagShape{VdTagLine{}}, R"(a key of VdTagLine does not match the pattern "^[a-n]")", "/type");
    expect_invalid(VdTagHolder{{VdTagCircle{}, VdTagCircle{1, 2}}},
                   "VdTagCircle has more properties than the maximum 2", "/shapes/1");
    expect_valid(VdShortNames{0, {{"xy", 1}}});
    expect_invalid(VdShortNames{0, {{"xyz", 1}}}, "a key of VdShortNames is longer than the maximum length 2", "/xyz");
    expect_invalid(VdLongField{}, "a key of VdLongField is longer than the maximum length 2", "/abc");
}

TEST(Validate, ObjectCounts)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    expect_valid(VdCount{});
    expect_valid(VdCount{0, 0, {{"x", 1}}});
    expect_invalid(VdCount{1, 1, {{"x", 1}}}, "VdCount has more properties than the maximum 2", "");
    // the defaults that `yyjson::emit_defaults` writes are counted
    const auto message = std::string("VdCount has more properties than the maximum 2");
    const auto defaults = VdCount{0, 0, {{"x", 1}}};
    EXPECT_EQ(validation_error([&] { yyjson::validate(defaults, emit_defaults); }).what(), message);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(defaults, emit_defaults, with_validation); }).what(), message);
    EXPECT_THROW((void)yyjson::read<VdCount>(std::string(yyjson::write(defaults, emit_defaults))), yyjson::bad_cast);
    expect_valid(VdCountEmit{0, 0, {{"x", 1}}});
    expect_invalid(VdCountEmit{0, 0, {{"x", 1}, {"y", 1}}}, "VdCountEmit has more properties than the maximum 2", "");
    expect_valid(VdCountMin{0, {{"x", 1}, {"y", 2}}});
    expect_invalid(VdCountMin{0, {{"x", 1}}}, "VdCountMin has fewer properties than the minimum 3", "");
    // at a field of struct type, and in a condition of the struct
    expect_valid(VdOuterObj{});
    expect_invalid(VdOuterObj{VdInnerObj{0, 1}, std::nullopt},
                   R"(the value of the key "in" of VdOuterObj has more properties than the maximum 1)", "/in");
    expect_invalid(VdOuterObj{{}, 1}, "VdOuterObj has more properties than the maximum 1 of yyjson::all_of", "");
}

TEST(Validate, Conditions)
{
    expect_valid(VdPresence{1, 1, 0});
    expect_valid(VdPresence{});
    expect_invalid(VdPresence{1, std::nullopt, 0},
                   R"(VdPresence is not constructible from JSON without the key "b", which yyjson::dependent_required )"
                   "requires",
                   "");
    expect_invalid(VdPresence{std::nullopt, std::nullopt, -1},
                   R"(the value of the key "v" of VdPresence is less than the minimum 0)", "/v");
    // a default that the write leaves out is a missing key
    expect_valid(VdDroppedDefault{1, 1});
    expect_invalid(VdDroppedDefault{0, 1},
                   R"(VdDroppedDefault is not constructible from JSON without the key "a", which )"
                   "yyjson::dependent_required requires",
                   "");
    EXPECT_NO_THROW(yyjson::validate(VdDroppedDefault{0, 1}, yyjson::emit_defaults));

    expect_valid(VdConditions{0, std::nullopt, 1, std::nullopt});
    expect_valid(VdConditions{1, 1, std::nullopt, 1});
    expect_invalid(VdConditions{1, std::nullopt, 1, std::nullopt},
                   R"(VdConditions is not constructible from JSON without the key "x", which yyjson::if_then_else )"
                   "requires",
                   "");
    expect_invalid(VdConditions{0, 1, 1, std::nullopt},
                   "VdConditions has the keys and the values that yyjson::not_ rejects", "");
    expect_invalid(VdConditions{0, std::nullopt, 1, 1},
                   R"(VdConditions is not constructible from JSON with all the keys of more than one of ["y"], ["z"])",
                   "");
    expect_invalid(VdConditions{0, std::nullopt, std::nullopt, std::nullopt},
                   R"(VdConditions is not constructible from JSON without all the keys of one of ["y"], ["z"])", "");
    expect_invalid(VdConditions{1, 1, 1, std::nullopt},
                   "VdConditions satisfies none of the subschemas of yyjson::any_of", "");
    expect_invalid(VdConditions{2, std::nullopt, 1, std::nullopt},
                   R"(the value of the key "kind" of VdConditions is greater than the maximum 1)", "/kind");
}

TEST(Validate, KeyMaps)
{
    expect_valid(VdKeyMaps{{{"ab", 1}}, {{"a", 0}}, {{"x", 1}}});
    // a key is checked by each pattern that matches it
    expect_invalid(VdKeyMaps{{{"ab", -1}}, {}, {}},
                   R"(the value of the key "ab" of VdKeyMaps is less than the minimum 0)", "/ab");
    expect_invalid(VdKeyMaps{{{"ab", 6}}, {}, {}},
                   R"(the value of the key "ab" of VdKeyMaps is greater than the maximum 5)", "/ab");
    expect_invalid(VdKeyMaps{{}, {}, {{"x", 0}}}, R"(the value of the key "x" of VdKeyMaps is less than the minimum 1)",
                   "/x");
    // a key that the read stores in another field
    expect_invalid(VdKeyMaps{{}, {{"abc", 1}}, {}},
                   R"(the key "abc" of the field "a" of VdKeyMaps is read into the field "ab")", "/abc",
                   read_leg::skipped);
    expect_invalid(VdKeyMaps{{}, {}, {{"ax", 1}}},
                   R"(the key "ax" of the field "rest" of VdKeyMaps is read into the field "a")", "/ax",
                   read_leg::skipped);
    expect_invalid(VdKeyMaps{{{"q", 1}}, {}, {}},
                   R"(the key "q" of the field "ab" of VdKeyMaps is read into the field "rest")", "/q",
                   read_leg::skipped);
}

TEST(Validate, InternalTagMismatch)
{
    const auto mismatch = VdTagShape{VdTagNamed{"other", 0}};
    EXPECT_NO_THROW(yyjson::validate(mismatch));
    EXPECT_THROW((void)yyjson::write(mismatch), std::invalid_argument);
    try
    {
        (void)yyjson::write(mismatch, yyjson::with_validation);
        ADD_FAILURE() << "no exception";
    }
    catch (const yyjson::bad_cast&)
    {
        ADD_FAILURE() << "the violation is thrown in the place of the error of write";
    }
    catch (const std::invalid_argument&)
    {
    }
}

TEST(Validate, NonFiniteBesideStruct)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    expect_valid(VdNanBeside{});
    auto e = validation_error([&] { (void)yyjson::write(VdNanBeside{{nan, {}}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "double is not constructible from JSON null at /t/0");
    EXPECT_EQ(e.path(), "/t/0");
    const auto nullable = VdNanBeside{{0, {}}, {{{}, nan}}};
    EXPECT_EQ(std::string(yyjson::write(nullable, as_null, with_validation)),
              R"({"t":[0.0,{"a":0}],"v":[[{"a":0},null]]})");
    // the struct beside the null is still checked
    e = validation_error([&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{0, 1}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "VdNanInner has more properties than the maximum 1 at /v/0/0");
    e = validation_error(
        [&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{}, nan}, {{}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()),
              R"(the value of the key "v" of VdNanBeside has more items than the maximum 1 at /v)");
}

// a non-finite number in a map of keys, written as null
struct VdNanExtra
{
    int x = 0;
    std::map<std::string, double> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdNanExtra> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct VdNanPattern
{
    int x = 0;
    std::map<std::string, double> p;
};
template <>
inline constexpr auto yyjson::annotations<VdNanPattern> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^k")));

// a keyword of a field that holds null in the place of a NaN
struct VdNotInner
{
    int a = 0;
};
struct VdNotNull
{
    std::vector<std::variant<std::optional<double>, VdNotInner>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNotNull> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::not_(yyjson::minimum(0)))));

TEST(Validate, NonFiniteInKeyMapsAndFields)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    auto e = validation_error([&] { (void)yyjson::write(VdNanExtra{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "double is not constructible from JSON null at /k");
    e = validation_error([&] { (void)yyjson::write(VdNanPattern{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "double is not constructible from JSON null at /k");
    EXPECT_EQ(std::string(yyjson::write(VdNanPattern{0, {{"k", 1.5}}}, as_null, with_validation)),
              R"({"x":0,"k":1.5})");

    // read checks the null, which `yyjson::not_` accepts, and not the NaN
    const auto not_null = VdNotNull{{std::optional<double>(nan)}};
    e = validation_error([&] { (void)yyjson::write(not_null, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()),
              R"(an element of the value of the key "v" of VdNotNull satisfies the settings of yyjson::not_ at /v/0)");
    EXPECT_THROW((void)yyjson::read<VdNotNull>(std::string(yyjson::write(not_null, as_null))), yyjson::bad_cast);
}

// a struct with object keywords inside another, checked on the JSON of the outer
struct VdLink
{
    int v = 0;
    std::optional<int> w;
    std::unique_ptr<VdLink> next;
};
template <>
inline constexpr auto yyjson::annotations<VdLink> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, NestedObjectKeywords)
{
    const auto chain = [](int depth, std::optional<int> w, int v) {
        auto head = std::unique_ptr<VdLink>();
        for (auto i = 0; i < depth; ++i)
            head = std::make_unique<VdLink>(VdLink{i == 0 ? v : 0, i == 0 ? w : std::nullopt, std::move(head)});
        return head;
    };
    const auto valid = VdLink{0, std::nullopt, chain(3, std::nullopt, 0)};
    EXPECT_NO_THROW(yyjson::validate(valid));
    const auto counted = VdLink{0, std::nullopt, chain(3, 1, 0)};
    auto e = validation_error([&] { yyjson::validate(counted); });
    EXPECT_EQ(std::string(e.what()), "VdLink has more properties than the maximum 2 at /next/next/next");
    e = validation_error([&] { (void)yyjson::write(counted, yyjson::with_validation); });
    EXPECT_EQ(e.path(), "/next/next/next");
    const auto negative = VdLink{0, std::nullopt, chain(2, std::nullopt, -1)};
    e = validation_error([&] { yyjson::validate(negative); });
    EXPECT_EQ(std::string(e.what()),
              R"(the value of the key "v" of VdLink is less than the minimum 0 at /next/next/v)");
}

// a key of a pattern map that read stores in no field
struct VdUnmatched
{
    int x = 0;
    std::map<std::string, int> p;
};
template <>
inline constexpr auto yyjson::annotations<VdUnmatched> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^p")));

TEST(Validate, KeyReadIntoNoField)
{
    expect_valid(VdUnmatched{0, {{"p1", 1}}});
    expect_invalid(VdUnmatched{0, {{"q", 1}}}, R"(the key "q" of the field "p" of VdUnmatched is read into no field)",
                   "/q", read_leg::skipped);
}

// the keys that read needs and that write leaves out
struct VdOmitZero
{
    static int to_json(int v) { return v; }
    static int from_json(int v) { return v; }
    static bool should_omit(int v) { return v == 0; }
};
struct VdRequiredOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOmitted> = yyjson::annotate(yyjson::field("a", yyjson::required));
struct VdMissingOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdMissingOmitted> = yyjson::annotate(yyjson::reject_missing_keys);

// a check of the keys of a struct without object keywords allocates no memory
struct VdUnknownOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnknownOnly> = yyjson::annotate(yyjson::reject_unknown_keys);
struct VdRequiredOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOnly> = yyjson::annotate(yyjson::field("a", yyjson::required));

namespace
{
    thread_local std::size_t vd_allocations = 0;

    template <typename T>
    std::size_t validate_allocations(const T& t)
    {
        const auto before = vd_allocations;
        yyjson::validate(t);
        return vd_allocations - before;
    }
}  // namespace

void* operator new(std::size_t size)
{
    ++vd_allocations;
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    ++vd_allocations;
    return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }

TEST(Validate, MissingKeys)
{
    expect_valid(VdRequiredOmitted{1});
    expect_invalid(VdRequiredOmitted{0}, R"(VdRequiredOmitted is not constructible from JSON without the key "a")", "");
    expect_valid(VdMissingOmitted{1, 0});
    expect_invalid(VdMissingOmitted{0, 1}, R"(VdMissingOmitted is not constructible from JSON without the key "a")",
                   "");
    EXPECT_EQ(validate_allocations(VdUnknownOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOmitted{1}), 0U);
}

// a key that write gives twice in one object, which a strict read rejects and a plain read takes
namespace
{
    // `Object` is the type of the object in `t` that gives `key` twice.
    template <typename Object = void, typename T>
    void expect_repeated_key(const T& t, std::string_view key, std::string_view path)
    {
        using named = std::conditional_t<std::is_void_v<Object>, T, Object>;
        const auto message =
            std::format(R"(the object read into {} has the key "{}" twice)", yyjson::detail::type_name<named>(), key);
        expect_invalid(t, message, path, read_leg::skipped);
        if constexpr (yyjson::readable<T>)
        {
            const auto json = std::string(yyjson::write(t));
            EXPECT_NO_THROW((void)yyjson::read<T>(json));
            const auto r = validation_error([&] { (void)yyjson::read<T>(json, yyjson::strict); });
            EXPECT_TRUE(std::string_view(r.what()).starts_with(message)) << json << "\n" << r.what();
            EXPECT_EQ(r.path(), path) << json;
        }
    }
}  // namespace

enum class VdSide
{
    left,
    right
};
// a key whose `<=>` compares more than its text
struct VdTagged
{
    std::string s;
    int n = 0;
    VdTagged() = default;
    VdTagged(std::string_view v, int k = 0) : s(v), n(k) {}
    operator std::string_view() const { return s; }
    auto operator<=>(const VdTagged&) const = default;
};
struct VdMulti
{
    std::multimap<std::string, int> m;
};

TEST(Validate, RepeatedKeys)
{
    using pairs = std::vector<std::pair<std::string, int>>;
    static_assert(yyjson::detail::needs_validation<std::multimap<std::string, int>>);
    static_assert(yyjson::detail::needs_validation<VdMulti>);
    expect_valid(std::multimap<std::string, int>{{"a", 1}, {"b", 1}});
    expect_repeated_key(std::multimap<std::string, int>{{"a", 1}, {"a", 2}}, "a", "/a");
    expect_repeated_key(std::unordered_multimap<std::string, int>{{"a", 1}, {"a", 2}}, "a", "/a");
    expect_repeated_key(pairs{{"a", 1}, {"b", 1}, {"a", 2}}, "a", "/a");
    expect_repeated_key(std::list<std::pair<std::string, int>>{{"a", 1}, {"a", 2}}, "a", "/a");
    expect_repeated_key(std::deque<std::pair<std::string, int>>{{"a", 1}, {"a", 2}}, "a", "/a");
    expect_repeated_key(std::vector<std::pair<VdSide, int>>{{VdSide::left, 1}, {VdSide::left, 2}}, "left", "/left");
    using keyed = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    expect_repeated_key(keyed{{"a", 1}, {"a", 2}}, "a", "/a");
    static_assert(!yyjson::detail::needs_validation<std::tuple<std::pair<std::string, int>>>);
    const auto first = std::string("k"), second = std::string("k");
    expect_repeated_key(std::map<const char*, int>{{first.c_str(), 1}, {second.c_str(), 2}}, "k", "/k");
    // keys that compare apart in C++ but are written as one text
    expect_repeated_key(std::map<VdTagged, int>{{VdTagged("a", 0), 1}, {VdTagged("a", 1), 2}}, "a", "/a");
    // a null key has no JSON text, so write throws for it and the check of the keys skips it
    using c_pairs = std::vector<std::pair<const char*, int>>;
    EXPECT_NO_THROW(yyjson::validate(c_pairs{{nullptr, 1}, {nullptr, 2}, {"", 3}}));
    EXPECT_NO_THROW(yyjson::validate(
        std::tuple<std::pair<const char*, int>, std::pair<const char*, int>>{{nullptr, 1}, {nullptr, 2}}));

    // more keys than the pairwise comparison takes
    auto many = pairs();
    for (int i = 0; i < 20; ++i) many.emplace_back(std::to_string(i), i);
    expect_valid(many);
    many.emplace_back("7", 0);
    many.emplace_back("3", 0);
    expect_repeated_key(many, "7", "/7");
    auto nulls = c_pairs(20, {nullptr, 0});
    EXPECT_NO_THROW(yyjson::validate(nulls));
    nulls.emplace_back("z", 1);
    nulls.emplace_back("z", 2);
    EXPECT_EQ(validation_error([&] { yyjson::validate(nulls); }).path(), "/z");
    auto sides = std::vector<std::pair<VdSide, int>>();
    for (int i = 0; i < 20; ++i) sides.emplace_back(i % 2 == 0 ? VdSide::right : VdSide::left, i);
    expect_repeated_key(sides, "right", "/right");

    expect_repeated_key<std::multimap<std::string, int>>(VdMulti{{{"a", 1}, {"a", 2}}}, "a", "/m/a");
    expect_repeated_key<pairs>(std::vector<pairs>{{{"a", 1}}, {{"a", 1}, {"a", 2}}}, "a", "/1/a");
    expect_repeated_key<pairs>(std::map<std::string, pairs>{{"k", {{"a", 1}, {"a", 2}}}}, "a", "/k/a");

    // the key is reported before the values, as a strict read does; a plain read reports the value
    using rows = std::multimap<std::string, VdRow>;
    const auto twice =
        std::format(R"(the object read into {} has the key "a" twice)", yyjson::detail::type_name<rows>());
    const auto bad_rows = rows{{"a", VdRow{-1, {}}}, {"a", VdRow{1, {}}}};
    expect_invalid(bad_rows, twice, "/a", read_leg::skipped);
    EXPECT_EQ(validation_error([&] { (void)yyjson::read<rows>(yyjson::write(bad_rows), yyjson::strict); }).path(),
              "/a");

    static_assert(!yyjson::detail::needs_validation<std::map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::unordered_map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::multimap<int, int>>);
    expect_valid(std::multimap<int, int>{{1, 1}, {1, 2}});
    auto map = std::map<std::string, int>();
    for (int i = 0; i < 100; ++i) map.emplace(std::to_string(i), i);
    EXPECT_EQ(validate_allocations(map), 0U);
    EXPECT_EQ(validate_allocations(std::multimap<std::string, int>{{"a", 1}, {"b", 2}}), 0U);
}

// a union: the settings of the alternative that it holds, the count of a one_of, and the read of its JSON when the
// alternative fails, which passes if another alternative reads it
struct VdUnionA
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionA> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
struct VdUnionB
{
    int x = 0;
};
struct VdUnionC
{
    int x = 0;
    std::optional<int> y;
};
struct VdUnionD
{
    int y = 0;
};
struct VdUnionE
{
    std::string msg;
};
using VdLowOrInt = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, int>;
using VdLowOrHigh =
    yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdLowOrShort =
    yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<std::string, yyjson::max_length(3)>>;
using VdItemsOrText =
    yyjson::any_of_variant<yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(0))>, std::string>;
using VdSignedOrUnsigned = yyjson::one_of_variant<std::int64_t, std::uint64_t>;
// some alternatives other than the last one may read its values and some may not
using VdPartlyOverlapping =
    yyjson::one_of_variant<yyjson::alt<double, yyjson::maximum(0.5)>, std::string,
                           yyjson::alt<std::int64_t, yyjson::minimum(-100), yyjson::maximum(10)>, std::uint64_t>;
using VdAB = std::variant<VdUnionA, VdUnionB>;
using VdAnyAB = yyjson::any_of_variant<VdUnionA, VdUnionB>;
using VdOneCD = yyjson::one_of_variant<VdUnionC, VdUnionD>;
using VdOneAD = yyjson::one_of_variant<VdUnionA, VdUnionD>;
struct VdUnionFields
{
    VdOneCD u = VdUnionD{};
    VdLowOrShort n = 0;
};
struct VdCounted
{
    int a = 0;
    int b = 0;
};
using VdCountedOrMap =
    yyjson::any_of_variant<yyjson::alt<VdCounted, yyjson::max_properties(1)>, std::map<std::string, int>>;
struct VdDefaulted
{
    int a = 0;
    int b = 5;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaulted> = yyjson::annotate(yyjson::field("b", yyjson::default_));
using VdDefaultedOrInt = yyjson::any_of_variant<yyjson::alt<VdDefaulted, yyjson::max_properties(1)>, int>;
struct VdDisjointField
{
    int a = 0;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdDisjointField> = yyjson::annotate(yyjson::field("a", yyjson::minimum(0)));

namespace
{
    template <typename V>
    std::string vd_name()
    {
        return std::string(yyjson::detail::variant_name<V>());
    }
}  // namespace

TEST(Validate, Unions)
{
    static_assert(yyjson::detail::needs_validation<VdLowOrInt>);
    static_assert(yyjson::detail::needs_validation<VdOneCD>);
    static_assert(!yyjson::detail::needs_validation<yyjson::any_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<yyjson::one_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<std::variant<VdUnionB, int>>);

    // the settings of the alternative, where another alternative may read the JSON
    expect_valid(VdLowOrInt(std::in_place_index<0>, 5));
    expect_valid(VdLowOrInt(std::in_place_index<0>, -5));
    expect_valid(VdLowOrHigh(std::in_place_index<0>, 3));
    expect_valid(VdLowOrHigh(std::in_place_index<1>, -20));
    expect_invalid(VdLowOrHigh(std::in_place_index<0>, -5),
                   vd_name<VdLowOrHigh>() +
                       " is not constructible from JSON: alternative 0 (int): the value is less than the minimum 0; "
                       "alternative 1 (int): the value is greater than the maximum -10",
                   "");
    const auto text = std::string(yyjson::detail::type_name<std::string>());
    expect_valid(VdLowOrShort(std::string("abc")));
    expect_invalid(VdLowOrShort(-1),
                   vd_name<VdLowOrShort>() +
                       " is not constructible from JSON: alternative 0 (int): the value is less than the minimum 0; "
                       "alternative 1 (" +
                       text + "): reads no JSON number",
                   "");
    expect_invalid(VdLowOrShort(std::string("abcd")),
                   vd_name<VdLowOrShort>() + " is not constructible from JSON: alternative 0 (int): reads no JSON " +
                       "string; alternative 1 (" + text + "): the value is longer than the maximum length 3",
                   "");
    expect_invalid(VdItemsOrText(std::vector<int>{1, -2}),
                   vd_name<VdItemsOrText>() + " is not constructible from JSON: alternative 0 (std::vector<int>): " +
                       "the value has an element that does not satisfy the constraints of yyjson::items; " +
                       "alternative 1 (" + text + "): reads no JSON array",
                   "");
    // a keyword that counts the JSON of the alternative
    expect_valid(VdCountedOrMap(VdCounted{1, 2}));
    expect_valid(VdDefaultedOrInt(VdDefaulted{1, 5}));
    const auto defaulted = vd_name<VdDefaultedOrInt>() +
                           " is not constructible from JSON: alternative 0 (VdDefaulted): the value has more "
                           "properties than the maximum 1; alternative 1 (int): reads no JSON object";
    EXPECT_EQ(std::string(validation_error([] {
                              yyjson::validate(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults);
                          }).what()),
              defaulted);
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults,
                                                  yyjson::with_validation);
                          }).what()),
              defaulted);

    // a value whose alternative fails passes if another reads its JSON
    expect_valid(VdAB(VdUnionA{-1}));
    expect_valid(VdAnyAB(VdUnionA{-1}));
    // another alternative that reads the JSON only without its required keys
    expect_invalid(
        std::variant<VdUnionA, VdUnionE>(VdUnionA{-1}),
        vd_name<std::variant<VdUnionA, VdUnionE>>() +
            R"( is not constructible from JSON: alternative 0 (VdUnionA) at /x: the value of the key "x" of )"
            R"(VdUnionA is less than the minimum 0; alternative 1 )"
            R"((VdUnionE): VdUnionE is not constructible from JSON without the key "msg")",
        "", read_leg::skipped);
    expect_invalid(VdOneAD(VdUnionA{-1}),
                   R"(yyjson::one_of_variant<VdUnionA, VdUnionD> is not constructible from JSON: alternative 0 )"
                   R"((VdUnionA) at /x: the value of the key "x" of VdUnionA is less than the minimum 0; )"
                   R"(alternative 1 (VdUnionD): VdUnionD is not constructible from JSON without the key "y")",
                   "");
    // a walk that fails for the alternative that read gives too, which read does not check
    expect_invalid(std::variant<VdReadBack, int>(VdReadBack{3, {}, std::nullopt}),
                   "the key \"z\" of VdReadBack holds an empty std::optional, which is not written and reads back "
                   "as the default value",
                   "/z", read_leg::skipped);

    // the count of a one_of, which a disjoint alternative skips
    const auto matches_cd = std::string("the value matches the alternatives 0 and 1 of ") + vd_name<VdOneCD>();
    expect_invalid(VdOneCD(VdUnionC{1, 2}), matches_cd, "");
    expect_valid(VdOneCD(VdUnionC{1, {}}));
    expect_valid(VdOneCD(VdUnionD{1}));
    expect_valid(yyjson::one_of_variant<int, std::string>(1));
    expect_valid(VdSignedOrUnsigned(std::int64_t{-5}));
    expect_invalid(VdSignedOrUnsigned(std::int64_t{5}),
                   "the value matches the alternatives 0 and 1 of " + vd_name<VdSignedOrUnsigned>(), "");
    expect_invalid(std::vector<VdSignedOrUnsigned>{std::int64_t{-5}, std::uint64_t{5}},
                   "the value matches the alternatives 0 and 1 of " + vd_name<VdSignedOrUnsigned>(), "/1");
    expect_valid(VdPartlyOverlapping(-300.0));
    expect_valid(VdPartlyOverlapping(std::uint64_t{20}));
    const auto partly = vd_name<VdPartlyOverlapping>();
    expect_invalid(VdPartlyOverlapping(std::int64_t{0}), "the value matches the alternatives 0 and 2 of " + partly, "");
    expect_invalid(VdPartlyOverlapping(std::uint64_t{5}), "the value matches the alternatives 2 and 3 of " + partly,
                   "");

    // nested unions, and a union in an array, a map and a struct
    expect_invalid(yyjson::any_of_variant<VdOneCD, int>(VdOneCD(VdUnionC{1, 2})),
                   vd_name<yyjson::any_of_variant<VdOneCD, int>>() + " is not constructible from JSON: alternative " +
                       "0 (" + vd_name<VdOneCD>() + "): " + matches_cd + "; alternative 1 (int): reads no JSON object",
                   "");
    expect_valid(yyjson::any_of_variant<VdOneAD, VdUnionB>(VdOneAD(VdUnionA{-1})));
    expect_valid(yyjson::one_of_variant<VdAB, VdUnionD>(VdAB(VdUnionA{-1})));
    expect_invalid(std::vector<VdOneCD>{VdUnionD{1}, VdUnionD{2}, VdUnionD{3}, VdUnionC{1, 2}}, matches_cd, "/3");
    expect_invalid(std::map<std::string, VdOneCD>{{"k", VdUnionC{1, 2}}}, matches_cd, "/k");
    expect_invalid(VdUnionFields{VdUnionC{1, 2}, 1}, matches_cd, "/u");
    expect_valid(VdUnionFields{VdUnionC{1, {}}, std::string("ab")});
    // the settings of an alternative that is a variant check the alternative that the inner variant holds
    using VdNested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<int, std::string>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    using VdInner = std::variant<std::variant<int, std::string>, bool>;
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(7))));
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(std::string("ab")))));
    expect_valid(VdNested(VdInner(true)));
    const auto nested = vd_name<VdNested>() + " is not constructible from JSON: alternative 0 (" + vd_name<VdInner>() +
                        "): the value is ";
    expect_invalid(VdNested(VdInner(std::variant<int, std::string>(1))), nested + "less than the minimum 5", "");
    expect_invalid(VdNested(VdInner(std::variant<int, std::string>(std::string("a")))),
                   nested + "shorter than the minimum length 2", "");

    // a passing check of a disjoint union, or of an alternative that satisfies its settings, allocates nothing
    EXPECT_EQ(validate_allocations(std::vector<VdLowOrInt>(100, VdLowOrInt(std::in_place_index<0>, 1))), 0U);
    EXPECT_EQ(validate_allocations(yyjson::one_of_variant<int, std::string>(1)), 0U);
    EXPECT_EQ(validate_allocations(VdDisjointField{1, std::string("x")}), 0U);
}

// a union in a tagged variant, and a variant that is tagged
struct VdClosedA
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedA> = yyjson::annotate(yyjson::additional_properties(false));
struct VdTypedA
{
    std::string type;
    int a = 0;
};
using VdOneClosed = yyjson::one_of_variant<VdClosedA, VdTypedA>;
struct VdOtherZ
{
    int z = 0;
};
using VdTaggedOne = std::variant<VdOneClosed, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedOne> = yyjson::annotate(yyjson::internal_tag("type", {"one", "z"}));
using VdTaggedAD = std::variant<VdUnionA, VdUnionD>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedAD> = yyjson::annotate(yyjson::internal_tag("kind", {"a", "d"}));
// a union that a wrapper holds in a tagged variant
struct VdOneWrapped
{
    VdOneClosed u;
};
template <>
inline constexpr auto yyjson::annotations<VdOneWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedWrapped = std::variant<VdOneWrapped, std::unique_ptr<VdOneClosed>, std::optional<VdOneClosed>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedWrapped> =
    yyjson::annotate(yyjson::internal_tag("type", {"w", "p", "o", "z"}));
// a variant that is not tagged, whose map alternative reads the written object only without the tag
using VdUnionOrMap = std::variant<VdUnionA, std::map<std::string, int>>;
struct VdMapWrapped
{
    VdUnionOrMap u;
};
template <>
inline constexpr auto yyjson::annotations<VdMapWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedMap =
    std::variant<VdUnionOrMap, VdMapWrapped, std::unique_ptr<VdUnionOrMap>, std::optional<VdUnionOrMap>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedMap> =
    yyjson::annotate(yyjson::internal_tag("type", {"u", "w", "p", "o", "z"}));
struct VdAdjacentAB
{
    std::string kind = "a";
    std::variant<VdUnionA, VdUnionB> data;
};
template <>
inline constexpr auto yyjson::annotations<VdAdjacentAB> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"a", "b"})));

TEST(Validate, TaggedUnions)
{
    // the key of the tag lets the alternatives of the one_of read the same object
    const auto matches = std::string("the value matches the alternatives 0 and 1 of ") + vd_name<VdOneClosed>();
    expect_valid(VdOneClosed(VdClosedA{0}));
    expect_invalid(VdTaggedOne(VdOneClosed(VdClosedA{0})), matches, "");
    expect_invalid(VdTaggedOne(VdOneClosed(VdTypedA{"one", 0})), matches, "");
    expect_invalid(std::vector<VdTaggedOne>{VdOtherZ{}, VdOneClosed(VdClosedA{0})}, matches, "/1");
    expect_valid(VdTaggedOne(VdOtherZ{1}));

    // the tag reaches a union through a transparent struct, a pointer and an optional
    expect_invalid(VdTaggedWrapped(VdOneWrapped{VdClosedA{0}}), matches, "");
    expect_invalid(VdTaggedWrapped(std::make_unique<VdOneClosed>(VdClosedA{0})), matches, "");
    expect_invalid(VdTaggedWrapped(std::optional<VdOneClosed>(VdClosedA{0})), matches, "");
    expect_valid(VdTaggedWrapped(VdOtherZ{1}));
    const auto no_map = vd_name<VdUnionOrMap>() + " is not constructible from JSON: alternative 0 (VdUnionA) at /x: " +
                        R"(the value of the key "x" of VdUnionA is less than the minimum 0; alternative 1 ()" +
                        vd_name<std::map<std::string, int>>() + ") at /type: int is not constructible from JSON string";
    expect_invalid(VdTaggedMap(VdUnionOrMap(VdUnionA{-1})), no_map, "");
    expect_invalid(VdTaggedMap(VdMapWrapped{VdUnionA{-1}}), no_map, "");
    expect_invalid(VdTaggedMap(std::make_unique<VdUnionOrMap>(VdUnionA{-1})), no_map, "");
    expect_invalid(VdTaggedMap(std::optional<VdUnionOrMap>(VdUnionA{-1})), no_map, "");
    expect_valid(VdTaggedMap(VdUnionOrMap(VdUnionA{1})));
    expect_valid(VdTaggedMap(VdMapWrapped{VdUnionA{1}}));

    // a tagged variant reads only the alternative of its tag
    const auto low = std::string(R"(the value of the key "x" of VdUnionA is less than the minimum 0)");
    expect_invalid(VdTaggedAD(VdUnionA{-1}), low, "/x");
    expect_invalid(VdAdjacentAB{"a", VdUnionA{-1}}, low, "/data/x");
    expect_valid(VdAdjacentAB{"b", VdUnionB{-1}});
}

// a union whose alternative write gives a repeated key or null for a NaN
using VdRepeatedOrMap = std::variant<std::multimap<std::string, int>, std::map<std::string, int>>;
using VdLowOrNull = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, std::nullptr_t>;
using VdLowDouble = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, int>;
// a tree whose depth the check limits through a union
struct VdUnionTree
{
    int v = 0;
    std::vector<std::variant<VdUnionTree, int>> kids;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionTree> = yyjson::annotate(yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, UnionWrittenForms)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    // the other alternative does not read the key twice either, as a strict read
    const auto repeated = VdRepeatedOrMap(std::multimap<std::string, int>{{"a", 1}, {"a", 2}});
    const auto twice = [](auto t) {
        return std::format(R"(alternative {} ({}) at /a: the object read into {} has the key "a" twice)", t.first,
                           t.second, t.second);
    };
    const auto multi = std::string(yyjson::detail::type_name<std::multimap<std::string, int>>());
    const auto single = std::string(yyjson::detail::type_name<std::map<std::string, int>>());
    const auto message = vd_name<VdRepeatedOrMap>() + " is not constructible from JSON: " + twice(std::pair(0, multi)) +
                         "; " + twice(std::pair(1, single));
    expect_invalid(repeated, message, "", read_leg::skipped);
    EXPECT_NO_THROW((void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated)));
    EXPECT_TRUE(std::string_view(validation_error([&] {
                                     (void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated), yyjson::strict);
                                 }).what())
                    .starts_with(message));

    // a NaN that write gives as null, which another alternative reads
    const auto nan = std::nan("");
    EXPECT_EQ(std::string(yyjson::write(VdLowOrNull(nan), WriteFlag::InfAndNanAsNull, with_validation)), "null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::InfAndNanAsNull, with_validation);
                          }).what()),
              vd_name<VdLowDouble>() + " is not constructible from JSON: alternative 0 (double): reads no JSON null; " +
                  "alternative 1 (int): reads no JSON null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              vd_name<VdLowDouble>() + " is not constructible from JSON: alternative 0 (double): the value is less " +
                  "than the minimum 0; alternative 1 (int): reads no JSON number");

    // the depth of a walk and of the read of a failing union
    expect_invalid(VdUnionTree{0, {VdUnionTree{-1, {}}}},
                   vd_name<std::variant<VdUnionTree, int>>() +
                       " is not constructible from JSON: alternative 0 (VdUnionTree) at /v: the value of the key " +
                       R"("v" of VdUnionTree is less than the minimum 0; alternative 1 (int): reads no JSON object)",
                   "/kids/0");
    auto tree = VdUnionTree();
    for (auto i = 0; i < 300; ++i) tree = VdUnionTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    auto failing = VdUnionTree{-1, {}};
    for (auto i = 0; i < 100; ++i) failing = VdUnionTree{0, {std::move(failing)}};
    EXPECT_THROW(yyjson::validate(failing), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
}

// a type without keywords is checked by nothing
struct VdPlain
{
    int a = 0;
    std::string b;
    std::vector<std::map<std::string, double>> c;
};

TEST(Validate, Plain)
{
    static_assert(!yyjson::detail::needs_validation<VdPlain>);
    static_assert(yyjson::detail::needs_validation<VdNested>);
    expect_valid(VdPlain{1, "x", {{{"k", 1.5}}}});
    expect_valid(std::vector<int>{1, 2});
    expect_valid(42);
}

// NOLINTEND
