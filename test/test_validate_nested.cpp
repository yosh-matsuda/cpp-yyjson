#include "test_validate_common.hpp"

// NOLINTBEGIN

TEST(Validate, NestedValues)
{
    const auto bad = VdInner{-1};
    expect_valid(VdNested{{{1}}, {{"a/b", {2}}}, {{3, {3}}}, {{VdColor::green, {4}}}, {1, {5}}, {6}, {}});
    expect_invalid(with<VdNested>([&](auto& t) { t.list = {{}, bad}; }),
                   R"(the number is less than the minimum 0 at /list/1/n; reading the key "n" of VdInner)",
                   "/list/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"a/b", bad}}; }),
                   R"(the number is less than the minimum 0 at /by_name/a~1b/n; reading the key "n" of VdInner)",
                   "/by_name/a~1b/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"m~n", bad}}; }),
                   R"(the number is less than the minimum 0 at /by_name/m~0n/n; reading the key "n" of VdInner)",
                   "/by_name/m~0n/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_id = {{7, bad}}; }),
                   R"(the number is less than the minimum 0 at /by_id/0/1/n; reading the key "n" of VdInner)",
                   "/by_id/0/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_color = {{VdColor::green, bad}}; }),
                   R"(the number is less than the minimum 0 at /by_color/green/n; reading the key "n" of VdInner)",
                   "/by_color/green/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.pair = {0, bad}; }),
                   R"(the number is less than the minimum 0 at /pair/1/n; reading the key "n" of VdInner)",
                   "/pair/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.inner = bad; }),
                   R"(the number is less than the minimum 0 at /inner/n; reading the key "n" of VdInner)", "/inner/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.fixed = {VdInner{}, bad}; }),
                   R"(the number is less than the minimum 0 at /fixed/1/n; reading the key "n" of VdInner)",
                   "/fixed/1/n");
    expect_invalid(std::vector<VdInner>{{}, {}, bad},
                   R"(the number is less than the minimum 0 at /2/n; reading the key "n" of VdInner)", "/2/n");
    expect_invalid(bad, R"(the number is less than the minimum 0 at /n; reading the key "n" of VdInner)", "/n");
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
                   R"(the number is less than the minimum 0 at /o; reading the key "o" of VdHolders)", "/o");
    expect_invalid(with<VdHolders>([](auto& t) { t.n = -1; }),
                   R"(the number is less than the minimum 0 at /n; reading the key "n" of VdHolders)", "/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.p = std::make_shared<VdInner>(-1); }),
                   R"(the number is less than the minimum 0 at /p/n; reading the key "n" of VdInner)", "/p/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.u = std::make_unique<int>(6); }),
                   R"(the number is greater than the maximum 5 at /u; reading the key "u" of VdHolders)", "/u");
    expect_invalid(with<VdHolders>([](auto& t) { t.i = yyjson::indirect<VdInner>(VdInner{-1}); }),
                   R"(the number is less than the minimum 0 at /i/n; reading the key "n" of VdInner)", "/i/n");

    expect_valid(VdNotMinimum{std::make_shared<int>(-1)});
    expect_invalid(VdNotMinimum{},
                   R"(the value satisfies the subschema of not at /p; reading the key "p" of VdNotMinimum)", "/p");
    expect_invalid(VdNotMinimum{std::make_shared<int>(0)},
                   R"(the value satisfies the subschema of not at /p; reading the key "p" of VdNotMinimum)", "/p");
    expect_valid(VdConstNull{1});
    expect_invalid(VdConstNull{},
                   R"(the value is not equal to the constant 1 at /n; reading the key "n" of VdConstNull)", "/n");
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
                   R"(the value does not satisfy the predicate at /even; reading the key "even" of VdLogic)", "/even");
    expect_invalid(with<VdLogic>([](auto& t) { t.any = 5; }),
                   R"(the value satisfies none of the subschemas of anyOf at /any; reading the key "any" of VdLogic)",
                   "/any");
    expect_invalid(with<VdLogic>([](auto& t) { t.one = 3; }),
                   R"(the value satisfies more than one subschema of oneOf at /one; reading the key "one" of VdLogic)",
                   "/one");
    expect_invalid(
        with<VdLogic>([](auto& t) { t.all = 4; }),
        R"(the value does not satisfy all the subschemas of allOf at /all; reading the key "all" of VdLogic)", "/all");
    expect_invalid(with<VdLogic>([](auto& t) { t.cond = 11; }),
                   R"(the value satisfies if and not then at /cond; reading the key "cond" of VdLogic)", "/cond");
    expect_invalid(with<VdLogic>([](auto& t) { t.cond = 6; }),
                   R"(the value satisfies neither if nor else at /cond; reading the key "cond" of VdLogic)", "/cond");
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
    expect_invalid(VdPercent{101}, "the number is greater than the maximum 100; reading VdPercent", "");
    expect_valid(VdDom{{5}, yyjson::read("[1,2]")});
    expect_invalid(VdDom{{-1}, yyjson::read("[1,2]")},
                   "the number is less than the minimum 0 at /pct; reading VdPercent", "/pct");
    expect_invalid(VdDom{{1}, yyjson::read("[1]")},
                   R"(the array has fewer items than the minimum 2 at /any; reading the key "any" of VdDom)", "/any");
    expect_invalid(
        VdDom{{1}, yyjson::read("[1,-1]")},
        R"(the number is less than the minimum 0 at /any/1; reading an element of the value at the key "any" of VdDom)",
        "/any/1");
}

TEST(Validate, AsArray)
{
    expect_valid(VdRow{1, {2, 5}});
    expect_invalid(VdRow{-1, {}}, "the number is less than the minimum 0 at /0; reading an element of VdRow", "/0");
    expect_invalid(VdRow{1, {1, 6}}, "the number is greater than the maximum 5 at /2; reading an element of VdRow",
                   "/2");
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
    expect_invalid(VdArrayKeys{7, 2, std::nullopt, {}},
                   "the array has fewer items than the minimum 3; reading VdArrayKeys", "");
    expect_invalid(VdArrayKeys{1, 1, 7, {}},
                   "the item is equal to an earlier item at /1; reading an element of VdArrayKeys", "/1");
    expect_invalid(VdArrayKeys{1, 2, 3, {}}, "the array has no item that satisfies contains; reading VdArrayKeys", "");
    expect_invalid(VdArrayKeys{1, 2, 7, {8, 9}}, "the array has more items than the maximum 4; reading VdArrayKeys",
                   "");
    expect_valid(VdArrayKeys{1, 2, 3, {7}});
    expect_invalid(VdArrayKeys{7, 2, 3, {2}},
                   "the item is equal to an earlier item at /3; reading an element of VdArrayKeys", "/3");
    expect_valid(VdArrayShort{1, 2});
    expect_invalid(VdArrayShort{1, std::nullopt}, "the array has fewer items than the minimum 2; reading VdArrayShort",
                   "");

    // write throws for a position that it leaves out before an extra item, and the keywords are not checked
    const auto gap = VdArrayKeys{7, 2, std::nullopt, {3}};
    EXPECT_NO_THROW(yyjson::validate(gap));
    EXPECT_THROW((void)yyjson::write(gap, yyjson::with_validation), yyjson::write_error);

    expect_valid(VdArrayAny{7, {}});
    expect_valid(VdArrayAny{1, {2, 3}});
    expect_invalid(VdArrayAny{1, {2}}, "the value satisfies none of the subschemas of anyOf; reading VdArrayAny", "");

    const auto cells = VdArrayCells{{1, 2}, {1, 2}};
    expect_valid(cells);
    expect_invalid(VdArrayCells{{1, 2}, {}}, "the value satisfies the subschema of not; reading VdArrayCells", "");

    expect_valid(VdArrayHolder{keys, cells});
    expect_invalid(VdArrayHolder{{1, 1, 7, {}}, cells},
                   "the item is equal to an earlier item at /row/1; reading an element of VdArrayKeys", "/row/1");
    expect_invalid(VdArrayHolder{keys, {}}, "the value satisfies the subschema of not at /cells; reading VdArrayCells",
                   "/cells");
    expect_invalid(std::vector<VdArrayKeys>{keys, {7, 2, std::nullopt, {}}},
                   "the array has fewer items than the minimum 3 at /1; reading VdArrayKeys", "/1");
    expect_invalid(std::vector<VdArrayCells>{cells, {}},
                   "the value satisfies the subschema of not at /1; reading VdArrayCells", "/1");
}

// elements that the keywords compare as the JSON values that write gives
struct VdZeroed
{
    int x = 0;
    bool operator==(const VdZeroed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VdZeroed> = yyjson::annotate(yyjson::field("x", yyjson::default_));
struct VdUniqueWritten
{
    std::vector<VdZeroed> defaulted;
    std::vector<double> reals;
    std::vector<std::optional<std::optional<int>>> nulls;
};
template <>
inline constexpr auto yyjson::annotations<VdUniqueWritten> =
    yyjson::annotate(yyjson::field("defaulted", yyjson::unique_items), yyjson::field("reals", yyjson::unique_items),
                     yyjson::field("nulls", yyjson::unique_items));
struct VdUniquePair
{
    double a = 0;
    double b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUniquePair> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::unique_items);

constexpr bool above_one(double x) { return x > 1; }
struct VdWrittenItems
{
    double s = 1.0001;
    std::vector<double> v = {1.0001};
    std::map<std::string, std::vector<double>> m = {{"a", {1.0001}}};
};
template <>
inline constexpr auto yyjson::annotations<VdWrittenItems> = yyjson::annotate(
    yyjson::field("s", yyjson::const_(1), yyjson::satisfies<above_one>),
    yyjson::field("v", yyjson::items(yyjson::const_(1), yyjson::satisfies<above_one>)),
    yyjson::field("m", yyjson::additional_properties(yyjson::items(yyjson::const_(1), yyjson::satisfies<above_one>))));

struct VdEscapedKey
{
    std::map<std::string, std::vector<double>> m;
};
template <>
inline constexpr auto yyjson::annotations<VdEscapedKey> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::items(yyjson::const_(1)))));

TEST(Validate, UniqueWrittenJson)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    const auto at = [](std::string_view key, std::string_view path) {
        return std::format(R"(the item is equal to an earlier item at {}; reading an element of the value at the )"
                           R"(key "{}" of VdUniqueWritten)",
                           path, key);
    };
    // both elements write {}, or {"x":0} with `yyjson::emit_defaults`
    const auto defaulted = VdUniqueWritten{.defaulted = {{0}, {0}}, .reals = {}, .nulls = {}};
    expect_invalid(defaulted, at("defaulted", "/defaulted/1"), "/defaulted/1");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(defaulted, emit_defaults); }).what()),
              at("defaulted", "/defaulted/1"));
    // both optionals write null
    expect_invalid(VdUniqueWritten{.defaulted = {}, .reals = {}, .nulls = {std::nullopt, std::optional<int>{}}},
                   at("nulls", "/nulls/1"), "/nulls/1");
#if YYJSON_VERSION_HEX >= 0x000C00
    // the flags write the two doubles as the same number
    const auto fixed = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(3));
    const auto reals = VdUniqueWritten{.defaulted = {}, .reals = {1.0, 1.0000000000000002}, .nulls = {}};
    expect_valid(reals);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(reals, fixed, with_validation); }).what()),
              at("reals", "/reals/1"));
    const auto pair = VdUniquePair{1.0, 1.0000000000000002};
    expect_valid(pair);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(pair, fixed, with_validation); }).what()),
              "the item is equal to an earlier item at /1; reading an element of VdUniquePair");
    // the predicate takes the number that the flags write, as `yyjson::read` gives it
    const auto rounded = std::string(
        R"(the value does not satisfy the predicate at /s (line 1, column 6, pos 5); reading the key "s" of )"
        R"(VdWrittenItems)");
    EXPECT_EQ(
        std::string(validation_error([&] { (void)yyjson::write(VdWrittenItems{}, fixed, with_validation); }).what()),
        rounded);
    // the flags write another key, and the member still takes the constant of its entry
    const auto invalid_key = std::string(1, static_cast<char>(0xff));
    const auto escaped = fixed | yyjson::WriteFlag::AllowInvalidUnicode | yyjson::WriteFlag::EscapeUnicode;
    EXPECT_EQ(std::string(yyjson::write(VdEscapedKey{{{invalid_key, {1.0001}}}}, escaped, with_validation)),
              R"({"m":{"\uFFFD":[1.0]}})");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdEscapedKey{{{invalid_key, {1.1}}}}, escaped, with_validation);
                          }).what()),
              "the value is not equal to the constant 1 at /m/\xEF\xBF\xBD/0; reading an element of the value at the "
              "key \"m\" of "
              "VdEscapedKey");
#endif
}

TEST(Validate, Presence)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    // the defaults of "d", "e" and "w" are left out also under `yyjson::emit_defaults` and are not checked
    const auto e1 = VdDefaults{0, 1, {0, 3}};
    expect_valid(e1);
    expect_valid(VdDefaults{0, 0, {0, 7}});
    EXPECT_NO_THROW(yyjson::validate(e1, emit_defaults));
    EXPECT_EQ(std::string(yyjson::write(e1, emit_defaults, with_validation)), R"({"e":1})");
    EXPECT_EQ(std::string(yyjson::write(VdDefaults{0, 0, {0, 7}}, emit_defaults, with_validation)), "{}");
    // a value other than the default is checked
    expect_invalid(VdDefaults{-1, 1, {0, 3}},
                   R"(the number is less than the minimum 1 at /d; reading the key "d" of VdDefaults)", "/d");
    expect_invalid(VdDefaults{1, -1, {0, 3}},
                   R"(the number is less than the minimum 1 at /e; reading the key "e" of VdDefaults)", "/e");
    expect_invalid(VdDefaults{1, 1, {-1, 3}},
                   R"(the number is less than the minimum 1 at /w/a; reading the key "a" of VdWeight)", "/w/a");

    // `yyjson::emit_defaults` of the call writes and checks the other defaults
    expect_valid(VdCapped{});
    const auto capped = std::string("the object has more properties than the maximum 1; reading VdCapped");
    EXPECT_EQ(validation_error([&] { yyjson::validate(VdCapped{}, emit_defaults); }).what(), capped);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(VdCapped{}, emit_defaults, with_validation); }).what(),
              capped);
    auto out = std::string("previous");
    EXPECT_EQ(validation_error([&] { yyjson::write(VdCapped{}, out, emit_defaults, with_validation); }).what(), capped);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(validation_error([&] {
                  (void)yyjson::write(VdCapped{}, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              capped);
    EXPECT_EQ(validation_error([&] {
                  yyjson::write(VdCapped{}, out, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              capped);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(yyjson::write(VdCapped{}, emit_defaults)), R"({"a":1,"b":2})");

    // an empty `std::optional`, whose default holds a value, is read back as that value
    expect_valid(VdReadBack{});
    expect_valid(VdReadBack{4, std::nullopt, 5});
    expect_invalid(
        VdReadBack{std::nullopt, {}, 1},
        R"(the value is not valid at /x; validating VdReadBack: the field "x" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/x", read_leg::skipped);
    expect_invalid(
        VdReadBack{3, {}, std::nullopt},
        R"(the value is not valid at /z; validating VdReadBack: the field "z" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/z", read_leg::skipped);
    expect_valid(VdReadBackTransparent{});
    expect_invalid(
        VdReadBackTransparent{VdOptionalPercent{}},
        R"(the value is not valid at /p; validating VdReadBackTransparent: the field "p" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/p", read_leg::skipped);
}

TEST(Validate, Precedence)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    // the error of write comes before the violation
    const auto violation_and_mismatch = VdEnvelope{"num", VdCircle{}, -1};
    EXPECT_THROW(yyjson::validate(violation_and_mismatch), std::invalid_argument);
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
    EXPECT_THROW(yyjson::validate(mismatch), std::invalid_argument);
    expect_write_error(mismatch);

    // a NaN that the write rejects, and one that it writes
    const auto nan = VdReal{std::nan("")};
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(nan); }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    EXPECT_THROW((void)yyjson::write(nan, with_validation), yyjson::write_error);
    auto out = std::string("previous");
    EXPECT_THROW(yyjson::write(nan, out, with_validation), yyjson::write_error);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(nan, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    EXPECT_EQ(std::string(yyjson::write(std::vector{std::nan("")}, WriteFlag::AllowInfAndNan, with_validation)),
              "[NaN]");
    const auto negative = VdReal{-INFINITY};
    EXPECT_THROW((void)yyjson::write(negative, with_validation), yyjson::write_error);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(negative, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    // a non-finite number written as null is checked as the null that a read gets
    const auto e = validation_error([&] { (void)yyjson::write(nan, WriteFlag::InfAndNanAsNull, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /d; reading double");
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
              "expected a number, got null at /0; reading double");
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
// holds itself through a translation
struct VdLoop
{
    std::shared_ptr<VdLoop> next;
};
struct VdLoopWire
{
    std::vector<VdLoop> next;
    int n = 1;
};
template <>
inline constexpr auto yyjson::annotations<VdLoopWire> = yyjson::annotate(yyjson::field("n", yyjson::minimum(1)));
template <>
struct yyjson::caster<VdLoop>
{
    static VdLoop from_json(const VdLoopWire&) { return {}; }
    static VdLoopWire to_json(const VdLoop& x)
    {
        auto w = VdLoopWire();
        if (x.next) w.next.push_back(*x.next);
        return w;
    }
};

TEST(Validate, DepthLimit)
{
    expect_invalid(VdTree{0, {VdTree{}, VdTree{0, {VdTree{-1, {}}}}}},
                   R"(the number is less than the minimum 0 at /kids/1/kids/0/v; reading the key "v" of VdTree)",
                   "/kids/1/kids/0/v");
    auto tree = VdTree();
    for (auto i = 0; i < 300; ++i) tree = VdTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(e.path(), "");
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    EXPECT_THROW((void)yyjson::write(tree, yyjson::with_validation), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);

    auto loop = std::make_shared<VdLoop>();
    loop->next = loop;
    const auto l = validation_error([&] { yyjson::validate(*loop); });
    EXPECT_TRUE(std::string_view(l.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << l.what();
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    loop->next.reset();
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
                   R"(the string does not match the pattern "^a" at /code; reading the key "code" of VdMore)", "/code");
    expect_invalid(with<VdMore>([](auto& t) { t.word = "日本"; }),
                   R"(the string is longer than the maximum length 1 at /word; reading the key "word" of VdMore)",
                   "/word");
    expect_invalid(with<VdMore>([](auto& t) { t.v = {10, 1}; }),
                   R"(the array has fewer than 2 items that satisfy contains at /v; reading the key "v" of VdMore)",
                   "/v");
    expect_invalid(with<VdMore>([](auto& t) { t.v = {10, 11, 12, 13}; }),
                   R"(the array has more than 3 items that satisfy contains at /v; reading the key "v" of VdMore)",
                   "/v");
    expect_invalid(with<VdMore>([](auto& t) { t.pair = {2, 1}; }),
                   R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of VdMore)",
                   "/pair");
    expect_invalid(
        with<VdMore>([](auto& t) { t.colors = {{VdColor::green, 0}}; }),
        R"(the key is not one of "red" at /colors/green; reading a key of the value at the key "colors" of VdMore)",
        "/colors/green");
    expect_invalid(with<VdMore>([](auto& t) { t.unit = std::make_shared<std::string>("km"); }),
                   R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of VdMore)", "/unit");
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
        "the value must not be null at /o; reading std::optional<double> under yyjson::reject_null_as_missing";
    auto e = validation_error([&] { (void)yyjson::write(VdOptionalReal{nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    EXPECT_EQ(e.path(), "/o");
    e = validation_error([&] { (void)yyjson::write(VdNullableReals{nan, {}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    const auto nulls = VdNullableReals{std::nullopt, {nan, 1}, nan};
    EXPECT_EQ(std::string(yyjson::write(nulls, as_null, with_validation)), R"({"v":[null,1.0],"n":null})");

    static_assert(!yyjson::readable<VdUnreadable>);
    e = validation_error([&] { (void)yyjson::write(VdUnreadable{"x", nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /d; reading double");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(VdUnreadable{"x", nan}); }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdUnreadable)");
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
              R"(the array has more items than the maximum 1 at /v; reading the key "v" of VdUnwritable)");
    EXPECT_EQ(e.path(), "/v");
    EXPECT_THROW((void)yyjson::write(many, with_validation), yyjson::write_error);
    const auto invalid = VdUnwritable{{}, "\xff\xfe"};
    e = validation_error([&] { yyjson::validate(invalid); });
    EXPECT_EQ(std::string(e.what()),
              R"(the string is longer than the maximum length 1 at /s; reading the key "s" of VdUnwritable)");
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
    expect_invalid(
        VdChainTop{VdChainOut{VdChainIn{std::nullopt}}},
        R"(the value is not valid at /out; validating VdChainTop: the field "out" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/out", read_leg::skipped);
}

// an enumerator without a name is an error of write, which the check reports as write does
struct VdNamedColor
{
    VdColor c = VdColor::red;
};
template <>
inline constexpr auto yyjson::annotations<VdNamedColor> = yyjson::annotate(yyjson::field("c", yyjson::enum_("red")));

TEST(Validate, UnnamedEnumerator)
{
    const auto unnamed = VdNamedColor{static_cast<VdColor>(7)};
    const auto write_error = [](auto&& f) {
        try
        {
            f();
        }
        catch (const yyjson::write_error& e)
        {
            EXPECT_EQ(e.code(), YYJSON_WRITE_ERROR_INVALID_VALUE_TYPE);
            return std::string(e.what());
        }
        ADD_FAILURE() << "no yyjson::write_error";
        return std::string();
    };
    const auto plain = write_error([&] { (void)yyjson::write(unnamed); });
    EXPECT_EQ(write_error([&] { (void)yyjson::write(unnamed, yyjson::with_validation); }), plain);
    EXPECT_EQ(write_error([&] { yyjson::validate(unnamed); }), plain);
    EXPECT_EQ(write_error([&] { yyjson::validate(static_cast<VdColor>(7)); }),
              write_error([&] { (void)yyjson::write(static_cast<VdColor>(7)); }));
    using unnamed_keys = std::map<VdColor, int>;
    const auto keys = unnamed_keys{{static_cast<VdColor>(7), 1}};
    EXPECT_EQ(write_error([&] { yyjson::validate(keys); }), write_error([&] { (void)yyjson::write(keys); }));
    EXPECT_NO_THROW(yyjson::validate(VdNamedColor{}));
    EXPECT_NO_THROW(yyjson::validate(unnamed_keys{{VdColor::red, 1}}));
}

// NOLINTEND
