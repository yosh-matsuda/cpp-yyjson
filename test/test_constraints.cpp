#include <regex>
#include "test_common.hpp"

// NOLINTBEGIN

// The keywords of the values of the fields: the bounds of a number, the lengths and the patterns of a string,
// `yyjson::format_assertion`, `yyjson::const_`, `yyjson::enum_` and `yyjson::satisfies`

// `yyjson::pattern` matches with the engine that the program gives
template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    explicit pattern_matcher(std::string_view pattern) : regex(pattern.begin(), pattern.end())
    {
        // the format "regex" passes on a std::bad_alloc of the matcher
        if (pattern == "<out of memory>") throw std::bad_alloc();
    }
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), regex); }
};

// a format that the program checks
template <>
struct yyjson::format_checker<>
{
    std::string_view name;
    explicit format_checker(std::string_view name) : name(name) {}
    bool operator()(std::string_view s) const
    {
        return name == "upper" && std::ranges::all_of(s, [](char c) { return c >= 'A' && c <= 'Z'; });
    }
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
    double cents = 0;
    std::int64_t tenths = 0;
    bool operator==(const MixedBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MixedBounds> = yyjson::annotate(
    yyjson::field("big", yyjson::maximum(9007199254740992.0)),
    yyjson::field("ubig", yyjson::minimum(-1), yyjson::multiple_of(9223372036854775808ULL)),
    yyjson::field("real", yyjson::maximum(9007199254740993LL)),
    yyjson::field("negative", yyjson::exclusive_maximum(-2.5), yyjson::multiple_of(7)),
    yyjson::field("quarter", yyjson::multiple_of(0.25)), yyjson::field("cents", yyjson::multiple_of(0.01)),
    yyjson::field("tenths", yyjson::multiple_of(0.1)));

// bounds of a float field, which compares with them rounded to a float
struct FloatBounds
{
    float tenth = 0;
    float under = 0;
    float big = 16777216.0F;
    bool operator==(const FloatBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatBounds> = yyjson::annotate(
    yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("under", yyjson::exclusive_maximum(0.1)),
    yyjson::field("big", yyjson::minimum(16777217)));

struct Texts
{
    std::string name;
    std::string code = "ABC";
    std::optional<std::string> note;
    bool operator==(const Texts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Texts> = yyjson::annotate(
    yyjson::field("name", yyjson::min_length(2), yyjson::max_length(3)),
    yyjson::field("code", yyjson::pattern("^[A-Z]{3}$")), yyjson::field("note", yyjson::pattern("^n")));

// several patterns at one level, which a value must all match, between other keywords too
struct Patterns
{
    std::string s;
    std::optional<std::string> o;
    std::string v;
    bool operator==(const Patterns&) const = default;
};
// a pattern made from a std::string_view, which the setting points to
inline constexpr std::string_view digits = "^[0-9]*$";
template <>
inline constexpr auto yyjson::annotations<Patterns> = yyjson::annotate(
    yyjson::field("s", yyjson::pattern("^a"), yyjson::pattern("z$")),
    yyjson::field("o", yyjson::pattern("^o"), yyjson::min_length(2), yyjson::pattern("o$"), yyjson::pattern("x")),
    yyjson::field("v", yyjson::pattern(digits)));

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
    yyjson::annotate(yyjson::field("even", yyjson::satisfies<is_even>),
                     yyjson::field("text", yyjson::satisfies<[](const std::string& s) noexcept { return s != "bad"; }>),
                     yyjson::field("nonzero", yyjson::satisfies<[](const std::optional<int>& v) { return v != 0; }>));

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

// a field that keeps its default value, which is out of the bounds, when the key is missing or null
struct ConstrainedDefaults
{
    int a = -1;
    std::optional<int> o = -1;
    bool operator==(const ConstrainedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConstrainedDefaults> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(0), yyjson::default_), yyjson::field("o", yyjson::minimum(0)));

// a struct read as its only field takes the keywords of that field
struct Percent
{
    int value = 0;
    bool operator==(const Percent&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Percent> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::minimum(0), yyjson::maximum(100)));
struct Progress
{
    Percent done;
    std::vector<Percent> steps;
    bool operator==(const Progress&) const = default;
};

// the values that yyjson::nullable, yyjson::indirect and the owning pointers hold, as those of std::optional
bool fits_in_three(const std::string& s) { return s.size() < 4; }
struct Nullables
{
    yyjson::nullable<std::string> code;
    yyjson::nullable<double> ratio;
    std::shared_ptr<std::string> name;
    yyjson::indirect<int> boxed;
    bool operator==(const Nullables& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<Nullables> = yyjson::annotate(
    yyjson::field("code", yyjson::max_length(3)), yyjson::field("ratio", yyjson::maximum(1)),
    yyjson::field("name", yyjson::satisfies<fits_in_three>), yyjson::field("boxed", yyjson::minimum(0)));
struct Owned
{
    std::unique_ptr<int> count;
    std::optional<std::unique_ptr<int>> level;
};
template <>
inline constexpr auto yyjson::annotations<Owned> = yyjson::annotate(
    yyjson::field("count", yyjson::minimum(0)),
    yyjson::field("level", yyjson::maximum(5), yyjson::satisfies<[](const int& v) { return v != 3; }>));

// const_ and enum_ reject JSON null unless they list nullptr, as JSON Schema's const and enum
struct NullableListed
{
    yyjson::nullable<std::string> unit;
    yyjson::nullable<int> level;
    std::shared_ptr<int> count;
    std::optional<std::string> note;
    std::optional<yyjson::nullable<int>> mode;
    std::shared_ptr<std::vector<int>> none;
    bool operator==(const NullableListed& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<NullableListed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm")), yyjson::field("level", yyjson::enum_(1, 2, nullptr)),
    yyjson::field("count", yyjson::const_(1)), yyjson::field("note", yyjson::const_("x")),
    yyjson::field("mode", yyjson::const_(nullptr)), yyjson::field("none", yyjson::const_(nullptr)));

// the string settings written as string literals, which hold their characters before C++26
struct Literals
{
    std::string unit;
    std::string day = "2024-01-01";
    bool operator==(const Literals&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Literals> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm")), yyjson::field("day", yyjson::format_assertion("date")));
// they are template arguments
template <auto... Settings>
struct setting_arguments
{
};
template struct setting_arguments<yyjson::pattern("^a"), yyjson::format("date"), yyjson::format_assertion("date"),
                                  yyjson::const_("c"), yyjson::enum_("e", 1), yyjson::const_(yyjson::json_text("[1]"))>;

// const_ and enum_ compare the value read as a value of the type of the field
struct Listed
{
    std::string unit = "m";
    int version = 2;
    float ratio = 0.1F;
    bool flag = true;
    std::uint64_t big = 0;
    std::optional<int> level;
    int odd = 1;
    bool operator==(const Listed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Listed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm", "m")), yyjson::field("version", yyjson::const_(2)),
    yyjson::field("ratio", yyjson::enum_(0.1, 0.5)), yyjson::field("flag", yyjson::const_(true)),
    yyjson::field("big", yyjson::enum_(-1, 0, 18446744073709551615ULL)), yyjson::field("level", yyjson::enum_(1, 2, 3)),
    yyjson::field("odd", yyjson::const_(1), yyjson::enum_(1, 3)));

// const_ and enum_ of an enumeration written by name take the JSON names of its enumerators
enum class Tone
{
    low,
    mid,
    high
};
template <>
inline constexpr auto yyjson::annotations<Tone> = yyjson::annotate(yyjson::enumerator("high", yyjson::rename("HIGH")));
struct ListedEnums
{
    Tone first = Tone::low;
    Tone loud = Tone::high;
    std::optional<Tone> maybe;
    bool operator==(const ListedEnums&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedEnums> =
    yyjson::annotate(yyjson::field("first", yyjson::const_("low")), yyjson::field("loud", yyjson::enum_("mid", "HIGH")),
                     yyjson::field("maybe", yyjson::const_("mid")));

// const_ and enum_ of JSON texts, read into the type of the value that they check
struct Spot
{
    int x = 0;
    int y = 0;
    bool operator==(const Spot&) const = default;
};
struct ListedTexts
{
    Spot at{};
    std::vector<int> pair = {1, 2};
    std::optional<std::vector<double>> scale{};
    std::variant<int, std::vector<int>> mixed{};
    std::vector<std::pair<std::string, int>> order = {{"a", 2}, {"b", 1}};
    bool operator==(const ListedTexts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedTexts> = yyjson::annotate(
    yyjson::field("at",
                  yyjson::enum_(yyjson::json_text(R"({ "y": 0, "x": 0 })"), yyjson::json_text(R"({"x":1,"y":1})"))),
    yyjson::field("pair", yyjson::const_(yyjson::json_text("[1, 2]"))),
    yyjson::field("scale", yyjson::enum_(yyjson::json_text("[1]"), yyjson::json_text("[2.5, 2.5]"))),
    yyjson::field("mixed", yyjson::enum_(yyjson::json_text("0"), yyjson::json_text("[0]"), yyjson::json_text("{}"))),
    yyjson::field("order", yyjson::const_(yyjson::json_text(R"({"a":2,"b":1})"))));
// a DOM value compares as JSON: 1 equals 1.0, keys in any order
struct ListedDom
{
    yyjson::value any;
};
template <>
inline constexpr auto yyjson::annotations<ListedDom> = yyjson::annotate(
    yyjson::field("any", yyjson::enum_(yyjson::json_text(R"({"a":[1,2.0],"b":true})"), yyjson::json_text("null"))));
// a constant that the type cannot read
struct UnreadableText
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<UnreadableText> =
    yyjson::annotate(yyjson::field("n", yyjson::const_(yyjson::json_text(R"("one")"))));

// the formats that the library checks, "regex" with the engine of pattern_matcher<>, one that the program checks,
// and yyjson::format, which checks nothing
struct Formatted
{
    std::string when = "2024-02-29T23:59:60Z";
    std::optional<std::string> mail{};
    std::string code = "X";
    std::optional<std::string> re{};
    std::string day = "not a date";
    bool operator==(const Formatted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Formatted> = yyjson::annotate(
    yyjson::field("when", yyjson::format_assertion("date-time")),
    yyjson::field("mail", yyjson::format_assertion("email")), yyjson::field("code", yyjson::format_assertion("upper")),
    yyjson::field("re", yyjson::format_assertion("regex")), yyjson::field("day", yyjson::format("date")));
// yyjson::format names any format, which needs no checker
struct Described
{
    std::string id;
    bool operator==(const Described&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Described> =
    yyjson::annotate(yyjson::field("id", yyjson::format("my-own-format")));

// The rules that the static assertions enforce, tested on the conditions that they assert
namespace constraint_rules
{
    // the table of the keys of a struct of the members `v` and `w`
    template <typename... As>
    consteval auto table_error(const As&... annotations)
    {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, false};
        (builder.apply_annotation(annotations), ...);
        return builder.result().error;
    }
    using enum yyjson::detail::field_key_error;
    using yyjson::field, yyjson::minimum, yyjson::maximum;
    static_assert(table_error(field("v", minimum(0)), field("w", yyjson::pattern("^a"))) == none);
    static_assert(table_error(minimum(0)) == struct_constraint);
    static_assert(table_error(field("v", yyjson::skip, minimum(0))) == skipped_with_setting);
    static_assert(table_error(field("v", minimum(0)), field("v", yyjson::skip)) == skipped_with_setting);
    static_assert(table_error(field("w", minimum(0), yyjson::adjacent_tag("t", {"x"}))) == tagged_constraint);
    static_assert(table_error(field("v", minimum(0)), field("w", yyjson::adjacent_tag("t", {"x"}))) == none);
}  // namespace constraint_rules

// the tree of the keywords of the field `v` of a struct, by its error
#define CONSTRAINT_RULE(NAME, TYPE, ...) \
    struct NAME                          \
    {                                    \
        TYPE v;                          \
    };                                   \
    template <>                          \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(yyjson::field("v", __VA_ARGS__));
CONSTRAINT_RULE(MinimumTwice, int, yyjson::minimum(0), yyjson::minimum(1))
CONSTRAINT_RULE(ConstTwice, int, yyjson::const_(1), yyjson::const_(1))
CONSTRAINT_RULE(EnumTwice, int, yyjson::enum_(1), yyjson::enum_(2))
CONSTRAINT_RULE(FormatTwice, std::string, yyjson::format("date"), yyjson::format("time"))
CONSTRAINT_RULE(SatisfiesTwice, int, yyjson::satisfies<is_even>, yyjson::satisfies<is_even>)
CONSTRAINT_RULE(PatternsTwice, std::string, yyjson::pattern("^a"), yyjson::pattern("^a"))
CONSTRAINT_RULE(MultipleOfZero, int, yyjson::multiple_of(0))
CONSTRAINT_RULE(MultipleOfNegative, double, yyjson::multiple_of(-0.5))
CONSTRAINT_RULE(BoundsInverted, int, yyjson::minimum(2), yyjson::maximum(1))
CONSTRAINT_RULE(BoundsExclusive, int, yyjson::exclusive_minimum(1), yyjson::maximum(1))
CONSTRAINT_RULE(BoundsEqual, int, yyjson::minimum(1), yyjson::maximum(1))
CONSTRAINT_RULE(LengthsInverted, std::string, yyjson::min_length(3), yyjson::max_length(2))
CONSTRAINT_RULE(ConstOutside, int, yyjson::const_(5), yyjson::maximum(4))
CONSTRAINT_RULE(EnumOutside, std::string, yyjson::enum_("abc", "de"), yyjson::max_length(1))
CONSTRAINT_RULE(EnumPartlyInside, std::string, yyjson::enum_("abc", "d"), yyjson::max_length(1))
#undef CONSTRAINT_RULE
namespace constraint_tree_rules
{
    template <typename T>
    constexpr auto tree_error = yyjson::detail::make_field_tree<T, 0>().error;
    using enum yyjson::detail::constraint_error;
    static_assert(tree_error<MinimumTwice> == keyword_twice && tree_error<ConstTwice> == keyword_twice &&
                  tree_error<EnumTwice> == keyword_twice && tree_error<FormatTwice> == keyword_twice &&
                  tree_error<SatisfiesTwice> == keyword_twice);
    static_assert(tree_error<PatternsTwice> == none);
    static_assert(tree_error<MultipleOfZero> == multiple_of_not_positive &&
                  tree_error<MultipleOfNegative> == multiple_of_not_positive);
    static_assert(tree_error<BoundsInverted> == reads_nothing && tree_error<BoundsExclusive> == reads_nothing &&
                  tree_error<LengthsInverted> == reads_nothing && tree_error<ConstOutside> == reads_nothing &&
                  tree_error<EnumOutside> == reads_nothing);
    static_assert(tree_error<BoundsEqual> == none && tree_error<EnumPartlyInside> == none);

    // the keywords that a field without them leaves unchecked, and those that are no check
    static_assert(!yyjson::detail::has_constraints<Spot> && yyjson::detail::has_constraints<Bounded>);
    static_assert(yyjson::detail::constrained_field<Bounded, 0> && !yyjson::detail::constrained_field<Described, 0>);
}  // namespace constraint_tree_rules

// a bound is compared as a double, so a long double that it would round does not compile; Clang 17 and older
// take no floating-point template argument
#if !defined(__clang__) || __clang_major__ >= 18
template <auto V>
concept bound_accepted = requires { typename std::integral_constant<bool, (yyjson::minimum(V), true)>; };
static_assert(bound_accepted<1.5L> && bound_accepted<9007199254740992.0L>);
static_assert(!bound_accepted<9007199254740993.0L> || sizeof(long double) == sizeof(double));
#endif

#if FIELD_RFL_ANNOTATIONS
struct ConstrainedAnnotated
{
    [[ = yyjson::minimum(1), = yyjson::maximum(3) ]] int n = 1;
    [[ = yyjson::pattern("^p"), = yyjson::pattern("q$") ]] std::string p = "pq";
    [[= yyjson::pattern("^x")]] std::string s = "x";
    [[= yyjson::satisfies<is_even>]] int even = 0;
    bool operator==(const ConstrainedAnnotated&) const = default;
};

struct KeywordsAnnotated
{
    [[= yyjson::const_(2)]] int version = 2;
    [[= yyjson::enum_("a", "b")]] std::string unit = "a";
    [[= yyjson::format_assertion("date")]] std::string day = "2024-01-01";
    [[= yyjson::format("date")]] std::string note = "x";
    [[ = yyjson::multiple_of(0.5), = yyjson::exclusive_maximum(2) ]] std::optional<double> step{};
    [[= yyjson::enum_(yyjson::json_text("[1,2]"), yyjson::json_text("[]"))]] std::vector<int> list{};
    bool operator==(const KeywordsAnnotated&) const = default;
};
#endif

namespace
{
    // `e.what()` without the "struct ", "class " and "enum " that MSVC writes before the name of a type
    std::string message_of(const std::exception& e)
    {
        auto message = std::string(e.what());
#if defined(_MSC_VER) && !defined(__clang__)
        for (const std::string_view keyword : {"struct ", "class ", "enum "})
        {
            for (auto i = message.find(keyword); i != std::string::npos; i = message.find(keyword, i + 1))
            {
                const auto c = i == 0 ? ' ' : message[i - 1];
                if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_') message.erase(i, keyword.size());
            }
        }
#endif
        return message;
    }

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
    // `path`, and the reader without a document leaves it to the document; `merge_into` too unless `json` is a
    // null that resets the field
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path, bool merged = true)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::read_into(json, t);
        }));
        if (merged)
            check(bad_cast_of([&] {
                auto t = T();
                yyjson::merge_into(json, t);
            }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(); }));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
        auto merged_directly = false;
        if (merged)
        {
            static_cast<void>(expect_merge_into(T(), json, &merged_directly));
            EXPECT_FALSE(merged_directly) << json;
        }
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

    // a multiple of a decimal is decided in decimal, which the quotient in binary is not
    for (const auto* json : {R"({"cents":19.99})", R"({"cents":0.07})", R"({"cents":-1234567.89})", R"({"cents":1e-2})",
                             R"({"tenths":-30})", R"({"tenths":0})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<MixedBounds>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).cast<MixedBounds>()) << json;
    }
    expect_rejected<MixedBounds>(R"({"cents":0.001})",
                                 R"(the value of the key "cents" of MixedBounds is not a multiple of 0.01)", "/cents");
    expect_rejected<MixedBounds>(R"({"cents":19.995})",
                                 R"(the value of the key "cents" of MixedBounds is not a multiple of 0.01)", "/cents");
    // an integer field is a multiple of 0.1 whatever it holds
    EXPECT_EQ(yyjson::read<MixedBounds>(R"({"tenths":7})").tenths, 7);
}

TEST(Constraints, FloatBounds)
{
    expect_accepted<FloatBounds>(R"({"tenth":0.1,"under":0.099999994,"big":16777217})",
                                 {0.1F, 0.099999994F, 16777216.0F});
    expect_rejected<FloatBounds>(R"({"tenth":0.10000001})",
                                 R"(the value of the key "tenth" of FloatBounds is greater than the maximum 0.1)",
                                 "/tenth");
    expect_rejected<FloatBounds>(
        R"({"under":0.1})", R"(the value of the key "under" of FloatBounds is not less than the exclusive maximum 0.1)",
        "/under");
}

TEST(Constraints, NaN)
{
    // NaN is in no bounds
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    EXPECT_THROW((void)yyjson::read<Bounded>(R"({"d":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"quarter":Infinity})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<MixedBounds>(R"({"cents":NaN})", flag), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"d":NaN})", flag).cast<Bounded>(), yyjson::bad_cast);
}

TEST(Constraints, Texts)
{
    expect_accepted<Texts>(R"({"name":"ab","code":"XYZ","note":"nb"})", {"ab", "XYZ", "nb"});
    // the length is in code points
    expect_accepted<Texts>(R"({"name":"日本語"})", {"日本語", "ABC", {}});

    expect_rejected<Texts>(R"({"name":"a"})",
                           R"(the value of the key "name" of Texts is shorter than the minimum length 2)", "/name");
    expect_rejected<Texts>(R"({"name":"abcd"})",
                           R"(the value of the key "name" of Texts is longer than the maximum length 3)", "/name");
    expect_rejected<Texts>(R"({"name":"日本語x"})",
                           R"(the value of the key "name" of Texts is longer than the maximum length 3)", "/name");
    expect_rejected<Texts>(R"({"name":"ab","code":"abc"})",
                           R"(the value of the key "code" of Texts does not match the pattern "^[A-Z]{3}$")", "/code");
    expect_rejected<Texts>(R"({"name":"ab","note":"x"})",
                           R"(the value of the key "note" of Texts does not match the pattern "^n")", "/note");
}

TEST(Constraints, Patterns)
{
    expect_accepted<Patterns>(R"({"s":"abz","o":"oxo","v":"123"})", {"abz", "oxo", "123"});
    expect_accepted<Patterns>(R"({"s":"az"})", {"az", {}, {}});

    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Patterns does not match all of the patterns )", key);
    };
    expect_rejected<Patterns>(R"({"s":"ab"})", value("s") + R"("^a", "z$")", "/s");
    expect_rejected<Patterns>(R"({"s":"bz"})", value("s") + R"("^a", "z$")", "/s");
    expect_rejected<Patterns>(R"({"s":"az","o":"oo"})", value("o") + R"("^o", "o$", "x")", "/o");
    expect_rejected<Patterns>(R"({"s":"az","o":"xo"})", value("o") + R"("^o", "o$", "x")", "/o");
    expect_rejected<Patterns>(R"({"s":"az","o":"o"})",
                              R"(the value of the key "o" of Patterns is shorter than the minimum length 2)", "/o");
    expect_rejected<Patterns>(R"({"s":"az","v":"1a"})",
                              R"(the value of the key "v" of Patterns does not match the pattern "^[0-9]*$")", "/v");
}

TEST(Constraints, Satisfies)
{
    expect_accepted<Validated>(R"({"even":2,"text":"ok","nonzero":null})", {2, "ok", {}});
    expect_accepted<Validated>(R"({"nonzero":1})", {0, {}, 1});

    expect_rejected<Validated>(R"({"even":1})",
                               R"(the value of the key "even" of Validated is rejected by yyjson::satisfies)", "/even");
    expect_rejected<Validated>(R"({"text":"bad"})",
                               R"(the value of the key "text" of Validated is rejected by yyjson::satisfies)", "/text");
    expect_rejected<Validated>(R"({"nonzero":0})",
                               R"(the value of the key "nonzero" of Validated is rejected by yyjson::satisfies)",
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

    // a default value that the key leaves is not checked, and a value that the key gives is
    expect_accepted<ConstrainedDefaults>("{}", {});
    expect_accepted<ConstrainedDefaults>(R"({"o":null})", {});
    expect_accepted<ConstrainedDefaults>(R"({"a":0,"o":0})", {0, 0});
    expect_rejected<ConstrainedDefaults>(
        R"({"a":-2})", R"(the value of the key "a" of ConstrainedDefaults is less than the minimum 0)", "/a");
    expect_rejected<ConstrainedDefaults>(
        R"({"o":-2})", R"(the value of the key "o" of ConstrainedDefaults is less than the minimum 0)", "/o");
}

TEST(Constraints, Transparent)
{
    expect_accepted<Percent>("50", {50});
    expect_rejected<Percent>("101", "the value of Percent is greater than the maximum 100", "");
    expect_accepted<Progress>(R"({"done":100,"steps":[0,100]})", {{100}, {{0}, {100}}});
    expect_rejected<Progress>(R"({"done":-1})", "the value of Percent is less than the minimum 0", "/done");
    expect_rejected<Progress>(R"({"steps":[1,200]})", "the value of Percent is greater than the maximum 100",
                              "/steps/1");
}

TEST(Constraints, Merge)
{
    // null resets a field without a check, and a struct merged into a field is checked as a whole
    auto t = Bounded{5, 1, 3, 2};
    yyjson::merge_into(R"({"o":null,"i":null})", t);
    EXPECT_EQ(t, (Bounded{0, 1, 3, {}}));
    EXPECT_THROW(yyjson::merge_into(R"({"u":5})", t), yyjson::bad_cast);

    auto outer = ConstrainedOuter{{}, {"ab", "ABC", {}}};
    EXPECT_EQ(expect_merge_into(outer, R"({"texts":{"note":"n"}})"),
              R"({"items":[],"texts":{"name":"ab","code":"ABC","note":"n"}})");
    EXPECT_THROW(yyjson::merge_into(R"({"texts":{"code":"x"}})", outer), yyjson::bad_cast);

    auto progress = Progress{{5}, {}};
    EXPECT_EQ(expect_merge_into(progress, R"({"done":7})"), R"({"done":7,"steps":[]})");
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"done":101})", progress); });
    EXPECT_EQ(e.path(), "/done");
}

TEST(Constraints, Write)
{
    // a value out of the bounds is written as it is
    EXPECT_EQ(written(Bounded{-1, 0, 1, {}}), R"({"i":-1,"d":0.0,"u":1})");
    EXPECT_EQ(written(Percent{200}), "200");
}

TEST(Constraints, Nullables)
{
    auto expected = Nullables();
    expect_accepted<Nullables>(R"({"code":null,"ratio":null,"name":null})", expected);
    expected.code = "abc";
    expected.ratio = 1.0;
    expected.name = std::make_shared<std::string>("abc");
    *expected.boxed = 3;
    expect_accepted<Nullables>(R"({"code":"abc","ratio":1.0,"name":"abc","boxed":3})", expected);
    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Nullables )", key);
    };
    expect_rejected<Nullables>(R"({"code":"abcd"})", field("code") + "is longer than the maximum length 3", "/code");
    expect_rejected<Nullables>(R"({"ratio":1.5})", field("ratio") + "is greater than the maximum 1", "/ratio");
    expect_rejected<Nullables>(R"({"name":"abcd"})", field("name") + "is rejected by yyjson::satisfies", "/name");
    expect_rejected<Nullables>(R"({"boxed":-1})", field("boxed") + "is less than the minimum 0", "/boxed");

    // std::unique_ptr, in a field and in a std::optional
    const auto owned = yyjson::read<Owned>(R"({"count":null,"level":null})");
    EXPECT_EQ(owned.count, nullptr);
    EXPECT_EQ(written(yyjson::read<Owned>(R"({"count":0,"level":5})")), R"({"count":0,"level":5})");
    const auto expect_owned_rejected = [](std::string_view json, std::string_view message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<Owned>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<Owned>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = Owned();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<Owned>(); }));
    };
    const auto owned_field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Owned )", key);
    };
    expect_owned_rejected(R"({"count":-1})", owned_field("count") + "is less than the minimum 0", "/count");
    expect_owned_rejected(R"({"level":6})", owned_field("level") + "is greater than the maximum 5", "/level");
    expect_owned_rejected(R"({"level":3})", owned_field("level") + "is rejected by yyjson::satisfies", "/level");
}

TEST(Constraints, NullableListed)
{
    // a key that the object leaves out is not checked
    expect_accepted<NullableListed>("{}", {});
    auto expected = NullableListed();
    expected.unit = "mm";
    expected.count = std::make_shared<int>(1);
    expected.note = "x";
    expect_accepted<NullableListed>(R"({"unit":"mm","level":null,"count":1,"note":"x","none":null})", expected);
    // null keeps a std::optional of yyjson::nullable, except in merge_into, where null resets the field
    expected = NullableListed();
    expected.mode = yyjson::nullable<int>();
    EXPECT_EQ(yyjson::read<NullableListed>(R"({"mode":null})"), expected);
    EXPECT_EQ(yyjson::read(R"({"mode":null})").cast<NullableListed>(), expected);
    expected = NullableListed();
    expected.level = 2;
    expect_accepted<NullableListed>(R"({"level":2})", expected);

    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of NullableListed )", key);
    };
    expect_rejected<NullableListed>(R"({"level":3})", field("level") + "is not one of 1, 2, null", "/level");
    expect_rejected<NullableListed>(R"({"mode":1})", field("mode") + "is not equal to the constant null", "/mode");
    expect_rejected<NullableListed>(R"({"none":[]})", field("none") + "is not equal to the constant null", "/none");
    // null that a field holds is checked, as JSON Schema checks null, but merge_into resets the field with null
    expect_rejected<NullableListed>(R"({"unit":null})", field("unit") + R"(is not one of "mm", "cm")", "/unit", false);
    expect_rejected<NullableListed>(R"({"count":null})", field("count") + "is not equal to the constant 1", "/count",
                                    false);
    // null of a std::optional is a missing key
    expect_accepted<NullableListed>(R"({"note":null})", {});
}

TEST(Constraints, StringLiterals)
{
    expect_accepted<Literals>(R"({"unit":"cm","day":"2024-01-31"})", {"cm", "2024-01-31"});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Literals )", key);
    };
    expect_rejected<Literals>(R"({"unit":"km"})", value("unit") + R"(is not one of "mm", "cm")", "/unit");
    expect_rejected<Literals>(R"({"day":"2024-01-32"})", value("day") + R"(does not have the format "date")", "/day");
}

TEST(Constraints, ConstAndEnum)
{
    expect_accepted<Listed>(R"({"unit":"mm","version":2,"ratio":0.5,"flag":true,"big":18446744073709551615,"level":2})",
                            {.unit = "mm", .ratio = 0.5F, .big = 18446744073709551615ULL, .level = 2});
    // a float equals 0.1 when it holds 0.1F, whatever the JSON writes
    expect_accepted<Listed>(R"({"ratio":0.100000001,"level":null})", {});
    const auto value = [](std::string_view key) { return std::format(R"(the value of the key "{}" of Listed )", key); };
    expect_rejected<Listed>(R"({"unit":"km"})", value("unit") + R"(is not one of "mm", "cm", "m")", "/unit");
    expect_rejected<Listed>(R"({"version":3})", value("version") + "is not equal to the constant 2", "/version");
    expect_rejected<Listed>(R"({"ratio":0.3})", value("ratio") + "is not one of 0.1, 0.5", "/ratio");
    expect_rejected<Listed>(R"({"flag":false})", value("flag") + "is not equal to the constant true", "/flag");
    expect_rejected<Listed>(R"({"big":18446744073709551614})",
                            value("big") + "is not one of -1, 0, 18446744073709551615", "/big");
    expect_rejected<Listed>(R"({"level":4})", value("level") + "is not one of 1, 2, 3", "/level");
    // the constant is checked first; keywords that leave no value, as with enum_(2, 3), do not compile
    expect_rejected<Listed>(R"({"odd":3})", value("odd") + "is not equal to the constant 1", "/odd");
    expect_accepted<Listed>(R"({"odd":1})", {});
}

TEST(Constraints, ConstAndEnumNames)
{
    expect_accepted<ListedEnums>(R"({"first":"low","loud":"mid","maybe":"mid"})",
                                 {.loud = Tone::mid, .maybe = Tone::mid});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ListedEnums )", key);
    };
    expect_rejected<ListedEnums>(R"({"first":"mid"})", value("first") + R"(is not equal to the constant "low")",
                                 "/first");
    expect_rejected<ListedEnums>(R"({"loud":"low"})", value("loud") + R"(is not one of "mid", "HIGH")", "/loud");
    expect_rejected<ListedEnums>(R"({"maybe":"HIGH"})", value("maybe") + R"(is not equal to the constant "mid")",
                                 "/maybe");
}

TEST(Constraints, ConstAndEnumTexts)
{
    expect_accepted<ListedTexts>(R"({"at":{"y":1,"x":1},"pair":[1,2],"scale":[2.5,2.5],"mixed":[0]})",
                                 {.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}});
    // a double reads 1 as 1.0, and an integer is a number of the variant, which reads it into its alternative
    expect_accepted<ListedTexts>(R"({"scale":[1.0],"mixed":0})", {.scale = std::vector{1.0}});
    expect_accepted<ListedTexts>("{}", {});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ListedTexts )", key);
    };
    // the members of an object have no order
    expect_accepted<ListedTexts>(R"({"order":{"b":1,"a":2}})", {.order = {{"b", 1}, {"a", 2}}});
    expect_rejected<ListedTexts>(R"({"order":{"a":2}})",
                                 value("order") + R"(is not equal to the constant {"a":2,"b":1})", "/order");
    expect_rejected<ListedTexts>(R"({"at":{"x":0,"y":1}})",
                                 value("at") + R"(is not one of { "y": 0, "x": 0 }, {"x":1,"y":1})", "/at");
    expect_rejected<ListedTexts>(R"({"pair":[2,1]})", value("pair") + "is not equal to the constant [1, 2]", "/pair");
    expect_rejected<ListedTexts>(R"({"scale":[2.5]})", value("scale") + "is not one of [1], [2.5, 2.5]", "/scale");
    // an object is no value of the variant: it reads no constant
    expect_rejected<ListedTexts>(R"({"mixed":1})", value("mixed") + "is not one of 0, [0], {}", "/mixed");

    EXPECT_NO_THROW((void)yyjson::read<ListedDom>(R"({"any":{"b":true,"a":[1.0,2]}})"));
    EXPECT_NO_THROW((void)yyjson::read<ListedDom>(R"({"any":null})"));
    const auto dom = bad_cast_of([] { (void)yyjson::read<ListedDom>(R"({"any":{"a":[1,2],"b":1}})"); });
    EXPECT_EQ(dom.path(), "/any");

    // the text is read when a value is checked first
    try
    {
        (void)yyjson::read<UnreadableText>(R"({"n":1})");
        ADD_FAILURE();
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_TRUE(std::string_view(e.what()).starts_with(
            R"(the JSON text "one" of yyjson::const_ or yyjson::enum_ cannot be read as int: )"))
            << e.what();
    }
}

TEST(Constraints, Format)
{
    expect_accepted<Formatted>(R"({"when":"1985-04-12T23:20:50.52+01:00","mail":"a.b@[IPv6:::1]","code":"AB"})",
                               {"1985-04-12T23:20:50.52+01:00", "a.b@[IPv6:::1]", "AB", std::nullopt});
    expect_accepted<Formatted>(R"({"mail":null})", {});
    // yyjson::format checks nothing
    expect_accepted<Formatted>(R"({"day":"tomorrow"})", {.day = "tomorrow"});
    expect_accepted<Described>(R"({"id":"anything"})", {"anything"});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Formatted )", key);
    };
    expect_rejected<Formatted>(R"({"when":"2023-02-29T00:00:00Z"})",
                               value("when") + R"(does not have the format "date-time")", "/when");
    expect_rejected<Formatted>(R"({"when":"2024-01-01T23:59:60+01:00"})",
                               value("when") + R"(does not have the format "date-time")", "/when");
    expect_rejected<Formatted>(R"({"mail":"a..b@example.com"})", value("mail") + R"(does not have the format "email")",
                               "/mail");
    expect_rejected<Formatted>(R"({"code":"Ab"})", value("code") + R"(does not have the format "upper")", "/code");
    expect_accepted<Formatted>(R"({"re":"([abc])+\\s+$"})", {.re = R"(([abc])+\s+$)"});
    expect_rejected<Formatted>(R"({"re":"^(abc]"})", value("re") + R"(does not have the format "regex")", "/re");
}

TEST(Constraints, FormatRegexOutOfMemory)
{
    constexpr auto json = R"({"re":"<out of memory>"})";
    EXPECT_THROW((void)yyjson::read<Formatted>(json), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(json).cast<Formatted>(), std::bad_alloc);
    // not passed on to the next alternative of a std::variant
    using variant = std::variant<Formatted, yyjson::value>;
    EXPECT_THROW((void)yyjson::read<variant>(json), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(json).cast<variant>(), std::bad_alloc);
}

TEST(Constraints, Formats)
{
    using namespace yyjson::detail;
    const auto check = [](std::string_view format, std::string_view s) {
        const auto index = builtin_format_index(format);
        EXPECT_LT(index, std::size(builtin_formats)) << format;
        return builtin_formats[index].second(s);
    };
    // the cases of the JSON-Schema-Test-Suite that the grammars of the formats decide on boundaries
    EXPECT_TRUE(check("date", "2020-02-29") && !check("date", "2021-02-29") && !check("date", "2000-13-01"));
    EXPECT_TRUE(check("time", "23:59:60Z") && check("time", "15:59:60-08:00") && !check("time", "22:59:60Z"));
    EXPECT_TRUE(!check("time", "08:30:06") && !check("time", "08:30:06.Z") && check("time", "08:30:06.123z"));
    EXPECT_TRUE(check("duration", "P4DT12H30M5S") && check("duration", "P2W") && check("duration", "PT1M"));
    EXPECT_TRUE(!check("duration", "P") && !check("duration", "PT") && !check("duration", "P1YT") &&
                !check("duration", "P2D1Y") && !check("duration", "P1Y2W") && !check("duration", "P1Y2D"));
    EXPECT_TRUE(check("ipv4", "192.168.0.1") && !check("ipv4", "087.10.0.1") && !check("ipv4", "256.0.0.0"));
    EXPECT_TRUE(check("ipv6", "::") && check("ipv6", "::ffff:192.168.0.1") && check("ipv6", "1:2:3:4:5:6:7:8"));
    EXPECT_TRUE(!check("ipv6", "1:2:3:4:5:6:7:8:9") && !check("ipv6", "1::2::3") && !check("ipv6", "12345::") &&
                !check("ipv6", "fe80::a%eth1"));
    EXPECT_TRUE(check("hostname", "a--b.com") && !check("hostname", "ab--c.com") && !check("hostname", "xn--X") &&
                !check("hostname", "example.com."));
    EXPECT_TRUE(check("email", "\"joe bloggs\"@example.com") && check("email", "joe@[127.0.0.1]") &&
                !check("email", "joe@[300.0.0.1]") && !check("email", ".joe@example.com"));
    EXPECT_TRUE(check("uri", "urn:oasis:names:specification:docbook:dtd:xml:4.1.2") &&
                check("uri", "http://[2001:db8::7]/c=GB?objectClass?one") && !check("uri", "//foo.bar/?baz=qux") &&
                !check("uri", "http:// shouldfail.com") && !check("uri", "http://a/%zz"));
    EXPECT_TRUE(check("uri-reference", "#fragment") && check("uri-reference", "../a") &&
                !check("uri-reference", "a:b:\\c") && !check("uri-reference", "#frag\\ment"));
    EXPECT_TRUE(check("uri-template", "http://example.com/dictionary/{term:1}/{term}") && check("uri-template", "") &&
                !check("uri-template", "http://example.com/dictionary/{term:1}/{term"));
    EXPECT_TRUE(check("uuid", "2EB8AA08-AA98-11EA-B4AA-73B441D16380") &&
                !check("uuid", "2eb8aa08-aa98-11ea-b4aa73b441d16380"));
    EXPECT_TRUE(check("json-pointer", "") && check("json-pointer", "/~0~1") && !check("json-pointer", "/~2") &&
                !check("json-pointer", "a"));
    EXPECT_TRUE(check("relative-json-pointer", "0#") && check("relative-json-pointer", "1/a") &&
                !check("relative-json-pointer", "01") && !check("relative-json-pointer", "/a"));
}

TEST(Constraints, Strict)
{
    // yyjson::strict checks the keywords as a read without it does
    const auto rejected = [](auto read, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of(read);
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
        EXPECT_EQ(e.path(), path);
    };
    rejected([] { (void)yyjson::read<Bounded>(R"({"i":11,"d":0.5,"u":3,"o":1})", yyjson::strict); },
             R"(the value of the key "i" of Bounded is greater than the maximum 10)", "/i");
    rejected(
        [] {
            auto t = Texts();
            yyjson::read_into(R"({"name":"a","code":"ABC"})", t, yyjson::strict);
        },
        R"(the value of the key "name" of Texts is shorter than the minimum length 2)", "/name");
    rejected(
        [] {
            auto t = Listed();
            yyjson::merge_into(R"({"version":1})", t, yyjson::strict);
        },
        R"(the value of the key "version" of Listed is not equal to the constant 2)", "/version");
    rejected([] { (void)yyjson::read(R"({"done":-1,"steps":[]})").cast<Progress>(yyjson::strict); },
             "the value of Percent is less than the minimum 0", "/done");
}

// the reader without a document and the cast of a document give the same value or the same error, of which the
// reader without a document leaves an error to the document
TEST(Constraints, Differential)
{
    const auto same = [](auto read, bool accepted, const char* json) {
        const bool direct = read(json);
        if (accepted)
        {
            EXPECT_TRUE(direct) << json;
        }
    };
    const auto bounded = [](const char* json) { return expect_direct_read<Bounded>(json); };
    same(bounded, true, R"({"i":3,"d":0.5,"u":6,"o":2})");
    for (const auto* json : {R"({"i":-1})", R"({"u":7})", R"({"o":-9})", R"({"d":2})", R"({"i":null})"})
        same(bounded, false, json);
    const auto texts = [](const char* json) { return expect_direct_read<Texts>(json); };
    same(texts, true, R"({"name":"ab","code":"QRS"})");
    same(texts, false, R"({"name":"abcd"})");
    same(texts, false, R"({"code":"Q"})");
    const auto listed = [](const char* json) { return expect_direct_read<Listed>(json); };
    same(listed, true, R"({"unit":"cm","version":2})");
    same(listed, false, R"({"unit":"x"})");
    same(listed, false, R"({"ratio":0.2})");
    const auto progress = [](const char* json) { return expect_direct_read<Progress>(json); };
    same(progress, true, R"({"done":3,"steps":[1,2]})");
    same(progress, false, R"({"done":3,"steps":[1,-2]})");
    const auto outer = [](const char* json) {
        const bool read = expect_direct_read<ConstrainedOuter>(json);
        return expect_read_into(ConstrainedOuter{{{1, 1, 3, {}}}, {}}, json) && read;
    };
    same(outer, true, R"({"items":[{"i":1},{"i":2}]})");
    same(outer, false, R"({"items":[{"i":1},{"u":2}]})");
}

#if FIELD_RFL_ANNOTATIONS
TEST(Constraints, Annotations)
{
    expect_accepted<ConstrainedAnnotated>(R"({"n":3,"s":"xy","even":4})", {3, "pq", "xy", 4});
    expect_rejected<ConstrainedAnnotated>(
        R"({"p":"pp"})",
        R"(the value of the key "p" of ConstrainedAnnotated does not match all of the patterns "^p", "q$")", "/p");
    expect_rejected<ConstrainedAnnotated>(
        R"({"n":4})", R"(the value of the key "n" of ConstrainedAnnotated is greater than the maximum 3)", "/n");
    expect_rejected<ConstrainedAnnotated>(
        R"({"s":"y"})", R"(the value of the key "s" of ConstrainedAnnotated does not match the pattern "^x")", "/s");
    expect_rejected<ConstrainedAnnotated>(
        R"({"even":3})", R"(the value of the key "even" of ConstrainedAnnotated is rejected by yyjson::satisfies)",
        "/even");

    expect_accepted<KeywordsAnnotated>(R"({"version":2,"unit":"b","day":"2024-02-29","note":"y","step":1.5})",
                                       {2, "b", "2024-02-29", "y", 1.5, {}});
    expect_accepted<KeywordsAnnotated>(R"({"list":[1,2]})", {.list = {1, 2}});
    const auto keywords = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of KeywordsAnnotated )", key);
    };
    expect_rejected<KeywordsAnnotated>(R"({"version":1})", keywords("version") + "is not equal to the constant 2",
                                       "/version");
    expect_rejected<KeywordsAnnotated>(R"({"unit":"c"})", keywords("unit") + R"(is not one of "a", "b")", "/unit");
    expect_rejected<KeywordsAnnotated>(R"({"day":"2024-02-30"})",
                                       keywords("day") + R"(does not have the format "date")", "/day");
    expect_rejected<KeywordsAnnotated>(R"({"step":0.7})", keywords("step") + "is not a multiple of 0.5", "/step");
    expect_rejected<KeywordsAnnotated>(R"({"step":2})", keywords("step") + "is not less than the exclusive maximum 2",
                                       "/step");
    expect_rejected<KeywordsAnnotated>(R"({"list":[2,1]})", keywords("list") + "is not one of [1,2], []", "/list");
}
#endif

// NOLINTEND
