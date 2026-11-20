#include <algorithm>
#include <condition_variable>
#include <functional>
#include <memory_resource>
#include <mutex>
#include <regex>
#include <thread>
#include <version>
#ifdef __cpp_lib_flat_map
#include <flat_map>
#endif
#include "test_common.hpp"
#include "test_pattern_matcher.hpp"
#include "test_validation.hpp"

// NOLINTBEGIN

// The keywords of the values of the fields: the bounds of a number, the lengths and the patterns of a string,
// `yyjson::format_assertion`, `yyjson::const_`, `yyjson::enum_` and `yyjson::satisfies`

// a format that the program checks, and one that it checks with a validate of its own
bool r14_valid_text(std::string_view s);
bool r15_nested_checks(std::string_view s);
bool r16_nested_reads(std::string_view s);
template <>
struct yyjson::format_checker<>
{
    std::string_view name;
    explicit format_checker(std::string_view name) : name(name) {}
    bool operator()(std::string_view s) const
    {
        if (name == "valid-text") return r14_valid_text(s);
        if (name == "nested-checks") return r15_nested_checks(s);
        if (name == "nested-reads") return r16_nested_reads(s);
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

// bounds of a float field, which compares the JSON number that the field is read from
struct FloatBounds
{
    float tenth = 0;
    float under = 0;
    float big = 16777216.0F;
    float step = 0;
    bool operator==(const FloatBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatBounds> = yyjson::annotate(
    yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("under", yyjson::exclusive_maximum(0.1)),
    yyjson::field("big", yyjson::minimum(16777217)), yyjson::field("step", yyjson::multiple_of(0.1)));

// floats in elements, in values of a map and in an alternative of a variant
struct FloatElements
{
    std::vector<float> v;
    std::map<std::string, float> m;
    std::variant<float, std::string> u = 0.0F;
    bool operator==(const FloatElements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatElements> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::maximum(0.1))),
    yyjson::field("m", yyjson::additional_properties(yyjson::maximum(0.1))), yyjson::field("u", yyjson::maximum(0.1)));

// bounds of a double field, which compares the JSON number that the field is read from
struct RealBounds
{
    double top = 0;
    double tenth = 0;
    double cent = 0;
    double half = 0;
    double third = 0;
    double three = 0;
    long double wide = 0;
    bool operator==(const RealBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RealBounds> = yyjson::annotate(
    yyjson::field("top", yyjson::maximum(9007199254740992LL)), yyjson::field("tenth", yyjson::multiple_of(0.1)),
    yyjson::field("cent", yyjson::multiple_of(0.01)), yyjson::field("half", yyjson::multiple_of(0.5)),
    yyjson::field("third", yyjson::multiple_of(1.5)), yyjson::field("three", yyjson::multiple_of(3)),
    yyjson::field("wide", yyjson::exclusive_minimum(-9007199254740992LL)));

// doubles in elements and in an alternative of a variant
struct RealElements
{
    std::vector<double> v;
    std::variant<double, std::string> u = 0.0;
    bool operator==(const RealElements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RealElements> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::maximum(9007199254740992LL))),
                     yyjson::field("u", yyjson::multiple_of(0.1)));

// divisors that no fixed number of digits holds: integers up to 2^64, and reals by their shortest decimal
struct WideDivisors
{
    double big = 0;
    std::uint64_t ubig = 0;
    double tiny = 0;
    double odd = 0;
    std::int64_t huge = 0;
    bool operator==(const WideDivisors&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WideDivisors> = yyjson::annotate(
    yyjson::field("big", yyjson::multiple_of(9007199254740992ULL)),
    yyjson::field("ubig", yyjson::multiple_of(9007199254740992ULL)), yyjson::field("tiny", yyjson::multiple_of(1e-18)),
    yyjson::field("odd", yyjson::multiple_of(900719925474098.8)), yyjson::field("huge", yyjson::multiple_of(1e20)));

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

// numbers that a document keeps as raw text with ReadFlag::NumberAsRaw
struct RawNumbers
{
    yyjson::value n;
    yyjson::value big;
    yyjson::value half;
    yyjson::value flag;
};
template <>
inline constexpr auto yyjson::annotations<RawNumbers> =
    yyjson::annotate(yyjson::field("n", yyjson::minimum(0), yyjson::maximum(100)),
                     yyjson::field("big", yyjson::minimum(9007199254740993LL)),
                     yyjson::field("half", yyjson::multiple_of(0.5)), yyjson::field("flag", yyjson::matches<bool>));

// raw numbers in the extended syntax of JSON5
struct RawExtended
{
    yyjson::value n;
    yyjson::value c;
    std::vector<yyjson::value> u = {};
    yyjson::value a;
};
template <>
inline constexpr auto yyjson::annotations<RawExtended> =
    yyjson::annotate(yyjson::field("n", yyjson::maximum(5)), yyjson::field("c", yyjson::const_(16)),
                     yyjson::field("u", yyjson::unique_items), yyjson::field("a", yyjson::unique_items));

// equality keywords on values that a document may keep as raw numbers
struct RawHolder
{
    yyjson::value v;
    bool operator==(const RawHolder&) const = default;
};
struct RawEquals
{
    yyjson::value one;
    yyjson::value listed;
    yyjson::value deep;
    RawHolder held;
    yyjson::value unique;
    yyjson::array items;
};
template <>
inline constexpr auto yyjson::annotations<RawEquals> =
    yyjson::annotate(yyjson::field("one", yyjson::const_(1)), yyjson::field("listed", yyjson::enum_("a", 2.5)),
                     yyjson::field("deep", yyjson::const_(yyjson::json_text(R"({"a":[1,2.5]})"))),
                     yyjson::field("held", yyjson::const_(yyjson::json_text(R"({"v":3})"))),
                     yyjson::field("unique", yyjson::unique_items), yyjson::field("items", yyjson::unique_items));

// a constant with a key that Spot does not have, which yyjson::strict alone accepts
struct WideText
{
    Spot at{};
};
template <>
inline constexpr auto yyjson::annotations<WideText> =
    yyjson::annotate(yyjson::field("at", yyjson::const_(yyjson::json_text(R"({"x":1,"y":2,"z":3})"))));

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

// The keywords of arrays and objects: the elements of ranges, the values and the keys of maps, the positions of
// tuples and the elements that an array contains
bool is_small_pair(const std::pair<const int, int>& p) { return p.first < p.second; }
struct Elements
{
    std::vector<int> scores = {};
    std::array<int, 2> pair = {};
    std::list<std::string> names = {};
    std::set<int> set = {};
    std::unordered_set<int> uset = {};
    std::map<std::string, int> limits = {};
    std::unordered_map<std::string, std::vector<int>> groups = {};
    std::map<int, int> by_id = {};  // read from an array of [key, value] pairs
    std::map<int, int> ordered = {};
    std::vector<std::optional<int>> holes = {};
    std::optional<std::vector<std::string>> tags = {};
    std::vector<std::vector<std::string>> nested = {};
    bool operator==(const Elements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Elements> = yyjson::annotate(
    yyjson::field("scores", yyjson::items(yyjson::minimum(0))),
    yyjson::field("pair", yyjson::items(yyjson::maximum(9))),
    yyjson::field("names", yyjson::items(yyjson::pattern("^[a-z]+$"))),
    yyjson::field("set", yyjson::items(yyjson::maximum(5))),
    yyjson::field("uset", yyjson::items(yyjson::satisfies<is_even>)),
    yyjson::field("limits", yyjson::max_properties(3), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field("groups", yyjson::additional_properties(yyjson::min_items(1), yyjson::items(yyjson::minimum(1)))),
    yyjson::field("by_id", yyjson::items(yyjson::prefix_items({}, yyjson::maximum(5)))),
    yyjson::field("ordered", yyjson::items(yyjson::satisfies<is_small_pair>)),
    yyjson::field("holes", yyjson::items(yyjson::maximum(5))),
    yyjson::field("tags", yyjson::items(yyjson::max_length(3))),
    yyjson::field("nested", yyjson::items(yyjson::min_items(1), yyjson::items(yyjson::max_length(3)))));

struct ElementsVisitable
{
    std::vector<int> v = {};
    bool operator==(const ElementsVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ElementsVisitable> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::exclusive_minimum(0))));
VISITABLE_STRUCT(ElementsVisitable, v);

// the ranges that the holders hold, and the holders that are elements
struct ElementNullables
{
    std::shared_ptr<std::vector<int>> list = {};
    std::vector<std::shared_ptr<int>> ids = {};
    std::vector<yyjson::nullable<int>> steps = {};
    yyjson::nullable<std::vector<int>> counts = {};
    bool operator==(const ElementNullables& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<ElementNullables> = yyjson::annotate(
    yyjson::field("list", yyjson::min_items(1), yyjson::items(yyjson::maximum(9))),
    yyjson::field("ids", yyjson::unique_items), yyjson::field("steps", yyjson::items(yyjson::enum_(1, 2))),
    yyjson::field("counts", yyjson::max_items(1)));
struct OwnedElements
{
    std::vector<std::unique_ptr<std::string>> tags = {};
};
template <>
inline constexpr auto yyjson::annotations<OwnedElements> =
    yyjson::annotate(yyjson::field("tags", yyjson::items(yyjson::max_length(3)), yyjson::unique_items));

struct Unordered
{
    int v = 0;
    bool operator==(const Unordered&) const = default;
};
struct Unique
{
    std::vector<int> ints = {};
    std::vector<std::string> strs = {};
    std::vector<double> reals = {};
    std::vector<Unordered> plain = {};
    std::vector<std::optional<int>> opts = {};
    std::vector<std::vector<int>> nested = {};
    bool operator==(const Unique&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Unique> = yyjson::annotate(
    yyjson::field("ints", yyjson::unique_items), yyjson::field("strs", yyjson::unique_items),
    yyjson::field("reals", yyjson::unique_items), yyjson::field("plain", yyjson::unique_items),
    yyjson::field("opts", yyjson::unique_items), yyjson::field("nested", yyjson::items(yyjson::unique_items)));

struct UniqueJson
{
    std::vector<yyjson::value> any = {};
};
template <>
inline constexpr auto yyjson::annotations<UniqueJson> = yyjson::annotate(yyjson::field("any", yyjson::unique_items));

// elements that are compared by the values that they are written with, and not by `operator==`
struct UniqueBox
{
    std::shared_ptr<int> p = {};
};
struct UniqueLoose
{
    int v = 0;
    bool operator==(const UniqueLoose&) const { return true; }
};
struct UniqueTree
{
    int v = 0;
    std::vector<UniqueTree> kids = {};
};
struct UniqueParts
{
    std::vector<UniqueBox> boxes = {};
    std::vector<UniqueLoose> loose = {};
    std::vector<UniqueTree> trees = {};
    std::vector<std::unordered_map<std::string, int>> maps = {};
    std::vector<std::vector<std::pair<std::string, int>>> pairs = {};
    std::vector<std::tuple<int, std::string>> tuples = {};
    std::pair<int, int> twins = {};
};
template <>
inline constexpr auto yyjson::annotations<UniqueParts> =
    yyjson::annotate(yyjson::field("boxes", yyjson::unique_items), yyjson::field("loose", yyjson::unique_items),
                     yyjson::field("trees", yyjson::unique_items), yyjson::field("maps", yyjson::unique_items),
                     yyjson::field("pairs", yyjson::unique_items), yyjson::field("tuples", yyjson::unique_items),
                     yyjson::field("twins", yyjson::unique_items));
// keywords that compare JSON values, so that each struct below reads through a document
enum class UjColor
{
    red,
    green
};
template <>
inline constexpr auto yyjson::annotations<UjColor> = yyjson::annotate(yyjson::enum_format::name_or_integer);
struct UniqueJsonEquality
{
    std::vector<Spot> spots = {};
    std::vector<std::set<int>> sets = {};
    std::vector<std::variant<int, double>> mixed = {};
    std::vector<std::map<std::string, int>> maps = {};
    std::vector<std::vector<std::pair<std::string, int>>> pairs = {};
    std::vector<std::vector<std::pair<std::string, std::string>>> kv = {};
    std::vector<UjColor> colors = {};
    std::tuple<Spot, Spot> twins = {Spot{0, 0}, Spot{1, 0}};
    std::vector<std::vector<Spot>> nested = {};
    bool operator==(const UniqueJsonEquality&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueJsonEquality> =
    yyjson::annotate(yyjson::field("spots", yyjson::unique_items), yyjson::field("sets", yyjson::unique_items),
                     yyjson::field("mixed", yyjson::unique_items), yyjson::field("maps", yyjson::unique_items),
                     yyjson::field("pairs", yyjson::unique_items), yyjson::field("kv", yyjson::unique_items),
                     yyjson::field("colors", yyjson::unique_items), yyjson::field("twins", yyjson::unique_items),
                     yyjson::field("nested", yyjson::items(yyjson::unique_items)));
struct UniqueReals2
{
    std::vector<double> v = {};
    std::set<double> s = {};
    std::vector<double> n = {0.0, 0.0};
    bool operator==(const UniqueReals2&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueReals2> =
    yyjson::annotate(yyjson::field("v", yyjson::unique_items), yyjson::field("s", yyjson::unique_items),
                     yyjson::field("n", yyjson::not_(yyjson::unique_items)));
inline constexpr auto spot_10 = yyjson::json_text(R"({"x":1,"y":0})");
struct ListedJsonEquality
{
    Spot at{1, 0};
    Spot not_at{};
    Spot any_at{1, 0};
    bool operator==(const ListedJsonEquality&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedJsonEquality> = yyjson::annotate(
    yyjson::field("at", yyjson::const_(spot_10)), yyjson::field("not_at", yyjson::not_(yyjson::const_(spot_10))),
    yyjson::field("any_at",
                  yyjson::any_of(yyjson::const_(spot_10), yyjson::const_(yyjson::json_text(R"({"x":2,"y":0})")))));
// a constant that the type of the field cannot read
struct BadLiteral
{
    Spot at{};
    bool operator==(const BadLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<BadLiteral> =
    yyjson::annotate(yyjson::field("at", yyjson::const_(yyjson::json_text(R"({"x":"a"})"))));
// a constant that no alternative of the variant reads
struct VariantLiteral
{
    std::variant<int, std::string> v{};
    bool operator==(const VariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VariantLiteral> =
    yyjson::annotate(yyjson::field("v", yyjson::const_(yyjson::json_text(R"({"x":1})"))));
// the keyword of the values of the key map is the only one that compares JSON values
struct KeyMapOnly
{
    std::map<std::string, std::vector<Spot>> extra = {};
    bool operator==(const KeyMapOnly&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyMapOnly> =
    yyjson::annotate(yyjson::field("extra", yyjson::extra_keys, yyjson::additional_properties(yyjson::unique_items)));
// keywords that compare C++ values equal to their JSON values, so that the struct reads directly
enum class FastColor
{
    a,
    b
};
struct FastUnique
{
    std::vector<int> ints = {};
    std::vector<std::string> strs = {};
    std::vector<std::vector<int>> rows = {};
    std::tuple<int, int> two = {0, 1};
    std::vector<std::optional<int>> opts = {};
    std::vector<FastColor> named = {};
    std::set<int> si = {};
    bool operator==(const FastUnique&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FastUnique> =
    yyjson::annotate(yyjson::field("ints", yyjson::unique_items), yyjson::field("strs", yyjson::unique_items),
                     yyjson::field("rows", yyjson::unique_items), yyjson::field("two", yyjson::unique_items),
                     yyjson::field("opts", yyjson::unique_items), yyjson::field("named", yyjson::unique_items),
                     yyjson::field("si", yyjson::unique_items));
struct ListedNumbersExact
{
    double d = 9007199254740992.0;
    double e = 0;
    float f = 0.1F;
    double n = 0;
    float g = 16777216.0F;
    bool operator==(const ListedNumbersExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedNumbersExact> = yyjson::annotate(
    yyjson::field("d", yyjson::const_(9007199254740992LL)),
    yyjson::field("e", yyjson::enum_(0, 18446744073709551615ULL)), yyjson::field("f", yyjson::const_(0.1)),
    yyjson::field("n", yyjson::not_(yyjson::const_(9007199254740992LL))),
    yyjson::field("g", yyjson::enum_(16777216, 16777217), yyjson::maximum(16777216)));
// a merge that changes the member "d" to a number that its double rounds
struct MergedExact
{
    double d = 9007199254740992.0;
    int keep = 1;
    bool operator==(const MergedExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedExact> =
    yyjson::annotate(yyjson::field("d", yyjson::const_(9007199254740992LL)), yyjson::field("keep", yyjson::const_(1)));

// the review of the JSON equality
constexpr auto big_integer = 9007199254740992LL;
struct ArrayExact
{
    double d = 0;
    bool operator==(const ArrayExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ArrayExact> =
    yyjson::annotate(yyjson::as_array, yyjson::field("d", yyjson::const_(big_integer)));
struct ArrayExactRest
{
    int a = 0;
    std::vector<double> rest = {};
    bool operator==(const ArrayExactRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ArrayExactRest> = yyjson::annotate(
    yyjson::as_array, yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::const_(big_integer))));
struct ExactSource
{
    double d = 0;
    bool operator==(const ExactSource&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExactSource> =
    yyjson::annotate(yyjson::field("d", yyjson::const_(big_integer)));
struct ExactWrap
{
    ExactSource s = {};
    bool operator==(const ExactWrap&) const = default;
};
template <>
struct yyjson::caster<ExactWrap>
{
    static ExactWrap from_json(ExactSource s) { return {s}; }
    static ExactSource to_json(const ExactWrap& w) { return w.s; }
};
using RoundedOneOf =
    yyjson::one_of_variant<yyjson::alt<double, yyjson::enum_(0, big_integer + 1), yyjson::maximum(big_integer)>,
                           yyjson::alt<long long, yyjson::enum_(1, big_integer + 1)>>;
using RealSetOneOf =
    yyjson::one_of_variant<yyjson::alt<std::set<double>, yyjson::unique_items>,
                           yyjson::alt<std::vector<double>, yyjson::min_items(2), yyjson::max_items(2)>>;
struct MergedVariantLiteral
{
    std::variant<Spot, bool> p = false;
    bool operator==(const MergedVariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedVariantLiteral> =
    yyjson::annotate(yyjson::not_(yyjson::property("p", yyjson::const_(yyjson::json_text("{}")))));
// casters that read two unequal JSON values as one key, and two equal ones as two keys
enum class OneKey
{
    only
};
template <>
struct yyjson::caster<OneKey>
{
    static OneKey from_json(std::string_view) { return OneKey::only; }
    static std::string to_json(OneKey) { return "only"; }
};
using OneKeyOneOf =
    yyjson::one_of_variant<yyjson::alt<std::set<OneKey>, yyjson::unique_items>,
                           yyjson::alt<std::vector<std::string>, yyjson::min_items(2), yyjson::max_items(2)>>;
#if YYJSON_VERSION_HEX >= 0x000C00
enum class KindKey
{
    integer,
    real
};
template <>
struct yyjson::caster<KindKey>
{
    static KindKey from_json(yyjson::json_view v) { return v.is_real() ? KindKey::real : KindKey::integer; }
    static double to_json(KindKey k) { return k == KindKey::real ? 1.5 : 1; }
};
using KindKeyOneOf = yyjson::one_of_variant<std::set<KindKey>, std::vector<int>>;
#endif
struct NestedVariantInner
{
    std::variant<Spot, bool> p = false;
    bool operator==(const NestedVariantInner&) const = default;
};
struct NestedVariantLiteral
{
    NestedVariantInner c = {};
    bool operator==(const NestedVariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NestedVariantLiteral> = yyjson::annotate(
    yyjson::not_(yyjson::property("c", yyjson::property("p", yyjson::const_(yyjson::json_text(R"({"x":1,"y":0})"))))),
    yyjson::not_(yyjson::property("c", yyjson::property("p", yyjson::const_(yyjson::json_text("{}"))))));
struct UniqueIntPairs
{
    std::vector<std::pair<int, int>> v = {};
    bool operator==(const UniqueIntPairs&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueIntPairs> = yyjson::annotate(yyjson::field("v", yyjson::unique_items));

bool is_lower(const std::string& s)
{
    return std::ranges::all_of(s, [](char c) { return 'a' <= c && c <= 'z'; });
}
struct Prefixed
{
    std::tuple<int, std::string, double> row = {};
    std::pair<int, int> range = {};
    std::vector<int> head = {};
    std::array<int, 3> arr = {};
    std::vector<std::vector<int>> grid = {};
    std::tuple<int, int, int> rest = {};
    bool operator==(const Prefixed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Prefixed> = yyjson::annotate(
    yyjson::field("row",
                  yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(2), yyjson::satisfies<is_lower>}, {})),
    yyjson::field("range", yyjson::prefix_items(yyjson::maximum(5), yyjson::minimum(5))),
    yyjson::field("head", yyjson::prefix_items({yyjson::minimum(0)}, {}), yyjson::items(yyjson::maximum(9))),
    yyjson::field("arr", yyjson::prefix_items({}, yyjson::minimum(1))),
    yyjson::field("grid", yyjson::prefix_items(yyjson::items(yyjson::minimum(1)), {yyjson::min_items(2)})),
    yyjson::field("rest", yyjson::prefix_items(yyjson::minimum(1)), yyjson::items(yyjson::maximum(1))));

// the string keywords written as string literals in the positions of prefix_items, which hold their characters
// before C++26, and a position that is made of them as temporaries
struct ElementLiterals
{
    std::tuple<std::string, std::vector<std::string>> pair = {};
    std::vector<std::string> days = {};
    bool operator==(const ElementLiterals&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ElementLiterals> = yyjson::annotate(
    yyjson::field("pair", yyjson::prefix_items({yyjson::pattern("^[a-z]+$"), yyjson::enum_("ab", "Ab", "abc")},
                                               yyjson::prefix_items(yyjson::const_("yz")))),
    yyjson::field("days", yyjson::items(yyjson::format_assertion("date"))));

bool is_short(const std::string& s) { return s.size() < 3; }
// maps in holders and maps of maps, whose keywords merge_into checks on the members of the patch
using Limits = std::map<std::string, int>;
struct HeldLimits
{
    std::shared_ptr<Limits> pointer = {};
    yyjson::nullable<Limits> held = {};
    yyjson::indirect<Limits> boxed;
    std::map<std::string, Limits> nested = {};
    bool operator==(const HeldLimits&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<HeldLimits> = yyjson::annotate(
    yyjson::field("pointer", yyjson::max_properties(1), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field("held", yyjson::max_properties(1), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field("boxed", yyjson::max_properties(1), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field(
        "nested", yyjson::max_properties(1),
        yyjson::additional_properties(yyjson::max_properties(1), yyjson::additional_properties(yyjson::maximum(100)))));

// maps of sets, whose values a patch with a repeated key merges and erases
struct RepeatedSets
{
    std::map<std::string, std::set<int>> m = {};
    std::map<std::string, int> n = {};
    bool operator==(const RepeatedSets&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RepeatedSets> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::max_items(1))),
                     yyjson::field("n", yyjson::max_properties(2), yyjson::additional_properties(yyjson::maximum(10))));

struct Keyed
{
    std::map<std::string, int> m = {};
    std::unordered_map<std::string, int> u = {};
    std::map<std::string, std::map<std::string, int>> deep = {};
    std::map<std::string, int> sized = {};
    bool operator==(const Keyed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Keyed> = yyjson::annotate(
    yyjson::field("m", yyjson::property_names(yyjson::max_length(3), yyjson::pattern("^[a-z]+$"))),
    yyjson::field("u", yyjson::property_names(yyjson::satisfies<is_short>)),
    yyjson::field("deep", yyjson::additional_properties(yyjson::property_names(yyjson::min_length(2)))),
    yyjson::field("sized", yyjson::min_properties(1), yyjson::property_names(yyjson::enum_("a", "b"))));
// a struct whose keys follow a pattern and a length in code points
struct KeyedStruct
{
    int ab = 0;
    std::optional<int> AB;
    bool operator==(const KeyedStruct&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedStruct> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(3), yyjson::pattern("^[^A-Z/]+$")));

struct Containing
{
    std::vector<std::string> roles = {};
    std::vector<int> v = {};
    std::vector<int> none = {};
    std::vector<std::vector<int>> rows = {};
    std::tuple<int, int> pair = {};
    bool operator==(const Containing&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Containing> = yyjson::annotate(
    yyjson::field("roles", yyjson::contains(yyjson::pattern("^admin$"))),
    yyjson::field("v", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2), yyjson::max_contains(3)),
    yyjson::field("none", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0), yyjson::max_contains(1)),
    yyjson::field("rows", yyjson::contains(yyjson::contains(yyjson::minimum(5)))),
    yyjson::field("pair", yyjson::contains(yyjson::const_(2))));

// the count keywords count the JSON value, also the elements that a set drops and the members that a map drops
struct InputCounted
{
    std::set<int> s = {};
    std::map<std::string, int> m = {};
    std::vector<int> v = {};
    std::unordered_set<int> c = {};
    bool operator==(const InputCounted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<InputCounted> =
    yyjson::annotate(yyjson::field("s", yyjson::min_items(2), yyjson::max_items(2)),
                     yyjson::field("m", yyjson::min_properties(2), yyjson::max_properties(2)),
                     yyjson::field("v", yyjson::min_items(2), yyjson::max_items(2)),
                     yyjson::field("c", yyjson::contains(yyjson::minimum(5)), yyjson::min_contains(2)));
// a condition of a struct on the count of the elements of a set
struct CountGate
{
    std::set<int> s = {};
    int k = 0;
    bool operator==(const CountGate&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountGate> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("s", yyjson::min_items(2))), yyjson::then(yyjson::required("k"))));

// the counts of yyjson::contains that are 0, null elements and a std::optional range
struct Counted
{
    std::vector<int> min0 = {};
    std::vector<int> max0 = {};
    std::vector<int> both0 = {};
    std::vector<std::optional<int>> opts = {};
    std::optional<std::vector<int>> unique = {};
    std::optional<std::vector<int>> head = {};
    std::optional<std::vector<int>> some = {};
    bool operator==(const Counted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Counted> = yyjson::annotate(
    yyjson::field("min0", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0)),
    yyjson::field("max0", yyjson::not_(yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(0))),
    yyjson::field("both0", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0), yyjson::max_contains(0)),
    yyjson::field("opts", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(1)),
    yyjson::field("unique", yyjson::unique_items), yyjson::field("head", yyjson::prefix_items(yyjson::minimum(0))),
    yyjson::field("some", yyjson::contains(yyjson::minimum(10))));

// the subschemas of yyjson::any_of, one_of, not_ and if_then_else that no value satisfies, also by the bounds of
// the type
struct DeadBranches
{
    unsigned char byte = 0;
    int any = 0;
    std::string text = {};
    std::vector<int> one = {};
    std::optional<unsigned char> low = {};
    bool operator==(const DeadBranches&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<DeadBranches> = yyjson::annotate(
    yyjson::field("byte", yyjson::not_(yyjson::minimum(256))),
    yyjson::field("any", yyjson::any_of(yyjson::all_of(yyjson::minimum(5), yyjson::maximum(1)), yyjson::maximum(3))),
    yyjson::field("text",
                  yyjson::if_then_else(yyjson::if_(yyjson::min_length(3), yyjson::max_length(2)),
                                       yyjson::then(yyjson::const_("a")), yyjson::else_(yyjson::max_length(2)))),
    yyjson::field("one",
                  yyjson::one_of(yyjson::min_items(2), yyjson::all_of(yyjson::min_items(3), yyjson::max_items(1)))),
    yyjson::field("low", yyjson::any_of(yyjson::maximum(-1), yyjson::minimum(10))));

// elements of a user caster, keys that are enumerations, ranges of pairs and sequences that are not vectors
struct CastPoint
{
    int x = 0;
    int y = 0;
    bool operator==(const CastPoint&) const = default;
};
template <>
struct yyjson::caster<CastPoint>
{
    static CastPoint from_json(std::array<int, 2> a) { return {a[0], a[1]}; }
    template <typename... Ts>
    static std::array<int, 2> to_json(const CastPoint& p, Ts...)
    {
        return {p.x, p.y};
    }
};
enum class Side
{
    left,
    right
};
struct Unusual
{
    std::vector<CastPoint> pts = {};
    std::map<Side, int> sides = {};
    std::vector<std::pair<std::string, int>> kv = {};
    std::deque<int> dq = {};
    std::list<int> ls = {};
    std::vector<double> reals = {};
    std::multimap<int, int> pm = {};
    bool operator==(const Unusual&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Unusual> = yyjson::annotate(
    yyjson::field("pts", yyjson::unique_items), yyjson::field("sides", yyjson::property_names(yyjson::const_("left"))),
    yyjson::field("kv", yyjson::property_names(yyjson::max_length(1))),
    yyjson::field("dq", yyjson::prefix_items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(9))),
    yyjson::field("ls", yyjson::prefix_items({}, yyjson::minimum(1)), yyjson::items(yyjson::maximum(9))),
    yyjson::field("reals", yyjson::unique_items), yyjson::field("pm", yyjson::unique_items, yyjson::max_items(2)));

// the keywords of the values in the values in the values
struct Deep
{
    std::vector<std::vector<std::vector<int>>> v = {};
    std::map<std::string, std::vector<int>> m = {};
    std::vector<std::map<std::string, int>> k = {};
    bool operator==(const Deep&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Deep> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::prefix_items(yyjson::items(yyjson::minimum(0)),
                                                                           {yyjson::contains(yyjson::minimum(1))}))),
                     yyjson::field("m", yyjson::additional_properties(yyjson::contains(yyjson::minimum(1)))),
                     yyjson::field("k", yyjson::items(yyjson::property_names(yyjson::max_length(1)))));

struct Visited
{
    std::vector<int> u = {};
    std::tuple<int, int> p = {};
    std::vector<int> c = {};
    std::map<std::string, int> k = {};
    bool operator==(const Visited&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Visited> = yyjson::annotate(
    yyjson::field("u", yyjson::unique_items), yyjson::field("p", yyjson::prefix_items(yyjson::minimum(0))),
    yyjson::field("c", yyjson::contains(yyjson::minimum(5))),
    yyjson::field("k", yyjson::property_names(yyjson::max_length(1))));
VISITABLE_STRUCT(Visited, u, p, c, k);

// the subschemas of yyjson::not_ at each position that takes keywords
struct Negated
{
    int n = 0;
    std::string s = {};
    std::vector<int> v = {};
    int twice = 0;
    std::map<std::string, int> m = {};
    std::tuple<int, std::string> row = {};
    std::vector<int> c = {};
    std::vector<std::optional<int>> holes = {};
    std::optional<int> o = {};
    std::vector<std::vector<int>> nested = {};
    bool operator==(const Negated&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Negated> = yyjson::annotate(
    yyjson::field("n", yyjson::not_(yyjson::minimum(10), yyjson::maximum(19))),
    yyjson::field("s", yyjson::not_(yyjson::pattern("^x"))),
    yyjson::field("v", yyjson::items(yyjson::not_(yyjson::not_(yyjson::minimum(0))))),
    yyjson::field("twice", yyjson::not_(yyjson::const_(1)), yyjson::not_(yyjson::const_(2))),
    yyjson::field("m", yyjson::additional_properties(yyjson::not_(yyjson::const_(0))),
                  yyjson::property_names(yyjson::not_(yyjson::pattern("^_")))),
    yyjson::field("row", yyjson::prefix_items(yyjson::not_(yyjson::const_(0)), {yyjson::not_(yyjson::const_("x"))})),
    yyjson::field("c", yyjson::contains(yyjson::not_(yyjson::minimum(0)))),
    yyjson::field("holes", yyjson::items(yyjson::not_(yyjson::minimum(5)))),
    yyjson::field("o", yyjson::not_(yyjson::minimum(5))),
    yyjson::field("nested", yyjson::not_(yyjson::items(yyjson::min_items(1)))));

// yyjson::if_then_else with both branches and with one
struct Conditional
{
    int n = 0;
    std::vector<int> v = {};
    std::vector<int> e = {};
    std::string s = {};
    std::map<std::string, std::string> m = {};
    bool operator==(const Conditional&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Conditional> = yyjson::annotate(
    yyjson::field("n", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                            yyjson::else_(yyjson::maximum(5)))),
    yyjson::field(
        "v", yyjson::if_then_else(yyjson::if_(yyjson::min_items(2)), yyjson::then(yyjson::items(yyjson::maximum(9))))),
    yyjson::field(
        "e", yyjson::items(yyjson::if_then_else(yyjson::if_(yyjson::const_(0)), yyjson::else_(yyjson::minimum(5))))),
    yyjson::field("s", yyjson::if_then_else(yyjson::if_(yyjson::pattern("^#")), yyjson::then(yyjson::min_length(4)))),
    yyjson::field("m", yyjson::additional_properties(yyjson::if_then_else(yyjson::if_(yyjson::const_("x")),
                                                                          yyjson::else_(yyjson::enum_("y", "z"))))));

// yyjson::all_of, any_of and one_of, whose subschemas are braced lists or settings alone
struct Composed
{
    int a = 0;
    int b = 0;
    int c = 0;
    std::vector<int> v = {};
    std::string s = {};
    std::map<std::string, int> m = {};
    int deep = 0;
    std::vector<std::vector<int>> rows = {};
    bool operator==(const Composed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Composed> = yyjson::annotate(
    yyjson::field("a", yyjson::all_of(yyjson::minimum(0), {yyjson::multiple_of(2), yyjson::maximum(10)})),
    yyjson::field("b", yyjson::any_of(yyjson::maximum(0), {yyjson::minimum(10), yyjson::maximum(20)})),
    yyjson::field("c", yyjson::one_of(yyjson::multiple_of(2), yyjson::multiple_of(3))),
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::const_(0), yyjson::minimum(10)))),
    yyjson::field("s", yyjson::any_of(yyjson::const_("a"), {yyjson::enum_("b", "c")}, {yyjson::pattern("^d")})),
    yyjson::field("m", yyjson::additional_properties(yyjson::one_of({yyjson::minimum(0)}, {yyjson::maximum(9)}))),
    yyjson::field("deep", yyjson::one_of(yyjson::not_(yyjson::minimum(0)),
                                         yyjson::all_of(yyjson::minimum(100),
                                                        yyjson::any_of(yyjson::const_(100), yyjson::minimum(200))))),
    yyjson::field("rows", yyjson::any_of(yyjson::max_items(1), yyjson::items(yyjson::min_items(2)))));

// yyjson::matches at the positions of values of the DOM
struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
};
struct Typed
{
    std::vector<yyjson::value> events = {};
    std::vector<yyjson::value> ids = {};
    yyjson::value point = {};
    yyjson::value text = {};
    std::map<std::string, yyjson::value> small = {};
    bool operator==(const Typed& other) const
    {
        const auto write = [](const auto& t) {
            return std::string(yyjson::write(std::tuple(t.events, t.ids, t.point, t.text, t.small)).data());
        };
        return write(*this) == write(other);
    }
};
template <>
inline constexpr auto yyjson::annotations<Typed> = yyjson::annotate(
    yyjson::field("events", yyjson::contains(yyjson::matches<Point>), yyjson::min_contains(2)),
    yyjson::field("ids", yyjson::items(yyjson::any_of(yyjson::matches<int>, yyjson::matches<std::string>))),
    yyjson::field("point", yyjson::matches<Point>), yyjson::field("text", yyjson::not_(yyjson::matches<std::string>)),
    yyjson::field("small", yyjson::additional_properties(yyjson::matches<std::uint8_t>)));

// the combinators of a struct that VISITABLE_STRUCT describes
struct VisitedCombined
{
    int n = 0;
    std::vector<int> v = {};
    bool operator==(const VisitedCombined&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VisitedCombined> =
    yyjson::annotate(yyjson::field("n", yyjson::not_(yyjson::const_(0))),
                     yyjson::field("v", yyjson::items(yyjson::one_of(yyjson::minimum(5), yyjson::maximum(7)))));
VISITABLE_STRUCT(VisitedCombined, n, v);

// the keywords of the elements of a struct that is transparent
struct Scores
{
    std::vector<int> values = {};
    bool operator==(const Scores&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Scores> = yyjson::annotate(
    yyjson::transparent, yyjson::field("values", yyjson::min_items(1), yyjson::items(yyjson::minimum(0))));

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
    static_assert(table_error(minimum(0)) == keyword_kind);
    static_assert(table_error(yyjson::not_(yyjson::min_items(1))) == keyword_kind);
    static_assert(table_error(yyjson::any_of(yyjson::const_(1))) == struct_value_keyword);
    static_assert(table_error(field("v", yyjson::skip, minimum(0))) == skipped_with_setting);
    static_assert(table_error(field("v", minimum(0)), field("v", yyjson::skip)) == skipped_with_setting);
    static_assert(table_error(field("w", minimum(0), yyjson::adjacent_tag("t", {"x"}))) == tagged_constraint);
    static_assert(table_error(field("v", minimum(0)), field("w", yyjson::adjacent_tag("t", {"x"}))) == none);
    // the keywords of an object that a struct takes: `yyjson::additional_properties(false)` and the counts
    using yyjson::additional_properties;
    static_assert(table_error(additional_properties(false)) == none);
    static_assert(table_error(additional_properties(false), additional_properties(false)) == closed_twice);
    static_assert(table_error(additional_properties(minimum(0))) == none);
    static_assert(table_error(yyjson::min_properties(1)) == none);
    static_assert(table_error(yyjson::max_properties(1), additional_properties(false)) == none);
    static_assert(table_error(yyjson::property_names(yyjson::min_length(1))) == none);
    static_assert(table_error(yyjson::max_properties(1), yyjson::property_names(yyjson::min_length(1))) == none);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), additional_properties(false)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::min_properties(1)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::max_properties(1), yyjson::transparent, field("w", yyjson::skip)) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip),
                              yyjson::property_names(yyjson::min_length(1))) == transparent_object_keyword);
    static_assert(table_error(yyjson::dependent_required("v", {"w"})) == none);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::dependent_required("v", {"w"})) ==
                  transparent_object_keyword);
    // the field of `yyjson::extra_keys`, which takes only the keywords of its values and keys
    using yyjson::extra_keys;
    static_assert(table_error(field("w", extra_keys)) == none);
    static_assert(table_error(field("w", extra_keys, additional_properties(minimum(0)),
                                    yyjson::property_names(yyjson::max_length(1)))) == none);
    static_assert(table_error(field("v", extra_keys), field("w", extra_keys)) == extra_keys_twice);
    static_assert(table_error(field("w", extra_keys, yyjson::rename("x"))) == extra_keys_with_setting);
    static_assert(table_error(field("w", yyjson::required), field("w", extra_keys)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::default_)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::skip)) == extra_keys_with_setting);
    static_assert(table_error(field("w", yyjson::skip), field("w", extra_keys)) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::min_properties(1))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::min_items(1))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, minimum(0))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys, yyjson::adjacent_tag("t", {"x"}))) == extra_keys_with_setting);
    static_assert(table_error(field("w", extra_keys), additional_properties(false)) == extra_keys_denied);
    static_assert(table_error(additional_properties(false), field("w", extra_keys)) == extra_keys_denied);
    static_assert(table_error(field("w", extra_keys), additional_properties(minimum(0))) == none);
    static_assert(table_error(yyjson::transparent, field("w", extra_keys)) == transparent_object_keyword);
    static_assert(table_error(field("w", extra_keys), yyjson::transparent) == transparent_object_keyword);
    static_assert([] {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, true};
        builder.apply_annotation(field("w", extra_keys));
        return builder.result().error;
    }() == extra_keys_visitable);
    // the fields of `yyjson::pattern_properties`, which take the same keywords, and a struct that is closed
    using yyjson::pattern_properties;
    static_assert(table_error(field("v", pattern_properties("^a")), field("w", pattern_properties(""))) == none);
    static_assert(table_error(field("w", pattern_properties("^a"), additional_properties(minimum(0)),
                                    yyjson::property_names(yyjson::max_length(1)))) == none);
    static_assert(table_error(field("v", pattern_properties("^a")), field("w", extra_keys)) == none);
    static_assert(table_error(additional_properties(false), field("w", pattern_properties("^a"))) == none);
    static_assert(table_error(field("w", pattern_properties("^a"), pattern_properties("^b"))) == pattern_twice);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::rename("x"))) == pattern_with_setting);
    static_assert(table_error(field("w", yyjson::required), field("w", pattern_properties("^a"))) ==
                  pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::default_)) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::skip)) == pattern_with_setting);
    static_assert(table_error(field("w", yyjson::skip), field("w", pattern_properties("^a"))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::min_properties(1))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::min_items(1))) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), extra_keys)) == pattern_with_setting);
    static_assert(table_error(field("w", pattern_properties("^a"), yyjson::adjacent_tag("t", {"x"}))) ==
                  pattern_with_setting);
    static_assert(table_error(yyjson::transparent, field("w", pattern_properties("^a"))) == transparent_object_keyword);
    static_assert([] {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, true};
        builder.apply_annotation(field("w", pattern_properties("^a")));
        return builder.result().error;
    }() == pattern_visitable);
    // 65 members of the names of one letter
    inline constexpr char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$";
    static_assert([] {
        auto members = std::array<std::string_view, 65>{};
        for (std::size_t i = 0; i < 65; ++i) members[i] = std::string_view(letters + i, 1);
        auto builder = yyjson::detail::field_key_builder<65>{members, {members}, false};
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (builder.apply_annotation(field(std::string_view(letters + I, 1), pattern_properties("^a"))), ...);
        }(std::make_index_sequence<65>());
        return builder.result().error;
    }() == too_many_patterns);
    using yyjson::detail::extra_keys_map;
    static_assert(extra_keys_map<std::map<std::string, int>> && extra_keys_map<std::unordered_map<std::string, int>> &&
                  extra_keys_map<std::map<std::string, yyjson::value>>);
    static_assert(!extra_keys_map<std::map<int, int>> && !extra_keys_map<std::multimap<std::string, int>> &&
                  !extra_keys_map<std::vector<std::pair<std::string, int>>> && !extra_keys_map<yyjson::object>);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::one_of(yyjson::required("v"))) ==
                  transparent_conditions);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), yyjson::required("v")) ==
                  transparent_object_keyword);
    static_assert(table_error(yyjson::property("v", yyjson::minimum(0)), yyjson::transparent,
                              field("w", yyjson::skip)) == transparent_object_keyword);
}  // namespace constraint_rules

// the tree of the keywords of the field `v` of a struct, by its error
#define CONSTRAINT_RULE(NAME, TYPE, ...) \
    struct NAME                          \
    {                                    \
        TYPE v;                          \
    };                                   \
    template <>                          \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(yyjson::field("v", __VA_ARGS__));
// `yyjson::property` on the struct adds keywords to the value of the field that `yyjson::field` constrains, each
// once
struct PropertyMerged
{
    int v = 0;
    bool operator==(const PropertyMerged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PropertyMerged> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(0)), yyjson::property("v", yyjson::maximum(9)));
struct PropertyTwice
{
    int v = 0;
};
template <>
inline constexpr auto yyjson::annotations<PropertyTwice> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(0)), yyjson::property("v", yyjson::minimum(1)));
// yyjson::additional_properties(false) allows the keys of yyjson::property only, and not those of yyjson::required
struct ClosedKeys
{
    std::optional<int> zip;
    std::optional<int> city;
};
CONSTRAINT_RULE(ClosedRequiredOnly, ClosedKeys, yyjson::additional_properties(false), yyjson::property("city"),
                yyjson::required("zip"))
CONSTRAINT_RULE(ClosedRequiredNamed, ClosedKeys, yyjson::additional_properties(false), yyjson::property("zip"),
                yyjson::required("zip"))
CONSTRAINT_RULE(ClosedRequiredAllOf, ClosedKeys, yyjson::additional_properties(false),
                yyjson::all_of(yyjson::required("zip")))
CONSTRAINT_RULE(ClosedRequiredOneOf, ClosedKeys, yyjson::one_of(yyjson::required("zip"), yyjson::required("city")),
                yyjson::additional_properties(false), yyjson::property("zip"), yyjson::property("city"))
#if !defined(__cpp_lib_define_static)
CONSTRAINT_RULE(RequiredKeyTwice, ClosedKeys, yyjson::any_of(yyjson::required("zip", "zip")))
#endif
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
CONSTRAINT_RULE(ItemsInverted, std::vector<int>, yyjson::min_items(3), yyjson::max_items(2))
CONSTRAINT_RULE(ContainsMissing, std::vector<int>, yyjson::min_contains(1))
CONSTRAINT_RULE(ContainsInverted, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(2),
                yyjson::max_contains(1))
using StringMap = std::map<std::string, int>;
using ScalarVariant = std::variant<int, std::string>;
CONSTRAINT_RULE(PropertiesInverted, StringMap, yyjson::min_properties(2), yyjson::max_properties(1))
CONSTRAINT_RULE(ItemsTwice, std::vector<int>, yyjson::items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(1)))
CONSTRAINT_RULE(ItemsInside, std::vector<int>, yyjson::items(yyjson::minimum(2), yyjson::maximum(1)))
// a position of yyjson::prefix_items holds at most 8 nodes before it is copied into the tree
using Nested10 = std::vector<std::vector<
    std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>>>>>>>;
CONSTRAINT_RULE(PrefixTooLong, Nested10,
                yyjson::prefix_items(yyjson::items(yyjson::items(yyjson::items(
                    yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::minimum(0)))))))))))
// a subschema of a combinator follows the rules of a field, and a field can take a combinator more than once
CONSTRAINT_RULE(NotTwice, int, yyjson::not_(yyjson::minimum(0)), yyjson::not_(yyjson::maximum(1)))
CONSTRAINT_RULE(NotInverted, int, yyjson::not_(yyjson::minimum(2), yyjson::maximum(1)))
CONSTRAINT_RULE(BranchMissingContains, std::vector<int>,
                yyjson::if_then_else(yyjson::if_(yyjson::min_items(1)), yyjson::then(yyjson::min_contains(1))))
CONSTRAINT_RULE(MatchesTwice, yyjson::value, yyjson::matches<int>, yyjson::matches<int>)
// the keywords of a position and of the subschemas of its yyjson::all_of constrain one value
CONSTRAINT_RULE(AllOfInverted, int, yyjson::minimum(1), yyjson::all_of(yyjson::maximum(0)))
CONSTRAINT_RULE(AllOfTwiceInverted, int, yyjson::all_of(yyjson::minimum(1)), yyjson::all_of(yyjson::maximum(0)))
CONSTRAINT_RULE(AllOfNestedInverted, int,
                yyjson::all_of(yyjson::all_of(yyjson::exclusive_minimum(0)), yyjson::not_(yyjson::const_(7))),
                yyjson::maximum(0))
CONSTRAINT_RULE(AllOfStricter, int, yyjson::minimum(0), yyjson::all_of(yyjson::minimum(1)))
CONSTRAINT_RULE(AllOfConstOutside, int, yyjson::const_(5), yyjson::all_of(yyjson::maximum(4)))
CONSTRAINT_RULE(AllOfItemsInverted, std::vector<int>, yyjson::min_items(3), yyjson::all_of(yyjson::max_items(2)))
CONSTRAINT_RULE(AllOfLengthsInverted, std::string, yyjson::all_of(yyjson::min_length(3), yyjson::max_length(2)))
CONSTRAINT_RULE(ContainsNone, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::max_contains(0))
// yyjson::contains of two subschemas count different elements
CONSTRAINT_RULE(ContainsAcrossAllOf, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(2),
                yyjson::all_of(yyjson::contains(yyjson::maximum(0)), yyjson::max_contains(1)))
// a subschema of yyjson::any_of, one_of, not_ and if_then_else may hold a branch that no value satisfies
CONSTRAINT_RULE(AnyOfDead, int,
                yyjson::any_of(yyjson::all_of(yyjson::minimum(5), yyjson::maximum(1)), yyjson::maximum(3)))
CONSTRAINT_RULE(OneOfDead, std::vector<int>,
                yyjson::one_of(yyjson::min_items(2), yyjson::all_of(yyjson::min_items(3), yyjson::max_items(1))))
CONSTRAINT_RULE(BranchesDead, std::string,
                yyjson::if_then_else(yyjson::if_(yyjson::min_length(3), yyjson::max_length(2)),
                                     yyjson::then(yyjson::const_("a"), yyjson::min_length(2)),
                                     yyjson::else_(yyjson::enum_("ab", "cd"), yyjson::max_length(1))))
CONSTRAINT_RULE(ContainsDead, std::vector<int>,
                yyjson::not_(yyjson::contains(yyjson::minimum(0)), yyjson::max_contains(0)))
CONSTRAINT_RULE(ByteDead, unsigned char, yyjson::not_(yyjson::minimum(256)))
CONSTRAINT_RULE(ByteOutside, unsigned char, yyjson::minimum(256))
CONSTRAINT_RULE(ByteAllOfOutside, unsigned char, yyjson::all_of(yyjson::maximum(-1)))
// a contradiction leaves no value of its kind, and a position of more kinds reads the others
CONSTRAINT_RULE(ValueBoundsInverted, yyjson::value, yyjson::minimum(1), yyjson::maximum(0))
CONSTRAINT_RULE(VariantBoundsInverted, ScalarVariant, yyjson::minimum(1), yyjson::maximum(0))
CONSTRAINT_RULE(ValueConstOutside, yyjson::value, yyjson::const_(5), yyjson::maximum(4))
CONSTRAINT_RULE(AnyOfTooLong, Nested10,
                yyjson::any_of(yyjson::items(yyjson::items(yyjson::items(
                    yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::minimum(0)))))))))))
#undef CONSTRAINT_RULE
// the counts of the members of a struct follow the rules of those of a map
struct StructCountsInverted
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsInverted> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::max_properties(1));
struct StructCountsTwice
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsTwice> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::max_properties(1));
struct StructCountsEqual
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructCountsEqual> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(1));
// the counts of the members of a struct and those of the subschemas of its yyjson::all_of constrain one object
struct StructAllOfCounts
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructAllOfCounts> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::all_of(yyjson::max_properties(1)));
struct StructAnyOfCounts
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructAnyOfCounts> = yyjson::annotate(
    yyjson::min_properties(1), yyjson::any_of(yyjson::max_properties(0), yyjson::property("v", yyjson::minimum(0))));
// the names of the members of a struct follow the rules of those of a map
struct StructNamesInverted
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructNamesInverted> =
    yyjson::annotate(yyjson::property_names(yyjson::min_length(3), yyjson::max_length(2)));
struct StructNamesTwice
{
    int v;
};
template <>
inline constexpr auto yyjson::annotations<StructNamesTwice> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::property_names(yyjson::max_length(1)));
namespace constraint_tree_rules
{
    template <typename T>
    constexpr auto names_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_names_tree<T>>().error;
    template <typename T>
    constexpr auto count_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>().error;
    template <typename T>
    constexpr auto tree_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_field_tree<T, 0>>().error;
    // whether the keywords of the position of the field `v` leave no value of its JSON kinds, or of its integer type
    template <typename T>
    constexpr auto tree_of = yyjson::detail::make_schema_tree<&yyjson::detail::build_field_tree<T, 0>>();
    template <typename T>
    constexpr bool empty_position =
        yyjson::detail::position_reads_nothing<yyjson::detail::constrained_value_t<decltype(T::v)>>(tree_of<T>, 0);
    template <typename T>
    constexpr bool out_of_type = yyjson::detail::position_out_of_type<decltype(T::v)>(tree_of<T>, 0);
    template <typename T>
    constexpr bool counts_empty =
        (yyjson::detail::position_empty_kinds(
             yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>(), 0) &
         yyjson::detail::object_kind) != 0;
    template <typename T>
    constexpr bool struct_empty = [] {
        constexpr auto counts =
            yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_count_tree<T>>().nodes[0].constraints;
        return (yyjson::detail::position_empty_kinds(
                    yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_tree<T>>(), 0, {&counts, 1}) &
                yyjson::detail::object_kind) != 0;
    }();
    template <typename T>
    constexpr bool names_empty = [] {
        constexpr auto tree = yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_names_tree<T>>();
        return yyjson::detail::position_reads_nothing<std::string>(tree, tree.nodes[0].keys);
    }();
    using enum yyjson::detail::constraint_error;
    static_assert(tree_error<MinimumTwice> == keyword_twice && tree_error<ConstTwice> == keyword_twice &&
                  tree_error<EnumTwice> == keyword_twice && tree_error<FormatTwice> == keyword_twice &&
                  tree_error<SatisfiesTwice> == keyword_twice);
    static_assert(tree_error<PatternsTwice> == none);
    static_assert(tree_error<PropertyMerged> == none && tree_error<PropertyTwice> == keyword_twice);
    static_assert(tree_error<ClosedRequiredOnly> == closed_required &&
                  tree_error<ClosedRequiredAllOf> == closed_required);
    static_assert(tree_error<ClosedRequiredNamed> == none && tree_error<ClosedRequiredOneOf> == none);
#if !defined(__cpp_lib_define_static)
    static_assert(tree_error<RequiredKeyTwice> == required_key_twice);
#endif
    static_assert(tree_error<MultipleOfZero> == multiple_of_not_positive &&
                  tree_error<MultipleOfNegative> == multiple_of_not_positive);
    static_assert(empty_position<BoundsInverted> && empty_position<BoundsExclusive> &&
                  empty_position<LengthsInverted> && empty_position<ConstOutside> && empty_position<EnumOutside>);
    static_assert(!empty_position<BoundsEqual> && !empty_position<EnumPartlyInside>);
    static_assert(empty_position<ItemsInverted> && empty_position<PropertiesInverted> &&
                  empty_position<ContainsInverted>);
    // the subschema of yyjson::items is a position of its own
    static_assert(tree_error<ItemsInside> == none && !empty_position<ItemsInside> &&
                  yyjson::detail::position_reads_nothing<int>(tree_of<ItemsInside>,
                                                              tree_of<ItemsInside>.nodes[0].items));
    static_assert(tree_error<ContainsMissing> == contains_missing && tree_error<ItemsTwice> == keyword_twice);
    static_assert(tree_error<PrefixTooLong> == prefix_position_too_large);
    static_assert(tree_error<NotTwice> == none && tree_error<NotInverted> == none && !empty_position<NotInverted> &&
                  tree_error<BranchMissingContains> == contains_missing && tree_error<MatchesTwice> == keyword_twice &&
                  tree_error<AnyOfTooLong> == prefix_position_too_large);
    static_assert(counts_empty<StructCountsInverted> && count_error<StructCountsTwice> == keyword_twice &&
                  count_error<StructCountsEqual> == none && !counts_empty<StructCountsEqual>);
    static_assert(names_empty<StructNamesInverted> && names_error<StructNamesTwice> == keyword_twice);
    static_assert(struct_empty<StructAllOfCounts> && !struct_empty<StructAnyOfCounts>);
    static_assert(empty_position<AllOfInverted> && empty_position<AllOfTwiceInverted> &&
                  empty_position<AllOfNestedInverted> && empty_position<AllOfConstOutside> &&
                  empty_position<AllOfItemsInverted> && empty_position<AllOfLengthsInverted>);
    static_assert(!empty_position<AllOfStricter> && tree_error<AllOfStricter> == none);
    static_assert(empty_position<ContainsNone> && !empty_position<ContainsAcrossAllOf>);
    static_assert(tree_error<AnyOfDead> == none && !empty_position<AnyOfDead> && !empty_position<OneOfDead> &&
                  !empty_position<BranchesDead> && !empty_position<ContainsDead>);
    static_assert(!out_of_type<ByteDead> && out_of_type<ByteOutside> && out_of_type<ByteAllOfOutside>);
    static_assert(!empty_position<ValueBoundsInverted> && !empty_position<VariantBoundsInverted> &&
                  empty_position<BoundsInverted> && empty_position<ValueConstOutside>);
    // the JSON kinds of a variant are those of its alternatives
    using yyjson::detail::json_kinds_of, yyjson::detail::misplaced_keywords, yyjson::detail::constraint_bit;
    static_assert(misplaced_keywords(constraint_bit<yyjson::min_items>, json_kinds_of<ScalarVariant>()) != 0 &&
                  misplaced_keywords(constraint_bit<yyjson::min_items>,
                                     json_kinds_of<std::variant<int, std::vector<int>>>()) == 0);

    // the keywords that a field without them leaves unchecked, and those that are no check
    static_assert(!yyjson::detail::has_constraints<Spot> && yyjson::detail::has_constraints<Bounded>);
    static_assert(yyjson::detail::constrained_field<Bounded, 0> && !yyjson::detail::constrained_field<Described, 0>);
}  // namespace constraint_tree_rules

// Casters of the user whose JSON value the keywords of a field check: translators of a scalar, a range and a
// struct, a caster that takes a value of the DOM, and casters that take a `json_view`.  A class with private
// members is not reflected, so that the keywords of an object check its JSON value too.
class CasterName
{
    std::string s_;

public:
    CasterName() = default;
    explicit CasterName(std::string s) : s_(std::move(s)) {}
    const std::string& str() const { return s_; }
    bool operator==(const CasterName&) const = default;
};
template <>
struct yyjson::caster<CasterName>
{
    static CasterName from_json(std::string s) { return CasterName(std::move(s)); }
    static std::string to_json(const CasterName& n) { return n.str(); }
};
// the in-place form of a translator
class CasterLabel
{
    std::string s_;

public:
    const std::string& str() const { return s_; }
    void set(std::string s) { s_ = std::move(s); }
    bool operator==(const CasterLabel&) const = default;
};
template <>
struct yyjson::caster<CasterLabel>
{
    static void from_json(CasterLabel& l, std::string s) { l.set(std::move(s)); }
    static std::string to_json(const CasterLabel& l) { return l.str(); }
};
template <typename T>
class CasterScalar
{
    T v_{};

public:
    CasterScalar() = default;
    explicit CasterScalar(T v) : v_(v) {}
    T get() const { return v_; }
    bool operator==(const CasterScalar& o) const { return !(v_ < o.v_) && !(o.v_ < v_); }
};
template <typename T>
struct yyjson::caster<CasterScalar<T>>
{
    static CasterScalar<T> from_json(T v) { return CasterScalar<T>(v); }
    static T to_json(const CasterScalar<T>& s) { return s.get(); }
};
class CasterList
{
    std::vector<int> v_;

public:
    CasterList() = default;
    explicit CasterList(std::vector<int> v) : v_(std::move(v)) {}
    const std::vector<int>& get() const { return v_; }
    bool operator==(const CasterList&) const = default;
};
template <>
struct yyjson::caster<CasterList>
{
    static CasterList from_json(std::vector<int> v) { return CasterList(std::move(v)); }
    static std::vector<int> to_json(const CasterList& l) { return l.get(); }
};
struct CasterPoint
{
    int x = 0;
    int y = 0;
    bool operator==(const CasterPoint&) const = default;
};
class CasterPos
{
    CasterPoint p_;

public:
    CasterPos() = default;
    explicit CasterPos(CasterPoint p) : p_(p) {}
    CasterPoint get() const { return p_; }
    bool operator==(const CasterPos&) const = default;
};
template <>
struct yyjson::caster<CasterPos>
{
    static CasterPos from_json(CasterPoint p) { return CasterPos(p); }
    static CasterPoint to_json(const CasterPos& p) { return p.get(); }
};
// a caster that takes a value of either DOM API
class CasterDom
{
    std::vector<int> v_;

public:
    CasterDom() = default;
    explicit CasterDom(std::vector<int> v) : v_(std::move(v)) {}
    const std::vector<int>& get() const { return v_; }
    bool operator==(const CasterDom&) const = default;
};
template <>
struct yyjson::caster<CasterDom>
{
    template <typename Json>
    static CasterDom from_json(const Json& json)
    {
        return CasterDom(json.template cast<std::vector<int>>());
    }
    static std::vector<int> to_json(const CasterDom& d) { return d.get(); }
};
// type-level counts of a struct that a translator from a map reads
struct CasterPair
{
    int x = 0;
    int y = 0;
    bool operator==(const CasterPair&) const = default;
};
template <>
struct yyjson::caster<CasterPair>
{
    static CasterPair from_json(const std::map<std::string, int>& m) { return {m.at("x"), m.at("y")}; }
    static std::map<std::string, int> to_json(const CasterPair& p) { return {{"x", p.x}, {"y", p.y}}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterPair> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::max_properties(2));

struct CasterFields
{
    CasterName color;
    CasterName name;
    CasterLabel label;
    CasterScalar<double> real;
    CasterScalar<int> small;
    CasterScalar<bool> flag;
    CasterList list;
    CasterPos pos;
    CasterDom dom;
    std::optional<CasterName> opt;
    yyjson::nullable<CasterName> nul;
    bool operator==(const CasterFields&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterFields> =
    yyjson::annotate(yyjson::field("color", yyjson::enum_("red", "blue")),
                     yyjson::field("name", yyjson::max_length(3), yyjson::pattern("^[a-z]")),
                     yyjson::field("label", yyjson::min_length(2)),
                     yyjson::field("real", yyjson::minimum(0), yyjson::exclusive_maximum(1.5)),
                     yyjson::field("small", yyjson::maximum(10)), yyjson::field("flag", yyjson::const_(true)),
                     yyjson::field("list", yyjson::max_items(2), yyjson::unique_items),
                     yyjson::field("pos", yyjson::const_(yyjson::json_text(R"({"x":1,"y":2})"))),
                     yyjson::field("dom", yyjson::min_items(1)), yyjson::field("opt", yyjson::max_length(2)),
                     yyjson::property("nul", yyjson::max_length(2)));

struct CasterPairs
{
    CasterPair pair;
    std::vector<CasterPair> pairs;
    bool operator==(const CasterPairs&) const = default;
};

// a caster that sorts the strings that it reads, so that the value that it returns differs from the JSON value
class CasterSorted
{
    std::vector<std::string> v_;

public:
    CasterSorted() = default;
    explicit CasterSorted(std::vector<std::string> v) : v_(std::move(v)) {}
    const std::vector<std::string>& get() const { return v_; }
    bool operator==(const CasterSorted&) const = default;
};
template <>
struct yyjson::caster<CasterSorted>
{
    static CasterSorted from_json(std::vector<std::string> v)
    {
        std::ranges::sort(v);
        return CasterSorted(std::move(v));
    }
    static std::vector<std::string> to_json(const CasterSorted& s) { return s.get(); }
};
struct CasterSortedField
{
    CasterSorted h;
    bool operator==(const CasterSortedField&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedField> =
    yyjson::annotate(yyjson::field("h", yyjson::enum_(yyjson::json_text(R"(["a","b"])"))));
struct CasterSortedProperty
{
    CasterSorted h;
    bool operator==(const CasterSortedProperty&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedProperty> =
    yyjson::annotate(yyjson::property("h", yyjson::enum_(yyjson::json_text(R"(["a","b"])"))));
struct CasterSortedText
{
    CasterSorted plain;
    CasterSorted quoted;
    bool operator==(const CasterSortedText&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedText> =
    yyjson::annotate(yyjson::field("plain", yyjson::enum_(yyjson::json_text(R"(["a","bc"])"))),
                     yyjson::field("quoted", yyjson::const_(yyjson::json_text(R"(["a\"b","\u00e9\n"])"))));

#if YYJSON_VERSION_HEX >= 0x000C00
// strings read through a view, from an array of strings or from one string
class CasterTags
{
    std::vector<std::string> v_;

public:
    CasterTags() = default;
    explicit CasterTags(std::vector<std::string> v) : v_(std::move(v)) {}
    const std::vector<std::string>& get() const { return v_; }
    bool operator==(const CasterTags&) const = default;
};
template <>
struct yyjson::caster<CasterTags>
{
    static CasterTags from_json(yyjson::json_view json)
    {
        auto v = std::vector<std::string>();
        if (const auto s = json.as_string())
            v.emplace_back(*s);
        else if (!json.is_null())
        {
            const auto a = *json.as_array();
            for (const auto e : a) v.emplace_back(*e.as_string());
        }
        return CasterTags(std::move(v));
    }
    static std::vector<std::string> to_json(const CasterTags& t) { return t.get(); }
};
// reads the first element of an array only
class CasterFirst
{
    int v_ = 0;

public:
    CasterFirst() = default;
    explicit CasterFirst(int v) : v_(v) {}
    int get() const { return v_; }
    bool operator==(const CasterFirst&) const = default;
};
template <>
struct yyjson::caster<CasterFirst>
{
    static CasterFirst from_json(yyjson::json_view json)
    {
        const auto a = *json.as_array();
        return CasterFirst(a[0].cast<int>());
    }
    static std::vector<int> to_json(const CasterFirst& f) { return {f.get()}; }
};
// an object, which the in-place form merges into the members that the field holds
class CasterBag
{
    std::map<std::string, int> m_;

public:
    const std::map<std::string, int>& get() const { return m_; }
    void set(std::string_view k, int v) { m_[std::string(k)] = v; }
    bool operator==(const CasterBag&) const = default;
};
template <>
struct yyjson::caster<CasterBag>
{
    static void from_json(CasterBag& b, yyjson::json_view json)
    {
        const auto o = *json.as_object();
        for (const auto& [k, v] : o) b.set(k, v.cast<int>());
    }
    static std::map<std::string, int> to_json(const CasterBag& b) { return b.get(); }
};
// a caster that reads the keys of an object from its members, and not through a view
class CasterKeySet
{
    std::map<std::string, int> m_;

public:
    CasterKeySet() = default;
    explicit CasterKeySet(std::map<std::string, int> m) : m_(std::move(m)) {}
    const std::map<std::string, int>& get() const { return m_; }
    bool operator==(const CasterKeySet&) const = default;
};
template <>
struct yyjson::caster<CasterKeySet>
{
    static CasterKeySet from_json(std::map<std::string, int> m) { return CasterKeySet(std::move(m)); }
    static std::map<std::string, int> to_json(const CasterKeySet& s) { return s.get(); }
};
// the keywords of the keys check the object that the caster reads
struct CasterKeysField
{
    CasterBag bag;
    CasterKeySet set;
    bool operator==(const CasterKeysField&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterKeysField> =
    yyjson::annotate(yyjson::field("bag", yyjson::additional_properties(false)),
                     yyjson::field("set", yyjson::property_names(yyjson::max_length(1))));
struct CasterKeysProperty
{
    CasterBag bag;
    CasterKeySet set;
    bool operator==(const CasterKeysProperty&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterKeysProperty> =
    yyjson::annotate(yyjson::property("bag", yyjson::property_names(yyjson::max_length(1))),
                     yyjson::property("set", yyjson::additional_properties(false)));
// a caster that reads the JSON value of any kind, whose keywords report as on a value of the DOM
class CasterAnyJson
{
};
template <>
struct yyjson::caster<CasterAnyJson>
{
    static CasterAnyJson from_json(yyjson::json_view) { return {}; }
    static int to_json(const CasterAnyJson&) { return 0; }
};
inline constexpr auto caster_message_keywords = yyjson::annotate(
    yyjson::field("a", yyjson::property_names(yyjson::max_length(1)), yyjson::min_properties(1),
                  yyjson::max_properties(2)),
    yyjson::field("u", yyjson::unique_items, yyjson::min_items(1), yyjson::max_items(3)),
    yyjson::field("c", yyjson::enum_(yyjson::json_text("[1]"), yyjson::json_text("{}"))),
    yyjson::field("k", yyjson::const_(yyjson::json_text(R"({"x":1})"))),
    yyjson::field("s", yyjson::max_length(2), yyjson::minimum(1)), yyjson::field("o", yyjson::unique_items));
struct CasterMessagesOfCaster
{
    CasterAnyJson a, u, c, k, s;
    std::optional<CasterAnyJson> o;
};
struct CasterMessagesOfValue
{
    yyjson::value a, u, c, k, s;
    std::optional<yyjson::value> o;
};
template <>
inline constexpr auto yyjson::annotations<CasterMessagesOfCaster> = caster_message_keywords;
template <>
inline constexpr auto yyjson::annotations<CasterMessagesOfValue> = caster_message_keywords;
// type-level counts of a reflectable struct that a caster reads through a view
struct CasterXY
{
    int x = 0;
    int y = 0;
    bool operator==(const CasterXY&) const = default;
};
template <>
struct yyjson::caster<CasterXY>
{
    static CasterXY from_json(yyjson::json_view json)
    {
        const auto o = *json.as_object();
        return {o["x"].cast<int>(), o["y"].cast<int>()};
    }
    static std::map<std::string, int> to_json(const CasterXY& p) { return {{"x", p.x}, {"y", p.y}}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterXY> = yyjson::annotate(yyjson::max_properties(2));
// type-level keys of the objects that casters read through a view
struct CasterShortKeys
{
    int n = 0;
    bool operator==(const CasterShortKeys&) const = default;
};
template <>
struct yyjson::caster<CasterShortKeys>
{
    static CasterShortKeys from_json(yyjson::json_view json) { return {static_cast<int>(json.as_object()->size())}; }
    static std::map<std::string, int> to_json(const CasterShortKeys&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterShortKeys> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(1)));
struct CasterNoKeys
{
    int n = 0;
    bool operator==(const CasterNoKeys&) const = default;
};
template <>
struct yyjson::caster<CasterNoKeys>
{
    static CasterNoKeys from_json(yyjson::json_view json) { return {static_cast<int>(json.as_object()->size())}; }
    static std::map<std::string, int> to_json(const CasterNoKeys&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterNoKeys> = yyjson::annotate(yyjson::additional_properties(false));

struct CasterViews
{
    CasterTags tags;
    CasterTags mixed;
    CasterFirst first;
    CasterBag bag;
    std::optional<CasterTags> opt;
    CasterXY xy;
    std::vector<CasterXY> xys;
    bool operator==(const CasterViews&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterViews> = yyjson::annotate(
    yyjson::field("tags", yyjson::min_items(1), yyjson::max_items(3), yyjson::unique_items),
    yyjson::field("mixed", yyjson::max_items(1), yyjson::max_length(2)), yyjson::field("first", yyjson::max_items(2)),
    yyjson::field("bag", yyjson::max_properties(1)),
    yyjson::field("opt", yyjson::max_items(2),
                  yyjson::enum_(yyjson::json_text(R"(["a"])"), yyjson::json_text(R"(["a","b","c"])"))));

// a null that the caster reads, in yyjson::const_ and yyjson::enum_
struct CasterNulls
{
    std::optional<CasterTags> opt;
    std::optional<CasterTags> text;
    CasterTags one;
    bool operator==(const CasterNulls&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterNulls> = yyjson::annotate(
    yyjson::field("opt", yyjson::enum_(yyjson::json_text(R"(["a"])"), yyjson::json_text("null"))),
    yyjson::field("text", yyjson::const_(yyjson::json_text("null"))), yyjson::field("one", yyjson::const_(nullptr)));

// one thread that runs each task, as a thread pool does
class CasterWorker
{
    std::mutex m_;
    std::condition_variable cv_;
    std::function<void()> job_;
    bool quit_ = false;
    std::thread t_;

public:
    CasterWorker()
        : t_([this] {
              auto lock = std::unique_lock(m_);
              while (true)
              {
                  cv_.wait(lock, [&] { return job_ || quit_; });
                  if (quit_) return;
                  job_();
                  job_ = nullptr;
                  cv_.notify_all();
              }
          })
    {
    }
    CasterWorker(const CasterWorker&) = delete;
    CasterWorker& operator=(const CasterWorker&) = delete;
    void run(std::function<void()> f)
    {
        auto lock = std::unique_lock(m_);
        job_ = std::move(f);
        cv_.notify_all();
        cv_.wait(lock, [&] { return !job_; });
    }
    ~CasterWorker()
    {
        {
            const auto lock = std::lock_guard(m_);
            quit_ = true;
        }
        cv_.notify_all();
        t_.join();
    }
};
// counts the elements of an array on the worker, if there is one
class CasterRemote
{
public:
    static inline CasterWorker* worker = nullptr;
    std::size_t n = 0;
    bool operator==(const CasterRemote&) const = default;
};
template <>
struct yyjson::caster<CasterRemote>
{
    static CasterRemote from_json(yyjson::json_view json)
    {
        auto r = CasterRemote();
        if (CasterRemote::worker != nullptr)
            CasterRemote::worker->run([&] {
                const auto a = *json.as_array();
                for (const auto e : a) static_cast<void>(e), ++r.n;
            });
        return r;
    }
    static std::size_t to_json(const CasterRemote& r) { return r.n; }
};
struct CasterRemoteItems
{
    CasterRemote items;
    bool operator==(const CasterRemoteItems&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterRemoteItems> =
    yyjson::annotate(yyjson::field("items", yyjson::max_items(3)));

// a struct with yyjson::transparent reads the JSON value of its field
struct CasterWrapped
{
    CasterTags tags;
    bool operator==(const CasterWrapped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterWrapped> =
    yyjson::annotate(yyjson::transparent, yyjson::field("tags", yyjson::max_items(1)));
#endif

#if YYJSON_VERSION_HEX >= 0x000C00
// the keywords of a field check the elements and the members of the JSON value that a caster reads too
struct CasterItems
{
    CasterList list;
    CasterTags tags;
    CasterKeySet set;
    CasterBag bag;
    bool operator==(const CasterItems&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterItems> = yyjson::annotate(
    yyjson::field("list", yyjson::prefix_items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(1)),
                  yyjson::any_of(yyjson::max_items(1), yyjson::contains(yyjson::const_(1)))),
    yyjson::field("tags", yyjson::items(yyjson::max_length(1)), yyjson::not_(yyjson::contains(yyjson::const_("x")))),
    yyjson::field("set", yyjson::property("a", yyjson::maximum(1)), yyjson::required("a"),
                  yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("bag", yyjson::dependent_required("a", {"b"}),
                  yyjson::if_then_else(yyjson::if_(yyjson::required("c")), yyjson::then(yyjson::max_properties(1)))));
#endif
namespace caster_json_rules
{
    using yyjson::detail::checks_caster_json, yyjson::detail::field_schema;
    static_assert(checks_caster_json<field_schema<CasterFields, 0>, CasterName>() &&
                  checks_caster_json<field_schema<CasterFields, 6>, CasterList>() &&
                  checks_caster_json<field_schema<CasterFields, 9>, std::optional<CasterName>>());
    static_assert(yyjson::detail::misplaced_keywords(yyjson::detail::constraint_bit<yyjson::min_items>,
                                                     yyjson::detail::json_kinds_of<CasterList>()) != 0);
}  // namespace caster_json_rules

// a bound is compared as a double, so a long double that it would round does not compile; Clang 17 and older
// take no floating-point template argument
#if !defined(__clang__) || __clang_major__ >= 18
template <auto V>
concept bound_accepted = requires { typename std::integral_constant<bool, (yyjson::minimum(V), true)>; };
static_assert(bound_accepted<1.5L> && bound_accepted<9007199254740992.0L>);
static_assert(!bound_accepted<9007199254740993.0L> || sizeof(long double) == sizeof(double));
#endif

// if_then_else and not_ on a struct: a circle has a radius of at least 0, another shape a point and three sides or
// more, and no kind starts with "x"; yyjson::field in them names a field by its JSON key
struct ConditionPoint
{
    int x = 0;
};
struct ConditionShape
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<yyjson::value> p;
    std::optional<std::vector<int>> sides;
};
template <>
inline constexpr auto yyjson::annotations<ConditionShape> = yyjson::annotate(
    yyjson::field("r", yyjson::rename("radius")),
    yyjson::if_then_else(
        yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("circle"))),
        yyjson::then(yyjson::required("radius"), yyjson::property("radius", yyjson::minimum(0))),
        yyjson::else_(yyjson::property("p", yyjson::matches<ConditionPoint>),
                      yyjson::property("sides", yyjson::items(yyjson::minimum(1)), yyjson::min_items(3)))),
    yyjson::not_(yyjson::property("kind", yyjson::pattern("^x"))));
// a field with yyjson::required in a condition, and an if_then_else without then
struct Leveled
{
    int level = 0;
    std::optional<std::string> note;
    bool operator==(const Leveled&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Leveled> = yyjson::annotate(
    yyjson::field("level", yyjson::required),
    yyjson::if_then_else(yyjson::if_(yyjson::property("level", yyjson::maximum(4))),
                         yyjson::else_(yyjson::required("note"), yyjson::property("note", yyjson::min_length(1)))));
// VISITABLE_STRUCT
struct ConditionVisitable
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const ConditionVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConditionVisitable> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::required("a")), yyjson::then(yyjson::required("b"))));
VISITABLE_STRUCT(ConditionVisitable, a, b);
// VISITABLE_STRUCT in another order than the declarations, and a struct that is not an aggregate
struct ConditionReordered
{
    int a = 0;
    int b = 0;
    int c = 0;
    bool operator==(const ConditionReordered&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConditionReordered> =
    yyjson::annotate(yyjson::any_of(yyjson::property("c", yyjson::minimum(5))));
VISITABLE_STRUCT(ConditionReordered, c, a, b);
class ConditionConstructed
{
public:
    ConditionConstructed() = default;
    explicit ConditionConstructed(int a) : a_(a) {}
    bool operator==(const ConditionConstructed&) const = default;

    int a_ = 0;
    int b_ = 0;
};
template <>
inline constexpr auto yyjson::annotations<ConditionConstructed> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("a_", yyjson::minimum(1))), yyjson::then(yyjson::required("b_"))));
VISITABLE_STRUCT(ConditionConstructed, b_, a_);
// fields whose keys a condition tests while their values cannot tell: a std::optional with an initializer and plain
// fields; reading tells by the keys of the object
struct ConditionPresence
{
    std::optional<int> a = 5;
    int b = 0;
    int c = 7;
    int e = 0;
    std::optional<int> d;
    bool operator==(const ConditionPresence&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConditionPresence> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::required("a")), yyjson::then(yyjson::required("d"))),
                     yyjson::if_then_else(yyjson::if_(yyjson::required("b")), yyjson::then(yyjson::required("d"))),
                     yyjson::not_(yyjson::required("c"), yyjson::required("e")));
// null in a field that cannot hold null, which the read takes as a missing key, and in one that holds it
struct NullKeys
{
    std::optional<int> a = 5;
    std::optional<int> b;
    yyjson::nullable<int> c;
    std::optional<int> d;
    bool operator==(const NullKeys&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NullKeys> =
    yyjson::annotate(yyjson::one_of(yyjson::required("a"), yyjson::required("b")),
                     yyjson::all_of(yyjson::property("a", yyjson::maximum(3))),
                     yyjson::if_then_else(yyjson::if_(yyjson::required("c")), yyjson::then(yyjson::required("d"))));
// all_of, any_of and one_of on a struct: a name or an id, exactly one of an email and a phone, an age of at least
// 0, and no id of 0 or less
struct Contact
{
    std::optional<std::string> name{};
    std::optional<int> id{};
    std::optional<std::string> email{};
    std::optional<std::string> phone{};
    std::optional<int> age{};
    bool operator==(const Contact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Contact> =
    yyjson::annotate(yyjson::any_of(yyjson::required("name"), yyjson::required("id")),
                     yyjson::one_of(yyjson::required("email"), yyjson::required("phone")),
                     yyjson::all_of({yyjson::property("age", yyjson::minimum(0))},
                                    yyjson::not_(yyjson::required("id"), yyjson::property("id", yyjson::maximum(0)))));
// the conditions that merge_into checks on the object after the merge: a port with a default that needs a
// certificate, with and without yyjson::emit_defaults
struct MergedServer
{
    std::string host;
    int port = 8080;
    std::optional<std::string> cert;
    bool operator==(const MergedServer&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedServer> =
    yyjson::annotate(yyjson::field("port", yyjson::default_), yyjson::dependent_required("port", {"cert"}));
struct MergedEmitted
{
    std::string host;
    int port = 8080;
    std::optional<std::string> cert;
    bool operator==(const MergedEmitted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedEmitted> = yyjson::annotate(
    yyjson::field("port", yyjson::default_, yyjson::emit_defaults), yyjson::dependent_required("port", {"cert"}));
// exactly one of "a" and "b"
struct MergedChoice
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const MergedChoice&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedChoice> =
    yyjson::annotate(yyjson::one_of(yyjson::required("a"), yyjson::required("b")));
// a key that the object needs
struct MergedRequired
{
    int id = 0;
    std::optional<int> n;
    bool operator==(const MergedRequired&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedRequired> = yyjson::annotate(yyjson::required("id"));
// counts of members: a field that the write leaves out when it is zero, and a default that the minimum writes
struct OmittedZero
{
    static int to_json(int v) { return v; }
    static int from_json(int v) { return v; }
    static bool should_omit(int v) { return v == 0; }
};
struct MergedCounted
{
    int a = 0;
    std::optional<int> b;
    yyjson::with_caster<int, OmittedZero> z;
};
template <>
inline constexpr auto yyjson::annotations<MergedCounted> = yyjson::annotate(yyjson::max_properties(2));
struct MergedMinimum
{
    int a = 0;
    std::optional<int> b;
    bool operator==(const MergedMinimum&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedMinimum> =
    yyjson::annotate(yyjson::field("a", yyjson::default_), yyjson::min_properties(2));
// a closed subschema in yyjson::one_of
struct MergedClosed
{
    std::optional<int> x;
    std::optional<int> z;
    bool operator==(const MergedClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedClosed> = yyjson::annotate(yyjson::one_of(
    {yyjson::property("x"), yyjson::additional_properties(false)}, {yyjson::required("x"), yyjson::required("z")}));
// the keywords of an object at a field
struct MergedPoint
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const MergedPoint&) const = default;
};
struct MergedPath
{
    MergedPoint start;
    std::optional<MergedPoint> end;
    bool operator==(const MergedPath&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedPath> =
    yyjson::annotate(yyjson::field("start", yyjson::required("y")), yyjson::field("end", yyjson::min_properties(2)));
// values that a merge reads anew: a member initializer that is not the default of the type, and arrays
struct MergedInit
{
    std::optional<int> x = 5;
    std::optional<int> y;
    bool operator==(const MergedInit&) const = default;
};
struct MergedArrays
{
    std::vector<MergedPoint> items;
    std::vector<std::optional<int>> v;
    bool operator==(const MergedArrays&) const = default;
};
struct MergedAbsent
{
    std::optional<MergedRequired> req;
    std::optional<MergedInit> init;
    std::optional<MergedPoint> point;
    std::optional<MergedArrays> arrays;
    std::map<std::string, MergedRequired> reqs;
    std::map<std::string, MergedMinimum> mins;
    bool operator==(const MergedAbsent&) const = default;
};
// a field that the write omits as equal to its default, which a merge reads from the patch as an absent value; the
// default of "min" violates the minimum of its struct, so that no write gives it
struct OmittedInner
{
    std::optional<int> y;
    int z = 0;
    bool operator==(const OmittedInner&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OmittedInner> = yyjson::annotate(yyjson::dependent_required("y", {"z"}));
struct OmittedOuter
{
    std::string name;
    OmittedInner in;
    MergedMinimum min;
    bool operator==(const OmittedOuter&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OmittedOuter> =
    yyjson::annotate(yyjson::field("in", yyjson::default_), yyjson::field("min", yyjson::default_));
// counts in a condition on a field that a patch does not name
struct KeptGate
{
    std::optional<int> a;
    std::set<int> s;
    std::map<std::string, int> m;
    bool operator==(const KeptGate&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeptGate> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("a")),
    yyjson::then(yyjson::property("s", yyjson::min_items(2)), yyjson::property("m", yyjson::min_properties(2)))));
// the maps of the keys that no field names
struct MergedRest
{
    int a = 0;
    std::map<std::string, MergedRequired> ks;
    std::map<std::string, MergedRequired> rest;
    bool operator==(const MergedRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedRest> =
    yyjson::annotate(yyjson::field("ks", yyjson::pattern_properties("^k")), yyjson::field("rest", yyjson::extra_keys));
// fields that the write omits as defaults, and an adjacent tag whose null makes the merge use a document
struct WrittenInner
{
    std::optional<int> y;
    int z = 0;
    bool operator==(const WrittenInner&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WrittenInner> = yyjson::annotate(yyjson::field("z", yyjson::required));
struct WrittenOuter
{
    WrittenInner in;
    std::map<std::string, int> m;
    std::variant<std::monostate, int, std::string> body;
    bool operator==(const WrittenOuter&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WrittenOuter> =
    yyjson::annotate(yyjson::field("in", yyjson::default_), yyjson::field("m", yyjson::default_),
                     yyjson::field("body", yyjson::adjacent_tag("t", {"num", "text"})));
// the keywords on the members of a map in a combinator, which hold on each member but not on the whole map
struct MergedMapCombinators
{
    std::map<std::string, int> no_short;
    std::map<std::string, int> one_length;
    std::map<std::string, int> no_small;
    std::map<std::string, std::map<std::string, int>> inner;
    bool operator==(const MergedMapCombinators&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedMapCombinators> = yyjson::annotate(
    yyjson::field("no_short", yyjson::not_(yyjson::property_names(yyjson::max_length(1)))),
    yyjson::field("one_length", yyjson::any_of(yyjson::property_names(yyjson::max_length(1)),
                                               yyjson::property_names(yyjson::min_length(2)))),
    yyjson::field("no_small", yyjson::not_(yyjson::additional_properties(yyjson::maximum(5)))),
    yyjson::field("inner", yyjson::additional_properties(yyjson::not_(yyjson::property_names(yyjson::max_length(1))))));
// a map whose comparator keeps a state
struct ReversedOrder
{
    bool reversed = false;
    bool operator()(const std::string& a, const std::string& b) const { return reversed ? b < a : a < b; }
};
struct MergedOrdered
{
    std::map<std::string, int, ReversedOrder> m;
};
template <>
inline constexpr auto yyjson::annotations<MergedOrdered> =
    yyjson::annotate(yyjson::field("m", yyjson::max_properties(3), yyjson::property_names(yyjson::min_length(1))));
struct MergedResource
{
    std::pmr::map<std::string, int> m;
};
template <>
inline constexpr auto yyjson::annotations<MergedResource> =
    yyjson::annotate(yyjson::field("m", yyjson::max_properties(3), yyjson::property_names(yyjson::min_length(1))));
// the counts of the items of a set in a combinator, which the input of the read gives
struct MergedSets
{
    std::map<std::string, std::set<int>> least;
    std::map<std::string, std::set<int>> most;
    std::map<std::string, std::map<std::string, std::set<int>>> inner;
    bool operator==(const MergedSets&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedSets> = yyjson::annotate(
    yyjson::field("least",
                  yyjson::any_of(yyjson::additional_properties(yyjson::min_items(2)), yyjson::max_properties(0))),
    yyjson::field("most",
                  yyjson::any_of(yyjson::additional_properties(yyjson::max_items(1)), yyjson::max_properties(0))),
    yyjson::field("inner", yyjson::additional_properties(yyjson::any_of(
                               yyjson::additional_properties(yyjson::min_items(2)), yyjson::max_properties(0)))));
// the object keywords on a struct that a map holds, which the patch merges in place
struct MergedSpot
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const MergedSpot&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedSpot> = yyjson::annotate(yyjson::field("x"), yyjson::field("y"));
struct MergedStructs
{
    std::map<std::string, MergedSpot> least;
    std::map<std::string, MergedSpot> most;
    std::map<std::string, MergedSpot> rest;
    std::map<std::string, std::map<std::string, MergedSpot>> deep;
    bool operator==(const MergedStructs&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedStructs> = yyjson::annotate(
    yyjson::field("least", yyjson::additional_properties(yyjson::min_properties(2))),
    yyjson::field("most",
                  yyjson::any_of(yyjson::additional_properties(yyjson::max_properties(1)), yyjson::max_properties(0))),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::min_properties(2))),
    yyjson::field("deep", yyjson::additional_properties(yyjson::additional_properties(yyjson::min_properties(2)))));
#ifdef __cpp_lib_flat_map
// a map whose merge moves its members
struct MergedFlatCounts
{
    std::flat_map<std::string, int> names;
    std::flat_map<std::string, int> values;
    bool operator==(const MergedFlatCounts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedFlatCounts> = yyjson::annotate(
    yyjson::field(
        "names",
        yyjson::any_of(yyjson::property_names(yyjson::max_length(1)), yyjson::property_names(yyjson::min_length(2))),
        yyjson::max_properties(2)),
    yyjson::field("values", yyjson::additional_properties(yyjson::maximum(5)), yyjson::max_properties(2)));
#endif
// matches in the conditions: an object in "auto" needs a name, "kind" is not null, and "point" read as
// ConditionPoint, which strict reading does not do with another key, needs a label
struct TypeTested
{
    std::optional<yyjson::value> auto_;
    std::optional<std::string> name;
    std::optional<yyjson::value> kind;
    std::optional<yyjson::value> point;
    std::optional<std::string> label;
};
template <>
inline constexpr auto yyjson::annotations<TypeTested> = yyjson::annotate(
    yyjson::field("auto_", yyjson::rename("auto")),
    yyjson::if_then_else(
        yyjson::if_(yyjson::required("auto"), yyjson::property("auto", yyjson::matches<yyjson::object>)),
        yyjson::then(yyjson::required("name"))),
    yyjson::if_then_else(
        yyjson::if_(yyjson::required("point"), yyjson::property("point", yyjson::matches<ConditionPoint>)),
        yyjson::then(yyjson::required("label"))),
    yyjson::not_(yyjson::required("kind"), yyjson::property("kind", yyjson::matches<std::nullptr_t>)));
// keywords of the values in the conditions: a kind "few" when there are two or three xs, but not with four or more
struct AlignedConditions
{
    std::optional<std::string> kind;
    std::vector<int> xs;
    std::vector<int> ys;
    std::vector<std::vector<int>> rows;
};
template <>
inline constexpr auto yyjson::annotations<AlignedConditions> = yyjson::annotate(
    yyjson::field("ys", yyjson::not_(yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2))),
    yyjson::field("rows", yyjson::items(yyjson::not_(yyjson::contains(yyjson::const_(0))))),
    yyjson::if_then_else(yyjson::if_(yyjson::property("xs", yyjson::min_items(2), yyjson::max_items(3))),
                         yyjson::then(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("few")))),
    yyjson::not_(yyjson::property("xs", yyjson::min_items(4)), yyjson::required("kind"),
                 yyjson::property("kind", yyjson::const_("few"))));
// prefix_items in the conditions: the elements at the positions satisfy the settings, and an array shorter than
// the positions satisfies them too
struct PrefixConditions
{
    std::vector<int> v;
    std::vector<std::string> w;
    std::tuple<int, std::string> t;
    std::optional<std::vector<yyjson::value>> d;
    std::optional<int> n;
};
template <>
inline constexpr auto yyjson::annotations<PrefixConditions> = yyjson::annotate(
    yyjson::field("v", yyjson::if_then_else(yyjson::if_(yyjson::prefix_items(yyjson::minimum(10))),
                                            yyjson::then(yyjson::max_items(2)))),
    yyjson::field("w", yyjson::not_(yyjson::min_items(2), yyjson::prefix_items({}, yyjson::const_("x")))),
    yyjson::field("t", yyjson::not_(yyjson::prefix_items(yyjson::const_(1), yyjson::const_("y")))),
    yyjson::if_then_else(
        yyjson::if_(yyjson::required("d"), yyjson::property("d", yyjson::prefix_items(yyjson::const_("a")))),
        yyjson::then(yyjson::required("n"))));

// yyjson::field in a subschema of a value of the DOM names a key of the object, and passes a value of another kind
struct DomConditions
{
    yyjson::value v;
    std::optional<yyjson::object> o;
    std::vector<yyjson::value> e;
    std::optional<std::map<std::string, yyjson::value>> m;
};
template <>
inline constexpr auto yyjson::annotations<DomConditions> = yyjson::annotate(
    yyjson::field("m", yyjson::additional_properties(yyjson::required("id"))),
    yyjson::field("v", yyjson::not_(yyjson::required("bad"))),
    // an absent key satisfies yyjson::field, so that yyjson::if_ tests it with yyjson::required
    yyjson::field(
        "o", yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("x"))),
                                  yyjson::then(yyjson::required("x"), yyjson::property("x", yyjson::minimum(1))))),
    yyjson::field("e", yyjson::items(yyjson::required("id"), yyjson::property("id", yyjson::minimum(0)))));

// the keywords on a value of several JSON kinds, each of which constrains only the values of its kind
enum class MultiShade
{
    light,
    dark
};
struct MultiKind
{
    yyjson::value v;
    std::variant<std::monostate, int, std::string> w;
    std::variant<MultiShade, int> s;
    yyjson::value list;
    std::vector<yyjson::value> elements;
    std::variant<std::vector<int>, std::map<std::string, int>, bool> c;
    std::variant<double, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<MultiKind> = yyjson::annotate(
    yyjson::field("v", yyjson::enum_(1, "x", nullptr), yyjson::max_length(3)),
    yyjson::field("w", yyjson::minimum(0), yyjson::max_length(2)), yyjson::field("s", yyjson::enum_("dark", 3)),
    yyjson::field("list", yyjson::min_items(2), yyjson::pattern("^a"), yyjson::max_properties(1),
                  yyjson::items(yyjson::minimum(0))),
    yyjson::field("elements", yyjson::items(yyjson::exclusive_maximum(2), yyjson::min_length(1))),
    yyjson::field("c", yyjson::max_items(1), yyjson::min_properties(1)),
    yyjson::field("u", yyjson::not_(yyjson::const_(1)), yyjson::enum_(1, 2.5, "a")));
// the keywords of arrays on a struct read as an array count and check the elements of the JSON array
struct MultiKindPair
{
    int a;
    std::optional<int> b;
    std::vector<int> rest;
    bool operator==(const MultiKindPair&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindPair> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items));
struct MultiKindWritten
{
    std::variant<MultiKindPair, std::string> p;
    MultiKindPair q;
    bool operator==(const MultiKindWritten&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindWritten> =
    yyjson::annotate(yyjson::field("p", yyjson::min_items(2), yyjson::max_items(3)),
                     yyjson::field("q", yyjson::max_items(2), yyjson::contains(yyjson::minimum(5))));

// the settings of the keys that an object has, by the keys of the fields: "b" is the member of the key "bb", and
// "s" is skipped
struct PresenceRenamed
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> s;
};
struct PresenceSkipped
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> s;
};
struct PresenceKnown
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> s;
};
template <>
inline constexpr auto yyjson::annotations<PresenceRenamed> = yyjson::annotate(
    yyjson::field("b", yyjson::rename("bb")), yyjson::one_of(yyjson::required("a"), yyjson::required("b")));
template <>
inline constexpr auto yyjson::annotations<PresenceSkipped> =
    yyjson::annotate(yyjson::field("s", yyjson::skip), yyjson::dependent_required("a", {"s"}));
template <>
inline constexpr auto yyjson::annotations<PresenceKnown> = yyjson::annotate(
    yyjson::field("b", yyjson::rename("bb")), yyjson::any_of(yyjson::required("a"), yyjson::required("bb", "s")));
// a key of `yyjson::property` and `yyjson::required` inside the value of a field is a key of the struct of that value
struct PresenceNestedKnown
{
    PresenceKnown v;
};
template <>
inline constexpr auto yyjson::annotations<PresenceNestedKnown> =
    yyjson::annotate(yyjson::field("v", yyjson::required("bb"), yyjson::property("a", yyjson::minimum(0))));
// a closed subschema of `yyjson::all_of` of a struct names each key that the struct requires
struct ClosedAllOfKeys
{
    int id = 0;
    std::optional<int> name;
};
template <>
inline constexpr auto yyjson::annotations<ClosedAllOfKeys> =
    yyjson::annotate(yyjson::all_of(yyjson::required("id"), yyjson::additional_properties(false)));
struct ClosedAllOfField
{
    int id = 0;
    std::optional<int> name;
};
template <>
inline constexpr auto yyjson::annotations<ClosedAllOfField> = yyjson::annotate(
    yyjson::field("id", yyjson::required),
    yyjson::all_of({yyjson::property("id"), yyjson::property("name"), yyjson::additional_properties(false)}));
// `yyjson::all_of` in `yyjson::all_of` of a struct that only requires keys is no condition
struct NestedAllOf
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<NestedAllOf> =
    yyjson::annotate(yyjson::all_of(yyjson::all_of(yyjson::required("a"))));
// `yyjson::all_of` in `yyjson::all_of` of a struct that also gives `yyjson::property` keeps it
struct NestedAllOfProperty
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<NestedAllOfProperty> =
    yyjson::annotate(yyjson::all_of(yyjson::all_of(yyjson::property("b", yyjson::minimum(1)))));
struct NestedAllOfMixed
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<NestedAllOfMixed> = yyjson::annotate(
    yyjson::all_of(yyjson::all_of({yyjson::required("a"), yyjson::property("b", yyjson::minimum(1))})));
struct NestedAllOfDeep
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<NestedAllOfDeep> = yyjson::annotate(
    yyjson::all_of(yyjson::all_of(yyjson::all_of(yyjson::required("a")), yyjson::property("b", yyjson::minimum(1)))));
struct NestedAllOfInner
{
    std::optional<int> c;
};
struct NestedAllOfObject
{
    int a = 0;
    std::optional<NestedAllOfInner> b;
};
template <>
inline constexpr auto yyjson::annotations<NestedAllOfObject> =
    yyjson::annotate(yyjson::all_of(yyjson::all_of(yyjson::property("b", yyjson::required("c")))));
// a group of `yyjson::required` with more keys than a mask of 8 bits holds, and than 16
struct ManyKeys
{
    std::optional<int> a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, z;
    bool operator==(const ManyKeys&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ManyKeys> = yyjson::annotate(yyjson::one_of(
    yyjson::required("a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"),
    yyjson::required("z")));
// the settings name any key with the map of `yyjson::extra_keys`, whose member it is
struct PresenceExtra
{
    std::optional<int> a;
    std::map<std::string, int> s;
};
template <>
inline constexpr auto yyjson::annotations<PresenceExtra> =
    yyjson::annotate(yyjson::field("s", yyjson::extra_keys), yyjson::dependent_required("a", {"s"}));
struct ConditionExtra
{
    std::optional<int> a;
    std::map<std::string, int> s;
};
template <>
inline constexpr auto yyjson::annotations<ConditionExtra> = yyjson::annotate(
    yyjson::field("s", yyjson::extra_keys), yyjson::not_(yyjson::property("s", yyjson::min_properties(1))));
// and with a map of `yyjson::pattern_properties`
struct PresencePattern
{
    std::optional<int> a;
    std::map<std::string, int> s;
};
template <>
inline constexpr auto yyjson::annotations<PresencePattern> =
    yyjson::annotate(yyjson::field("s", yyjson::pattern_properties("^s")), yyjson::dependent_required("a", {"s"}));
struct ConditionPattern
{
    std::optional<int> a;
    std::map<std::string, int> s;
};
template <>
inline constexpr auto yyjson::annotations<ConditionPattern> =
    yyjson::annotate(yyjson::field("s", yyjson::pattern_properties("^s")),
                     yyjson::not_(yyjson::property("s", yyjson::min_properties(1))));

// The members of an object that the maps of `yyjson::pattern_properties` receive by the patterns of their keys
struct PatternMaps
{
    int ab = 10;
    std::map<std::string, int> a;
    std::map<std::string, int> b;
    std::map<std::string, std::string> s;
    bool operator==(const PatternMaps&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternMaps> = yyjson::annotate(
    yyjson::field("a", yyjson::pattern_properties("^a"), yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("b", yyjson::pattern_properties("b$"), yyjson::additional_properties(yyjson::minimum(10))),
    yyjson::field("s", yyjson::pattern_properties("^s")));
// the keys that no pattern matches go to the field of `yyjson::extra_keys`
struct PatternRest
{
    std::unordered_map<std::string, int> a;
    std::map<std::string, bool> rest;
    bool operator==(const PatternRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternRest> =
    yyjson::annotate(yyjson::field("a", yyjson::pattern_properties("^a")), yyjson::field("rest", yyjson::extra_keys));
// a pattern reads a value that it does not keep under reject_missing_keys and reject_null_as_missing
struct PatternPoint
{
    int x = 0;
    bool operator==(const PatternPoint&) const = default;
};
struct PatternStrict
{
    std::map<std::string, PatternPoint> a;
    std::map<std::string, PatternPoint> b;
    std::map<std::string, yyjson::value> v;
    bool operator==(const PatternStrict&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternStrict> = yyjson::annotate(
    yyjson::field("a", yyjson::pattern_properties("^a")), yyjson::field("b", yyjson::pattern_properties("b$")),
    yyjson::field("v", yyjson::pattern_properties("^v")));
// a pattern that keeps the value as JSON, and a later one that reads it again
struct PatternInner
{
    int x = 0;
};
struct PatternOuter
{
    PatternInner in;
};
struct PatternReread
{
    std::map<std::string, yyjson::value> v;
    std::map<std::string, PatternOuter> n;
};
template <>
inline constexpr auto yyjson::annotations<PatternReread> = yyjson::annotate(
    yyjson::field("v", yyjson::pattern_properties("^v")), yyjson::field("n", yyjson::pattern_properties("n$")));
// a declared field that a merge merges in place, and a pattern that matches its key
struct PatternLoose
{
    yyjson::value in;
};
struct PatternDeclared
{
    PatternLoose vn;
    std::map<std::string, PatternOuter> n;
};
template <>
inline constexpr auto yyjson::annotations<PatternDeclared> =
    yyjson::annotate(yyjson::field("n", yyjson::pattern_properties("n$")));
struct PatternClosed
{
    std::map<std::string, yyjson::value> x;
    bool operator==(const PatternClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternClosed> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("x", yyjson::pattern_properties("^x")));
struct PatternOpen
{
    int a = 0;
    std::map<std::string, int> p;
    bool operator==(const PatternOpen&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternOpen> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^p")));
// a field whose null reads as a missing key, and a map whose values hold null
struct PatternOptional
{
    std::optional<int> ab;
    std::map<std::string, int> a;
    std::map<std::string, std::optional<int>> n;
    bool operator==(const PatternOptional&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternOptional> = yyjson::annotate(
    yyjson::field("a", yyjson::pattern_properties("^a"), yyjson::additional_properties(yyjson::minimum(10))),
    yyjson::field("n", yyjson::pattern_properties("^n")));
struct PatternSeeded
{
    int x = 0;
    std::map<std::string, int> a = {{"aseed", 1}};
    bool operator==(const PatternSeeded&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternSeeded> =
    yyjson::annotate(yyjson::field("a", yyjson::pattern_properties("^a")));
// the keywords of the struct count and check the keys that the maps receive
struct PatternCounted
{
    int x = 0;
    std::map<std::string, int> a;
    bool operator==(const PatternCounted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternCounted> = yyjson::annotate(
    yyjson::min_properties(2), yyjson::max_properties(3), yyjson::property_names(yyjson::max_length(3)),
    yyjson::field("a", yyjson::pattern_properties("^a")));
// additional_properties(false) in a subschema takes no key of a pattern, which the subschema does not give
struct PatternConditional
{
    std::optional<std::string> kind;
    std::map<std::string, int> p;
    bool operator==(const PatternConditional&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternConditional> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("closed"))),
                         yyjson::then(yyjson::property("kind"), yyjson::additional_properties(false))),
    yyjson::field("p", yyjson::pattern_properties("^p")));
// a pattern that matches the key of a tag checks the value of the tag
struct PatternTagShort
{
    int x = 0;
    std::map<std::string, std::string> m;
    bool operator==(const PatternTagShort&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternTagShort> = yyjson::annotate(
    yyjson::field("m", yyjson::pattern_properties("^k"), yyjson::additional_properties(yyjson::max_length(1))));
struct PatternTagInt
{
    int x = 0;
    std::map<std::string, int> m;
    bool operator==(const PatternTagInt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternTagInt> =
    yyjson::annotate(yyjson::field("m", yyjson::pattern_properties("^k")));
struct PatternTagOther
{
    int y = 0;
    bool operator==(const PatternTagOther&) const = default;
};
using PatternTagged = std::variant<PatternTagShort, PatternTagOther>;
template <>
inline constexpr auto yyjson::annotations<PatternTagged> =
    yyjson::annotate(yyjson::internal_tag("kind", {"s", "longer"}));
using PatternTaggedLong = std::variant<PatternTagOther, PatternTagShort>;
template <>
inline constexpr auto yyjson::annotations<PatternTaggedLong> =
    yyjson::annotate(yyjson::internal_tag("kind", {"o", "st"}));
using PatternTaggedShared = std::variant<std::shared_ptr<PatternTagInt>, PatternTagOther>;
template <>
inline constexpr auto yyjson::annotations<PatternTaggedShared> =
    yyjson::annotate(yyjson::internal_tag("kind", {"i", "o"}));
struct PatternAdjacent
{
    std::variant<int, std::string> data;
    std::map<std::string, int> m;
    bool operator==(const PatternAdjacent&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternAdjacent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})),
                     yyjson::field("m", yyjson::pattern_properties("^k")));
// the struct inside other types
struct PatternHolder
{
    std::vector<PatternMaps> v;
    std::map<std::string, PatternMaps> m;
    std::optional<PatternMaps> o;
    bool operator==(const PatternHolder&) const = default;
};
// `object_emitter::emplace_members` writes the maps as the struct does
struct PatternEmitted
{
    PatternMaps maps;
};
template <>
struct yyjson::caster<PatternEmitted>
{
    static void to_json(yyjson::object_emitter& out, const PatternEmitted& e)
    {
        out.emplace("first", true);
        out.emplace_members(e.maps);
    }
};
#if FIELD_RFL_ANNOTATIONS
struct PatternAnnotated
{
    int a = 0;
    // clang-format off
    [[= yyjson::pattern_properties("^x"), = yyjson::additional_properties(yyjson::minimum(0))]]
    std::map<std::string, int> xs;
    // clang-format on
    bool operator==(const PatternAnnotated&) const = default;
};
#endif

// additional_properties(false) in then and else_: a circle has only a kind and a radius, another shape only a kind
// and a width
struct ConditionClosed
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<int> w;
};
template <>
inline constexpr auto yyjson::annotations<ConditionClosed> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("circle"))),
    yyjson::then(yyjson::property("kind"), yyjson::required("r"), yyjson::property("r", yyjson::minimum(0)),
                 yyjson::additional_properties(false)),
    yyjson::else_(yyjson::property("kind"), yyjson::property("w"), yyjson::additional_properties(false))));
// the keys of tags are allowed: an adjacent tag, and the internal tag of an alternative
struct ClosedTagged
{
    std::variant<int, std::string> data;
    std::optional<int> n;
};
template <>
inline constexpr auto yyjson::annotations<ClosedTagged> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})),
                     yyjson::if_then_else(yyjson::if_(yyjson::required("n")),
                                          yyjson::then(yyjson::property("n"), yyjson::property("data"),
                                                       yyjson::additional_properties(false))));
struct ClosedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<ClosedAlternative> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("a")), yyjson::then(yyjson::property("a"), yyjson::additional_properties(false))));
using ClosedAlternatives = std::variant<ClosedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<ClosedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"c", "p"}));

// yyjson::min_properties and yyjson::max_properties in the conditions count the keys of the object
struct ConditionCounted
{
    std::optional<std::string> kind;
    std::optional<int> a;
};
template <>
inline constexpr auto yyjson::annotations<ConditionCounted> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("pair"))),
                         yyjson::then(yyjson::min_properties(3)), yyjson::else_(yyjson::max_properties(2))),
    yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2), yyjson::max_properties(2)),
                         yyjson::then(yyjson::required("a"))),
    yyjson::not_(yyjson::min_properties(5)));
// the internal tag of an alternative is a key of its object
struct CountedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<CountedAlternative> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2)), yyjson::then(yyjson::required("b"))));
using CountedAlternatives = std::variant<CountedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<CountedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"c", "p"}));
// yyjson::property_names in a condition
struct ConditionNamed
{
    std::optional<std::string> kind;
};
template <>
inline constexpr auto yyjson::annotations<ConditionNamed> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("short"))),
                         yyjson::then(yyjson::property_names(yyjson::max_length(4)))));

// yyjson::one_of and yyjson::any_of of yyjson::required("key", ...) in the conditions require the keys of groups
struct ConditionGrouped
{
    std::optional<std::string> kind;
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<ConditionGrouped> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("pair"))),
    yyjson::then(yyjson::property("c", yyjson::maximum(9)),
                 yyjson::one_of(yyjson::required("a"), yyjson::required("b"))),
    yyjson::else_(yyjson::any_of(yyjson::required("a", "c"), yyjson::required("b")), yyjson::max_properties(2))));
// the internal tag of an alternative is not a member of a group
struct GroupedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<GroupedAlternative> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2)),
                                          yyjson::then(yyjson::any_of(yyjson::required("a"), yyjson::required("b")))));
using GroupedAlternatives = std::variant<GroupedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<GroupedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"g", "p"}));

// the keywords of an object on a struct that a field, an element, a value of a map and an alternative hold
struct Positioned
{
    std::optional<int> a;
    std::optional<int> b;
};
struct PositionedOther
{
    std::optional<int> a;
    int x = 0;
};
using PositionedAlternatives = std::variant<Positioned, PositionedOther>;
template <>
inline constexpr auto yyjson::annotations<PositionedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"p", "o"}));
struct Positions
{
    std::optional<Positioned> field;
    std::vector<Positioned> list;
    std::map<std::string, Positioned> map;
    std::optional<PositionedAlternatives> alternative;
    std::optional<Positioned> groups;
    std::optional<Positioned> conditional;
};
template <>
inline constexpr auto yyjson::annotations<Positions> = yyjson::annotate(
    yyjson::field("field", yyjson::min_properties(1), yyjson::required("a"), yyjson::property("a", yyjson::maximum(5)),
                  yyjson::additional_properties(false)),
    yyjson::field("list", yyjson::items(yyjson::max_properties(1), yyjson::property_names(yyjson::max_length(1)))),
    yyjson::field("map", yyjson::additional_properties(yyjson::not_(yyjson::min_properties(2)))),
    yyjson::field("alternative", yyjson::additional_properties(false), yyjson::property("a", yyjson::minimum(0))),
    yyjson::field("groups", yyjson::one_of(yyjson::required("a"), yyjson::required("b"))),
    yyjson::field("conditional",
                  yyjson::if_then_else(yyjson::if_(yyjson::required("a")),
                                       yyjson::then(yyjson::property("a"), yyjson::additional_properties(false)),
                                       yyjson::else_(yyjson::dependent_required("b", {"a"})))));

namespace struct_condition_rules
{
    // the settings that a condition of a struct takes, which the types alone tell
    template <typename... As>
    concept conditions_accepted = requires { yyjson::not_(std::declval<As>()...); };
    static_assert(conditions_accepted<decltype(yyjson::field("k", yyjson::required, yyjson::minimum(0)))>);
    static_assert(yyjson::detail::has_struct_conditions<ConditionShape> &&
                  yyjson::detail::has_struct_conditions<ConditionVisitable> &&
                  !yyjson::detail::has_struct_conditions<Bounded> && !yyjson::detail::has_constraints<Contact>);

    // `yyjson::dependent_required` and `yyjson::required("key", ...)` name the keys of the fields, each once
    template <int K>
    consteval auto make_presence()
    {
        if constexpr (K == 0)
            return yyjson::dependent_required("a", {"b", "c"});
        else if constexpr (K == 1)
            return yyjson::dependent_required("a", {"b", "b"});
        else if constexpr (K == 2)
            return yyjson::dependent_required("a", {"b", "a"});
        else if constexpr (K == 3)
            return yyjson::dependent_required("a", {});
        else if constexpr (K == 4)
            return yyjson::one_of(yyjson::required("a"), yyjson::required("a", "b"));
        else if constexpr (K == 5)
            return yyjson::one_of(yyjson::required("a"), yyjson::required("b", "c", "b"));
        else
            return yyjson::any_of(yyjson::required("a", "a"));
    }
    template <int K>
    concept presence_made = requires { typename std::integral_constant<bool, (make_presence<K>(), true)>; };
    static_assert(presence_made<0> && !presence_made<1> && !presence_made<2> && !presence_made<3> && presence_made<4>);
    // a string literal given twice is an error of the tree before C++26, and a std::string_view of the call
#if defined(__cpp_lib_define_static)
    static_assert(!presence_made<5> && !presence_made<6>);
#else
    static_assert(presence_made<5> && presence_made<6>);
#endif
    template <typename S>
    concept keys_made = requires(S s) { yyjson::one_of(yyjson::required(s), yyjson::required(s, s)); };
    static_assert(keys_made<std::string_view> && keys_made<std::string>);
    // a key that no field has, which a renamed or skipped member does not
    template <typename T>
    constexpr auto presence_error =
        yyjson::detail::check_struct_tree<T>(yyjson::detail::make_schema_tree<&yyjson::detail::build_struct_tree<T>>());
    using enum yyjson::detail::struct_condition_error;
    static_assert(presence_error<PresenceRenamed> == unknown_key && presence_error<PresenceSkipped> == unknown_key &&
                  presence_error<PresenceKnown> == none);
    static_assert(presence_error<PresenceExtra> == none && presence_error<ConditionExtra> == none);
    static_assert(presence_error<PresencePattern> == none && presence_error<ConditionPattern> == none);
    static_assert(presence_error<PresenceNestedKnown> == none);
    static_assert(presence_error<ClosedAllOfKeys> == closed_required && presence_error<ClosedAllOfField> == none);
    static_assert(!yyjson::detail::has_struct_conditions<NestedAllOf> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfProperty> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfMixed> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfDeep> &&
                  yyjson::detail::has_struct_conditions<NestedAllOfObject>);
    static_assert(yyjson::detail::has_struct_conditions<PresenceKnown> &&
                  !yyjson::detail::has_constraints<PresenceKnown>);
    // the keywords of an object at a position of a struct, which a map, a struct written as an array and a value
    // of another type do not take as a struct
    using yyjson::detail::holds_object_struct, yyjson::detail::object_struct_value;
    static_assert(object_struct_value<Positioned>() && holds_object_struct<PositionedAlternatives>() &&
                  holds_object_struct<ClosedAlternatives>());
    static_assert(!object_struct_value<int>() && !object_struct_value<std::map<std::string, int>>() &&
                  !object_struct_value<std::vector<Positioned>>() && !object_struct_value<Scores>() &&
                  !holds_object_struct<std::variant<int, std::string>>() && !object_struct_value<yyjson::object>());
    using yyjson::detail::field_schema, yyjson::detail::struct_object_node;
    static_assert(field_schema<Positions, 0>::node.closed && field_schema<Positions, 3>::node.closed &&
                  !struct_object_node<field_schema<Positions, 1>> && !struct_object_node<field_schema<Positions, 4>>);
    // they are settings of a subschema and of a field too, as a struct at any position takes them
    static_assert(
        yyjson::detail::subschema_setting<decltype(yyjson::one_of(yyjson::required("a"), yyjson::required("b")))> &&
        yyjson::detail::subschema_setting<decltype(yyjson::dependent_required("a", {"b"}))> &&
        yyjson::detail::field_setting<decltype(yyjson::any_of(yyjson::required("a")))>);
}  // namespace struct_condition_rules

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

// the keywords of a field that a caster reads check the JSON value
struct CasterAnnotated
{
    [[ = yyjson::max_items(2), = yyjson::unique_items ]] CasterList list{};
    [[= yyjson::max_length(3)]] CasterName name{};
    bool operator==(const CasterAnnotated&) const = default;
};

struct ElementsAnnotated
{
    [[= yyjson::items(yyjson::minimum(0))]] std::vector<int> scores{};
    [[ = yyjson::unique_items, = yyjson::min_items(1) ]] std::vector<int> ids{1};
    [[= yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(1)})]] std::tuple<int, std::string> row{};
    [[= yyjson::property_names(yyjson::pattern("^[a-z]+$"))]] std::map<std::string, int> names{};
    [[ = yyjson::contains(yyjson::const_("x")), = yyjson::max_contains(1) ]] std::vector<std::string> tags{"x"};
    [[ = yyjson::additional_properties(yyjson::items(yyjson::maximum(9))),
       = yyjson::max_properties(1) ]] std::map<std::string, std::vector<int>>
        groups{};
    bool operator==(const ElementsAnnotated&) const = default;
};

// the conditions of a struct as annotations of the struct: a or b, not both, and a key renamed by its member
struct[[
    = yyjson::not_(yyjson::required("first"), yyjson::required("b")),
    = yyjson::any_of(yyjson::required("first"), yyjson::property("b", yyjson::minimum(0)))
]] ExclusiveAnnotated
{
    [[= yyjson::rename("first")]] std::optional<int> a{};
    std::optional<int> b{};
    bool operator==(const ExclusiveAnnotated&) const = default;
};

struct CombinedAnnotated
{
    [[= yyjson::not_(yyjson::const_(0))]] int n = 1;
    [[= yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)))]] int even = 0;
    [[= yyjson::all_of(yyjson::minimum(0), {yyjson::maximum(9)})]] int digit = 0;
    [[= yyjson::any_of(yyjson::const_("a"), yyjson::pattern("^b"))]] std::string s = "a";
    [[= yyjson::one_of(yyjson::minimum(5), yyjson::maximum(7))]] int one = 0;
    [[= yyjson::items(yyjson::matches<int>)]] std::vector<yyjson::value> ints{};
    bool operator==(const CombinedAnnotated& other) const
    {
        return n == other.n && even == other.even && digit == other.digit && s == other.s && one == other.one &&
               std::string(yyjson::write(ints).data()) == std::string(yyjson::write(other.ints).data());
    }
};
#endif

namespace
{
    // `e.what()` without the position of the read, and without the "struct ", "class " and "enum " that MSVC writes
    // before the name of a type
    std::string message_of(const std::exception& e)
    {
        const auto* cast = dynamic_cast<const yyjson::bad_cast*>(&e);
        auto message = cast != nullptr ? what_without_position(*cast) : std::string(e.what());
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

    // `message` about the value at `path`, with the path of the element 0 of an array that holds the value
    inline std::string at_element(std::string_view message, std::string_view path)
    {
        auto text = std::string(message);
        if (!path.empty())
        {
            const auto at = text.find(" at " + std::string(path));
            if (at != std::string::npos) text.insert(at + 4, "/0");
            return text;
        }
        const auto note = text.find("; ");
        text.insert(note == std::string::npos ? text.size() : note, " at /0");
        return text;
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

    // every read of `json` gives `expected`, and the reader without a document reads it unless `direct_read` is false
    template <typename T>
    void expect_accepted(std::string_view json, const T& expected, bool direct_read = true)
    {
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        auto into = T();
        yyjson::read_into(json, into);
        EXPECT_EQ(into, expected) << json;
        // the merge checks the fields of T() that `json` leaves out too
        auto merged = T();
        if (rejected_by_rule(T(), json))
            EXPECT_THROW(yyjson::merge_into(json, merged), yyjson::bad_cast) << json;
        else
        {
            yyjson::merge_into(json, merged);
            EXPECT_EQ(merged, expected) << json;
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_EQ(direct.has_value(), direct_read) << json;
        if (direct)
        {
            EXPECT_EQ(*direct, read_or_error([&]() -> const T& { return expected; })) << json;
        }
        EXPECT_EQ(expect_read_into(T(), json), direct_read) << json;
#endif
    }

    // every read of `json`, with and without a document, throws `bad_cast` with `message` about the value at
    // `path`, and the reader without a document leaves it to the document; `merge_into` too unless `json` is a
    // null that resets the field or T() writes members, and else it follows the merge of `json` into them
    template <typename T>
    void expect_rejected(std::string_view json, const expected_text& message, std::string_view path, bool merged = true)
    {
        merged = merged && writes_no_member<T>();
        static_cast<void>(expect_merge_rule(T(), json));
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
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

    expect_rejected<Bounded>(R"({"i":-1})",
                             R"(the number is less than the minimum 0 at /i; reading the key "i" of Bounded)", "/i");
    expect_rejected<Bounded>(
        R"({"i":11})", R"(the number is greater than the maximum 10 at /i; reading the key "i" of Bounded)", "/i");
    expect_rejected<Bounded>(
        R"({"d":0})", R"(the number is not greater than the exclusive minimum 0 at /d; reading the key "d" of Bounded)",
        "/d");
    expect_rejected<Bounded>(
        R"({"d":1.5})",
        R"(the number is not less than the exclusive maximum 1.5 at /d; reading the key "d" of Bounded)", "/d");
    expect_rejected<Bounded>(R"({"u":4})", R"(the number is not a multiple of 3 at /u; reading the key "u" of Bounded)",
                             "/u");
    expect_rejected<Bounded>(R"({"i":1,"o":-6})",
                             R"(the number is less than the minimum -5 at /o; reading the key "o" of Bounded)", "/o");
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
        R"(the number is greater than the maximum 9007199254740992 at /big; reading the key "big" of MixedBounds)",
        "/big");
    expect_rejected<MixedBounds>(
        R"({"ubig":1})",
        R"(the number is not a multiple of 9223372036854775808 at /ubig; reading the key "ubig" of MixedBounds)",
        "/ubig");
    expect_rejected<MixedBounds>(
        R"({"real":9007199254740994})",
        R"(the number is greater than the maximum 9007199254740993 at /real; reading the key "real" of MixedBounds)",
        "/real");
    expect_rejected<MixedBounds>(
        R"({"negative":-2})",
        R"(the number is not less than the exclusive maximum -2.5 at /negative; reading the key "negative" of MixedBounds)",
        "/negative");
    expect_rejected<MixedBounds>(
        R"({"negative":-6})",
        R"(the number is not a multiple of 7 at /negative; reading the key "negative" of MixedBounds)", "/negative");
    expect_rejected<MixedBounds>(
        R"({"quarter":0.3})",
        R"(the number is not a multiple of 0.25 at /quarter; reading the key "quarter" of MixedBounds)", "/quarter");

    // a multiple of a decimal is decided in decimal, which the quotient in binary is not
    for (const auto* json : {R"({"cents":19.99})", R"({"cents":0.07})", R"({"cents":-1234567.89})", R"({"cents":1e-2})",
                             R"({"tenths":-30})", R"({"tenths":0})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<MixedBounds>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).cast<MixedBounds>()) << json;
    }
    expect_rejected<MixedBounds>(
        R"({"cents":0.001})",
        R"(the number is not a multiple of 0.01 at /cents; reading the key "cents" of MixedBounds)", "/cents");
    expect_rejected<MixedBounds>(
        R"({"cents":19.995})",
        R"(the number is not a multiple of 0.01 at /cents; reading the key "cents" of MixedBounds)", "/cents");
    // an integer field is a multiple of 0.1 whatever it holds
    EXPECT_EQ(yyjson::read<MixedBounds>(R"({"tenths":7})").tenths, 7);
}

TEST(Constraints, FloatBounds)
{
    // the JSON number is compared before the read rounds it to a float
    expect_accepted<FloatBounds>(R"({"tenth":0.1,"under":0.09999999,"big":16777217,"step":0.3})",
                                 {0.1F, 0.09999999F, 16777216.0F, 0.3F});
    for (const auto* json : {R"({"tenth":0.10000001})", R"({"tenth":0.10000000149011612})"})
        expect_rejected<FloatBounds>(
            json, R"(the number is greater than the maximum 0.1 at /tenth; reading the key "tenth" of FloatBounds)",
            "/tenth");
    expect_rejected<FloatBounds>(
        R"({"under":0.1})",
        R"(the number is not less than the exclusive maximum 0.1 at /under; reading the key "under" of FloatBounds)",
        "/under");
    expect_rejected<FloatBounds>(
        R"({"big":16777216})",
        R"(the number is less than the minimum 16777217 at /big; reading the key "big" of FloatBounds)", "/big");
    expect_rejected<FloatBounds>(
        R"({"step":0.30000001})",
        R"(the number is not a multiple of 0.1 at /step; reading the key "step" of FloatBounds)", "/step");

    expect_accepted<FloatElements>(R"({"v":[0.1,0.05],"m":{"a":0.1},"u":0.1})", {{0.1F, 0.05F}, {{"a", 0.1F}}, 0.1F});
    expect_rejected<FloatElements>(
        R"({"v":[0.1,0.10000001]})",
        R"(the number is greater than the maximum 0.1 at /v/1; reading an element of the value at the key "v" of FloatElements)",
        "/v/1");
    expect_rejected<FloatElements>(
        R"({"m":{"a":0.1,"b":0.10000001}})",
        R"(the number is greater than the maximum 0.1 at /m/b; reading an element of the value at the key "m" of FloatElements)",
        "/m/b");
    expect_rejected<FloatElements>(
        R"({"u":0.10000001})",
        R"(the number is greater than the maximum 0.1 at /u; reading the key "u" of FloatElements)", "/u");
}

TEST(Constraints, RealBounds)
{
    // the JSON integer is compared before the read rounds it to a double
    expect_rejected<RealBounds>(
        R"({"top":9007199254740993})",
        R"(the number is greater than the maximum 9007199254740992 at /top; reading the key "top" of RealBounds)",
        "/top");
    expect_rejected<RealBounds>(
        R"({"wide":-9007199254740993})",
        R"(the number is not greater than the exclusive minimum -9007199254740992 at /wide; reading the key "wide" of RealBounds)",
        "/wide");
    // the text of a real is read as a double before the comparison
    expect_accepted<RealBounds>(R"({"top":9007199254740992,"wide":-9007199254740991})",
                                {9007199254740992.0, 0, 0, 0, 0, 0, -9007199254740991.0L});
    expect_accepted<RealBounds>(R"({"top":9007199254740993.0})", {9007199254740992.0});

    // a decimal multiple is decided on the shortest decimal of the double
    expect_accepted<RealBounds>(R"({"tenth":0.3,"cent":-1234567.89,"half":5,"third":4.5,"three":9007199254740993})",
                                {0, 0.3, -1234567.89, 5, 4.5, 9007199254740992.0});
    for (const auto* json : {R"({"tenth":-0.3})", R"({"tenth":1e300})", R"({"tenth":0.1e-0})"})
        EXPECT_NO_THROW((void)yyjson::read<RealBounds>(json)) << json;
    for (const auto* json : {R"({"tenth":0.30000000000000004})", R"({"tenth":0.35})", R"({"tenth":1e-300})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 0.1 at /tenth; reading the key "tenth" of RealBounds)", "/tenth");
    expect_rejected<RealBounds>(
        R"({"cent":0.001})", R"(the number is not a multiple of 0.01 at /cent; reading the key "cent" of RealBounds)",
        "/cent");
    for (const auto* json : {R"({"third":4.0})", R"({"third":4})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 1.5 at /third; reading the key "third" of RealBounds)", "/third");
    for (const auto* json : {R"({"three":9007199254740992})", R"({"three":1e300})"})
        expect_rejected<RealBounds>(
            json, R"(the number is not a multiple of 3 at /three; reading the key "three" of RealBounds)", "/three");

    expect_accepted<RealElements>(R"({"v":[9007199254740992],"u":0.7})", {{9007199254740992.0}, 0.7});
    expect_rejected<RealElements>(
        R"({"v":[0,9007199254740993]})",
        R"(the number is greater than the maximum 9007199254740992 at /v/1; reading an element of the value at the key "v" of RealElements)",
        "/v/1");
    expect_rejected<RealElements>(R"({"u":0.7000000000000001})",
                                  R"(the number is not a multiple of 0.1 at /u; reading the key "u" of RealElements)",
                                  "/u");
}

TEST(Constraints, WideDivisors)
{
    // an integer divisor divides the JSON integer exactly
    for (const auto* json : {R"({"big":9007199254740993})", R"({"big":-9007199254740993})"})
        expect_rejected<WideDivisors>(
            json, R"(the number is not a multiple of 9007199254740992 at /big; reading the key "big" of WideDivisors)",
            "/big");
    expect_rejected<WideDivisors>(
        R"({"ubig":9007199254740993})",
        R"(the number is not a multiple of 9007199254740992 at /ubig; reading the key "ubig" of WideDivisors)",
        "/ubig");
    expect_accepted<WideDivisors>(R"({"big":18014398509481984,"ubig":18014398509481984})",
                                  {18014398509481984.0, 18014398509481984U});
    // a real divisor takes its shortest decimal, with any exponent
    expect_accepted<WideDivisors>(R"({"tiny":3e-18,"odd":900719925474098.8})", {0, 0, 3e-18, 900719925474098.8});
    expect_accepted<WideDivisors>(R"({"tiny":1e300})", {0, 0, 1e300});
    expect_rejected<WideDivisors>(
        R"({"tiny":3.5e-18})",
        R"(the number is not a multiple of 1e-18 at /tiny; reading the key "tiny" of WideDivisors)", "/tiny");
    expect_rejected<WideDivisors>(
        R"({"odd":900719925474098.9})",
        R"(the number is not a multiple of 900719925474098.8 at /odd; reading the key "odd" of WideDivisors)", "/odd");
    // no integer of 64 bits but 0 is a multiple of 1e20
    expect_accepted<WideDivisors>(R"({"huge":0})", {});
    expect_rejected<WideDivisors>(
        R"({"huge":9223372036854775807})",
        R"(the number is not a multiple of 1e+20 at /huge; reading the key "huge" of WideDivisors)", "/huge");
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

    expect_rejected<Texts>(
        R"({"name":"a"})",
        R"(the string is shorter than the minimum length 2 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"日本語x"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of Texts)", "/name");
    expect_rejected<Texts>(
        R"({"name":"ab","code":"abc"})",
        R"(the string does not match the pattern "^[A-Z]{3}$" at /code; reading the key "code" of Texts)", "/code");
    expect_rejected<Texts>(R"({"name":"ab","note":"x"})",
                           R"(the string does not match the pattern "^n" at /note; reading the key "note" of Texts)",
                           "/note");
}

TEST(Constraints, Patterns)
{
    expect_accepted<Patterns>(R"({"s":"abz","o":"oxo","v":"123"})", {"abz", "oxo", "123"});
    expect_accepted<Patterns>(R"({"s":"az"})", {"az", {}, {}});

    expect_rejected<Patterns>(
        R"({"s":"ab"})",
        R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)", "/s");
    expect_rejected<Patterns>(
        R"({"s":"bz"})",
        R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)", "/s");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"oo"})",
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"xo"})",
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","o":"o"})",
        R"(the string is shorter than the minimum length 2 at /o; reading the key "o" of Patterns)", "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","v":"1a"})",
        R"(the string does not match the pattern "^[0-9]*$" at /v; reading the key "v" of Patterns)", "/v");

    // validate and the checked write check them as well
    expect_valid(Patterns{"abz", "oxo", "123"});
    expect_invalid(Patterns{"ab", {}, {}},
                   R"(the string does not match all of the patterns "^a", "z$" at /s; reading the key "s" of Patterns)",
                   "/s");
    expect_invalid(
        Patterns{"az", "oxx", {}},
        R"(the string does not match all of the patterns "^o", "o$", "x" at /o; reading the key "o" of Patterns)",
        "/o");
    expect_invalid(Patterns{"az", "o", {}},
                   R"(the string is shorter than the minimum length 2 at /o; reading the key "o" of Patterns)", "/o");
    expect_invalid(Patterns{"az", {}, "1a"},
                   R"(the string does not match the pattern "^[0-9]*$" at /v; reading the key "v" of Patterns)", "/v");
}

TEST(Constraints, Satisfies)
{
    expect_accepted<Validated>(R"({"even":2,"text":"ok","nonzero":null})", {2, "ok", {}});
    expect_accepted<Validated>(R"({"nonzero":1})", {0, {}, 1});

    expect_rejected<Validated>(
        R"({"even":1})", R"(the value does not satisfy the predicate at /even; reading the key "even" of Validated)",
        "/even");
    expect_rejected<Validated>(
        R"({"text":"bad"})",
        R"(the value does not satisfy the predicate at /text; reading the key "text" of Validated)", "/text");
    expect_rejected<Validated>(
        R"({"nonzero":0})",
        R"(the value does not satisfy the predicate at /nonzero; reading the key "nonzero" of Validated)", "/nonzero");
}

TEST(Constraints, Structs)
{
    expect_accepted<ConstrainedVisitable>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedVisitable>(
        R"({"a":0})", R"(the number is less than the minimum 1 at /a; reading the key "a" of ConstrainedVisitable)",
        "/a");
    expect_rejected<ConstrainedVisitable>(
        R"({"b":"xy"})",
        R"(the string is longer than the maximum length 1 at /b; reading the key "b" of ConstrainedVisitable)", "/b");

    expect_accepted<ConstrainedLarge>(R"({"i":1,"r":0,"a":5})", {5, 0, 0, 0, 0, 0, 0, 0, 1, 0});
    expect_rejected<ConstrainedLarge>(
        R"({"a":1,"i":2})",
        R"(the number is greater than the maximum 1 at /i; reading the key "i" of ConstrainedLarge)", "/i");
    expect_rejected<ConstrainedLarge>(
        R"({"r":-1})", R"(the number is less than the minimum 0 at /r; reading the key "r" of ConstrainedLarge)", "/r");

    expect_rejected<ConstrainedOuter>(
        R"({"items":[{"i":1},{"i":12}]})",
        R"(the number is greater than the maximum 10 at /items/1/i; reading the key "i" of Bounded)", "/items/1/i");
    expect_rejected<ConstrainedOuter>(
        R"({"texts":{"name":"x"}})",
        R"(the string is shorter than the minimum length 2 at /texts/name; reading the key "name" of Texts)",
        "/texts/name");
    // the position of the value is added
    const auto e = bad_cast_of([] { (void)yyjson::read<ConstrainedOuter>(R"({"items":[{"i":-3}]})"); });
    ASSERT_TRUE(e.position().has_value());
    EXPECT_EQ(e.position()->pos, 15);

    // the fields are checked by their own constraints whatever their places in the struct
    expect_accepted<ConstrainedSkipped>(R"({"a":5,"s":"xy","c":1})", {0, 5, "xy", 1, 5});
    expect_rejected<ConstrainedSkipped>(
        R"({"a":6})", R"(the number is greater than the maximum 5 at /a; reading the key "a" of ConstrainedSkipped)",
        "/a");
    expect_rejected<ConstrainedSkipped>(
        R"({"a":1,"s":"x"})",
        R"(the string is shorter than the minimum length 2 at /s; reading the key "s" of ConstrainedSkipped)", "/s");
    expect_rejected<ConstrainedSkipped>(
        R"({"c":0})", R"(the number is less than the minimum 1 at /c; reading the key "c" of ConstrainedSkipped)",
        "/c");
    expect_rejected<ConstrainedSkipped>(
        R"({"o":4})", R"(the number is greater than the maximum 3 at /o; reading the key "o" of ConstrainedSkipped)",
        "/o");
    expect_accepted<ConstrainedReordered>(R"({"b":"x","a":1})", {1, "x"});
    expect_rejected<ConstrainedReordered>(
        R"({"b":"xy","a":1})",
        R"(the string is longer than the maximum length 1 at /b; reading the key "b" of ConstrainedReordered)", "/b");
    expect_rejected<ConstrainedReordered>(
        R"({"b":"x","a":0})",
        R"(the number is less than the minimum 1 at /a; reading the key "a" of ConstrainedReordered)", "/a");

    // null leaves a field at its value in `T()`, which is not checked
    expect_accepted<ConstrainedSkipped>(R"({"o":null})", {});
    expect_accepted<ConstrainedReordered>(R"({"o":null})", {});
    expect_rejected<ConstrainedReordered>(
        R"({"o":4})", R"(the number is greater than the maximum 3 at /o; reading the key "o" of ConstrainedReordered)",
        "/o");
    EXPECT_THROW((void)yyjson::read<ConstrainedSkipped>(R"({"o":null})", yyjson::strict), yyjson::bad_cast);

    // a default value that the key leaves is not checked, and a value that the key gives is
    expect_accepted<ConstrainedDefaults>("{}", {});
    expect_accepted<ConstrainedDefaults>(R"({"o":null})", {});
    expect_accepted<ConstrainedDefaults>(R"({"a":0,"o":0})", {0, 0});
    expect_rejected<ConstrainedDefaults>(
        R"({"a":-2})", R"(the number is less than the minimum 0 at /a; reading the key "a" of ConstrainedDefaults)",
        "/a");
    expect_rejected<ConstrainedDefaults>(
        R"({"o":-2})", R"(the number is less than the minimum 0 at /o; reading the key "o" of ConstrainedDefaults)",
        "/o");
}

TEST(Constraints, Transparent)
{
    expect_accepted<Percent>("50", {50});
    expect_rejected<Percent>("101", "the number is greater than the maximum 100; reading Percent", "");
    expect_accepted<Progress>(R"({"done":100,"steps":[0,100]})", {{100}, {{0}, {100}}});
    expect_rejected<Progress>(R"({"done":-1})", "the number is less than the minimum 0 at /done; reading Percent",
                              "/done");
    expect_rejected<Progress>(R"({"steps":[1,200]})",
                              "the number is greater than the maximum 100 at /steps/1; reading Percent", "/steps/1");
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
    expect_rejected<Nullables>(
        R"({"code":"abcd"})",
        R"(the string is longer than the maximum length 3 at /code; reading the key "code" of Nullables)", "/code");
    expect_rejected<Nullables>(
        R"({"ratio":1.5})",
        R"(the number is greater than the maximum 1 at /ratio; reading the key "ratio" of Nullables)", "/ratio");
    expect_rejected<Nullables>(
        R"({"name":"abcd"})",
        R"(the value does not satisfy the predicate at /name; reading the key "name" of Nullables)", "/name");
    expect_rejected<Nullables>(
        R"({"boxed":-1})", R"(the number is less than the minimum 0 at /boxed; reading the key "boxed" of Nullables)",
        "/boxed");

    // std::unique_ptr, in a field and in a std::optional
    const auto owned = yyjson::read<Owned>(R"({"count":null,"level":null})");
    EXPECT_EQ(owned.count, nullptr);
    EXPECT_EQ(written(yyjson::read<Owned>(R"({"count":0,"level":5})")), R"({"count":0,"level":5})");
    const auto expect_owned_rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
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
    expect_owned_rejected(R"({"count":-1})",
                          R"(the number is less than the minimum 0 at /count; reading the key "count" of Owned)",
                          "/count");
    expect_owned_rejected(R"({"level":6})",
                          R"(the number is greater than the maximum 5 at /level; reading the key "level" of Owned)",
                          "/level");
    expect_owned_rejected(R"({"level":3})",
                          R"(the value does not satisfy the predicate at /level; reading the key "level" of Owned)",
                          "/level");
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
    expect_rejected<NullableListed>(
        R"({"level":3})", R"(the value is not one of 1, 2, null at /level; reading the key "level" of NullableListed)",
        "/level");
    expect_rejected<NullableListed>(
        R"({"mode":1})",
        R"(the value is not equal to the constant null at /mode; reading the key "mode" of NullableListed)", "/mode");
    expect_rejected<NullableListed>(
        R"({"none":[]})",
        R"(the value is not equal to the constant null at /none; reading the key "none" of NullableListed)", "/none");
    // null that a field holds is checked, as JSON Schema checks null, but merge_into resets the field with null
    expect_rejected<NullableListed>(
        R"({"unit":null})", R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of NullableListed)",
        "/unit", false);
    expect_rejected<NullableListed>(
        R"({"count":null})",
        R"(the value is not equal to the constant 1 at /count; reading the key "count" of NullableListed)", "/count",
        false);
    // null of a std::optional is a missing key
    expect_accepted<NullableListed>(R"({"note":null})", {});
}

TEST(Constraints, StringLiterals)
{
    expect_accepted<Literals>(R"({"unit":"cm","day":"2024-01-31"})", {"cm", "2024-01-31"});
    expect_rejected<Literals>(R"({"unit":"km"})",
                              R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of Literals)",
                              "/unit");
    expect_rejected<Literals>(
        R"({"day":"2024-01-32"})",
        R"(the string does not have the format "date" at /day; reading the key "day" of Literals)", "/day");
}

TEST(Constraints, ConstAndEnum)
{
    // a float with a listed value reads through a document, which keeps the JSON number to compare
    expect_accepted<Listed>(R"({"unit":"mm","version":2,"ratio":0.5,"flag":true,"big":18446744073709551615,"level":2})",
                            {.unit = "mm", .ratio = 0.5F, .big = 18446744073709551615ULL, .level = 2}, false);
    expect_accepted<Listed>(R"({"ratio":0.1,"level":null})", {}, false);
    // the JSON number compares, and not the float that it rounds to
    expect_rejected<Listed>(R"({"ratio":0.100000001})",
                            R"(the value is not one of 0.1, 0.5 at /ratio; reading the key "ratio" of Listed)",
                            "/ratio");
    const auto value = [](std::string_view key) { return std::format(R"(the value of the key "{}" of Listed )", key); };
    expect_rejected<Listed>(R"({"unit":"km"})",
                            R"(the value is not one of "mm", "cm", "m" at /unit; reading the key "unit" of Listed)",
                            "/unit");
    expect_rejected<Listed>(
        R"({"version":3})",
        R"(the value is not equal to the constant 2 at /version; reading the key "version" of Listed)", "/version");
    expect_rejected<Listed>(R"({"ratio":0.3})",
                            R"(the value is not one of 0.1, 0.5 at /ratio; reading the key "ratio" of Listed)",
                            "/ratio");
    expect_rejected<Listed>(R"({"flag":false})",
                            R"(the value is not equal to the constant true at /flag; reading the key "flag" of Listed)",
                            "/flag");
    expect_rejected<Listed>(
        R"({"big":18446744073709551614})",
        R"(the value is not one of -1, 0, 18446744073709551615 at /big; reading the key "big" of Listed)", "/big");
    expect_rejected<Listed>(
        R"({"level":4})", R"(the value is not one of 1, 2, 3 at /level; reading the key "level" of Listed)", "/level");
    // the constant is checked first; keywords that leave no value, as with enum_(2, 3), do not compile
    expect_rejected<Listed>(
        R"({"odd":3})", R"(the value is not equal to the constant 1 at /odd; reading the key "odd" of Listed)", "/odd");
    expect_accepted<Listed>(R"({"odd":1})", {}, false);
}

TEST(Constraints, ConstAndEnumNames)
{
    expect_accepted<ListedEnums>(R"({"first":"low","loud":"mid","maybe":"mid"})",
                                 {.loud = Tone::mid, .maybe = Tone::mid});
    expect_rejected<ListedEnums>(
        R"({"first":"mid"})",
        R"(the value is not equal to the constant "low" at /first; reading the key "first" of ListedEnums)", "/first");
    expect_rejected<ListedEnums>(
        R"({"loud":"low"})", R"(the value is not one of "mid", "HIGH" at /loud; reading the key "loud" of ListedEnums)",
        "/loud");
    expect_rejected<ListedEnums>(
        R"({"maybe":"HIGH"})",
        R"(the value is not equal to the constant "mid" at /maybe; reading the key "maybe" of ListedEnums)", "/maybe");
}

TEST(Constraints, RawNumbers)
{
    const auto raw = [](std::string_view json) {
        return yyjson::read<RawNumbers>(json, yyjson::ReadFlag::NumberAsRaw);
    };
    const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    EXPECT_EQ(*raw(R"({"n":5,"big":9007199254740993,"half":1.5,"flag":true})").n.as_raw(), "5");
    expect_raw_rejected(R"({"n":-1})",
                        R"(the number is less than the minimum 0 at /n; reading the key "n" of RawNumbers)", "/n");
    expect_raw_rejected(R"({"n":1e400})",
                        R"(the number is greater than the maximum 100 at /n; reading the key "n" of RawNumbers)", "/n");
    // an integer that fits 64 bits compares exactly, which a double does not
    expect_raw_rejected(
        R"({"big":9007199254740992})",
        R"(the number is less than the minimum 9007199254740993 at /big; reading the key "big" of RawNumbers)", "/big");
    expect_raw_rejected(R"({"half":1.25})",
                        R"(the number is not a multiple of 0.5 at /half; reading the key "half" of RawNumbers)",
                        "/half");
    // a raw number is a number, which yyjson::matches<bool> does not read
    expect_raw_rejected(R"({"flag":1})",
                        R"(the value does not satisfy matches at /flag; reading the key "flag" of RawNumbers)",
                        "/flag");
}

TEST(Constraints, RawEqualities)
{
    const auto raw = [](std::string_view json) { return yyjson::read<RawEquals>(json, yyjson::ReadFlag::NumberAsRaw); };
    const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    // a raw number equals the number that its text is without the flag
    EXPECT_NO_THROW((void)raw(R"({"one":1,"listed":2.5,"deep":{"a":[1.0,2.50]},"held":{"v":3.0}})"));
    EXPECT_NO_THROW((void)raw(R"({"one":1.0,"listed":"a","unique":[1,"1",2],"items":[[1],[2]]})"));
    // a number that overflows a double is an infinity
    expect_raw_rejected(
        R"({"unique":[1e400,2e400]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/1");
    expect_raw_rejected(R"({"one":2})",
                        R"(the value is not equal to the constant 1 at /one; reading the key "one" of RawEquals)",
                        "/one");
    expect_raw_rejected(R"({"listed":2})",
                        R"(the value is not one of "a", 2.5 at /listed; reading the key "listed" of RawEquals)",
                        "/listed");
    expect_raw_rejected(
        R"({"deep":{"a":[1,2.4]}})",
        R"(the value is not equal to the constant {"a":[1,2.5]} at /deep; reading the key "deep" of RawEquals)",
        "/deep");
    expect_raw_rejected(
        R"({"held":{"v":4}})",
        R"(the value is not equal to the constant {"v":3} at /held; reading the key "held" of RawEquals)", "/held");
    // a duplicate is reported at the later element, as for a std::vector
    expect_raw_rejected(
        R"({"unique":[1,1.0]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/1");
    expect_raw_rejected(
        R"({"items":[[2],[1],[1e0]]})",
        R"(the item is equal to an earlier item at /items/2; reading an element of the value at the key "items" of RawEquals)",
        "/items/2");
    expect_rejected<RawEquals>(
        R"({"unique":[0,{"a":1},{"a":1.0}]})",
        R"(the item is equal to an earlier item at /unique/2; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/2");

    // a raw number equals a number that is not raw, and the public operator== compares the raw text
    const auto doc = yyjson::read(R"([1,1.0])", yyjson::ReadFlag::NumberAsRaw);
    EXPECT_NE((*doc.as_array())[0], (*doc.as_array())[1]);
    EXPECT_NE((*doc.as_array())[0], yyjson::read("1"));
    auto mixed = yyjson::object(yyjson::read(R"({"unique":[1]})", yyjson::ReadFlag::NumberAsRaw));
    (*mixed["unique"].as_array()).emplace_back(1.0);
    const auto e = bad_cast_of([&] { (void)yyjson::value(mixed).cast<RawEquals>(); });
    EXPECT_EQ(
        message_of(e),
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)")
        << e.what();
    EXPECT_EQ(e.path(), "/unique/1");
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, RawExtendedNumbers)
{
    // a raw number in the syntax of JSON5 compares as the number that JSON5 reads
    for (const auto flag : {yyjson::ReadFlag::Json5, yyjson::ReadFlag::Json5 | yyjson::ReadFlag::NumberAsRaw})
    {
        const auto read = [flag](std::string_view json) { return yyjson::read<RawExtended>(json, flag); };
        const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message,
                                             std::string_view path) {
            const auto e = bad_cast_of([&] { (void)read(json); });
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        EXPECT_NO_THROW((void)read(R"({"n":+5,"c":0x10,"u":[0x10,15],"a":[16.,+15]})"));
        EXPECT_NO_THROW((void)read(R"({"c":16.})"));
        expect_raw_rejected(R"({"n":0x10})",
                            R"(the number is greater than the maximum 5 at /n; reading the key "n" of RawExtended)",
                            "/n");
        expect_raw_rejected(R"({"n":+10})",
                            R"(the number is greater than the maximum 5 at /n; reading the key "n" of RawExtended)",
                            "/n");
        expect_raw_rejected(R"({"c":0x11})",
                            R"(the value is not equal to the constant 16 at /c; reading the key "c" of RawExtended)",
                            "/c");
        expect_raw_rejected(
            R"({"u":[0x10,16]})",
            R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of RawExtended)",
            "/u/1");
        expect_raw_rejected(
            R"({"a":[0x10,16.]})",
            R"(the item is equal to an earlier item at /a/1; reading an element of the value at the key "a" of RawExtended)",
            "/a/1");
    }
}
#endif

TEST(Constraints, TextUnderStrict)
{
    // the constant compares the JSON object, with the key that Spot does not have
    EXPECT_EQ(yyjson::read<WideText>(R"({"at":{"x":1,"y":2,"z":3}})").at, (Spot{1, 2}));
    EXPECT_THROW((void)yyjson::read<WideText>(R"({"at":{"x":1,"y":2,"z":3}})", yyjson::reject_unknown_keys),
                 yyjson::bad_cast);
    for (const auto* json : {R"({"at":{"x":1,"y":2}})", R"({"at":{"x":1,"y":3,"z":3}})"})
        expect_rejected<WideText>(
            json,
            R"(the value is not equal to the constant {"x":1,"y":2,"z":3} at /at; reading the key "at" of WideText)",
            "/at");
}

TEST(Constraints, ConstAndEnumTexts)
{
    // the JSON values compare, which only a document keeps
    expect_accepted<ListedTexts>(R"({"at":{"y":1,"x":1},"pair":[1,2],"scale":[2.5,2.5],"mixed":[0]})",
                                 {.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}}, false);
    // the number 1.0 equals 1, and an integer is a number of the variant, which reads it into its alternative
    expect_accepted<ListedTexts>(R"({"scale":[1.0],"mixed":0})", {.scale = std::vector{1.0}}, false);
    expect_accepted<ListedTexts>("{}", {}, false);
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ListedTexts )", key);
    };
    // the members of an object have no order
    expect_accepted<ListedTexts>(R"({"order":{"b":1,"a":2}})", {.order = {{"b", 1}, {"a", 2}}}, false);
    expect_rejected<ListedTexts>(
        R"({"order":{"a":2}})",
        R"(the value is not equal to the constant {"a":2,"b":1} at /order; reading the key "order" of ListedTexts)",
        "/order");
    expect_rejected<ListedTexts>(
        R"({"at":{"x":0,"y":1}})",
        R"(the value is not one of { "y": 0, "x": 0 }, {"x":1,"y":1} at /at; reading the key "at" of ListedTexts)",
        "/at");
    expect_rejected<ListedTexts>(
        R"({"pair":[2,1]})",
        R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of ListedTexts)", "/pair");
    expect_rejected<ListedTexts>(
        R"({"scale":[2.5]})",
        R"(the value is not one of [1], [2.5, 2.5] at /scale; reading the key "scale" of ListedTexts)", "/scale");
    // an object is no value of the variant: it reads no constant
    expect_rejected<ListedTexts>(
        R"({"mixed":1})", R"(the value is not one of 0, [0], {} at /mixed; reading the key "mixed" of ListedTexts)",
        "/mixed");

    // validate and the checked write compare the text that the write gives
    expect_valid(ListedTexts());
    expect_valid(ListedTexts{.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}});
    expect_valid(ListedTexts{.order = {{"b", 1}, {"a", 2}}});
    expect_invalid(
        ListedTexts{.at = {2, 2}},
        R"(the value is not one of { "y": 0, "x": 0 }, {"x":1,"y":1} at /at; reading the key "at" of ListedTexts)",
        "/at");
    expect_invalid(ListedTexts{.pair = {}},
                   R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of ListedTexts)",
                   "/pair");
    expect_invalid(ListedTexts{.scale = std::vector{2.5}},
                   R"(the value is not one of [1], [2.5, 2.5] at /scale; reading the key "scale" of ListedTexts)",
                   "/scale");
    expect_invalid(ListedTexts{.mixed = 3},
                   R"(the value is not one of 0, [0], {} at /mixed; reading the key "mixed" of ListedTexts)", "/mixed");
    expect_invalid(
        ListedTexts{.order = {{"a", 2}}},
        R"(the value is not equal to the constant {"a":2,"b":1} at /order; reading the key "order" of ListedTexts)",
        "/order");

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
    expect_rejected<Formatted>(
        R"({"when":"2023-02-29T00:00:00Z"})",
        R"(the string does not have the format "date-time" at /when; reading the key "when" of Formatted)", "/when");
    expect_rejected<Formatted>(
        R"({"when":"2024-01-01T23:59:60+01:00"})",
        R"(the string does not have the format "date-time" at /when; reading the key "when" of Formatted)", "/when");
    expect_rejected<Formatted>(
        R"({"mail":"a..b@example.com"})",
        R"(the string does not have the format "email" at /mail; reading the key "mail" of Formatted)", "/mail");
    expect_rejected<Formatted>(
        R"({"code":"Ab"})",
        R"(the string does not have the format "upper" at /code; reading the key "code" of Formatted)", "/code");
    expect_accepted<Formatted>(R"({"re":"([abc])+\\s+$"})", {.re = R"(([abc])+\s+$)"});
    expect_rejected<Formatted>(
        R"({"re":"^(abc]"})",
        R"(the string does not have the format "regex" at /re; reading the key "re" of Formatted)", "/re");
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
    const auto rejected = [](auto read, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of(read);
        expect_text(message_of(e), message);
        EXPECT_EQ(e.path(), path);
    };
    rejected([] { (void)yyjson::read<Bounded>(R"({"i":11,"d":0.5,"u":3,"o":1})", yyjson::strict); },
             R"(the number is greater than the maximum 10 at /i; reading the key "i" of Bounded)", "/i");
    rejected(
        [] {
            auto t = Texts();
            yyjson::read_into(R"({"name":"a","code":"ABC"})", t, yyjson::strict);
        },
        R"(the string is shorter than the minimum length 2 at /name; reading the key "name" of Texts)", "/name");
    rejected(
        [] {
            auto t = Listed();
            yyjson::merge_into(R"({"version":1})", t, yyjson::strict);
        },
        R"(the value is not equal to the constant 2 at /version; reading the key "version" of Listed)", "/version");
    rejected([] { (void)yyjson::read(R"({"done":-1,"steps":[]})").cast<Progress>(yyjson::strict); },
             "the number is less than the minimum 0 at /done; reading Percent", "/done");
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
    // a float with a listed value leaves Listed to the document
    const auto listed = [](const char* json) { return expect_direct_read<Listed>(json); };
    same(listed, false, R"({"unit":"cm","version":2})");
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

TEST(Constraints, Elements)
{
    expect_accepted<Elements>(
        R"({"scores":[0,1],"pair":[9,0],"names":["ab"],"set":[5,1],"uset":[4],"limits":{"a":100},)"
        R"("groups":{"g":[1]},"by_id":[[1,5]],"ordered":[[1,2]],"holes":[null,5],"tags":["abc"],"nested":[["abc"]]})",
        {{0, 1},
         {9, 0},
         {"ab"},
         {1, 5},
         {4},  // one element, since the order of an unordered_set depends on the standard library
         {{"a", 100}},
         {{"g", {1}}},
         {{1, 5}},
         {{1, 2}},
         {std::nullopt, 5},
         std::vector<std::string>{"abc"},
         {{"abc"}}});
    expect_accepted<Elements>(R"({"tags":null})", {});

    expect_rejected<Elements>(
        R"({"scores":[0,-1]})",
        R"(the number is less than the minimum 0 at /scores/1; reading an element of the value at the key "scores" of Elements)",
        "/scores/1");
    expect_rejected<Elements>(
        R"({"pair":[10,0]})",
        R"(the number is greater than the maximum 9 at /pair/0; reading an element of the value at the key "pair" of Elements)",
        "/pair/0");
    expect_rejected<Elements>(
        R"({"names":["a","B"]})",
        R"(the string does not match the pattern "^[a-z]+$" at /names/1; reading an element of the value at the key "names" of Elements)",
        "/names/1");
    // the element of a set or of an unordered set is found in the array
    expect_rejected<Elements>(
        R"({"set":[1,9,3]})",
        R"(the number is greater than the maximum 5 at /set/1; reading an element of the value at the key "set" of Elements)",
        "/set/1");
    expect_rejected<Elements>(
        R"({"uset":[2,4,6,3,8]})",
        R"(the value does not satisfy the predicate at /uset/3; reading an element of the value at the key "uset" of Elements)",
        "/uset/3");
    // the values of a map, and the keywords of the map itself
    expect_rejected<Elements>(
        R"({"limits":{"a":1,"b":101}})",
        R"(the number is greater than the maximum 100 at /limits/b; reading an element of the value at the key "limits" of Elements)",
        "/limits/b");
    expect_rejected<Elements>(
        R"({"limits":{"a":1,"b":2,"c":3,"d":4}})",
        R"(the object has more properties than the maximum 3 at /limits; reading the key "limits" of Elements)",
        "/limits");
    expect_rejected<Elements>(
        R"({"groups":{"g":[1],"h":[]}})",
        R"(the array has fewer items than the minimum 1 at /groups/h; reading an element of the value at the key "groups" of Elements)",
        "/groups/h");
    expect_rejected<Elements>(
        R"({"groups":{"g":[1,0]}})",
        R"(the number is less than the minimum 1 at /groups/g/1; reading an element of the value at the key "groups" of Elements)",
        "/groups/g/1");
    // a map whose keys are not strings is an array of pairs, whose values are the second elements
    expect_rejected<Elements>(
        R"({"by_id":[[1,5],[2,6]]})",
        R"(the number is greater than the maximum 5 at /by_id/1/1; reading an element of the value at the key "by_id" of Elements)",
        "/by_id/1/1");
    expect_rejected<Elements>(
        R"({"ordered":[[1,2],[4,3]]})",
        R"(the value does not satisfy the predicate at /ordered/1; reading an element of the value at the key "ordered" of Elements)",
        "/ordered/1");
    // the value of a std::optional element, and the elements of a std::optional field
    expect_rejected<Elements>(
        R"({"holes":[null,6]})",
        R"(the number is greater than the maximum 5 at /holes/1; reading an element of the value at the key "holes" of Elements)",
        "/holes/1");
    expect_rejected<Elements>(
        R"({"tags":["abcd"]})",
        R"(the string is longer than the maximum length 3 at /tags/0; reading an element of the value at the key "tags" of Elements)",
        "/tags/0");
    // the elements of the elements
    expect_rejected<Elements>(
        R"({"nested":[["a"],[]]})",
        R"(the array has fewer items than the minimum 1 at /nested/1; reading an element of the value at the key "nested" of Elements)",
        "/nested/1");
    expect_rejected<Elements>(
        R"({"nested":[["a"],["b","abcd"]]})",
        R"(the string is longer than the maximum length 3 at /nested/1/1; reading an element of the value at the key "nested" of Elements)",
        "/nested/1/1");

    expect_accepted<ElementsVisitable>(R"({"v":[1]})", {{1}});
    expect_rejected<ElementsVisitable>(
        R"({"v":[1,0]})",
        R"(the number is not greater than the exclusive minimum 0 at /v/1; reading an element of the value at the key "v" of ElementsVisitable)",
        "/v/1");

    // the elements of a transparent struct
    EXPECT_EQ(yyjson::read<Scores>("[0,1]"), (Scores{{0, 1}}));
    expect_rejected<Scores>("[]", "the array has fewer items than the minimum 1; reading Scores", "");
    expect_rejected<Scores>("[0,-1]", "the number is less than the minimum 0 at /1; reading an element of Scores",
                            "/1");
}

TEST(Constraints, ElementNullables)
{
    auto expected = ElementNullables();
    expected.ids = {nullptr, std::make_shared<int>(1)};
    expected.steps = {1, 2};
    // the unique pointers compare their JSON values, which only a document keeps
    expect_accepted<ElementNullables>(R"({"list":null,"ids":[null,1],"steps":[1,2],"counts":null})", expected, false);
    expected.list = std::make_shared<std::vector<int>>(std::vector{9});
    expected.counts = std::vector{1};
    expect_accepted<ElementNullables>(R"({"list":[9],"ids":[null,1],"steps":[1,2],"counts":[1]})", expected, false);

    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of ElementNullables )", key);
    };
    expect_rejected<ElementNullables>(
        R"({"list":[]})",
        R"(the array has fewer items than the minimum 1 at /list; reading the key "list" of ElementNullables)",
        "/list");
    expect_rejected<ElementNullables>(
        R"({"list":[1,10]})",
        R"(the number is greater than the maximum 9 at /list/1; reading an element of the value at the key "list" of ElementNullables)",
        "/list/1");
    // smart pointers compare the values that they hold, and null is equal to null
    expect_rejected<ElementNullables>(
        R"({"ids":[1,2,1]})",
        R"(the item is equal to an earlier item at /ids/2; reading an element of the value at the key "ids" of ElementNullables)",
        "/ids/2");
    expect_rejected<ElementNullables>(
        R"({"ids":[null,1,null]})",
        R"(the item is equal to an earlier item at /ids/2; reading an element of the value at the key "ids" of ElementNullables)",
        "/ids/2");
    // a null element is a value, which yyjson::enum_ without nullptr rejects
    expect_rejected<ElementNullables>(
        R"({"steps":[1,null]})",
        R"(the value is not one of 1, 2 at /steps/1; reading an element of the value at the key "steps" of ElementNullables)",
        "/steps/1");
    expect_rejected<ElementNullables>(
        R"({"counts":[1,2]})",
        R"(the array has more items than the maximum 1 at /counts; reading the key "counts" of ElementNullables)",
        "/counts");

    // std::unique_ptr elements
    EXPECT_EQ(written(yyjson::read<OwnedElements>(R"({"tags":[null,"abc"]})")), R"({"tags":[null,"abc"]})");
    const auto expect_owned_rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<OwnedElements>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<OwnedElements>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = OwnedElements();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<OwnedElements>(); }));
    };
    expect_owned_rejected(
        R"({"tags":[null,"abcd"]})",
        R"(the string is longer than the maximum length 3 at /tags/1; reading an element of the value at the key "tags" of OwnedElements)",
        "/tags/1");
    expect_owned_rejected(
        R"({"tags":["a",null,"a"]})",
        R"(the item is equal to an earlier item at /tags/2; reading an element of the value at the key "tags" of OwnedElements)",
        "/tags/2");
}

TEST(Constraints, ElementsMerge)
{
    // the keywords on the values check the members of the patch, and the members that the map keeps on what they
    // write; the counts count the map after the merge
    auto t = Elements();
    t.limits = {{"old", 50}};
    EXPECT_TRUE(expect_merge_into(t, R"({"limits":{"a":1}})").find(R"("limits":{"a":1,"old":50})") !=
                std::string::npos);
    t.limits = {{"old", 200}};
    EXPECT_EQ(
        expect_merge_into(t, R"({"limits":{"a":1}})"),
        R"(bad cast: the number is greater than the maximum 100 at /limits/old; reading an element of the value at the key "limits" of Elements)");
    const auto limits = [](std::string_view text) {
        return std::format(R"(the value of the key "limits" of Elements {})", text);
    };
    const auto expect_merge_rejected = [](Elements value, std::string_view json, const expected_text& message,
                                          std::string_view path) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    // the merged map keeps "old" before the new key "a", and read reports the first member that it rejects
    expect_merge_rejected(
        t, R"({"limits":{"a":101}})",
        R"(the number is greater than the maximum 100 at /limits/old; reading an element of the value at the key "limits" of Elements)",
        "/limits/old");
    t.limits = {{"old", 50}};
    expect_merge_rejected(
        t, R"({"limits":{"a":101}})",
        R"(the number is greater than the maximum 100 at /limits/a; reading an element of the value at the key "limits" of Elements)",
        "/limits/a");
    t.limits = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto direct = false;
    EXPECT_TRUE(
        expect_merge_into(t, R"({"limits":{"a":null,"d":4}})", &direct).find(R"("limits":{"b":2,"c":3,"d":4})") !=
        std::string::npos);
    EXPECT_TRUE(direct);
    for (const auto* json : {R"({"limits":{"d":4}})", R"({"limits":{"a":null,"d":4,"e":5}})"})
        expect_merge_rejected(
            t, json,
            R"(the object has more properties than the maximum 3 at /limits; reading the key "limits" of Elements)",
            "/limits");
    // the elements of an array replace those of the range, which are checked as they are read
    auto merged = Elements();
    merged.scores = {-1};
    yyjson::merge_into(R"({"scores":[1,2]})", merged);
    EXPECT_EQ(merged.scores, (std::vector{1, 2}));
    // null resets a std::optional range, which is not checked
    merged.tags = std::vector<std::string>{"abcd"};
    yyjson::merge_into(R"({"tags":null})", merged);
    EXPECT_EQ(merged.tags, std::nullopt);
}

TEST(Constraints, InputCounts)
{
    // every read counts the elements and the members of the JSON value, without a document
    const auto key = [](std::string_view k, std::string_view text) {
        return std::format(R"(the value of the key "{}" of InputCounted {})", k, text);
    };
    expect_accepted<InputCounted>(R"({"s":[1,1]})", {.s = {1}});
    expect_rejected<InputCounted>(
        R"({"s":[1]})", R"(the array has fewer items than the minimum 2 at /s; reading the key "s" of InputCounted)",
        "/s");
    expect_rejected<InputCounted>(
        R"({"s":[1,2,2]})", R"(the array has more items than the maximum 2 at /s; reading the key "s" of InputCounted)",
        "/s");
    // a merge keeps the last value of a repeated key, and a read the first
    const auto* const repeated = R"({"m":{"a":1,"a":2}})";
    EXPECT_EQ(yyjson::read<InputCounted>(repeated).m.size(), 1U);
    EXPECT_EQ(yyjson::read(repeated).cast<InputCounted>().m.size(), 1U);
    EXPECT_TRUE(expect_direct_read<InputCounted>(repeated));
    EXPECT_TRUE(expect_read_into(InputCounted(), repeated));
    expect_rejected<InputCounted>(
        R"({"m":{"a":1}})",
        R"(the object has fewer properties than the minimum 2 at /m; reading the key "m" of InputCounted)", "/m");
    expect_rejected<InputCounted>(
        R"({"m":{"a":1,"b":2,"a":3}})",
        R"(the object has more properties than the maximum 2 at /m; reading the key "m" of InputCounted)", "/m");
    expect_accepted<InputCounted>(R"({"v":[1,1]})", {.v = {1, 1}});
    expect_rejected<InputCounted>(
        R"({"v":[1]})", R"(the array has fewer items than the minimum 2 at /v; reading the key "v" of InputCounted)",
        "/v");
    expect_accepted<InputCounted>(R"({"c":[5,5]})", {.c = {5}});
    expect_rejected<InputCounted>(
        R"({"c":[5,1]})",
        R"(the array has fewer than 2 items that satisfy contains at /c; reading the key "c" of InputCounted)", "/c");

    // `merge_into` counts the members of the patch, and replaces a set with the elements that it counts
    const auto seed = InputCounted{.s = {1, 2}, .m = {{"x", 0}, {"y", 0}}, .v = {1, 1}, .c = {5, 6}};
    auto t = seed;
    t.m = {{"x", 0}};
    for (const auto& [start, json] :
         {std::pair{seed, R"({"s":[1,1]})"}, std::pair{t, R"({"m":{"a":1,"a":2}})"}, std::pair{seed, R"({"c":[5,5]})"}})
    {
        auto direct = false;
        EXPECT_FALSE(expect_merge_into(start, json, &direct).starts_with("bad cast")) << json;
        EXPECT_TRUE(direct || !reads_directly) << json;
    }
    // the fields that the patch leaves out are counted too
    EXPECT_EQ(
        expect_merge_into(t, R"({"s":[1,1]})"),
        R"(bad cast: the object has fewer properties than the minimum 2 at /m; reading the key "m" of InputCounted)");
    yyjson::merge_into(R"({"m":{"a":1,"a":2}})", t);
    EXPECT_EQ(t.m, (std::map<std::string, int>{{"a", 2}, {"x", 0}}));
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"b":1}})", t), yyjson::bad_cast);

    // a condition of the struct counts the elements of the JSON array too
    expect_accepted<CountGate>(R"({"s":[1,1],"k":1})", {{1}, 1});
    expect_accepted<CountGate>(R"({"s":[1]})", {{1}, 0});
    expect_rejected<CountGate>(
        R"({"s":[1,1]})",
        R"(a required key is missing; reading the key "k" of CountGate, which yyjson::if_then_else requires)", "");
    auto direct = false;
    static_cast<void>(expect_merge_into(CountGate{}, R"({"s":[2,2],"k":3})", &direct));
    EXPECT_TRUE(direct || !reads_directly);
}

TEST(Constraints, MergeHeldMaps)
{
    auto t = HeldLimits();
    t.pointer = std::make_shared<Limits>(Limits{{"old", 200}});
    t.held = Limits{{"old", 200}};
    t.boxed = yyjson::indirect<Limits>(Limits{{"old", 200}});
    t.nested = {{"k", {{"old", 200}}}};
    // a pointer takes the map of the patch, and the other holders keep the members that the patch does not give,
    // which are checked too
    EXPECT_EQ(expect_merge_into(t, R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{}}})"),
              "bad cast: the number is greater than the maximum 100 at /nested/k/old; reading an element of the value "
              "at the key "
              "\"nested\" of HeldLimits");
    EXPECT_EQ(
        expect_merge_into(t, R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{"old":2}}})"),
        R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{"old":2}}})");
    const auto expect_merge_rejected = [](std::string_view json, const expected_text& message, std::string_view path,
                                          HeldLimits value = HeldLimits()) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto more_properties = [](std::string_view key) {
        return std::format(
            R"(the object has more properties than the maximum 1 at /{0}; reading the key "{0}" of HeldLimits)", key);
    };
    for (const auto* key : {"pointer", "held", "boxed"})
    {
        expect_merge_rejected(
            std::format(R"({{"{}":{{"a":101}}}})", key),
            std::format(
                R"(the number is greater than the maximum 100 at /{0}/a; reading an element of the value at the key "{0}" of HeldLimits)",
                key),
            std::format("/{}/a", key));
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1,"b":2}}}})", key), more_properties(key),
                              std::format("/{}", key));
    }
    expect_merge_rejected(
        R"({"nested":{"k":{"a":101}}})",
        R"(the number is greater than the maximum 100 at /nested/k/a; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k/a");
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1,"b":2}}})",
        R"(the object has more properties than the maximum 1 at /nested/k; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k");
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1},"i":{}}})",
        R"(the object has more properties than the maximum 1 at /nested; reading the key "nested" of HeldLimits)",
        "/nested");
    // the counts count the members that the holders keep; the kept values satisfy the other keywords, which read
    // checks first in the merged object
    t.pointer = std::make_shared<Limits>(Limits{{"old", 1}});
    t.held = Limits{{"old", 1}};
    t.boxed = yyjson::indirect<Limits>(Limits{{"old", 1}});
    t.nested = {{"k", {{"old", 1}}}};
    for (const auto* key : {"held", "boxed"})
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1}}}})", key), more_properties(key), std::format("/{}", key),
                              t);
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1}}})",
        R"(the object has more properties than the maximum 1 at /nested/k; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k", t);
    expect_merge_rejected(
        R"({"nested":{"i":{}}})",
        R"(the object has more properties than the maximum 1 at /nested; reading the key "nested" of HeldLimits)",
        "/nested", t);
}

TEST(Constraints, MergeRepeatedKeys)
{
    // a patch can give a key more than once; the checks look only at the members that the merged map has, and the
    // counts count the members of that map.  An absent map reads the patch, in which a null removes the members of
    // its key before it, and the map that it reads counts its members only
    auto t = HeldLimits();
    t.boxed = yyjson::indirect<Limits>(Limits{});
    EXPECT_EQ(expect_merge_into(t, R"({"nested":{"a":{"x":1},"a":null}})"), expect_merge_into(t, R"({"nested":{}})"));
    EXPECT_EQ(expect_merge_into(t, R"({"held":{"a":1,"a":null}})"), expect_merge_into(t, R"({"held":{}})"));
    EXPECT_EQ(merged(t, R"({"held":{"a":1,"a":null,"b":1}})"),
              R"({"pointer":null,"held":{"b":1},"boxed":{},"nested":{}})");
    t.held = Limits{};
    EXPECT_EQ(merged(t, R"({"held":{"a":1,"a":null,"b":1}})"),
              R"({"pointer":null,"held":{"b":1},"boxed":{},"nested":{}})");
    EXPECT_EQ(
        expect_merge_into(t, R"({"held":{"a":1,"b":null,"b":1}})"),
        R"(bad cast: the object has more properties than the maximum 1 at /held; reading the key "held" of HeldLimits)");
    const auto sets = RepeatedSets();
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1,2],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"m":{"a":null,"a":[1,2]}})"),
        R"(bad cast: the array has more items than the maximum 1 at /m/a; reading an element of the value at the key "m" of RepeatedSets)");
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":11,"a":null,"b":9}})"), R"({"m":{},"n":{"b":9}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":11}})"),
        R"(bad cast: the number is greater than the maximum 10 at /n/b; reading an element of the value at the key "n" of RepeatedSets)");
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":1,"c":1}})"), R"({"m":{},"n":{"b":1,"c":1}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"n":{"a":1,"b":1,"c":null,"c":1}})"),
        R"(bad cast: the object has more properties than the maximum 2 at /n; reading the key "n" of RepeatedSets)");
    // a patch without a repeated key, through the document too
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1]},"n":{"a":1}})"), R"({"m":{"a":[1]},"n":{"a":1}})");
}

TEST(Constraints, MergeKept)
{
    // yyjson::property_names and yyjson::min_properties check the keys of the merged map, without null
    auto keyed = Keyed();
    keyed.sized = {{"a", 1}};
    keyed.m = {{"abc", 1}};
    auto direct = false;
    EXPECT_TRUE(expect_merge_into(keyed, R"({"m":{"ab":1}})", &direct).find(R"("m":{"ab":1,"abc":1})") !=
                std::string::npos);
    EXPECT_TRUE(direct);
    keyed.m = {{"ABC", 1}};
    EXPECT_EQ(
        expect_merge_into(keyed, R"({"m":{"ab":1}})"),
        R"(bad cast: the key does not match the pattern "^[a-z]+$" at /m/ABC; reading a key of the value at the key "m" of Keyed)");
    keyed.m = {{"abc", 1}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"ab":null,"Ab":1}})", keyed); });
    EXPECT_EQ(
        message_of(e),
        R"(the key does not match the pattern "^[a-z]+$" at /m/Ab; reading a key of the value at the key "m" of Keyed)")
        << e.what();
    EXPECT_EQ(e.path(), "/m/Ab");
    keyed = Keyed();
    keyed.sized = {{"a", 1}};
    EXPECT_TRUE(expect_merge_into(keyed, R"({"sized":{"b":2}})").find(R"("sized":{"a":1,"b":2})") != std::string::npos);
    const auto empty = bad_cast_of([&] { yyjson::merge_into(R"({"sized":{"a":null}})", keyed); });
    EXPECT_EQ(message_of(empty),
              R"(the object has fewer properties than the minimum 1 at /sized; reading the key "sized" of Keyed)")
        << empty.what();
    EXPECT_EQ(empty.path(), "/sized");
}

TEST(Constraints, UniqueItems)
{
    expect_accepted<Unique>(R"({"ints":[1,2],"strs":["a","b"],"reals":[0.5,1],"plain":[{"v":1},{"v":2}],)"
                            R"("opts":[null,1],"nested":[[1],[1,2]]})",
                            {{1, 2}, {"a", "b"}, {0.5, 1.0}, {{1}, {2}}, {std::nullopt, 1}, {{1}, {1, 2}}}, false);

    const auto equal = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Unique is equal to an earlier element)", key);
    };
    // the error is at the second of the equal elements
    expect_rejected<Unique>(
        R"({"ints":[1,2,1]})",
        R"(the item is equal to an earlier item at /ints/2; reading an element of the value at the key "ints" of Unique)",
        "/ints/2");
    expect_rejected<Unique>(
        R"({"reals":[0.0,-0.0]})",
        R"(the item is equal to an earlier item at /reals/1; reading an element of the value at the key "reals" of Unique)",
        "/reals/1");
    expect_rejected<Unique>(
        R"({"plain":[{"v":1},{"v":2},{"v":2}]})",
        R"(the item is equal to an earlier item at /plain/2; reading an element of the value at the key "plain" of Unique)",
        "/plain/2");
    expect_rejected<Unique>(
        R"({"opts":[null,1,null]})",
        R"(the item is equal to an earlier item at /opts/2; reading an element of the value at the key "opts" of Unique)",
        "/opts/2");
    expect_rejected<Unique>(
        R"({"nested":[[1],[1,2,1]]})",
        R"(the item is equal to an earlier item at /nested/1/2; reading an element of the value at the key "nested" of Unique)",
        "/nested/1/2");

    // more than 16 elements are sorted, and the error is still at the first element that has an earlier equal one
    auto strs = std::string(R"({"strs":[)");
    auto reals = std::string(R"({"reals":[)");
    for (auto i = 0; i < 20; ++i)
    {
        strs += std::format(R"("s{}",)", 19 - i);
        reals += std::format("{},", i);
    }
    expect_accepted<Unique>(strs.substr(0, strs.size() - 1) + "]}",
                            {.strs = {"s19", "s18", "s17", "s16", "s15", "s14", "s13", "s12", "s11", "s10",
                                      "s9",  "s8",  "s7",  "s6",  "s5",  "s4",  "s3",  "s2",  "s1",  "s0"}},
                            false);
    expect_rejected<Unique>(
        strs + R"("s3","s19"]})",
        R"(the item is equal to an earlier item at /strs/20; reading an element of the value at the key "strs" of Unique)",
        "/strs/20");
    expect_rejected<Unique>(
        reals + "-0.0]}",
        R"(the item is equal to an earlier item at /reals/20; reading an element of the value at the key "reals" of Unique)",
        "/reals/20");

    // values of the DOM compare as JSON: numbers by value, objects regardless of the order of the members
    EXPECT_EQ(yyjson::read<UniqueJson>(R"({"any":[1,"1",[1],{"a":1},null,true]})").any.size(), 6);
    expect_rejected<UniqueJson>(
        R"({"any":[{"a":1,"b":[2]},{"b":[2.0],"a":1}]})",
        R"(the item is equal to an earlier item at /any/1; reading an element of the value at the key "any" of UniqueJson)",
        "/any/1");

    // C++ values compare as the JSON that they are written as
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"boxes":[{"p":1},{"p":2},{"p":null}],"loose":[{"v":1},{"v":2}],)"
                                        R"("trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[]}],)"
                                        R"("maps":[{"a":1,"b":2},{"a":1}],"twins":[1,2]})")
                  .boxes.size(),
              3);
    expect_rejected<UniqueParts>(
        R"({"boxes":[{"p":1},{"p":1}]})",
        R"(the item is equal to an earlier item at /boxes/1; reading an element of the value at the key "boxes" of UniqueParts)",
        "/boxes/1");
    expect_rejected<UniqueParts>(
        R"({"boxes":[{"p":null},{"p":2},{"p":null}]})",
        R"(the item is equal to an earlier item at /boxes/2; reading an element of the value at the key "boxes" of UniqueParts)",
        "/boxes/2");
    expect_rejected<UniqueParts>(
        R"({"loose":[{"v":1},{"v":1}]})",
        R"(the item is equal to an earlier item at /loose/1; reading an element of the value at the key "loose" of UniqueParts)",
        "/loose/1");
    expect_rejected<UniqueParts>(
        R"({"trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[{"v":2,"kids":[]}]}]})",
        R"(the item is equal to an earlier item at /trees/1; reading an element of the value at the key "trees" of UniqueParts)",
        "/trees/1");
    expect_rejected<UniqueParts>(
        R"({"maps":[{"a":1,"b":2,"c":3},{"c":3,"b":2,"a":1}]})",
        R"(the item is equal to an earlier item at /maps/1; reading an element of the value at the key "maps" of UniqueParts)",
        "/maps/1");
    // a range of key-value pairs is written as an object, whose members have no order
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"a":1,"b":3}]})").pairs.size(), 2);
    expect_rejected<UniqueParts>(
        R"({"pairs":[{"a":1,"b":2},{"b":2,"a":1}]})",
        R"(the item is equal to an earlier item at /pairs/1; reading an element of the value at the key "pairs" of UniqueParts)",
        "/pairs/1");
    // a range of tuples is sorted from 17 elements on
    auto tuples = std::string(R"({"tuples":[)");
    for (auto i = 0; i < 20; ++i) tuples += std::format(R"([{},"{}"],)", i % 2, i);
    EXPECT_EQ(yyjson::read<UniqueParts>(tuples.substr(0, tuples.size() - 1) + "]}").tuples.size(), 20);
    expect_rejected<UniqueParts>(
        tuples + R"([1,"7"]]})",
        R"(the item is equal to an earlier item at /tuples/20; reading an element of the value at the key "tuples" of UniqueParts)",
        "/tuples/20");
    // the elements of a tuple of one type
    expect_rejected<UniqueParts>(
        R"({"twins":[3,3]})",
        R"(the item is equal to an earlier item at /twins/1; reading an element of the value at the key "twins" of UniqueParts)",
        "/twins/1");
}

TEST(Constraints, UniqueReals)
{
    // the JSON numbers compare, and not the doubles that they round to
    expect_accepted<Unusual>(R"({"reals":[9007199254740993,9007199254740992]})",
                             {.reals = {9007199254740992.0, 9007199254740992.0}}, false);
    expect_rejected<Unusual>(
        R"({"reals":[9007199254740992,9007199254740993,9007199254740992]})",
        R"(the item is equal to an earlier item at /reals/2; reading an element of the value at the key "reals" of Unusual)",
        "/reals/2");
    // NaN is equal to nothing, also among more than 16 elements, which are sorted
    auto nans = std::string(R"({"reals":[)");
    for (auto i = 0; i < 20; ++i) nans += i % 4 == 0 ? "NaN," : std::format("{},", i);
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    const auto read = yyjson::read<Unusual>(nans.substr(0, nans.size() - 1) + "]}", flag);
    EXPECT_EQ(read.reals.size(), 20);
    const auto e = bad_cast_of([&] { (void)yyjson::read<Unusual>(nans + "Infinity,-Infinity,Infinity]}", flag); });
    EXPECT_EQ(
        message_of(e),
        R"(the item is equal to an earlier item at /reals/22; reading an element of the value at the key "reals" of Unusual)")
        << e.what();
    EXPECT_EQ(e.path(), "/reals/22");
}

TEST(Constraints, UniqueJsonEquality)
{
    using U = UniqueJsonEquality;
    const auto at = [](std::string_view key, std::string_view path) {
        return std::format(
            R"(the item is equal to an earlier item at {}; reading an element of the value at the key "{}" of )"
            "UniqueJsonEquality",
            path, key);
    };
    // a struct element compares its JSON object, with the keys that the struct ignores
    expect_accepted<U>(R"({"spots":[{"x":1,"y":0},{"x":1,"y":0,"z":2}]})", {.spots = {{1, 0}, {1, 0}}}, false);
    expect_rejected<U>(R"({"spots":[{"x":1,"y":0},{"x":1.0,"y":0}]})", at("spots", "/spots/1"), "/spots/1");
    auto spots = std::string(R"({"spots":[)");
    for (auto i = 0; i < 19; ++i) spots += std::format(R"({{"x":{},"y":0}},)", i);
    expect_rejected<U>(spots + R"({"x":18,"y":0}]})", at("spots", "/spots/19"), "/spots/19");
    // a set compares its JSON array, with its order and its repeats
    expect_accepted<U>(R"({"sets":[[1,2],[2,1]]})", {.sets = {{1, 2}, {1, 2}}}, false);
    expect_accepted<U>(R"({"sets":[[1,1],[1]]})", {.sets = {{1}, {1}}}, false);
    expect_rejected<U>(R"({"sets":[[1,2],[1,2]]})", at("sets", "/sets/1"), "/sets/1");
    expect_rejected<U>(R"({"mixed":[1,1.0]})", at("mixed", "/mixed/1"), "/mixed/1");
    // an object compares as a multiset of members, also with repeated keys
    expect_accepted<U>(R"({"maps":[{"a":1,"a":2},{"a":1}]})", {.maps = {{{"a", 1}}, {{"a", 1}}}}, false);
    expect_rejected<U>(R"({"maps":[{"a":1,"a":2},{"a":2,"a":1}]})", at("maps", "/maps/1"), "/maps/1");
    // a range of pairs compares the JSON that it was read from, an object or an array
    expect_accepted<U>(R"({"pairs":[[["a",1]],{"a":1}],"colors":["red",1]})",
                       {.pairs = {{{"a", 1}}, {{"a", 1}}}, .colors = {UjColor::red, UjColor::green}}, false);
    expect_accepted<U>(R"({"kv":[{"a":"b"},[["a","b"]]]})", {.kv = {{{"a", "b"}}, {{"a", "b"}}}}, false);
    expect_rejected<U>(R"({"kv":[{"a":"b"},{"a":"b"}]})", at("kv", "/kv/1"), "/kv/1");
    expect_accepted<U>(R"({"colors":["red",0]})", {.colors = {UjColor::red, UjColor::red}}, false);
    expect_accepted<U>(R"({"twins":[{"x":1,"y":0},{"x":1,"y":0,"z":0}]})", {.twins = {Spot{1, 0}, Spot{1, 0}}}, false);
    expect_rejected<U>(R"({"twins":[{"x":1,"y":0},{"y":0,"x":1}]})", at("twins", "/twins/1"), "/twins/1");
    expect_rejected<U>(R"({"nested":[[{"x":0,"y":0},{"y":0,"x":0}]]})", at("nested", "/nested/0/1"), "/nested/0/1");
    // the forms of read with a flag, with a policy and into a value take the document too
    const auto json = std::string(R"({"spots":[{"x":1,"y":0},{"x":1,"y":0,"z":2}]})");
    EXPECT_EQ(yyjson::read<U>(json, yyjson::ReadFlag::AllowComments).spots.size(), 2);
    EXPECT_EQ(bad_cast_of([&] { (void)yyjson::read<U>(R"({"mixed":[1,1.0]})", yyjson::reject_unknown_keys); }).path(),
              "/mixed/1");
    auto into = U();
    EXPECT_EQ(bad_cast_of([&] { yyjson::read_into(R"({"mixed":[1,1.0]})", into); }).path(), "/mixed/1");

    // validate compares the JSON values that write gives
    expect_invalid(U{.mixed = {1, 1.0}}, at("mixed", "/mixed/1"), "/mixed/1");
    expect_invalid(U{.sets = {{1, 2}, {2, 1}}}, at("sets", "/sets/1"), "/sets/1");
}

TEST(Constraints, FastUnique)
{
    // the C++ values compare as their JSON values, so that the struct reads directly
    static_assert(!yyjson::detail::compares_json_values<FastUnique>);
    static_assert(yyjson::detail::compares_json_values<UniqueJsonEquality>);
    expect_accepted<FastUnique>(
        R"({"ints":[1,2],"strs":["a","b"],"rows":[[1],[1,2]],"two":[1,2],"opts":[null,1],"named":["a","b"],"si":[1,2]})",
        {{1, 2}, {"a", "b"}, {{1}, {1, 2}}, {1, 2}, {std::nullopt, 1}, {FastColor::a, FastColor::b}, {1, 2}});
    for (const auto [key, json] : {std::pair{"ints", "[1,1]"},
                                   {"strs", R"(["a","a"])"},
                                   {"rows", "[[1],[1]]"},
                                   {"two", "[1,1]"},
                                   {"opts", "[null,null]"},
                                   {"named", R"(["a","a"])"},
                                   {"si", "[1,1]"}})
        expect_rejected<FastUnique>(
            std::format(R"({{"{}":{}}})", key, json),
            std::format(R"(the item is equal to an earlier item at /{}/1; reading an element of the value at the )"
                        R"(key "{}" of FastUnique)",
                        key, key),
            std::format("/{}/1", key));
}

TEST(Constraints, UniqueReals2)
{
    constexpr auto big = 9007199254740992.0;
    expect_accepted<UniqueReals2>(
        R"({"v":[9007199254740992,9007199254740993],"s":[9007199254740992,9007199254740993]})",
        {.v = {big, big}, .s = {big}}, false);
    expect_rejected<UniqueReals2>(
        R"({"v":[9007199254740992,9007199254740993,9007199254740992]})",
        R"(the item is equal to an earlier item at /v/2; reading an element of the value at the key "v" of UniqueReals2)",
        "/v/2");
    expect_rejected<UniqueReals2>(
        R"({"n":[9007199254740992,9007199254740993]})",
        R"(the value satisfies the subschema of not at /n; reading the key "n" of UniqueReals2)", "/n");
}

TEST(Constraints, ListedJsonEquality)
{
    using L = ListedJsonEquality;
    expect_rejected<L>(R"({"at":{"x":1,"y":0,"z":2}})",
                       R"(the value is not equal to the constant {"x":1,"y":0} at /at; reading the key "at" of )"
                       "ListedJsonEquality",
                       "/at");
    expect_accepted<L>(R"({"at":{"y":0.0,"x":1}})", {}, false);
    expect_accepted<L>(R"({"not_at":{"x":1,"y":0,"z":2}})", {.not_at = {1, 0}}, false);
    expect_rejected<L>(R"({"not_at":{"x":1,"y":0}})",
                       R"(the value satisfies the subschema of not at /not_at; reading the key "not_at" of )"
                       "ListedJsonEquality",
                       "/not_at");
    expect_rejected<L>(R"({"any_at":{"x":2,"y":0,"k":1}})",
                       R"(the value satisfies none of the subschemas of anyOf at /any_at; reading the key "any_at" )"
                       "of ListedJsonEquality",
                       "/any_at");

    // a constant that the type cannot read throws at its first use
    try
    {
        (void)yyjson::read<BadLiteral>(R"({"at":{"x":1,"y":0}})");
        ADD_FAILURE();
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_TRUE(std::string_view(e.what()).starts_with(
            R"(the JSON text {"x":"a"} of yyjson::const_ or yyjson::enum_ cannot be read as Spot: )"))
            << e.what();
    }
    // a constant that no alternative reads equals no value of the variant
    for (const auto json : {R"({"v":1})", R"({"v":"a"})"})
        expect_rejected<VariantLiteral>(
            json, R"(the value is not equal to the constant {"x":1} at /v; reading the key "v" of VariantLiteral)",
            "/v");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<VariantLiteral>(R"({"v":{"x":1}})"); }).path(), "/v");

    // the keyword of the values of a key map alone takes the document
    static_assert(yyjson::detail::compares_json_values<KeyMapOnly>);
    expect_accepted<KeyMapOnly>(R"({"k":[{"x":1,"y":0},{"x":1,"y":0,"z":1}]})", {{{"k", {{1, 0}, {1, 0}}}}}, false);
    expect_rejected<KeyMapOnly>(R"({"k":[{"x":1,"y":0},{"x":1,"y":0}]})",
                                "the array has duplicate items at /k; reading a member value of KeyMapOnly", "/k");
}

TEST(Constraints, ListedNumbersExact)
{
    using N = ListedNumbersExact;
    expect_rejected<N>(R"({"d":9007199254740993})",
                       R"(the value is not equal to the constant 9007199254740992 at /d; reading the key "d" of )"
                       "ListedNumbersExact",
                       "/d");
    expect_accepted<N>(R"({"d":9007199254740992})", {}, false);
    expect_rejected<N>(R"({"e":18446744073709551616})",
                       R"(the value is not one of 0, 18446744073709551615 at /e; reading the key "e" of )"
                       "ListedNumbersExact",
                       "/e");
    expect_rejected<N>(R"({"f":0.100000001})",
                       R"(the value is not equal to the constant 0.1 at /f; reading the key "f" of ListedNumbersExact)",
                       "/f");
    // the combinator takes the JSON number too
    expect_accepted<N>(R"({"n":9007199254740993})", {.n = 9007199254740992.0}, false);
    expect_rejected<N>(R"({"n":9007199254740992})",
                       R"(the value satisfies the subschema of not at /n; reading the key "n" of ListedNumbersExact)",
                       "/n");
    // the maximum checks the JSON number before the float rounds it
    expect_rejected<N>(R"({"g":16777217})",
                       R"(the number is greater than the maximum 16777216 at /g; reading the key "g" of )"
                       "ListedNumbersExact",
                       "/g");
    expect_accepted<N>(R"({"g":16777216})", {}, false);
    // validate compares the number that write gives
    expect_invalid(
        N{.e = 0x1p64},
        R"(the value is not one of 0, 18446744073709551615 at /e; reading the key "e" of ListedNumbersExact)", "/e");
#if YYJSON_VERSION_HEX >= 0x000B00
    EXPECT_NO_THROW(yyjson::validate(N{}));
#endif
}

TEST(Constraints, MergedExact)
{
    // the merge compares the number of the patch, which the double cannot hold
    auto t = MergedExact{};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"d":9007199254740993})", t); });
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(t, MergedExact{});
    yyjson::merge_into(R"({"d":9007199254740992})", t);
    EXPECT_EQ(t, MergedExact{});
}

TEST(Constraints, ReviewedJsonEquality)
{
    constexpr auto big = 9007199254740992.0;
    // the fields of a struct annotated with `yyjson::as_array` compare the JSON numbers
    static_assert(yyjson::detail::compares_json_values<ArrayExact>);
    expect_rejected<ArrayExact>(
        "[9007199254740993]",
        "the value is not equal to the constant 9007199254740992 at /0; reading an element of ArrayExact", "/0");
    expect_accepted<ArrayExact>("[9007199254740992]", {big}, false);
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<ArrayExactRest>("[0,9007199254740993]"); }).path(), "/1");
    expect_accepted<ArrayExactRest>("[0,9007199254740992]", {0, {big}}, false);

    // a caster that reads a struct with such a keyword reads it from a document
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<ExactWrap>(R"({"d":9007199254740993})"); }).path(), "/d");
    EXPECT_EQ(yyjson::read<ExactWrap>(R"({"d":9007199254740992})"), ExactWrap{{big}});

    // the maximum of the double compares the JSON integer 9007199254740993, so only the integer matches
    EXPECT_EQ(yyjson::read<RoundedOneOf>("9007199254740993").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("9007199254740993")).cast<RoundedOneOf>().index(), 1);
    // both alternatives match: the set reads the two distinct JSON numbers as one double
    const auto ambiguous = []<typename V>(std::type_identity<V>, const std::string& json) {
        for (const auto& e : {bad_cast_of([&] { (void)yyjson::read<V>(json); }),
                              bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<V>(); })})
            EXPECT_NE(std::string_view(e.what()).find("the alternatives 0 and 1 match"), std::string_view::npos)
                << e.what();
    };
    ambiguous(std::type_identity<RealSetOneOf>(), "[9007199254740992,9007199254740993]");
    // the set rejects a repeated JSON number
    EXPECT_EQ(yyjson::read<RealSetOneOf>("[1,1.0]").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("[1,1.0]")).cast<RealSetOneOf>().index(), 1);

    // the merge reads the member as the variant, which cannot read {} under the policies of the subschema
    expect_accepted<MergedVariantLiteral>(R"({"p":{}})", {Spot{}}, false);
    auto merged = MergedVariantLiteral{};
    yyjson::merge_into(R"({"p":{}})", merged);
    EXPECT_EQ(merged, MergedVariantLiteral{Spot{}});

    // a set of keys of a caster compares the JSON elements, whether the set grows or not
    ambiguous(std::type_identity<OneKeyOneOf>(), R"(["a","b"])");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(yyjson::read<KindKeyOneOf>("[1,1.0]").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("[1,1.0]")).cast<KindKeyOneOf>().index(), 1);
    EXPECT_EQ(yyjson::read<KindKeyOneOf>("[1,1.5,2]").index(), 0);
#endif

    // the merge into a member keeps the types of its fields
    auto nested = NestedVariantLiteral{};
    yyjson::merge_into(R"({"c":{"p":{}}})", nested);
    EXPECT_EQ(nested, NestedVariantLiteral{{Spot{}}});
    nested = NestedVariantLiteral{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"c":{"p":{"x":1,"y":0}}})", nested); }).path(), "");
    EXPECT_EQ(nested, NestedVariantLiteral{});

    // pairs of integers compare as arrays and read directly
    static_assert(!yyjson::detail::compares_json_values<UniqueIntPairs>);
    expect_accepted<UniqueIntPairs>(R"({"v":[[1,2],[2,1]]})", {{{1, 2}, {2, 1}}});
    expect_rejected<UniqueIntPairs>(
        R"({"v":[[1,2],[1.0,2]]})",
        R"(the item is equal to an earlier item at /v/1; reading an element of the value at the key "v" of )"
        "UniqueIntPairs",
        "/v/1");
}

TEST(Constraints, PrefixItems)
{
    expect_accepted<Prefixed>(R"({"row":[0,"ab",1.5],"range":[5,5],"head":[0,10],"arr":[0,1,0],)"
                              R"("grid":[[1],[0,0],[0]],"rest":[2,1,0]})",
                              {{0, "ab", 1.5}, {5, 5}, {0, 10}, {0, 1, 0}, {{1}, {0, 0}, {0}}, {2, 1, 0}});
    // an array may be shorter than the prefix
    expect_accepted<Prefixed>(R"({"head":[],"grid":[[1]]})", {.head = {}, .grid = {{1}}});

    const auto prefix = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Prefixed )", key);
    };
    expect_rejected<Prefixed>(
        R"({"row":[-1,"ab",0]})",
        R"(the number is less than the minimum 0 at /row/0; reading an element of the value at the key "row" of Prefixed)",
        "/row/0");
    expect_rejected<Prefixed>(
        R"({"row":[0,"abc",0]})",
        R"(the string is longer than the maximum length 2 at /row/1; reading an element of the value at the key "row" of Prefixed)",
        "/row/1");
    expect_rejected<Prefixed>(
        R"({"row":[0,"AB",0]})",
        R"(the value does not satisfy the predicate at /row/1; reading an element of the value at the key "row" of Prefixed)",
        "/row/1");
    expect_rejected<Prefixed>(
        R"({"range":[6,5]})",
        R"(the number is greater than the maximum 5 at /range/0; reading an element of the value at the key "range" of Prefixed)",
        "/range/0");
    expect_rejected<Prefixed>(
        R"({"range":[5,4]})",
        R"(the number is less than the minimum 5 at /range/1; reading an element of the value at the key "range" of Prefixed)",
        "/range/1");
    // yyjson::items applies to the elements after the prefix
    expect_rejected<Prefixed>(
        R"({"head":[-1]})",
        R"(the number is less than the minimum 0 at /head/0; reading an element of the value at the key "head" of Prefixed)",
        "/head/0");
    expect_rejected<Prefixed>(
        R"({"head":[0,10,10]})",
        R"(the number is greater than the maximum 9 at /head/2; reading an element of the value at the key "head" of Prefixed)",
        "/head/2");
    expect_rejected<Prefixed>(
        R"({"arr":[0,0,0]})",
        R"(the number is less than the minimum 1 at /arr/1; reading an element of the value at the key "arr" of Prefixed)",
        "/arr/1");
    expect_rejected<Prefixed>(
        R"({"grid":[[0]]})",
        R"(the number is less than the minimum 1 at /grid/0/0; reading an element of the value at the key "grid" of Prefixed)",
        "/grid/0/0");
    expect_rejected<Prefixed>(
        R"({"grid":[[1],[0]]})",
        R"(the array has fewer items than the minimum 2 at /grid/1; reading an element of the value at the key "grid" of Prefixed)",
        "/grid/1");
    expect_rejected<Prefixed>(
        R"({"rest":[0,1,1]})",
        R"(the number is less than the minimum 1 at /rest/0; reading an element of the value at the key "rest" of Prefixed)",
        "/rest/0");
    expect_rejected<Prefixed>(
        R"({"rest":[1,1,2]})",
        R"(the number is greater than the maximum 1 at /rest/2; reading an element of the value at the key "rest" of Prefixed)",
        "/rest/2");
}

TEST(Constraints, ElementLiterals)
{
    expect_accepted<ElementLiterals>(R"({"pair":["ab",["yz"]],"days":["2024-01-31"]})",
                                     {{"ab", {"yz"}}, {"2024-01-31"}});
    expect_rejected<ElementLiterals>(
        R"({"pair":["Ab",[]]})",
        R"(the string does not match the pattern "^[a-z]+$" at /pair/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/0");
    expect_rejected<ElementLiterals>(
        R"({"pair":["abd",[]]})",
        R"(the value is not one of "ab", "Ab", "abc" at /pair/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/0");
    expect_rejected<ElementLiterals>(
        R"({"pair":["ab",["y"]]})",
        R"(the value is not equal to the constant "yz" at /pair/1/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/1/0");
    expect_rejected<ElementLiterals>(
        R"({"days":["2024-02-30"]})",
        R"(the string does not have the format "date" at /days/0; reading an element of the value at the key "days" of ElementLiterals)",
        "/days/0");
}

TEST(Constraints, PropertyNames)
{
    expect_accepted<Keyed>(R"({"m":{"ab":1},"u":{"x":1},"deep":{"x":{"ab":1}},"sized":{"a":1,"b":2}})",
                           {{{"ab", 1}}, {{"x", 1}}, {{"x", {{"ab", 1}}}}, {{"a", 1}, {"b", 2}}});

    // the error is at the value of the member whose key it is
    expect_rejected<Keyed>(
        R"({"m":{"ab":1,"abcd":2}})",
        R"(the key is longer than the maximum length 3 at /m/abcd; reading a key of the value at the key "m" of Keyed)",
        "/m/abcd");
    expect_rejected<Keyed>(
        R"({"m":{"A":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/A; reading a key of the value at the key "m" of Keyed)",
        "/m/A");
    expect_rejected<Keyed>(
        R"({"u":{"abc":1}})",
        R"(the key does not satisfy the predicate at /u/abc; reading a key of the value at the key "u" of Keyed)",
        "/u/abc");
    expect_rejected<Keyed>(
        R"({"deep":{"x":{"ab":1,"a":2}}})",
        R"(the key is shorter than the minimum length 2 at /deep/x/a; reading a key of the value at the key "deep" of Keyed)",
        "/deep/x/a");
    expect_rejected<Keyed>(
        R"({"sized":{"c":1}})",
        R"(the key is not one of "a", "b" at /sized/c; reading a key of the value at the key "sized" of Keyed)",
        "/sized/c");
    expect_rejected<Keyed>(
        R"({"sized":{}})",
        R"(the object has fewer properties than the minimum 1 at /sized; reading the key "sized" of Keyed)", "/sized");
}

TEST(Constraints, StructPropertyNames)
{
    expect_accepted<KeyedStruct>(R"({"ab":1,"x":2,"\u00e9\u00e9\u00e9":3})", {.ab = 1, .AB = {}});
    expect_rejected<KeyedStruct>(
        R"({"ab":1,"AB":2})", R"(the key does not match the pattern "^[^A-Z/]+$" at /AB; reading a key of KeyedStruct)",
        "/AB");
    expect_rejected<KeyedStruct>(
        R"({"abcd":1})", "the key is longer than the maximum length 3 at /abcd; reading a key of KeyedStruct", "/abcd");
    expect_rejected<KeyedStruct>(
        R"({"a/b":1})", R"(the key does not match the pattern "^[^A-Z/]+$" at /a~1b; reading a key of KeyedStruct)",
        "/a~1b");
}

TEST(Constraints, EscapedKeys)
{
    // a key is escaped in the path, and its length is counted in code points
    expect_rejected<Keyed>(
        R"({"m":{"a/b":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/a~1b; reading a key of the value at the key "m" of Keyed)",
        "/m/a~1b");
    expect_rejected<Keyed>(
        R"({"m":{"~x":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/~0x; reading a key of the value at the key "m" of Keyed)",
        "/m/~0x");
    expect_accepted<Keyed>(R"({"deep":{"x":{"\u00e9\u00e9":1,"\ud83d\ude00\ud83d\ude00":2}}})",
                           {.deep = {{"x", {{"\xc3\xa9\xc3\xa9", 1}, {"\xf0\x9f\x98\x80\xf0\x9f\x98\x80", 2}}}}});
    expect_rejected<Keyed>(
        R"({"deep":{"x":{"\ud83d\ude00":1}}})",
        R"(the key is shorter than the minimum length 2 at /deep/x/😀; reading a key of the value at the key "deep" of Keyed)",
        "/deep/x/\xf0\x9f\x98\x80");
}

TEST(Constraints, Contains)
{
    expect_accepted<Containing>(R"({"roles":["user","admin"],"v":[10,1,11],"none":[1],"rows":[[1],[5]],"pair":[1,2]})",
                                {{"user", "admin"}, {10, 1, 11}, {1}, {{1}, {5}}, {1, 2}});
    expect_accepted<Containing>(R"({"none":[],"pair":[2,0]})", {.pair = {2, 0}});

    expect_rejected<Containing>(
        R"({"roles":["user"]})",
        R"(the array has no item that satisfies contains at /roles; reading the key "roles" of Containing)", "/roles");
    expect_rejected<Containing>(
        R"({"roles":[]})",
        R"(the array has no item that satisfies contains at /roles; reading the key "roles" of Containing)", "/roles");
    expect_rejected<Containing>(
        R"({"v":[1,10]})",
        R"(the array has fewer than 2 items that satisfy contains at /v; reading the key "v" of Containing)", "/v");
    expect_rejected<Containing>(
        R"({"v":[10,11,12,13]})",
        R"(the array has more than 3 items that satisfy contains at /v; reading the key "v" of Containing)", "/v");
    expect_rejected<Containing>(
        R"({"none":[10,11]})",
        R"(the array has more than 1 item that satisfies contains at /none; reading the key "none" of Containing)",
        "/none");
    expect_rejected<Containing>(
        R"({"rows":[[1],[2]]})",
        R"(the array has no item that satisfies contains at /rows; reading the key "rows" of Containing)", "/rows");
    expect_rejected<Containing>(
        R"({"pair":[1,3]})",
        R"(the array has no item that satisfies contains at /pair; reading the key "pair" of Containing)", "/pair");
}

TEST(Constraints, ContainsCounts)
{
    // min_contains(0) alone accepts any array, max_contains(0) alone still needs one item, so that no array
    // satisfies it, and both reject one
    expect_accepted<Counted>(R"({"min0":[],"both0":[1]})", {.both0 = {1}});
    expect_accepted<Counted>(R"({"max0":[1,10]})", {.max0 = {1, 10}});
    expect_accepted<Counted>(R"({"max0":[]})", {});
    expect_accepted<Counted>(R"({"min0":[1],"both0":[]})", {.min0 = {1}});
    expect_rejected<Counted>(
        R"({"both0":[1,10]})",
        R"(the array has more than 0 items that satisfy contains at /both0; reading the key "both0" of Counted)",
        "/both0");
    // a null element satisfies the keywords of its value, as a null field does
    expect_accepted<Counted>(R"({"opts":[null,1]})", {.opts = {std::nullopt, 1}});
    expect_rejected<Counted>(
        R"({"opts":[null,null]})",
        R"(the array has more than 1 item that satisfies contains at /opts; reading the key "opts" of Counted)",
        "/opts");
}

TEST(Constraints, OptionalRange)
{
    expect_accepted<Counted>(R"({"unique":null,"head":null,"some":null})", {});
    expect_accepted<Counted>(R"({"unique":[1,2],"head":[0,-1],"some":[10]})",
                             {.unique = std::vector{1, 2}, .head = std::vector{0, -1}, .some = std::vector{10}});
    expect_rejected<Counted>(
        R"({"unique":[1,1]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of Counted)",
        "/unique/1");
    expect_rejected<Counted>(
        R"({"head":[-1]})",
        R"(the number is less than the minimum 0 at /head/0; reading an element of the value at the key "head" of Counted)",
        "/head/0");
    expect_rejected<Counted>(
        R"({"some":[1]})",
        R"(the array has no item that satisfies contains at /some; reading the key "some" of Counted)", "/some");
}

TEST(Constraints, ElementsStrict)
{
    // yyjson::strict checks the keywords of the elements as a read without it does
    const auto rejected = [](auto read, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of(read);
        expect_text(message_of(e), message);
        EXPECT_EQ(e.path(), path);
    };
    rejected(
        [] { (void)yyjson::read<Visited>(R"({"u":[1,1],"p":[0,0],"c":[5],"k":{}})", yyjson::strict); },
        R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of Visited)",
        "/u/1");
    rejected(
        [] { (void)yyjson::read<Visited>(R"({"u":[],"p":[-1,0],"c":[5],"k":{}})", yyjson::strict); },
        R"(the number is less than the minimum 0 at /p/0; reading an element of the value at the key "p" of Visited)",
        "/p/0");
    rejected(
        [] {
            auto t = Deep();
            yyjson::read_into(R"({"v":[],"m":{},"k":[{"ab":1}]})", t, yyjson::strict);
        },
        R"(the key is longer than the maximum length 1 at /k/0/ab; reading a key of the value at the key "k" of Deep)",
        "/k/0/ab");
    rejected(
        [] {
            auto t = ElementsVisitable();
            yyjson::merge_into(R"({"v":[1,0]})", t, yyjson::strict);
        },
        R"(the number is not greater than the exclusive minimum 0 at /v/1; reading an element of the value at the key "v" of ElementsVisitable)",
        "/v/1");
    rejected([] { (void)yyjson::read(R"({"u":[],"p":[0,0],"c":[4],"k":{}})").cast<Visited>(yyjson::strict); },
             R"(the array has no item that satisfies contains at /c; reading the key "c" of Visited)", "/c");
}

TEST(Constraints, VisitableElements)
{
    expect_accepted<Visited>(R"({"u":[1,2],"p":[0,-1],"c":[5],"k":{"a":1}})", {{1, 2}, {0, -1}, {5}, {{"a", 1}}});
    expect_rejected<Visited>(
        R"({"u":[1,1],"c":[5]})",
        R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of Visited)",
        "/u/1");
    expect_rejected<Visited>(
        R"({"p":[-1,0],"c":[5]})",
        R"(the number is less than the minimum 0 at /p/0; reading an element of the value at the key "p" of Visited)",
        "/p/0");
    expect_rejected<Visited>(
        R"({"c":[4]})", R"(the array has no item that satisfies contains at /c; reading the key "c" of Visited)", "/c");
    expect_rejected<Visited>(
        R"({"c":[5],"k":{"ab":1}})",
        R"(the key is longer than the maximum length 1 at /k/ab; reading a key of the value at the key "k" of Visited)",
        "/k/ab");
}

TEST(Constraints, DeepElements)
{
    expect_accepted<Deep>(R"({"v":[[[0],[1]]],"m":{"y":[1]},"k":[{"a":1}]})",
                          {{{{0}, {1}}}, {{"y", {1}}}, {{{"a", 1}}}});
    expect_rejected<Deep>(
        R"({"v":[[[0,-1]]]})",
        R"(the number is less than the minimum 0 at /v/0/0/1; reading an element of the value at the key "v" of Deep)",
        "/v/0/0/1");
    expect_rejected<Deep>(
        R"({"v":[[[0],[0]]]})",
        R"(the array has no item that satisfies contains at /v/0/1; reading an element of the value at the key "v" of Deep)",
        "/v/0/1");
    expect_rejected<Deep>(
        R"({"m":{"x":[1],"y":[0]}})",
        R"(the array has no item that satisfies contains at /m/y; reading an element of the value at the key "m" of Deep)",
        "/m/y");
    expect_rejected<Deep>(
        R"({"k":[{"a":1},{"bb":1}]})",
        R"(the key is longer than the maximum length 1 at /k/1/bb; reading a key of the value at the key "k" of Deep)",
        "/k/1/bb");
}

TEST(Constraints, UnusualElements)
{
    expect_accepted<Unusual>(R"({"pts":[[0,0],[0,1]],"sides":{"left":1},"kv":{"a":1},)"
                             R"("dq":[0,-1,9],"ls":[0,1,9],"pm":[[1,2],[1,3]]})",
                             {.pts = {{0, 0}, {0, 1}},
                              .sides = {{Side::left, 1}},
                              .kv = {{"a", 1}},
                              .dq = {0, -1, 9},
                              .ls = {0, 1, 9},
                              .pm = {{1, 2}, {1, 3}}},
                             false);
    // the elements that a user caster reads are converted again to find the equal one
    expect_rejected<Unusual>(
        R"({"pts":[[0,1],[0,1]]})",
        R"(the item is equal to an earlier item at /pts/1; reading an element of the value at the key "pts" of Unusual)",
        "/pts/1");
    // a key that is an enumeration is checked by the name that it is written as
    expect_rejected<Unusual>(
        R"({"sides":{"left":1,"right":2}})",
        R"(the key is not equal to the constant "left" at /sides/right; reading a key of the value at the key "sides" of Unusual)",
        "/sides/right");
    // a range of pairs is read from an object, whose members have the keys, or from an array of pairs
    expect_rejected<Unusual>(
        R"({"kv":{"a":1,"bb":2}})",
        R"(the key is longer than the maximum length 1 at /kv/bb; reading a key of the value at the key "kv" of Unusual)",
        "/kv/bb");
    expect_rejected<Unusual>(
        R"({"kv":[["a",1],["bb",2]]})",
        R"(the object has a key that does not satisfy propertyNames at /kv; reading the key "kv" of Unusual)", "/kv");
    // the prefix and the elements after it, in sequences that are not vectors
    expect_rejected<Unusual>(
        R"({"dq":[-1]})",
        R"(the number is less than the minimum 0 at /dq/0; reading an element of the value at the key "dq" of Unusual)",
        "/dq/0");
    expect_rejected<Unusual>(
        R"({"dq":[0,10]})",
        R"(the number is greater than the maximum 9 at /dq/1; reading an element of the value at the key "dq" of Unusual)",
        "/dq/1");
    expect_rejected<Unusual>(
        R"({"ls":[0,0]})",
        R"(the number is less than the minimum 1 at /ls/1; reading an element of the value at the key "ls" of Unusual)",
        "/ls/1");
    expect_rejected<Unusual>(
        R"({"ls":[0,1,9,10]})",
        R"(the number is greater than the maximum 9 at /ls/3; reading an element of the value at the key "ls" of Unusual)",
        "/ls/3");
    // a multimap whose keys are not strings is an array of pairs, which may repeat one
    expect_rejected<Unusual>(
        R"({"pm":[[1,2],[1,2]]})",
        R"(the item is equal to an earlier item at /pm/1; reading an element of the value at the key "pm" of Unusual)",
        "/pm/1");
    expect_rejected<Unusual>(R"({"pm":[[1,2],[1,3],[1,4]]})",
                             R"(the array has more items than the maximum 2 at /pm; reading the key "pm" of Unusual)",
                             "/pm");
}

// the reader without a document gives the value or leaves the error to the document as the cast of a document does
TEST(Constraints, ElementsDifferential)
{
    const auto same = [](auto read, bool accepted, const char* json) {
        const bool direct = read(json);
        if (accepted)
        {
            EXPECT_TRUE(direct) << json;
        }
    };
    const auto elements = [](const char* json) { return expect_direct_read<Elements>(json); };
    same(elements, true, R"({"scores":[1],"limits":{"a":1},"by_id":[[1,2]],"nested":[["a"]]})");
    for (const auto* json : {R"({"scores":[-1]})", R"({"limits":{"a":101}})", R"({"by_id":[[1,6]]})",
                             R"({"nested":[[]]})", R"({"groups":{"g":[]}})"})
        same(elements, false, json);
    // the unique reals leave Unique to the document
    const auto unique = [](const char* json) { return expect_direct_read<Unique>(json); };
    same(unique, false, R"({"ints":[1,2],"reals":[0.5]})");
    same(unique, false, R"({"ints":[1,1]})");
    same(unique, false, R"({"reals":[0,-0.0]})");
    const auto keyed = [](const char* json) { return expect_direct_read<Keyed>(json); };
    same(keyed, true, R"({"m":{"ab":1},"sized":{"a":1}})");
    same(keyed, false, R"({"m":{"A":1}})");
    same(keyed, false, R"({"sized":{}})");
    const auto containing = [](const char* json) { return expect_direct_read<Containing>(json); };
    same(containing, true, R"({"roles":["admin"],"v":[10,10]})");
    same(containing, false, R"({"v":[10,10,10,10]})");
    const auto prefixed = [](const char* json) {
        const bool read = expect_direct_read<Prefixed>(json);
        return expect_read_into(Prefixed{.head = {5}}, json) && read;
    };
    same(prefixed, true, R"({"row":[0,"ab",1],"head":[0,1]})");
    same(prefixed, false, R"({"row":[0,"abc",1]})");
}

TEST(Constraints, Not)
{
    expect_accepted<Negated>(
        R"({"n":9,"s":"y","v":[0,1],"twice":3,"m":{"a":1},"row":[1,"y"],"c":[1,-1],"holes":[1],"o":3,"nested":[[]]})",
        {9, "y", {0, 1}, 3, {{"a", 1}}, {1, "y"}, {1, -1}, {1}, 3, {{}}});
    expect_accepted<Negated>(R"({"n":20,"o":null})", {.n = 20});

    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Negated )", key);
    };
    expect_rejected<Negated>(R"({"n":10})",
                             R"(the value satisfies the subschema of not at /n; reading the key "n" of Negated)", "/n");
    expect_rejected<Negated>(R"({"n":19})",
                             R"(the value satisfies the subschema of not at /n; reading the key "n" of Negated)", "/n");
    expect_rejected<Negated>(R"({"s":"xy"})",
                             R"(the value satisfies the subschema of not at /s; reading the key "s" of Negated)", "/s");
    // not_(not_(minimum(0))) is minimum(0)
    expect_rejected<Negated>(
        R"({"v":[0,-1]})",
        R"(the value satisfies the subschema of not at /v/1; reading an element of the value at the key "v" of Negated)",
        "/v/1");
    // each yyjson::not_ of a field is checked
    expect_rejected<Negated>(
        R"({"twice":1})", R"(the value satisfies the subschema of not at /twice; reading the key "twice" of Negated)",
        "/twice");
    expect_rejected<Negated>(
        R"({"twice":2})", R"(the value satisfies the subschema of not at /twice; reading the key "twice" of Negated)",
        "/twice");
    expect_rejected<Negated>(
        R"({"m":{"a":0}})",
        R"(the value satisfies the subschema of not at /m/a; reading an element of the value at the key "m" of Negated)",
        "/m/a");
    expect_rejected<Negated>(
        R"({"m":{"_a":1}})",
        R"(the key satisfies the subschema of not at /m/_a; reading a key of the value at the key "m" of Negated)",
        "/m/_a");
    expect_rejected<Negated>(
        R"({"row":[0,"y"]})",
        R"(the value satisfies the subschema of not at /row/0; reading an element of the value at the key "row" of Negated)",
        "/row/0");
    expect_rejected<Negated>(
        R"({"row":[1,"x"]})",
        R"(the value satisfies the subschema of not at /row/1; reading an element of the value at the key "row" of Negated)",
        "/row/1");
    expect_rejected<Negated>(R"({"c":[1,2]})",
                             R"(the array has no item that satisfies contains at /c; reading the key "c" of Negated)",
                             "/c");
    // null satisfies minimum(5), so not_ rejects a null element, but a null field is missing
    expect_rejected<Negated>(
        R"({"holes":[1,null]})",
        R"(the value satisfies the subschema of not at /holes/1; reading an element of the value at the key "holes" of Negated)",
        "/holes/1");
    expect_rejected<Negated>(R"({"o":5})",
                             R"(the value satisfies the subschema of not at /o; reading the key "o" of Negated)", "/o");
    // not_ of a keyword of the elements
    expect_rejected<Negated>(
        R"({"nested":[[1]]})",
        R"(the value satisfies the subschema of not at /nested; reading the key "nested" of Negated)", "/nested");
    expect_rejected<Negated>(
        R"({"nested":[]})",
        R"(the value satisfies the subschema of not at /nested; reading the key "nested" of Negated)", "/nested");
}

TEST(Constraints, IfThenElse)
{
    expect_accepted<Conditional>(R"({"n":12,"v":[10],"e":[0,5],"s":"#abc","m":{"a":"x","b":"y"}})",
                                 {12, {10}, {0, 5}, "#abc", {{"a", "x"}, {"b", "y"}}});
    expect_accepted<Conditional>(R"({"n":5,"v":[1,9],"s":"ab"})", {5, {1, 9}, {}, "ab", {}});

    expect_rejected<Conditional>(
        R"({"n":11})", R"(the value satisfies if and not then at /n; reading the key "n" of Conditional)", "/n");
    expect_rejected<Conditional>(
        R"({"n":6})", R"(the value satisfies neither if nor else at /n; reading the key "n" of Conditional)", "/n");
    expect_rejected<Conditional>(
        R"({"v":[1,10]})", R"(the value satisfies if and not then at /v; reading the key "v" of Conditional)", "/v");
    expect_rejected<Conditional>(
        R"({"e":[0,4]})",
        R"(the value satisfies neither if nor else at /e/1; reading an element of the value at the key "e" of Conditional)",
        "/e/1");
    expect_rejected<Conditional>(
        R"({"s":"#ab"})", R"(the value satisfies if and not then at /s; reading the key "s" of Conditional)", "/s");
    expect_rejected<Conditional>(
        R"({"m":{"a":"w"}})",
        R"(the value satisfies neither if nor else at /m/a; reading an element of the value at the key "m" of Conditional)",
        "/m/a");
}

TEST(Constraints, AllAnyOneOf)
{
    expect_accepted<Composed>(R"({"a":10,"b":-1,"c":4,"v":[0,10],"s":"c","m":{"k":-1},"deep":-5,"rows":[[1,2],[3,4]]})",
                              {10, -1, 4, {0, 10}, "c", {{"k", -1}}, -5, {{1, 2}, {3, 4}}});
    expect_accepted<Composed>(R"({"b":15,"c":3,"s":"a","m":{"k":10},"deep":100,"rows":[[]]})",
                              {.b = 15, .c = 3, .s = "a", .m = {{"k", 10}}, .deep = 100, .rows = {{}}});
    expect_accepted<Composed>(R"({"c":9,"s":"dz","deep":300})", {.c = 9, .s = "dz", .deep = 300});

    expect_rejected<Composed>(
        R"({"a":-2})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"a":3})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"a":12})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of Composed)", "/a");
    expect_rejected<Composed>(
        R"({"b":5})", R"(the value satisfies none of the subschemas of anyOf at /b; reading the key "b" of Composed)",
        "/b");
    expect_rejected<Composed>(
        R"({"b":21})", R"(the value satisfies none of the subschemas of anyOf at /b; reading the key "b" of Composed)",
        "/b");
    expect_rejected<Composed>(
        R"({"c":1})", R"(the value satisfies none of the subschemas of oneOf at /c; reading the key "c" of Composed)",
        "/c");
    expect_rejected<Composed>(
        R"({"c":6})", R"(the value satisfies more than one subschema of oneOf at /c; reading the key "c" of Composed)",
        "/c");
    expect_rejected<Composed>(
        R"({"v":[0,5]})",
        R"(the value satisfies none of the subschemas of anyOf at /v/1; reading an element of the value at the key "v" of Composed)",
        "/v/1");
    expect_rejected<Composed>(
        R"({"s":"e"})", R"(the value satisfies none of the subschemas of anyOf at /s; reading the key "s" of Composed)",
        "/s");
    // minimum(0) and maximum(9) both hold for 5
    expect_rejected<Composed>(
        R"({"m":{"k":5}})",
        R"(the value satisfies more than one subschema of oneOf at /m/k; reading an element of the value at the key "m" of Composed)",
        "/m/k");
    expect_rejected<Composed>(
        R"({"deep":0})",
        R"(the value satisfies none of the subschemas of oneOf at /deep; reading the key "deep" of Composed)", "/deep");
    expect_rejected<Composed>(
        R"({"deep":150})",
        R"(the value satisfies none of the subschemas of oneOf at /deep; reading the key "deep" of Composed)", "/deep");
    expect_rejected<Composed>(
        R"({"rows":[[1],[2]]})",
        R"(the value satisfies none of the subschemas of anyOf at /rows; reading the key "rows" of Composed)", "/rows");
}

TEST(Constraints, DeadBranches)
{
    // a subschema that no value satisfies is false, as in JSON Schema
    expect_accepted<DeadBranches>(R"({"byte":255,"any":3,"text":"ab","one":[1,2],"low":10})",
                                  {255, 3, "ab", {1, 2}, 10});
    expect_accepted<DeadBranches>(R"({"byte":0,"any":-7,"text":"","one":[1,2,3]})", {0, -7, "", {1, 2, 3}});
    expect_rejected<DeadBranches>(
        R"({"any":4})",
        R"(the value satisfies none of the subschemas of anyOf at /any; reading the key "any" of DeadBranches)",
        "/any");
    expect_rejected<DeadBranches>(
        R"({"text":"abc"})",
        R"(the value satisfies neither if nor else at /text; reading the key "text" of DeadBranches)", "/text");
    expect_rejected<DeadBranches>(
        R"({"one":[1]})",
        R"(the value satisfies none of the subschemas of oneOf at /one; reading the key "one" of DeadBranches)",
        "/one");
    expect_rejected<DeadBranches>(
        R"({"low":9})",
        R"(the value satisfies none of the subschemas of anyOf at /low; reading the key "low" of DeadBranches)",
        "/low");
    expect_rejected<DeadBranches>(R"({"byte":256})",
                                  "the number is out of range, got 256 at /byte; reading unsigned char", "/byte");
}

TEST(Constraints, Matches)
{
    expect_accepted<Typed>(
        R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},"text":1,)"
        R"("small":{"a":255}})",
        yyjson::read<Typed>(R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},)"
                            R"("text":1,"small":{"a":255}})"));

    // a point without "y" is not read under yyjson::strict
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1,"y":null}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"y":3}],"point":{"x":1,"y":2}})",
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)",
        "/events");
    expect_rejected<Typed>(
        R"({"ids":[1,1.5],"point":{"x":1,"y":2}})",
        R"(the value satisfies none of the subschemas of anyOf at /ids/1; reading an element of the value at the key "ids" of Typed)",
        "/ids/1");
    expect_rejected<Typed>(R"({"point":[1,2]})",
                           R"(the value does not satisfy matches at /point; reading the key "point" of Typed)",
                           "/point");
    expect_rejected<Typed>(R"({"point":{"x":1,"y":2},"text":"a"})",
                           R"(the value satisfies the subschema of not at /text; reading the key "text" of Typed)",
                           "/text");
    expect_rejected<Typed>(
        R"({"point":{"x":1,"y":2},"small":{"a":256}})",
        R"(the value does not satisfy matches at /small/a; reading an element of the value at the key "small" of Typed)",
        "/small/a");

    // yyjson::reject_unknown_keys of the read around yyjson::matches closes T; other policies do not change it
    constexpr auto unknown = R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"z":3}],"ids":[],"point":{"x":1,"y":2,"z":3},)"
                             R"("text":null,"small":{}})";
    EXPECT_EQ(
        what_without_position(bad_cast_of([&] { (void)yyjson::read<Typed>(unknown, yyjson::reject_unknown_keys); })),
        R"(the array has fewer than 2 items that satisfy contains at /events; reading the key "events" of Typed)");
    EXPECT_NO_THROW((void)yyjson::read<Typed>(unknown, yyjson::strict));
    constexpr auto point = R"({"point":{"x":1,"y":2,"z":3}})";
    EXPECT_NO_THROW((void)yyjson::read<Typed>(point));
    EXPECT_EQ(
        what_without_position(bad_cast_of([&] { (void)yyjson::read<Typed>(point, yyjson::reject_unknown_keys); })),
        R"(the value does not satisfy matches at /point; reading the key "point" of Typed)");
}

TEST(Constraints, MatchesFunction)
{
    const auto matches = [](auto m, const char* json) { return m(yyjson::value(yyjson::read(json))); };
    EXPECT_TRUE(matches(yyjson::matches<int>, "1"));
    EXPECT_FALSE(matches(yyjson::matches<int>, "1.5"));
    EXPECT_FALSE(matches(yyjson::matches<std::uint8_t>, "300"));
    EXPECT_TRUE(matches(yyjson::matches<Point>, R"({"x":1,"y":2,"z":3})"));
    EXPECT_FALSE(matches(yyjson::matches<Point>, R"({"x":1})"));
    EXPECT_TRUE(matches(yyjson::matches<std::optional<int>>, "null"));
    EXPECT_TRUE(yyjson::matches<std::vector<int>>(yyjson::read("[1,2]")));
    EXPECT_FALSE(yyjson::matches<std::vector<int>>(*yyjson::read("[1,true]").as_array()));

    // the kind of the value decides the types that read each value of their kinds, as a read under strict does
    const auto read_strictly = []<typename T>(std::type_identity<T>, const char* json) {
        try
        {
            (void)yyjson::read<std::tuple<T>>(std::format("[{}]", json), yyjson::strict);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    };
    const auto agree = [&]<typename T>(std::type_identity<T> type) {
        for (const auto* json : {"null", "true", "0", "-1", "1.5", "1e300", R"("a")", "[1]", R"({"a":1})"})
        {
            EXPECT_EQ(yyjson::matches<T>(yyjson::value(yyjson::read(json))), read_strictly(type, json))
                << json << " " << yyjson::detail::type_name<T>();
        }
    };
    agree(std::type_identity<bool>());
    agree(std::type_identity<std::nullptr_t>());
    agree(std::type_identity<double>());
    agree(std::type_identity<std::string>());
    agree(std::type_identity<yyjson::value>());
    agree(std::type_identity<yyjson::array>());
    agree(std::type_identity<yyjson::object>());
    agree(std::type_identity<int>());
    agree(std::type_identity<float>());
}

TEST(Constraints, CombinatorsMerge)
{
    // merge_into checks the values of the patch, and a null patch resets the field without a check; the fields
    // that the patch leaves out are checked as they are written
    auto start = Negated();
    EXPECT_THROW(yyjson::merge_into(R"({"n":9})", start), yyjson::bad_cast);
    auto merged = Negated{.n = 5, .row = {1, "y"}, .c = {-1}, .o = 1, .nested = {{}}};
    yyjson::merge_into(R"({"n":9,"o":null})", merged);
    EXPECT_EQ(merged.n, 9);
    EXPECT_FALSE(merged.o.has_value());
    EXPECT_THROW(yyjson::merge_into(R"({"n":11})", merged), yyjson::bad_cast);

    expect_accepted<VisitedCombined>(R"({"n":1,"v":[4,8]})", {1, {4, 8}});
    expect_rejected<VisitedCombined>(
        R"({"n":0})", R"(the value satisfies the subschema of not at /n; reading the key "n" of VisitedCombined)",
        "/n");
    expect_rejected<VisitedCombined>(
        R"({"n":1,"v":[6]})",
        R"(the value satisfies more than one subschema of oneOf at /v/0; reading an element of the value at the key "v" of VisitedCombined)",
        "/v/0");
}

TEST(Constraints, CombinatorsDifferential)
{
    // the reader without a document reads what the document gives, and leaves a rejected value to it
    const auto negated = [](const char* json) { return expect_direct_read<Negated>(json); };
    EXPECT_TRUE(negated(R"({"n":9,"v":[1],"m":{"a":1},"row":[1,"y"],"holes":[1]})"));
    for (const auto* json : {R"({"n":10})", R"({"v":[-1]})", R"({"m":{"_":1}})", R"({"holes":[null]})"})
        (void)negated(json);
    const auto conditional = [](const char* json) { return expect_direct_read<Conditional>(json); };
    EXPECT_TRUE(conditional(R"({"n":12,"e":[0,5]})"));
    (void)conditional(R"({"e":[1]})");
    const auto composed = [](const char* json) { return expect_direct_read<Composed>(json); };
    EXPECT_TRUE(composed(R"({"c":2,"v":[0,11],"s":"b"})"));
    for (const auto* json : {R"({"c":6})", R"({"v":[1]})", R"({"m":{"k":3}})"}) (void)composed(json);
    const auto typed = [](const char* json) { return expect_direct_read<Typed>(json); };
    EXPECT_TRUE(typed(R"({"events":[{"x":1,"y":2},{"x":1,"y":2}],"point":{"x":0,"y":0}})"));
    (void)typed(R"({"events":[{"x":1,"y":2},{"x":1,"y":2}],"point":{"x":0}})");
}

namespace
{
    // every read of `json` into T, alone and as an element, and `merge_into` unless `merged` is false or T() writes
    // members: accepted, or rejected with `message` at `path`; the reader without a document reads the accepted
    // ones as the document does, and `merge_into` follows the merge of `json` into what T() writes
    template <typename T>
    void expect_conditions(std::string_view json, const expected_text& message, std::string_view path = "",
                           bool merged = true)
    {
        merged = merged && writes_no_member<T>();
        static_cast<void>(expect_merge_rule(T(), json));
        const auto text = std::string(json);
        const auto reads = std::array<std::function<void()>, 5>{
            [&] { (void)yyjson::read<T>(text); }, [&] { (void)yyjson::read(text).template cast<T>(); },
            [&] {
                auto t = T();
                yyjson::read_into(text, t);
            },
            [&] { (void)yyjson::read<std::vector<T>>("[" + text + "]"); },
            [&] {
                auto t = T();
                yyjson::merge_into(text, t);
            }};
        for (std::size_t i = 0; i < reads.size() - (merged ? 0 : 1); ++i)
        {
            if (message.text.empty())
                EXPECT_NO_THROW(reads[i]()) << json << " " << i;
            else
            {
                const auto e = bad_cast_of(reads[i]);
                if (i == 3)
                    EXPECT_EQ(message_of(e), at_element(message.text, path)) << json;
                else
                    expect_text(message_of(e), message, json);
                EXPECT_EQ(e.path(), i == 3 ? "/0" + std::string(path) : std::string(path)) << json << " " << i;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        // a type whose keywords compare JSON values reads through a document
        constexpr bool direct = !yyjson::detail::compares_json_values<T>;
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), direct && message.text.empty()) << json;
        EXPECT_TRUE(expect_direct_read<T>(json) == direct || !message.text.empty()) << json;
        EXPECT_TRUE(expect_read_into(T(), json) == direct || !message.text.empty()) << json;
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(T(), json, &merged_directly));
        if (merged)
        {
            EXPECT_EQ(merged_directly, direct && message.text.empty()) << json;
        }
#endif
    }
}  // namespace

TEST(Constraints, StructConditions)
{
    using S = ConditionShape;
    expect_conditions<S>(R"({"kind":"circle","radius":1})", "");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1},"sides":[1,2,3]})", "");
    // another kind takes else_, whose keys are absent, and leaves the radius unchecked
    expect_conditions<S>(R"({"kind":"square","radius":-1})", "");
    expect_conditions<S>(
        R"({"kind":"circle"})",
        R"(a required key is missing; reading the key "radius" of ConditionShape, which yyjson::if_then_else requires)");
    expect_conditions<S>(
        R"({"kind":"circle","radius":-1})",
        R"(the value satisfies if and not then at /radius; reading the key "radius" of ConditionShape)", "/radius");
    expect_conditions<S>(R"({"kind":"square","p":{"x":"a"}})",
                         R"(the value satisfies neither if nor else at /p; reading the key "p" of ConditionShape)",
                         "/p");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1,"y":2}})", "");
    expect_conditions<S>(
        R"({"kind":"square","sides":[1,0,2]})",
        R"(the value satisfies neither if nor else at /sides/1; reading an element of the value at the key "sides" of ConditionShape)",
        "/sides/1");
    expect_conditions<S>(
        R"({"kind":"square","sides":[1,2]})",
        R"(the value satisfies neither if nor else at /sides; reading the key "sides" of ConditionShape)", "/sides");
    // not_ of a key holds for an object without the key, as JSON Schema's `not` of `properties`
    expect_conditions<S>(R"({"kind":"xy"})",
                         "the value satisfies the subschema of not; reading ConditionShape in yyjson::not_");
    expect_conditions<S>(R"({})", "the value satisfies the subschema of not; reading ConditionShape in yyjson::not_");
    // a null that the read takes as a missing key is no key
    expect_conditions<S>(
        R"({"kind":"circle","radius":null})",
        R"(a required key is missing; reading the key "radius" of ConditionShape, which yyjson::if_then_else requires)");

    expect_conditions<Leveled>(R"({"level":4})", "");
    expect_conditions<Leveled>(R"({"level":5,"note":"n"})", "");
    expect_conditions<Leveled>(
        R"({"level":5})",
        R"(a required key is missing; reading the key "note" of Leveled, which yyjson::if_then_else requires)");
    expect_conditions<Leveled>(R"({"level":5,"note":""})",
                               R"(the value satisfies neither if nor else at /note; reading the key "note" of Leveled)",
                               "/note");

    expect_conditions<ConditionVisitable>(R"({"a":1,"b":2})", "");
    expect_conditions<ConditionVisitable>(R"({"b":2})", "");
    expect_conditions<ConditionVisitable>(
        R"({"a":1})",
        R"(a required key is missing; reading the key "b" of ConditionVisitable, which yyjson::if_then_else requires)");
    expect_conditions<ConditionReordered>(R"({"a":1,"b":1,"c":5})", "");
    expect_conditions<ConditionReordered>(
        R"({"a":9,"b":9,"c":1})", "the value satisfies none of the subschemas of anyOf; reading ConditionReordered");
    expect_conditions<ConditionConstructed>(R"({"a_":0})", "");
    expect_conditions<ConditionConstructed>(R"({"a_":1,"b_":0})", "");
    expect_conditions<ConditionConstructed>(
        R"({"a_":1})",
        R"(a required key is missing; reading the key "b_" of ConditionConstructed, which yyjson::if_then_else requires)");

    // the keys of the object tell which fields it has, whatever values the fields have in T()
    expect_conditions<ConditionPresence>(R"({})", "");
    expect_conditions<ConditionPresence>(R"({"c":7})", "");
    expect_conditions<ConditionPresence>(R"({"a":5,"b":0,"c":7,"d":1})", "");
    expect_conditions<ConditionPresence>(R"({"d":1,"a":5})", "");
    expect_conditions<ConditionPresence>(
        R"({"a":5})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"b":0})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"c":7,"b":0})",
        R"(a required key is missing; reading the key "d" of ConditionPresence, which yyjson::if_then_else requires)");
    expect_conditions<ConditionPresence>(
        R"({"a":5,"b":0,"c":7,"e":0,"d":1})",
        "the value satisfies the subschema of not; reading ConditionPresence in yyjson::not_");
    expect_conditions<ConditionPresence>(
        R"({"e":0,"c":7})", "the value satisfies the subschema of not; reading ConditionPresence in yyjson::not_");

    expect_conditions<Contact>(R"({"name":"a","email":"e"})", "");
    expect_conditions<Contact>(R"({"id":1,"phone":"p","age":0})", "");
    // a yyjson::one_of or any_of of yyjson::required("key", ...) names the keys in its message
    expect_conditions<Contact>(
        R"({"email":"e"})",
        R"(the value satisfies none of the subschemas of anyOf; reading Contact without all the keys of one of ["name"], ["id"])");
    expect_conditions<Contact>(
        R"({"name":"a"})",
        R"(the value satisfies none of the subschemas of oneOf; reading Contact without all the keys of one of ["email"], ["phone"])");
    expect_conditions<Contact>(
        R"({"name":"a","email":"e","phone":"p"})",
        R"(the value satisfies more than one subschema of oneOf; reading Contact with all the keys of more than one of ["email"], ["phone"])");
    expect_conditions<Contact>(
        R"({"name":"a","email":"e","age":-1})",
        R"(the value does not satisfy all the subschemas of allOf at /age; reading the key "age" of Contact)", "/age");
    expect_conditions<Contact>(
        R"({"id":0,"email":"e"})",
        "the value does not satisfy all the subschemas of allOf; reading Contact in yyjson::not_");
}

TEST(Constraints, PropertyAndField)
{
    expect_accepted<PropertyMerged>(R"({"v":9})", {9});
    expect_rejected<PropertyMerged>(
        R"({"v":-1})", R"(the number is less than the minimum 0 at /v; reading the key "v" of PropertyMerged)", "/v");
    expect_rejected<PropertyMerged>(
        R"({"v":10})", R"(the number is greater than the maximum 9 at /v; reading the key "v" of PropertyMerged)",
        "/v");

    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1})",
        "");
    expect_conditions<ManyKeys>(R"({"z":1})", "");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1})",
        R"(the value satisfies none of the subschemas of oneOf; reading ManyKeys without all the keys of one of ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1,"z":1})",
        R"(the value satisfies more than one subschema of oneOf; reading ManyKeys with all the keys of more than one of ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");

    expect_conditions<NestedAllOfProperty>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfProperty>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfProperty)",
        "/b");
    expect_conditions<NestedAllOfMixed>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfMixed>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfMixed)",
        "/b");
    expect_conditions<NestedAllOfDeep>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfDeep>(
        R"({"a":1,"b":0})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfDeep)",
        "/b");
    expect_conditions<NestedAllOfObject>(R"({"a":1,"b":{"c":1}})", "");
    expect_conditions<NestedAllOfObject>(
        R"({"a":1,"b":{}})",
        R"(the value does not satisfy all the subschemas of allOf at /b; reading the key "b" of NestedAllOfObject)",
        "/b");

    expect_conditions<PresenceNestedKnown>(R"({"v":{"bb":1,"a":0}})", "");
    expect_conditions<PresenceNestedKnown>(
        R"({"v":{"a":0}})",
        R"(the object does not satisfy properties at /v; reading the key "v" of PresenceNestedKnown)", "/v");
    expect_conditions<PresenceNestedKnown>(
        R"({"v":{"bb":1,"a":-1}})",
        R"(the object does not satisfy properties at /v; reading the key "v" of PresenceNestedKnown)", "/v");
}

TEST(Constraints, StructConditionsOfNull)
{
    // a null that the read takes as a missing key is no key, and the conditions do not check the value that the
    // field keeps
    expect_conditions<NullKeys>(R"({"a":null,"b":1})", "");
    expect_conditions<NullKeys>(
        R"({"a":null})",
        R"(the value satisfies none of the subschemas of oneOf; reading NullKeys without all the keys of one of ["a"], ["b"])");
    expect_conditions<NullKeys>(
        R"({"a":4})",
        R"(the value does not satisfy all the subschemas of allOf at /a; reading the key "a" of NullKeys)", "/a");
    // a null in a field that holds null is a present key, which a merge takes as a removal
    expect_conditions<NullKeys>(R"({"b":1,"c":null,"d":1})", "");
    expect_conditions<NullKeys>(
        R"({"b":1,"c":null})",
        R"(a required key is missing; reading the key "d" of NullKeys, which yyjson::if_then_else requires)", "",
        false);
}

TEST(Constraints, StructConditionsMerged)
{
    // merge_into checks the object after the merge: the members of the patch that are not null, and the fields that
    // the patch does not name and that the write gives, with the values that the merge gives
    auto c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"a","email":"e","age":3})", c);
    EXPECT_EQ(c.age, 3);
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"name":"a","email":"e","age":-1})", c); });
    EXPECT_EQ(message_of(e),
              R"(the value does not satisfy all the subschemas of allOf at /age; reading the key "age" of Contact)")
        << e.what();
    EXPECT_EQ(e.path(), "/age");
    yyjson::merge_into(R"({"age":4})", c);
    EXPECT_EQ(c, (Contact{.name = "a", .email = "e", .age = 4}));
    e = bad_cast_of([&] { yyjson::merge_into(R"({"name":null})", c); });
    EXPECT_EQ(
        message_of(e),
        R"(the value satisfies none of the subschemas of anyOf; reading Contact without all the keys of one of ["name"], ["id"])")
        << e.what();
    c = Contact{.name = "a", .email = "e"};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"phone":"p"})", c); });
    EXPECT_EQ(
        message_of(e),
        R"(the value satisfies more than one subschema of oneOf; reading Contact with all the keys of more than one of ["email"], ["phone"])")
        << e.what();
    c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"b","email":null,"phone":"p"})", c);
    EXPECT_EQ(c, (Contact{.name = "b", .phone = "p"}));
#if YYJSON_VERSION_HEX >= 0x000C00
    auto merged_directly = false;
    static_cast<void>(expect_merge_into(Contact{.name = "a", .email = "e"}, R"({"name":"b","email":"e","phone":"p"})",
                                        &merged_directly));
    EXPECT_FALSE(merged_directly);
    static_cast<void>(expect_merge_into(Contact{.name = "a", .email = "e"}, R"({"name":"b","email":null,"phone":"p"})",
                                        &merged_directly));
    EXPECT_TRUE(merged_directly);
#endif
    // read_into checks the keys of the text that it reads, not the values that `t` held before
    auto p = ConditionPresence{};
    yyjson::read_into(R"({"d":1})", p);
    EXPECT_THROW(yyjson::read_into(R"({"a":1})", p), yyjson::bad_cast);
}

TEST(Constraints, MergedObject)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    // a field equal to its default is not in the object that the write gives, and a field with
    // yyjson::emit_defaults is
    const auto server = yyjson::read<MergedServer>(R"({"host":"a"})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b"})"), R"({"host":"b","port":8080})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b","cert":"x"})"), R"({"host":"b","port":8080,"cert":"x"})");
    rejected(
        expect_merge_rule(server, R"({"port":8080})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedServer, which yyjson::dependent_required requires)");
    const auto secure = yyjson::read<MergedServer>(R"({"host":"a","port":9000,"cert":"x"})");
    EXPECT_EQ(expect_merge_rule(secure, R"({"host":"b"})"), R"({"host":"b","port":9000,"cert":"x"})");
    rejected(
        expect_merge_rule(secure, R"({"cert":null})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedServer, which yyjson::dependent_required requires)");
    EXPECT_EQ(expect_merge_rule(secure, R"({"cert":null,"port":null})"), R"({"host":"a","port":8080})");
    EXPECT_EQ(expect_merge_rule(MergedServer{"a", 8080, "x"}, R"({"cert":null})"), R"({"host":"a","port":8080})");
    rejected(
        expect_merge_rule(MergedEmitted{"a", 8080, "x"}, R"({"cert":null})"),
        R"(the object does not have all the keys that dependentRequired requires; reading the key "cert" of MergedEmitted, which yyjson::dependent_required requires)");

    // the keys that the patch does not name stay in the object
    const auto choice = yyjson::read<MergedChoice>(R"({"a":1})");
    rejected(
        expect_merge_rule(choice, R"({"b":2})"),
        R"(the value satisfies more than one subschema of oneOf; reading MergedChoice with all the keys of more than one of ["a"], ["b"])");
    EXPECT_EQ(expect_merge_rule(choice, R"({"x":1})"), R"({"a":1})");
    EXPECT_EQ(expect_merge_rule(choice, R"({"a":null,"b":2})"), R"({"b":2})");
    rejected(
        expect_merge_rule(choice, R"({"a":null})"),
        R"(the value satisfies none of the subschemas of oneOf; reading MergedChoice without all the keys of one of ["a"], ["b"])");

    // a null removes the key that the object needs
    EXPECT_EQ(expect_merge_rule(MergedRequired{1, {}}, R"({"n":2})"), R"({"id":1,"n":2})");
    rejected(expect_merge_rule(MergedRequired{1, {}}, R"({"id":null})"),
             R"(a required key is missing; reading the key "id" of MergedRequired)");
    EXPECT_THROW(yyjson::merge_into(R"({"id":null})", *std::make_unique<MergedRequired>()), yyjson::bad_cast);

    // the counts take the members of the object after the merge
    const auto counted = [](int a, std::optional<int> b, int z) {
        auto t = MergedCounted{a, b, {}};
        t.z = z;
        return t;
    };
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"b":2})"), R"({"a":1,"b":2})");
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"z":3})"), R"({"a":1,"z":3})");
    rejected(expect_merge_rule(counted(1, {}, 3), R"({"b":2})"),
             "the object has more properties than the maximum 2; reading MergedCounted");
    rejected(expect_merge_rule(counted(1, {}, 0), R"({"b":2,"y":1})"),
             "the object has more properties than the maximum 2; reading MergedCounted");
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 3), R"({"b":2,"z":null})"), R"({"a":1,"b":2})");
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"c":1})"), R"({"a":0,"b":1})");
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":2})"), R"({"a":0,"b":2})");
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":null})"),
             "the object has fewer properties than the minimum 2; reading MergedMinimum");
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"a":null,"b":null,"c":1})"),
             "the object has fewer properties than the minimum 2; reading MergedMinimum");

    // a closed subschema sees the keys of the object after the merge
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"x":2})"), R"({"x":2})");
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"z":1})"), R"({"x":1,"z":1})");
    rejected(expect_merge_rule(MergedClosed{1, {}}, R"({"y":1})"),
             "the value satisfies none of the subschemas of oneOf; reading MergedClosed");
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, 1}, R"({"z":null})"), R"({"x":1})");
    rejected(expect_merge_rule(MergedClosed{1, 1}, R"({"x":null})"),
             "the value satisfies none of the subschemas of oneOf; reading MergedClosed");
}

TEST(Constraints, MergedObjectAtField)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    const auto merge = [](const MergedPath& t, std::string_view patch) {
        return expect_merge_rule<MergedPath, true>(t, patch);
    };
    const auto path = MergedPath{{1, 3}, {}};
    EXPECT_EQ(merge(path, R"({"start":{"x":2}})"), R"({"start":{"x":2,"y":3}})");
    rejected(merge(path, R"({"start":{"y":null}})"),
             R"(the object does not satisfy properties at /start; reading the key "start" of MergedPath)");
    rejected(merge(MergedPath{{1, {}}, {}}, R"({"start":{"x":2}})"),
             R"(the object does not satisfy properties at /start; reading the key "start" of MergedPath)");
    rejected(merge(path, R"({"end":{"x":2}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    rejected(merge(path, R"({"end":{"y":4}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    rejected(merge(path, R"({"end":{"x":2,"y":null}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
    EXPECT_EQ(merge(path, R"({"end":{"x":2,"y":4}})"), R"({"start":{"x":1,"y":3},"end":{"x":2,"y":4}})");
    const auto ended = MergedPath{{1, 3}, MergedPoint{2, 4}};
    EXPECT_EQ(merge(ended, R"({"end":{"x":5}})"), R"({"start":{"x":1,"y":3},"end":{"x":5,"y":4}})");
    rejected(merge(ended, R"({"end":{"y":null}})"),
             R"(the object has fewer properties than the minimum 2 at /end; reading the key "end" of MergedPath)");
}

TEST(Constraints, MergedAbsent)
{
    // a value that is absent, as an empty optional or a new key of a map, reads the patch without the null members
    // of its objects
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    const auto merge = [](std::string_view patch) {
        return expect_merge_rule<MergedAbsent, true>(MergedAbsent(), patch);
    };
    rejected(merge(R"({"req":{"n":1}})"),
             R"(a required key is missing at /req; reading the key "id" of MergedRequired)");
    rejected(merge(R"({"req":{"id":null,"n":1}})"),
             R"(a required key is missing at /req; reading the key "id" of MergedRequired)");
    EXPECT_EQ(merge(R"({"req":{"id":1,"n":null}})"), R"({"req":{"id":1},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"init":{"x":null,"y":4}})"), R"({"init":{"x":5,"y":4},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"point":{"x":null,"y":4}})"), R"({"point":{"x":0,"y":4},"reqs":{},"mins":{}})");
    rejected(merge(R"({"reqs":{"k":{"n":1}}})"),
             R"(a required key is missing at /reqs/k; reading the key "id" of MergedRequired)");
    rejected(merge(R"({"reqs":{"k":{"id":null}}})"),
             R"(a required key is missing at /reqs/k; reading the key "id" of MergedRequired)");
    EXPECT_EQ(merge(R"({"reqs":{"k":{"id":1,"n":null}}})"), R"({"reqs":{"k":{"id":1}},"mins":{}})");
    rejected(merge(R"({"mins":{"k":{"b":1}}})"),
             "the object has fewer properties than the minimum 2 at /mins/k; reading MergedMinimum");
    rejected(merge(R"({"mins":{"k":{"b":1,"c":null}}})"),
             "the object has fewer properties than the minimum 2 at /mins/k; reading MergedMinimum");
    EXPECT_EQ(merge(R"({"mins":{"k":{"a":1,"b":null,"c":2}}})"), R"({"reqs":{},"mins":{"k":{"a":1}}})");
    // a null in an array stays, and so does a null member of an object in an array
    EXPECT_EQ(merge(R"({"arrays":{"v":[1,null],"w":null}})"),
              R"({"arrays":{"items":[],"v":[1,null]},"reqs":{},"mins":{}})");
    EXPECT_TRUE(merge(R"({"arrays":{"items":[{"x":null}],"w":null}})").starts_with("bad cast"));
    // a value that is present takes the null members of the patch
    auto present = MergedAbsent();
    present.point = MergedPoint{1, 2};
    EXPECT_EQ((expect_merge_rule<MergedAbsent, true>(present, R"({"point":{"x":null,"y":4}})")),
              R"({"point":{"x":0,"y":4},"reqs":{},"mins":{}})");
}

TEST(Constraints, MergedAbsentField)
{
    const auto rejected = [](std::string_view result, const expected_text& message) {
        expect_text(result.starts_with("bad cast: ") ? result.substr(10) : result, message);
    };
    // a field that the write omits is absent
    const auto outer = OmittedOuter{.name = "a", .in = {}, .min = {}};
    const auto merge = [](const OmittedOuter& t, std::string_view patch) {
        return expect_merge_rule<OmittedOuter, true>(t, patch);
    };
    rejected(
        merge(outer, R"({"in":{"y":1}})"),
        R"(the object does not have all the keys that dependentRequired requires at /in; reading the key "z" of OmittedInner, which yyjson::dependent_required requires)");
    EXPECT_EQ(merge(outer, R"({"in":{"y":1,"z":2}})"), R"({"name":"a","in":{"y":1,"z":2}})");
    rejected(merge(outer, R"({"min":{"b":1}})"),
             "the object has fewer properties than the minimum 2 at /min; reading MergedMinimum");
    EXPECT_EQ(merge(outer, R"({"min":{"a":1,"b":null,"c":2}})"), R"({"name":"a","in":{"z":0},"min":{"a":1}})");
    auto written = outer;
    written.in.z = 1;
    EXPECT_EQ(merge(written, R"({"in":{"y":1}})"), R"({"name":"a","in":{"y":1,"z":1}})");

    // the counts in a condition count a field that the patch does not name
    rejected(expect_merge_rule(KeptGate{{}, {1}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
             R"(the value satisfies if and not then at /s; reading the key "s" of KeptGate)");
    rejected(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1})"),
             R"(the value satisfies if and not then at /m; reading the key "m" of KeptGate)");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1,"m":{"l":2}})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");

    // a new key of a map of the keys that no field names is absent
    const auto rest = [](std::string_view patch) {
        return expect_merge_rule<MergedRest, true>(MergedRest{.a = 1, .ks = {}, .rest = {{"r", {1, {}}}}}, patch);
    };
    rejected(rest(R"({"x":{"n":1}})"), R"(a required key is missing at /x; reading the key "id" of MergedRequired)");
    rejected(rest(R"({"k":{"n":1}})"), R"(a required key is missing at /k; reading the key "id" of MergedRequired)");
    EXPECT_EQ(rest(R"({"x":{"id":2,"n":null},"k":{"id":3,"n":null}})"),
              R"({"a":1,"k":{"id":3},"r":{"id":1},"x":{"id":2}})");
    EXPECT_EQ(rest(R"({"r":{"n":2}})"), R"({"a":1,"r":{"id":1,"n":2}})");
    rejected(rest(R"({"r":{"id":null}})"),
             R"(a required key is missing at /r; reading the key "id" of MergedRequired)");

    // a field that the write wrote before a merge without a document changed it to its default is not absent in
    // the merge through a document that follows, which the null of the adjacent tag starts
    const auto fallback = [](const WrittenOuter& t, std::string_view patch) {
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(t, patch, &merged_directly));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(merged_directly) << patch;
#endif
        return expect_merge_rule<WrittenOuter, true>(t, patch);
    };
    EXPECT_EQ(fallback(WrittenOuter{{1, 0}, {}, {}}, R"({"in":{"y":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    EXPECT_EQ(fallback(WrittenOuter{{}, {{"a", 1}}, {}}, R"({"m":{"a":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    rejected(fallback(WrittenOuter(), R"({"in":{"y":1},"body":null,"t":null})"),
             R"(a required key is missing at /in; reading the key "z" of WrittenInner)");
}

TEST(Constraints, MergedMapMembers)
{
    const auto rejected = [](std::string_view result) { EXPECT_TRUE(result.starts_with("bad cast: ")) << result; };
    // a keyword on the members in a combinator checks the whole map
    const auto t = MergedMapCombinators{{{"bb", 1}}, {{"bb", 1}}, {{"b", 10}}, {{"a", {{"bb", 1}}}}};
    const auto merge = [&t](std::string_view patch) { return expect_merge_rule<MergedMapCombinators, true>(t, patch); };
    EXPECT_EQ(merge(R"({"no_short":{"a":1}})"),
              R"({"no_short":{"a":1,"bb":1},"one_length":{"bb":1},"no_small":{"b":10},"inner":{"a":{"bb":1}}})");
    rejected(merge(R"({"no_short":{"bb":null,"a":1}})"));
    rejected(merge(R"({"one_length":{"a":1}})"));
    EXPECT_EQ(merge(R"({"one_length":{"cc":1}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1,"cc":1},"no_small":{"b":10},"inner":{"a":{"bb":1}}})");
    EXPECT_EQ(merge(R"({"no_small":{"a":1}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1},"no_small":{"a":1,"b":10},"inner":{"a":{"bb":1}}})");
    rejected(merge(R"({"no_small":{"b":null,"a":1}})"));
    EXPECT_EQ(merge(R"({"inner":{"a":{"c":1}}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1},"no_small":{"b":10},"inner":{"a":{"bb":1,"c":1}}})");
    rejected(merge(R"({"inner":{"a":{"bb":null,"c":1}}})"));
    rejected(merge(R"({"inner":{"z":{"c":1}}})"));

    // the merge keeps the comparator and the allocator of a map
    for (const auto& [patch, expected] : {std::pair{R"({"m":{"c":3}})", R"({"m":{"c":3,"b":2,"a":1}})"},
                                          std::pair{R"({"m":{"c":3,"a":null}})", R"({"m":{"c":3,"b":2}})"}})
    {
        auto ordered = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
        ordered.m = {{"a", 1}, {"b", 2}};
        yyjson::merge_into(patch, ordered);
        EXPECT_TRUE(ordered.m.key_comp().reversed) << patch;
        EXPECT_EQ(written(ordered), expected) << patch;
        auto from_value = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
        from_value.m = {{"a", 1}, {"b", 2}};
        yyjson::detail::merge_text(patch, from_value, yyjson::ReadFlag::NoFlag);
        EXPECT_EQ(written(from_value), written(ordered)) << patch;

        auto resource = std::pmr::monotonic_buffer_resource();
        auto pmr = MergedResource{std::pmr::map<std::string, int>(&resource)};
        pmr.m = {{"a", 1}, {"b", 2}};
        yyjson::merge_into(patch, pmr);
        EXPECT_EQ(pmr.m.get_allocator().resource(), &resource) << patch;
        EXPECT_EQ(written(pmr), written(MergedResource{{pmr.m.begin(), pmr.m.end()}})) << patch;
        yyjson::detail::merge_text(R"({"m":{"d":4,"c":null}})", pmr, yyjson::ReadFlag::NoFlag);
        EXPECT_EQ(pmr.m.get_allocator().resource(), &resource) << patch;
    }
    // the members of the patch in a check of the whole map count the items that their input gives
    const auto sets = [](const MergedSets& start, std::string_view patch) {
        return expect_merge_rule<MergedSets, true>(start, patch);
    };
    EXPECT_EQ(sets(MergedSets{{{"j", {1, 2}}}, {}, {}}, R"({"least":{"k":[1,1]}})"),
              R"({"least":{"j":[1,2],"k":[1]},"most":{},"inner":{}})");
    rejected(sets(MergedSets{{{"j", {1, 2}}}, {}, {}}, R"({"least":{"k":[1]}})"));
    rejected(sets(MergedSets{{}, {{"j", {1}}}, {}}, R"({"most":{"k":[1,1]}})"));
    EXPECT_EQ(sets(MergedSets{{}, {{"j", {1}}}, {}}, R"({"most":{"k":[2]}})"),
              R"({"least":{},"most":{"j":[1],"k":[2]},"inner":{}})");
    EXPECT_EQ(sets(MergedSets{{}, {}, {{"o", {{"j", {1, 2}}}}}}, R"({"inner":{"o":{"k":[1,1]}}})"),
              R"({"least":{},"most":{},"inner":{"o":{"j":[1,2],"k":[1]}}})");
    rejected(sets(MergedSets{{}, {}, {{"o", {{"j", {1, 2}}}}}}, R"({"inner":{"o":{"k":[1]}}})"));
    // a struct merged in place counts the members of its object after the merge, and a struct read into a new key
    // counts those of the patch without its null members
    const auto structs = [](const MergedStructs& start, std::string_view patch) {
        return expect_merge_rule<MergedStructs, true>(start, patch);
    };
    EXPECT_EQ(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"a":{"x":3}}})"),
              R"({"least":{"a":{"x":3,"y":2}},"most":{},"deep":{}})");
    rejected(structs(MergedStructs{{}, {{"a", {1, {}}}}, {}, {}}, R"({"most":{"a":{"y":3}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"x":3}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"x":3,"y":null}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"y":3}}})"));
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"y":3}})"));
    EXPECT_EQ(structs(MergedStructs{{}, {{"a", {1, {}}}}, {}, {}}, R"({"most":{"b":{"y":3,"x":null}}})"),
              R"({"least":{},"most":{"a":{"x":1},"b":{"x":0,"y":3}},"deep":{}})");
    EXPECT_EQ(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"r":{"x":3}})"),
              R"({"least":{},"most":{},"deep":{},"r":{"x":3,"y":2}})");
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"x":3}})"));
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"x":3,"y":null}})"));
    const auto deep = MergedStructs{{}, {}, {}, {{"o", {{"a", {1, 2}}}}}};
    EXPECT_EQ(structs(deep, R"({"deep":{"o":{"a":{"x":3}}}})"),
              R"({"least":{},"most":{},"deep":{"o":{"a":{"x":3,"y":2}}}})");
    rejected(structs(deep, R"({"deep":{"o":{"b":{"y":3}}}})"));
    rejected(structs(deep, R"({"deep":{"p":{"b":{"y":3}}}})"));
    rejected(structs(deep, R"({"deep":{"p":{"b":{"x":1,"y":null}}}})"));
    EXPECT_EQ(structs(deep, R"({"deep":{"p":{"b":{"x":1,"y":2}}}})"),
              R"({"least":{},"most":{},"deep":{"o":{"a":{"x":1,"y":2}},"p":{"b":{"x":1,"y":2}}}})");
#ifdef __cpp_lib_flat_map
    // the counts of a map whose merge moves its members are of the map after the merge
    const auto flat = [](const MergedFlatCounts& start, std::string_view patch) {
        return expect_merge_rule<MergedFlatCounts, true>(start, patch);
    };
    rejected(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"cc":1,"dd":1}})"));
    rejected(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"a":1}})"));
    EXPECT_EQ(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"cc":1}})"),
              R"({"names":{"bb":1,"cc":1},"values":{}})");
    EXPECT_EQ(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":2,"b":null,"c":1}})"),
              R"({"names":{},"values":{"a":2,"c":1}})");
    rejected(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":2,"c":1}})"));
    rejected(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":9}})"));
#endif

    auto ordered = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
    ordered.m = {{"a", 1}, {"b", 2}};
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"c":3,"d":4}})", ordered), yyjson::bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"":3}})", ordered), yyjson::bad_cast);
}

#ifdef __cpp_lib_flat_map
// a map whose elements move when it changes
struct MergedFlat
{
    std::flat_map<std::string, MergedInit> m;
    std::optional<MergedInit> o;
    bool operator==(const MergedFlat&) const = default;
};
TEST(Constraints, MergedAbsentFlatMap)
{
    const auto merge = [](const MergedFlat& t, std::string_view patch) {
        return expect_merge_rule<MergedFlat, true>(t, patch);
    };
    EXPECT_EQ(merge(MergedFlat(), R"({"o":{"x":null},"m":{"a":{"y":1},"c":{"x":null,"y":2}}})"),
              R"({"m":{"a":{"x":5,"y":1},"c":{"x":5,"y":2}},"o":{"x":5}})");
    EXPECT_EQ(merge(MergedFlat{{{"b", {}}}, {}}, R"({"m":{"a":{"y":1},"b":{"y":3},"c":{"y":2}}})"),
              R"({"m":{"a":{"x":5,"y":1},"b":{"x":5,"y":3},"c":{"x":5,"y":2}}})");
}
#endif

TEST(Constraints, MergedObjectRule)
{
    // merge_into accepts a patch when reading the merge of the patch into what `t` writes accepts it
    const auto patches = std::array{R"({})",
                                    R"({"host":"b"})",
                                    R"({"port":8080})",
                                    R"({"port":9000,"cert":"y"})",
                                    R"({"cert":null})",
                                    R"({"port":null})",
                                    R"({"cert":null,"port":null})",
                                    R"({"host":null,"z":1})",
                                    R"({"a":1,"b":null})",
                                    R"({"a":null,"b":2,"x":1})",
                                    R"({"x":1,"z":1})",
                                    R"({"x":null,"z":2,"y":null})",
                                    R"({"id":null})",
                                    R"({"id":3,"n":null})"};
    for (const auto* patch : patches)
    {
        static_cast<void>(expect_merge_rule(MergedServer{"a", 8080, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedServer{"a", 9000, "x"}, patch));
        static_cast<void>(expect_merge_rule(MergedEmitted{"a", 8080, "x"}, patch));
        static_cast<void>(expect_merge_rule(MergedChoice{1, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedChoice{{}, 2}, patch));
        static_cast<void>(expect_merge_rule(MergedRequired{1, 2}, patch));
        static_cast<void>(expect_merge_rule(MergedMinimum{0, 1}, patch));
        static_cast<void>(expect_merge_rule(MergedMinimum{3, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedClosed{1, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedClosed{1, 1}, patch));
        static_cast<void>(expect_merge_rule(Contact{.name = "a", .email = "e"}, patch));
    }
}

TEST(Constraints, StructConditionsOfValues)
{
    expect_conditions<TypeTested>(R"({})", "");
    expect_conditions<TypeTested>(
        R"({"auto":{}})",
        R"(a required key is missing; reading the key "name" of TypeTested, which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"auto":{},"name":"a"})", "");
    expect_conditions<TypeTested>(R"({"auto":[]})", "");
    expect_conditions<TypeTested>(
        R"({"kind":null})", "the value satisfies the subschema of not; reading TypeTested in yyjson::not_", "", false);
    expect_conditions<TypeTested>(R"({"kind":1})", "");
    expect_conditions<TypeTested>(
        R"({"point":{"x":1}})",
        R"(a required key is missing; reading the key "label" of TypeTested, which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"point":{"x":1,"y":2},"label":"l"})", "");
    expect_conditions<TypeTested>(R"({"point":"a"})", "");

    expect_conditions<AlignedConditions>(R"({"xs":[1],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2],"ys":[],"rows":[]})",
        R"(a required key is missing; reading the key "kind" of AlignedConditions, which yyjson::if_then_else requires)");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3],"kind":"few","ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3,4],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2,3,4],"kind":"few","ys":[],"rows":[]})",
        "the value satisfies the subschema of not; reading AlignedConditions in yyjson::not_");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2],"kind":"many","ys":[],"rows":[]})",
        R"(the value satisfies if and not then at /kind; reading the key "kind" of AlignedConditions)", "/kind");

    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""]})", "");
    expect_conditions<PrefixConditions>(
        R"({"v":[],"w":[],"t":[0,""],"d":["a"]})",
        R"(a required key is missing; reading the key "n" of PrefixConditions, which yyjson::if_then_else requires)");
    expect_conditions<PrefixConditions>(
        R"({"v":[],"w":[],"t":[0,""],"d":[]})",
        R"(a required key is missing; reading the key "n" of PrefixConditions, which yyjson::if_then_else requires)");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["b"]})", "");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["a"],"n":1})", "");
}

TEST(Constraints, ClosedConditions)
{
    // `then` is true when the key "kind" satisfies yyjson::if_
    const auto shape = [&](std::string_view json, std::string_view key = "", bool then = true) {
        const auto failure = then ? "if and not then" : "neither if nor else";
        expect_conditions<ConditionClosed>(
            json,
            key.empty() ? ""
                        : std::format("the value satisfies {} at /{}; reading ConditionClosed in yyjson::if_then_else",
                                      failure, key),
            key.empty() ? "" : "/" + std::string(key));
    };
    shape(R"({"kind":"circle","r":1})");
    shape(R"({"r":1,"kind":"circle"})");
    shape(R"({})");
    shape(R"({"kind":"square","w":1})");
    shape(R"({"kind":"circle","r":1,"w":2})", "w");
    shape(R"({"w":2,"kind":"circle","r":1})", "w");
    shape(R"({"kind":"circle","r":1,"x":1})", "x");
    shape(R"({"kind":"square","r":1})", "r", false);
    shape(R"({"x":1})", "x", false);
    // in a subschema, the keys come before the values
    shape(R"({"kind":"circle","r":-1,"w":2})", "w");

    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"n":1})", "");
    expect_conditions<ClosedTagged>(R"({"n":1,"data":"a","kind":"s"})", "");
    expect_conditions<ClosedTagged>(
        R"({"kind":"i","data":1,"n":1,"z":0})",
        "the value satisfies if and not then at /z; reading ClosedTagged in yyjson::if_then_else", "/z");
    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"z":0})", "");

    expect_conditions<ClosedAlternatives>(R"({"type":"c","a":1})", "");
    expect_conditions<ClosedAlternatives>(R"({"a":1,"type":"c"})", "");
    expect_conditions<ClosedAlternatives>(
        R"({"type":"c","a":1,"b":2})",
        "the value satisfies if and not then at /b; reading ClosedAlternative in yyjson::if_then_else", "/b");

    // merge_into checks the keys of the object after the merge
    auto merged = ConditionClosed{"circle", 1.0, {}};
    yyjson::merge_into(R"({"kind":"circle","r":2})", merged);
    EXPECT_EQ(merged.r, 2.0);
    // the key "kind" stays, so yyjson::then applies
    yyjson::merge_into(R"({"r":3})", merged);
    EXPECT_EQ(merged.r, 3.0);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"w":1})", merged); }).path(), "/w");
    // without the key "kind", yyjson::else_ applies, and rejects the key "r" that the merged object keeps
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":null})", merged); });
    expect_text(message_of(e),
                "the value satisfies neither if nor else at /r; reading ConditionClosed in yyjson::if_then_else");
    EXPECT_EQ(e.path(), "/r");
    merged = ConditionClosed{"circle", 1.0, {}};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":"circle","r":1,"w":1})", merged); });
    EXPECT_EQ(e.path(), "/w");
}

TEST(Constraints, CountedConditions)
{
    expect_conditions<ConditionCounted>(R"({})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"pair","a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(R"({"x":2,"kind":"pair","a":1,"y":3})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"one"})", "");
    expect_conditions<ConditionCounted>(
        R"({"kind":"pair","a":1})",
        "the value satisfies if and not then; reading ConditionCounted in yyjson::if_then_else");
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","a":1,"x":2})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else");
    // the keys of no field count too
    expect_conditions<ConditionCounted>(
        R"({"x":1,"y":2,"z":3})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else");
    expect_conditions<ConditionCounted>(R"({"a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","x":2})",
        R"(a required key is missing; reading the key "a" of ConditionCounted, which yyjson::if_then_else requires)");
    expect_conditions<ConditionCounted>(
        R"({"kind":"pair","a":1,"x":2,"y":3,"z":4})",
        "the value satisfies the subschema of not; reading ConditionCounted in yyjson::not_");
    // a null is a key of the object that a read counts, and no key of a patch
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","a":null,"x":2})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else", "", false);
    auto merged = ConditionCounted{};
    yyjson::merge_into(R"({"kind":"one","a":null})", merged);
    EXPECT_EQ(merged.kind, "one");

    expect_conditions<CountedAlternatives>(R"({"type":"c","b":1})", "");
    expect_conditions<CountedAlternatives>(R"({"type":"c"})", "");
    expect_conditions<CountedAlternatives>(
        R"({"type":"c","a":1})",
        R"(a required key is missing; reading the key "b" of CountedAlternative, which yyjson::if_then_else requires)");
    EXPECT_NO_THROW((void)yyjson::read<CountedAlternative>(R"({"a":1})"));

    expect_conditions<ConditionNamed>(R"({"kind":"short","a":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"long","alpha":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"short","alpha":1})",
                                      "the value satisfies if and not then at /alpha; reading a key of ConditionNamed "
                                      "in yyjson::property_names of yyjson::if_then_else",
                                      "/alpha");
}

TEST(Constraints, GroupedConditions)
{
    // in the order of the fields, and out of it
    expect_conditions<ConditionGrouped>(R"({"kind":"pair","a":1})", "");
    expect_conditions<ConditionGrouped>(R"({"b":1,"kind":"pair"})", "");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","a":1,"b":2})",
        R"(the value satisfies if and not then; reading ConditionGrouped with all the keys of more than one of ["a"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","c":1})",
        R"(the value satisfies if and not then; reading ConditionGrouped without all the keys of one of ["a"], ["b"])");
    // the values of the fields come first
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","c":10})",
        R"(the value satisfies if and not then at /c; reading the key "c" of ConditionGrouped)", "/c");
    expect_conditions<ConditionGrouped>(R"({"a":1,"c":2})", "");
    expect_conditions<ConditionGrouped>(R"({"c":2,"b":1})", "");
    expect_conditions<ConditionGrouped>(
        R"({"a":1})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    // the number of the keys comes before the groups
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","x":1,"y":2})",
        "the value satisfies neither if nor else; reading ConditionGrouped in yyjson::if_then_else");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","x":1})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","b":1,"y":2})",
        "the value satisfies neither if nor else; reading ConditionGrouped in yyjson::if_then_else");

    expect_conditions<GroupedAlternatives>(R"({"type":"g"})", "");
    expect_conditions<GroupedAlternatives>(R"({"type":"g","b":1})", "");
    expect_conditions<GroupedAlternatives>(R"({"a":1,"type":"g","b":1})", "");
}

TEST(Constraints, ObjectKeywordsAtPositions)
{
    const auto value = [](std::string_view key) {
        return R"(the value of the key ")" + std::string(key) + R"(" of Positions )";
    };
    expect_conditions<Positions>(
        R"({"field":{"a":1},"list":[{"a":1},{}],"map":{"k":{"a":1}},"alternative":{"type":"p","a":1},)"
        R"("groups":{"a":1},"conditional":{"a":1}})",
        "");
    expect_conditions<Positions>(
        R"({"field":{}})",
        R"(the object has fewer properties than the minimum 1 at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"b":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"a":1,"z":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    // the empty key is not a tag
    expect_conditions<Positions>(
        R"({"field":{"a":1,"":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"a":9}})",
        R"(the object does not satisfy properties at /field; reading the key "field" of Positions)", "/field");
    // a null in a field that cannot hold null is no key
    expect_conditions<Positions>(
        R"({"field":{"a":null,"b":null}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");

    expect_conditions<Positions>(
        R"({"list":[{"a":1},{"a":1,"b":2}]})",
        R"(the object has more properties than the maximum 1 at /list/1; reading an element of the value at the key "list" of Positions)",
        "/list/1");
    expect_conditions<Positions>(
        R"({"list":[{"zz":1}]})",
        R"(the object has a key that does not satisfy propertyNames at /list/0; reading an element of the value at the key "list" of Positions)",
        "/list/0");
    expect_conditions<Positions>(
        R"({"map":{"k":{"a":1,"b":2}}})",
        R"(the value satisfies the subschema of not at /map/k; reading an element of the value at the key "map" of Positions)",
        "/map/k");

    // the internal tag is a key that the alternative has
    expect_conditions<Positions>(
        R"({"alternative":{"type":"o","x":1}})",
        R"(the object has a key that additionalProperties does not allow at /alternative; reading the key "alternative" of Positions)",
        "/alternative");
    expect_conditions<Positions>(
        R"({"alternative":{"a":-1,"type":"p"}})",
        R"(the object does not satisfy properties at /alternative; reading the key "alternative" of Positions)",
        "/alternative");
    expect_conditions<Positions>(R"({"alternative":{"type":"o","a":2}})", "");

    expect_conditions<Positions>(R"({"groups":{"b":1}})", "");
    expect_conditions<Positions>(
        R"({"groups":{"a":1,"b":1}})",
        R"(the object has all the keys that more than one subschema of oneOf requires at /groups; reading the key "groups" of Positions)",
        "/groups");
    expect_conditions<Positions>(
        R"({"groups":{}})",
        R"(the object does not have all the keys that one subschema of oneOf requires at /groups; reading the key "groups" of Positions)",
        "/groups");
    expect_conditions<Positions>(
        R"({"conditional":{"a":1,"b":1}})",
        R"(the value satisfies if and not then at /conditional; reading the key "conditional" of Positions)",
        "/conditional");
    expect_conditions<Positions>(
        R"({"conditional":{"b":1}})",
        R"(the value satisfies neither if nor else at /conditional; reading the key "conditional" of Positions)",
        "/conditional");
    expect_conditions<Positions>(R"({"conditional":{}})", "");

    // merge_into checks the keys of the patch, with the values of the merged struct
    auto merged = Positions{};
    merged.field = Positioned{1, {}};
    yyjson::merge_into(R"({"field":{"a":2}})", merged);
    EXPECT_EQ(merged.field->a, 2);
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"field":{"b":1}})", merged); });
    EXPECT_EQ(
        message_of(e),
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)")
        << e.what();
    // a merge that throws leaves the value unspecified
    merged = Positions{};
    merged.groups = Positioned{1, {}};
    yyjson::merge_into(R"({"groups":{"a":null,"b":1}})", merged);
    EXPECT_EQ(merged.groups->b, 1);
}

TEST(Constraints, DomConditions)
{
    expect_conditions<DomConditions>(R"({"v":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{"good":1},"e":[]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{"bad":null},"e":[]})",
        R"(the value satisfies the subschema of not at /v; reading the key "v" of DomConditions)", "/v");
    // a value that is not an object has every key that yyjson::required names, as in JSON Schema
    expect_conditions<DomConditions>(
        R"({"v":1,"e":[]})", R"(the value satisfies the subschema of not at /v; reading the key "v" of DomConditions)",
        "/v");
    expect_conditions<DomConditions>(R"({"v":{},"o":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"y"},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":1},"e":[]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"o":{"kind":"x"},"e":[]})",
        R"(the value satisfies if and not then at /o; reading the key "o" of DomConditions)", "/o");
    expect_conditions<DomConditions>(
        R"({"v":{},"o":{"kind":"x","x":0},"e":[]})",
        R"(the value satisfies if and not then at /o; reading the key "o" of DomConditions)", "/o");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":"1"},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"e":[{"id":0},1,"a",null,[]]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[{"id":0},{}]})",
        R"(the object does not satisfy properties at /e/1; reading an element of the value at the key "e" of DomConditions)",
        "/e/1");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[{"id":-1}]})",
        R"(the object does not satisfy properties at /e/0; reading an element of the value at the key "e" of DomConditions)",
        "/e/0");
    expect_conditions<DomConditions>(R"({"v":{},"e":[],"m":{"a":{"id":1},"b":2}})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[],"m":{"a":{"id":1},"b":{}}})",
        R"(the object does not satisfy properties at /m/b; reading an element of the value at the key "m" of DomConditions)",
        "/m/b");
}

TEST(Constraints, MultiKindValues)
{
    // the members of a valid object that `members` does not give
    const auto json = [](std::string_view members) {
        auto text = std::string("{") + std::string(members);
        for (const auto& [key, value] : {std::pair{"v", "1"},
                                         {"w", "1"},
                                         {"s", "\"dark\""},
                                         {"list", "[0,1]"},
                                         {"elements", "[]"},
                                         {"c", "true"},
                                         {"u", "\"a\""}})
            if (text.find(std::format(R"("{}":)", key)) == std::string::npos)
                text += std::format(R"({}"{}":{})", text.size() == 1 ? "" : ",", key, value);
        return text + "}";
    };
    for (const auto members :
         {R"("v":1.0,"w":null)", R"("v":"x","w":"ab")", R"("v":null)", R"("s":3,"list":"abc")", R"("list":{"k":1})",
          R"("list":[1,2],"w":5)", R"("list":true)", R"("elements":[1,"a",null,{},[]])", R"("c":[1])", R"("c":{"k":1})",
          R"("c":false)", R"("u":2.5)"})
        expect_conditions<MultiKind>(json(members), "");
    // the listed values on the variants compare JSON values
    static_assert(yyjson::detail::compares_json_values<MultiKind>);
    expect_conditions<MultiKind>(
        json(R"("v":2)"), R"(the value is not one of 1, "x", null at /v; reading the key "v" of MultiKind)", "/v");
    expect_conditions<MultiKind>(
        json(R"("v":[])"), R"(the value is not one of 1, "x", null at /v; reading the key "v" of MultiKind)", "/v");
    expect_conditions<MultiKind>(
        json(R"("w":-1)"), R"(the number is less than the minimum 0 at /w; reading the key "w" of MultiKind)", "/w");
    expect_conditions<MultiKind>(
        json(R"("w":"abc")"),
        R"(the string is longer than the maximum length 2 at /w; reading the key "w" of MultiKind)", "/w");
    expect_conditions<MultiKind>(json(R"("s":"light")"),
                                 R"(the value is not one of "dark", 3 at /s; reading the key "s" of MultiKind)", "/s");
    expect_conditions<MultiKind>(json(R"("s":4)"),
                                 R"(the value is not one of "dark", 3 at /s; reading the key "s" of MultiKind)", "/s");
    expect_conditions<MultiKind>(
        json(R"("list":[1])"),
        R"(the array has fewer items than the minimum 2 at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":"b")"),
        R"(the string does not match the pattern "^a" at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":{"a":1,"b":2})"),
        R"(the object has more properties than the maximum 1 at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":[0,-1])"),
        R"(the number is less than the minimum 0 at /list/1; reading an element of the value at the key "list" of MultiKind)",
        "/list/1");
    expect_conditions<MultiKind>(
        json(R"("elements":[1,2])"),
        R"(the number is not less than the exclusive maximum 2 at /elements/1; reading an element of the value at the key "elements" of MultiKind)",
        "/elements/1");
    expect_conditions<MultiKind>(
        json(R"("elements":[""])"),
        R"(the string is shorter than the minimum length 1 at /elements/0; reading an element of the value at the key "elements" of MultiKind)",
        "/elements/0");
    expect_conditions<MultiKind>(
        json(R"("c":[1,2])"), R"(the array has more items than the maximum 1 at /c; reading the key "c" of MultiKind)",
        "/c");
    expect_conditions<MultiKind>(
        json(R"("c":{})"),
        R"(the object has fewer properties than the minimum 1 at /c; reading the key "c" of MultiKind)", "/c");
    expect_conditions<MultiKind>(
        json(R"("u":1)"), R"(the value satisfies the subschema of not at /u; reading the key "u" of MultiKind)", "/u");
    expect_conditions<MultiKind>(
        json(R"("u":2)"), R"(the value is not one of 1, 2.5, "a" at /u; reading the key "u" of MultiKind)", "/u");
    expect_conditions<MultiKind>(
        json(R"("u":"b")"), R"(the value is not one of 1, 2.5, "a" at /u; reading the key "u" of MultiKind)", "/u");
}

TEST(Constraints, MultiKindWrittenForms)
{
    // the reader without a document counts the elements of the JSON array while it reads them
    expect_accepted<MultiKindWritten>(R"({"p":"x","q":[5]})", {"x", {5, {}, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,2],"q":[1,5]})", {MultiKindPair{1, 2, {}}, {1, 5, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,4,3],"q":[5,6]})", {MultiKindPair{1, 4, {3}}, {5, 6, {}}});
    // null at the position of a std::optional is an error, also in an alternative of a variant
    expect_rejected<MultiKindWritten>(R"({"p":"x","q":[5,null]})",
                                      "the value must not be null at /q/1; reading std::optional<int>", "/q/1");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1,null],"q":[5]})",
        std::format(
            "no alternative accepts the value at /p; reading {}: alternative 0 (MultiKindPair) at /1: the value must "
            "not be null (reading std::optional<int>); alternative 1 ({}): expected a string, got an array",
            yyjson::detail::variant_name<std::variant<MultiKindPair, std::string>>(),
            yyjson::detail::type_name<std::string>()),
        "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1],"q":[5]})",
        R"(the array has fewer items than the minimum 2 at /p; reading the key "p" of MultiKindWritten)", "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1,2,3,4],"q":[5]})",
        R"(the array has more items than the maximum 3 at /p; reading the key "p" of MultiKindWritten)", "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":"x","q":[5,1,2]})",
        R"(the array has more items than the maximum 2 at /q; reading the key "q" of MultiKindWritten)", "/q");
    expect_rejected<MultiKindWritten>(
        R"({"p":"x","q":[1,2]})",
        R"(the array has no item that satisfies contains at /q; reading the key "q" of MultiKindWritten)", "/q");
}

namespace
{
    // every read of `json` under the policy, through a document and without one, throws `message` at `path`
    template <typename T, typename P>
    void rejected_by(P policy, std::string_view json, const expected_text& message, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, policy); }));
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::ReadFlag::AllowComments, policy); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(policy); }));
    }

    template <typename F>
    std::string invalid_argument_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const std::invalid_argument& e)
        {
            return e.what();
        }
        return "no exception";
    }

    std::string pattern_conflict(std::string_view type, std::string_view key)
    {
        return std::format(
            "a field of yyjson::pattern_properties of {} has the key \"{}\", which {} writes for a field or a tag",
            type, key, type);
    }
}  // namespace

TEST(Constraints, PatternProperties)
{
    // a key goes to the first field whose pattern matches it, in the order of the members
    expect_accepted<PatternMaps>(R"({"ax":1,"xb":10,"sv":"t"})", {10, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}});
    expect_accepted<PatternMaps>(R"({"sv":"t","ax":1,"ay":2})", {10, {{"ax", 1}, {"ay", 2}}, {}, {{"sv", "t"}}});
    expect_rejected<PatternMaps>(R"({"ax":"x"})", "expected an integer, got a string at /ax; reading int", "/ax");
    expect_rejected<PatternMaps>(
        R"({"xb":9})", "the number is less than the minimum 10 at /xb; reading a member value of PatternMaps", "/xb");
    expect_rejected<PatternMaps>(
        R"({"aaaa":1})", "the key is longer than the maximum length 3 at /aaaa; reading a key of PatternMaps", "/aaaa");
    // a key that no pattern matches is unknown
    expect_accepted<PatternMaps>(R"({"zz":1,"ax":1})", {10, {{"ax", 1}}, {}, {}});
    rejected_by<PatternMaps>(yyjson::reject_unknown_keys, R"({"ax":1,"zz":1})",
                             "the object has a key that is not allowed at /zz; reading PatternMaps", "/zz");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ab":11,"ax":1,"sv":"t"})", yyjson::reject_unknown_keys),
              (PatternMaps{11, {{"ax", 1}}, {}, {{"sv", "t"}}}));
    rejected_by<PatternMaps>(yyjson::reject_duplicates, R"({"ax":1,"ax":2})",
                             "the object has a duplicate key at /ax; reading PatternMaps", "/ax");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ax":1,"ax":2})"), (PatternMaps{10, {{"ax", 2}}, {}, {}}));
    EXPECT_EQ(yyjson::read<PatternOpen>(R"({"a":1,"p1":3,"zz":1})"), (PatternOpen{1, {{"p1", 3}}}));

    // every later pattern that matches a key checks its value, also when merge_into merges it
    expect_rejected<PatternMaps>(
        R"({"axb":9})", "the number is less than the minimum 10 at /axb; reading a member value of PatternMaps",
        "/axb");
    expect_rejected<PatternMaps>(R"({"sb":"x"})", "expected an integer, got a string at /sb; reading int", "/sb");
    expect_accepted<PatternMaps>(R"({"axb":10})", {10, {{"axb", 10}}, {}, {}});
    // and so does every pattern that matches the key of a field
    expect_rejected<PatternMaps>(
        R"({"ab":9})", "the number is less than the minimum 10 at /ab; reading a member value of PatternMaps", "/ab");
    expect_accepted<PatternMaps>(R"({"ab":12,"ax":1})", {12, {{"ax", 1}}, {}, {}}, false);
    // but not a null that reads as a missing field
    expect_accepted<PatternOptional>(R"({"ab":null,"ax":10})", {{}, {{"ax", 10}}, {}}, false);
    expect_accepted<PatternOptional>(R"({"ab":10})", {10, {}, {}}, false);
    expect_rejected<PatternOptional>(
        R"({"ab":5})", "the number is less than the minimum 10 at /ab; reading a member value of PatternOptional",
        "/ab");
    // a null is an error for a map whose values do not hold null, and merge_into erases the key
    expect_rejected<PatternOptional>(R"({"ax":null})", "expected an integer, got null at /ax; reading int", "/ax",
                                     false);
    EXPECT_EQ(yyjson::read<PatternOptional>(R"({"nx":null})"), (PatternOptional{{}, {}, {{"nx", std::nullopt}}}));
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_TRUE(read_directly<PatternOptional>(R"({"nx":null})", yyjson::ReadFlag::NoFlag).has_value());
#endif

    // the keys that no pattern matches go to the field of `yyjson::extra_keys`
    expect_accepted<PatternRest>(R"({"ax":1,"z":true})", {{{"ax", 1}}, {{"z", true}}});
    EXPECT_EQ(yyjson::read<PatternRest>(R"({"ax":1,"z":true})", yyjson::reject_unknown_keys),
              (PatternRest{{{"ax", 1}}, {{"z", true}}}));
    expect_rejected<PatternRest>(R"({"ax":true})", "expected an integer, got a boolean at /ax; reading int", "/ax");
    expect_rejected<PatternRest>(R"({"z":1})", "expected a boolean, got a number at /z; reading bool", "/z");
    // a closed struct takes the keys that a pattern matches
    expect_accepted<PatternClosed>(R"({"x1":[1]})", {{{"x1", yyjson::read("[1]")}}});
    expect_rejected<PatternClosed>(R"({"x1":1,"y":1})",
                                   "the object has a key that is not allowed at /y; reading PatternClosed, which "
                                   "yyjson::additional_properties(false) closes",
                                   "/y");

    // the map starts from its value in `T()`
    auto seeded = PatternSeeded{5, {{"old", 3}}};
    yyjson::read_into(R"({"ax":2})", seeded);
    EXPECT_EQ(seeded, (PatternSeeded{0, {{"aseed", 1}, {"ax", 2}}}));
    expect_accepted<PatternSeeded>(R"({"aseed":1,"x":1})", {1, {{"aseed", 1}}});

    // the keywords of the struct count and check the keys that the maps receive
    expect_accepted<PatternCounted>(R"({"x":1,"ab":2})", {1, {{"ab", 2}}});
    expect_rejected<PatternCounted>(R"({"x":1})",
                                    "the object has fewer properties than the minimum 2; reading PatternCounted", "");
    expect_rejected<PatternCounted>(R"({"a1":1,"a2":2,"a3":3,"x":1})",
                                    "the object has more properties than the maximum 3; reading PatternCounted", "");
    expect_rejected<PatternCounted>(
        R"({"x":1,"abcd":1})", "the key is longer than the maximum length 3 at /abcd; reading a key of PatternCounted",
        "/abcd");
    // additional_properties(false) in a subschema rejects them
    expect_accepted<PatternConditional>(R"({"kind":"open","p1":2})", {"open", {{"p1", 2}}});
    expect_accepted<PatternConditional>(R"({"kind":"closed"})", {"closed", {}});
    expect_rejected<PatternConditional>(
        R"({"kind":"closed","p1":2})",
        "the value satisfies if and not then at /p1; reading PatternConditional in yyjson::if_then_else", "/p1");

    // the value of a tag whose key a pattern matches satisfies that pattern
    expect_accepted<PatternTagged>(R"({"kind":"s","x":1})", PatternTagShort{1, {}}, false);
    expect_accepted<PatternTagged>(R"({"x":1,"kind":"s","k1":"v"})", PatternTagShort{1, {{"k1", "v"}}}, false);
    expect_accepted<PatternTagged>(R"({"kind":"longer","y":1})", PatternTagOther{1});
    expect_rejected<PatternTaggedLong>(
        R"({"kind":"st","x":1})",
        "the string is longer than the maximum length 1 at /kind; reading a member value of PatternTagShort", "/kind");
    expect_rejected<PatternTaggedShared>(R"({"kind":"i","x":1})",
                                         "expected an integer, got a string at /kind; reading int", "/kind");
    for (const auto json : {R"({"kind":"i","data":1})", R"({"data":1,"kind":"i"})"})
        expect_rejected<PatternAdjacent>(json, "expected an integer, got a string at /kind; reading int", "/kind");

    // a pattern that does not keep the value reads it under reject_missing_keys and reject_null_as_missing
    expect_accepted<PatternStrict>(R"({"ax":{}})", {{{"ax", {}}}, {}, {}});
    expect_accepted<PatternStrict>(R"({"ab":{"x":1}})", {{{"ab", {1}}}, {}, {}}, false);
    expect_rejected<PatternStrict>(
        R"({"ab":{}})", R"(a required key is missing at /ab; reading the key "x" of PatternPoint)", "/ab", false);
    expect_rejected<PatternStrict>(R"({"vb":{"x":null}})", "expected an integer, got null at /vb/x; reading int",
                                   "/vb/x", false);
    EXPECT_EQ(
        yyjson::read<PatternStrict>(R"({"ab":{"x":1}})", yyjson::reject_missing_keys, yyjson::reject_null_as_missing),
        (PatternStrict{{{"ab", {1}}}, {}, {}}));
    // a merge that reads the value again for a later pattern gives the path within the value, as read gives it
    for (const auto& [json, message, path] :
         {std::tuple{R"({"vn":{"in":{"x":"s"}}})", "expected an integer, got a string at /vn/in/x; reading int",
                     "/vn/in/x"},
          std::tuple{R"({"vn":{"in":{}}})",
                     R"(a required key is missing at /vn/in; reading the key "x" of PatternInner)", "/vn/in"},
          std::tuple{R"({"vn":1})", "expected an object, got a number at /vn; reading PatternOuter", "/vn"}})
    {
        const auto read = bad_cast_of([&] { (void)yyjson::read<PatternReread>(json); });
        EXPECT_EQ(what_without_position(read), message) << json;
        const auto merged = bad_cast_of([&] {
            auto t = PatternReread();
            yyjson::detail::merge_text(json, t, yyjson::ReadFlag::NoFlag);
        });
        EXPECT_EQ(what_without_position(merged), message) << json;
        EXPECT_EQ(merged.path(), path) << json;
        // the position is that of the member of the patch
        ASSERT_TRUE(merged.position().has_value()) << json;
        EXPECT_EQ(merged.position()->pos, 6U) << json;
    }

    // the struct inside other types
    expect_accepted<PatternHolder>(
        R"({"v":[{"ax":1}],"m":{"k":{"xb":10}},"o":{"sv":"t"}})",
        {{{10, {{"ax", 1}}, {}, {}}}, {{"k", {10, {}, {{"xb", 10}}, {}}}}, PatternMaps{10, {}, {}, {{"sv", "t"}}}});
    expect_rejected<PatternHolder>(
        R"({"v":[{},{"xb":9}]})",
        "the number is less than the minimum 10 at /v/1/xb; reading a member value of PatternMaps", "/v/1/xb");
    expect_rejected<PatternHolder>(
        R"({"m":{"k":{"axb":9}}})",
        "the number is less than the minimum 10 at /m/k/axb; reading a member value of PatternMaps", "/m/k/axb");
    expect_rejected<PatternHolder>(
        R"({"o":{"aaaa":1}})", "the key is longer than the maximum length 3 at /o/aaaa; reading a key of PatternMaps",
        "/o/aaaa");
}

// a closed struct whose patterns take keys that it does not name, as an alternative of an untagged variant
struct PatternClosedPair
{
    int x = 0;
    int y = 0;
    std::map<std::string, int> extra;
    bool operator==(const PatternClosedPair&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternClosedPair> = yyjson::annotate(
    yyjson::additional_properties(false), yyjson::field("extra", yyjson::pattern_properties("^p")));
struct PatternPlainX
{
    int x = 0;
    bool operator==(const PatternPlainX&) const = default;
};

TEST(Constraints, PatternPropertiesUntagged)
{
    // the struct is a candidate whatever other keys the object has, and its patterns decide whether it reads them
    using x_or_pair = std::variant<PatternPlainX, PatternClosedPair>;
    const auto json = R"({"x":1,"y":2,"p1":3})";
    EXPECT_EQ(yyjson::read<x_or_pair>(json), x_or_pair(PatternClosedPair{1, 2, {{"p1", 3}}}));
    EXPECT_EQ(yyjson::read(json).cast<x_or_pair>(), x_or_pair(PatternClosedPair{1, 2, {{"p1", 3}}}));
    EXPECT_EQ(yyjson::read<x_or_pair>(json, yyjson::reject_unknown_keys).index(), 1);
    EXPECT_EQ(yyjson::read(json).cast<x_or_pair>(yyjson::reject_unknown_keys).index(), 1);
    EXPECT_EQ(yyjson::read<x_or_pair>(R"({"x":1,"y":2,"q":3})").index(), 0);
    EXPECT_TRUE(expect_direct_read<std::vector<x_or_pair>>(R"([{"x":1,"y":2,"p1":3},{"x":1,"y":2,"q":3}])"));
}

TEST(Constraints, PatternPropertiesMerged)
{
    const auto merged = [](PatternMaps t, std::string_view json) {
        yyjson::merge_into(json, t);
        return t;
    };
    const auto rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] {
            auto t = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}};
            yyjson::merge_into(json, t);
        });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto t = PatternMaps{12, {{"ax", 1}, {"axb", 20}}, {{"xb", 10}}, {}};
    // a key of the patch is added or merged into the map of the first pattern, and null erases it
    EXPECT_EQ(merged(t, R"({"ax":null,"ay":2,"xb":11,"sv":"t"})"),
              (PatternMaps{12, {{"axb", 20}, {"ay", 2}}, {{"xb", 11}}, {{"sv", "t"}}}));
    EXPECT_EQ(merged(t, R"({"axb":null,"zz":1})"), (PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}}));
    EXPECT_EQ(merged(t, R"({"axb":30,"ab":11})"), (PatternMaps{11, {{"ax", 1}, {"axb", 30}}, {{"xb", 10}}, {}}));
    // every pattern that matches a key of the patch checks the merged value
    rejected(R"({"axb":5})", "the number is less than the minimum 10 at /axb; reading a member value of PatternMaps",
             "/axb");
    rejected(R"({"ab":5})", "the number is less than the minimum 10 at /ab; reading a member value of PatternMaps",
             "/ab");
    rejected(R"({"sb":"x"})", "expected an integer, got a string at /sb; reading int", "/sb");
    rejected(R"({"ax":"x"})", "expected an integer, got a string at /ax; reading int", "/ax");
    rejected(R"({"aaaa":1})", "the key is longer than the maximum length 3 at /aaaa; reading a key of PatternMaps",
             "/aaaa");
    // a closed struct rejects a key that no pattern matches
    auto closed = PatternClosed{{{"x1", yyjson::read("1")}}};
    yyjson::merge_into(R"({"x1":null,"x2":2})", closed);
    EXPECT_EQ(closed, (PatternClosed{{{"x2", yyjson::read("2")}}}));
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"y":1})", closed); });
    EXPECT_EQ(message_of(e),
              "the object has a key that is not allowed at /y; reading PatternClosed, which "
              "yyjson::additional_properties(false) closes")
        << e.what();
    // the merged values of a field and of a map
    auto optional = PatternOptional{12, {{"ax", 10}}, {}};
    yyjson::merge_into(R"({"ab":null,"ax":11,"nx":null})", optional);
    EXPECT_EQ(optional, (PatternOptional{{}, {{"ax", 11}}, {}}));
    auto rest = PatternRest{{{"ax", 1}}, {{"z", true}}};
    yyjson::merge_into(R"({"ay":2,"z":null,"w":false})", rest);
    EXPECT_EQ(rest, (PatternRest{{{"ax", 1}, {"ay", 2}}, {{"w", false}}}));
}

TEST(Constraints, PatternPropertiesMergedInPlace)
{
    static_assert(yyjson::detail::checks_merged_json<PatternReread>);
    static_assert(yyjson::detail::checks_merged_json<PatternDeclared>);
    // a later pattern checks a value that the merge merges into an entry of the map of the first pattern
    const auto reread = [] {
        auto t = PatternReread();
        t.v.emplace("vn", yyjson::read(R"({"in":{"x":1}})"));
        return t;
    };
    for (const auto& [json, message, path] :
         {std::tuple{R"({"vn":{"in":{"x":"s"}}})", "expected an integer, got a string at /vn/in/x; reading int",
                     "/vn/in/x"},
          std::tuple{R"({"vn":{"in":1}})", "expected an object, got a number at /vn/in; reading PatternInner",
                     "/vn/in"},
          std::tuple{R"({"vn":1})", "expected an object, got a number at /vn; reading PatternOuter", "/vn"}})
    {
        auto t = reread();
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, t); });
        EXPECT_EQ(message_of(e), message) << json;
        EXPECT_EQ(e.path(), path) << json;
        EXPECT_EQ(yyjson::write(t.v.at("vn")), R"({"in":{"x":1}})") << json;
    }
    auto t = reread();
    yyjson::merge_into(R"({"vn":{"in":{"x":3}}})", t);
    EXPECT_EQ(yyjson::write(t.v.at("vn")), R"({"in":{"x":3}})");

    // a pattern checks the value of a declared field that the merge merges in place, as read checks it
    for (const auto& json : {std::string_view(R"({"vn":{"in":{"x":"s"}}})"), std::string_view(R"({"vn":{"in":1}})"),
                             std::string_view(R"({"vn":{"in":{}}})")})
    {
        const auto read = bad_cast_of([&] { (void)yyjson::read<PatternDeclared>(json); });
        auto declared = PatternDeclared();
        const auto merged = bad_cast_of([&] { yyjson::merge_into(json, declared); });
        EXPECT_EQ(message_of(merged), message_of(read)) << json;
        EXPECT_EQ(merged.path(), read.path()) << json;
    }
    auto declared = PatternDeclared();
    yyjson::merge_into(R"({"vn":{"in":{"x":3}}})", declared);
    EXPECT_EQ(yyjson::write(declared.vn.in), R"({"x":3})");
}

TEST(Constraints, PatternPropertiesWrite)
{
    // the maps follow the fields and the tags, in the order of the members, and the field of yyjson::extra_keys
    // follows them
    const auto maps = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}};
    const auto text = R"({"ab":12,"ax":1,"xb":10,"sv":"t"})";
    EXPECT_EQ(std::string(yyjson::write(maps)), text);
    EXPECT_EQ(std::string(yyjson::value(maps).write()), text);
    EXPECT_EQ(std::string(yyjson::write(std::vector{maps})), std::format("[{}]", text));
    EXPECT_EQ(std::string(yyjson::value(std::vector{maps}).write()), std::format("[{}]", text));
    EXPECT_EQ(yyjson::read<PatternMaps>(text), maps);
    EXPECT_EQ(std::string(yyjson::write(PatternMaps())), R"({"ab":10})");
    EXPECT_EQ(std::string(yyjson::write(PatternRest{{{"ax", 1}}, {{"z", true}}})), R"({"ax":1,"z":true})");
    EXPECT_EQ(std::string(yyjson::value(PatternRest{{{"ax", 1}}, {{"z", true}}}).write()), R"({"ax":1,"z":true})");
    const auto tagged = PatternTagged(PatternTagShort{1, {{"k1", "v"}}});
    EXPECT_EQ(std::string(yyjson::write(tagged)), R"({"kind":"s","x":1,"k1":"v"})");
    EXPECT_EQ(std::string(yyjson::value(tagged).write()), R"({"kind":"s","x":1,"k1":"v"})");
    EXPECT_EQ(std::string(yyjson::write(PatternAdjacent{1, {{"k1", 2}}})), R"({"kind":"i","data":1,"k1":2})");
    EXPECT_EQ(std::string(yyjson::value(PatternAdjacent{1, {{"k1", 2}}}).write()), R"({"kind":"i","data":1,"k1":2})");
    // the values and the keys are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(PatternMaps{12, {}, {{"qq", 1}}, {}})), R"({"ab":12,"qq":1})");
    EXPECT_EQ(std::string(yyjson::write(PatternMaps{12, {{"aaaa", 1}}, {}, {}})), R"({"ab":12,"aaaa":1})");

    // a key that the struct writes for a field or a tag
    auto t = maps;
    t.a["ab"] = 3;
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(t); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(t); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(std::vector{t}); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(PatternAdjacent{1, {{"kind", 1}}}); }),
              pattern_conflict("PatternAdjacent", "kind"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(PatternAdjacent{1, {{"kind", 1}}}); }),
              pattern_conflict("PatternAdjacent", "kind"));
    const auto tag_key = PatternTagged(PatternTagShort{1, {{"kind", "v"}}});
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(tag_key); }), pattern_conflict("PatternTagShort", "kind"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(tag_key); }), pattern_conflict("PatternTagShort", "kind"));
    // a key in two maps
    t = maps;
    t.b["ax"] = 30;
    const auto twice = R"(the fields of yyjson::pattern_properties of PatternMaps have the key "ax" more than once)";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(t); }), twice);
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(t); }), twice);
    const auto rest = PatternRest{{{"ay", 1}}, {{"ay", true}}};
    const auto rest_twice =
        R"(the fields of yyjson::pattern_properties and yyjson::extra_keys of PatternRest have the key "ay" more than once)";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(rest); }), rest_twice);
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(rest); }), rest_twice);

    // object_emitter::emplace_members
    EXPECT_EQ(std::string(yyjson::write(PatternEmitted{maps})), R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    EXPECT_EQ(std::string(yyjson::value(PatternEmitted{maps}).write()),
              R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    t = maps;
    t.s["ab"] = "x";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(PatternEmitted{t}); }),
              pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(PatternEmitted{t}); }),
              pattern_conflict("PatternMaps", "ab"));
}

// The alternatives of `yyjson::any_of_variant`, with settings given as values and through a pointer
constexpr std::string_view union_id_regex = "^[a-z]+$";
constexpr auto union_id_pattern = yyjson::pattern(union_id_regex);
constexpr bool union_even(int v) { return v % 2 == 0; }
using UnionValues = yyjson::any_of_variant<
    yyjson::alt<int, yyjson::minimum(0), yyjson::maximum(9), yyjson::satisfies<union_even>>,
    yyjson::alt<int, yyjson::minimum(100), yyjson::enum_(100, 200)>,
    yyjson::alt<std::string, &union_id_pattern, yyjson::max_length(8)>,
    yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(1)), yyjson::contains(yyjson::maximum(3))>,
    std::tuple<int, std::string>>;
struct UnionPoint
{
    int x = 0;
    int y = 0;
    bool operator==(const UnionPoint&) const = default;
};
struct UnionLabel
{
    std::string label;
    bool operator==(const UnionLabel&) const = default;
};
struct UnionClosed
{
    int x = 0;
    bool operator==(const UnionClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UnionClosed> = yyjson::annotate(yyjson::additional_properties(false));
using UnionMap = std::map<std::string, int>;
struct UnionShape
{
    std::string kind;
    double size = 0;
    bool operator==(const UnionShape&) const = default;
};
constexpr auto union_y_needs_x = yyjson::dependent_required("y", {"x"});
// The tag of the variant that holds a union is a key of the object of its alternative.
using UnionTagged = std::variant<
    yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::property("x"), yyjson::additional_properties(false)>>>;
template <>
inline constexpr auto yyjson::annotations<UnionTagged> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
// A union takes no tag of the std::variant that stores its alternatives.
using UnionStorage = std::variant<yyjson::value>;
template <>
inline constexpr auto yyjson::annotations<UnionStorage> = yyjson::annotate(yyjson::internal_tag("kind", {"v"}));
struct UnionFields
{
    UnionValues v;
    yyjson::any_of_variant<UnionPoint, std::string> p;
    bool operator==(const UnionFields&) const = default;
};

namespace
{
    // the reason why an alternative does not read a value, about the value at `at` within it
    struct alternative_reason
    {
        alternative_reason(const char* r, std::string_view a = {}) : reason(r), at(a) {}         // NOLINT
        alternative_reason(const std::string& r, std::string_view a = {}) : reason(r), at(a) {}  // NOLINT
        alternative_reason(std::string_view r, std::string_view a = {}) : reason(r), at(a) {}    // NOLINT
        std::string_view reason;
        std::string_view at;
    };

    // the message of a value at `path` that no alternative of V reads, with the reason of each alternative
    template <typename V>
    std::string read_by_none(std::initializer_list<alternative_reason> reasons, std::string_view path = "")
    {
        const auto names = []<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array{yyjson::detail::variant_name<std::variant_alternative_t<Is, V>>()...};
        }(std::make_index_sequence<std::variant_size_v<V>>());
        auto message =
            std::format("no alternative accepts the value{}; reading {}",
                        path.empty() ? std::string() : " at " + std::string(path), yyjson::detail::variant_name<V>());
        for (auto i = std::size_t(0); const auto& r : reasons)
        {
            message += std::format("{} alternative {} ({}){}: {}", i == 0 ? ":" : ";", i, names[i],
                                   r.at.empty() ? std::string() : " at " + std::string(r.at), r.reason);
            ++i;
        }
        return message;
    }

    // `json` is read into the alternative `index` of V, with and without a document
    template <typename V, typename... Policies>
    void expect_alternative(std::string_view json, std::size_t index, Policies... policies)
    {
        EXPECT_EQ(yyjson::read<V>(json, policies...).index(), index) << json;
        EXPECT_EQ(yyjson::read(json).cast<V>(policies...).index(), index) << json;
    }
}  // namespace

TEST(Constraints, UnionTypes)
{
    // the alternatives of one type are told apart by the index
    auto a = UnionValues(std::in_place_index<1>, 200);
    EXPECT_EQ(a.index(), 1);
    EXPECT_EQ(std::get<1>(a), 200);
    a = std::string("abc");
    EXPECT_TRUE(std::holds_alternative<std::string>(a));
    EXPECT_EQ(std::visit([](const auto& v) { return sizeof(v); }, a), sizeof(std::string));
    const auto b = a;
    EXPECT_EQ(a, b);
    a.emplace<0>(4);
    EXPECT_NE(a, b);
    EXPECT_EQ(*std::get_if<0>(&a), 4);
    static_assert(std::is_same_v<yyjson::any_of_variant<yyjson::alt<int>>::variant_type, std::variant<int>>);
}

TEST(Constraints, UnionReading)
{
    // the first alternative whose type reads the value and whose settings it satisfies
    expect_accepted<UnionValues>("4", UnionValues(std::in_place_index<0>, 4));
    expect_accepted<UnionValues>("200", UnionValues(std::in_place_index<1>, 200));
    expect_accepted<UnionValues>(R"("abc")", UnionValues(std::string("abc")));
    expect_accepted<UnionValues>("[3,1]", UnionValues(std::vector{3, 1}));
    expect_accepted<UnionValues>(R"([5,"a"])", UnionValues(std::tuple(5, std::string("a"))));
    // the reasons of the alternatives for a value of each kind but an array
    const auto kind_reasons = [](std::string_view kind, std::string_view text) {
        const auto got = [&](std::string_view expected) { return std::format("expected {}, got {}", expected, kind); };
        return read_by_none<UnionValues>({got("an integer"), got("an integer"),
                                          text.empty() ? got("a string") : std::string(text), got("an array"),
                                          got("an array")});
    };
    const auto number_reasons = [](std::string_view first, std::string_view path = "") {
        return read_by_none<UnionValues>({first, "the value is not one of 100, 200", "expected a string, got a number",
                                          "expected an array, got a number", "expected an array, got a number"},
                                         path);
    };
    expect_rejected<UnionValues>("5", number_reasons("the value does not satisfy the predicate"), "");
    expect_rejected<UnionValues>("150", number_reasons("the number is greater than the maximum 9"), "");
    expect_rejected<UnionValues>(R"("ABC")",
                                 kind_reasons("a string", R"(the string does not match the pattern "^[a-z]+$")"), "");
    expect_rejected<UnionValues>(R"("abcdefghi")",
                                 kind_reasons("a string", "the string is longer than the maximum length 8"), "");
    expect_rejected<UnionValues>("[5,6]",
                                 read_by_none<UnionValues>({"expected an integer, got an array",
                                                            "expected an integer, got an array",
                                                            "expected a string, got an array",
                                                            "the array has no item that satisfies contains",
                                                            {std::format("expected a string, got a number (reading {})",
                                                                         yyjson::detail::type_name<std::string>()),
                                                             "/1"}}),
                                 "");
    expect_rejected<UnionValues>("1.5", kind_reasons("a number", ""), "");
    expect_rejected<UnionValues>("null", kind_reasons("null", ""), "");
    expect_rejected<UnionFields>(R"({"v":5})", number_reasons("the value does not satisfy the predicate", "/v"), "/v");

    // an earlier alternative of the same type may read the value that a later one held
    using Ints = yyjson::any_of_variant<int, yyjson::alt<int, yyjson::minimum(0)>>;
    EXPECT_EQ(yyjson::read<Ints>(yyjson::write(Ints(std::in_place_index<1>, 5))).index(), 0);

    // the keys of an object rank the structs as those of a std::variant
    using PointOrLabel = yyjson::any_of_variant<UnionPoint, UnionLabel>;
    expect_alternative<PointOrLabel>(R"({"label":"a"})", 1);
    expect_alternative<PointOrLabel>(R"({"x":1})", 0);
    using ClosedOrPoint = yyjson::any_of_variant<UnionClosed, UnionPoint>;
    expect_alternative<ClosedOrPoint>(R"({"x":1})", 0);
    expect_alternative<ClosedOrPoint>(R"({"x":1,"y":2})", 1);
    using PointOrMap = yyjson::any_of_variant<UnionPoint, UnionMap>;
    expect_alternative<PointOrMap>(R"({"x":1,"k":2})", 0);
    expect_alternative<PointOrMap>(R"({"x":1,"k":2})", 1, yyjson::reject_unknown_keys);
    // the settings of a struct alternative check the keys of the object after the struct reads it
    using CountedOrMap = yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::min_properties(2)>, UnionMap>;
    expect_alternative<CountedOrMap>(R"({"x":1,"y":2})", 0);
    expect_alternative<CountedOrMap>(R"({"x":1})", 1);
    // in an alternative, additional_properties(false) accepts only the keys of the tags, as at a field
    using ClosedAltOrMap =
        yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::additional_properties(false)>, UnionMap>;
    expect_alternative<ClosedAltOrMap>("{}", 0);
    expect_alternative<ClosedAltOrMap>(R"({"x":1})", 1);
    expect_alternative<ClosedAltOrMap>(R"({"k":1})", 1);

    // yyjson::field in an alternative constrains a member, as in a field that holds a struct
    using Shapes = yyjson::any_of_variant<yyjson::alt<UnionShape, yyjson::property("kind", yyjson::const_("circle"))>,
                                          yyjson::alt<UnionShape, yyjson::property("kind", yyjson::const_("square"))>>;
    expect_alternative<Shapes>(R"({"kind":"square","size":2})", 1);
    expect_alternative<Shapes>(R"({"kind":"circle"})", 0);
    expect_rejected<Shapes>(R"({"kind":"hexagon"})",
                            "no alternative accepts the value; reading yyjson::any_of_variant<UnionShape, UnionShape>: "
                            "alternative 0 (UnionShape): the object does not satisfy properties; alternative 1 "
                            "(UnionShape): the object does not satisfy properties",
                            "");
    using RequiredX = yyjson::any_of_variant<yyjson::alt<UnionPoint, yyjson::required("x")>, UnionPoint>;
    expect_alternative<RequiredX>(R"({"x":1})", 0);
    expect_alternative<RequiredX>(R"({"y":1})", 1);
    // additional_properties(false) takes only the keys that yyjson::field names, also when it ranks the structs
    using NamedX = yyjson::any_of_variant<
        yyjson::alt<UnionPoint, yyjson::property("x", yyjson::minimum(0)), yyjson::additional_properties(false)>,
        UnionMap>;
    expect_alternative<NamedX>(R"({"x":1})", 0);
    expect_alternative<NamedX>(R"({"x":1,"y":2})", 1);
    expect_alternative<NamedX>(R"({"x":-1})", 1);
    // a setting that holds pointers to strings is given through a pointer before C++26
    using YNeedsX = yyjson::any_of_variant<yyjson::alt<UnionPoint, &union_y_needs_x>, UnionMap>;
    expect_alternative<YNeedsX>(R"({"x":1,"y":2})", 0);
    expect_alternative<YNeedsX>(R"({"y":2})", 1);
    // additional_properties(false) also accepts the tag of the variant that holds the union
    expect_alternative<UnionTagged>(R"({"kind":"p","x":1})", 0);
    expect_alternative<UnionTagged>(R"({"x":1,"kind":"p"})", 0);
    expect_rejected<UnionTagged>(
        R"({"kind":"p","x":1,"z":1})",
        "no alternative accepts the value; reading yyjson::any_of_variant<UnionPoint>: alternative 0 (UnionPoint): the "
        "object has a key that additionalProperties does not allow",
        "");

    // the settings of an alternative reach the value that a variant held in a variant holds, at any depth
    using IntOrText = std::variant<int, std::string>;
    using Nested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<IntOrText, double>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    for (const auto json : {"1", "1.5"})
        expect_rejected<Nested>(json, read_by_none<Nested>({"the number is less than the minimum 5"}), "");
    expect_rejected<Nested>(R"("a")", read_by_none<Nested>({"the string is shorter than the minimum length 2"}), "");
    for (const auto json : {"7", "7.5", R"("ab")", "true"}) EXPECT_NO_THROW((void)yyjson::read<Nested>(json)) << json;
    using NestedListed = yyjson::any_of_variant<yyjson::alt<std::variant<IntOrText, bool>, yyjson::enum_(5, "ab")>>;
    expect_alternative<NestedListed>("5", 0);
    expect_alternative<NestedListed>(R"("ab")", 0);
    expect_rejected<NestedListed>(
        "1",
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 ({}): the value is not one of 5, "ab")",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");
    // read_into and merge_into replace the value
    auto fields = UnionFields{UnionValues(std::in_place_index<0>, 2), UnionPoint{1, 2}};
    yyjson::merge_into(R"({"p":{"x":5}})", fields);
    EXPECT_EQ(fields.p, (yyjson::any_of_variant<UnionPoint, std::string>(UnionPoint{5, 0})));
    yyjson::read_into(R"({"v":"id","p":{"y":3}})", fields);
    EXPECT_EQ(fields, (UnionFields{UnionValues(std::string("id")), UnionPoint{0, 3}}));

    // written as the alternative held, in both writers
    fields = UnionFields{UnionValues(std::in_place_index<1>, 100), std::string("s")};
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"v":100,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::object(fields).write()), R"({"v":100,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::value(UnionValues(std::in_place_index<0>, 99)).write()), "99");
}

TEST(Constraints, UnionDomValues)
{
    // yyjson::matches is a setting of an alternative of a value of the DOM
    using ListOrText =
        yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::matches<std::vector<int>>>, std::string>;
    EXPECT_EQ(yyjson::read("[1,2]").cast<ListOrText>().index(), 0);
    EXPECT_EQ(yyjson::read(R"("a")").cast<ListOrText>().index(), 1);
    EXPECT_THROW((void)yyjson::read(R"([1,"a"])").cast<ListOrText>(), yyjson::bad_cast);

    // the union reads any value, though the std::variant that stores it reads only an object
    static_assert(std::same_as<yyjson::any_of_variant<yyjson::value>::variant_type, UnionStorage>);
    using ValueOrText = std::variant<yyjson::any_of_variant<yyjson::value>, std::string>;
    expect_alternative<ValueOrText>("1", 0);
    expect_alternative<ValueOrText>(R"("a")", 0);
}

// The alternatives of `yyjson::one_of_variant`
struct OneOfX
{
    int x = 0;
    bool operator==(const OneOfX&) const = default;
};
struct OneOfXZ
{
    int x = 0;
    int z = 0;
    bool operator==(const OneOfXZ&) const = default;
};
struct OneOfMaybeX
{
    std::optional<int> x;
    bool operator==(const OneOfMaybeX&) const = default;
};
struct OneOfMaybeXY
{
    std::optional<int> x;
    int y = 0;
    bool operator==(const OneOfMaybeXY&) const = default;
};
struct OneOfY
{
    int y = 0;
    bool operator==(const OneOfY&) const = default;
};
struct OneOfClosedX
{
    int x = 0;
    bool operator==(const OneOfClosedX&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OneOfClosedX> = yyjson::annotate(yyjson::additional_properties(false));
// a caster of the user that takes only a value of the mutable document, which the read gives a copy of
struct OneOfOtherCode
{
    int n = 0;
};
template <>
struct yyjson::caster<OneOfOtherCode>
{
    static OneOfOtherCode from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        return {1};
    }
};
// a range and a map of this file that such a caster reads in place of their elements and values
struct OtherCodeRange : std::vector<int>
{
};
template <>
struct yyjson::caster<OtherCodeRange>
{
    static OtherCodeRange from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_array()) throw yyjson::bad_cast("not an array");
        auto result = OtherCodeRange();
        result.push_back(7);
        return result;
    }
};
struct OtherCodeMap : std::map<std::string, int>
{
};
template <>
struct yyjson::caster<OtherCodeMap>
{
    static OtherCodeMap from_json(const yyjson::writer::const_value_ref& json)
    {
        if (!json.is_object()) throw yyjson::bad_cast("not an object");
        auto result = OtherCodeMap();
        result.emplace("k", 7);
        return result;
    }
};
struct OneOfZ
{
    int z = 0;
    bool operator==(const OneOfZ&) const = default;
};
struct OneOfTextX
{
    std::string x;
    bool operator==(const OneOfTextX&) const = default;
};
struct OneOfNullX
{
    std::nullptr_t x;
    bool operator==(const OneOfNullX&) const = default;
};
struct OneOfXMaybeY
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const OneOfXMaybeY&) const = default;
};
struct OneOfXMaybeZ
{
    int x = 0;
    std::optional<int> z;
    bool operator==(const OneOfXMaybeZ&) const = default;
};
struct OneOfXRest
{
    int x = 0;
    std::map<std::string, int> rest;
    bool operator==(const OneOfXRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OneOfXRest> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct OneOfXAny
{
    int x = 0;
    std::map<std::string, yyjson::value> rest;
};
template <>
inline constexpr auto yyjson::annotations<OneOfXAny> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct OneOfSignedX
{
    std::int64_t x = 0;
    bool operator==(const OneOfSignedX&) const = default;
};
struct OneOfUnsignedX
{
    std::uint64_t x = 0;
    bool operator==(const OneOfUnsignedX&) const = default;
};
template <typename T>
struct OneOfWrapped
{
    T inner;
    bool operator==(const OneOfWrapped&) const = default;
};
template <typename T>
inline constexpr auto yyjson::annotations<OneOfWrapped<T>> = yyjson::annotate(yyjson::transparent);
struct OneOfMany16
{
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
};
struct OneOfMany17
{
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q;
};
// a field of OneOfCode reads a string in place, but `cast<OneOfCode>` reads no value
struct OneOfCode
{
    int n = 0;
};
template <>
struct yyjson::caster<OneOfCode>
{
    template <typename Json>
    static void from_json(OneOfCode& c, const Json& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        c.n = 1;
    }
};
struct OneOfCodeX
{
    OneOfCode x;
};
struct OneOfOtherCodeX
{
    OneOfOtherCode x;
};
// a std::optional of a caster of the user, which reads a string
struct OneOfNumber
{
    int n = 0;
};
template <>
struct yyjson::caster<std::optional<OneOfNumber>>
{
    template <typename Json>
    static std::optional<OneOfNumber> from_json(const Json& json)
    {
        if (!json.is_string()) throw yyjson::bad_cast("not a string");
        return OneOfNumber{1};
    }
};
struct OneOfMaybeNumberX
{
    std::optional<OneOfNumber> x;
};
// a closed struct takes the key of the tag of an enclosing variant
struct OneOfXKind
{
    int x = 0;
    std::string kind;
};
struct OneOfClosedXMaybeKind
{
    int x = 0;
    std::optional<int> kind;
};
template <>
inline constexpr auto yyjson::annotations<OneOfClosedXMaybeKind> =
    yyjson::annotate(yyjson::additional_properties(false));
using OneOfTaggedXKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfXKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedXKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
using OneOfTaggedMaybeKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfClosedXMaybeKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedMaybeKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
struct OneOfXMaybeTextKind
{
    int x = 0;
    std::optional<std::string> kind;
};
using OneOfTaggedTextKind = std::variant<yyjson::one_of_variant<OneOfClosedX, OneOfXMaybeTextKind>>;
template <>
inline constexpr auto yyjson::annotations<OneOfTaggedTextKind> = yyjson::annotate(yyjson::internal_tag("kind", {"p"}));
// a field of an adjacent tag reads the value of the alternative, not the object of its own internal tag
using OneOfPayload = std::variant<long, std::string>;
template <>
inline constexpr auto yyjson::annotations<OneOfPayload> = yyjson::annotate(yyjson::internal_tag("inner", {"i", "s"}));
struct OneOfAdjacentX
{
    OneOfPayload x;
};
template <>
inline constexpr auto yyjson::annotations<OneOfAdjacentX> =
    yyjson::annotate(yyjson::field("x", yyjson::adjacent_tag("kind", {"i", "s"})));
// a transparent struct between two others with a setting of the field of the inner one
struct OneOfMidX
{
    OneOfX b;
};
template <>
inline constexpr auto yyjson::annotations<OneOfMidX> =
    yyjson::annotate(yyjson::transparent, yyjson::field("b", yyjson::property("x", yyjson::minimum(0))));
using OneOfInts = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
inline bool one_of_has_value(const yyjson::nullable<int>& v) { return v.has_value(); }
inline constexpr auto one_of_present = yyjson::satisfies<one_of_has_value>;
inline constexpr auto one_of_object = yyjson::matches<yyjson::object>;
struct OneOfOrder
{
    bool operator()(int a, int b) const { return a < b; }
};
struct OneOfFields
{
    OneOfInts n;
    yyjson::one_of_variant<OneOfX, std::string> p;
    bool operator==(const OneOfFields&) const = default;
};

namespace
{
    // the message of a value that the alternatives i and j of V both read
    template <typename V>
    std::string read_by_two(std::size_t i, std::size_t j)
    {
        return std::format("the value matches the alternatives {} and {} of {}", i, j,
                           yyjson::detail::variant_name<V>());
    }

    // reading `json` into V under `policies` throws `bad_cast` with `message`, with and without a document
    template <typename V, typename... Policies>
    void expect_rejected_under(std::string_view json, const expected_text& message, Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) { expect_text(message_of(e), message, json); };
        check(bad_cast_of([&] { (void)yyjson::read<V>(json, policies...); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<V>(policies...); }));
    }

    // whether an alternative of the `yyjson::one_of_variant` V reads only values that another one reads
    template <typename V>
    constexpr bool one_of_contained =
        yyjson::detail::one_of_relations<V>::contained.contained != std::variant_size_v<V>;
    // whether the alternatives I and J of V may both read a value
    template <typename V, std::size_t I, std::size_t J>
    constexpr bool one_of_overlapping = yyjson::detail::one_of_relations<V>::template overlapping<J>[I];
}  // namespace

TEST(Constraints, OneOfReading)
{
    // the struct that misses a key without a default under yyjson::reject_missing_keys does not match
    using XOrZ = yyjson::one_of_variant<OneOfX, OneOfZ>;
    expect_accepted<XOrZ>(R"({"x":1})", XOrZ(OneOfX{1}));
    // both match an object of both keys, since each one takes other keys, unless yyjson::reject_unknown_keys of
    // the call closes them
    expect_rejected<XOrZ>(R"({"x":1,"z":2,"w":0})",
                          "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                          "OneOfZ>: the alternatives 0 and 1 match",
                          "");
    expect_rejected_under<XOrZ>(R"({"x":1,"z":2})",
                                "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                                "OneOfZ>: the alternatives 0 and 1 match");
    expect_rejected_under<XOrZ>(
        R"({"x":1,"z":2})",
        "no alternative accepts the value; reading yyjson::one_of_variant<OneOfX, OneOfZ>: alternative 0 (OneOfX) at "
        "/z: the object has a key that is not allowed (reading OneOfX); alternative 1 (OneOfZ) at /x: the object has a "
        "key that is not allowed (reading OneOfZ)",
        yyjson::reject_unknown_keys);
    // an alternative may match with none of its keys
    using MaybeXOrY = yyjson::one_of_variant<OneOfMaybeX, OneOfY>;
    expect_rejected<MaybeXOrY>(R"({"y":1})",
                               "the value matches more than one alternative; reading "
                               "yyjson::one_of_variant<OneOfMaybeX, OneOfY>: the alternatives 0 and 1 match",
                               "");
    expect_accepted<MaybeXOrY>(R"({"x":1})", MaybeXOrY(OneOfMaybeX{1}));
    expect_accepted<MaybeXOrY>("{}", MaybeXOrY(OneOfMaybeX{}));
    // a closed struct does not match an object of other keys
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_accepted<ClosedOrXZ>(R"({"x":1,"z":2})", ClosedOrXZ(OneOfXZ{1, 2}));
    expect_accepted<ClosedOrXZ>(R"({"x":1})", ClosedOrXZ(OneOfClosedX{1}));
    expect_rejected<ClosedOrXZ>(
        R"({"z":1})",
        R"(no alternative accepts the value; reading yyjson::one_of_variant<OneOfClosedX, OneOfXZ>: alternative 0 (OneOfClosedX) at /z: the object has a key that is not allowed (reading OneOfClosedX, which yyjson::additional_properties(false) closes); alternative 1 (OneOfXZ): a required key is missing (reading the key "x" of OneOfXZ))",
        "");

    // yyjson::reject_unknown_keys of the call closes the struct alternatives
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_alternative<XOrText>(R"({"x":1,"k":2})", 0);
    EXPECT_THROW((void)yyjson::read<XOrText>(R"({"x":1,"k":2})", yyjson::reject_unknown_keys), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"k":2})").cast<XOrText>(yyjson::reject_unknown_keys), yyjson::bad_cast);
    expect_alternative<XOrText>(R"({"x":1})", 0, yyjson::reject_unknown_keys);
    expect_alternative<XOrText>(R"("a")", 1, yyjson::reject_unknown_keys);
    // so an object of a key that only one of them declares matches that one
    using XOrMaybeXY = yyjson::one_of_variant<OneOfX, OneOfMaybeXY>;
    expect_rejected_under<XOrMaybeXY>(R"({"x":1,"y":2})",
                                      "the value matches more than one alternative; reading "
                                      "yyjson::one_of_variant<OneOfX, OneOfMaybeXY>: the alternatives 0 and 1 match");
    expect_accepted<XOrMaybeXY>(R"({"x":1})", XOrMaybeXY(OneOfX{1}));
    expect_alternative<XOrMaybeXY>(R"({"x":1,"y":2})", 1, yyjson::reject_unknown_keys);
    EXPECT_EQ(std::get<1>(yyjson::read<XOrMaybeXY>(R"({"x":1,"y":2})", yyjson::reject_unknown_keys)),
              (OneOfMaybeXY{1, 2}));
    // a set does not match a repeated element, which `uniqueItems` rejects, under any policies
    using SetOrList = yyjson::one_of_variant<std::set<int>, std::vector<int>>;
    expect_alternative<SetOrList>("[1,1]", 1);
    expect_alternative<SetOrList>("[1,1]", 1, yyjson::reject_duplicates);
    expect_rejected_under<SetOrList>(
        "[1,2]", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                             yyjson::detail::variant_name<SetOrList>()));
    using CountedSet = yyjson::one_of_variant<yyjson::alt<std::set<int>, yyjson::min_items(1)>,
                                              yyjson::alt<std::vector<int>, yyjson::max_items(2)>>;
    expect_alternative<CountedSet>("[1,1]", 1, yyjson::strict);
    // a set of another order has no `uniqueItems`, and no schema rejects a repeated key
    using OrderedOrList = yyjson::one_of_variant<std::set<int, OneOfOrder>, std::vector<int>>;
    expect_rejected_under<OrderedOrList>(
        "[1,1]", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                             yyjson::detail::variant_name<OrderedOrList>()));
    EXPECT_THROW((void)yyjson::read<XOrText>(R"({"x":1,"x":2})", yyjson::reject_duplicates), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"x":2})").cast<XOrText>(yyjson::reject_duplicates), yyjson::bad_cast);

    // the settings of an alternative decide whether it matches
    using SignedX = yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>,
                                           yyjson::alt<OneOfX, yyjson::property("x", yyjson::maximum(-1))>>;
    expect_alternative<SignedX>(R"({"x":1})", 0);
    expect_alternative<SignedX>(R"({"x":-1})", 1);
    expect_accepted<OneOfInts>("-1", OneOfInts(std::in_place_index<1>, -1));
    // the reader without a document leaves a value to it when a later alternative may read it too
    expect_accepted<OneOfInts>("11", OneOfInts(std::in_place_index<0>, 11), false);
    expect_rejected<OneOfInts>("5",
                               "the value matches more than one alternative; reading yyjson::one_of_variant<int, int>: "
                               "the alternatives 0 and 1 match",
                               "");
    expect_rejected<OneOfInts>(
        "1.5",
        "no alternative accepts the value; reading yyjson::one_of_variant<int, int>: alternative 0 (int): expected an "
        "integer, got a number; alternative 1 (int): expected an integer, got a number",
        "");
    expect_rejected<OneOfFields>(R"({"n":5,"p":"a"})",
                                 "the value matches more than one alternative at /n; reading "
                                 "yyjson::one_of_variant<int, int>: the alternatives 0 and 1 match",
                                 "/n");

    // read_into and merge_into replace the value
    auto fields = OneOfFields{OneOfInts(std::in_place_index<0>, 20), OneOfX{1}};
    yyjson::merge_into(R"({"p":{"x":5}})", fields);
    EXPECT_EQ(fields.p, (yyjson::one_of_variant<OneOfX, std::string>(OneOfX{5})));
    yyjson::read_into(R"({"n":-2,"p":"s"})", fields);
    EXPECT_EQ(fields, (OneOfFields{OneOfInts(std::in_place_index<1>, -2), std::string("s")}));

    // written as the alternative held, in both writers
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"n":-2,"p":"s"})");
    EXPECT_EQ(std::string(yyjson::object(fields).write()), R"({"n":-2,"p":"s"})");
    fields.p = OneOfX{3};
    EXPECT_EQ(std::string(yyjson::write(fields)), R"({"n":-2,"p":{"x":3}})");
    EXPECT_EQ(std::string(yyjson::value(OneOfInts(std::in_place_index<0>, 5)).write()), "5");
}

struct UnionReasonField
{
    std::variant<int, UnionPoint> v;
};
struct UnionReasonLine
{
    UnionPoint from;
    std::vector<UnionPoint> to;
};
struct UnionReasonNode
{
    std::variant<int, OneOfX> v;
    yyjson::one_of_variant<std::nullptr_t, yyjson::indirect<UnionReasonNode>> next;
};
struct UnionReasonCount
{
    int x = 0;
    int y = 0;
};
struct UnionReasonText
{
    std::string x;
    int y = 0;
};

namespace
{
    // reading `json` into T under `policies` throws `bad_cast` with exactly `message` about the value at `path`,
    // with and without a document, and from a mutable document
    template <typename T, typename... Policies>
    void expect_reasons(std::string_view json, const expected_text& message, std::string_view path,
                        Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(what_without_position(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, policies...); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(policies...); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(policies...); }));
    }
}  // namespace

TEST(Constraints, UnionFailureReasons)
{
    // a message names the union by the types of its alternatives, without their settings
    EXPECT_EQ((yyjson::detail::variant_name<std::variant<int, bool>>()), "std::variant<int, bool>");
    EXPECT_EQ((yyjson::detail::variant_name<yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>>()),
              "yyjson::any_of_variant<int, bool>");
    EXPECT_EQ((yyjson::detail::variant_name<const yyjson::one_of_variant<int, bool>>()),
              "yyjson::one_of_variant<int, bool>");
    expect_reasons<yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>>(
        "0",
        "no alternative accepts the value; reading yyjson::one_of_variant<int, bool>: alternative 0 (int): the number "
        "is less than the minimum 1; alternative 1 (bool): expected a boolean, got a number",
        "");
    // a union inside a union has the same name in the label of its alternative and in its reason
    expect_reasons<yyjson::any_of_variant<yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>, bool>>(
        "0",
        "no alternative accepts the value; reading yyjson::any_of_variant<yyjson::any_of_variant<int, bool>, bool>: "
        "alternative 0 (yyjson::any_of_variant<int, bool>): no alternative accepts the value (reading "
        "yyjson::any_of_variant<int, bool>); alternative 1 (bool): expected a boolean, got a number",
        "");
    expect_reasons<std::variant<std::variant<int, bool>, bool>>(
        "null",
        "no alternative accepts the value; reading std::variant<std::variant<int, bool>, bool>: alternative 0 "
        "(std::variant<int, bool>): expected a boolean or an integer, got null; alternative 1 (bool): expected a "
        "boolean, got null",
        "");

    // each alternative that does not read the kind of the value
    using Kinds = std::variant<int, std::string, UnionPoint>;
    expect_reasons<Kinds>("true",
                          std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an "
                                      "integer, got a boolean; alternative 1 ({}): expected a string, got a boolean; "
                                      "alternative 2 (UnionPoint): expected an object, got a boolean",
                                      yyjson::detail::variant_name<std::variant<int, std::string, UnionPoint>>(),
                                      yyjson::detail::type_name<std::string>()),
                          "");
    expect_reasons<std::vector<Kinds>>(
        R"([1,"a",null])",
        std::format(
            "no alternative accepts the value at /2; reading {}: alternative 0 (int): expected an integer, got null; "
            "alternative 1 ({}): expected a string, got null; alternative 2 (UnionPoint): expected an object, got null",
            yyjson::detail::variant_name<std::variant<int, std::string, UnionPoint>>(),
            yyjson::detail::type_name<std::string>()),
        "/2");
    // the reason of a struct gives the path of the value within it, and the path of the variant ends the message once
    using IntOrPoint = std::variant<int, UnionPoint>;
    expect_reasons<IntOrPoint>(
        R"({"x":"a"})",
        "no alternative accepts the value; reading std::variant<int, UnionPoint>: alternative 0 (int): expected an "
        "integer, got an object; alternative 1 (UnionPoint) at /x: expected an integer, got a string (reading int)",
        "");
    expect_reasons<UnionReasonField>(
        R"({"v":{"x":"a"}})",
        "no alternative accepts the value at /v; reading std::variant<int, UnionPoint>: alternative 0 (int): expected "
        "an integer, got an object; alternative 1 (UnionPoint) at /x: expected an integer, got a string (reading int)",
        "/v");
    try
    {
        (void)yyjson::read<UnionReasonField>(R"({"v":{"x":"a"}})");
        ADD_FAILURE();
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(std::string(e.what()),
                  "no alternative accepts the value at /v (line 1, column 6, pos 5); reading std::variant<int, "
                  "UnionPoint>: alternative 0 (int): expected an integer, got an object; alternative 1 (UnionPoint) at "
                  "/x: expected an integer, got a string (reading int)");
    }

    // the setting of an alternative that the value does not satisfy
    using NonNegative = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, std::string>;
    expect_reasons<NonNegative>(
        "-1",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): the number is less than the "
                    "minimum 0; alternative 1 ({}): expected a string, got a number",
                    yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                    yyjson::detail::type_name<std::string>()),
        "");

    // the alternatives of yyjson::one_of_variant under the policies that count the matches, the one that the keys
    // of the object prove to fail too
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_reasons<ClosedOrXZ>(
        R"({"z":1})",
        R"(no alternative accepts the value; reading yyjson::one_of_variant<OneOfClosedX, OneOfXZ>: alternative 0 (OneOfClosedX) at /z: the object has a key that is not allowed (reading OneOfClosedX, which yyjson::additional_properties(false) closes); alternative 1 (OneOfXZ): a required key is missing (reading the key "x" of OneOfXZ))",
        "");
    // the one that matches under those policies and fails under the policies of the call gives its own failure
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_reasons<XOrText>(R"({"x":1,"x":2})", "the object has a duplicate key at /x; reading OneOfX", "/x",
                            yyjson::reject_duplicates);
    // also a variant within it, which gives the reasons of its alternatives
    expect_reasons<UnionReasonNode>(
        R"({"v":1,"next":{"v":{"x":1,"x":2},"next":null}})",
        "no alternative accepts the value at /next/v; reading std::variant<int, OneOfX>: alternative 0 (int): expected "
        "an integer, got an object; alternative 1 (OneOfX) at /x: the object has a duplicate key (reading OneOfX)",
        "/next/v", yyjson::reject_duplicates);
    // yyjson::reject_unknown_keys of the call closes the struct alternatives before the count
    expect_reasons<XOrText>(
        R"({"x":1,"k":2})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (OneOfX) at /k: the object has a key "
                    "that is not allowed (reading OneOfX); alternative 1 ({}): expected a string, got an object",
                    yyjson::detail::variant_name<XOrText>(), yyjson::detail::type_name<std::string>()),
        "", yyjson::reject_unknown_keys);

    // the path within an alternative goes through its members and elements, and a failure of the whole value has none
    using IntOrLine = std::variant<int, UnionReasonLine>;
    expect_reasons<IntOrLine>(R"({"from":{"x":1,"y":2},"to":[{"x":3,"y":4},{"x":5,"y":true}]})",
                              "no alternative accepts the value; reading std::variant<int, UnionReasonLine>: "
                              "alternative 0 (int): expected an integer, got an object; alternative 1 "
                              "(UnionReasonLine) at /to/1/y: expected an integer, got a boolean (reading int)",
                              "");
    expect_reasons<IntOrLine>(
        "[]",
        "no alternative accepts the value; reading std::variant<int, UnionReasonLine>: alternative 0 (int): expected "
        "an integer, got an array; alternative 1 (UnionReasonLine): expected an object, got an array",
        "");
    using IntOrMap = std::variant<int, std::map<std::string, int>>;
    expect_reasons<IntOrMap>(
        R"({"a/b~c":"x"})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got an "
                    "object; alternative 1 ({}) at /a~1b~0c: expected an integer, got a string (reading int)",
                    yyjson::detail::variant_name<IntOrMap>(), yyjson::detail::type_name<std::map<std::string, int>>()),
        "");

    // a struct alternative that a member of the object fails without a read still gives the reason of that read
    using CountOrText = std::variant<UnionReasonCount, UnionReasonText>;
    for (const auto& [json, first] : {std::pair{R"({"x":"a","y":"b"})", "x"}, std::pair{R"({"y":"b","x":"a"})", "y"}})
    {
        expect_reasons<CountOrText>(
            json,
            std::format("no alternative accepts the value; reading std::variant<UnionReasonCount, UnionReasonText>: "
                        "alternative 0 (UnionReasonCount) at /{}: expected an integer, got a string (reading int); "
                        "alternative 1 (UnionReasonText) at /y: expected an integer, got a string (reading int)",
                        first),
            "");
    }
    expect_reasons<CountOrText>(
        R"({"x":null})",
        std::format("no alternative accepts the value; reading std::variant<UnionReasonCount, UnionReasonText>: "
                    "alternative 0 (UnionReasonCount) at /x: expected an integer, got null (reading int); alternative "
                    "1 (UnionReasonText) at /x: expected a string, got null (reading {})",
                    yyjson::detail::type_name<std::string>()),
        "");
    EXPECT_EQ(yyjson::read<CountOrText>(R"({"x":"a","y":1})").index(), 1);
    EXPECT_EQ(yyjson::read(R"({"x":"a","y":1})").cast<CountOrText>().index(), 1);

    // a variant within an alternative gives no reasons of its own alternatives, which bounds the work
    using Nested = std::variant<int, std::variant<std::string, std::variant<bool, UnionPoint>>>;
    using Inner = std::variant_alternative_t<1, Nested>;
    expect_reasons<Nested>(
        R"({"x":"a"})",
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got an "
                    "object; alternative 1 ({}): no alternative accepts the value (reading {})",
                    yyjson::detail::variant_name<Nested>(), yyjson::detail::variant_name<Inner>(),
                    yyjson::detail::variant_name<Inner>()),
        "");
}

TEST(Constraints, OneOfBounds)
{
    // alternatives that read no value in common are not both read
    using Signs = yyjson::one_of_variant<yyjson::alt<int, yyjson::maximum(0)>, yyjson::alt<int, yyjson::minimum(1)>>;
    static_assert(!one_of_overlapping<Signs, 0, 1> && !one_of_contained<Signs>);
    expect_accepted<Signs>("-3", Signs(std::in_place_index<0>, -3));
    expect_accepted<Signs>("5", Signs(std::in_place_index<1>, 5));
    using Kinds = yyjson::one_of_variant<int, std::string, bool, std::vector<int>, std::nullptr_t>;
    static_assert(!one_of_overlapping<Kinds, 0, 1> && !one_of_overlapping<Kinds, 1, 3> && !one_of_contained<Kinds>);
    expect_accepted<Kinds>("[1]", Kinds(std::vector{1}));
    expect_accepted<Kinds>("true", Kinds(true));
    using Words = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::enum_("a", "b")>,
                                         yyjson::alt<std::string, yyjson::min_length(2)>>;
    static_assert(!one_of_overlapping<Words, 0, 1>);
    expect_accepted<Words>(R"("ab")", Words(std::in_place_index<1>, "ab"));
    using Lists = yyjson::one_of_variant<yyjson::alt<std::vector<int>, yyjson::max_items(2)>,
                                         yyjson::alt<std::vector<int>, yyjson::min_items(3)>>;
    static_assert(!one_of_overlapping<Lists, 0, 1>);
    expect_accepted<Lists>("[1,2,3]", Lists(std::in_place_index<1>, std::vector{1, 2, 3}));
    // a combinator may reject the empty value of a nullable alternative
    using NonNull = yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, yyjson::not_(yyjson::const_(nullptr))>,
                                           std::nullptr_t>;
    static_assert(!one_of_contained<NonNull>);
    static_assert(!one_of_contained<yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, &one_of_present>,
                                                           std::nullptr_t>>);
    static_assert(!one_of_contained<yyjson::one_of_variant<
                      yyjson::alt<yyjson::nullable<yyjson::object>, &one_of_object>, std::nullptr_t>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<yyjson::nullable<int>, yyjson::minimum(0)>, std::nullptr_t>>);
    expect_alternative<NonNull>("null", 1);
    expect_alternative<NonNull>("1", 0);
    using Owned = yyjson::one_of_variant<std::unique_ptr<int>, std::string>;
    expect_alternative<Owned>("1", 0);
    expect_alternative<Owned>(R"("a")", 1);

    // alternatives that may read a value in common are counted
    static_assert(one_of_overlapping<OneOfInts, 0, 1> && !one_of_contained<OneOfInts>);
    using Wide = yyjson::one_of_variant<std::int64_t, std::uint64_t>;
    static_assert(one_of_overlapping<Wide, 0, 1> && !one_of_contained<Wide>);
    expect_accepted<Wide>("-1", Wide(std::int64_t{-1}), false);
    expect_accepted<Wide>("18446744073709551615", Wide(std::uint64_t{18446744073709551615u}));
    expect_rejected<Wide>(
        "1",
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "");
    using Steps =
        yyjson::one_of_variant<yyjson::alt<int, yyjson::multiple_of(2)>, yyjson::alt<int, yyjson::multiple_of(3)>>;
    static_assert(one_of_overlapping<Steps, 0, 1> && !one_of_contained<Steps>);
    expect_accepted<Steps>("4", Steps(std::in_place_index<0>, 4), false);
    expect_rejected<Steps>("6",
                           "the value matches more than one alternative; reading yyjson::one_of_variant<int, int>: the "
                           "alternatives 0 and 1 match",
                           "");
    // a float compares a JSON integer exactly, also from 2^53, where a double rounds it
    using BigFloat = yyjson::one_of_variant<yyjson::alt<float, yyjson::exclusive_minimum(9007199254740992)>,
                                            yyjson::alt<std::uint64_t, yyjson::maximum(9007199254740993U)>>;
    static_assert(one_of_overlapping<BigFloat, 0, 1>);
    expect_rejected<BigFloat>(
        "9007199254740993",
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<float, std::uint64_t>>()),
        "");
    using FloatOrBig =
        yyjson::one_of_variant<yyjson::alt<float, yyjson::maximum(9007199254740992)>,
                               yyjson::alt<std::uint64_t, yyjson::const_(std::uint64_t{9007199254740993U})>>;
    static_assert(!one_of_contained<FloatOrBig>);
    expect_alternative<FloatOrBig>("9007199254740993", 1);
    expect_alternative<FloatOrBig>("9007199254740992", 0);

    // an alternative whose values another reads all of is a compile error
    static_assert(one_of_contained<yyjson::one_of_variant<int, double>>);
    static_assert(one_of_contained<yyjson::one_of_variant<int, int>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfX>>);
    static_assert(one_of_contained<yyjson::one_of_variant<std::nullptr_t, std::optional<int>>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>,
                                         yyjson::alt<int, yyjson::minimum(0), yyjson::maximum(5)>>>);
    static_assert(
        one_of_contained<yyjson::one_of_variant<yyjson::alt<std::string, yyjson::enum_("a", "b")>, std::string>>);
    static_assert(one_of_contained<yyjson::one_of_variant<yyjson::alt<std::string, &union_id_pattern>, std::string>>);
    using Numbers = yyjson::detail::one_of_relations<yyjson::one_of_variant<std::string, int, double>>;
    static_assert(Numbers::contained.contained == 1 && Numbers::contained.container == 2);
    // the analysis does not prove what a setting that it does not know reads
    static_assert(!one_of_contained<
                  yyjson::one_of_variant<std::vector<int>, yyjson::alt<std::vector<int>, yyjson::max_items(2)>>>);
    // a caster of the user that takes a value of the other document reads any value
    using OtherOrString = yyjson::one_of_variant<OneOfOtherCode, std::string>;
    static_assert(one_of_overlapping<OtherOrString, 0, 1>);
    expect_rejected_under<OtherOrString>(
        R"("a")", std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                              yyjson::detail::variant_name<yyjson::one_of_variant<OneOfOtherCode, std::string>>()));
    expect_alternative<std::variant<OneOfOtherCode, std::string>>(R"("a")", 0);
    // such a caster of a range or a map reads values that the elements or the values cannot
    expect_alternative<std::variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<std::variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);

    // a struct reads each object of another one that requires each key that it requires and takes no key that it
    // does not name, with values that it reads, with and without yyjson::reject_unknown_keys of the call
    using XAndClosedX = yyjson::detail::one_of_relations<yyjson::one_of_variant<OneOfX, OneOfClosedX>>;
    static_assert(XAndClosedX::contained.contained == 1 && XAndClosedX::contained.container == 0);
    using ClosedByAlt =
        yyjson::alt<OneOfXZ, yyjson::property("x"), yyjson::property("z"), yyjson::additional_properties(false)>;
    static_assert(one_of_contained<yyjson::one_of_variant<ClosedByAlt, OneOfXZ>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>, OneOfX>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfWrapped<OneOfClosedX>>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfWrapped<OneOfX>, OneOfClosedX>>);
    // the call's yyjson::reject_unknown_keys closes a struct that names fewer keys, so that it reads no object
    // with a key that only the other one names
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, ClosedByAlt>>);
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, OneOfXMaybeY>>);
    static_assert(!one_of_contained<yyjson::one_of_variant<OneOfX, OneOfWrapped<OneOfXZ>>>);
    using XOrXZ = yyjson::one_of_variant<OneOfX, OneOfXZ>;
    static_assert(!one_of_contained<XOrXZ> && one_of_overlapping<XOrXZ, 0, 1>);
    expect_rejected_under<XOrXZ>(R"({"x":1,"z":2})",
                                 "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                                 "OneOfXZ>: the alternatives 0 and 1 match");
    expect_alternative<XOrXZ>(R"({"x":1,"z":2})", 1, yyjson::reject_unknown_keys);
    expect_alternative<XOrXZ>(R"({"x":1})", 0, yyjson::reject_unknown_keys);
    // a map of other keys keeps a struct open under the call's yyjson::reject_unknown_keys
    using XOrRest = yyjson::one_of_variant<OneOfX, OneOfXRest>;
    static_assert(!one_of_contained<XOrRest>);
    expect_alternative<XOrRest>(R"({"x":1,"a":2})", 1, yyjson::reject_unknown_keys);

    // a struct reads no object of another one that requires a key that it rejects, or a value there that it does
    // not read
    using ClosedXOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    static_assert(!one_of_overlapping<ClosedXOrXZ, 0, 1>);
    expect_alternative<ClosedXOrXZ>(R"({"x":1,"z":2})", 1);
    expect_alternative<ClosedXOrXZ>(R"({"x":1})", 0);
    using IntOrText = yyjson::one_of_variant<OneOfX, OneOfTextX>;
    static_assert(!one_of_overlapping<IntOrText, 0, 1>);
    expect_alternative<IntOrText>(R"({"x":"a"})", 1);
    expect_alternative<IntOrText>(R"({"x":1})", 0);
    using Letters = yyjson::one_of_variant<yyjson::alt<OneOfTextX, yyjson::property("x", yyjson::enum_("a", "b"))>,
                                           yyjson::alt<OneOfTextX, yyjson::property("x", yyjson::enum_("c"))>>;
    static_assert(!one_of_overlapping<Letters, 0, 1>);
    expect_alternative<Letters>(R"({"x":"c"})", 1);
    expect_alternative<Letters>(R"({"x":"a"})", 0);
    // a std::optional does not take null as a missing key while the alternatives are counted
    using IntOrNull = yyjson::one_of_variant<OneOfMaybeX, OneOfNullX>;
    static_assert(!one_of_overlapping<IntOrNull, 0, 1> && !one_of_contained<IntOrNull>);
    expect_alternative<IntOrNull>(R"({"x":null})", 1);
    expect_alternative<IntOrNull>(R"({"x":1})", 0);
    expect_alternative<IntOrNull>("{}", 0);

    // two structs that read some values of each other at a key are counted
    using SignedOrUnsigned = yyjson::one_of_variant<OneOfSignedX, OneOfUnsignedX>;
    static_assert(!one_of_contained<SignedOrUnsigned> && one_of_overlapping<SignedOrUnsigned, 0, 1>);
    expect_rejected<SignedOrUnsigned>(
        R"({"x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfSignedX, OneOfUnsignedX>: "
        "the alternatives 0 and 1 match",
        "");
    expect_alternative<SignedOrUnsigned>(R"({"x":-1})", 0);

    // the analysis does not prove that a struct reads each value at a key that it does not name
    using MaybeYOrZ = yyjson::one_of_variant<OneOfXMaybeY, OneOfXMaybeZ>;
    static_assert(!one_of_contained<MaybeYOrZ> && one_of_overlapping<MaybeYOrZ, 0, 1>);
    expect_rejected<MaybeYOrZ>(R"({"x":1})",
                               "the value matches more than one alternative; reading "
                               "yyjson::one_of_variant<OneOfXMaybeY, OneOfXMaybeZ>: the alternatives 0 and 1 match",
                               "");
    expect_alternative<MaybeYOrZ>(R"({"x":1,"y":"a"})", 1);
    // nor what a struct reads with a setting other than the keys that it takes
    using MinimumX = yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>,
                                            yyjson::alt<OneOfXZ, yyjson::property("x", yyjson::minimum(0))>>;
    static_assert(!one_of_contained<MinimumX> && one_of_overlapping<MinimumX, 0, 1>);
    expect_rejected<MinimumX>(R"({"x":1,"z":2})",
                              "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfX, "
                              "OneOfXZ>: the alternatives 0 and 1 match",
                              "");
    // nor what a struct with a map of other keys reads at those keys
    using XZOrAny = yyjson::one_of_variant<OneOfXZ, OneOfXAny>;
    static_assert(!one_of_contained<XZOrAny> && one_of_overlapping<XZOrAny, 0, 1>);
    expect_rejected<XZOrAny>(R"({"x":1,"z":2})",
                             "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfXZ, "
                             "OneOfXAny>: the alternatives 0 and 1 match",
                             "");
    // nor compares the keys of two structs beyond 256 pairs of them
    using Many = yyjson::one_of_variant<OneOfMany16, OneOfMany17>;
    static_assert(!one_of_contained<Many> && one_of_overlapping<Many, 0, 1>);

    // a field that is read otherwise than its type reads a value of its own may read any value: in place by a
    // caster of the user, by a caster of the user of a std::optional, or as the value of an adjacent tag
    using CodeOrText = yyjson::one_of_variant<OneOfCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<CodeOrText, 0, 1>);
    expect_rejected_under<CodeOrText>(R"({"x":"a"})",
                                      "the value matches more than one alternative; reading "
                                      "yyjson::one_of_variant<OneOfCodeX, OneOfTextX>: the alternatives 0 and 1 match");
    // a caster of the user of the other document reads any value, also in a field
    using OtherCodeOrText = yyjson::one_of_variant<OneOfOtherCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<OtherCodeOrText, 0, 1>);
    expect_rejected_under<OtherCodeOrText>(
        R"({"x":"a"})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfOtherCodeX, OneOfTextX>: the "
        "alternatives 0 and 1 match");
    using NumberOrText = yyjson::one_of_variant<OneOfMaybeNumberX, OneOfTextX>;
    static_assert(one_of_overlapping<NumberOrText, 0, 1>);
    expect_rejected_under<NumberOrText>(
        R"({"x":"a"})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfMaybeNumberX, OneOfTextX>: "
        "the alternatives 0 and 1 match");
    using AdjacentOrX = yyjson::one_of_variant<OneOfAdjacentX, OneOfX>;
    static_assert(one_of_overlapping<AdjacentOrX, 0, 1>);
    expect_rejected_under<AdjacentOrX>(
        R"({"kind":"i","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfAdjacentX, OneOfX>: the "
        "alternatives 0 and 1 match");
    // the key of the tag of an enclosing variant tells no closed struct apart, and a closed struct does not read
    // only what another one reads that names that key
    using XKind = std::variant_alternative_t<0, OneOfTaggedXKind>;
    static_assert(!one_of_overlapping<XKind, 0, 1> && yyjson::detail::one_of_variant_cast<XKind>::tagged_overlap[1][0]);
    expect_alternative<XKind>(R"({"x":1,"kind":"p"})", 1);
    expect_rejected_under<OneOfTaggedXKind>(
        R"({"kind":"p","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfClosedX, OneOfXKind>: the "
        "alternatives 0 and 1 match");
    using MaybeKind = std::variant_alternative_t<0, OneOfTaggedMaybeKind>;
    static_assert(!one_of_contained<MaybeKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedMaybeKind>(R"({"kind":"p","x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","x":1})").cast<OneOfTaggedMaybeKind>()).index(), 0);
    // the closed struct also takes the key of the tag again with a value that the other one does not read
    using TextKind = std::variant_alternative_t<0, OneOfTaggedTextKind>;
    static_assert(!one_of_contained<TextKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedTextKind>(R"({"kind":"p","kind":1,"x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","kind":1,"x":1})").cast<OneOfTaggedTextKind>()).index(), 0);
    expect_rejected_under<OneOfTaggedTextKind>(
        R"({"kind":"p","x":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfClosedX, "
        "OneOfXMaybeTextKind>: the alternatives 0 and 1 match");
    // each transparent struct between an alternative and its keyed struct may have settings
    using LayeredOrXZ = yyjson::one_of_variant<OneOfWrapped<OneOfMidX>, OneOfXZ>;
    static_assert(!one_of_contained<LayeredOrXZ>);
    expect_alternative<LayeredOrXZ>(R"({"x":-1,"z":1})", 1);
    expect_rejected_under<LayeredOrXZ>(
        R"({"x":1,"z":1})",
        "the value matches more than one alternative; reading yyjson::one_of_variant<OneOfWrapped<OneOfMidX>, "
        "OneOfXZ>: the alternatives 0 and 1 match");
}

#if defined(__cpp_lib_define_static)
TEST(Constraints, UnionStringViewSettings)
{
    // a setting made from a std::string_view is a template argument
    using Id = yyjson::any_of_variant<yyjson::alt<std::string, yyjson::pattern(union_id_regex)>, int>;
    expect_alternative<Id>(R"("abc")", 0);
    EXPECT_THROW((void)yyjson::read<Id>(R"("ABC")"), yyjson::bad_cast);
    using OneId = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::pattern(union_id_regex)>,
                                         yyjson::alt<std::string, yyjson::max_length(2)>>;
    expect_alternative<OneId>(R"("abc")", 0);
    expect_alternative<OneId>(R"("AB")", 1);
    EXPECT_THROW((void)yyjson::read<OneId>(R"("ab")"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"("ab")").cast<OneId>(), yyjson::bad_cast);
}
#endif

TEST(Constraints, CasterJson)
{
    using P = CasterPoint;
    auto label = CasterLabel();
    label.set("xy");
    expect_accepted<CasterFields>(
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[1,2],"pos":{"y":2,"x":1},"dom":[3],"opt":"o","nul":null})",
        {CasterName("red"),
         CasterName("ab"),
         label,
         CasterScalar(1.25),
         CasterScalar(10),
         CasterScalar(true),
         CasterList({1, 2}),
         CasterPos(P{1, 2}),
         CasterDom({3}),
         CasterName("o"),
         {}},
        false);
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CasterFields )", key);
    };
    expect_rejected<CasterFields>(
        R"({"color":"green"})",
        R"(the value is not one of "red", "blue" at /color; reading the key "color" of CasterFields)", "/color");
    expect_rejected<CasterFields>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of CasterFields)", "/name");
    expect_rejected<CasterFields>(
        R"({"name":"Ab"})",
        R"(the string does not match the pattern "^[a-z]" at /name; reading the key "name" of CasterFields)", "/name");
    expect_rejected<CasterFields>(
        R"({"label":"x"})",
        R"(the string is shorter than the minimum length 2 at /label; reading the key "label" of CasterFields)",
        "/label");
    expect_rejected<CasterFields>(
        R"({"real":-1})", R"(the number is less than the minimum 0 at /real; reading the key "real" of CasterFields)",
        "/real");
    expect_rejected<CasterFields>(
        R"({"real":1.5})",
        R"(the number is not less than the exclusive maximum 1.5 at /real; reading the key "real" of CasterFields)",
        "/real");
    expect_rejected<CasterFields>(
        R"({"small":11})",
        R"(the number is greater than the maximum 10 at /small; reading the key "small" of CasterFields)", "/small");
    expect_rejected<CasterFields>(
        R"({"flag":false})",
        R"(the value is not equal to the constant true at /flag; reading the key "flag" of CasterFields)", "/flag");
    expect_rejected<CasterFields>(
        R"({"list":[1,2,3]})",
        R"(the array has more items than the maximum 2 at /list; reading the key "list" of CasterFields)", "/list");
    expect_rejected<CasterFields>(
        R"({"list":[1,1]})",
        R"(the item is equal to an earlier item at /list/1; reading an element of the value at the key "list" of CasterFields)",
        "/list/1");
    expect_rejected<CasterFields>(
        R"({"pos":{"x":2,"y":2}})",
        R"(the value is not equal to the constant {"x":1,"y":2} at /pos; reading the key "pos" of CasterFields)",
        "/pos");
    expect_rejected<CasterFields>(
        R"({"dom":[]})",
        R"(the array has fewer items than the minimum 1 at /dom; reading the key "dom" of CasterFields)", "/dom");
    expect_rejected<CasterFields>(
        R"({"opt":"abc"})",
        R"(the string is longer than the maximum length 2 at /opt; reading the key "opt" of CasterFields)", "/opt");
    expect_rejected<CasterFields>(
        R"({"nul":"abc"})",
        R"(the string is longer than the maximum length 2 at /nul; reading the key "nul" of CasterFields)", "/nul");
    // a value that the keywords accept can still fail to convert
    EXPECT_THROW((void)yyjson::read<CasterFields>(R"({"small":1.5})"), yyjson::bad_cast);

    expect_accepted<CasterPairs>(R"({"pair":{"x":1,"y":2},"pairs":[{"y":3,"x":4}]})", {{1, 2}, {{4, 3}}});
    expect_rejected<CasterPairs>(R"({"pair":{"x":1,"y":2,"z":3}})",
                                 "the object has more properties than the maximum 2 at /pair; reading CasterPair",
                                 "/pair");
    expect_rejected<CasterPairs>(R"({"pairs":[{"x":1}]})",
                                 "the object has fewer properties than the minimum 2 at /pairs/0; reading CasterPair",
                                 "/pairs/0");
}

TEST(Constraints, CasterJsonSameKind)
{
    // a keyword of the kind that the type writes checks the JSON value too, and not the sorted value
    const auto check = []<typename T>(std::type_identity<T>) {
        const auto message = std::format(R"(the value is not one of ["a","b"] at /h; reading the key "h" of {})",
                                         yyjson::detail::type_name<T>());
        expect_accepted<T>(R"({"h":["a","b"]})", {CasterSorted({"a", "b"})}, false);
        expect_rejected<T>(R"({"h":["b","a"]})", message, "/h");
        expect_rejected<T>(R"({"h":["c"]})", message, "/h");
        EXPECT_TRUE(expect_merge_rule(T(), R"({"h":["b","a"]})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T(), R"({"h":["a","b"]})").starts_with("bad cast"));
    };
    check(std::type_identity<CasterSortedField>());
    check(std::type_identity<CasterSortedProperty>());
}

TEST(Constraints, CasterJsonText)
{
    // the strings of the JSON value compare after the escapes are read
    using T = CasterSortedText;
    const auto good = T{CasterSorted({"a", "bc"}), CasterSorted({"a\"b", "\u00e9\n"})};
    expect_accepted<T>(R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9\n"]})", {good}, false);
    expect_accepted<T>(R"({"plain":["\u0061","b\u0063"],"quoted":["\u0061\"b","\u00e9\u000a"]})", {good}, false);
    expect_rejected<T>(R"({"plain":["a","b"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(R"({"plain":["a","bcd"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(R"({"plain":["a","bc","d"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(
        R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9"]})",
        R"(the value is not equal to the constant ["a\"b","\u00e9\n"] at /quoted; reading the key "quoted" of CasterSortedText)",
        "/quoted");
    expect_rejected<T>(
        R"({"plain":["a","bc"],"quoted":["a\\b","\u00e9\n"]})",
        R"(the value is not equal to the constant ["a\"b","\u00e9\n"] at /quoted; reading the key "quoted" of CasterSortedText)",
        "/quoted");
}

TEST(Constraints, CasterJsonMerged)
{
    // a caster replaces the value of its field, so the keywords check the member of the patch
    const auto t = yyjson::read<CasterFields>(
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[1],"pos":{"y":2,"x":1},"dom":[3],"opt":"o","nul":null})");
    EXPECT_EQ(
        expect_merge_rule(t, R"({"list":[2,3]})"),
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[2,3],"pos":{"x":1,"y":2},"dom":[3],"opt":"o","nul":null})");
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,3,4]})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,2]})").starts_with("bad cast"));
    // the fields that the patch leaves out are checked as they are written, and a null gives a field its default
    EXPECT_FALSE(expect_merge_rule(t, R"({"small":1})").starts_with("bad cast"));
    EXPECT_EQ(expect_merge_rule(CasterFields(), R"({"small":1})"),
              R"(bad cast: the value is not one of "red", "blue" at /color; reading the key "color" of CasterFields)");
    EXPECT_FALSE(expect_merge_rule(t, R"({"opt":null})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(t, R"({"opt":"abc"})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_rule(CasterPairs{{1, 2}, {}}, R"({"pair":{"x":3,"y":4}})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(CasterPairs{{1, 2}, {}}, R"({"pair":{"x":3}})").starts_with("bad cast"));
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, CasterJsonViews)
{
    using T = CasterTags;
    auto bag = CasterBag();
    bag.set("k", 1);
    expect_accepted<CasterViews>(R"({"tags":["a","b"],"mixed":"ab","first":[7,8],"bag":{"k":1}})",
                                 {T({"a", "b"}), T({"ab"}), CasterFirst(7), bag, {}, {}, {}}, false);
    expect_accepted<CasterViews>(R"({"mixed":["abc"],"opt":["a"],"xy":{"x":1,"y":2},"xys":[{"x":3,"y":4}]})",
                                 {{}, T({"abc"}), {}, {}, T({"a"}), {1, 2}, {{3, 4}}}, false);
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of CasterViews )", key);
    };
    expect_rejected<CasterViews>(
        R"({"tags":[]})",
        R"(the array has fewer items than the minimum 1 at /tags; reading the key "tags" of CasterViews)", "/tags");
    expect_rejected<CasterViews>(
        R"({"tags":["a","b","c","d"]})",
        R"(the array has more items than the maximum 3 at /tags; reading the key "tags" of CasterViews)", "/tags");
    expect_rejected<CasterViews>(
        R"({"tags":["a","a"]})",
        R"(the item is equal to an earlier item at /tags/1; reading an element of the value at the key "tags" of CasterViews)",
        "/tags/1");
    expect_rejected<CasterViews>(
        R"({"mixed":["a","b"]})",
        R"(the array has more items than the maximum 1 at /mixed; reading the key "mixed" of CasterViews)", "/mixed");
    expect_rejected<CasterViews>(
        R"({"mixed":"abc"})",
        R"(the string is longer than the maximum length 2 at /mixed; reading the key "mixed" of CasterViews)",
        "/mixed");
    // the caster reads the first element only, and the keywords count them all
    expect_rejected<CasterViews>(
        R"({"first":[1,2,3]})",
        R"(the array has more items than the maximum 2 at /first; reading the key "first" of CasterViews)", "/first");
    expect_rejected<CasterViews>(
        R"({"bag":{"a":1,"b":2}})",
        R"(the object has more properties than the maximum 1 at /bag; reading the key "bag" of CasterViews)", "/bag");
    expect_rejected<CasterViews>(
        R"({"opt":["a","b","c"]})",
        R"(the array has more items than the maximum 2 at /opt; reading the key "opt" of CasterViews)", "/opt");
    // the caster reads null, so the keywords check it
    expect_rejected<CasterViews>(
        R"({"opt":null})",
        R"(the value is not one of ["a"], ["a","b","c"] at /opt; reading the key "opt" of CasterViews)", "/opt");
    expect_rejected<CasterViews>(
        R"({"opt":["b"]})",
        R"(the value is not one of ["a"], ["a","b","c"] at /opt; reading the key "opt" of CasterViews)", "/opt");
    expect_rejected<CasterViews>(R"({"xy":{"x":1,"y":2,"z":3}})",
                                 "the object has more properties than the maximum 2 at /xy; reading CasterXY", "/xy");
    expect_rejected<CasterViews>(R"({"xys":[{"x":1,"y":2,"z":3}]})",
                                 "the object has more properties than the maximum 2 at /xys/0; reading CasterXY",
                                 "/xys/0");
    expect_rejected<CasterXY>(R"({"x":1,"y":2,"z":3})",
                              "the object has more properties than the maximum 2; reading CasterXY", "");
    EXPECT_EQ(yyjson::read<CasterShortKeys>(R"({"a":1,"b":2})"), CasterShortKeys{2});
    expect_rejected<CasterShortKeys>(
        R"({"a":1,"bc":2})", "the key is longer than the maximum length 1 at /bc; reading a key of CasterShortKeys",
        "/bc");
    expect_rejected<CasterShortKeys>(
        R"({"\u0061b":1})", "the key is longer than the maximum length 1 at /ab; reading a key of CasterShortKeys",
        "/ab");
    EXPECT_EQ(yyjson::read<CasterNoKeys>("{}"), CasterNoKeys{0});
    expect_rejected<CasterNoKeys>(R"({"a":1})",
                                  "the object has a key that is not allowed at /a; reading CasterNoKeys, which "
                                  "yyjson::additional_properties(false) closes",
                                  "/a");

    expect_accepted<CasterWrapped>(R"(["a"])", {T({"a"})});
    expect_rejected<CasterWrapped>(R"(["a","b"])", "the array has more items than the maximum 1; reading CasterWrapped",
                                   "");

    // the in-place form fills a field that the merge resets, so the keywords count the members of the patch
    const auto t = yyjson::read<CasterViews>(R"({"tags":["a"],"bag":{"a":1}})");
    EXPECT_EQ(
        expect_merge_into(CasterViews(), R"({"bag":{"b":2}})"),
        R"(bad cast: the array has fewer items than the minimum 1 at /tags; reading the key "tags" of CasterViews)");
    const auto merged = expect_merge_into(t, R"({"bag":{"b":2}})");
    EXPECT_NE(merged.find(R"("bag":{"b":2})"), std::string::npos) << merged;
    EXPECT_TRUE(expect_merge_into(t, R"({"bag":{"b":2,"c":3}})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"tags":["x","x"]})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_into(t, R"({"tags":["x","y"]})").starts_with("bad cast"));
}

TEST(Constraints, CasterJsonThreads)
{
    // a view that the caster iterates on another thread leaves no count of its elements to a later reader there
    auto worker = CasterWorker();
    auto text = std::string();
    text.reserve(64);
    text = R"({"items":[1,2,3]})";
    CasterRemote::worker = &worker;
    EXPECT_EQ(yyjson::read<CasterRemoteItems>(text).items.n, 3);
    CasterRemote::worker = nullptr;
    text = R"({"items":[1,2,3,4,5]})";
    auto direct = std::string();
    worker.run([&] { direct = read_or_error([&] { return yyjson::read<CasterRemoteItems>(text); }); });
    EXPECT_EQ(
        direct,
        R"(bad cast: the array has more items than the maximum 3 at /items; reading the key "items" of CasterRemoteItems)");
}

TEST(Constraints, CasterJsonNull)
{
    using T = CasterTags;
    // an absent member leaves the std::optional empty, and a null gives it what the caster reads from null
    for (const auto json : {R"({})", R"({"opt":null,"text":null,"one":null})", R"({"opt":["a"]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<CasterNulls>(json)) << json;
        // the listed JSON texts leave CasterNulls to the document, which gives the same value
        static_cast<void>(expect_direct_read<CasterNulls>(json));
    }
    EXPECT_FALSE(yyjson::read<CasterNulls>(R"({})").opt.has_value());
    EXPECT_EQ(yyjson::read<CasterNulls>(R"({"opt":null})").opt, std::optional(T()));
    expect_rejected<CasterNulls>(R"({"opt":["b"]})",
                                 R"(the value is not one of ["a"], null at /opt; reading the key "opt" of CasterNulls)",
                                 "/opt", false);
    expect_rejected<CasterNulls>(
        R"({"text":[]})",
        R"(the value is not equal to the constant null at /text; reading the key "text" of CasterNulls)", "/text",
        false);
    expect_rejected<CasterNulls>(
        R"({"one":[]})", R"(the value is not equal to the constant null at /one; reading the key "one" of CasterNulls)",
        "/one", false);
    // a null member of the patch gives the field its default, which the keywords do not check, while the fields
    // that the patch leaves out are checked as they are written
    const auto t = yyjson::read<CasterNulls>(R"({"opt":["a"],"one":null})");
    EXPECT_EQ(expect_merge_into(t, R"({"opt":null})"),
              R"(bad cast: the value is not equal to the constant null at /one; reading the key "one" of CasterNulls)");
    EXPECT_FALSE(expect_merge_into(t, R"({"one":null})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_into(t, R"({"opt":null,"one":null})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"opt":["b"]})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"text":[]})").starts_with("bad cast"));
}

TEST(Constraints, CasterJsonMessages)
{
    // each read reports a keyword on the JSON value of a caster as on a field of yyjson::value
    const auto failure = []<typename T>(std::type_identity<T>, std::string_view json) {
        const auto of = [](const yyjson::bad_cast& e) {
            auto m = message_of(e);
            const auto name = yyjson::detail::type_name<T>();
            if (const auto i = m.find(name); i != std::string::npos) m.replace(i, name.size(), "T");
            return std::pair(m, std::string(e.path()));
        };
        return std::array{
            of(bad_cast_of([&] { (void)yyjson::read<T>(json); })),
            of(bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); })),
            of(bad_cast_of([&] {
                auto t = T();
                yyjson::read_into(json, t);
            })),
            of(bad_cast_of([&] {
                auto t = T();
                yyjson::merge_into(json, t);
            })),
        };
    };
    for (const auto json : {R"({"a":{"b":1,"aa":1}})", R"({"a":{}})", R"({"a":{"b":1,"c":1,"d":1}})",
                            R"({"u":[1,2,1]})", R"({"u":[]})", R"({"u":[1,2,3,4]})", R"({"c":[2]})", R"({"k":{"x":2}})",
                            R"({"s":"abc"})", R"({"s":0})", R"({"o":[1,[2],[2]]})"})
    {
        const auto caster = failure(std::type_identity<CasterMessagesOfCaster>(), json);
        EXPECT_EQ(caster, failure(std::type_identity<CasterMessagesOfValue>(), json)) << json;
        // the message names the field that the caster of the struct reads
        EXPECT_NE(caster[0].first.find("the key \""), std::string::npos) << caster[0].first;
    }
}

TEST(Constraints, CasterJsonKeys)
{
    {
        using T = CasterKeysField;
        auto bag = CasterBag();
        expect_accepted<T>(R"({"bag":{},"set":{"k":1}})", {T{bag, CasterKeySet({{"k", 1}})}});
        expect_rejected<T>(
            R"({"bag":{"k":1},"set":{}})",
            R"(the object has a key that additionalProperties does not allow at /bag; reading the key "bag" of CasterKeysField)",
            "/bag");
        expect_rejected<T>(
            R"({"bag":{},"set":{"k":1,"kk":2}})",
            R"(the key is longer than the maximum length 1 at /set/kk; reading a key of the value at the key "set" of CasterKeysField)",
            "/set/kk");
        bag.set("k", 1);
        EXPECT_TRUE(expect_merge_rule(T{bag, {}}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{bag, {}}, R"({"bag":{}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
    }
    {
        using T = CasterKeysProperty;
        auto bag = CasterBag();
        bag.set("k", 1);
        expect_accepted<T>(R"({"bag":{"k":1},"set":{}})", {T{bag, {}}});
        expect_rejected<T>(
            R"({"bag":{"kk":1},"set":{}})",
            R"(the key is longer than the maximum length 1 at /bag/kk; reading a key of the value at the key "bag" of CasterKeysProperty)",
            "/bag/kk");
        expect_rejected<T>(
            R"({"bag":{},"set":{"k":1}})",
            R"(the object has a key that additionalProperties does not allow at /set; reading the key "set" of CasterKeysProperty)",
            "/set");
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"bag":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{}})").starts_with("bad cast"));
    }
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, CasterJsonElements)
{
    // the subschemas of the elements and the members check the JSON value that the caster reads
    using T = CasterItems;
    auto bag = CasterBag();
    bag.set("a", 1);
    bag.set("b", 2);
    expect_accepted<T>(R"({"list":[0,1],"tags":["a"],"set":{"a":1,"b":0},"bag":{"a":1,"b":2}})",
                       {CasterList({0, 1}), CasterTags({"a"}), CasterKeySet({{"a", 1}, {"b", 0}}), bag});
    expect_rejected<T>(
        R"({"list":[-1]})",
        R"(the number is less than the minimum 0 at /list/0; reading an element of the value at the key "list" of CasterItems)",
        "/list/0");
    expect_rejected<T>(
        R"({"list":[0,2]})",
        R"(the number is greater than the maximum 1 at /list/1; reading an element of the value at the key "list" of CasterItems)",
        "/list/1");
    expect_rejected<T>(
        R"({"list":[0,0]})",
        R"(the value satisfies none of the subschemas of anyOf at /list; reading the key "list" of CasterItems)",
        "/list");
    expect_rejected<T>(
        R"({"tags":["ab"]})",
        R"(the string is longer than the maximum length 1 at /tags/0; reading an element of the value at the key "tags" of CasterItems)",
        "/tags/0");
    expect_rejected<T>(R"({"tags":["a","x"]})",
                       R"(the value satisfies the subschema of not at /tags; reading the key "tags" of CasterItems)",
                       "/tags");
    expect_rejected<T>(R"({"set":{"a":2}})",
                       R"(the object does not satisfy properties at /set; reading the key "set" of CasterItems)",
                       "/set");
    expect_rejected<T>(R"({"set":{"b":0}})",
                       R"(the object does not satisfy properties at /set; reading the key "set" of CasterItems)",
                       "/set");
    expect_rejected<T>(
        R"({"set":{"a":1,"b":-1}})",
        R"(the number is less than the minimum 0 at /set/b; reading an element of the value at the key "set" of CasterItems)",
        "/set/b");
    expect_rejected<T>(
        R"({"bag":{"a":1}})",
        R"(the object does not have all the keys that dependentRequired requires at /bag; reading the key "bag" of CasterItems)",
        "/bag");
    expect_rejected<T>(R"({"bag":{"c":1,"d":2}})",
                       R"(the value satisfies if and not then at /bag; reading the key "bag" of CasterItems)", "/bag");
    // validate checks the JSON that the caster writes
    auto t = T{CasterList({0, 1}), CasterTags({"a"}), CasterKeySet({{"a", 1}}), {}};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.list = CasterList({0, 2});
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.list = CasterList({0, 1});
    t.set = CasterKeySet({{"a", 1}, {"b", -1}});
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.set = CasterKeySet({{"a", 1}});
    t.bag.set("a", 1);
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
}
#endif
#endif

#if FIELD_RFL_ANNOTATIONS
TEST(Constraints, Annotations)
{
    expect_accepted<ConstrainedAnnotated>(R"({"n":3,"s":"xy","even":4})", {3, "pq", "xy", 4});
    expect_rejected<ConstrainedAnnotated>(
        R"({"p":"pp"})",
        R"(the string does not match all of the patterns "^p", "q$" at /p; reading the key "p" of ConstrainedAnnotated)",
        "/p");
    expect_rejected<ConstrainedAnnotated>(
        R"({"n":4})", R"(the number is greater than the maximum 3 at /n; reading the key "n" of ConstrainedAnnotated)",
        "/n");
    expect_rejected<ConstrainedAnnotated>(
        R"({"s":"y"})",
        R"(the string does not match the pattern "^x" at /s; reading the key "s" of ConstrainedAnnotated)", "/s");
    expect_rejected<ConstrainedAnnotated>(
        R"({"even":3})",
        R"(the value does not satisfy the predicate at /even; reading the key "even" of ConstrainedAnnotated)",
        "/even");

    expect_accepted<KeywordsAnnotated>(R"({"version":2,"unit":"b","day":"2024-02-29","note":"y","step":1.5})",
                                       {2, "b", "2024-02-29", "y", 1.5, {}});
    expect_accepted<KeywordsAnnotated>(R"({"list":[1,2]})", {.list = {1, 2}});
    const auto keywords = [](std::string_view what, std::string_view key) {
        return std::format(R"({} at /{}; reading the key "{}" of KeywordsAnnotated)", what, key, key);
    };
    expect_rejected<KeywordsAnnotated>(R"({"version":1})",
                                       keywords("the value is not equal to the constant 2", "version"), "/version");
    expect_rejected<KeywordsAnnotated>(R"({"unit":"c"})", keywords(R"(the value is not one of "a", "b")", "unit"),
                                       "/unit");
    expect_rejected<KeywordsAnnotated>(R"({"day":"2024-02-30"})",
                                       keywords(R"(the string does not have the format "date")", "day"), "/day");
    expect_rejected<KeywordsAnnotated>(R"({"step":0.7})", keywords("the number is not a multiple of 0.5", "step"),
                                       "/step");
    expect_rejected<KeywordsAnnotated>(
        R"({"step":2})", keywords("the number is not less than the exclusive maximum 2", "step"), "/step");
    expect_rejected<KeywordsAnnotated>(R"({"list":[2,1]})", keywords("the value is not one of [1,2], []", "list"),
                                       "/list");
}

// The settings of a member as its annotations, and the same settings in `yyjson::field`
struct ParityInner
{
    int a = 0;
    std::optional<int> b;
    bool operator==(const ParityInner&) const = default;
};
struct[[= yyjson::required("kind")]] AnnotatedParity
{
    [[= yyjson::property("a", yyjson::minimum(1))]] ParityInner low;
    [[ = yyjson::property("a"), = yyjson::additional_properties(false) ]] ParityInner closed;
    [[= yyjson::dependent_required("a", {"b"})]] ParityInner dep;
    [[= yyjson::adjacent_tag("kind", {"i", "s"})]] std::variant<int, std::string> data;
};
struct FieldParity
{
    ParityInner low;
    ParityInner closed;
    ParityInner dep;
    std::variant<int, std::string> data;
};
template <>
inline constexpr auto yyjson::annotations<FieldParity> =
    yyjson::annotate(yyjson::field("low", yyjson::property("a", yyjson::minimum(1))),
                     yyjson::field("closed", yyjson::property("a"), yyjson::additional_properties(false)),
                     yyjson::field("dep", yyjson::dependent_required("a", {"b"})),
                     yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})), yyjson::required("kind"));

namespace
{
    // What the read of `json` gives: the JSON that the value writes, or the error with its path
    template <typename T>
    std::string parity_outcome(std::string_view json)
    {
        try
        {
            auto t = yyjson::read<T>(json);
            try
            {
                yyjson::validate(t);
            }
            catch (const yyjson::bad_cast& e)
            {
                return "invalid at " + std::string(e.path());
            }
            return std::string(yyjson::write(t));
        }
        catch (const yyjson::bad_cast& e)
        {
            auto message = message_of(e);
            if (const auto i = message.find("AnnotatedParity"); i != std::string::npos)
                message.replace(i, std::string_view("AnnotatedParity").size(), "FieldParity");
            return message + " at " + std::string(e.path());
        }
    }
}  // namespace

TEST(Constraints, MemberAnnotationsAsField)
{
    for (const std::string_view json :
         {R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":0},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0,"b":2},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"s","data":"x"})"})
    {
        EXPECT_EQ(parity_outcome<AnnotatedParity>(json), parity_outcome<FieldParity>(json)) << json;
    }
    EXPECT_EQ(parity_outcome<AnnotatedParity>(R"({"low":{"a":0},"kind":"i","data":1})"),
              R"(the object does not satisfy properties at /low; reading the key "low" of FieldParity at /low)");
    // validate checks the members of a value that no read made
    auto t = AnnotatedParity{{1, {}}, {0, {}}, {0, 1}, 1};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.closed.b = 2;
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.closed.b.reset();
    t.dep.b.reset();
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
}

TEST(Constraints, ElementAnnotations)
{
    expect_accepted<ElementsAnnotated>(
        R"({"scores":[0],"ids":[1,2],"row":[0,"a"],"names":{"a":1},"tags":["x","y"],"groups":{"g":[9]}})",
        {{0}, {1, 2}, {0, "a"}, {{"a", 1}}, {"x", "y"}, {{"g", {9}}}});
    const auto element = [](std::string_view what, std::string_view path, std::string_view key) {
        return std::format(R"({} at {}; reading an element of the value at the key "{}" of ElementsAnnotated)", what,
                           path, key);
    };
    const auto value = [](std::string_view what, std::string_view key) {
        return std::format(R"({} at /{}; reading the key "{}" of ElementsAnnotated)", what, key, key);
    };
    expect_rejected<ElementsAnnotated>(
        R"({"scores":[0,-1]})", element("the number is less than the minimum 0", "/scores/1", "scores"), "/scores/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[1,1]})",
                                       element("the item is equal to an earlier item", "/ids/1", "ids"), "/ids/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[]})", value("the array has fewer items than the minimum 1", "ids"),
                                       "/ids");
    expect_rejected<ElementsAnnotated>(
        R"({"row":[0,"ab"]})", element("the string is longer than the maximum length 1", "/row/1", "row"), "/row/1");
    expect_rejected<ElementsAnnotated>(
        R"({"names":{"A":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /names/A; reading a key of the value at the key "names" of ElementsAnnotated)",
        "/names/A");
    expect_rejected<ElementsAnnotated>(
        R"({"tags":["x","x"]})", value("the array has more than 1 item that satisfies contains", "tags"), "/tags");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[10]}})",
                                       element("the number is greater than the maximum 9", "/groups/g/0", "groups"),
                                       "/groups/g/0");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[],"h":[]}})",
                                       value("the object has more properties than the maximum 1", "groups"), "/groups");
}

TEST(Constraints, CombinatorAnnotations)
{
    expect_accepted<CombinedAnnotated>(R"({"n":2,"even":12,"digit":9,"s":"bc","one":4})", {2, 12, 9, "bc", 4, {}});
    const auto value = [](std::string_view what, std::string_view key) {
        return std::format(R"(the value {} at /{}; reading the key "{}" of CombinedAnnotated)", what, key, key);
    };
    expect_rejected<CombinedAnnotated>(R"({"n":0})", value("satisfies the subschema of not", "n"), "/n");
    expect_rejected<CombinedAnnotated>(R"({"even":11})", value("satisfies if and not then", "even"), "/even");
    expect_rejected<CombinedAnnotated>(R"({"digit":10})",
                                       value("does not satisfy all the subschemas of allOf", "digit"), "/digit");
    expect_rejected<CombinedAnnotated>(R"({"s":"c"})", value("satisfies none of the subschemas of anyOf", "s"), "/s");
    expect_rejected<CombinedAnnotated>(R"({"one":6})", value("satisfies more than one subschema of oneOf", "one"),
                                       "/one");
    expect_rejected<CombinedAnnotated>(
        R"({"ints":[1,"a"]})",
        R"(the value does not satisfy matches at /ints/1; reading an element of the value at the key "ints" of CombinedAnnotated)",
        "/ints/1");
}

TEST(Constraints, StructConditionAnnotations)
{
    static_assert(yyjson::detail::has_struct_conditions<ExclusiveAnnotated>);
    expect_accepted<ExclusiveAnnotated>(R"({"first":1})", {1, {}});
    expect_accepted<ExclusiveAnnotated>(R"({"b":0})", {{}, 0});
    expect_rejected<ExclusiveAnnotated>(
        R"({"first":1,"b":2})", "the value satisfies the subschema of not; reading ExclusiveAnnotated in yyjson::not_",
        "");
    expect_rejected<ExclusiveAnnotated>(
        R"({"b":-1})", "the value satisfies none of the subschemas of anyOf; reading ExclusiveAnnotated", "");
}

TEST(Constraints, PatternAnnotations)
{
    expect_accepted<PatternAnnotated>(R"({"x1":2,"a":1})", {1, {{"x1", 2}}});
    expect_rejected<PatternAnnotated>(
        R"({"x1":-1})", "the number is less than the minimum 0 at /x1; reading a member value of PatternAnnotated",
        "/x1");
    EXPECT_EQ(std::string(yyjson::write(PatternAnnotated{1, {{"x1", 2}}})), R"({"a":1,"x1":2})");
}

TEST(Constraints, CasterAnnotations)
{
    // `yyjson::unique_items` on a caster reads the type through the document
    expect_accepted<CasterAnnotated>(R"({"list":[1,2],"name":"abc"})", {CasterList({1, 2}), CasterName("abc")}, false);
    expect_rejected<CasterAnnotated>(
        R"({"list":[1,1]})",
        R"(the item is equal to an earlier item at /list/1; reading an element of the value at the key "list" of CasterAnnotated)",
        "/list/1");
    expect_rejected<CasterAnnotated>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of CasterAnnotated)",
        "/name");
}
#endif

// the keywords check the JSON value at their position, and those of another JSON kind check nothing there
enum class KjColor
{
    red,
    green
};
struct KjRect
{
    int w = 0;
    int h = 0;
    bool operator==(const KjRect&) const = default;
};
struct KjCircle
{
    double r = 0;
    bool operator==(const KjCircle&) const = default;
};
using KjShape = std::variant<KjCircle, KjRect>;
template <>
inline constexpr auto yyjson::annotations<KjShape> = yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct KjObjects
{
    KjRect r;
    KjShape s;
    bool operator==(const KjObjects&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjObjects> =
    yyjson::annotate(yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::max_properties(2)));
struct KjScalars
{
    std::variant<KjColor, std::string> v;
    KjColor c{};
    std::variant<int, std::string> w = std::string();
    bool operator==(const KjScalars&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjScalars> =
    yyjson::annotate(yyjson::field("v", yyjson::max_length(3)), yyjson::field("c", yyjson::max_length(3)),
                     yyjson::field("w", yyjson::not_(yyjson::min_length(1))));
struct KjArrays
{
    std::array<char, 3> a{};
    std::tuple<int, double, std::string> t = {0, 1, ""};
    std::set<int> s;
    bool operator==(const KjArrays&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjArrays> =
    yyjson::annotate(yyjson::field("a", yyjson::items(yyjson::maximum(100))),
                     yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::unique_items));
struct KjExtra
{
    int a = 0;
    std::map<std::string, int> rest;
    bool operator==(const KjExtra&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjExtra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
struct KjEvent
{
    int id = 0;
    std::variant<KjRect, int> data;
    bool operator==(const KjEvent&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));
// written as a string, and read as an object by its field
struct KjText
{
    std::string s;
    bool operator==(const KjText&) const = default;
};
template <>
struct yyjson::caster<KjText>
{
    static std::string to_json(const KjText& t) { return t.s; }
};
struct KjCasters
{
    std::vector<CasterName> names;
    std::map<std::string, CasterName> map;
    KjText text;
    bool operator==(const KjCasters&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjCasters> =
    yyjson::annotate(yyjson::field("names", yyjson::items(yyjson::max_length(2))),
                     yyjson::field("map", yyjson::additional_properties(yyjson::max_length(2))),
                     yyjson::field("text", yyjson::max_length(1)));

TEST(Constraints, JsonKindObjects)
{
    expect_accepted<KjObjects>(R"({"r":{"w":1,"h":2},"s":{"type":"circle","r":1}})", {{1, 2}, KjCircle{1}});
    // every member of the object of the struct satisfies the subschema, and the tag counts as a member
    expect_rejected<KjObjects>(
        R"({"r":{"w":1,"h":-2}})",
        R"(the object has a value that does not satisfy additionalProperties at /r; reading the key "r" of KjObjects)",
        "/r");
    expect_rejected<KjObjects>(
        R"({"s":{"type":"rect","w":1,"h":2}})",
        R"(the object has more properties than the maximum 2 at /s; reading the key "s" of KjObjects)", "/s");
}

TEST(Constraints, JsonKindScalars)
{
    expect_accepted<KjScalars>(R"({"v":"red","c":"red","w":""})", {KjColor::red, KjColor::red, std::string()});
    expect_rejected<KjScalars>(
        R"({"v":"green"})", R"(the string is longer than the maximum length 3 at /v; reading the key "v" of KjScalars)",
        "/v");
    expect_rejected<KjScalars>(
        R"({"c":"green"})", R"(the string is longer than the maximum length 3 at /c; reading the key "c" of KjScalars)",
        "/c");
    // `yyjson::min_length` holds for a number, so that `yyjson::not_` rejects it
    expect_rejected<KjScalars>(
        R"({"w":1})", R"(the value satisfies the subschema of not at /w; reading the key "w" of KjScalars)", "/w");
}

TEST(Constraints, JsonKindArrays)
{
    // the unique elements of a tuple of several types compare their JSON values
    expect_accepted<KjArrays>(R"({"a":[1,2,3],"t":[1,2,"x"],"s":[1,2]})", {{1, 2, 3}, {1, 2.0, "x"}, {1, 2}}, false);
    expect_rejected<KjArrays>(
        R"({"a":[1,2,101]})",
        R"(the number is greater than the maximum 100 at /a/2; reading an element of the value at the key "a" of KjArrays)",
        "/a/2");
    expect_rejected<KjArrays>(
        R"({"t":[-1,2,"x"]})",
        R"(the number is less than the minimum 0 at /t/0; reading an element of the value at the key "t" of KjArrays)",
        "/t/0");
    expect_rejected<KjArrays>(
        R"({"t":[1,1,"x"]})",
        R"(the item is equal to an earlier item at /t/1; reading an element of the value at the key "t" of KjArrays)",
        "/t/1");
    // the set drops a duplicate, but the keyword checks the JSON value
    expect_rejected<KjArrays>(
        R"({"s":[1,1]})",
        R"(the item is equal to an earlier item at /s/1; reading an element of the value at the key "s" of KjArrays)",
        "/s/1");
}

TEST(Constraints, JsonKindKeys)
{
    // the struct writes any key with the map of `yyjson::extra_keys`, and the key of an adjacent tag
    expect_accepted<KjExtra>(R"({"a":1,"b":5})", {1, {{"b", 5}}});
    expect_rejected<KjExtra>(R"({"a":1,"b":-5})",
                             R"(the number is less than the minimum 0 at /b; reading the key "b" of KjExtra)", "/b");
    expect_rejected<KjExtra>(R"({"a":1})", R"(a required key is missing; reading the key "b" of KjExtra)", "");
    expect_accepted<KjEvent>(R"({"id":1,"data":5,"kind":"count"})", {1, 5});
    expect_rejected<KjEvent>(
        R"({"id":1,"data":{"w":1,"h":2},"kind":"rect"})",
        R"(the value is not equal to the constant "count" at /kind; reading the key "kind" of KjEvent)", "/kind");
}

TEST(Constraints, JsonKindCasters)
{
    using N = CasterName;
    expect_accepted<KjCasters>(R"({"names":["a","bc"],"map":{"k":"ab"},"text":{"s":"long"}})",
                               {{N("a"), N("bc")}, {{"k", N("ab")}}, {"long"}});
    expect_rejected<KjCasters>(
        R"({"names":["a","bcd"]})",
        R"(the string is longer than the maximum length 2 at /names/1; reading an element of the value at the key "names" of KjCasters)",
        "/names/1");
    expect_rejected<KjCasters>(
        R"({"map":{"k":"abc"}})",
        R"(the string is longer than the maximum length 2 at /map/k; reading an element of the value at the key "map" of KjCasters)",
        "/map/k");
}

// `yyjson::additional_properties` on the type of a struct checks each member that `yyjson::property` does not name
struct KjTypeRest
{
    int a = 0;
    std::string b = "x";
    std::map<std::string, int> rest;
    bool operator==(const KjTypeRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjTypeRest> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::min_length(1)),
                     yyjson::additional_properties(yyjson::minimum(0)));
// the keywords at the position of an internally tagged variant name its tag
struct KjTagged
{
    KjShape s;
    bool operator==(const KjTagged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjTagged> =
    yyjson::annotate(yyjson::field("s", yyjson::property("type", yyjson::enum_("circle")), yyjson::required("type"),
                                   yyjson::if_then_else(yyjson::if_(yyjson::property("type", yyjson::const_("circle"))),
                                                        yyjson::then(yyjson::max_properties(2)))));
// a value that the extra keys read is written in another form
enum class KjLevel
{
    zero,
    one
};
template <>
inline constexpr auto yyjson::annotations<KjLevel> = yyjson::annotate(yyjson::enum_format::name_or_integer);
struct KjLevels
{
    int a = 0;
    std::map<std::string, KjLevel> rest;
    bool operator==(const KjLevels&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KjLevels> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("z", yyjson::maximum(0)));
struct KjDomPrefix
{
    yyjson::value v;
};
template <>
inline constexpr auto yyjson::annotations<KjDomPrefix> =
    yyjson::annotate(yyjson::field("v", yyjson::prefix_items(yyjson::minimum(0))));

TEST(Constraints, JsonKindTypeRest)
{
    expect_accepted<KjTypeRest>(R"({"a":1,"b":"x","z":3})", {1, "x", {{"z", 3}}});
    expect_rejected<KjTypeRest>(
        R"({"a":-1,"b":"x"})",
        "the number is less than the minimum 0 at /a; reading a value of KjTypeRest in yyjson::additional_properties",
        "/a");
    expect_rejected<KjTypeRest>(
        R"({"a":1,"b":"x","z":-3})",
        "the number is less than the minimum 0 at /z; reading a value of KjTypeRest in yyjson::additional_properties",
        "/z");
    expect_rejected<KjTypeRest>(
        R"({"a":1,"b":""})",
        R"(the string is shorter than the minimum length 1 at /b; reading the key "b" of KjTypeRest)", "/b");
    EXPECT_NO_THROW(yyjson::validate(KjTypeRest{1, "x", {{"q", 2}}}));
    EXPECT_THROW(yyjson::validate(KjTypeRest{-1, "x", {}}), yyjson::bad_cast);
    EXPECT_THROW(yyjson::validate(KjTypeRest{1, "x", {{"q", -2}}}), yyjson::bad_cast);
}

TEST(Constraints, JsonKindTag)
{
    expect_accepted<KjTagged>(R"({"s":{"type":"circle","r":1}})", {KjCircle{1}});
    expect_rejected<KjTagged>(R"({"s":{"type":"rect","w":1}})",
                              R"(the object does not satisfy properties at /s; reading the key "s" of KjTagged)", "/s");
    // the read checks the input object, which can hold a key that the struct does not write
    expect_rejected<KjTagged>(R"({"s":{"type":"circle","r":1,"x":0}})",
                              R"(the value satisfies if and not then at /s; reading the key "s" of KjTagged)", "/s");
    EXPECT_NO_THROW(yyjson::validate(KjTagged{KjCircle{1}}));
    EXPECT_THROW(yyjson::validate(KjTagged{KjRect{}}), yyjson::bad_cast);
    // a merge checks the object as merged, with the tag of the alternative that it holds
    auto t = KjTagged{KjCircle{1}};
    yyjson::merge_into(R"({"s":{"r":2}})", t);
    EXPECT_EQ(t, KjTagged{KjCircle{2}});
}

TEST(Constraints, JsonKindInputValues)
{
    // the read checks the value of the input, and `validate` the value that the struct writes
    expect_accepted<KjLevels>(R"({"a":0,"z":"one"})", {0, {{"z", KjLevel::one}}});
    expect_accepted<KjLevels>(R"({"a":0,"z":0})", {0, {{"z", KjLevel::zero}}});
    expect_rejected<KjLevels>(
        R"({"a":0,"z":1})", R"(the number is greater than the maximum 0 at /z; reading the key "z" of KjLevels)", "/z");
    EXPECT_NO_THROW(yyjson::validate(KjLevels{0, {{"z", KjLevel::one}}}));
    // a value of the DOM checks its elements by their positions
    EXPECT_NO_THROW((void)yyjson::read<KjDomPrefix>(R"({"v":[0,-1]})"));
    const auto e = bad_cast_of([] { (void)yyjson::read<KjDomPrefix>(R"({"v":[-1]})"); });
    EXPECT_EQ(e.path(), "/v/0");
}

// a key of `yyjson::property` that the extra keys receive leaves the other extra keys to additional_properties
struct ScExtra
{
    int a = 0;
    std::map<std::string, int> rest;
    bool operator==(const ScExtra&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScExtra> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)),
                     yyjson::additional_properties(yyjson::minimum(0)));
// a caster that only writes, and the casters that read a JSON object
struct ScWritten
{
    std::string s;
};
template <>
struct yyjson::caster<ScWritten>
{
    static std::string to_json(const ScWritten& c) { return c.s; }
};
struct ScWrittenVec
{
    std::vector<ScWritten> v;
    std::map<std::string, ScWritten> m;
};
template <>
inline constexpr auto yyjson::annotations<ScWrittenVec> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(2))),
                     yyjson::field("m", yyjson::additional_properties(yyjson::max_length(2))));
struct ScCount
{
    int n = 0;
    bool operator==(const ScCount&) const = default;
};
template <>
struct yyjson::caster<ScCount>
{
    static ScCount from_json(const std::map<std::string, int>& m) { return {m.contains("n") ? m.at("n") : 0}; }
    static std::map<std::string, int> to_json(const ScCount&) { return {{"n", 0}}; }
};
struct ScCounts
{
    std::map<std::string, ScCount> m;
    bool operator==(const ScCounts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScCounts> = yyjson::annotate(
    yyjson::field("m", yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0)))));
// a member that a patch gives is checked by the value of the patch
enum class ScLevel
{
    zero,
    one
};
template <>
inline constexpr auto yyjson::annotations<ScLevel> = yyjson::annotate(yyjson::enum_format::name_or_integer);
struct ScLevelRest
{
    ScLevel e = ScLevel::zero;
    bool operator==(const ScLevelRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScLevelRest> =
    yyjson::annotate(yyjson::additional_properties(yyjson::maximum(0)));
// a caster that reads in place takes the kinds of its source
struct ScInPlace
{
    int v = 0;
    bool operator==(const ScInPlace&) const = default;
};
template <>
struct yyjson::caster<ScInPlace>
{
    static void from_json(ScInPlace& x, int v) { x.v = v; }
    static std::string to_json(const ScInPlace& x) { return std::to_string(x.v); }
};
struct ScInPlaceField
{
    ScInPlace x;
    bool operator==(const ScInPlaceField&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScInPlaceField> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
// the elements of a tuple take the keywords of each kind in a combinator
struct ScPair
{
    std::pair<int, std::string> t;
    bool operator==(const ScPair&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScPair> =
    yyjson::annotate(yyjson::field("t", yyjson::contains(yyjson::all_of(yyjson::min_length(2), yyjson::maximum(0)))));
// a tag is a JSON string
struct ScTagged
{
    std::variant<int, std::string> v;
    bool operator==(const ScTagged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ScTagged> = yyjson::annotate(
    yyjson::field("v", yyjson::adjacent_tag("kind", {"ii", "s"})), yyjson::property("kind", yyjson::min_length(2)));

TEST(Constraints, ExtraKeyProperty)
{
    expect_accepted<ScExtra>(R"({"a":1,"b":2,"c":1})", {1, {{"b", 2}, {"c", 1}}});
    expect_rejected<ScExtra>(
        R"({"a":1,"b":2,"c":-1})",
        "the number is less than the minimum 0 at /c; reading a value of ScExtra in yyjson::additional_properties",
        "/c");
    expect_rejected<ScExtra>(R"({"a":1,"b":-2})",
                             R"(the number is less than the minimum 0 at /b; reading the key "b" of ScExtra)", "/b");
    EXPECT_NO_THROW(yyjson::validate(ScExtra{0, {{"b", 2}, {"c", 1}}}));
    EXPECT_THROW(yyjson::validate(ScExtra{0, {{"b", 2}, {"c", -1}}}), yyjson::bad_cast);
}

TEST(Constraints, WrittenCasterElements)
{
    // `validate` checks the JSON that the caster writes, and does not read it back
    EXPECT_NO_THROW(yyjson::validate(ScWrittenVec{{{"ok"}}, {{"k", {"ok"}}}}));
    const auto v = bad_cast_of([] { yyjson::validate(ScWrittenVec{{{"bad"}}, {}}); });
    EXPECT_EQ(v.path(), "/v/0");
    const auto m = bad_cast_of([] { yyjson::validate(ScWrittenVec{{}, {{"k", {"bad"}}}}); });
    EXPECT_EQ(m.path(), "/m/k");
    EXPECT_EQ(yyjson::write(ScWrittenVec{{{"ok"}}, {}}, yyjson::with_validation), R"({"v":["ok"],"m":{}})");
    EXPECT_THROW((void)yyjson::write(ScWrittenVec{{{"bad"}}, {}}, yyjson::with_validation), yyjson::bad_cast);
}

TEST(Constraints, MergeChecksPatch)
{
    // a merge into a value that a caster replaces checks the JSON of the patch
    auto counts = ScCounts{{{"k", ScCount{0}}}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"k":{"n":1}}})", counts); });
    EXPECT_EQ(e.path(), "/m/k");
    yyjson::merge_into(R"({"m":{"k":{"n":0}}})", counts);
    // a patched member is checked by the value of the patch, not by the value that it writes
    auto rest = ScLevelRest{};
    const auto r = bad_cast_of([&] { yyjson::merge_into(R"({"e":1})", rest); });
    EXPECT_EQ(message_of(r),
              "the number is greater than the maximum 0 at /e; reading a value of ScLevelRest in "
              "yyjson::additional_properties")
        << r.what();
    yyjson::merge_into(R"({"e":0})", rest);
    EXPECT_EQ(rest, ScLevelRest{});
}

TEST(Constraints, InPlaceCasterKind)
{
    expect_accepted<ScInPlaceField>(R"({"x":1})", {{1}});
    expect_rejected<ScInPlaceField>(
        R"({"x":-1})", R"(the number is less than the minimum 0 at /x; reading the key "x" of ScInPlaceField)", "/x");
}

TEST(Constraints, TupleCombinatorKinds)
{
    expect_accepted<ScPair>(R"({"t":[0,"a"]})", {{0, "a"}});
    expect_accepted<ScPair>(R"({"t":[1,"ab"]})", {{1, "ab"}});
    expect_rejected<ScPair>(R"({"t":[1,"a"]})",
                            R"(the array has no item that satisfies contains at /t; reading the key "t" of ScPair)",
                            "/t");
    EXPECT_NO_THROW(yyjson::validate(ScPair{{1, "ab"}}));
    EXPECT_THROW(yyjson::validate(ScPair{{1, "a"}}), yyjson::bad_cast);
}

TEST(Constraints, TagStringKeywords)
{
    expect_accepted<ScTagged>(R"({"kind":"ii","v":1})", {1});
    EXPECT_THROW((void)yyjson::read<ScTagged>(R"({"kind":"s","v":"x"})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"kind":"s","v":"x"})").cast<ScTagged>(), yyjson::bad_cast);
    EXPECT_THROW(yyjson::validate(ScTagged{std::string("x")}), yyjson::bad_cast);
}

// the keywords on a type check the JSON that its caster reads and writes
struct CtList
{
    std::vector<int> v;
    bool operator==(const CtList&) const = default;
};
template <>
struct yyjson::caster<CtList>
{
    static CtList from_json(const std::vector<int>& v) { return {v}; }
    static std::vector<int> to_json(const CtList& l) { return l.v; }
};
template <>
inline constexpr auto yyjson::annotations<CtList> = yyjson::annotate(
    yyjson::not_(yyjson::contains(yyjson::minimum(5))), yyjson::prefix_items(yyjson::maximum(3)), yyjson::unique_items);
struct CtMap
{
    std::map<std::string, int> m;
    bool operator==(const CtMap&) const = default;
};
template <>
struct yyjson::caster<CtMap>
{
    static CtMap from_json(const std::map<std::string, int>& m) { return {m}; }
    static std::map<std::string, int> to_json(const CtMap& o) { return o.m; }
};
template <>
inline constexpr auto yyjson::annotations<CtMap> =
    yyjson::annotate(yyjson::property("a", yyjson::minimum(0)), yyjson::additional_properties(yyjson::maximum(9)),
                     yyjson::dependent_required("a", {"b"}), yyjson::additional_properties(false),
                     yyjson::property("b", yyjson::minimum(0)));
struct CtNumber
{
    int n = 0;
    bool operator==(const CtNumber&) const = default;
};
template <>
struct yyjson::caster<CtNumber>
{
    static CtNumber from_json(int n) { return {n}; }
    static int to_json(const CtNumber& n) { return n.n; }
};
template <>
inline constexpr auto yyjson::annotations<CtNumber> =
    yyjson::annotate(yyjson::not_(yyjson::any_of(yyjson::minimum(5), yyjson::multiple_of(2))));
// the read reflects the struct, and the caster writes an array
struct CtWritten
{
    int a = 0;
    bool operator==(const CtWritten&) const = default;
};
template <>
struct yyjson::caster<CtWritten>
{
    static std::vector<int> to_json(const CtWritten& w) { return {w.a}; }
};
template <>
inline constexpr auto yyjson::annotations<CtWritten> =
    yyjson::annotate(yyjson::items(yyjson::minimum(0)), yyjson::property("a", yyjson::maximum(5)));
struct CtHolder
{
    CtList l;
    std::vector<CtMap> ms;
    bool operator==(const CtHolder&) const = default;
};

TEST(Constraints, CasterTypeKeywords)
{
    // the unique items of the JSON of a caster leave the type to the document
    expect_accepted<CtList>("[1,2]", {{1, 2}}, false);
    expect_rejected<CtList>("[1,5]", "the value satisfies the subschema of not; reading CtList", "");
    expect_rejected<CtList>("[4]", "the array has an item that does not satisfy prefixItems; reading CtList", "");
    expect_rejected<CtList>("[1,1]", "the array has duplicate items; reading CtList", "");
    EXPECT_NO_THROW(yyjson::validate(CtList{{1, 2}}));
    EXPECT_THROW(yyjson::validate(CtList{{6}}), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(CtList{{1, 1}}, yyjson::with_validation), yyjson::bad_cast);

    expect_accepted<CtMap>(R"({"a":1,"b":2})", {{{"a", 1}, {"b", 2}}});
    expect_rejected<CtMap>(R"({"a":1})",
                           "the object does not have all the keys that dependentRequired requires; reading CtMap", "");
    expect_rejected<CtMap>(R"({"a":-1,"b":1})", "the object does not satisfy properties; reading CtMap", "");
    expect_rejected<CtMap>(R"({"c":1})",
                           "the object has a key that is not allowed at /c; reading CtMap, which "
                           "yyjson::additional_properties(false) closes",
                           "/c");
    EXPECT_NO_THROW(yyjson::validate(CtMap{{{"a", 1}, {"b", 1}}}));
    EXPECT_THROW(yyjson::validate(CtMap{{{"b", -1}}}), yyjson::bad_cast);

    expect_accepted<CtNumber>("1", {1});
    expect_rejected<CtNumber>("2", "the value satisfies the subschema of not; reading CtNumber", "");
    expect_rejected<CtNumber>("7", "the value satisfies the subschema of not; reading CtNumber", "");
    EXPECT_THROW(yyjson::validate(CtNumber{2}), yyjson::bad_cast);

    // the read checks the object that it reads, and `validate` the array that the caster writes
    EXPECT_EQ(yyjson::read<CtWritten>(R"({"a":1})"), CtWritten{1});
    EXPECT_THROW((void)yyjson::read<CtWritten>(R"({"a":6})"), yyjson::bad_cast);
    EXPECT_NO_THROW(yyjson::validate(CtWritten{6}));
    EXPECT_THROW(yyjson::validate(CtWritten{-1}), yyjson::bad_cast);

    // a value at a position checks its caster type too, and a merge the JSON of the patch
    const auto e = bad_cast_of([] { (void)yyjson::read<CtHolder>(R"({"l":[1],"ms":[{"c":1}]})"); });
    EXPECT_EQ(e.path(), "/ms/0/c");
    auto holder = CtHolder{};
    const auto m = bad_cast_of([&] { yyjson::merge_into(R"({"l":[1,1]})", holder); });
    EXPECT_EQ(m.path(), "/l");
}

// a typed predicate on an element that a caster writes takes the C++ element, and the other keywords its JSON
struct C4Word
{
    std::string s;
    bool operator==(const C4Word&) const = default;
};
template <>
struct yyjson::caster<C4Word>
{
    static C4Word from_json(std::string s) { return {std::move(s)}; }
    static std::string to_json(const C4Word& w) { return w.s; }
};
inline bool c4_allowed(const C4Word& w) { return w.s != "no"; }
struct C4Words
{
    std::vector<C4Word> v;
    std::map<std::string, C4Word> m;
    bool operator==(const C4Words&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C4Words> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(3), yyjson::satisfies<c4_allowed>)),
                     yyjson::field("m", yyjson::additional_properties(yyjson::max_length(3),
                                                                      yyjson::all_of(yyjson::satisfies<c4_allowed>))));
// a merge checks a member that the patch merges into on the patch at each depth
struct C4Inner
{
    ScLevel e = ScLevel::zero;
    int k = 3;
    bool operator==(const C4Inner&) const = default;
};
struct C4Outer
{
    C4Inner in;
    bool operator==(const C4Outer&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C4Outer> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
// an enumeration with a caster takes the keywords of its JSON
enum class C4Code
{
    a,
    b
};
template <>
struct yyjson::caster<C4Code>
{
    static C4Code from_json(int i) { return static_cast<C4Code>(i); }
    static int to_json(const C4Code& c) { return static_cast<int>(c); }
};
template <>
inline constexpr auto yyjson::annotations<C4Code> = yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
// a class that is not an aggregate
class C4Number
{
    int n_ = 0;

public:
    C4Number(int n = 0) : n_(n) {}  // NOLINT(google-explicit-constructor)
    [[nodiscard]] int n() const { return n_; }
    bool operator==(const C4Number&) const = default;
};
template <>
struct yyjson::caster<C4Number>
{
    static C4Number from_json(int i) { return {i}; }
    static int to_json(const C4Number& x) { return x.n(); }
};
template <>
inline constexpr auto yyjson::annotations<C4Number> = yyjson::annotate(yyjson::all_of(yyjson::maximum(1)));
// `items(false)` on the array of a caster
struct C4Empty
{
    std::vector<int> v;
    bool operator==(const C4Empty&) const = default;
};
template <>
struct yyjson::caster<C4Empty>
{
    static C4Empty from_json(const std::vector<int>& v) { return {v}; }
    static std::vector<int> to_json(const C4Empty& x) { return x.v; }
};
template <>
inline constexpr auto yyjson::annotations<C4Empty> = yyjson::annotate(yyjson::items(false));
struct C4EmptyHolder
{
    C4Empty x;
    bool operator==(const C4EmptyHolder&) const = default;
};

TEST(Constraints, CasterElementPredicates)
{
    expect_accepted<C4Words>(R"({"v":["yes"],"m":{"k":"yes"}})", {{{"yes"}}, {{"k", {"yes"}}}});
    expect_rejected<C4Words>(
        R"({"v":["no"],"m":{}})",
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C4Words)",
        "/v/0");
    expect_rejected<C4Words>(
        R"({"v":["long"],"m":{}})",
        R"(the string is longer than the maximum length 3 at /v/0; reading an element of the value at the key "v" of C4Words)",
        "/v/0");
    expect_rejected<C4Words>(
        R"({"v":[],"m":{"k":"no"}})",
        R"(the value does not satisfy all the subschemas of allOf at /m/k; reading an element of the value at the key "m" of C4Words)",
        "/m/k");
    EXPECT_NO_THROW(yyjson::validate(C4Words{{{"yes"}}, {{"k", {"yes"}}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{{"no"}}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{}, {{"k", {"no"}}}}); }).path(), "/m/k");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C4Words{{}, {{"k", {"long"}}}}); }).path(), "/m/k");
}

TEST(Constraints, NestedMergeObject)
{
    expect_rejected<C4Outer>(R"({"in":{"e":1,"k":0}})",
                             "the object has a value that does not satisfy additionalProperties at /in; reading a "
                             "value of C4Outer in yyjson::additional_properties",
                             "/in", false);
    auto outer = C4Outer{};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"in":{"e":1,"k":0}})", outer); });
    EXPECT_EQ(e.path(), "/in");
    // the kept member k = 3 is checked on its written value
    outer = C4Outer{};
    EXPECT_THROW(yyjson::merge_into(R"({"in":{"e":0}})", outer), yyjson::bad_cast);
    outer = C4Outer{};
    yyjson::merge_into(R"({"in":{"e":0,"k":0}})", outer);
    EXPECT_EQ(outer, (C4Outer{{ScLevel::zero, 0}}));
}

TEST(Constraints, CasterTypeKeywordsOfOtherTypes)
{
    expect_accepted<C4Code>("0", C4Code::a);
    expect_rejected<C4Code>("1", "the value does not satisfy all the subschemas of allOf; reading C4Code", "");
    EXPECT_NO_THROW(yyjson::validate(C4Code::a));
    EXPECT_THROW(yyjson::validate(C4Code::b), yyjson::bad_cast);

    expect_accepted<C4Number>("1", C4Number{1});
    for (const auto& e : {bad_cast_of([] { (void)yyjson::read<C4Number>("2"); }),
                          bad_cast_of([] { (void)yyjson::read("2").cast<C4Number>(); }), bad_cast_of([] {
                              auto x = C4Number();
                              yyjson::merge_into("2", x);
                          })})
        EXPECT_EQ(what_without_position(e), "the value does not satisfy all the subschemas of allOf; reading C4Number");
    EXPECT_THROW(yyjson::validate(C4Number{2}), yyjson::bad_cast);

    expect_accepted<C4Empty>("[]", C4Empty{});
    expect_rejected<C4Empty>("[1]", "the array has an item that does not satisfy items; reading C4Empty", "");
    EXPECT_THROW(yyjson::validate(C4Empty{{1}}), yyjson::bad_cast);
    auto holder = C4EmptyHolder{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"x":[1]})", holder); }).path(), "/x");
}

// a merge into a variant with `internal_tag` checks the members of the patch at each depth
struct C5Named
{
    ScLevel e = ScLevel::zero;
    bool operator==(const C5Named&) const = default;
};
struct C5Counted
{
    int k = 0;
    ScLevel f = ScLevel::zero;
    bool operator==(const C5Counted&) const = default;
};
using C5Tagged = std::variant<C5Named, C5Counted>;
template <>
inline constexpr auto yyjson::annotations<C5Tagged> = yyjson::annotate(yyjson::internal_tag("type", {"i", "j"}));
struct C5TaggedHolder
{
    C5Tagged in = C5Counted{};
    bool operator==(const C5TaggedHolder&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C5TaggedHolder> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
// an enumeration with a caster that reads and writes a JSON object
enum class C5Shape
{
    a,
    b
};
template <>
struct yyjson::caster<C5Shape>
{
    static C5Shape from_json(const std::map<std::string, int>& m) { return m.size() > 1 ? C5Shape::b : C5Shape::a; }
    static std::map<std::string, int> to_json(const C5Shape& s)
    {
        return s == C5Shape::a ? std::map<std::string, int>{{"a", 1}} : std::map<std::string, int>{{"a", 1}, {"b", 1}};
    }
};
template <>
inline constexpr auto yyjson::annotations<C5Shape> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::property_names(yyjson::max_length(1)));
enum class C5Closed
{
    a
};
template <>
struct yyjson::caster<C5Closed>
{
    static C5Closed from_json(const std::map<std::string, int>&) { return C5Closed::a; }
    static std::map<std::string, int> to_json(const C5Closed&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<C5Closed> = yyjson::annotate(yyjson::additional_properties(false));
// the elements that a caster only writes, and a set of elements that do not read back as written
struct C5Text
{
    std::string s;
    auto operator<=>(const C5Text&) const = default;
};
template <>
struct yyjson::caster<C5Text>
{
    static std::string to_json(const C5Text& t) { return "!" + t.s; }
};
template <>
struct std::hash<C5Text>
{
    std::size_t operator()(const C5Text& t) const noexcept { return std::hash<std::string>()(t.s); }
};
inline bool c5_allowed(const C5Text& t) { return t.s != "no"; }
struct C5Texts
{
    std::vector<C5Text> v;
    std::set<C5Text> s;
    std::unordered_set<C5Text> u;
};
template <>
inline constexpr auto yyjson::annotations<C5Texts> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)),
                     yyjson::field("s", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)),
                     yyjson::field("u", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_allowed>)));
struct C5Word
{
    std::string s;
    auto operator<=>(const C5Word&) const = default;
};
template <>
struct yyjson::caster<C5Word>
{
    static C5Word from_json(const std::string& s)
    {
        if (s.starts_with("!")) throw std::runtime_error("not readable");
        return {s};
    }
    static std::string to_json(const C5Word& w) { return "!" + w.s; }
};
inline bool c5_word_allowed(const C5Word& w) { return w.s != "no"; }
struct C5Words
{
    std::set<C5Word> v;
    bool operator==(const C5Words&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C5Words> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::max_length(4), yyjson::satisfies<c5_word_allowed>)));

TEST(Constraints, MergeTaggedVariant)
{
    const auto merged = [](std::string_view patch, C5TaggedHolder h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_THROW((void)yyjson::read<C5TaggedHolder>(R"({"in":{"type":"i","e":1}})"), yyjson::bad_cast);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"i","e":1}})", {}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"i"}})", {}), C5TaggedHolder{C5Named{}});
    // the alternative that the holder keeps takes the patch at its members
    EXPECT_THROW((void)merged(R"({"in":{"f":1}})", {}), yyjson::bad_cast);
    const auto kept = C5TaggedHolder{C5Counted{0, ScLevel::one}};
    EXPECT_EQ(merged(R"({"in":{"k":0}})", kept), kept);
    EXPECT_THROW((void)merged(R"({"in":{"type":"j","f":1}})", {}), yyjson::bad_cast);
}

TEST(Constraints, CasterEnumObjectKeywords)
{
    expect_accepted<C5Shape>(R"({"a":1,"b":1})", C5Shape::b);
    for (const auto& e : {bad_cast_of([] { (void)yyjson::read<C5Shape>(R"({"a":1})"); }),
                          bad_cast_of([] { (void)yyjson::read(R"({"a":1})").cast<C5Shape>(); }), bad_cast_of([] {
                              auto x = C5Shape::b;
                              yyjson::merge_into(R"({"a":1})", x);
                          })})
        EXPECT_EQ(what_without_position(e), "the object has fewer properties than the minimum 2; reading C5Shape");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C5Shape>(R"({"a":1,"bb":1})"); }).path(), "/bb");
    EXPECT_NO_THROW(yyjson::validate(C5Shape::b));
    EXPECT_THROW(yyjson::validate(C5Shape::a), yyjson::bad_cast);

    EXPECT_NO_THROW((void)yyjson::read<C5Closed>("{}"));
    EXPECT_THROW((void)yyjson::read<C5Closed>(R"({"a":1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"a":1})").cast<C5Closed>(), yyjson::bad_cast);
}

TEST(Constraints, WrittenCasterElementsOfSets)
{
    EXPECT_NO_THROW(yyjson::validate(C5Texts{{{"yes"}}, {{"yes"}}, {{"a"}, {"b"}, {"yes"}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{{"no"}}, {}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{}, {{"a"}, {"no"}}, {}}); }).path(), "/s/1");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C5Texts{{}, {{"a"}, {"long"}}, {}}); }).path(), "/s/1");
    // the path is the position of the element in the written array
    const auto u = C5Texts{{}, {}, {{"a"}, {"no"}, {"b"}}};
    const auto at = std::ranges::distance(u.u.begin(), std::ranges::find(u.u, C5Text{"no"}));
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(u); }).path(), "/u/" + std::to_string(at));
    EXPECT_THROW((void)yyjson::write(u, yyjson::with_validation), yyjson::bad_cast);

    expect_accepted<C5Words>(R"({"v":["yes"]})", {{{"yes"}}});
    expect_rejected<C5Words>(
        R"({"v":["no"]})",
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C5Words)",
        "/v/0");
    EXPECT_NO_THROW(yyjson::validate(C5Words{{{"yes"}}}));
    EXPECT_EQ(
        message_of(bad_cast_of([] { yyjson::validate(C5Words{{{"no"}}}); })),
        R"(the value does not satisfy the predicate at /v/0; reading an element of the value at the key "v" of C5Words)");
    EXPECT_EQ(
        message_of(bad_cast_of([] { yyjson::validate(C5Words{{{"long"}}}); })),
        R"(the string is longer than the maximum length 4 at /v/0; reading an element of the value at the key "v" of C5Words)");
}

// the elements of a vector<bool> and the values of a map, which the check copies
struct C6Flags
{
    std::vector<bool> v;
    std::map<std::string, bool> m;
};
template <>
inline constexpr auto yyjson::annotations<C6Flags> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::const_(true))),
                     yyjson::field("m", yyjson::additional_properties(yyjson::const_(true))));
// a read inside validate checks its own input
struct C6Letter
{
    std::string s;
    auto operator<=>(const C6Letter&) const = default;
};
template <>
struct yyjson::caster<C6Letter>
{
    static C6Letter from_json(const std::string& s) { return {s}; }
    static std::string to_json(const C6Letter& c) { return c.s; }
};
inline bool c6_is_a(const C6Letter& c) { return c.s == "a"; }
struct C6Letters
{
    std::set<C6Letter> v;
};
template <>
inline constexpr auto yyjson::annotations<C6Letters> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::max_length(1), yyjson::satisfies<c6_is_a>))));
inline bool c6_reads_inside(int)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read(R"({"v":["long","a"]})").cast<C6Letters>(); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C6Letters>(R"({"v":["long","a"]})"); }).path(), "/v/0");
    return true;
}
struct C6Outer
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<C6Outer> =
    yyjson::annotate(yyjson::field("x", yyjson::satisfies<c6_reads_inside>));
// casters whose written value differs from the value that they read
struct C6Step
{
    int n = 0;
    bool operator==(const C6Step&) const = default;
};
template <>
struct yyjson::caster<C6Step>
{
    static C6Step from_json(int n) { return {n}; }
    static int to_json(const C6Step& c) { return c.n + 1; }
};
template <>
inline constexpr auto yyjson::annotations<C6Step> = yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
enum class C6Level
{
    a,
    b
};
template <>
struct yyjson::caster<C6Level>
{
    static C6Level from_json(int n) { return static_cast<C6Level>(n); }
    static int to_json(C6Level x) { return static_cast<int>(x) + 1; }
};
template <>
inline constexpr auto yyjson::annotations<C6Level> = yyjson::annotate(yyjson::all_of(yyjson::maximum(1)));
struct C6Kept
{
    C6Step c;
    C6Level x = C6Level::a;
    int n = 0;
    bool operator==(const C6Kept&) const = default;
};
// a patch that switches the alternative gives a new one, read from the patch alone
struct C6First
{
    int a = 0;
    bool operator==(const C6First&) const = default;
};
struct C6Second
{
    std::optional<int> k = 1;
    bool operator==(const C6Second&) const = default;
};
using C6Choice = std::variant<C6First, C6Second>;
template <>
inline constexpr auto yyjson::annotations<C6Choice> = yyjson::annotate(yyjson::internal_tag("type", {"a", "b"}));
struct C6Switch
{
    C6Choice in;
    bool operator==(const C6Switch&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C6Switch> =
    yyjson::annotate(yyjson::additional_properties(yyjson::additional_properties(yyjson::maximum(0))));
struct C6Counted
{
    C6Choice in;
    bool operator==(const C6Counted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<C6Counted> =
    yyjson::annotate(yyjson::additional_properties(yyjson::min_properties(2)));

TEST(Constraints, WrittenProxyElements)
{
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{false}, {}}); }).path(), "/v/0");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{true, false}, {}}); }).path(), "/v/1");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(C6Flags{{}, {{"a", false}}}); }).path(), "/m/a");
    EXPECT_NO_THROW(yyjson::validate(C6Flags{{true, true}, {{"a", true}}}));
}

TEST(Constraints, ReadInsideValidate)
{
    EXPECT_NO_THROW(yyjson::validate(C6Outer{}));
    EXPECT_NO_THROW((void)yyjson::write(C6Outer{}, yyjson::with_validation));
}

TEST(Constraints, MergeKeepsCasterValues)
{
    const auto merged = [](std::string_view patch, C6Kept k) {
        yyjson::merge_into(patch, k);
        return k;
    };
    const auto kept = yyjson::read<C6Kept>(R"({"c":0,"x":0,"n":0})");
    EXPECT_EQ(yyjson::write(kept), R"({"c":1,"x":1,"n":0})");
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(kept); }).path(), "/c");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", kept); }).path(), "/c");
    // the patch gives the value that the check takes
    EXPECT_EQ(merged(R"({"c":0,"n":1})", kept).n, 1);
    EXPECT_EQ(merged(R"({"c":-1,"x":0})", {}).n, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"c":1})", {}); }).path(), "/c");
    const auto level = C6Kept{{-1}, C6Level::b, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", level); }).path(), "/x");
    EXPECT_EQ(merged(R"({"x":1})", level).x, C6Level::b);
}

TEST(Constraints, MergeSwitchesAlternative)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_EQ(yyjson::write(yyjson::read<C6Switch>(R"({"in":{"type":"b"}})")), R"({"in":{"type":"b","k":1}})");
    EXPECT_EQ(merged(R"({"in":{"type":"b"}})", C6Switch{}), C6Switch{C6Second{}});
    EXPECT_EQ(merged(R"({"in":{"type":"b","k":0}})", C6Switch{}), C6Switch{C6Second{0}});
    EXPECT_THROW((void)merged(R"({"in":{"type":"b","k":1}})", C6Switch{}), yyjson::bad_cast);
    // the alternative that the patch names is kept, and its written members count
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"b"}})", C6Switch{C6Second{}}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"a"}})", C6Switch{C6Second{}}), C6Switch{});

    EXPECT_THROW((void)yyjson::read<C6Counted>(R"({"in":{"type":"b"}})"), yyjson::bad_cast);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"type":"b"}})", C6Counted{}); }).path(), "/in");
    EXPECT_EQ(merged(R"({"in":{"type":"b"}})", C6Counted{C6Second{}}), C6Counted{C6Second{}});
}

struct C7Pair
{
    int x = 0;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Pair> = yyjson::annotate(yyjson::field("x", yyjson::maximum(0)));
struct C7Nested
{
    C7Pair in;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::map<std::string, C7Pair> pairs;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Nested> = yyjson::annotate(
    yyjson::field("v", yyjson::max_items(1)), yyjson::field("m", yyjson::additional_properties(yyjson::maximum(0))));
struct C7Other
{
    std::optional<int> k = 1;
    bool operator==(const C7Other&) const = default;
};
struct C7Adjacent
{
    std::variant<C7Pair, C7Other> v;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Adjacent> =
    yyjson::annotate(yyjson::field("v", yyjson::adjacent_tag("kind", {"a", "b"})));
enum class C7Key
{
    x,
    y
};
struct C7EnumKeys
{
    std::map<C7Key, C7Pair> m;
    std::map<C7Key, int> counts;
};
template <>
inline constexpr auto yyjson::annotations<C7EnumKeys> =
    yyjson::annotate(yyjson::field("counts", yyjson::additional_properties(yyjson::maximum(0))));

TEST(Constraints, MergeChecksKeptFields)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C7Pair>(R"({"x":1,"n":1})"); }).path(), "/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", C7Pair{1, 0}); }).path(), "/x");
    EXPECT_EQ(merged(R"({"x":0})", C7Pair{1, 0}).x, 0);

    const auto bad_in = C7Nested{{1, 0}, {}, {}, {}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", bad_in); }).path(), "/in/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"in":{"n":1}})", bad_in); }).path(), "/in/x");
    EXPECT_EQ(merged(R"({"in":{"x":0}})", bad_in).in.x, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", C7Nested{{}, {1, 2}, {}, {}, 0}); }).path(), "/v");
    const auto bad_m = C7Nested{{}, {}, {{"a", 1}}, {}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"b":0}})", bad_m); }).path(), "/m/a");
    EXPECT_EQ(merged(R"({"m":{"a":0}})", bad_m).m.at("a"), 0);
    const auto bad_pairs = C7Nested{{}, {}, {}, {{"a", {1, 0}}}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"pairs":{"b":{}}})", bad_pairs); }).path(), "/pairs/a/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"pairs":{"a":{"n":1}}})", bad_pairs); }).path(), "/pairs/a/x");
}

TEST(Constraints, MergeChecksKeptAdjacentAlternative)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto bad = C7Adjacent{C7Pair{1, 0}, 0};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"n":1})", bad); }).path(), "/v/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"v":{"n":1}})", bad); }).path(), "/v/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"kind":"a","v":{"n":1}})", bad); }).path(), "/v/x");
    EXPECT_EQ(std::get<C7Pair>(merged(R"({"v":{"x":0}})", bad).v).x, 0);
    // a tag that names another alternative reads it from the patch alone
    EXPECT_EQ(std::get<C7Other>(merged(R"({"kind":"b","v":{}})", bad).v), C7Other{});
}

TEST(Constraints, MergeChecksKeptEnumKeys)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto bad = C7EnumKeys{{{C7Key::x, {1, 0}}}, {}};
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"y":{}}})", bad); }).path(), "/m/x/x");
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"m":{"x":{"n":1}}})", bad); }).path(), "/m/x/x");
    EXPECT_EQ(merged(R"({"m":{"x":{"x":0}}})", bad).m.at(C7Key::x).x, 0);
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"counts":{"y":0}})", C7EnumKeys{{}, {{C7Key::x, 1}}}); }).path(),
              "/counts/x");
}

// a struct whose map of the other keys merges objects into its values
struct C7Spot
{
    int x = 0;
    std::optional<int> y;
};
struct C7Extra
{
    int a = 0;
    std::map<std::string, C7Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C7Extra> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("a"),
                     yyjson::additional_properties(yyjson::min_properties(2)));

TEST(Constraints, MergeChecksExtraKeyObjects)
{
    // the object that a patch merges into a value of the map is checked as merged
    auto t = C7Extra{0, {{"r", {1, 2}}}};
    yyjson::merge_into(R"({"r":{"x":3}})", t);
    EXPECT_EQ(written(t), R"({"a":0,"r":{"x":3,"y":2}})");
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"r":{"y":null}})", t); }).path(), "/r");
}

// a field that yyjson::adjacent_tag tags, whose content a property of the struct around it checks
struct C7Two
{
    std::optional<int> a;
    std::optional<int> b;
};
struct C7AdjacentTwo
{
    std::variant<C7Two, C7Other> v;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7AdjacentTwo> =
    yyjson::annotate(yyjson::field("v", yyjson::adjacent_tag("kind", {"a", "b"})));
struct C7AdjacentOuter
{
    C7AdjacentTwo h;
};
template <>
inline constexpr auto yyjson::annotations<C7AdjacentOuter> =
    yyjson::annotate(yyjson::field("h", yyjson::property("v", yyjson::max_properties(1))));

TEST(Constraints, MergeChecksMergedAdjacentContent)
{
    const auto merged = [](std::string_view patch, auto h) {
        yyjson::merge_into(patch, h);
        return h;
    };
    const auto t = C7AdjacentOuter{{C7Two{1, {}}, 0}};
    EXPECT_EQ(
        bad_cast_of([] { (void)yyjson::read<C7AdjacentOuter>(R"({"h":{"kind":"a","v":{"a":1,"b":2}}})"); }).path(),
        "/h");
    // the content is checked as merged: {"a":1,"b":2}, then {"b":2}
    EXPECT_EQ(bad_cast_of([&] { (void)merged(R"({"h":{"v":{"b":2}}})", t); }).path(), "/h");
    EXPECT_EQ(written(merged(R"({"h":{"v":{"a":null,"b":2}}})", t)), R"({"h":{"kind":"a","v":{"b":2},"n":0}})");
    EXPECT_EQ(written(merged(R"({"h":{"kind":"b","v":{"k":2}}})", t)), R"({"h":{"kind":"b","v":{"k":2},"n":0}})");
}

// a field whose type has no JSON to write: the merge does not check the value that it keeps
struct C7ReadOnly
{
    int n = 0;
};
template <>
struct yyjson::caster<C7ReadOnly>
{
    static C7ReadOnly from_json(int n) { return {n}; }
};
struct C7Unwritable
{
    C7ReadOnly r;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C7Unwritable> = yyjson::annotate(yyjson::field("r", yyjson::minimum(0)));

TEST(Constraints, MergeKeepsUnwritableValues)
{
    EXPECT_THROW((void)yyjson::read<C7Unwritable>(R"({"r":-1,"n":0})"), yyjson::bad_cast);
    auto t = C7Unwritable{{-1}, 0};
    yyjson::merge_into(R"({"n":1})", t);
    EXPECT_EQ(t.r.n, -1);
    EXPECT_EQ(t.n, 1);
    EXPECT_THROW(yyjson::merge_into(R"({"r":-2})", t), yyjson::bad_cast);
}

// a struct with keywords on the keys of its object only, inside another struct
struct C9Pairless
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Pairless> = yyjson::annotate(yyjson::min_properties(2));
struct C9HoldsPairless
{
    C9Pairless in;
    int n = 0;
};
// a struct whose keys `yyjson::property_names` checks, and a keyword on another field
struct C9Names
{
    int longname = 0;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Names> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(1)), yyjson::field("n", yyjson::maximum(0)));
// maps of the other keys whose values the patch creates
struct C9Spot
{
    int x = 0;
};
struct C9Rest
{
    std::map<std::string, C9Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C9Rest> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::additional_properties(yyjson::min_properties(1)));
struct C9Pattern
{
    std::map<std::string, C9Spot> rest;
};
template <>
inline constexpr auto yyjson::annotations<C9Pattern> = yyjson::annotate(
    yyjson::field("rest", yyjson::pattern_properties("^r")), yyjson::additional_properties(yyjson::min_properties(1)));
// a struct whose other keys must hold objects with a member
struct C9Closed
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Closed> =
    yyjson::annotate(yyjson::additional_properties(yyjson::min_properties(1)));
// the types of the differential test
struct C9Inner
{
    int x = 0;
    std::optional<int> y;
};
template <>
inline constexpr auto yyjson::annotations<C9Inner> =
    yyjson::annotate(yyjson::field("x", yyjson::maximum(5)), yyjson::max_properties(2));
struct C9Other
{
    int k = 0;
};
using C9Internal = std::variant<C9Inner, C9Other>;
template <>
inline constexpr auto yyjson::annotations<C9Internal> = yyjson::annotate(yyjson::internal_tag("type", {"i", "o"}));
struct C9Target
{
    C9Inner in;
    std::map<std::string, int> m;
    std::vector<int> v;
    std::optional<int> o;
    C9Internal u;
    std::variant<C9Inner, C9Other> a;
    std::map<std::string, C9Inner> rest;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C9Target> = yyjson::annotate(
    yyjson::field("m", yyjson::max_properties(2), yyjson::additional_properties(yyjson::maximum(3))),
    yyjson::field("v", yyjson::max_items(2)), yyjson::field("o", yyjson::maximum(1)),
    yyjson::field("u", yyjson::max_properties(2)), yyjson::field("a", yyjson::adjacent_tag("kind", {"i", "o"})),
    yyjson::field("rest", yyjson::extra_keys), yyjson::dependent_required("o", {"r"}), yyjson::max_properties(9));

TEST(Constraints, MergeChecksObjectKeywordsOfKeptStructs)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9HoldsPairless>(R"({"in":{"x":0},"n":1})"); }).path(), "/in");
    auto h = C9HoldsPairless{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":1})", h); }).path(), "/in");
    EXPECT_EQ(h.n, 0);
}

TEST(Constraints, MergeChecksAllKeyNames)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Names>(R"({"longname":0,"n":0})"); }).path(), "/longname");
    auto t = C9Names{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":0})", t); }).path(), "/longname");
}

TEST(Constraints, MergeChecksNewMapValues)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Rest>(R"({"a":{}})"); }).path(), "/a");
    auto rest = C9Rest{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"a":{}})", rest); }).path(), "/a");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Pattern>(R"({"r":{}})"); }).path(), "/r");
    auto pattern = C9Pattern{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"r":{}})", pattern); }).path(), "/r");
    EXPECT_TRUE(pattern.rest.empty());
}

TEST(Constraints, MergeRemovesNullMembersOfOtherKeys)
{
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C9Closed>(R"({"n":0,"extra":{}})"); }).path(), "/extra");
    auto t = C9Closed{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"extra":{"x":null}})", t); }).path(), "/extra");
}

// merge_into rejects a patch if and only if read rejects the object that the patch merges into the written target
TEST(Constraints, MergeChecksAsReadOfMergedObject)
{
    const auto path_of = [](auto&& f) -> std::optional<std::string> {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::string(e.path());
        }
        return std::nullopt;
    };
    const auto expect_as_read = [&](const C9Target& target, std::string_view patch, std::string_view merged,
                                    bool kept = true) {
        const auto read = path_of([&] { (void)yyjson::read<C9Target>(merged); });
        auto t = target;
        const auto merge = path_of([&] { yyjson::merge_into(patch, t); });
        EXPECT_EQ(merge, read) << patch;
        if (merge.has_value() && kept)
        {
            EXPECT_EQ(written(t), written(target)) << patch;
        }
    };
    auto t = C9Target{};
    t.m = {{"a", 1}};
    t.v = {1};
    t.o = 1;
    t.rest = {{"r", {}}};
    ASSERT_EQ(written(t), R"({"in":{"x":0},"m":{"a":1},"v":[1],"o":1,"u":{"type":"i","x":0},"kind":"i","a":{"x":0},)"
                          R"("n":0,"r":{"x":0}})");
    const auto with = [](std::string_view in, std::string_view m, std::string_view v, std::string_view o,
                         std::string_view u, std::string_view a, std::string_view n, std::string_view rest) {
        return std::format(R"({{"in":{},"m":{},"v":{}{},"u":{},{},"n":{}{}}})", in, m, v, o, u, a, n, rest);
    };
    const auto base = [&](std::string_view n) {
        return with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                    R"("kind":"i","a":{"x":0})", n, R"(,"r":{"x":0})");
    };
    // nested structs and optional fields
    expect_as_read(t, R"({"n":1})", base("1"));
    expect_as_read(t, R"({"in":{"x":6}})",
                   with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"in":{"y":1}})",
                   with(R"({"x":0,"y":1})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    auto kept_y = t;
    kept_y.in.y = 1;
    const auto with_y = with(R"({"x":0,"y":1})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                             R"("kind":"i","a":{"x":0})", "1", R"(,"r":{"x":0})");
    expect_as_read(kept_y, R"({"n":1})", with_y);
    expect_as_read(kept_y, R"({"in":{"y":null}})", base("0"));
    expect_as_read(t, R"({"o":2})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":2)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"o":null})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", "", R"({"type":"i","x":0})", R"("kind":"i","a":{"x":0})",
                        "0", R"(,"r":{"x":0})"));
    // maps, arrays, and null removal
    expect_as_read(t, R"({"m":{"b":2}})",
                   with(R"({"x":0})", R"({"a":1,"b":2})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":2,"c":3}})",
                   with(R"({"x":0})", R"({"a":1,"b":2,"c":3})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"a":null,"b":2,"c":3}})",
                   with(R"({"x":0})", R"({"b":2,"c":3})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":4}})",
                   with(R"({"x":0})", R"({"a":1,"b":4})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"v":[1,2,3]})",
                   with(R"({"x":0})", R"({"a":1})", "[1,2,3]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"v":[5]})",
                   with(R"({"x":0})", R"({"a":1})", "[5]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    // the members of `yyjson::extra_keys`, and the dependent key that a null removes
    expect_as_read(t, R"({"s":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0},"s":{"x":6})"));
    expect_as_read(t, R"({"r":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0,"y":1})"));
    expect_as_read(t, R"({"r":null})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", ""));
    expect_as_read(t, R"({"zz":null})", base("0"));
    // the internal tag and the adjacent tag: a tag that names another alternative takes the patch alone
    expect_as_read(t, R"({"u":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":6})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"u":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0,"y":1})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"u":{"type":"o","k":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"o","k":1})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"a":{"x":6}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":6})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"a":{"y":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0,"y":1})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"kind":"o","a":{"k":1}})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"o","a":{"k":1})", "0", R"(,"r":{"x":0})"));
    // repeated keys apply in their order, and the last one gives the value
    expect_as_read(t, R"({"n":1,"n":2})", base("2"));
    expect_as_read(t, R"({"n":2,"n":1})", base("1"));
    expect_as_read(t, R"({"m":{"b":1,"b":5}})",
                   with(R"({"x":0})", R"({"a":1,"b":5})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    expect_as_read(t, R"({"m":{"b":5,"b":1}})",
                   with(R"({"x":0})", R"({"a":1,"b":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
    // the merge checks each value of a repeated key as it applies it, as read checks each, after the values before
    const auto first_in = with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                               R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})");
    expect_as_read(t, R"({"in":{"x":6},"in":{"x":0}})", first_in, false);
    expect_as_read(t, R"({"in":{"x":6,"x":0}})", first_in, false);
    expect_as_read(t, R"({"o":2,"o":1})",
                   with(R"({"x":0})", R"({"a":1})", "[1]", R"(,"o":2)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"),
                   false);
    expect_as_read(t, R"({"in":{"x":0,"x":6}})",
                   with(R"({"x":6})", R"({"a":1})", "[1]", R"(,"o":1)", R"({"type":"i","x":0})",
                        R"("kind":"i","a":{"x":0})", "0", R"(,"r":{"x":0})"));
}

struct C10Count
{
    int n = 0;
};
inline int c10_reads = 0;
template <>
struct yyjson::caster<C10Count>
{
    static C10Count from_json(int n)
    {
        ++c10_reads;
        return {n};
    }
    static int to_json(const C10Count& c) { return c.n + 1; }
};
inline bool c10_is_five(const C10Count& c) { return c.n == 5; }
struct C10Holder
{
    C10Count c;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<C10Holder> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<c10_is_five>), yyjson::field("n", yyjson::maximum(1)));

struct C10Outer
{
    C10Holder in;
    int m = 0;
};

// the merge checks a kept value on the C++ value, and reads only the values of the patch
TEST(Constraints, MergeChecksKeptValuesWithoutReadingThem)
{
    auto h = C10Holder{{5}, 0};
    c10_reads = 0;
    yyjson::merge_into(R"({"n":1})", h);
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(h.c.n, 5);
    EXPECT_EQ(h.n, 1);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"n":2})", h); }).path(), "/n");
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C10Holder>(R"({"c":7,"n":0})"); }).path(), "/c");
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"c":7})", h); }).path(), "/c");
    EXPECT_EQ(h.c.n, 5);
    yyjson::merge_into(R"({"c":5,"n":0})", h);
    EXPECT_EQ(h.c.n, 5);
    EXPECT_EQ(h.n, 0);
    auto outer = C10Outer{{{5}, 0}, 0};
    c10_reads = 0;
    yyjson::merge_into(R"({"in":{"n":1}})", outer);
    EXPECT_EQ(c10_reads, 0);
    EXPECT_EQ(outer.in.n, 1);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"in":{"n":2}})", outer); }).path(), "/in/n");
}

struct C10Pair
{
    int a = 0;
    int b = 0;
};
inline bool c10_sum_small(const C10Pair& p) { return p.a + p.b <= 1; }
struct C10Typed
{
    C10Pair in;
    std::map<std::string, C10Pair> rest;
};
template <>
inline constexpr auto yyjson::annotations<C10Typed> = yyjson::annotate(
    yyjson::field("in", yyjson::satisfies<c10_sum_small>),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::satisfies<c10_sum_small>)));

// a predicate that takes a C++ value checks a value that the patch merges into after the merge
TEST(Constraints, MergeChecksTypedPredicatesAfterTheMerge)
{
    const auto target = C10Typed{{1, 0}, {{"k", {1, 0}}}};
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<C10Typed>(R"({"in":{"a":1,"b":1}})"); }).path(), "/in");
    auto t = target;
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"in":{"b":1}})", t); }).path(), "/in");
    t = target;
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"k":{"b":1}})", t); }).path(), "/k");
    t = target;
    yyjson::merge_into(R"({"in":{"a":0,"b":1},"k":{"a":0}})", t);
    EXPECT_EQ(t.in.b, 1);
    EXPECT_EQ(t.rest.at("k").a, 0);
}

// a merge walks a member that no field takes without recursion, at any depth of the patch
struct R11Deep
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Deep> = yyjson::annotate(yyjson::field("n", yyjson::maximum(1)));

TEST(Constraints, MergeWalksADeepPatchWithoutRecursion)
{
    constexpr auto depth = 200000;
    auto patch = std::string(R"({"n":0,"x":null,"extra":)");
    for (auto i = 0; i < depth; ++i) patch += R"({"a":null,"b":)";
    patch += '0';
    patch.append(depth, '}');
    patch += '}';
    auto t = R11Deep{1};
    yyjson::merge_into(patch, t);
    EXPECT_EQ(t.n, 0);
}

// a keyword of a field on the members of its object checks the merged members
struct R11Point
{
    int x = 0;
    int y = 0;
};
struct R11Property
{
    R11Point f;
};
template <>
inline constexpr auto yyjson::annotations<R11Property> =
    yyjson::annotate(yyjson::field("f", yyjson::property("x", yyjson::maximum(0))));

TEST(Constraints, MergeChecksAFieldPropertyOnTheMergedMember)
{
    expect_rejected<R11Property>(R"({"f":{"x":1}})",
                                 R"(the object does not satisfy properties at /f; reading the key "f" of R11Property)",
                                 "/f", false);
    auto t = R11Property{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", t); }).path(), "/f");
    t = R11Property{{1, 0}};
    yyjson::merge_into(R"({"f":{"x":0}})", t);
    EXPECT_EQ(t.f.x, 0);
}

// a predicate that takes a C++ value checks the merged value, inside a condition and through a transparent type
inline bool r11_sum_small(const R11Point& p) { return p.x + p.y <= 0; }
inline bool r11_any(const R11Point&) { return true; }
struct R11Condition
{
    R11Point f;
};
struct R11Kept
{
    R11Point f;
    int k = 1;
};
struct R11Wrapper
{
    R11Point value;
};
struct R11Wrapped
{
    R11Wrapper w;
};
template <>
inline constexpr auto yyjson::annotations<R11Condition> =
    yyjson::annotate(yyjson::all_of(yyjson::property("f", yyjson::satisfies<r11_sum_small>)));
template <>
inline constexpr auto yyjson::annotations<R11Kept> = yyjson::annotate(
    yyjson::all_of(yyjson::property("f", yyjson::satisfies<r11_any>), yyjson::property("k", yyjson::maximum(0))));
template <>
inline constexpr auto yyjson::annotations<R11Wrapper> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::satisfies<r11_sum_small>));

TEST(Constraints, MergeChecksEveryDeferredPredicate)
{
    auto c = R11Condition{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", c); }).path(), "/f");
    auto k = R11Kept{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":0}})", k); }).path(), "/k");
    auto w = R11Wrapped{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"w":{"x":1}})", w); }).path(), "/w");
    yyjson::merge_into(R"({"f":{"x":-1}})", c);
    EXPECT_EQ(c.f.x, -1);
}

// a schema that also takes a C++ value checks the merged JSON, which the merge does not write again
struct R11Mixed
{
    R11Point f;
};
template <>
inline constexpr auto yyjson::annotations<R11Mixed> =
    yyjson::annotate(yyjson::field("f", yyjson::min_properties(2), yyjson::satisfies<r11_any>));

TEST(Constraints, MergeChecksAMixedSchemaOnTheMergedJson)
{
    expect_rejected<R11Mixed>(
        R"({"f":{"y":0}})",
        R"(the object has fewer properties than the minimum 2 at /f; reading the key "f" of R11Mixed)", "/f", false);
    auto t = R11Mixed{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":null}})", t); }).path(), "/f");
}

// a value that only writes leaves the checks of the other values in place
struct R11WriteOnly
{
    int x = 0;
};
template <>
struct yyjson::caster<R11WriteOnly>
{
    static std::map<std::string, int> to_json(const R11WriteOnly& c) { return {{"x", c.x}}; }
};
struct R11WithWriteOnly
{
    R11WriteOnly c;
    int n = 1;
};
template <>
inline constexpr auto yyjson::annotations<R11WithWriteOnly> = yyjson::annotate(yyjson::field("n", yyjson::maximum(0)));

TEST(Constraints, MergeChecksKeptValuesBesideAWriteOnlyValue)
{
    EXPECT_THROW(yyjson::validate(R11WithWriteOnly{}), yyjson::bad_cast);
    auto t = R11WithWriteOnly{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into("{}", t); }).path(), "/n");
}

// `validate` checks a value of a map with its C++ value, and does not read the JSON that it writes
struct R11Prefixed
{
    std::string s;
};
inline int r11_prefixed_reads = 0;
template <>
struct yyjson::caster<R11Prefixed>
{
    static R11Prefixed from_json(std::string s)
    {
        ++r11_prefixed_reads;
        if (s.starts_with("!")) throw std::runtime_error("unreadable prefix");
        return {std::move(s)};
    }
    static std::string to_json(const R11Prefixed& c) { return "!" + c.s; }
};
struct R11PrefixedMap
{
    std::map<std::string, R11Prefixed> rest;
};
template <>
inline constexpr auto yyjson::annotations<R11PrefixedMap> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, ValidateBindsAMapValueToItsEntry)
{
    r11_prefixed_reads = 0;
    EXPECT_NO_THROW(yyjson::validate(R11PrefixedMap{{{"a", {"ab"}}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(R11PrefixedMap{{{"a", {"abcd"}}}}); }).path(), "/a");
    EXPECT_EQ(r11_prefixed_reads, 0);
    // a merge reads only the value of the patch, and binds the kept value "!ab" to its entry
    auto t = R11PrefixedMap{{{"a", {"ab"}}}};
    yyjson::merge_into(R"({"b":"cd"})", t);
    EXPECT_EQ(t.rest.size(), 2U);
}

// a member that the patch replaces keeps no binding to the value before the merge
struct R11Negative
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Negative> = yyjson::annotate(yyjson::field("x", yyjson::maximum(-1)));
struct R11Replaced
{
    R11Negative f;
};

TEST(Constraints, MergeReplacesAMemberForTheLaterDuplicates)
{
    auto t = R11Replaced{};
    yyjson::merge_into(R"({"f":null,"f":{"x":-1},"f":{"y":0}})", t);
    EXPECT_EQ(t.f.x, -1);
}

// a merge into an empty optional checks the patch before it sets the optional
struct R11Root
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Root> = yyjson::annotate(yyjson::field("y", yyjson::minimum(0)));

TEST(Constraints, MergeChecksAnEmptyRootBeforeTheCommit)
{
    auto t = std::optional<R11Root>{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"x":1,"y":-1})", t); }).path(), "/y");
    EXPECT_FALSE(t.has_value());
}

// a predicate of the values of a map gives the key of the value that it rejects
struct R11Pair
{
    int a = 0;
    int b = 0;
};
inline bool r11_pair_small(const R11Pair& c) { return c.a + c.b <= 1; }
struct R11Pairs
{
    std::map<std::string, R11Pair> m;
};
template <>
inline constexpr auto yyjson::annotations<R11Pairs> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::satisfies<r11_pair_small>)));

TEST(Constraints, MergeGivesTheKeyOfARejectedMapValue)
{
    auto t = R11Pairs{{{"k", {1, 0}}}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"m":{"k":{"b":1}}})", t); }).path(), "/m/k");
}

// a check before a merge that does not depend on the merged C++ value rejects the patch before the commit
struct R12Pair
{
    int x = 0;
    int n = 0;
};
inline bool r12_yes(const R12Pair&) { return true; }
inline bool r12_small(const R12Pair& f) { return f.x <= 1; }
inline bool r12_zero(const int& n) { return n == 0; }
struct R12Counted
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12Counted> =
    yyjson::annotate(yyjson::field("f", yyjson::min_properties(2), yyjson::satisfies<r12_yes>));
struct R12Condition
{
    R12Pair f;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<R12Condition> =
    yyjson::annotate(yyjson::all_of(yyjson::property("n", yyjson::satisfies<r12_zero>)));

TEST(Constraints, MergeRejectsBeforeTheCommitWhatNoMergedValueDecides)
{
    auto t = R12Counted{{9, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":null}})", t); }).path(), "/f");
    EXPECT_EQ(t.f.x, 9);

    auto c = R12Condition{{0, 0}, 0};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":7},"n":1})", c); }).path(), "/n");
    EXPECT_EQ(c.f.x, 0);
    EXPECT_EQ(c.n, 0);
}

// a combinator of a keyword on JSON and a predicate of a merged value is left to the commit only if the
// predicate can change its result
struct R12AnyOf
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12AnyOf> =
    yyjson::annotate(yyjson::field("f", yyjson::any_of(yyjson::min_properties(3), yyjson::satisfies<r12_small>)));
struct R12AnyOfAll
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12AnyOfAll> = yyjson::annotate(
    yyjson::field("f", yyjson::any_of(yyjson::max_properties(0),
                                      yyjson::all_of(yyjson::min_properties(3), yyjson::satisfies<r12_small>))));
struct R12Not
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12Not> = yyjson::annotate(
    yyjson::field("f", yyjson::not_(yyjson::all_of(yyjson::min_properties(1), yyjson::satisfies<r12_small>))));
struct R12NotJson
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12NotJson> = yyjson::annotate(
    yyjson::field("f", yyjson::not_(yyjson::any_of(yyjson::min_properties(1), yyjson::satisfies<r12_small>))));
struct R12OneOf
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12OneOf> =
    yyjson::annotate(yyjson::field("f", yyjson::one_of(yyjson::min_properties(2), yyjson::satisfies<r12_small>)));
struct R12OneOfJson
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12OneOfJson> = yyjson::annotate(yyjson::field(
    "f", yyjson::one_of(yyjson::min_properties(1), yyjson::max_properties(5), yyjson::satisfies<r12_small>)));

TEST(Constraints, MergeDecidesACombinatorBeforeTheCommitIfItCan)
{
    // the predicate decides: the merge commits, and then rejects the merged value
    auto any = R12AnyOf{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", any); }).path(), "/f");
    EXPECT_EQ(any.f.x, 5);
    yyjson::merge_into(R"({"f":{"x":1}})", any);
    EXPECT_EQ(any.f.x, 1);

    auto one = R12OneOf{{5, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":0}})", one); }).path(), "/f");
    EXPECT_EQ(one.f.x, 0);

    auto negated = R12Not{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", negated); }).path(), "/f");
    EXPECT_EQ(negated.f.x, 1);

    // the keywords on JSON decide whatever the predicate gives: the merge rejects the patch before the commit
    auto all = R12AnyOfAll{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", all); }).path(), "/f");
    EXPECT_EQ(all.f.x, 0);

    auto json = R12NotJson{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", json); }).path(), "/f");
    EXPECT_EQ(json.f.x, 0);

    auto many = R12OneOfJson{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", many); }).path(), "/f");
    EXPECT_EQ(many.f.x, 0);
}

// a pattern that matches a declared field of its value type checks the field without a read of its JSON
struct R12Prefixed
{
    std::string s;
};
inline int r12_prefixed_reads = 0;
template <>
struct yyjson::caster<R12Prefixed>
{
    static R12Prefixed from_json(std::string s)
    {
        ++r12_prefixed_reads;
        if (s.starts_with("!")) throw std::runtime_error("unreadable prefix");
        return {s};
    }
    static std::string to_json(const R12Prefixed& c) { return "!" + c.s; }
};
struct R12Declared
{
    R12Prefixed a;
    std::map<std::string, R12Prefixed> p;
};
template <>
inline constexpr auto yyjson::annotations<R12Declared> = yyjson::annotate(
    yyjson::field("p", yyjson::pattern_properties("^a$"), yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, PatternOfADeclaredFieldTakesItsValue)
{
    r12_prefixed_reads = 0;
    auto t = R12Declared{{"ab"}, {}};
    EXPECT_NO_THROW(yyjson::validate(t));
    yyjson::merge_into(R"({"p":{}})", t);
    EXPECT_EQ(r12_prefixed_reads, 0);
    t.a.s = "abcd";
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/a");
    EXPECT_EQ(r12_prefixed_reads, 0);
}

// a finite value that a caster writes as a float is not read back with WriteFlag::InfAndNanAsNull
struct R12Finite
{
    double x = 5;
};
inline int r12_finite_reads = 0;
template <>
struct yyjson::caster<R12Finite>
{
    static R12Finite from_json(double)
    {
        ++r12_finite_reads;
        throw std::runtime_error("finite caster reread");
    }
    static double to_json(const R12Finite& d) { return d.x; }
};
struct R12Finites
{
    std::map<std::string, R12Finite> m;
};
template <>
inline constexpr auto yyjson::annotations<R12Finites> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));

TEST(Constraints, InfAndNanAsNullReadsNoFiniteCasterValue)
{
    r12_finite_reads = 0;
    const auto t = R12Finites{{{"k", R12Finite{5}}}};
    EXPECT_EQ(std::string_view(yyjson::write(t, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation)),
              R"({"k":5.0})");
    EXPECT_EQ(r12_finite_reads, 0);
}

// a validate that a predicate calls during the check of a merge checks its own value
struct R13Part
{
    int x = 0;
};
inline bool r13_positive(const R13Part& f) { return f.x > 0; }
struct R13Check
{
    std::shared_ptr<R13Part> f;
};
template <>
inline constexpr auto yyjson::annotations<R13Check> =
    yyjson::annotate(yyjson::field("f", yyjson::satisfies<r13_positive>));
struct R13Gated
{
    R13Part f;
    int gate = 0;
};
inline R13Gated* r13_target = nullptr;
inline bool r13_rejects(int)
{
    try
    {
        yyjson::validate(R13Check{std::shared_ptr<R13Part>(&r13_target->f, [](R13Part*) {})});
        return false;
    }
    catch (const yyjson::bad_cast&)
    {
        return true;
    }
}
template <>
inline constexpr auto yyjson::annotations<R13Gated> = yyjson::annotate(
    yyjson::field("f", yyjson::satisfies<r13_positive>), yyjson::field("gate", yyjson::satisfies<r13_rejects>));

TEST(Constraints, MergeKeepsANestedValidateIndependent)
{
    auto t = R13Gated{};
    r13_target = &t;
    EXPECT_NO_THROW((void)yyjson::read<R13Gated>(R"({"f":{"x":1},"gate":0})"));
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(t.f.x, 1);
}

// a merge calls no predicate of a branch that read does not select
inline bool r13_explode(int) { throw std::runtime_error("inactive branch"); }
struct R13Branch
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13Branch> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r13_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("x", yyjson::satisfies<r13_explode>)))));
struct R13AnyOf
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13AnyOf> = yyjson::annotate(yyjson::field(
    "f", yyjson::any_of(yyjson::satisfies<r13_positive>, yyjson::property("x", yyjson::satisfies<r13_explode>))));
struct R13Both
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13Both> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r13_positive>), yyjson::then(yyjson::max_properties(0)),
                              yyjson::else_(yyjson::max_properties(0)))));

TEST(Constraints, MergeCallsNoPredicateOfAnInactiveBranch)
{
    EXPECT_NO_THROW((void)yyjson::read<R13Branch>(R"({"f":{"x":1}})"));
    auto branch = R13Branch{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", branch));
    EXPECT_EQ(branch.f.x, 1);
    auto any = R13AnyOf{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", any));
    EXPECT_EQ(any.f.x, 1);

    // both branches fail: the message does not tell which way the condition went
    auto both = R13Both{{-1}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":-2}})", both); });
    EXPECT_EQ(e.path(), "/f");
    EXPECT_EQ(std::string_view(e.what()).find("if_"), std::string_view::npos);
    EXPECT_EQ(both.f.x, -1);
}

// a pattern binds the C++ value of a declared field through a wrapper or a variant
struct R13Wrap
{
    R12Prefixed value;
};
template <>
inline constexpr auto yyjson::annotations<R13Wrap> = yyjson::annotate(yyjson::transparent);
template <typename A>
struct R13Patterned
{
    A a;
    std::map<std::string, R12Prefixed> p;
};
template <typename A>
inline constexpr auto yyjson::annotations<R13Patterned<A>> = yyjson::annotate(
    yyjson::field("p", yyjson::pattern_properties("^a$"), yyjson::additional_properties(yyjson::max_length(4))));
using R13Adjacent = std::variant<R12Prefixed, std::string>;
struct R13AdjacentField
{
    R13Adjacent v;
    std::map<std::string, R13Adjacent> p;
};
template <>
inline constexpr auto yyjson::annotations<R13AdjacentField> = yyjson::annotate(
    yyjson::field("v", yyjson::adjacent_tag("t", {"c", "s"})),
    yyjson::field("p", yyjson::pattern_properties("^v$"), yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, PatternOfAWrappedFieldTakesItsValue)
{
    r12_prefixed_reads = 0;
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<std::optional<R12Prefixed>>{R12Prefixed{"ab"}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<R13Wrap>{R13Wrap{{"ab"}}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<std::variant<R12Prefixed, int>>{R12Prefixed{"ab"}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13AdjacentField{R12Prefixed{"ab"}, {}}));
    auto t = R13Patterned<std::optional<R12Prefixed>>{R12Prefixed{"ab"}, {}};
    yyjson::merge_into(R"({"p":{}})", t);
    EXPECT_EQ(r12_prefixed_reads, 0);
    t.a = R12Prefixed{"abcd"};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/a");
    EXPECT_EQ(r12_prefixed_reads, 0);
}

// a NaN that write gives as null is checked as the JSON of the caster, without its from_json
struct R13Real
{
    double x = 0;
};
inline int r13_real_reads = 0;
template <>
struct yyjson::caster<R13Real>
{
    static R13Real from_json(std::optional<double> x)
    {
        ++r13_real_reads;
        return {x.value_or(0)};
    }
    static std::optional<double> to_json(const R13Real& c) { return c.x; }
};
struct R13Reals
{
    std::map<std::string, R13Real> m;
};
template <>
inline constexpr auto yyjson::annotations<R13Reals> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));
struct R13Positions
{
    R12Finite a;
    std::vector<R13Real> rest;
};
template <>
inline constexpr auto yyjson::annotations<R13Positions> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::max_items(5));

TEST(Constraints, InfAndNanAsNullReadsNoCasterValue)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    r13_real_reads = 0;
    r12_finite_reads = 0;
    EXPECT_EQ(std::string_view(yyjson::write(R13Reals{{{"k", R13Real{nan}}}}, as_null, yyjson::with_validation)),
              R"({"k":null})");
    EXPECT_EQ(std::string_view(yyjson::write(R13Real{nan}, as_null, yyjson::with_validation)), "null");
    EXPECT_EQ(
        std::string_view(yyjson::write(R13Positions{R12Finite{1}, {R13Real{nan}}}, as_null, yyjson::with_validation)),
        "[1.0,null]");
    // a `double` that `from_json` takes cannot read null
    const auto e =
        bad_cast_of([&] { (void)yyjson::write(R13Positions{R12Finite{nan}, {}}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected a number, got null at /0; reading double");
    EXPECT_EQ(r13_real_reads, 0);
    EXPECT_EQ(r12_finite_reads, 0);
}

// a validate inside a format of a merge checks its value on its own
struct R14Text
{
    std::string text;
};
inline bool r14_has_s(const std::string& s) { return s.find('S') != std::string::npos; }
template <>
inline constexpr auto yyjson::annotations<R14Text> =
    yyjson::annotate(yyjson::field("text", yyjson::satisfies<r14_has_s>));
bool r14_valid_text(std::string_view s)
{
    try
    {
        yyjson::validate(R14Text{std::string(s)});
        return true;
    }
    catch (const yyjson::bad_cast&)
    {
        return false;
    }
}
struct R14Text2
{
    int x = 0;
    std::string s = "s";
};
inline bool r14_positive(const R14Text2& f) { return f.x > 0; }
struct R14Nested
{
    R14Text2 f;
};
template <>
inline constexpr auto yyjson::annotations<R14Nested> = yyjson::annotate(
    yyjson::field("f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r14_positive>),
                                            yyjson::then(yyjson::property("s", yyjson::format_assertion("valid-text"))),
                                            yyjson::else_(yyjson::max_properties(0)))));

TEST(Constraints, MergeRejectsBeforeANestedValidate)
{
    EXPECT_THROW((void)yyjson::read<R14Nested>(R"({"f":{"x":1,"s":"s"}})"), yyjson::bad_cast);
    auto t = R14Nested{};
    EXPECT_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t), yyjson::bad_cast);
    EXPECT_EQ(t.f.x, 0);
}

// a merge calls no predicate after a property of the struct position that fails
struct R14Inner
{
    int x = 0;
};
inline bool r14_inner_positive(const R14Inner& a) { return a.x > 0; }
inline int r14_later_calls = 0;
inline bool r14_later(int)
{
    ++r14_later_calls;
    throw yyjson::bad_cast("later callback");
}
struct R14Pair
{
    R14Inner a;
    int b = 0;
};
struct R14Chain
{
    R14Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R14Chain> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::property("a", yyjson::satisfies<r14_inner_positive>),
                                          yyjson::property("b", yyjson::satisfies<r14_later>)),
                              yyjson::then(yyjson::min_properties(1)), yyjson::else_(yyjson::min_properties(1)))));

TEST(Constraints, MergeCallsNoPredicateAfterAFailedProperty)
{
    r14_later_calls = 0;
    EXPECT_NO_THROW((void)yyjson::read<R14Chain>(R"({"f":{"a":{"x":-1},"b":0}})"));
    auto t = R14Chain{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"a":{"x":-1}}})", t));
    EXPECT_EQ(r14_later_calls, 0);
}

// InfAndNanAsNull reads no caster value that a variant or a map holds
struct R14Key
{
    std::string s;
    explicit R14Key(std::string_view x) : s("!" + std::string(x)) {}
    operator std::string_view() const { return s; }  // NOLINT(google-explicit-constructor)
    bool operator<(const R14Key& k) const { return s < k.s; }
};
struct R14Keyed
{
    std::map<R14Key, R13Real> m;
};
template <>
inline constexpr auto yyjson::annotations<R14Keyed> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));
struct R14Int
{
    static double from_json(int n) { return n; }
};
struct R14Source
{
    double a = 0;
};
struct R14Written
{
    std::optional<double> a;
};
struct R14Translated
{
    double v = 0;
};
template <>
struct yyjson::caster<R14Translated>
{
    static R14Translated from_json(R14Source s) { return {s.a}; }
    static R14Written to_json(const R14Translated& t) { return {t.v}; }
};

struct R14Accepting
{
    double v = 0;
};
template <>
struct yyjson::caster<R14Accepting>
{
    static R14Accepting from_json(R14Written w) { return {w.a.value_or(0)}; }
    static R14Source to_json(const R14Accepting& t) { return {t.v}; }
};

TEST(Constraints, InfAndNanAsNullReadsTheHeldCasterValue)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    r13_real_reads = 0;
    EXPECT_EQ(std::string_view(
                  yyjson::write(std::variant<R13Real, std::string>{R13Real{nan}}, as_null, yyjson::with_validation)),
              "null");
    EXPECT_EQ(std::string_view(yyjson::write(R14Keyed{{{R14Key("k"), R13Real{5}}}}, as_null, yyjson::with_validation)),
              R"({"!k":5.0})");
    EXPECT_EQ(
        std::string_view(yyjson::write(R14Keyed{{{R14Key("k"), R13Real{nan}}}}, as_null, yyjson::with_validation)),
        R"({"!k":null})");
    EXPECT_EQ(r13_real_reads, 0);
    // null is read as the type that `from_json` takes
    const auto e = bad_cast_of(
        [&] { (void)yyjson::write(yyjson::with_caster<double, R14Int>{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected an integer, got null; reading int");
    const auto f = bad_cast_of([&] { (void)yyjson::write(R14Translated{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(f.what()), "expected a number, got null at /a; reading double");
    // a std::optional field of the type that `from_json` takes rejects null, as read rejects it under the policy
    const auto g = bad_cast_of([&] { (void)yyjson::write(R14Accepting{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(g.what()),
              "the value must not be null at /a; reading std::optional<double> under yyjson::reject_null_as_missing");
}

// the check of a merge follows no field that write leaves out
struct R15Node;
struct R15Link
{
    std::shared_ptr<R15Node> p;
    bool operator==(const R15Link&) const { return true; }
};
template <>
struct yyjson::caster<R15Link>
{
    static const R15Node& to_json(const R15Link& l) { return *l.p; }
};
struct R15Node
{
    double x = 0;
    R15Link next;
};
inline R15Link r15_empty_link() { return {}; }
template <>
inline constexpr auto yyjson::annotations<R15Node> =
    yyjson::annotate(yyjson::field("next", yyjson::default_from<r15_empty_link>));
struct R15Cyclic
{
    std::shared_ptr<R15Node> p;
};
template <>
struct yyjson::caster<R15Cyclic>
{
    static R15Cyclic from_json(std::map<std::string, double>) { return {std::make_shared<R15Node>()}; }
    static const R15Node& to_json(const R15Cyclic& c) { return *c.p; }
};

TEST(Constraints, InfAndNanAsNullFollowsTheWrittenFields)
{
    auto p = std::make_shared<R15Node>();
    p->next.p = p;
    EXPECT_EQ(
        std::string_view(yyjson::write(R15Cyclic{p}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation)),
        R"({"x":0.0})");
    p->next.p.reset();
}

// a validate inside a speculative branch of a merge checks types that only its write reaches
inline bool r15_positive(int x) { return x > 0; }
struct R15Array
{
    int x = -1;
};
template <>
inline constexpr auto yyjson::annotations<R15Array> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::contains(yyjson::satisfies<r15_positive>));
struct R15Limited
{
    int v = 5;
};
template <>
inline constexpr auto yyjson::annotations<R15Limited> = yyjson::annotate(yyjson::field("v", yyjson::maximum(0)));
struct R15WriteOnly
{
    int v = 5;
};
template <>
struct yyjson::caster<R15WriteOnly>
{
    static R15Limited to_json(const R15WriteOnly& w) { return {w.v}; }
};
inline int r15_nested_accepted = 0;
bool r15_nested_checks(std::string_view)
{
    const auto accepts = [](const auto& v) {
        try
        {
            yyjson::validate(v);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    };
    r15_nested_accepted += static_cast<int>(accepts(R15Array{})) + static_cast<int>(accepts(R15WriteOnly{}));
    return true;
}
struct R15Gate
{
    R14Text2 f;
};
template <>
inline constexpr auto yyjson::annotations<R15Gate> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r14_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("s", yyjson::format_assertion("nested-checks"))))));

TEST(Constraints, NestedValidateInASpeculativeBranchChecksAll)
{
    r15_nested_accepted = 0;
    (void)r15_nested_checks("");
    EXPECT_EQ(r15_nested_accepted, 0);
    auto t = R15Gate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r15_nested_accepted, 0);
}

// null is read as the type that read gives `from_json` at the position
struct R15Both
{
    double x = 0;
};
template <>
struct yyjson::caster<R15Both>
{
    static R15Both from_json(std::optional<double> x) { return {x.value_or(0)}; }
    static void from_json(R15Both& c, int x) { c.x = x; }
    static std::optional<double> to_json(const R15Both& c) { return c.x; }
};
struct R15BothField
{
    R15Both b;
};
// the native positions beside a caster in the written value are read back
struct R15Opaque
{
    int x = 0;
};
template <>
struct yyjson::caster<R15Opaque>
{
    static R15Opaque from_json(int x) { return {x}; }
    static int to_json(const R15Opaque& d) { return d.x; }
};
struct R15In
{
    double a = 0;
    R15Opaque b;
};
struct R15Out
{
    double a = 0;
    int b = 0;
};
struct R15Beside
{
    double x = 0;
};
template <>
struct yyjson::caster<R15Beside>
{
    static R15Beside from_json(R15In v) { return {v.a}; }
    static R15Out to_json(const R15Beside& v) { return {v.x, 0}; }
};

TEST(Constraints, InfAndNanAsNullReadsAsTheReadPosition)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    EXPECT_NO_THROW((void)yyjson::read<R15Both>("null"));
    EXPECT_EQ(std::string_view(yyjson::write(R15Both{nan}, as_null, yyjson::with_validation)), "null");
    EXPECT_THROW((void)yyjson::read<R15BothField>(R"({"b":null})"), yyjson::bad_cast);
    const auto e = bad_cast_of([&] { (void)yyjson::write(R15BothField{{nan}}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected an integer, got null at /b; reading int");
    EXPECT_THROW((void)yyjson::read<R15Beside>(R"({"a":null,"b":0})"), yyjson::bad_cast);
    const auto f = bad_cast_of([&] { (void)yyjson::write(R15Beside{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(f.what()), "expected a number, got null at /a; reading double");
}

// a key of a map that read stores in another map is found before a value of the map is read back
struct R15Routed
{
    std::map<std::string, R13Real> first, rest;
};
template <>
inline constexpr auto yyjson::annotations<R15Routed> = yyjson::annotate(
    yyjson::field("first", yyjson::pattern_properties("^x")), yyjson::field("rest", yyjson::extra_keys));

TEST(Constraints, InfAndNanAsNullChecksTheRoutesFirst)
{
    r13_real_reads = 0;
    const auto e = bad_cast_of([] {
        (void)yyjson::write(R15Routed{{}, {{"x", R13Real{5}}}}, yyjson::WriteFlag::InfAndNanAsNull,
                            yyjson::with_validation);
    });
    EXPECT_EQ(
        std::string_view(e.what()),
        R"(the value is not valid at /x; validating R15Routed: a read gives this key of the field "rest" to the field "first")");
    EXPECT_EQ(r13_real_reads, 0);
}

// a merge calls a predicate of a branch that its check cannot select only after the merge
struct R15Pair
{
    int x = 0, y = 0;
};
inline bool r15_pair_positive(const R15Pair& f) { return f.x > 0; }
inline bool r15_never(int) { return false; }
struct R15Late
{
    R15Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R15Late> = yyjson::annotate(
    yyjson::field("f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r15_pair_positive>),
                                            yyjson::then(yyjson::property("y", yyjson::satisfies<r15_never>)),
                                            yyjson::else_(yyjson::max_properties(0)))));

TEST(Constraints, MergeChecksASpeculativePredicateAfterTheMerge)
{
    EXPECT_THROW((void)yyjson::read<R15Late>(R"({"f":{"x":1,"y":0}})"), yyjson::bad_cast);
    auto t = R15Late{};
    EXPECT_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t), yyjson::bad_cast);
    // documented: the predicate runs after the merge, so `t` holds the merged value
    EXPECT_EQ(t.f.x, 1);
}

// a merge checks a field that `from_json` reads in place as read gives it
struct R16InPlace
{
    std::string value;
};
template <>
struct yyjson::caster<R16InPlace>
{
    static R16InPlace from_json(int x) { return {std::to_string(x)}; }
    static void from_json(R16InPlace& c, std::string x) { c.value = std::move(x); }
    static std::string to_json(const R16InPlace& c) { return c.value; }
};
struct R16InPlaceField
{
    int n = 0;
    R16InPlace c{"old"};
};
template <>
inline constexpr auto yyjson::annotations<R16InPlaceField> =
    yyjson::annotate(yyjson::field("c", yyjson::max_length(3)));

TEST(Constraints, MergeChecksAFieldReadInPlace)
{
    const auto* json = R"({"n":1,"c":"new"})";
    EXPECT_NO_THROW((void)yyjson::read<R16InPlaceField>(json));
    auto t = R16InPlaceField{};
    EXPECT_NO_THROW(yyjson::merge_into(json, t));
    EXPECT_EQ(t.n, 1);
    EXPECT_EQ(t.c.value, "new");
}

// InfAndNanAsNull reads the written JSON as the type that read gives `from_json` of the caster
namespace r16
{
    constexpr auto inf = std::numeric_limits<double>::infinity();
    struct D
    {
        double x = inf;
    };
    struct Transparent
    {
        D d;
    };
    struct W
    {
        double x = 0;
    };
    struct Translated
    {
        double x = inf;
    };
    struct Row
    {
        double x = inf;
    };
    struct ToRow
    {
        double x = inf;
    };
    struct Values
    {
        std::map<std::string, double> values;
    };
    struct ToValues
    {
        double x = inf;
    };
    struct Opaque
    {
        int x = 0;
    };
    struct ArrayIn
    {
        double a[1]{};
        Opaque d;
    };
    struct ArrayOut
    {
        std::array<double, 1> a;
        int d = 0;
    };
    struct ToArray
    {
        double x = inf;
    };
    struct MapIn
    {
        double a = 0;
        Opaque d;
        std::map<std::string, double> rest;
    };
    struct PairOut
    {
        double a = 0;
        int d = 0;
    };
    struct ToMap
    {
        double x = inf;
    };
    struct RowIn
    {
        double a = 0;
        Opaque d;
    };
    struct RowOut
    {
        double a = 0;
        int d = 0;
    };
    struct ToRowIn
    {
        double x = inf;
    };
    inline int outer = 0;
    inline int inner = 0;
    struct Counted
    {
        double x = 0;
    };
    struct CountedTransparent
    {
        Counted d;
    };
    struct ToCounted
    {
        double x = inf;
    };
    struct ExtraIn
    {
        yyjson::nullable<double> a;
        std::map<std::string, Opaque> rest;
    };
    struct ToExtra
    {
        double x = inf;
    };
}  // namespace r16
template <>
struct yyjson::caster<r16::D>
{
    static r16::D from_json(int x) { return {double(x)}; }
    static void from_json(r16::D& d, std::optional<double> x) { d.x = x.value_or(0); }
    static std::optional<double> to_json(const r16::D& d) { return d.x; }
};
template <>
inline constexpr auto yyjson::annotations<r16::Transparent> = yyjson::annotate(yyjson::transparent);
template <>
struct yyjson::caster<r16::Translated>
{
    static r16::Translated from_json(r16::W w) { return {w.x}; }
    static void from_json(r16::Translated& c, int x) { c.x = x; }
    static r16::W to_json(const r16::Translated& c) { return {c.x}; }
};
template <>
inline constexpr auto yyjson::annotations<r16::Row> =
    yyjson::annotate(yyjson::as_array, yyjson::field("x", yyjson::default_));
template <>
struct yyjson::caster<r16::ToRow>
{
    static r16::ToRow from_json(std::vector<double> x) { return {x.at(0)}; }
    static r16::Row to_json(const r16::ToRow& c) { return {c.x}; }
};
template <>
inline constexpr auto yyjson::annotations<r16::Values> = yyjson::annotate(yyjson::field("values", yyjson::extra_keys));
template <>
struct yyjson::caster<r16::ToValues>
{
    static r16::ToValues from_json(std::map<std::string, double> x) { return {x.at("x")}; }
    static r16::Values to_json(const r16::ToValues& c) { return {{{"x", c.x}}}; }
};
template <>
struct yyjson::caster<r16::Opaque>
{
    static r16::Opaque from_json(int x) { return {x}; }
    static int to_json(const r16::Opaque& d) { return d.x; }
};
template <>
struct yyjson::caster<r16::ToArray>
{
    static r16::ToArray from_json(r16::ArrayIn) { return {}; }
    static r16::ArrayOut to_json(const r16::ToArray& c) { return {{c.x}, 0}; }
};
template <>
inline constexpr auto yyjson::annotations<r16::MapIn> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
template <>
struct yyjson::caster<r16::ToMap>
{
    static r16::ToMap from_json(r16::MapIn) { return {}; }
    static r16::PairOut to_json(const r16::ToMap& c) { return {c.x, 0}; }
};
template <>
inline constexpr auto yyjson::annotations<r16::RowIn> = yyjson::annotate(yyjson::as_array);
template <>
inline constexpr auto yyjson::annotations<r16::RowOut> = yyjson::annotate(yyjson::as_array);
template <>
struct yyjson::caster<r16::ToRowIn>
{
    static r16::ToRowIn from_json(r16::RowIn) { return {}; }
    static r16::RowOut to_json(const r16::ToRowIn& c) { return {c.x, 0}; }
};
template <>
struct yyjson::caster<r16::Counted>
{
    static r16::Counted from_json(std::optional<double> x)
    {
        ++r16::inner;
        return {x.value_or(0)};
    }
    static double to_json(const r16::Counted& d) { return d.x; }
};
template <>
inline constexpr auto yyjson::annotations<r16::CountedTransparent> = yyjson::annotate(yyjson::transparent);
template <>
struct yyjson::caster<r16::ToCounted>
{
    static r16::ToCounted from_json(r16::CountedTransparent)
    {
        ++r16::outer;
        return {};
    }
    static double to_json(const r16::ToCounted& c) { return c.x; }
};
template <>
inline constexpr auto yyjson::annotations<r16::ExtraIn> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
template <>
struct yyjson::caster<r16::ToExtra>
{
    static r16::ToExtra from_json(r16::ExtraIn)
    {
        ++r16::outer;
        return {};
    }
    static r16::PairOut to_json(const r16::ToExtra& c) { return {c.x, 0}; }
};

namespace
{
    // whether the validated write accepts the value exactly when read accepts its plain write
    template <typename T>
    void expect_read_agrees()
    {
        const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
        const auto text = std::string(std::string_view(yyjson::write(T{}, as_null)));
        bool read_accepts = true;
        try
        {
            (void)yyjson::read<T>(text);
        }
        catch (const std::exception&)
        {
            read_accepts = false;
        }
        bool validate_accepts = true;
        try
        {
            (void)yyjson::write(T{}, as_null, yyjson::with_validation);
        }
        catch (const std::exception&)
        {
            validate_accepts = false;
        }
        EXPECT_EQ(validate_accepts, read_accepts) << text;
    }
}  // namespace

TEST(Constraints, InfAndNanAsNullReadsTheSourceOfTheCaster)
{
    expect_read_agrees<r16::Transparent>();
    expect_read_agrees<r16::Translated>();
    expect_read_agrees<r16::ToRow>();
    expect_read_agrees<r16::ToValues>();
    expect_read_agrees<r16::ToArray>();
    expect_read_agrees<r16::ToMap>();
    expect_read_agrees<r16::ToRowIn>();
    EXPECT_THROW((void)yyjson::write(r16::ToArray{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(r16::ToMap{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(r16::ToRowIn{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
}

TEST(Constraints, InfAndNanAsNullCallsOnlyTheNestedCasters)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    r16::outer = 0;
    EXPECT_NO_THROW((void)yyjson::write(r16::ToCounted{}, as_null, yyjson::with_validation));
    EXPECT_EQ(r16::outer, 0);
    expect_read_agrees<r16::ToExtra>();
    r16::outer = 0;
    EXPECT_NO_THROW((void)yyjson::write(r16::ToExtra{}, as_null, yyjson::with_validation));
    EXPECT_EQ(r16::outer, 0);
}

// a merge does not call the predicate of a field of `yyjson::with_caster` in a branch that it cannot select
struct R16Unread
{
    static int from_json(int x) { return x; }
};
inline int r16_invoked_calls = 0;
inline bool r16_invoked(int)
{
    ++r16_invoked_calls;
    return true;
}
struct R16WithCaster
{
    int x = 0;
    yyjson::with_caster<int, R16Unread> y{1};
};
inline bool r16_with_caster_positive(const R16WithCaster& f) { return f.x > 0; }
struct R16WithCasterGate
{
    R16WithCaster f;
};
template <>
inline constexpr auto yyjson::annotations<R16WithCasterGate> = yyjson::annotate(
    yyjson::field("f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r16_with_caster_positive>),
                                            yyjson::then(yyjson::min_properties(1)),
                                            yyjson::else_(yyjson::property("y", yyjson::satisfies<r16_invoked>)))));

TEST(Constraints, MergeDefersThePredicateOfAWithCaster)
{
    r16_invoked_calls = 0;
    EXPECT_NO_THROW((void)yyjson::read<R16WithCasterGate>(R"({"f":{"x":1,"y":1}})"));
    EXPECT_EQ(r16_invoked_calls, 0);
    auto t = R16WithCasterGate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r16_invoked_calls, 0);
}

// a nested read in a speculative branch checks the keywords of the type that it reads with an opaque write
inline bool r16_positive(int x) { return x > 0; }
struct R16Entry
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<R16Entry> =
    yyjson::annotate(yyjson::field("x", yyjson::satisfies<r16_positive>));
struct R16ReadOnly
{
    std::map<std::string, R16Entry> rest;
};
template <>
inline constexpr auto yyjson::annotations<R16ReadOnly> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
template <>
struct yyjson::caster<R16ReadOnly>
{
    static int to_json(const R16ReadOnly&) { return 0; }
};
inline int r16_nested_accepted = 0;
bool r16_nested_reads(std::string_view)
{
    try
    {
        (void)yyjson::read<R16ReadOnly>(R"({"k":{"x":-1}})");
        ++r16_nested_accepted;
    }
    catch (const yyjson::bad_cast&)
    {
    }
    return true;
}
struct R16ReadGate
{
    R14Text2 f;
};
template <>
inline constexpr auto yyjson::annotations<R16ReadGate> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r14_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("s", yyjson::format_assertion("nested-reads"))))));

TEST(Constraints, NestedReadInASpeculativeBranchChecksAll)
{
    EXPECT_TRUE(yyjson::detail::speculation_sensitive<R16ReadOnly>::value);
    r16_nested_accepted = 0;
    (void)r16_nested_reads("");
    EXPECT_EQ(r16_nested_accepted, 0);
    auto t = R16ReadGate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r16_nested_accepted, 0);
}

TEST(Constraints, RepeatedKeysNeedNoSpeculationState)
{
    EXPECT_TRUE((std::is_empty_v<yyjson::detail::independent_checks_for<std::multimap<std::string, int>>>));
}

// a present value of an in-place caster field with `yyjson::default_from` starts from the field of `T()`, as read
struct R17InPlace
{
    int x = 10;
    bool operator==(const R17InPlace&) const = default;
};
template <>
struct yyjson::caster<R17InPlace>
{
    static R17InPlace from_json(int x) { return {x + 100}; }
    static void from_json(R17InPlace& c, int x) { c.x += x; }
    static int to_json(const R17InPlace& c) { return c.x; }
};
inline R17InPlace r17_twenty() { return {20}; }
inline bool r17_is21(const R17InPlace& c) { return c.x == 21; }
struct R17InPlaceField
{
    R17InPlace c;
};
template <>
inline constexpr auto yyjson::annotations<R17InPlaceField> =
    yyjson::annotate(yyjson::field("c", yyjson::default_from<r17_twenty>, yyjson::satisfies<r17_is21>));
struct R17InPlaceProperty
{
    R17InPlace c;
};
template <>
inline constexpr auto yyjson::annotations<R17InPlaceProperty> = yyjson::annotate(
    yyjson::field("c", yyjson::default_from<r17_twenty>), yyjson::property("c", yyjson::satisfies<r17_is21>));

template <typename T>
void expect_in_place_from_object()
{
    EXPECT_ANY_THROW((void)yyjson::read<T>(R"({"c":1})"));
    auto t = T{};
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":1})", t));
    EXPECT_EQ(t.c.x, 10);
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":1})", t, yyjson::strict));
    EXPECT_EQ(t.c.x, 10);
    EXPECT_EQ(yyjson::read<T>(R"({"c":11})").c.x, 21);
    t.c.x = 5;
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":11})", t));
    EXPECT_EQ(t.c.x, 21);
    t.c.x = 5;
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":11})", t, yyjson::strict));
    EXPECT_EQ(t.c.x, 21);
}

TEST(Constraints, MergeReadsAPresentInPlaceFieldFromTheObject)
{
    expect_in_place_from_object<R17InPlaceField>();
    expect_in_place_from_object<R17InPlaceProperty>();
}

// a transparent struct with its own caster is read by that caster, also at a field
struct R17Transparent
{
    int x = 10;
};
template <>
inline constexpr auto yyjson::annotations<R17Transparent> = yyjson::annotate(yyjson::transparent);
template <>
struct yyjson::caster<R17Transparent>
{
    static R17Transparent from_json(int x) { return {x + 100}; }
    static void from_json(R17Transparent& c, int x) { c.x += x; }
    static int to_json(const R17Transparent& c) { return c.x; }
};
inline bool r17_eleven(const R17Transparent& c) { return c.x == 11; }
struct R17TransparentField
{
    R17Transparent c;
};
template <>
inline constexpr auto yyjson::annotations<R17TransparentField> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<r17_eleven>));

TEST(Constraints, MergeReadsATransparentCasterAsRead)
{
    EXPECT_EQ(yyjson::read<R17TransparentField>(R"({"c":1})").c.x, 11);
    auto t = R17TransparentField{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t));
    EXPECT_EQ(t.c.x, 11);
}

// a new value of a map is an element, which read gives with the returning `from_json`
struct R17Element
{
    std::string value = "old";
};
template <>
struct yyjson::caster<R17Element>
{
    static R17Element from_json(int x) { return {std::to_string(x)}; }
    static void from_json(R17Element& c, std::string x) { c.value = std::move(x); }
    static std::string to_json(const R17Element& c) { return c.value; }
};
struct R17ExtraElements
{
    int n = 0;
    std::map<std::string, R17Element> m;
};
template <>
inline constexpr auto yyjson::annotations<R17ExtraElements> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::max_length(3))));
struct R17PatternElements
{
    int n = 0;
    std::map<std::string, R17Element> m;
};
template <>
inline constexpr auto yyjson::annotations<R17PatternElements> =
    yyjson::annotate(yyjson::field("m", yyjson::pattern_properties("^a")));
struct R17MapElements
{
    std::map<std::string, R17Element> m;
};

TEST(Constraints, MergeInsertsAMapValueAsAnElement)
{
    auto extra = R17ExtraElements{};
    EXPECT_EQ(yyjson::read<R17ExtraElements>(R"({"n":1,"a":5})").m.at("a").value, "5");
    EXPECT_NO_THROW(yyjson::merge_into(R"({"n":1,"a":5})", extra));
    EXPECT_EQ(extra.n, 1);
    EXPECT_EQ(extra.m.at("a").value, "5");
    auto pattern = R17PatternElements{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"n":1,"a":5})", pattern));
    EXPECT_EQ(pattern.m.at("a").value, "5");
    auto map = R17MapElements{};
    EXPECT_EQ(yyjson::read<R17MapElements>(R"({"m":{"a":5}})", yyjson::strict).m.at("a").value, "5");
    EXPECT_NO_THROW(yyjson::merge_into(R"({"m":{"a":5}})", map, yyjson::strict));
    EXPECT_EQ(map.m.at("a").value, "5");
}

// the null that write gives for a caster is read as the type of its `from_json` with the casters in that type
inline int r17_inner_calls = 0;
inline int r17_outer_calls = 0;
struct R17Inner
{
};
template <>
struct yyjson::caster<R17Inner>
{
    static R17Inner from_json(std::optional<double> x)
    {
        ++r17_inner_calls;
        if (!x) throw yyjson::bad_cast("R17Inner rejects null");
        return {};
    }
    static double to_json(const R17Inner&) { return 0; }
};
struct R17Outer
{
    double x = std::numeric_limits<double>::infinity();
};
template <>
struct yyjson::caster<R17Outer>
{
    static R17Outer from_json(R17Inner)
    {
        ++r17_outer_calls;
        return {0};
    }
    static double to_json(const R17Outer& c) { return c.x; }
};

TEST(Constraints, ValidatedNullCallsTheNestedCasters)
{
    EXPECT_ANY_THROW((void)yyjson::read<R17Outer>("null"));
    r17_inner_calls = r17_outer_calls = 0;
    EXPECT_ANY_THROW((void)yyjson::write(R17Outer{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
    EXPECT_GT(r17_inner_calls, 0);
    EXPECT_EQ(r17_outer_calls, 0);
}

// a field that write leaves out is not written for the check of an infinity or a NaN
inline int r17_omitted_calls = 0;
struct R17OmittedWritten
{
    double x = 0;
};
struct R17Omitted
{
    int x = 0;
    bool operator==(const R17Omitted&) const = default;
};
template <>
struct yyjson::caster<R17Omitted>
{
    static R17Omitted from_json(std::map<std::string, double>) { return {}; }
    static R17OmittedWritten to_json(const R17Omitted&)
    {
        ++r17_omitted_calls;
        throw std::runtime_error("omitted caster");
    }
};
struct R17OmittedField
{
    std::optional<R17Omitted> c = R17Omitted{};
};
template <>
inline constexpr auto yyjson::annotations<R17OmittedField> = yyjson::annotate(yyjson::field("c", yyjson::default_));

TEST(Constraints, ValidationWritesNoOmittedField)
{
    r17_omitted_calls = 0;
    EXPECT_NO_THROW(
        (void)yyjson::write(R17OmittedField{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
    EXPECT_EQ(r17_omitted_calls, 0);
}

// a `from_json` that borrows its value from the document is checked with null too
struct R17Borrowed
{
    double x = std::numeric_limits<double>::infinity();
};
template <>
struct yyjson::caster<R17Borrowed>
{
    static R17Borrowed from_json(std::string_view) { return {0}; }
    static double to_json(const R17Borrowed& c) { return c.x; }
};

TEST(Constraints, ValidatedNullReadsABorrowedType)
{
    const auto json = yyjson::write(R17Borrowed{}, yyjson::WriteFlag::InfAndNanAsNull);
    EXPECT_EQ(json, "null");
    EXPECT_ANY_THROW((void)yyjson::read<R17Borrowed>(json));
    EXPECT_ANY_THROW((void)yyjson::write(R17Borrowed{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
}

// the layout flags do not hide the null of an infinity in a `std::optional` field
struct R17OptionalInf
{
    std::optional<double> x = std::numeric_limits<double>::infinity();
};

TEST(Constraints, NullAsMissingCheckIgnoresTheLayout)
{
    for (const auto flag : {
#if YYJSON_VERSION_HEX >= 0x000900
             yyjson::WriteFlag::NewlineAtEnd,
#endif
             yyjson::WriteFlag::Pretty})
    {
        const auto f = yyjson::WriteFlag::InfAndNanAsNull | flag;
        const auto json = yyjson::write(R17OptionalInf{}, f);
        EXPECT_ANY_THROW((void)yyjson::read<R17OptionalInf>(json, yyjson::reject_null_as_missing));
        EXPECT_ANY_THROW((void)yyjson::write(R17OptionalInf{}, f, yyjson::with_validation));
    }
}

// an untagged variant with an alternative that borrows from the JSON is read back as read reads it
struct R18Walked
{
    int x = -1;
};
template <>
inline constexpr auto yyjson::annotations<R18Walked> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
using R18Fallback = yyjson::any_of_variant<R18Walked, std::map<std::string, int>, std::string_view>;
using R18Violating = yyjson::any_of_variant<yyjson::alt<std::string, yyjson::max_length(1)>,
                                            yyjson::alt<std::string_view, yyjson::max_length(1)>>;
using R18Ambiguous = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::min_length(2)>,
                                            yyjson::alt<std::string_view, yyjson::max_length(3)>>;
template <typename V>
struct R18Borrowing
{
};
template <typename V>
struct yyjson::caster<R18Borrowing<V>>
{
    static R18Borrowing<V> from_json(V) { return {}; }
    static V to_json(const R18Borrowing<V>&)
    {
        if constexpr (std::same_as<V, R18Fallback>)
            return V(std::in_place_index<0>, R18Walked{});
        else
            return V(std::in_place_index<0>, "ab");
    }
};

template <typename T>
bool r18_reads_written()
{
    try
    {
        (void)yyjson::read<T>(yyjson::write(T{}));
        return true;
    }
    catch (const yyjson::bad_cast&)
    {
        return false;
    }
}

TEST(Constraints, ValidationReadsABorrowedAlternativeBack)
{
    EXPECT_TRUE(r18_reads_written<R18Borrowing<R18Fallback>>());
    EXPECT_NO_THROW((void)yyjson::write(R18Borrowing<R18Fallback>{}, yyjson::with_validation));
    EXPECT_FALSE(r18_reads_written<R18Borrowing<R18Violating>>());
    EXPECT_ANY_THROW((void)yyjson::write(R18Borrowing<R18Violating>{}, yyjson::with_validation));
    EXPECT_FALSE(r18_reads_written<R18Borrowing<R18Ambiguous>>());
    EXPECT_ANY_THROW((void)yyjson::write(R18Borrowing<R18Ambiguous>{}, yyjson::with_validation));
}

// a `yyjson::nullable` or a `yyjson::indirect` field reads its value in place as read does, from the field of `T()`
inline bool r18_eleven(const R17Transparent& c) { return c.x == 11; }
template <template <typename> typename W>
struct R18WrappedField
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedField<W>> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<r18_eleven>));
template <template <typename> typename W>
struct R18WrappedProperty
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedProperty<W>> =
    yyjson::annotate(yyjson::property("c", yyjson::satisfies<r18_eleven>));
template <template <typename> typename W>
struct R18WrappedMinimum
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedMinimum<W>> =
    yyjson::annotate(yyjson::field("c", yyjson::minimum(0)));

template <typename T>
void expect_wrapped_in_place()
{
    EXPECT_EQ(yyjson::read<T>(R"({"c":1})").c->x, 11);
    auto read = T{};
    read.c = R17Transparent{99};
    yyjson::read_into(R"({"c":1})", read);
    EXPECT_EQ(read.c->x, 11);
    for (const auto strict : {false, true})
    {
        auto t = T{};
        t.c = R17Transparent{99};
        if (strict)
            EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t, yyjson::strict));
        else
            EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t));
        EXPECT_EQ(t.c->x, 11);
    }
}

TEST(Constraints, MergeReadsAWrappedInPlaceCasterAsRead)
{
    expect_wrapped_in_place<R18WrappedField<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedField<yyjson::indirect>>();
    expect_wrapped_in_place<R18WrappedProperty<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedProperty<yyjson::indirect>>();
    expect_wrapped_in_place<R18WrappedMinimum<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedMinimum<yyjson::indirect>>();
    auto t = R18WrappedField<yyjson::nullable>{};
    t.c = R17Transparent{99};
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":2})", t));
    EXPECT_EQ(t.c->x, 99);
}

// a default that borrows from the JSON is a sentinel if the strict read of its written text fails
struct R18BorrowedDefault
{
    std::string_view s = "ok";
    bool operator==(const R18BorrowedDefault&) const = default;
};
template <>
struct yyjson::caster<R18BorrowedDefault>
{
    static int to_json(const R18BorrowedDefault&) { return 0; }
};
struct R18BorrowedDefaultField
{
    R18BorrowedDefault r;
};
template <>
inline constexpr auto yyjson::annotations<R18BorrowedDefaultField> =
    yyjson::annotate(yyjson::field("r", yyjson::default_));
struct R18BorrowedDefaultHolder
{
};
template <>
struct yyjson::caster<R18BorrowedDefaultHolder>
{
    static R18BorrowedDefaultHolder from_json(R18BorrowedDefaultField) { return {}; }
    static R18BorrowedDefaultField to_json(const R18BorrowedDefaultHolder&) { return {}; }
};

TEST(Constraints, BorrowedDefaultIsASentinel)
{
    const auto json = yyjson::write(R18BorrowedDefaultHolder{}, yyjson::emit_defaults);
    EXPECT_EQ(json, "{}");
    EXPECT_NO_THROW((void)yyjson::read<R18BorrowedDefaultHolder>(json));
    EXPECT_NO_THROW((void)yyjson::write(R18BorrowedDefaultHolder{}, yyjson::emit_defaults, yyjson::with_validation));
}

// the values of a range of pairs read from the array form are checked as in the object form
struct PairSetValues
{
    std::vector<std::pair<std::string, std::set<int>>> m = {};
};
template <>
inline constexpr auto yyjson::annotations<PairSetValues> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::min_items(2))));
struct PairVectorValues
{
    std::vector<std::pair<std::string, std::vector<int>>> m = {};
};
template <>
inline constexpr auto yyjson::annotations<PairVectorValues> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::min_items(2))));

template <typename T>
void expect_pair_values()
{
    for (const auto* json : {R"({"m":{"k":[1]}})", R"({"m":[["k",[1]]]})"})
    {
        EXPECT_THROW((void)yyjson::read<T>(json), yyjson::bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).template cast<T>(), yyjson::bad_cast) << json;
    }
    for (const auto* json : {R"({"m":{"k":[1,2]}})", R"({"m":[["k",[1,2]]]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<T>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).template cast<T>()) << json;
    }
    EXPECT_THROW(yyjson::validate(T{{{"k", {1}}}}), yyjson::bad_cast);
    EXPECT_NO_THROW(yyjson::validate(T{{{"k", {1, 2}}}}));
}

TEST(Constraints, PairValuesFromTheArrayForm)
{
    expect_pair_values<PairSetValues>();
    expect_pair_values<PairVectorValues>();
    try
    {
        (void)yyjson::read<PairSetValues>(R"({"m":[["k",[1]]]})");
        FAIL();
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(e.path(), "/m");
    }
}

// NOLINTEND
