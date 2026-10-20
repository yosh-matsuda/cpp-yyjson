#include <regex>
#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::pattern` matches with the engine that the program gives
template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    explicit pattern_matcher(std::string_view pattern) : regex(pattern.begin(), pattern.end()) {}
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), regex); }
};

struct Bounded
{
    int i = 0;
    double d = 1;
    unsigned u = 0;
    std::optional<int> o;
    bool operator==(const Bounded&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Bounded> =
    yyjson::annotate(yyjson::field("i", yyjson::minimum(0), yyjson::maximum(10)),
                     yyjson::field("d", yyjson::exclusive_minimum(0), yyjson::exclusive_maximum(1.5)),
                     yyjson::field("u", yyjson::multiple_of(3)), yyjson::field("o", yyjson::minimum(-5)));

// bounds of another kind than the field, near the limits of the field
struct MixedBounds
{
    std::int64_t big = 0;
    std::uint64_t ubig = 0;
    double real = 0;
    std::int64_t negative = 0;
    float quarter = 0;
    bool operator==(const MixedBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MixedBounds> =
    yyjson::annotate(yyjson::field("big", yyjson::maximum(9007199254740992.0)),
                     yyjson::field("ubig", yyjson::minimum(-1), yyjson::multiple_of(9223372036854775808ULL)),
                     yyjson::field("real", yyjson::maximum(9007199254740993LL)),
                     yyjson::field("negative", yyjson::exclusive_maximum(-2.5), yyjson::multiple_of(7)),
                     yyjson::field("quarter", yyjson::multiple_of(0.25)));

struct Texts
{
    std::string name;
    std::vector<int> list;
    std::string code = "ABC";
    std::map<std::string, int> map;
    std::optional<std::string> note;
    bool operator==(const Texts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Texts> =
    yyjson::annotate(yyjson::field("name", yyjson::min_length(2), yyjson::max_length(3)),
                     yyjson::field("list", yyjson::min_items(1), yyjson::max_items(2)),
                     yyjson::field("code", yyjson::pattern("^[A-Z]{3}$")), yyjson::field("map", yyjson::max_items(1)),
                     yyjson::field("note", yyjson::pattern("^n")));

bool is_even(int v) { return v % 2 == 0; }
struct Validated
{
    int even = 0;
    std::optional<std::string> text;
    std::optional<int> nonzero;
    bool operator==(const Validated&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Validated> =
    yyjson::annotate(yyjson::field("even", yyjson::validate<is_even>),
                     yyjson::field("text", yyjson::validate<[](const std::string& s) noexcept { return s != "bad"; }>),
                     yyjson::field("nonzero", yyjson::validate<[](const std::optional<int>& v) { return v != 0; }>));

struct ConstrainedVisitable
{
    int a = 0;
    std::string b;
    bool operator==(const ConstrainedVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedVisitable> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("b", yyjson::max_length(1)));
VISITABLE_STRUCT(ConstrainedVisitable, a, b);

// a renamed field is constrained under its key, and the direct reader compares more fields than in their order
struct ConstrainedLarge
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    int renamed = 0;
    bool operator==(const ConstrainedLarge&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedLarge> = yyjson::annotate(
    yyjson::field("i", yyjson::maximum(1)), yyjson::field("renamed", yyjson::rename("r"), yyjson::minimum(0)));

struct ConstrainedOuter
{
    std::vector<Bounded> items;
    Texts texts;
    bool operator==(const ConstrainedOuter&) const = default;
};

// a skipped field before the constrained ones, a character type and a std::optional field with a default value
struct ConstrainedSkipped
{
    int skipped = 0;
    int a = 0;
    std::string s = "xy";
    char c = 1;
    std::optional<int> o = 5;
    bool operator==(const ConstrainedSkipped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedSkipped> =
    yyjson::annotate(yyjson::field("skipped", yyjson::skip), yyjson::field("a", yyjson::maximum(5)),
                     yyjson::field("s", yyjson::min_length(2)), yyjson::field("c", yyjson::minimum(1)),
                     yyjson::field("o", yyjson::maximum(3)));
// the members of VISITABLE_STRUCT in another order than they are declared, and a std::optional field with a default
// value
struct ConstrainedReordered
{
    int a = 0;
    std::string b;
    std::optional<int> o = 5;
    bool operator==(const ConstrainedReordered&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedReordered> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("b", yyjson::max_length(1)),
                     yyjson::field("o", yyjson::maximum(3)));
VISITABLE_STRUCT(ConstrainedReordered, b, o, a);

#if FIELD_RFL_ANNOTATIONS
struct ConstrainedAnnotated
{
    [[ = yyjson::minimum(1), = yyjson::maximum(3) ]] int n = 1;
    [[= yyjson::pattern("^x")]] std::string s = "x";
    [[= yyjson::validate<is_even>]] int even = 0;
    bool operator==(const ConstrainedAnnotated&) const = default;
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

    // every read of `json` gives `expected`, and the reader without a document reads it
    template <typename T>
    void expect_accepted(std::string_view json, const T& expected)
    {
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        auto into = T();
        yyjson::read_into(json, into);
        EXPECT_EQ(into, expected) << json;
        auto merged = T();
        yyjson::merge_into(json, merged);
        EXPECT_EQ(merged, expected) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_TRUE(direct.has_value()) << json;
        EXPECT_EQ(*direct, read_or_error([&]() -> const T& { return expected; })) << json;
        EXPECT_TRUE(expect_read_into(T(), json)) << json;
#endif
    }

    // every read of `json`, with and without a document, throws `bad_cast` with `message` about the value at
    // `path`, and the reader without a document leaves it to the document
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(std::string_view(e.what()).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::ReadFlag::AllowComments); }));
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
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(T(), json, &merged_directly));
        EXPECT_FALSE(merged_directly) << json;
#endif
    }
}  // namespace

TEST(Constraints, Numbers)
{
    expect_accepted<Bounded>(R"({"i":0,"d":0.5,"u":9,"o":-5})", {0, 0.5, 9, -5});
    expect_accepted<Bounded>(R"({"i":10,"d":1.25,"u":0})", {10, 1.25, 0, {}});
    expect_accepted<Bounded>(R"({"o":null})", {});

    expect_rejected<Bounded>(R"({"i":-1})", R"(the value of the key "i" of Bounded is less than the minimum 0)", "/i");
    expect_rejected<Bounded>(R"({"i":11})", R"(the value of the key "i" of Bounded is greater than the maximum 10)",
                             "/i");
    expect_rejected<Bounded>(
        R"({"d":0})", R"(the value of the key "d" of Bounded is not greater than the exclusive minimum 0)", "/d");
    expect_rejected<Bounded>(R"({"d":1.5})",
                             R"(the value of the key "d" of Bounded is not less than the exclusive maximum 1.5)", "/d");
    expect_rejected<Bounded>(R"({"u":4})", R"(the value of the key "u" of Bounded is not a multiple of 3)", "/u");
    expect_rejected<Bounded>(R"({"i":1,"o":-6})", R"(the value of the key "o" of Bounded is less than the minimum -5)",
                             "/o");
    // the key that the object leaves out is not checked, and the field keeps a value out of the bounds
    EXPECT_EQ(yyjson::read<Bounded>(R"({"i":1})").d, 1);
    // `yyjson::strict` checks the keys besides
    EXPECT_EQ(yyjson::read<Bounded>(R"({"i":1,"d":1,"u":3})", yyjson::strict), (Bounded{1, 1, 3, {}}));
    EXPECT_THROW((void)yyjson::read<Bounded>(R"({"i":1,"d":1,"u":2})", yyjson::strict), yyjson::bad_cast);

    expect_accepted<MixedBounds>(R"({"big":9007199254740992,"ubig":9223372036854775808,"real":9007199254740992,)"
                                 R"("negative":-7,"quarter":0.75})",
                                 {9007199254740992, 9223372036854775808ULL, 9007199254740992.0, -7, 0.75F});
    expect_accepted<MixedBounds>(R"({"negative":-9223372036854775807})", {0, 0, 0, -9223372036854775807LL, 0});
    expect_rejected<MixedBounds>(
        R"({"big":9007199254740993})",
        R"(the value of the key "big" of MixedBounds is greater than the maximum 9007199254740992)", "/big");
    expect_rejected<MixedBounds>(
        R"({"ubig":1})", R"(the value of the key "ubig" of MixedBounds is not a multiple of 9223372036854775808)",
        "/ubig");
    expect_rejected<MixedBounds>(
        R"({"real":9007199254740994})",
        R"(the value of the key "real" of MixedBounds is greater than the maximum 9007199254740993)", "/real");
    expect_rejected<MixedBounds>(
        R"({"negative":-2})",
        R"(the value of the key "negative" of MixedBounds is not less than the exclusive maximum -2.5)", "/negative");
    expect_rejected<MixedBounds>(
        R"({"negative":-6})", R"(the value of the key "negative" of MixedBounds is not a multiple of 7)", "/negative");
    expect_rejected<MixedBounds>(
        R"({"quarter":0.3})", R"(the value of the key "quarter" of MixedBounds is not a multiple of 0.25)", "/quarter");
}

TEST(Constraints, NaN)
{
    // NaN is in no bounds
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    EXPECT_THROW((void)yyjson::read<Bounded>(R"({"d":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":Infinity})", flag), yyjson::bad_cast);
}

TEST(Constraints, Texts)
{
    expect_accepted<Texts>(R"({"name":"ab","list":[1],"code":"XYZ","map":{"a":1},"note":"nb"})",
                           {"ab", {1}, "XYZ", {{"a", 1}}, "nb"});
    // the length is in code points
    expect_accepted<Texts>(R"({"name":"日本語","list":[1,2]})", {"日本語", {1, 2}, "ABC", {}, {}});

    expect_rejected<Texts>(R"({"name":"a"})",
                           R"(the value of the key "name" of Texts is shorter than the minimum length 2)", "/name");
    expect_rejected<Texts>(R"({"name":"abcd"})",
                           R"(the value of the key "name" of Texts is longer than the maximum length 3)", "/name");
    expect_rejected<Texts>(R"({"list":[]})",
                           R"(the value of the key "list" of Texts has fewer items than the minimum 1)", "/list");
    expect_rejected<Texts>(R"({"list":[1,2,3]})",
                           R"(the value of the key "list" of Texts has more items than the maximum 2)", "/list");
    expect_rejected<Texts>(R"({"code":"abc"})",
                           R"(the value of the key "code" of Texts does not match the pattern "^[A-Z]{3}$")", "/code");
    expect_rejected<Texts>(R"({"map":{"a":1,"b":2}})",
                           R"(the value of the key "map" of Texts has more items than the maximum 1)", "/map");
    expect_rejected<Texts>(R"({"note":"x"})", R"(the value of the key "note" of Texts does not match the pattern "^n")",
                           "/note");
}

TEST(Constraints, Validate)
{
    expect_accepted<Validated>(R"({"even":2,"text":"ok","nonzero":null})", {2, "ok", {}});
    expect_accepted<Validated>(R"({"nonzero":1})", {0, {}, 1});

    expect_rejected<Validated>(R"({"even":1})",
                               R"(the value of the key "even" of Validated is rejected by yyjson::validate)", "/even");
    expect_rejected<Validated>(R"({"text":"bad"})",
                               R"(the value of the key "text" of Validated is rejected by yyjson::validate)", "/text");
    expect_rejected<Validated>(R"({"nonzero":0})",
                               R"(the value of the key "nonzero" of Validated is rejected by yyjson::validate)",
                               "/nonzero");
}

TEST(Constraints, Structs)
{
    expect_accepted<ConstrainedVisitable>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedVisitable>(
        R"({"a":0})", R"(the value of the key "a" of ConstrainedVisitable is less than the minimum 1)", "/a");
    expect_rejected<ConstrainedVisitable>(
        R"({"b":"xy"})", R"(the value of the key "b" of ConstrainedVisitable is longer than the maximum length 1)",
        "/b");

    expect_accepted<ConstrainedLarge>(R"({"i":1,"r":0,"a":5})", {5, 0, 0, 0, 0, 0, 0, 0, 1, 0});
    expect_rejected<ConstrainedLarge>(
        R"({"a":1,"i":2})", R"(the value of the key "i" of ConstrainedLarge is greater than the maximum 1)", "/i");
    expect_rejected<ConstrainedLarge>(
        R"({"r":-1})", R"(the value of the key "r" of ConstrainedLarge is less than the minimum 0)", "/r");

    expect_rejected<ConstrainedOuter>(R"({"items":[{"i":1},{"i":12}]})",
                                      R"(the value of the key "i" of Bounded is greater than the maximum 10)",
                                      "/items/1/i");
    expect_rejected<ConstrainedOuter>(R"({"texts":{"name":"x"}})",
                                      R"(the value of the key "name" of Texts is shorter than the minimum length 2)",
                                      "/texts/name");
    // the position of the value is added
    const auto e = bad_cast_of([] { (void)yyjson::read<ConstrainedOuter>(R"({"items":[{"i":-3}]})"); });
    ASSERT_TRUE(e.position().has_value());
    EXPECT_EQ(e.position()->pos, 15);

    // the fields are checked by their own constraints whatever their places in the struct
    expect_accepted<ConstrainedSkipped>(R"({"a":5,"s":"xy","c":1})", {0, 5, "xy", 1, 5});
    expect_rejected<ConstrainedSkipped>(
        R"({"a":6})", R"(the value of the key "a" of ConstrainedSkipped is greater than the maximum 5)", "/a");
    expect_rejected<ConstrainedSkipped>(
        R"({"a":1,"s":"x"})", R"(the value of the key "s" of ConstrainedSkipped is shorter than the minimum length 2)",
        "/s");
    expect_rejected<ConstrainedSkipped>(
        R"({"c":0})", R"(the value of the key "c" of ConstrainedSkipped is less than the minimum 1)", "/c");
    expect_rejected<ConstrainedSkipped>(
        R"({"o":4})", R"(the value of the key "o" of ConstrainedSkipped is greater than the maximum 3)", "/o");
    expect_accepted<ConstrainedReordered>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedReordered>(
        R"({"b":"xy","a":1})",
        R"(the value of the key "b" of ConstrainedReordered is longer than the maximum length 1)", "/b");
    expect_rejected<ConstrainedReordered>(
        R"({"b":"x","a":0})", R"(the value of the key "a" of ConstrainedReordered is less than the minimum 1)", "/a");

    // null leaves a field at its value in `T()`, which is not checked
    expect_accepted<ConstrainedSkipped>(R"({"o":null})", {});
    expect_accepted<ConstrainedReordered>(R"({"o":null})", {});
    expect_rejected<ConstrainedReordered>(
        R"({"o":4})", R"(the value of the key "o" of ConstrainedReordered is greater than the maximum 3)", "/o");
    EXPECT_THROW((void)yyjson::read<ConstrainedSkipped>(R"({"o":null})", yyjson::strict), yyjson::bad_cast);
}

TEST(Constraints, Merge)
{
    // null resets a field without a check, and a struct merged into a field is checked as a whole
    auto t = Bounded{5, 1, 3, 2};
    yyjson::merge_into(R"({"o":null,"i":null})", t);
    EXPECT_EQ(t, (Bounded{0, 1, 3, {}}));
    EXPECT_THROW(yyjson::merge_into(R"({"u":5})", t), yyjson::bad_cast);

    auto outer = ConstrainedOuter{{}, {"ab", {1}, "ABC", {}, {}}};
    EXPECT_EQ(expect_merge_into(outer, R"({"texts":{"list":[1,2]}})"),
              R"({"items":[],"texts":{"name":"ab","list":[1,2],"code":"ABC","map":{}}})");
    EXPECT_THROW(yyjson::merge_into(R"({"texts":{"code":"x"}})", outer), yyjson::bad_cast);
}

TEST(Constraints, Write)
{
    // a value out of the bounds is written as it is
    EXPECT_EQ(written(Bounded{-1, 0, 1, {}}), R"({"i":-1,"d":0.0,"u":1})");
}

#if FIELD_RFL_ANNOTATIONS
TEST(Constraints, Annotations)
{
    expect_accepted<ConstrainedAnnotated>(R"({"n":3,"s":"xy","even":4})", {3, "xy", 4});
    expect_rejected<ConstrainedAnnotated>(
        R"({"n":4})", R"(the value of the key "n" of ConstrainedAnnotated is greater than the maximum 3)", "/n");
    expect_rejected<ConstrainedAnnotated>(
        R"({"s":"y"})", R"(the value of the key "s" of ConstrainedAnnotated does not match the pattern "^x")", "/s");
    expect_rejected<ConstrainedAnnotated>(
        R"({"even":3})", R"(the value of the key "even" of ConstrainedAnnotated is rejected by yyjson::validate)",
        "/even");
}
#endif

// NOLINTEND
