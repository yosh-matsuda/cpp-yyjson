#include <cmath>
#include <memory>
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

// the keywords that the check does not see yet still let the types be checked and written
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
