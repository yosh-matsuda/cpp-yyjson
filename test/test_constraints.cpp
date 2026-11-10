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
    static_assert(table_error(additional_properties(minimum(0))) == struct_additional_properties);
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
    static_assert(table_error(field("w", extra_keys), additional_properties(minimum(0))) ==
                  struct_additional_properties);
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

// the keywords of a field check the JSON value that a caster reads, but not its elements and members, and a
// subschema of a combinator is a position of the type
struct CasterItems
{
    CasterList v;
};
template <>
inline constexpr auto yyjson::annotations<CasterItems> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::maximum(1))));
namespace caster_json_rules
{
    using yyjson::detail::caster_json_keywords_fit, yyjson::detail::checks_caster_json, yyjson::detail::field_schema;
    static_assert(checks_caster_json<field_schema<CasterFields, 0>, CasterName>() &&
                  checks_caster_json<field_schema<CasterFields, 6>, CasterList>() &&
                  checks_caster_json<field_schema<CasterFields, 9>, std::optional<CasterName>>());
    static_assert(caster_json_keywords_fit<field_schema<CasterFields, 6>>() &&
                  !caster_json_keywords_fit<field_schema<CasterItems, 0>>());
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
// a field that the write omits as equal to its default, which a merge reads from the patch as an absent value
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
// the field of `yyjson::extra_keys` has no key, so that the settings cannot name it
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
// nor a field of `yyjson::pattern_properties`
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
    static_assert(presence_error<PresenceExtra> == unknown_key && presence_error<ConditionExtra> == unknown_key);
    static_assert(presence_error<PresencePattern> == unknown_key && presence_error<ConditionPattern> == unknown_key);
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
        auto merged = T();
        yyjson::merge_into(json, merged);
        EXPECT_EQ(merged, expected) << json;
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
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path, bool merged = true)
    {
        merged = merged && writes_no_member<T>();
        static_cast<void>(expect_merge_rule(T(), json));
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

    // validate and the checked write check them as well
    expect_valid(Patterns{"abz", "oxo", "123"});
    expect_invalid(Patterns{"ab", {}, {}}, value("s") + R"("^a", "z$")", "/s");
    expect_invalid(Patterns{"az", "oxx", {}}, value("o") + R"("^o", "o$", "x")", "/o");
    expect_invalid(Patterns{"az", "o", {}},
                   R"(the value of the key "o" of Patterns is shorter than the minimum length 2)", "/o");
    expect_invalid(Patterns{"az", {}, "1a"},
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

TEST(Constraints, RawNumbers)
{
    const auto raw = [](std::string_view json) {
        return yyjson::read<RawNumbers>(json, yyjson::ReadFlag::NumberAsRaw);
    };
    const auto expect_raw_rejected = [&](std::string_view json, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        EXPECT_TRUE(message_of(e).starts_with(
            std::format(R"(the value of the key "{}" of RawNumbers {})", path.substr(1), message)))
            << json << "\n"
            << e.what();
        EXPECT_EQ(e.path(), path) << json;
    };
    EXPECT_EQ(*raw(R"({"n":5,"big":9007199254740993,"half":1.5,"flag":true})").n.as_raw(), "5");
    expect_raw_rejected(R"({"n":-1})", "is less than the minimum 0", "/n");
    expect_raw_rejected(R"({"n":1e400})", "is greater than the maximum 100", "/n");
    // an integer that fits 64 bits compares exactly, which a double does not
    expect_raw_rejected(R"({"big":9007199254740992})", "is less than the minimum 9007199254740993", "/big");
    expect_raw_rejected(R"({"half":1.25})", "is not a multiple of 0.5", "/half");
    // a raw number is a number, which yyjson::matches<bool> does not read
    expect_raw_rejected(R"({"flag":1})", "is not read as bool by yyjson::matches", "/flag");
}

TEST(Constraints, RawEqualities)
{
    const auto raw = [](std::string_view json) { return yyjson::read<RawEquals>(json, yyjson::ReadFlag::NumberAsRaw); };
    const auto expect_raw_rejected = [&](std::string_view json, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
        EXPECT_EQ(e.path(), path) << json;
    };
    // a raw number equals the number that its text is without the flag
    EXPECT_NO_THROW((void)raw(R"({"one":1,"listed":2.5,"deep":{"a":[1.0,2.50]},"held":{"v":3.0}})"));
    EXPECT_NO_THROW((void)raw(R"({"one":1.0,"listed":"a","unique":[1,"1",2],"items":[[1],[2]]})"));
    // a number that overflows a double is an infinity
    expect_raw_rejected(R"({"unique":[1e400,2e400]})", "an element of the value", "/unique/1");
    expect_raw_rejected(R"({"one":2})", R"(the value of the key "one" of RawEquals is not equal to the constant 1)",
                        "/one");
    expect_raw_rejected(R"({"listed":2})", R"(the value of the key "listed" of RawEquals is not one of)", "/listed");
    expect_raw_rejected(R"({"deep":{"a":[1,2.4]}})", R"(the value of the key "deep" of RawEquals is not equal to)",
                        "/deep");
    expect_raw_rejected(R"({"held":{"v":4}})", R"(the value of the key "held" of RawEquals is not equal to)", "/held");
    // a duplicate is reported at the later element, as for a std::vector
    const auto duplicate = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of RawEquals is equal to an earlier element)",
                           key);
    };
    expect_raw_rejected(R"({"unique":[1,1.0]})", duplicate("unique"), "/unique/1");
    expect_raw_rejected(R"({"items":[[2],[1],[1e0]]})", duplicate("items"), "/items/2");
    expect_rejected<RawEquals>(R"({"unique":[0,{"a":1},{"a":1.0}]})", duplicate("unique"), "/unique/2");

    // a raw number equals a number that is not raw, and the public operator== compares the raw text
    const auto doc = yyjson::read(R"([1,1.0])", yyjson::ReadFlag::NumberAsRaw);
    EXPECT_NE((*doc.as_array())[0], (*doc.as_array())[1]);
    EXPECT_NE((*doc.as_array())[0], yyjson::read("1"));
    auto mixed = yyjson::object(yyjson::read(R"({"unique":[1]})", yyjson::ReadFlag::NumberAsRaw));
    (*mixed["unique"].as_array()).emplace_back(1.0);
    const auto e = bad_cast_of([&] { (void)yyjson::value(mixed).cast<RawEquals>(); });
    EXPECT_TRUE(message_of(e).starts_with(duplicate("unique"))) << e.what();
    EXPECT_EQ(e.path(), "/unique/1");
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, RawExtendedNumbers)
{
    // a raw number in the syntax of JSON5 compares as the number that JSON5 reads
    for (const auto flag : {yyjson::ReadFlag::Json5, yyjson::ReadFlag::Json5 | yyjson::ReadFlag::NumberAsRaw})
    {
        const auto read = [flag](std::string_view json) { return yyjson::read<RawExtended>(json, flag); };
        const auto expect_raw_rejected = [&](std::string_view json, std::string_view message, std::string_view path) {
            const auto e = bad_cast_of([&] { (void)read(json); });
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        EXPECT_NO_THROW((void)read(R"({"n":+5,"c":0x10,"u":[0x10,15],"a":[16.,+15]})"));
        EXPECT_NO_THROW((void)read(R"({"c":16.})"));
        expect_raw_rejected(R"({"n":0x10})", R"(the value of the key "n" of RawExtended is greater than the maximum 5)",
                            "/n");
        expect_raw_rejected(R"({"n":+10})", R"(the value of the key "n" of RawExtended is greater than)", "/n");
        expect_raw_rejected(R"({"c":0x11})", R"(the value of the key "c" of RawExtended is not equal to)", "/c");
        expect_raw_rejected(R"({"u":[0x10,16]})", R"(an element of the value of the key "u" of RawExtended is equal)",
                            "/u/1");
        expect_raw_rejected(R"({"a":[0x10,16.]})", R"(an element of the value of the key "a" of RawExtended is equal)",
                            "/a/1");
    }
}
#endif

TEST(Constraints, TextUnderStrict)
{
    // the policies of the read that first compares the constant do not change it
    EXPECT_EQ(yyjson::read<WideText>(R"({"at":{"x":1,"y":2}})", yyjson::reject_unknown_keys).at, (Spot{1, 2}));
    EXPECT_EQ(yyjson::read<WideText>(R"({"at":{"x":1,"y":2}})").at, (Spot{1, 2}));
    expect_rejected<WideText>(R"({"at":{"x":1,"y":3}})", R"(the value of the key "at" of WideText is not equal to)",
                              "/at");
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

    // validate and the checked write compare the text that the write gives
    expect_valid(ListedTexts());
    expect_valid(ListedTexts{.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}});
    expect_valid(ListedTexts{.order = {{"b", 1}, {"a", 2}}});
    expect_invalid(ListedTexts{.at = {2, 2}}, value("at") + R"(is not one of { "y": 0, "x": 0 }, {"x":1,"y":1})",
                   "/at");
    expect_invalid(ListedTexts{.pair = {}}, value("pair") + "is not equal to the constant [1, 2]", "/pair");
    expect_invalid(ListedTexts{.scale = std::vector{2.5}}, value("scale") + "is not one of [1], [2.5, 2.5]", "/scale");
    expect_invalid(ListedTexts{.mixed = 3}, value("mixed") + "is not one of 0, [0], {}", "/mixed");
    expect_invalid(ListedTexts{.order = {{"a", 2}}}, value("order") + R"(is not equal to the constant {"a":2,"b":1})",
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

    const auto prefix = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Elements )", key);
    };
    expect_rejected<Elements>(R"({"scores":[0,-1]})", prefix("scores") + "is less than the minimum 0", "/scores/1");
    expect_rejected<Elements>(R"({"pair":[10,0]})", prefix("pair") + "is greater than the maximum 9", "/pair/0");
    expect_rejected<Elements>(R"({"names":["a","B"]})", prefix("names") + R"(does not match the pattern "^[a-z]+$")",
                              "/names/1");
    // the element of a set or of an unordered set is found in the array
    expect_rejected<Elements>(R"({"set":[1,9,3]})", prefix("set") + "is greater than the maximum 5", "/set/1");
    expect_rejected<Elements>(R"({"uset":[2,4,6,3,8]})", prefix("uset") + "is rejected by yyjson::satisfies",
                              "/uset/3");
    // the values of a map, and the keywords of the map itself
    expect_rejected<Elements>(R"({"limits":{"a":1,"b":101}})", prefix("limits") + "is greater than the maximum 100",
                              "/limits/b");
    expect_rejected<Elements>(R"({"limits":{"a":1,"b":2,"c":3,"d":4}})",
                              R"(the value of the key "limits" of Elements has more properties than the maximum 3)",
                              "/limits");
    expect_rejected<Elements>(R"({"groups":{"g":[1],"h":[]}})", prefix("groups") + "has fewer items than the minimum 1",
                              "/groups/h");
    expect_rejected<Elements>(R"({"groups":{"g":[1,0]}})", prefix("groups") + "is less than the minimum 1",
                              "/groups/g/1");
    // a map whose keys are not strings is an array of pairs, whose values are the second elements
    expect_rejected<Elements>(R"({"by_id":[[1,5],[2,6]]})", prefix("by_id") + "is greater than the maximum 5",
                              "/by_id/1/1");
    expect_rejected<Elements>(R"({"ordered":[[1,2],[4,3]]})", prefix("ordered") + "is rejected by yyjson::satisfies",
                              "/ordered/1");
    // the value of a std::optional element, and the elements of a std::optional field
    expect_rejected<Elements>(R"({"holes":[null,6]})", prefix("holes") + "is greater than the maximum 5", "/holes/1");
    expect_rejected<Elements>(R"({"tags":["abcd"]})", prefix("tags") + "is longer than the maximum length 3",
                              "/tags/0");
    // the elements of the elements
    expect_rejected<Elements>(R"({"nested":[["a"],[]]})", prefix("nested") + "has fewer items than the minimum 1",
                              "/nested/1");
    expect_rejected<Elements>(R"({"nested":[["a"],["b","abcd"]]})",
                              prefix("nested") + "is longer than the maximum length 3", "/nested/1/1");

    expect_accepted<ElementsVisitable>(R"({"v":[1]})", {{1}});
    expect_rejected<ElementsVisitable>(
        R"({"v":[1,0]})",
        R"(an element of the value of the key "v" of ElementsVisitable is not greater than the exclusive minimum 0)",
        "/v/1");

    // the elements of a transparent struct
    EXPECT_EQ(yyjson::read<Scores>("[0,1]"), (Scores{{0, 1}}));
    expect_rejected<Scores>("[]", "the value of Scores has fewer items than the minimum 1", "");
    expect_rejected<Scores>("[0,-1]", "an element of the value of Scores is less than the minimum 0", "/1");
}

TEST(Constraints, ElementNullables)
{
    auto expected = ElementNullables();
    expected.ids = {nullptr, std::make_shared<int>(1)};
    expected.steps = {1, 2};
    expect_accepted<ElementNullables>(R"({"list":null,"ids":[null,1],"steps":[1,2],"counts":null})", expected);
    expected.list = std::make_shared<std::vector<int>>(std::vector{9});
    expected.counts = std::vector{1};
    expect_accepted<ElementNullables>(R"({"list":[9],"ids":[null,1],"steps":[1,2],"counts":[1]})", expected);

    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ElementNullables )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of ElementNullables )", key);
    };
    expect_rejected<ElementNullables>(R"({"list":[]})", field("list") + "has fewer items than the minimum 1", "/list");
    expect_rejected<ElementNullables>(R"({"list":[1,10]})", element("list") + "is greater than the maximum 9",
                                      "/list/1");
    // smart pointers compare the values that they hold, and null is equal to null
    expect_rejected<ElementNullables>(R"({"ids":[1,2,1]})", element("ids") + "is equal to an earlier element",
                                      "/ids/2");
    expect_rejected<ElementNullables>(R"({"ids":[null,1,null]})", element("ids") + "is equal to an earlier element",
                                      "/ids/2");
    // a null element is a value, which yyjson::enum_ without nullptr rejects
    expect_rejected<ElementNullables>(R"({"steps":[1,null]})", element("steps") + "is not one of 1, 2", "/steps/1");
    expect_rejected<ElementNullables>(R"({"counts":[1,2]})", field("counts") + "has more items than the maximum 1",
                                      "/counts");

    // std::unique_ptr elements
    EXPECT_EQ(written(yyjson::read<OwnedElements>(R"({"tags":[null,"abc"]})")), R"({"tags":[null,"abc"]})");
    const auto expect_owned_rejected = [](std::string_view json, std::string_view message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
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
        R"(an element of the value of the key "tags" of OwnedElements is longer than the maximum length 3)", "/tags/1");
    expect_owned_rejected(
        R"({"tags":["a",null,"a"]})",
        R"(an element of the value of the key "tags" of OwnedElements is equal to an earlier element)", "/tags/2");
}

TEST(Constraints, ElementsMerge)
{
    // the keywords on the values check the members of the patch, and the counts count the map after the merge
    auto t = Elements();
    t.limits = {{"old", 200}};
    EXPECT_TRUE(expect_merge_into(t, R"({"limits":{"a":1}})").find(R"("limits":{"a":1,"old":200})") !=
                std::string::npos);
    const auto limits = [](std::string_view text) {
        return std::format(R"(the value of the key "limits" of Elements {})", text);
    };
    const auto expect_merge_rejected = [](Elements value, std::string_view json, std::string_view message,
                                          std::string_view path) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
        EXPECT_EQ(e.path(), path) << json;
    };
    expect_merge_rejected(t, R"({"limits":{"a":101}})",
                          R"(an element of the value of the key "limits" of Elements is greater than the maximum 100)",
                          "/limits/a");
    t.limits = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto direct = false;
    EXPECT_TRUE(
        expect_merge_into(t, R"({"limits":{"a":null,"d":4}})", &direct).find(R"("limits":{"b":2,"c":3,"d":4})") !=
        std::string::npos);
    EXPECT_TRUE(direct);
    for (const auto* json : {R"({"limits":{"d":4}})", R"({"limits":{"a":null,"d":4,"e":5}})"})
        expect_merge_rejected(t, json, limits("has more properties than the maximum 3"), "/limits");
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
    expect_rejected<InputCounted>(R"({"s":[1]})", key("s", "has fewer items than the minimum 2"), "/s");
    expect_rejected<InputCounted>(R"({"s":[1,2,2]})", key("s", "has more items than the maximum 2"), "/s");
    // a merge keeps the last value of a repeated key, and a read the first
    const auto* const repeated = R"({"m":{"a":1,"a":2}})";
    EXPECT_EQ(yyjson::read<InputCounted>(repeated).m.size(), 1U);
    EXPECT_EQ(yyjson::read(repeated).cast<InputCounted>().m.size(), 1U);
    EXPECT_TRUE(expect_direct_read<InputCounted>(repeated));
    EXPECT_TRUE(expect_read_into(InputCounted(), repeated));
    expect_rejected<InputCounted>(R"({"m":{"a":1}})", key("m", "has fewer properties than the minimum 2"), "/m");
    expect_rejected<InputCounted>(R"({"m":{"a":1,"b":2,"a":3}})", key("m", "has more properties than the maximum 2"),
                                  "/m");
    expect_accepted<InputCounted>(R"({"v":[1,1]})", {.v = {1, 1}});
    expect_rejected<InputCounted>(R"({"v":[1]})", key("v", "has fewer items than the minimum 2"), "/v");
    expect_accepted<InputCounted>(R"({"c":[5,5]})", {.c = {5}});
    expect_rejected<InputCounted>(R"({"c":[5,1]})", key("c", "has fewer than 2 items that satisfy"), "/c");

    // `merge_into` counts the members of the patch, and replaces a set with the elements that it counts
    for (const auto* json : {R"({"s":[1,1]})", R"({"m":{"a":1,"a":2}})", R"({"c":[5,5]})"})
    {
        auto direct = false;
        static_cast<void>(expect_merge_into(InputCounted{.m = {{"x", 0}}}, json, &direct));
        EXPECT_TRUE(direct || !reads_directly) << json;
    }
    auto t = InputCounted{.m = {{"x", 0}}};
    yyjson::merge_into(R"({"m":{"a":1,"a":2}})", t);
    EXPECT_EQ(t.m, (std::map<std::string, int>{{"a", 2}, {"x", 0}}));
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"b":1}})", t), yyjson::bad_cast);

    // a condition of the struct counts the elements of the JSON array too
    expect_accepted<CountGate>(R"({"s":[1,1],"k":1})", {{1}, 1});
    expect_accepted<CountGate>(R"({"s":[1]})", {{1}, 0});
    expect_rejected<CountGate>(R"({"s":[1,1]})", R"(CountGate is not constructible from JSON without the key "k")", "");
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
    // a pointer takes the map of the patch, and the other holders keep the members that the patch does not give
    EXPECT_EQ(expect_merge_into(t, R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{}}})"),
              R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{"old":200}}})");
    const auto expect_merge_rejected = [](std::string_view json, std::string_view message, std::string_view path,
                                          HeldLimits value = HeldLimits()) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of HeldLimits )", key);
    };
    for (const auto* key : {"pointer", "held", "boxed"})
    {
        expect_merge_rejected(std::format(R"({{"{}":{{"a":101}}}})", key),
                              element(key) + "is greater than the maximum 100", std::format("/{}/a", key));
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1,"b":2}}}})", key),
                              std::format(R"(the value of the key "{}" of HeldLimits has more properties)", key),
                              std::format("/{}", key));
    }
    expect_merge_rejected(R"({"nested":{"k":{"a":101}}})", element("nested") + "is greater than the maximum 100",
                          "/nested/k/a");
    expect_merge_rejected(R"({"nested":{"k":{"a":1,"b":2}}})", element("nested") + "has more properties", "/nested/k");
    expect_merge_rejected(R"({"nested":{"k":{"a":1},"i":{}}})",
                          R"(the value of the key "nested" of HeldLimits has more properties)", "/nested");
    // the counts count the members that the holders keep
    for (const auto* key : {"held", "boxed"})
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1}}}})", key),
                              std::format(R"(the value of the key "{}" of HeldLimits has more properties)", key),
                              std::format("/{}", key), t);
    expect_merge_rejected(R"({"nested":{"k":{"a":1}}})", element("nested") + "has more properties", "/nested/k", t);
    expect_merge_rejected(R"({"nested":{"i":{}}})",
                          R"(the value of the key "nested" of HeldLimits has more properties)", "/nested", t);
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
    EXPECT_TRUE(expect_merge_into(t, R"({"held":{"a":1,"b":null,"b":1}})")
                    .starts_with(R"(bad cast: the value of the key "held" of HeldLimits has more properties)"));
    const auto sets = RepeatedSets();
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1,2],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_TRUE(expect_merge_into(sets, R"({"m":{"a":null,"a":[1,2]}})")
                    .starts_with(R"(bad cast: an element of the value of the key "m" of RepeatedSets has more items)"));
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":11,"a":null,"b":9}})"), R"({"m":{},"n":{"b":9}})");
    EXPECT_TRUE(expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":11}})")
                    .starts_with(R"(bad cast: an element of the value of the key "n" of RepeatedSets is greater)"));
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":1,"c":1}})"), R"({"m":{},"n":{"b":1,"c":1}})");
    EXPECT_TRUE(expect_merge_into(sets, R"({"n":{"a":1,"b":1,"c":null,"c":1}})")
                    .starts_with(R"(bad cast: the value of the key "n" of RepeatedSets has more properties)"));
    // a patch without a repeated key, through the document too
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1]},"n":{"a":1}})"), R"({"m":{"a":[1]},"n":{"a":1}})");
}

TEST(Constraints, MergeKept)
{
    // yyjson::property_names and yyjson::min_properties check the keys of the patch without null
    auto keyed = Keyed();
    keyed.m = {{"ABC", 1}};
    auto direct = false;
    EXPECT_TRUE(expect_merge_into(keyed, R"({"m":{"ab":1}})", &direct).find(R"("m":{"ABC":1,"ab":1})") !=
                std::string::npos);
    EXPECT_TRUE(direct);
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"ab":null,"Ab":1}})", keyed); });
    EXPECT_TRUE(message_of(e).starts_with(R"(a key of the value of the key "m" of Keyed does not match the pattern)"))
        << e.what();
    EXPECT_EQ(e.path(), "/m/Ab");
    keyed = Keyed();
    keyed.sized = {{"a", 1}};
    EXPECT_TRUE(expect_merge_into(keyed, R"({"sized":{"b":2}})").find(R"("sized":{"a":1,"b":2})") != std::string::npos);
    const auto empty = bad_cast_of([&] { yyjson::merge_into(R"({"sized":{"a":null}})", keyed); });
    EXPECT_TRUE(message_of(empty).starts_with(
        R"(the value of the key "sized" of Keyed has fewer properties than the minimum 1)"))
        << empty.what();
    EXPECT_EQ(empty.path(), "/sized");
}

TEST(Constraints, UniqueItems)
{
    expect_accepted<Unique>(R"({"ints":[1,2],"strs":["a","b"],"reals":[0.5,1],"plain":[{"v":1},{"v":2}],)"
                            R"("opts":[null,1],"nested":[[1],[1,2]]})",
                            {{1, 2}, {"a", "b"}, {0.5, 1.0}, {{1}, {2}}, {std::nullopt, 1}, {{1}, {1, 2}}});

    const auto equal = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Unique is equal to an earlier element)", key);
    };
    // the error is at the second of the equal elements
    expect_rejected<Unique>(R"({"ints":[1,2,1]})", equal("ints"), "/ints/2");
    expect_rejected<Unique>(R"({"reals":[0.0,-0.0]})", equal("reals"), "/reals/1");
    expect_rejected<Unique>(R"({"plain":[{"v":1},{"v":2},{"v":2}]})", equal("plain"), "/plain/2");
    expect_rejected<Unique>(R"({"opts":[null,1,null]})", equal("opts"), "/opts/2");
    expect_rejected<Unique>(R"({"nested":[[1],[1,2,1]]})", equal("nested"), "/nested/1/2");

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
                                      "s9",  "s8",  "s7",  "s6",  "s5",  "s4",  "s3",  "s2",  "s1",  "s0"}});
    expect_rejected<Unique>(strs + R"("s3","s19"]})", equal("strs"), "/strs/20");
    expect_rejected<Unique>(reals + "-0.0]}", equal("reals"), "/reals/20");

    // values of the DOM compare as JSON: numbers by value, objects regardless of the order of the members
    EXPECT_EQ(yyjson::read<UniqueJson>(R"({"any":[1,"1",[1],{"a":1},null,true]})").any.size(), 6);
    expect_rejected<UniqueJson>(
        R"({"any":[{"a":1,"b":[2]},{"b":[2.0],"a":1}]})",
        R"(an element of the value of the key "any" of UniqueJson is equal to an earlier element)", "/any/1");

    // C++ values compare as the JSON that they are written as
    const auto parts = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of UniqueParts is equal to an earlier element)",
                           key);
    };
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"boxes":[{"p":1},{"p":2},{"p":null}],"loose":[{"v":1},{"v":2}],)"
                                        R"("trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[]}],)"
                                        R"("maps":[{"a":1,"b":2},{"a":1}],"twins":[1,2]})")
                  .boxes.size(),
              3);
    expect_rejected<UniqueParts>(R"({"boxes":[{"p":1},{"p":1}]})", parts("boxes"), "/boxes/1");
    expect_rejected<UniqueParts>(R"({"boxes":[{"p":null},{"p":2},{"p":null}]})", parts("boxes"), "/boxes/2");
    expect_rejected<UniqueParts>(R"({"loose":[{"v":1},{"v":1}]})", parts("loose"), "/loose/1");
    expect_rejected<UniqueParts>(R"({"trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[{"v":2,"kids":[]}]}]})",
                                 parts("trees"), "/trees/1");
    expect_rejected<UniqueParts>(R"({"maps":[{"a":1,"b":2,"c":3},{"c":3,"b":2,"a":1}]})", parts("maps"), "/maps/1");
    // a range of key-value pairs is written as an object, whose members have no order
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"a":1,"b":3}]})").pairs.size(), 2);
    expect_rejected<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"b":2,"a":1}]})", parts("pairs"), "/pairs/1");
    // a range of tuples is sorted from 17 elements on
    auto tuples = std::string(R"({"tuples":[)");
    for (auto i = 0; i < 20; ++i) tuples += std::format(R"([{},"{}"],)", i % 2, i);
    EXPECT_EQ(yyjson::read<UniqueParts>(tuples.substr(0, tuples.size() - 1) + "]}").tuples.size(), 20);
    expect_rejected<UniqueParts>(tuples + R"([1,"7"]]})", parts("tuples"), "/tuples/20");
    // the elements of a tuple of one type
    expect_rejected<UniqueParts>(R"({"twins":[3,3]})", parts("twins"), "/twins/1");
}

TEST(Constraints, UniqueReals)
{
    // the reals that JSON writes differently but a double holds as the same value are equal
    expect_rejected<Unusual>(R"({"reals":[9007199254740993,9007199254740992]})",
                             R"(an element of the value of the key "reals" of Unusual is equal to an earlier element)",
                             "/reals/1");
    // NaN is equal to nothing, also among more than 16 elements, which are sorted
    auto nans = std::string(R"({"reals":[)");
    for (auto i = 0; i < 20; ++i) nans += i % 4 == 0 ? "NaN," : std::format("{},", i);
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    const auto read = yyjson::read<Unusual>(nans.substr(0, nans.size() - 1) + "]}", flag);
    EXPECT_EQ(read.reals.size(), 20);
    const auto e = bad_cast_of([&] { (void)yyjson::read<Unusual>(nans + "Infinity,-Infinity,Infinity]}", flag); });
    EXPECT_TRUE(message_of(e).starts_with(
        R"(an element of the value of the key "reals" of Unusual is equal to an earlier element)"))
        << e.what();
    EXPECT_EQ(e.path(), "/reals/22");
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
    expect_rejected<Prefixed>(R"({"row":[-1,"ab",0]})", prefix("row") + "is less than the minimum 0", "/row/0");
    expect_rejected<Prefixed>(R"({"row":[0,"abc",0]})", prefix("row") + "is longer than the maximum length 2",
                              "/row/1");
    expect_rejected<Prefixed>(R"({"row":[0,"AB",0]})", prefix("row") + "is rejected by yyjson::satisfies", "/row/1");
    expect_rejected<Prefixed>(R"({"range":[6,5]})", prefix("range") + "is greater than the maximum 5", "/range/0");
    expect_rejected<Prefixed>(R"({"range":[5,4]})", prefix("range") + "is less than the minimum 5", "/range/1");
    // yyjson::items applies to the elements after the prefix
    expect_rejected<Prefixed>(R"({"head":[-1]})", prefix("head") + "is less than the minimum 0", "/head/0");
    expect_rejected<Prefixed>(R"({"head":[0,10,10]})", prefix("head") + "is greater than the maximum 9", "/head/2");
    expect_rejected<Prefixed>(R"({"arr":[0,0,0]})", prefix("arr") + "is less than the minimum 1", "/arr/1");
    expect_rejected<Prefixed>(R"({"grid":[[0]]})", prefix("grid") + "is less than the minimum 1", "/grid/0/0");
    expect_rejected<Prefixed>(R"({"grid":[[1],[0]]})", prefix("grid") + "has fewer items than the minimum 2",
                              "/grid/1");
    expect_rejected<Prefixed>(R"({"rest":[0,1,1]})", prefix("rest") + "is less than the minimum 1", "/rest/0");
    expect_rejected<Prefixed>(R"({"rest":[1,1,2]})", prefix("rest") + "is greater than the maximum 1", "/rest/2");
}

TEST(Constraints, ElementLiterals)
{
    expect_accepted<ElementLiterals>(R"({"pair":["ab",["yz"]],"days":["2024-01-31"]})",
                                     {{"ab", {"yz"}}, {"2024-01-31"}});
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of ElementLiterals )", key);
    };
    expect_rejected<ElementLiterals>(R"({"pair":["Ab",[]]})",
                                     element("pair") + R"(does not match the pattern "^[a-z]+$")", "/pair/0");
    expect_rejected<ElementLiterals>(R"({"pair":["abd",[]]})", element("pair") + R"(is not one of "ab", "Ab", "abc")",
                                     "/pair/0");
    expect_rejected<ElementLiterals>(R"({"pair":["ab",["y"]]})",
                                     element("pair") + R"(is not equal to the constant "yz")", "/pair/1/0");
    expect_rejected<ElementLiterals>(R"({"days":["2024-02-30"]})",
                                     element("days") + R"(does not have the format "date")", "/days/0");
}

TEST(Constraints, PropertyNames)
{
    expect_accepted<Keyed>(R"({"m":{"ab":1},"u":{"x":1},"deep":{"x":{"ab":1}},"sized":{"a":1,"b":2}})",
                           {{{"ab", 1}}, {{"x", 1}}, {{"x", {{"ab", 1}}}}, {{"a", 1}, {"b", 2}}});

    const auto prefix = [](std::string_view key) {
        return std::format(R"(a key of the value of the key "{}" of Keyed )", key);
    };
    // the error is at the value of the member whose key it is
    expect_rejected<Keyed>(R"({"m":{"ab":1,"abcd":2}})", prefix("m") + "is longer than the maximum length 3",
                           "/m/abcd");
    expect_rejected<Keyed>(R"({"m":{"A":1}})", prefix("m") + R"(does not match the pattern "^[a-z]+$")", "/m/A");
    expect_rejected<Keyed>(R"({"u":{"abc":1}})", prefix("u") + "is rejected by yyjson::satisfies", "/u/abc");
    expect_rejected<Keyed>(R"({"deep":{"x":{"ab":1,"a":2}}})", prefix("deep") + "is shorter than the minimum length 2",
                           "/deep/x/a");
    expect_rejected<Keyed>(R"({"sized":{"c":1}})", prefix("sized") + R"(is not one of "a", "b")", "/sized/c");
    expect_rejected<Keyed>(R"({"sized":{}})",
                           R"(the value of the key "sized" of Keyed has fewer properties than the minimum 1)",
                           "/sized");
}

TEST(Constraints, StructPropertyNames)
{
    expect_accepted<KeyedStruct>(R"({"ab":1,"x":2,"\u00e9\u00e9\u00e9":3})", {.ab = 1, .AB = {}});
    expect_rejected<KeyedStruct>(R"({"ab":1,"AB":2})",
                                 R"(a key of KeyedStruct does not match the pattern "^[^A-Z/]+$")", "/AB");
    expect_rejected<KeyedStruct>(R"({"abcd":1})", "a key of KeyedStruct is longer than the maximum length 3", "/abcd");
    expect_rejected<KeyedStruct>(R"({"a/b":1})", "a key of KeyedStruct does not match", "/a~1b");
}

TEST(Constraints, EscapedKeys)
{
    // a key is escaped in the path, and its length is counted in code points
    expect_rejected<Keyed>(R"({"m":{"a/b":1}})", R"(a key of the value of the key "m" of Keyed does not match)",
                           "/m/a~1b");
    expect_rejected<Keyed>(R"({"m":{"~x":1}})", R"(a key of the value of the key "m" of Keyed does not match)",
                           "/m/~0x");
    expect_accepted<Keyed>(R"({"deep":{"x":{"\u00e9\u00e9":1,"\ud83d\ude00\ud83d\ude00":2}}})",
                           {.deep = {{"x", {{"\xc3\xa9\xc3\xa9", 1}, {"\xf0\x9f\x98\x80\xf0\x9f\x98\x80", 2}}}}});
    expect_rejected<Keyed>(R"({"deep":{"x":{"\ud83d\ude00":1}}})",
                           R"(a key of the value of the key "deep" of Keyed is shorter than the minimum length 2)",
                           "/deep/x/\xf0\x9f\x98\x80");
}

TEST(Constraints, Contains)
{
    expect_accepted<Containing>(R"({"roles":["user","admin"],"v":[10,1,11],"none":[1],"rows":[[1],[5]],"pair":[1,2]})",
                                {{"user", "admin"}, {10, 1, 11}, {1}, {{1}, {5}}, {1, 2}});
    expect_accepted<Containing>(R"({"none":[],"pair":[2,0]})", {.pair = {2, 0}});

    const auto prefix = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Containing )", key);
    };
    expect_rejected<Containing>(R"({"roles":["user"]})",
                                prefix("roles") + "has no item that satisfies the constraints of yyjson::contains",
                                "/roles");
    expect_rejected<Containing>(R"({"roles":[]})",
                                prefix("roles") + "has no item that satisfies the constraints of yyjson::contains",
                                "/roles");
    expect_rejected<Containing>(R"({"v":[1,10]})",
                                prefix("v") + "has fewer than 2 items that satisfy the constraints of yyjson::contains",
                                "/v");
    expect_rejected<Containing>(R"({"v":[10,11,12,13]})",
                                prefix("v") + "has more than 3 items that satisfy the constraints of yyjson::contains",
                                "/v");
    expect_rejected<Containing>(
        R"({"none":[10,11]})",
        prefix("none") + "has more than 1 item that satisfies the constraints of yyjson::contains", "/none");
    expect_rejected<Containing>(R"({"rows":[[1],[2]]})",
                                prefix("rows") + "has no item that satisfies the constraints of yyjson::contains",
                                "/rows");
    expect_rejected<Containing>(R"({"pair":[1,3]})",
                                prefix("pair") + "has no item that satisfies the constraints of yyjson::contains",
                                "/pair");
}

TEST(Constraints, ContainsCounts)
{
    // min_contains(0) alone accepts any array, max_contains(0) alone still needs one item, so that no array
    // satisfies it, and both reject one
    expect_accepted<Counted>(R"({"min0":[],"both0":[1]})", {.both0 = {1}});
    expect_accepted<Counted>(R"({"max0":[1,10]})", {.max0 = {1, 10}});
    expect_accepted<Counted>(R"({"max0":[]})", {});
    expect_accepted<Counted>(R"({"min0":[1],"both0":[]})", {.min0 = {1}});
    const auto prefix = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Counted )", key);
    };
    expect_rejected<Counted>(R"({"both0":[1,10]})",
                             prefix("both0") + "has more than 0 items that satisfy the constraints of yyjson::contains",
                             "/both0");
    // a null element satisfies the keywords of its value, as a null field does
    expect_accepted<Counted>(R"({"opts":[null,1]})", {.opts = {std::nullopt, 1}});
    expect_rejected<Counted>(R"({"opts":[null,null]})",
                             prefix("opts") + "has more than 1 item that satisfies the constraints of yyjson::contains",
                             "/opts");
}

TEST(Constraints, OptionalRange)
{
    expect_accepted<Counted>(R"({"unique":null,"head":null,"some":null})", {});
    expect_accepted<Counted>(R"({"unique":[1,2],"head":[0,-1],"some":[10]})",
                             {.unique = std::vector{1, 2}, .head = std::vector{0, -1}, .some = std::vector{10}});
    expect_rejected<Counted>(R"({"unique":[1,1]})",
                             R"(an element of the value of the key "unique" of Counted is equal to an earlier element)",
                             "/unique/1");
    expect_rejected<Counted>(R"({"head":[-1]})",
                             R"(an element of the value of the key "head" of Counted is less than the minimum 0)",
                             "/head/0");
    expect_rejected<Counted>(
        R"({"some":[1]})",
        R"(the value of the key "some" of Counted has no item that satisfies the constraints of yyjson::contains)",
        "/some");
}

TEST(Constraints, ElementsStrict)
{
    // yyjson::strict checks the keywords of the elements as a read without it does
    const auto rejected = [](auto read, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of(read);
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
        EXPECT_EQ(e.path(), path);
    };
    rejected([] { (void)yyjson::read<Visited>(R"({"u":[1,1],"p":[0,0],"c":[5],"k":{}})", yyjson::strict); },
             R"(an element of the value of the key "u" of Visited is equal to an earlier element)", "/u/1");
    rejected([] { (void)yyjson::read<Visited>(R"({"u":[],"p":[-1,0],"c":[5],"k":{}})", yyjson::strict); },
             R"(an element of the value of the key "p" of Visited is less than the minimum 0)", "/p/0");
    rejected(
        [] {
            auto t = Deep();
            yyjson::read_into(R"({"v":[],"m":{},"k":[{"ab":1}]})", t, yyjson::strict);
        },
        R"(a key of the value of the key "k" of Deep is longer than the maximum length 1)", "/k/0/ab");
    rejected(
        [] {
            auto t = ElementsVisitable();
            yyjson::merge_into(R"({"v":[1,0]})", t, yyjson::strict);
        },
        R"(an element of the value of the key "v" of ElementsVisitable is not greater than)", "/v/1");
    rejected([] { (void)yyjson::read(R"({"u":[],"p":[0,0],"c":[4],"k":{}})").cast<Visited>(yyjson::strict); },
             R"(the value of the key "c" of Visited has no item that satisfies)", "/c");
}

TEST(Constraints, VisitableElements)
{
    expect_accepted<Visited>(R"({"u":[1,2],"p":[0,-1],"c":[5],"k":{"a":1}})", {{1, 2}, {0, -1}, {5}, {{"a", 1}}});
    const auto prefix = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Visited )", key);
    };
    expect_rejected<Visited>(R"({"u":[1,1],"c":[5]})",
                             R"(an element of the value of the key "u" of Visited is equal to an earlier element)",
                             "/u/1");
    expect_rejected<Visited>(R"({"p":[-1,0],"c":[5]})",
                             R"(an element of the value of the key "p" of Visited is less than the minimum 0)", "/p/0");
    expect_rejected<Visited>(R"({"c":[4]})",
                             prefix("c") + "has no item that satisfies the constraints of yyjson::contains", "/c");
    expect_rejected<Visited>(R"({"c":[5],"k":{"ab":1}})",
                             R"(a key of the value of the key "k" of Visited is longer than the maximum length 1)",
                             "/k/ab");
}

TEST(Constraints, DeepElements)
{
    expect_accepted<Deep>(R"({"v":[[[0],[1]]],"m":{"y":[1]},"k":[{"a":1}]})",
                          {{{{0}, {1}}}, {{"y", {1}}}, {{{"a", 1}}}});
    const auto prefix = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Deep )", key);
    };
    expect_rejected<Deep>(R"({"v":[[[0,-1]]]})", prefix("v") + "is less than the minimum 0", "/v/0/0/1");
    expect_rejected<Deep>(R"({"v":[[[0],[0]]]})",
                          prefix("v") + "has no item that satisfies the constraints of yyjson::contains", "/v/0/1");
    expect_rejected<Deep>(R"({"m":{"x":[1],"y":[0]}})",
                          prefix("m") + "has no item that satisfies the constraints of yyjson::contains", "/m/y");
    expect_rejected<Deep>(R"({"k":[{"a":1},{"bb":1}]})",
                          R"(a key of the value of the key "k" of Deep is longer than the maximum length 1)",
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
                              .pm = {{1, 2}, {1, 3}}});
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Unusual )", key);
    };
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Unusual )", key);
    };
    // the elements that a user caster reads are converted again to find the equal one
    expect_rejected<Unusual>(R"({"pts":[[0,1],[0,1]]})", element("pts") + "is equal to an earlier element", "/pts/1");
    // a key that is an enumeration is checked by the name that it is written as
    expect_rejected<Unusual>(R"({"sides":{"left":1,"right":2}})",
                             R"(a key of the value of the key "sides" of Unusual is not equal to the constant "left")",
                             "/sides/right");
    // a range of pairs is read from an object, whose members have the keys, or from an array of pairs
    expect_rejected<Unusual>(R"({"kv":{"a":1,"bb":2}})",
                             R"(a key of the value of the key "kv" of Unusual is longer than the maximum length 1)",
                             "/kv/bb");
    expect_rejected<Unusual>(R"({"kv":[["a",1],["bb",2]]})",
                             value("kv") + "has a key that does not satisfy the constraints of yyjson::property_names",
                             "/kv");
    // the prefix and the elements after it, in sequences that are not vectors
    expect_rejected<Unusual>(R"({"dq":[-1]})", element("dq") + "is less than the minimum 0", "/dq/0");
    expect_rejected<Unusual>(R"({"dq":[0,10]})", element("dq") + "is greater than the maximum 9", "/dq/1");
    expect_rejected<Unusual>(R"({"ls":[0,0]})", element("ls") + "is less than the minimum 1", "/ls/1");
    expect_rejected<Unusual>(R"({"ls":[0,1,9,10]})", element("ls") + "is greater than the maximum 9", "/ls/3");
    // a multimap whose keys are not strings is an array of pairs, which may repeat one
    expect_rejected<Unusual>(R"({"pm":[[1,2],[1,2]]})", element("pm") + "is equal to an earlier element", "/pm/1");
    expect_rejected<Unusual>(R"({"pm":[[1,2],[1,3],[1,4]]})", value("pm") + "has more items than the maximum 2", "/pm");
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
    const auto unique = [](const char* json) { return expect_direct_read<Unique>(json); };
    same(unique, true, R"({"ints":[1,2],"reals":[0.5]})");
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

    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Negated )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Negated )", key);
    };
    constexpr auto negated = "satisfies the settings of yyjson::not_";
    expect_rejected<Negated>(R"({"n":10})", value("n") + negated, "/n");
    expect_rejected<Negated>(R"({"n":19})", value("n") + negated, "/n");
    expect_rejected<Negated>(R"({"s":"xy"})", value("s") + negated, "/s");
    // not_(not_(minimum(0))) is minimum(0)
    expect_rejected<Negated>(R"({"v":[0,-1]})", element("v") + negated, "/v/1");
    // each yyjson::not_ of a field is checked
    expect_rejected<Negated>(R"({"twice":1})", value("twice") + negated, "/twice");
    expect_rejected<Negated>(R"({"twice":2})", value("twice") + negated, "/twice");
    expect_rejected<Negated>(R"({"m":{"a":0}})", element("m") + negated, "/m/a");
    expect_rejected<Negated>(R"({"m":{"_a":1}})",
                             R"(a key of the value of the key "m" of Negated )" + std::string(negated), "/m/_a");
    expect_rejected<Negated>(R"({"row":[0,"y"]})", element("row") + negated, "/row/0");
    expect_rejected<Negated>(R"({"row":[1,"x"]})", element("row") + negated, "/row/1");
    expect_rejected<Negated>(R"({"c":[1,2]})",
                             value("c") + "has no item that satisfies the constraints of yyjson::contains", "/c");
    // null satisfies minimum(5), so not_ rejects a null element, but a null field is missing
    expect_rejected<Negated>(R"({"holes":[1,null]})", element("holes") + negated, "/holes/1");
    expect_rejected<Negated>(R"({"o":5})", value("o") + negated, "/o");
    // not_ of a keyword of the elements
    expect_rejected<Negated>(R"({"nested":[[1]]})", value("nested") + negated, "/nested");
    expect_rejected<Negated>(R"({"nested":[]})", value("nested") + negated, "/nested");
}

TEST(Constraints, IfThenElse)
{
    expect_accepted<Conditional>(R"({"n":12,"v":[10],"e":[0,5],"s":"#abc","m":{"a":"x","b":"y"}})",
                                 {12, {10}, {0, 5}, "#abc", {{"a", "x"}, {"b", "y"}}});
    expect_accepted<Conditional>(R"({"n":5,"v":[1,9],"s":"ab"})", {5, {1, 9}, {}, "ab", {}});

    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Conditional )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Conditional )", key);
    };
    constexpr auto then = "satisfies the settings of yyjson::if_ and not those of yyjson::then";
    constexpr auto else_ = "satisfies neither the settings of yyjson::if_ nor those of yyjson::else_";
    expect_rejected<Conditional>(R"({"n":11})", value("n") + then, "/n");
    expect_rejected<Conditional>(R"({"n":6})", value("n") + else_, "/n");
    expect_rejected<Conditional>(R"({"v":[1,10]})", value("v") + then, "/v");
    expect_rejected<Conditional>(R"({"e":[0,4]})", element("e") + else_, "/e/1");
    expect_rejected<Conditional>(R"({"s":"#ab"})", value("s") + then, "/s");
    expect_rejected<Conditional>(R"({"m":{"a":"w"}})", element("m") + else_, "/m/a");
}

TEST(Constraints, AllAnyOneOf)
{
    expect_accepted<Composed>(R"({"a":10,"b":-1,"c":4,"v":[0,10],"s":"c","m":{"k":-1},"deep":-5,"rows":[[1,2],[3,4]]})",
                              {10, -1, 4, {0, 10}, "c", {{"k", -1}}, -5, {{1, 2}, {3, 4}}});
    expect_accepted<Composed>(R"({"b":15,"c":3,"s":"a","m":{"k":10},"deep":100,"rows":[[]]})",
                              {.b = 15, .c = 3, .s = "a", .m = {{"k", 10}}, .deep = 100, .rows = {{}}});
    expect_accepted<Composed>(R"({"c":9,"s":"dz","deep":300})", {.c = 9, .s = "dz", .deep = 300});

    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Composed )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Composed )", key);
    };
    constexpr auto all = "does not satisfy all the subschemas of yyjson::all_of";
    constexpr auto any = "satisfies none of the subschemas of yyjson::any_of";
    constexpr auto none = "satisfies none of the subschemas of yyjson::one_of";
    constexpr auto many = "satisfies more than one subschema of yyjson::one_of";
    expect_rejected<Composed>(R"({"a":-2})", value("a") + all, "/a");
    expect_rejected<Composed>(R"({"a":3})", value("a") + all, "/a");
    expect_rejected<Composed>(R"({"a":12})", value("a") + all, "/a");
    expect_rejected<Composed>(R"({"b":5})", value("b") + any, "/b");
    expect_rejected<Composed>(R"({"b":21})", value("b") + any, "/b");
    expect_rejected<Composed>(R"({"c":1})", value("c") + none, "/c");
    expect_rejected<Composed>(R"({"c":6})", value("c") + many, "/c");
    expect_rejected<Composed>(R"({"v":[0,5]})", element("v") + any, "/v/1");
    expect_rejected<Composed>(R"({"s":"e"})", value("s") + any, "/s");
    // minimum(0) and maximum(9) both hold for 5
    expect_rejected<Composed>(R"({"m":{"k":5}})", element("m") + many, "/m/k");
    expect_rejected<Composed>(R"({"deep":0})", value("deep") + none, "/deep");
    expect_rejected<Composed>(R"({"deep":150})", value("deep") + none, "/deep");
    expect_rejected<Composed>(R"({"rows":[[1],[2]]})", value("rows") + any, "/rows");
}

TEST(Constraints, DeadBranches)
{
    // a subschema that no value satisfies is false, as in JSON Schema
    expect_accepted<DeadBranches>(R"({"byte":255,"any":3,"text":"ab","one":[1,2],"low":10})",
                                  {255, 3, "ab", {1, 2}, 10});
    expect_accepted<DeadBranches>(R"({"byte":0,"any":-7,"text":"","one":[1,2,3]})", {0, -7, "", {1, 2, 3}});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of DeadBranches )", key);
    };
    expect_rejected<DeadBranches>(R"({"any":4})", value("any") + "satisfies none of the subschemas of yyjson::any_of",
                                  "/any");
    expect_rejected<DeadBranches>(
        R"({"text":"abc"})", value("text") + "satisfies neither the settings of yyjson::if_ nor those of yyjson::else_",
        "/text");
    expect_rejected<DeadBranches>(R"({"one":[1]})", value("one") + "satisfies none of the subschemas of yyjson::one_of",
                                  "/one");
    expect_rejected<DeadBranches>(R"({"low":9})", value("low") + "satisfies none of the subschemas of yyjson::any_of",
                                  "/low");
    expect_rejected<DeadBranches>(
        R"({"byte":256})", "overflow detected: unsigned char is not constructible from JSON integer 256", "/byte");
}

TEST(Constraints, Matches)
{
    expect_accepted<Typed>(
        R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},"text":1,)"
        R"("small":{"a":255}})",
        yyjson::read<Typed>(R"({"events":[{"x":1,"y":2},1,{"x":0,"y":0,"z":0}],"ids":[1,"a"],"point":{"x":1,"y":2},)"
                            R"("text":1,"small":{"a":255}})"));

    const auto value = [](std::string_view key) { return std::format(R"(the value of the key "{}" of Typed )", key); };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Typed )", key);
    };
    const auto few = "has fewer than 2 items that satisfy the constraints of yyjson::contains";
    // a point without "y" is not read under yyjson::strict
    expect_rejected<Typed>(R"({"events":[{"x":1,"y":2},{"x":1}],"point":{"x":1,"y":2}})", value("events") + few,
                           "/events");
    expect_rejected<Typed>(R"({"events":[{"x":1,"y":2},{"x":1,"y":null}],"point":{"x":1,"y":2}})",
                           value("events") + few, "/events");
    expect_rejected<Typed>(R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"y":3}],"point":{"x":1,"y":2}})",
                           value("events") + few, "/events");
    expect_rejected<Typed>(R"({"ids":[1,1.5],"point":{"x":1,"y":2}})",
                           element("ids") + "satisfies none of the subschemas of yyjson::any_of", "/ids/1");
    expect_rejected<Typed>(R"({"point":[1,2]})", value("point") + "is not read as Point by yyjson::matches", "/point");
    expect_rejected<Typed>(R"({"point":{"x":1,"y":2},"text":"a"})",
                           value("text") + "satisfies the settings of yyjson::not_", "/text");
    expect_rejected<Typed>(R"({"point":{"x":1,"y":2},"small":{"a":256}})",
                           element("small") + "is not read as unsigned char by yyjson::matches", "/small/a");

    // the policies of the read around yyjson::matches do not change it
    constexpr auto unknown = R"({"events":[{"x":1,"y":2},{"x":1,"y":2,"z":3}],"ids":[],"point":{"x":1,"y":2,"z":3},)"
                             R"("text":null,"small":{}})";
    EXPECT_NO_THROW((void)yyjson::read<Typed>(unknown, yyjson::reject_unknown_keys));
    EXPECT_NO_THROW((void)yyjson::read<Typed>(unknown, yyjson::strict));
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
    // merge_into checks the values of the patch, and a null patch resets the field without a check
    auto merged = Negated{.n = 5, .o = 1};
    yyjson::merge_into(R"({"n":9,"o":null})", merged);
    EXPECT_EQ(merged.n, 9);
    EXPECT_FALSE(merged.o.has_value());
    EXPECT_THROW(yyjson::merge_into(R"({"n":11})", merged), yyjson::bad_cast);

    expect_accepted<VisitedCombined>(R"({"n":1,"v":[4,8]})", {1, {4, 8}});
    expect_rejected<VisitedCombined>(
        R"({"n":0})", R"(the value of the key "n" of VisitedCombined satisfies the settings of yyjson::not_)", "/n");
    expect_rejected<VisitedCombined>(
        R"({"n":1,"v":[6]})",
        R"(an element of the value of the key "v" of VisitedCombined satisfies more than one subschema of yyjson::one_of)",
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
    void expect_conditions(std::string_view json, std::string_view message, std::string_view path = "",
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
            if (message.empty())
                EXPECT_NO_THROW(reads[i]()) << json << " " << i;
            else
            {
                const auto e = bad_cast_of(reads[i]);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << " " << i << "\n" << e.what();
                EXPECT_EQ(e.path(), i == 3 ? "/0" + std::string(path) : std::string(path)) << json << " " << i;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty()) << json;
        EXPECT_TRUE(expect_direct_read<T>(json) || !message.empty()) << json;
        EXPECT_TRUE(expect_read_into(T(), json) || !message.empty()) << json;
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(T(), json, &merged_directly));
        if (merged)
        {
            EXPECT_EQ(merged_directly, message.empty()) << json;
        }
#endif
    }
}  // namespace

TEST(Constraints, StructConditions)
{
    using S = ConditionShape;
    const auto rejected = "ConditionShape has the keys and the values that yyjson::not_ rejects";
    const auto point = R"(the value of the key "p" of ConditionShape is not read as ConditionPoint by yyjson::matches)";
    expect_conditions<S>(R"({"kind":"circle","radius":1})", "");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1},"sides":[1,2,3]})", "");
    // another kind takes else_, whose keys are absent, and leaves the radius unchecked
    expect_conditions<S>(R"({"kind":"square","radius":-1})", "");
    expect_conditions<S>(R"({"kind":"circle"})",
                         R"(ConditionShape is not constructible from JSON without the key "radius", which )"
                         R"(yyjson::if_then_else requires)");
    expect_conditions<S>(R"({"kind":"circle","radius":-1})",
                         R"(the value of the key "radius" of ConditionShape is less than the minimum 0)", "/radius");
    expect_conditions<S>(R"({"kind":"square","p":{"x":"a"}})", point, "/p");
    expect_conditions<S>(R"({"kind":"square","p":{"x":1,"y":2}})", "");
    expect_conditions<S>(R"({"kind":"square","sides":[1,0,2]})",
                         R"(an element of the value of the key "sides" of ConditionShape is less than the minimum 1)",
                         "/sides/1");
    expect_conditions<S>(R"({"kind":"square","sides":[1,2]})",
                         R"(the value of the key "sides" of ConditionShape has fewer items than the minimum 3)",
                         "/sides");
    // not_ of a key holds for an object without the key, as JSON Schema's `not` of `properties`
    expect_conditions<S>(R"({"kind":"xy"})", rejected);
    expect_conditions<S>(R"({})", rejected);
    // a null that the read takes as a missing key is no key
    expect_conditions<S>(R"({"kind":"circle","radius":null})",
                         R"(ConditionShape is not constructible from JSON without the key "radius", which )"
                         R"(yyjson::if_then_else requires)");

    expect_conditions<Leveled>(R"({"level":4})", "");
    expect_conditions<Leveled>(R"({"level":5,"note":"n"})", "");
    expect_conditions<Leveled>(
        R"({"level":5})",
        R"(Leveled is not constructible from JSON without the key "note", which yyjson::if_then_else requires)");
    expect_conditions<Leveled>(R"({"level":5,"note":""})",
                               R"(the value of the key "note" of Leveled is shorter than the minimum length 1)",
                               "/note");

    expect_conditions<ConditionVisitable>(R"({"a":1,"b":2})", "");
    expect_conditions<ConditionVisitable>(R"({"b":2})", "");
    expect_conditions<ConditionVisitable>(R"({"a":1})",
                                          R"(ConditionVisitable is not constructible from JSON without the key "b", )"
                                          R"(which yyjson::if_then_else requires)");
    expect_conditions<ConditionReordered>(R"({"a":1,"b":1,"c":5})", "");
    expect_conditions<ConditionReordered>(R"({"a":9,"b":9,"c":1})",
                                          "ConditionReordered satisfies none of the subschemas of yyjson::any_of");
    expect_conditions<ConditionConstructed>(R"({"a_":0})", "");
    expect_conditions<ConditionConstructed>(R"({"a_":1,"b_":0})", "");
    expect_conditions<ConditionConstructed>(R"({"a_":1})",
                                            R"(ConditionConstructed is not constructible from JSON )"
                                            R"(without the key "b_", which yyjson::if_then_else requires)");

    // the keys of the object tell which fields it has, whatever values the fields have in T()
    const auto missing_d =
        R"(ConditionPresence is not constructible from JSON without the key "d", which yyjson::if_then_else requires)";
    const auto both = "ConditionPresence has the keys and the values that yyjson::not_ rejects";
    expect_conditions<ConditionPresence>(R"({})", "");
    expect_conditions<ConditionPresence>(R"({"c":7})", "");
    expect_conditions<ConditionPresence>(R"({"a":5,"b":0,"c":7,"d":1})", "");
    expect_conditions<ConditionPresence>(R"({"d":1,"a":5})", "");
    expect_conditions<ConditionPresence>(R"({"a":5})", missing_d);
    expect_conditions<ConditionPresence>(R"({"b":0})", missing_d);
    expect_conditions<ConditionPresence>(R"({"c":7,"b":0})", missing_d);
    expect_conditions<ConditionPresence>(R"({"a":5,"b":0,"c":7,"e":0,"d":1})", both);
    expect_conditions<ConditionPresence>(R"({"e":0,"c":7})", both);

    expect_conditions<Contact>(R"({"name":"a","email":"e"})", "");
    expect_conditions<Contact>(R"({"id":1,"phone":"p","age":0})", "");
    // a yyjson::one_of or any_of of yyjson::required("key", ...) names the keys in its message
    expect_conditions<Contact>(R"({"email":"e"})",
                               R"(Contact is not constructible from JSON without all the keys of one of ["name"], )"
                               R"(["id"])");
    expect_conditions<Contact>(R"({"name":"a"})",
                               R"(Contact is not constructible from JSON without all the keys of one of ["email"], )"
                               R"(["phone"])");
    expect_conditions<Contact>(R"({"name":"a","email":"e","phone":"p"})",
                               R"(Contact is not constructible from JSON with all the keys of more than one of )"
                               R"(["email"], ["phone"])");
    expect_conditions<Contact>(R"({"name":"a","email":"e","age":-1})",
                               R"(the value of the key "age" of Contact is less than the minimum 0)", "/age");
    expect_conditions<Contact>(R"({"id":0,"email":"e"})",
                               "Contact has the keys and the values that yyjson::not_ rejects");
}

TEST(Constraints, PropertyAndField)
{
    expect_accepted<PropertyMerged>(R"({"v":9})", {9});
    expect_rejected<PropertyMerged>(R"({"v":-1})",
                                    R"(the value of the key "v" of PropertyMerged is less than the minimum 0)", "/v");
    expect_rejected<PropertyMerged>(
        R"({"v":10})", R"(the value of the key "v" of PropertyMerged is greater than the maximum 9)", "/v");

    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1})",
        "");
    expect_conditions<ManyKeys>(R"({"z":1})", "");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1})",
        R"(ManyKeys is not constructible from JSON without all the keys of one of )"
        R"(["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");
    expect_conditions<ManyKeys>(
        R"({"a":1,"b":1,"c":1,"d":1,"e":1,"f":1,"g":1,"h":1,"i":1,"j":1,"k":1,"l":1,"m":1,"n":1,"o":1,"p":1,"q":1,"z":1})",
        R"(ManyKeys is not constructible from JSON with all the keys of more than one of )"
        R"(["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q"], ["z"])");

    const auto below = [](std::string_view type) {
        return std::format(R"(the value of the key "b" of {} is less than the minimum 1)", type);
    };
    expect_conditions<NestedAllOfProperty>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfProperty>(R"({"a":1,"b":0})", below("NestedAllOfProperty"), "/b");
    expect_conditions<NestedAllOfMixed>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfMixed>(R"({"a":1,"b":0})", below("NestedAllOfMixed"), "/b");
    expect_conditions<NestedAllOfDeep>(R"({"a":1,"b":2})", "");
    expect_conditions<NestedAllOfDeep>(R"({"a":1,"b":0})", below("NestedAllOfDeep"), "/b");
    expect_conditions<NestedAllOfObject>(R"({"a":1,"b":{"c":1}})", "");
    expect_conditions<NestedAllOfObject>(R"({"a":1,"b":{}})",
                                         R"(the value of the key "b" of NestedAllOfObject does not have the keys )"
                                         R"(and the values that yyjson::property and required give)",
                                         "/b");

    expect_conditions<PresenceNestedKnown>(R"({"v":{"bb":1,"a":0}})", "");
    expect_conditions<PresenceNestedKnown>(R"({"v":{"a":0}})",
                                           R"(the value of the key "v" of PresenceNestedKnown does not have the keys )"
                                           R"(and the values that yyjson::property and required give)",
                                           "/v");
    expect_conditions<PresenceNestedKnown>(
        R"({"v":{"bb":1,"a":-1}})",
        R"(the value of the key "v" of PresenceNestedKnown does not have the keys and the values that )", "/v");
}

TEST(Constraints, StructConditionsOfNull)
{
    // a null that the read takes as a missing key is no key, and the conditions do not check the value that the
    // field keeps
    expect_conditions<NullKeys>(R"({"a":null,"b":1})", "");
    expect_conditions<NullKeys>(
        R"({"a":null})", R"(NullKeys is not constructible from JSON without all the keys of one of ["a"], ["b"])");
    expect_conditions<NullKeys>(R"({"a":4})", R"(the value of the key "a" of NullKeys is greater than the maximum 3)",
                                "/a");
    // a null in a field that holds null is a present key, which a merge takes as a removal
    expect_conditions<NullKeys>(R"({"b":1,"c":null,"d":1})", "");
    expect_conditions<NullKeys>(R"({"b":1,"c":null})",
                                R"(NullKeys is not constructible from JSON without the key "d", which )"
                                R"(yyjson::if_then_else requires)",
                                "", false);
}

TEST(Constraints, StructConditionsMerged)
{
    // merge_into checks the object after the merge: the members of the patch that are not null, and the fields that
    // the patch does not name and that the write gives, with the values that the merge gives
    auto c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"a","email":"e","age":3})", c);
    EXPECT_EQ(c.age, 3);
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"name":"a","email":"e","age":-1})", c); });
    EXPECT_TRUE(message_of(e).starts_with(R"(the value of the key "age" of Contact is less than the minimum 0)"))
        << e.what();
    EXPECT_EQ(e.path(), "/age");
    yyjson::merge_into(R"({"age":4})", c);
    EXPECT_EQ(c, (Contact{.name = "a", .email = "e", .age = 4}));
    e = bad_cast_of([&] { yyjson::merge_into(R"({"name":null})", c); });
    EXPECT_TRUE(message_of(e).starts_with(
        R"(Contact is not constructible from JSON without all the keys of one of ["name"], ["id"])"))
        << e.what();
    c = Contact{.name = "a", .email = "e"};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"phone":"p"})", c); });
    EXPECT_TRUE(message_of(e).starts_with(
        R"(Contact is not constructible from JSON with all the keys of more than one of ["email"], ["phone"])"))
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
    const auto rejected = [](std::string_view result, std::string_view message) {
        EXPECT_TRUE(result.starts_with("bad cast: " + std::string(message))) << result;
    };
    const auto cert = R"(MergedServer is not constructible from JSON without the key "cert", which )"
                      R"(yyjson::dependent_required requires)";
    // a field equal to its default is not in the object that the write gives, and a field with
    // yyjson::emit_defaults is
    const auto server = yyjson::read<MergedServer>(R"({"host":"a"})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b"})"), R"({"host":"b","port":8080})");
    EXPECT_EQ(expect_merge_rule(server, R"({"host":"b","cert":"x"})"), R"({"host":"b","port":8080,"cert":"x"})");
    rejected(expect_merge_rule(server, R"({"port":8080})"), cert);
    const auto secure = yyjson::read<MergedServer>(R"({"host":"a","port":9000,"cert":"x"})");
    EXPECT_EQ(expect_merge_rule(secure, R"({"host":"b"})"), R"({"host":"b","port":9000,"cert":"x"})");
    rejected(expect_merge_rule(secure, R"({"cert":null})"), cert);
    EXPECT_EQ(expect_merge_rule(secure, R"({"cert":null,"port":null})"), R"({"host":"a","port":8080})");
    EXPECT_EQ(expect_merge_rule(MergedServer{"a", 8080, "x"}, R"({"cert":null})"), R"({"host":"a","port":8080})");
    rejected(expect_merge_rule(MergedEmitted{"a", 8080, "x"}, R"({"cert":null})"),
             R"(MergedEmitted is not constructible from JSON without the key "cert")");

    // the keys that the patch does not name stay in the object
    const auto choice = yyjson::read<MergedChoice>(R"({"a":1})");
    const auto many = R"(MergedChoice is not constructible from JSON with all the keys of more than one of )";
    rejected(expect_merge_rule(choice, R"({"b":2})"), many);
    EXPECT_EQ(expect_merge_rule(choice, R"({"x":1})"), R"({"a":1})");
    EXPECT_EQ(expect_merge_rule(choice, R"({"a":null,"b":2})"), R"({"b":2})");
    rejected(expect_merge_rule(choice, R"({"a":null})"),
             R"(MergedChoice is not constructible from JSON without all the keys of one of )");

    // a null removes the key that the object needs
    EXPECT_EQ(expect_merge_rule(MergedRequired{1, {}}, R"({"n":2})"), R"({"id":1,"n":2})");
    rejected(expect_merge_rule(MergedRequired{1, {}}, R"({"id":null})"),
             R"(MergedRequired is not constructible from JSON without the key "id")");
    EXPECT_THROW(yyjson::merge_into(R"({"id":null})", *std::make_unique<MergedRequired>()), yyjson::bad_cast);

    // the counts take the members of the object after the merge
    const auto counted = [](int a, std::optional<int> b, int z) {
        auto t = MergedCounted{a, b, {}};
        t.z = z;
        return t;
    };
    const auto more = "MergedCounted has more properties than the maximum 2";
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"b":2})"), R"({"a":1,"b":2})");
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 0), R"({"z":3})"), R"({"a":1,"z":3})");
    rejected(expect_merge_rule(counted(1, {}, 3), R"({"b":2})"), more);
    rejected(expect_merge_rule(counted(1, {}, 0), R"({"b":2,"y":1})"), more);
    EXPECT_EQ(expect_merge_rule(counted(1, {}, 3), R"({"b":2,"z":null})"), R"({"a":1,"b":2})");
    const auto fewer = "MergedMinimum has fewer properties than the minimum 2";
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"c":1})"), R"({"a":0,"b":1})");
    EXPECT_EQ(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":2})"), R"({"a":0,"b":2})");
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"b":null})"), fewer);
    rejected(expect_merge_rule(MergedMinimum{0, 1}, R"({"a":null,"b":null,"c":1})"), fewer);

    // a closed subschema sees the keys of the object after the merge
    const auto none = "MergedClosed satisfies none of the subschemas of yyjson::one_of";
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"x":2})"), R"({"x":2})");
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, {}}, R"({"z":1})"), R"({"x":1,"z":1})");
    rejected(expect_merge_rule(MergedClosed{1, {}}, R"({"y":1})"), none);
    EXPECT_EQ(expect_merge_rule(MergedClosed{1, 1}, R"({"z":null})"), R"({"x":1})");
    rejected(expect_merge_rule(MergedClosed{1, 1}, R"({"x":null})"), none);
}

TEST(Constraints, MergedObjectAtField)
{
    const auto rejected = [](std::string_view result, std::string_view message) {
        EXPECT_TRUE(result.starts_with("bad cast: " + std::string(message))) << result;
    };
    const auto keys = R"(the value of the key "start" of MergedPath does not have the keys and the values that )"
                      R"(yyjson::property and required give at /start)";
    const auto fewer = R"(the value of the key "end" of MergedPath has fewer properties than the minimum 2 at /end)";
    const auto merge = [](const MergedPath& t, std::string_view patch) {
        return expect_merge_rule<MergedPath, true>(t, patch);
    };
    const auto path = MergedPath{{1, 3}, {}};
    EXPECT_EQ(merge(path, R"({"start":{"x":2}})"), R"({"start":{"x":2,"y":3}})");
    rejected(merge(path, R"({"start":{"y":null}})"), keys);
    rejected(merge(MergedPath{{1, {}}, {}}, R"({"start":{"x":2}})"), keys);
    rejected(merge(path, R"({"end":{"x":2}})"), fewer);
    rejected(merge(path, R"({"end":{"y":4}})"), fewer);
    rejected(merge(path, R"({"end":{"x":2,"y":null}})"), fewer);
    EXPECT_EQ(merge(path, R"({"end":{"x":2,"y":4}})"), R"({"start":{"x":1,"y":3},"end":{"x":2,"y":4}})");
    const auto ended = MergedPath{{1, 3}, MergedPoint{2, 4}};
    EXPECT_EQ(merge(ended, R"({"end":{"x":5}})"), R"({"start":{"x":1,"y":3},"end":{"x":5,"y":4}})");
    rejected(merge(ended, R"({"end":{"y":null}})"), fewer);
}

TEST(Constraints, MergedAbsent)
{
    // a value that is absent, as an empty optional or a new key of a map, reads the patch without the null members
    // of its objects
    const auto rejected = [](std::string_view result, std::string_view message) {
        EXPECT_TRUE(result.starts_with("bad cast: " + std::string(message))) << result;
    };
    const auto merge = [](std::string_view patch) {
        return expect_merge_rule<MergedAbsent, true>(MergedAbsent(), patch);
    };
    const auto id = R"(MergedRequired is not constructible from JSON without the key "id")";
    rejected(merge(R"({"req":{"n":1}})"), id);
    rejected(merge(R"({"req":{"id":null,"n":1}})"), id);
    EXPECT_EQ(merge(R"({"req":{"id":1,"n":null}})"), R"({"req":{"id":1},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"init":{"x":null,"y":4}})"), R"({"init":{"x":5,"y":4},"reqs":{},"mins":{}})");
    EXPECT_EQ(merge(R"({"point":{"x":null,"y":4}})"), R"({"point":{"x":0,"y":4},"reqs":{},"mins":{}})");
    rejected(merge(R"({"reqs":{"k":{"n":1}}})"), id);
    rejected(merge(R"({"reqs":{"k":{"id":null}}})"), id);
    EXPECT_EQ(merge(R"({"reqs":{"k":{"id":1,"n":null}}})"), R"({"reqs":{"k":{"id":1}},"mins":{}})");
    const auto fewer = "MergedMinimum has fewer properties than the minimum 2";
    rejected(merge(R"({"mins":{"k":{"b":1}}})"), fewer);
    rejected(merge(R"({"mins":{"k":{"b":1,"c":null}}})"), fewer);
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
    const auto rejected = [](std::string_view result, std::string_view message) {
        EXPECT_TRUE(result.starts_with("bad cast: " + std::string(message))) << result;
    };
    // a field that the write omits is absent
    const auto outer = OmittedOuter{.name = "a", .in = {}, .min = {}};
    const auto merge = [](const OmittedOuter& t, std::string_view patch) {
        return expect_merge_rule<OmittedOuter, true>(t, patch);
    };
    rejected(merge(outer, R"({"in":{"y":1}})"), R"(OmittedInner is not constructible from JSON without the key "z")");
    EXPECT_EQ(merge(outer, R"({"in":{"y":1,"z":2}})"), R"({"name":"a","in":{"y":1,"z":2},"min":{"a":0}})");
    rejected(merge(outer, R"({"min":{"b":1}})"), "MergedMinimum has fewer properties than the minimum 2");
    EXPECT_EQ(merge(outer, R"({"min":{"a":1,"b":null,"c":2}})"), R"({"name":"a","in":{"z":0},"min":{"a":1}})");
    auto written = outer;
    written.in.z = 1;
    EXPECT_EQ(merge(written, R"({"in":{"y":1}})"), R"({"name":"a","in":{"y":1,"z":1},"min":{"a":0}})");

    // the counts in a condition count a field that the patch does not name
    rejected(expect_merge_rule(KeptGate{{}, {1}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
             R"(the value of the key "s" of KeptGate has fewer items than the minimum 2)");
    rejected(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1})"),
             R"(the value of the key "m" of KeptGate has fewer properties than the minimum 2)");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}}}, R"({"a":1,"m":{"l":2}})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");
    EXPECT_EQ(expect_merge_rule(KeptGate{{}, {1, 2}, {{"k", 1}, {"l", 2}}}, R"({"a":1})"),
              R"({"a":1,"s":[1,2],"m":{"k":1,"l":2}})");

    // a new key of a map of the keys that no field names is absent
    const auto id = R"(MergedRequired is not constructible from JSON without the key "id")";
    const auto rest = [](std::string_view patch) {
        return expect_merge_rule<MergedRest, true>(MergedRest{.a = 1, .ks = {}, .rest = {{"r", {1, {}}}}}, patch);
    };
    rejected(rest(R"({"x":{"n":1}})"), id);
    rejected(rest(R"({"k":{"n":1}})"), id);
    EXPECT_EQ(rest(R"({"x":{"id":2,"n":null},"k":{"id":3,"n":null}})"),
              R"({"a":1,"k":{"id":3},"r":{"id":1},"x":{"id":2}})");
    EXPECT_EQ(rest(R"({"r":{"n":2}})"), R"({"a":1,"r":{"id":1,"n":2}})");
    rejected(rest(R"({"r":{"id":null}})"), id);

    // a field that the write wrote before a merge without a document changed it to its default is not absent in
    // the merge through a document that follows, which the null of the adjacent tag starts
    const auto fallback = [](const WrittenOuter& t, std::string_view patch) {
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(t, patch, &merged_directly));
        EXPECT_FALSE(merged_directly) << patch;
        return expect_merge_rule<WrittenOuter, true>(t, patch);
    };
    EXPECT_EQ(fallback(WrittenOuter{{1, 0}, {}, {}}, R"({"in":{"y":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    EXPECT_EQ(fallback(WrittenOuter{{}, {{"a", 1}}, {}}, R"({"m":{"a":null},"body":null,"t":null})"),
              R"({"in":{"z":0},"m":{},"body":null})");
    rejected(fallback(WrittenOuter(), R"({"in":{"y":1},"body":null,"t":null})"),
             R"(WrittenInner is not constructible from JSON without the key "z")");
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
    expect_conditions<TypeTested>(R"({"auto":{}})", R"(TypeTested is not constructible from JSON without the key )"
                                                    R"("name", which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"auto":{},"name":"a"})", "");
    expect_conditions<TypeTested>(R"({"auto":[]})", "");
    expect_conditions<TypeTested>(R"({"kind":null})",
                                  "TypeTested has the keys and the values that yyjson::not_ rejects", "", false);
    expect_conditions<TypeTested>(R"({"kind":1})", "");
    expect_conditions<TypeTested>(R"({"point":{"x":1}})", R"(TypeTested is not constructible from JSON without )"
                                                          R"(the key "label", which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"point":{"x":1,"y":2},"label":"l"})", "");
    expect_conditions<TypeTested>(R"({"point":"a"})", "");

    const auto missing = R"(AlignedConditions is not constructible from JSON without the key "kind", which )"
                         R"(yyjson::if_then_else requires)";
    const auto rejected = "AlignedConditions has the keys and the values that yyjson::not_ rejects";
    expect_conditions<AlignedConditions>(R"({"xs":[1],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2],"ys":[],"rows":[]})", missing);
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3],"kind":"few","ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3,4],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3,4],"kind":"few","ys":[],"rows":[]})", rejected);
    expect_conditions<AlignedConditions>(R"({"xs":[1,2],"kind":"many","ys":[],"rows":[]})",
                                         R"(the value of the key "kind" of AlignedConditions is not )", "/kind");

    const auto n = R"(PrefixConditions is not constructible from JSON without the key "n", which )"
                   R"(yyjson::if_then_else requires)";
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""]})", "");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["a"]})", n);
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":[]})", n);
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["b"]})", "");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["a"],"n":1})", "");
}

TEST(Constraints, ClosedConditions)
{
    const auto closed = [](std::string_view type, std::string_view key) {
        return std::string(type) + R"( has the key ")" + std::string(key) +
               R"(", which yyjson::additional_properties(false) of yyjson::if_then_else rejects)";
    };
    const auto shape = [&](std::string_view json, std::string_view key = "") {
        expect_conditions<ConditionClosed>(json, key.empty() ? "" : closed("ConditionClosed", key),
                                           key.empty() ? "" : "/" + std::string(key));
    };
    shape(R"({"kind":"circle","r":1})");
    shape(R"({"r":1,"kind":"circle"})");
    shape(R"({})");
    shape(R"({"kind":"square","w":1})");
    shape(R"({"kind":"circle","r":1,"w":2})", "w");
    shape(R"({"w":2,"kind":"circle","r":1})", "w");
    shape(R"({"kind":"circle","r":1,"x":1})", "x");
    shape(R"({"kind":"square","r":1})", "r");
    shape(R"({"x":1})", "x");
    // in a subschema, the keys come before the values
    shape(R"({"kind":"circle","r":-1,"w":2})", "w");

    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"n":1})", "");
    expect_conditions<ClosedTagged>(R"({"n":1,"data":"a","kind":"s"})", "");
    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"n":1,"z":0})", closed("ClosedTagged", "z"), "/z");
    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"z":0})", "");

    expect_conditions<ClosedAlternatives>(R"({"type":"c","a":1})", "");
    expect_conditions<ClosedAlternatives>(R"({"a":1,"type":"c"})", "");
    expect_conditions<ClosedAlternatives>(R"({"type":"c","a":1,"b":2})", closed("ClosedAlternative", "b"), "/b");

    // merge_into checks the keys of the object after the merge
    auto merged = ConditionClosed{"circle", 1.0, {}};
    yyjson::merge_into(R"({"kind":"circle","r":2})", merged);
    EXPECT_EQ(merged.r, 2.0);
    // the key "kind" stays, so yyjson::then applies
    yyjson::merge_into(R"({"r":3})", merged);
    EXPECT_EQ(merged.r, 3.0);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"w":1})", merged); }).path(), "/w");
    // without the key "kind", yyjson::else_ applies, and the error is at the object, as the patch has no key "r"
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":null})", merged); });
    EXPECT_TRUE(message_of(e).starts_with(closed("ConditionClosed", "r"))) << e.what();
    EXPECT_EQ(e.path(), "");
    merged = ConditionClosed{"circle", 1.0, {}};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":"circle","r":1,"w":1})", merged); });
    EXPECT_EQ(e.path(), "/w");
}

TEST(Constraints, CountedConditions)
{
    const auto fewer = "ConditionCounted has fewer properties than the minimum 3 of yyjson::if_then_else";
    const auto more = "ConditionCounted has more properties than the maximum 2 of yyjson::if_then_else";
    expect_conditions<ConditionCounted>(R"({})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"pair","a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(R"({"x":2,"kind":"pair","a":1,"y":3})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"one"})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"pair","a":1})", fewer);
    expect_conditions<ConditionCounted>(R"({"kind":"one","a":1,"x":2})", more);
    // the keys of no field count too
    expect_conditions<ConditionCounted>(R"({"x":1,"y":2,"z":3})", more);
    expect_conditions<ConditionCounted>(R"({"a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"one","x":2})",
                                        R"(ConditionCounted is not constructible from JSON without the key "a", )"
                                        R"(which yyjson::if_then_else requires)");
    expect_conditions<ConditionCounted>(R"({"kind":"pair","a":1,"x":2,"y":3,"z":4})",
                                        "ConditionCounted has the keys and the values that yyjson::not_ rejects");
    // a null is a key of the object that a read counts, and no key of a patch
    expect_conditions<ConditionCounted>(R"({"kind":"one","a":null,"x":2})", more, "", false);
    auto merged = ConditionCounted{};
    yyjson::merge_into(R"({"kind":"one","a":null})", merged);
    EXPECT_EQ(merged.kind, "one");

    expect_conditions<CountedAlternatives>(R"({"type":"c","b":1})", "");
    expect_conditions<CountedAlternatives>(R"({"type":"c"})", "");
    expect_conditions<CountedAlternatives>(R"({"type":"c","a":1})",
                                           R"(CountedAlternative is not constructible from JSON without the key )"
                                           R"("b", which yyjson::if_then_else requires)");
    EXPECT_NO_THROW((void)yyjson::read<CountedAlternative>(R"({"a":1})"));

    expect_conditions<ConditionNamed>(R"({"kind":"short","a":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"long","alpha":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"short","alpha":1})",
                                      R"(a key of ConditionNamed is longer than the maximum length 4 in )"
                                      R"(yyjson::property_names of yyjson::if_then_else)",
                                      "/alpha");
}

TEST(Constraints, GroupedConditions)
{
    const auto without = [](std::string_view groups) {
        return "ConditionGrouped is not constructible from JSON without all the keys of one of " + std::string(groups);
    };
    // in the order of the fields, and out of it
    expect_conditions<ConditionGrouped>(R"({"kind":"pair","a":1})", "");
    expect_conditions<ConditionGrouped>(R"({"b":1,"kind":"pair"})", "");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","a":1,"b":2})",
        R"(ConditionGrouped is not constructible from JSON with all the keys of more than one of ["a"], ["b"])");
    expect_conditions<ConditionGrouped>(R"({"kind":"pair","c":1})", without(R"(["a"], ["b"])"));
    // the values of the fields come first
    expect_conditions<ConditionGrouped>(R"({"kind":"pair","c":10})",
                                        R"(the value of the key "c" of ConditionGrouped is greater than)", "/c");
    expect_conditions<ConditionGrouped>(R"({"a":1,"c":2})", "");
    expect_conditions<ConditionGrouped>(R"({"c":2,"b":1})", "");
    expect_conditions<ConditionGrouped>(R"({"a":1})", without(R"(["a", "c"], ["b"])"));
    expect_conditions<ConditionGrouped>(R"({})", without(R"(["a", "c"], ["b"])"));
    // the number of the keys comes before the groups
    expect_conditions<ConditionGrouped>(R"({"kind":"one","x":1,"y":2})",
                                        "ConditionGrouped has more properties than the maximum 2 of "
                                        "yyjson::if_then_else");
    expect_conditions<ConditionGrouped>(R"({"kind":"one","x":1})", without(R"(["a", "c"], ["b"])"));
    expect_conditions<ConditionGrouped>(R"({"kind":"one","b":1,"y":2})",
                                        "ConditionGrouped has more properties than the maximum 2 of "
                                        "yyjson::if_then_else");

    expect_conditions<GroupedAlternatives>(R"({"type":"g"})", "");
    expect_conditions<GroupedAlternatives>(R"({"type":"g","b":1})", "");
    expect_conditions<GroupedAlternatives>(R"({"a":1,"type":"g","b":1})", "");
}

TEST(Constraints, ObjectKeywordsAtPositions)
{
    const auto value = [](std::string_view key) {
        return R"(the value of the key ")" + std::string(key) + R"(" of Positions )";
    };
    const auto element = [](std::string_view key) {
        return R"(an element of the value of the key ")" + std::string(key) + R"(" of Positions )";
    };
    const auto closed = "has a key that yyjson::additional_properties(false) rejects";
    const auto fields = "does not have the keys and the values that yyjson::property and required give";
    expect_conditions<Positions>(
        R"({"field":{"a":1},"list":[{"a":1},{}],"map":{"k":{"a":1}},"alternative":{"type":"p","a":1},)"
        R"("groups":{"a":1},"conditional":{"a":1}})",
        "");
    expect_conditions<Positions>(R"({"field":{}})", value("field") + "has fewer properties than the minimum 1",
                                 "/field");
    expect_conditions<Positions>(R"({"field":{"b":1}})", value("field") + closed, "/field");
    expect_conditions<Positions>(R"({"field":{"a":1,"z":1}})", value("field") + closed, "/field");
    // the empty key is not a tag
    expect_conditions<Positions>(R"({"field":{"a":1,"":1}})", value("field") + closed, "/field");
    expect_conditions<Positions>(R"({"field":{"a":9}})", value("field") + fields, "/field");
    // a null in a field that cannot hold null is no key
    expect_conditions<Positions>(R"({"field":{"a":null,"b":null}})", value("field") + closed, "/field");

    expect_conditions<Positions>(R"({"list":[{"a":1},{"a":1,"b":2}]})",
                                 element("list") + "has more properties than the maximum 1", "/list/1");
    expect_conditions<Positions>(
        R"({"list":[{"zz":1}]})",
        element("list") + "has a key that does not satisfy the constraints of yyjson::property_names", "/list/0");
    expect_conditions<Positions>(R"({"map":{"k":{"a":1,"b":2}}})",
                                 element("map") + "satisfies the settings of yyjson::not_", "/map/k");

    // the internal tag is a key that the alternative has
    expect_conditions<Positions>(R"({"alternative":{"type":"o","x":1}})", value("alternative") + closed,
                                 "/alternative");
    expect_conditions<Positions>(R"({"alternative":{"a":-1,"type":"p"}})", value("alternative") + fields,
                                 "/alternative");
    expect_conditions<Positions>(R"({"alternative":{"type":"o","a":2}})", "");

    expect_conditions<Positions>(R"({"groups":{"b":1}})", "");
    expect_conditions<Positions>(
        R"({"groups":{"a":1,"b":1}})",
        value("groups") + "has all the keys that more than one subschema of yyjson::one_of requires", "/groups");
    expect_conditions<Positions>(R"({"groups":{}})",
                                 value("groups") +
                                     "does not have all the keys that one subschema of yyjson::one_of or any_of "
                                     "requires",
                                 "/groups");
    expect_conditions<Positions>(
        R"({"conditional":{"a":1,"b":1}})",
        value("conditional") + "satisfies the settings of yyjson::if_ and not those of yyjson::then", "/conditional");
    expect_conditions<Positions>(
        R"({"conditional":{"b":1}})",
        value("conditional") + "satisfies neither the settings of yyjson::if_ nor those of yyjson::else_",
        "/conditional");
    expect_conditions<Positions>(R"({"conditional":{}})", "");

    // merge_into checks the keys of the patch, with the values of the merged struct
    auto merged = Positions{};
    merged.field = Positioned{1, {}};
    yyjson::merge_into(R"({"field":{"a":2}})", merged);
    EXPECT_EQ(merged.field->a, 2);
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"field":{"b":1}})", merged); });
    EXPECT_TRUE(message_of(e).starts_with(value("field") + closed)) << e.what();
    merged.groups = Positioned{1, {}};
    yyjson::merge_into(R"({"groups":{"a":null,"b":1}})", merged);
    EXPECT_EQ(merged.groups->b, 1);
}

TEST(Constraints, DomConditions)
{
    const auto v = R"(the value of the key "v" of DomConditions )";
    expect_conditions<DomConditions>(R"({"v":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{"good":1},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{"bad":null},"e":[]})", v, "/v");
    // a value that is not an object has every key that yyjson::required names, as in JSON Schema
    expect_conditions<DomConditions>(R"({"v":1,"e":[]})", v, "/v");
    const auto o = R"(the value of the key "o" of DomConditions )";
    expect_conditions<DomConditions>(R"({"v":{},"o":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"y"},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":1},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x"},"e":[]})", o, "/o");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":0},"e":[]})", o, "/o");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":"1"},"e":[]})", "");
    const auto e = R"(an element of the value of the key "e" of DomConditions does not have the keys and the values )"
                   R"(that yyjson::property and required give)";
    expect_conditions<DomConditions>(R"({"v":{},"e":[{"id":0},1,"a",null,[]]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"e":[{"id":0},{}]})", e, "/e/1");
    expect_conditions<DomConditions>(R"({"v":{},"e":[{"id":-1}]})", e, "/e/0");
    expect_conditions<DomConditions>(R"({"v":{},"e":[],"m":{"a":{"id":1},"b":2}})", "");
    expect_conditions<DomConditions>(R"({"v":{},"e":[],"m":{"a":{"id":1},"b":{}}})",
                                     R"(an element of the value of the key "m" of DomConditions )", "/m/b");
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
    const auto key = [](std::string_view k, std::string_view text) {
        return std::format(R"(the value of the key "{}" of MultiKind {})", k, text);
    };
    expect_conditions<MultiKind>(json(R"("v":2)"), key("v", "is not one of"), "/v");
    expect_conditions<MultiKind>(json(R"("v":[])"), key("v", "is not one of"), "/v");
    expect_conditions<MultiKind>(json(R"("w":-1)"), key("w", "is less than"), "/w");
    expect_conditions<MultiKind>(json(R"("w":"abc")"), key("w", "is longer"), "/w");
    expect_conditions<MultiKind>(json(R"("s":"light")"), key("s", "is not one of"), "/s");
    expect_conditions<MultiKind>(json(R"("s":4)"), key("s", "is not one of"), "/s");
    expect_conditions<MultiKind>(json(R"("list":[1])"), key("list", "has fewer"), "/list");
    expect_conditions<MultiKind>(json(R"("list":"b")"), key("list", "does not match"), "/list");
    expect_conditions<MultiKind>(json(R"("list":{"a":1,"b":2})"), key("list", "has more"), "/list");
    expect_conditions<MultiKind>(json(R"("list":[0,-1])"), R"(an element of the value of the key "list")", "/list/1");
    expect_conditions<MultiKind>(json(R"("elements":[1,2])"), R"(an element of the value of the key "elements")",
                                 "/elements/1");
    expect_conditions<MultiKind>(json(R"("elements":[""])"), R"(an element of the value of the key "elements")",
                                 "/elements/0");
    expect_conditions<MultiKind>(json(R"("c":[1,2])"), key("c", "has more"), "/c");
    expect_conditions<MultiKind>(json(R"("c":{})"), key("c", "has fewer"), "/c");
    expect_conditions<MultiKind>(json(R"("u":1)"), key("u", "satisfies"), "/u");
    expect_conditions<MultiKind>(json(R"("u":2)"), key("u", "is not one of"), "/u");
    expect_conditions<MultiKind>(json(R"("u":"b")"), key("u", "is not one of"), "/u");
}

TEST(Constraints, MultiKindWrittenForms)
{
    // the reader without a document counts the elements of the JSON array while it reads them
    expect_accepted<MultiKindWritten>(R"({"p":"x","q":[5]})", {"x", {5, {}, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,2],"q":[1,5]})", {MultiKindPair{1, 2, {}}, {1, 5, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,4,3],"q":[5,6]})", {MultiKindPair{1, 4, {3}}, {5, 6, {}}});
    // null at the position of a std::optional is an error, also in an alternative of a variant
    expect_rejected<MultiKindWritten>(R"({"p":"x","q":[5,null]})",
                                      "std::optional<int> is not constructible from JSON null", "/q/1");
    expect_rejected<MultiKindWritten>(R"({"p":[1,null],"q":[5]})", "std::variant<MultiKindPair, ", "/p");
    const auto message = [](std::string_view key, std::string_view text) {
        return std::format(R"(the value of the key "{}" of MultiKindWritten {})", key, text);
    };
    expect_rejected<MultiKindWritten>(R"({"p":[1],"q":[5]})", message("p", "has fewer"), "/p");
    expect_rejected<MultiKindWritten>(R"({"p":[1,2,3,4],"q":[5]})", message("p", "has more"), "/p");
    expect_rejected<MultiKindWritten>(R"({"p":"x","q":[5,1,2]})", message("q", "has more"), "/q");
    expect_rejected<MultiKindWritten>(R"({"p":"x","q":[1,2]})", message("q", "has no item"), "/q");
}

namespace
{
    // every read of `json` under the policy, through a document and without one, throws `message` at `path`
    template <typename T, typename P>
    void rejected_by(P policy, std::string_view json, std::string_view message, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
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
    expect_rejected<PatternMaps>(R"({"ax":"x"})", "int is not constructible from JSON string", "/ax");
    expect_rejected<PatternMaps>(R"({"xb":9})",
                                 R"(the value of the key "xb" of PatternMaps is less than the minimum 10)", "/xb");
    expect_rejected<PatternMaps>(R"({"aaaa":1})", "a key of PatternMaps is longer than the maximum length 3", "/aaaa");
    // a key that no pattern matches is unknown
    expect_accepted<PatternMaps>(R"({"zz":1,"ax":1})", {10, {{"ax", 1}}, {}, {}});
    rejected_by<PatternMaps>(yyjson::reject_unknown_keys, R"({"ax":1,"zz":1})",
                             R"(PatternMaps has no field of the key "zz")", "/zz");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ab":11,"ax":1,"sv":"t"})", yyjson::reject_unknown_keys),
              (PatternMaps{11, {{"ax", 1}}, {}, {{"sv", "t"}}}));
    rejected_by<PatternMaps>(yyjson::reject_duplicates, R"({"ax":1,"ax":2})",
                             R"(the object read into PatternMaps has the key "ax" twice)", "/ax");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ax":1,"ax":2})"), (PatternMaps{10, {{"ax", 2}}, {}, {}}));
    EXPECT_EQ(yyjson::read<PatternOpen>(R"({"a":1,"p1":3,"zz":1})"), (PatternOpen{1, {{"p1", 3}}}));

    // every later pattern that matches a key checks its value, also when merge_into merges it
    expect_rejected<PatternMaps>(R"({"axb":9})",
                                 R"(the value of the key "axb" of PatternMaps is less than the minimum 10)", "/axb");
    expect_rejected<PatternMaps>(R"({"sb":"x"})", "int is not constructible from JSON string", "/sb");
    expect_accepted<PatternMaps>(R"({"axb":10})", {10, {{"axb", 10}}, {}, {}});
    // and so does every pattern that matches the key of a field
    expect_rejected<PatternMaps>(R"({"ab":9})",
                                 R"(the value of the key "ab" of PatternMaps is less than the minimum 10)", "/ab");
    expect_accepted<PatternMaps>(R"({"ab":12,"ax":1})", {12, {{"ax", 1}}, {}, {}}, false);
    // but not a null that reads as a missing field
    expect_accepted<PatternOptional>(R"({"ab":null,"ax":10})", {{}, {{"ax", 10}}, {}}, false);
    expect_accepted<PatternOptional>(R"({"ab":10})", {10, {}, {}}, false);
    expect_rejected<PatternOptional>(
        R"({"ab":5})", R"(the value of the key "ab" of PatternOptional is less than the minimum 10)", "/ab");
    // a null is an error for a map whose values do not hold null, and merge_into erases the key
    expect_rejected<PatternOptional>(R"({"ax":null})", "int is not constructible from JSON null", "/ax", false);
    EXPECT_EQ(yyjson::read<PatternOptional>(R"({"nx":null})"), (PatternOptional{{}, {}, {{"nx", std::nullopt}}}));
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_TRUE(read_directly<PatternOptional>(R"({"nx":null})", yyjson::ReadFlag::NoFlag).has_value());
#endif

    // the keys that no pattern matches go to the field of `yyjson::extra_keys`
    expect_accepted<PatternRest>(R"({"ax":1,"z":true})", {{{"ax", 1}}, {{"z", true}}});
    EXPECT_EQ(yyjson::read<PatternRest>(R"({"ax":1,"z":true})", yyjson::reject_unknown_keys),
              (PatternRest{{{"ax", 1}}, {{"z", true}}}));
    expect_rejected<PatternRest>(R"({"ax":true})", "int is not constructible from JSON bool", "/ax");
    expect_rejected<PatternRest>(R"({"z":1})", "bool is not constructible from JSON integer", "/z");
    // a closed struct takes the keys that a pattern matches
    expect_accepted<PatternClosed>(R"({"x1":[1]})", {{{"x1", yyjson::read("[1]")}}});
    expect_rejected<PatternClosed>(
        R"({"x1":1,"y":1})", R"(PatternClosed has the key "y", which yyjson::additional_properties(false) rejects)",
        "/y");

    // the map starts from its value in `T()`
    auto seeded = PatternSeeded{5, {{"old", 3}}};
    yyjson::read_into(R"({"ax":2})", seeded);
    EXPECT_EQ(seeded, (PatternSeeded{0, {{"aseed", 1}, {"ax", 2}}}));
    expect_accepted<PatternSeeded>(R"({"aseed":1,"x":1})", {1, {{"aseed", 1}}});

    // the keywords of the struct count and check the keys that the maps receive
    expect_accepted<PatternCounted>(R"({"x":1,"ab":2})", {1, {{"ab", 2}}});
    expect_rejected<PatternCounted>(R"({"x":1})", "PatternCounted has fewer properties than the minimum 2", "");
    expect_rejected<PatternCounted>(R"({"a1":1,"a2":2,"a3":3,"x":1})",
                                    "PatternCounted has more properties than the maximum 3", "");
    expect_rejected<PatternCounted>(R"({"x":1,"abcd":1})",
                                    "a key of PatternCounted is longer than the maximum length 3", "/abcd");
    // additional_properties(false) in a subschema rejects them
    expect_accepted<PatternConditional>(R"({"kind":"open","p1":2})", {"open", {{"p1", 2}}});
    expect_accepted<PatternConditional>(R"({"kind":"closed"})", {"closed", {}});
    expect_rejected<PatternConditional>(
        R"({"kind":"closed","p1":2})",
        R"(PatternConditional has the key "p1", which yyjson::additional_properties(false) of yyjson::if_then_else rejects)",
        "/p1");

    // the value of a tag whose key a pattern matches satisfies that pattern
    expect_accepted<PatternTagged>(R"({"kind":"s","x":1})", PatternTagShort{1, {}}, false);
    expect_accepted<PatternTagged>(R"({"x":1,"kind":"s","k1":"v"})", PatternTagShort{1, {{"k1", "v"}}}, false);
    expect_accepted<PatternTagged>(R"({"kind":"longer","y":1})", PatternTagOther{1});
    expect_rejected<PatternTaggedLong>(
        R"({"kind":"st","x":1})",
        R"(the value of the key "kind" of PatternTagShort is longer than the maximum length 1)", "/kind");
    expect_rejected<PatternTaggedShared>(R"({"kind":"i","x":1})", "int is not constructible from JSON string", "/kind");
    for (const auto json : {R"({"kind":"i","data":1})", R"({"data":1,"kind":"i"})"})
        expect_rejected<PatternAdjacent>(json, "int is not constructible from JSON string", "/kind");

    // the struct inside other types
    expect_accepted<PatternHolder>(
        R"({"v":[{"ax":1}],"m":{"k":{"xb":10}},"o":{"sv":"t"}})",
        {{{10, {{"ax", 1}}, {}, {}}}, {{"k", {10, {}, {{"xb", 10}}, {}}}}, PatternMaps{10, {}, {}, {{"sv", "t"}}}});
    expect_rejected<PatternHolder>(
        R"({"v":[{},{"xb":9}]})", R"(the value of the key "xb" of PatternMaps is less than the minimum 10)", "/v/1/xb");
    expect_rejected<PatternHolder>(R"({"m":{"k":{"axb":9}}})",
                                   R"(the value of the key "axb" of PatternMaps is less than the minimum 10)",
                                   "/m/k/axb");
    expect_rejected<PatternHolder>(R"({"o":{"aaaa":1}})", "a key of PatternMaps is longer than the maximum length 3",
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
    const auto rejected = [](std::string_view json, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of([&] {
            auto t = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}};
            yyjson::merge_into(json, t);
        });
        EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto t = PatternMaps{12, {{"ax", 1}, {"axb", 20}}, {{"xb", 10}}, {}};
    // a key of the patch is added or merged into the map of the first pattern, and null erases it
    EXPECT_EQ(merged(t, R"({"ax":null,"ay":2,"xb":11,"sv":"t"})"),
              (PatternMaps{12, {{"axb", 20}, {"ay", 2}}, {{"xb", 11}}, {{"sv", "t"}}}));
    EXPECT_EQ(merged(t, R"({"axb":null,"zz":1})"), (PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}}));
    EXPECT_EQ(merged(t, R"({"axb":30,"ab":11})"), (PatternMaps{11, {{"ax", 1}, {"axb", 30}}, {{"xb", 10}}, {}}));
    // every pattern that matches a key of the patch checks the merged value
    rejected(R"({"axb":5})", R"(the value of the key "axb" of PatternMaps is less than the minimum 10)", "/axb");
    rejected(R"({"ab":5})", R"(the value of the key "ab" of PatternMaps is less than the minimum 10)", "/ab");
    rejected(R"({"sb":"x"})", "int is not constructible from JSON string", "/sb");
    rejected(R"({"ax":"x"})", "int is not constructible from JSON string", "/ax");
    rejected(R"({"aaaa":1})", "a key of PatternMaps is longer than the maximum length 3", "/aaaa");
    // a closed struct rejects a key that no pattern matches
    auto closed = PatternClosed{{{"x1", yyjson::read("1")}}};
    yyjson::merge_into(R"({"x1":null,"x2":2})", closed);
    EXPECT_EQ(closed, (PatternClosed{{{"x2", yyjson::read("2")}}}));
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"y":1})", closed); });
    EXPECT_TRUE(message_of(e).starts_with(
        R"(PatternClosed has the key "y", which yyjson::additional_properties(false) rejects)"))
        << e.what();
    // the merged values of a field and of a map
    auto optional = PatternOptional{12, {{"ax", 10}}, {}};
    yyjson::merge_into(R"({"ab":null,"ax":11,"nx":null})", optional);
    EXPECT_EQ(optional, (PatternOptional{{}, {{"ax", 11}}, {}}));
    auto rest = PatternRest{{{"ax", 1}}, {{"z", true}}};
    yyjson::merge_into(R"({"ay":2,"z":null,"w":false})", rest);
    EXPECT_EQ(rest, (PatternRest{{{"ax", 1}, {"ay", 2}}, {{"w", false}}}));
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
    // the message of a value that no alternative of V reads
    template <typename V>
    std::string read_by_none()
    {
        return std::format("{} is not constructible from JSON", yyjson::detail::variant_name<V>());
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
    for (const auto json : {"5", "150", R"("ABC")", R"("abcdefghi")", "[5,6]", "1.5", "null"})
        expect_rejected<UnionValues>(json, read_by_none<UnionValues>(), "");
    expect_rejected<UnionFields>(R"({"v":5})", read_by_none<UnionValues>(), "/v");

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
    expect_rejected<Shapes>(R"({"kind":"hexagon"})", read_by_none<Shapes>(), "");
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
    expect_rejected<UnionTagged>(R"({"kind":"p","x":1,"z":1})",
                                 read_by_none<std::variant_alternative_t<0, UnionTagged>>(), "");

    // the settings of an alternative reach the value that a variant held in a variant holds, at any depth
    using IntOrText = std::variant<int, std::string>;
    using Nested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<IntOrText, double>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    for (const auto json : {"1", "1.5", R"("a")"}) expect_rejected<Nested>(json, read_by_none<Nested>(), "");
    for (const auto json : {"7", "7.5", R"("ab")", "true"}) EXPECT_NO_THROW((void)yyjson::read<Nested>(json)) << json;
    using NestedListed = yyjson::any_of_variant<yyjson::alt<std::variant<IntOrText, bool>, yyjson::enum_(5, "ab")>>;
    expect_alternative<NestedListed>("5", 0);
    expect_alternative<NestedListed>(R"("ab")", 0);
    expect_rejected<NestedListed>("1", read_by_none<NestedListed>(), "");
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
    void expect_rejected_under(std::string_view json, std::string_view message, Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
        };
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
    // both match an object of both keys, since each one takes other keys, and the policies of the call do not
    // change the match
    expect_rejected<XOrZ>(R"({"x":1,"z":2,"w":0})", read_by_two<XOrZ>(0, 1), "");
    expect_rejected_under<XOrZ>(R"({"x":1,"z":2})", read_by_two<XOrZ>(0, 1));
    expect_rejected_under<XOrZ>(R"({"x":1,"z":2})", read_by_two<XOrZ>(0, 1), yyjson::reject_unknown_keys);
    // an alternative may match with none of its keys
    using MaybeXOrY = yyjson::one_of_variant<OneOfMaybeX, OneOfY>;
    expect_rejected<MaybeXOrY>(R"({"y":1})", read_by_two<MaybeXOrY>(0, 1), "");
    expect_accepted<MaybeXOrY>(R"({"x":1})", MaybeXOrY(OneOfMaybeX{1}));
    expect_accepted<MaybeXOrY>("{}", MaybeXOrY(OneOfMaybeX{}));
    // a closed struct does not match an object of other keys
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_accepted<ClosedOrXZ>(R"({"x":1,"z":2})", ClosedOrXZ(OneOfXZ{1, 2}));
    expect_accepted<ClosedOrXZ>(R"({"x":1})", ClosedOrXZ(OneOfClosedX{1}));
    expect_rejected<ClosedOrXZ>(R"({"z":1})", read_by_none<ClosedOrXZ>(), "");

    // the policies of the call apply to the alternative that matches
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_alternative<XOrText>(R"({"x":1,"k":2})", 0);
    EXPECT_THROW((void)yyjson::read<XOrText>(R"({"x":1,"k":2})", yyjson::reject_unknown_keys), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"x":1,"k":2})").cast<XOrText>(yyjson::reject_unknown_keys), yyjson::bad_cast);
    expect_alternative<XOrText>(R"({"x":1})", 0, yyjson::reject_unknown_keys);
    expect_alternative<XOrText>(R"("a")", 1, yyjson::reject_unknown_keys);
    // a set does not match a repeated element, which `uniqueItems` rejects, under any policies
    using SetOrList = yyjson::one_of_variant<std::set<int>, std::vector<int>>;
    expect_alternative<SetOrList>("[1,1]", 1);
    expect_alternative<SetOrList>("[1,1]", 1, yyjson::reject_duplicates);
    expect_rejected_under<SetOrList>("[1,2]", read_by_two<SetOrList>(0, 1));
    using CountedSet = yyjson::one_of_variant<yyjson::alt<std::set<int>, yyjson::min_items(1)>,
                                              yyjson::alt<std::vector<int>, yyjson::max_items(2)>>;
    expect_alternative<CountedSet>("[1,1]", 1, yyjson::strict);
    // a set of another order has no `uniqueItems`, and no schema rejects a repeated key
    using OrderedOrList = yyjson::one_of_variant<std::set<int, OneOfOrder>, std::vector<int>>;
    expect_rejected_under<OrderedOrList>("[1,1]", read_by_two<OrderedOrList>(0, 1));
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
    expect_rejected<OneOfInts>("5", read_by_two<OneOfInts>(0, 1), "");
    expect_rejected<OneOfInts>("1.5", read_by_none<OneOfInts>(), "");
    expect_rejected<OneOfFields>(R"({"n":5,"p":"a"})", read_by_two<OneOfInts>(0, 1), "/n");

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

namespace
{
    // the reason why an alternative does not read a value, about the value at `at` within it
    struct alternative_reason
    {
        alternative_reason(const char* r, std::string_view a = {}) : reason(r), at(a) {}         // NOLINT
        alternative_reason(const std::string& r, std::string_view a = {}) : reason(r), at(a) {}  // NOLINT
        std::string_view reason;
        std::string_view at;
    };

    // the message of a value that no alternative of V reads, with the reason of each alternative
    template <typename V>
    std::string read_by_none_because(std::initializer_list<alternative_reason> reasons)
    {
        const auto names = []<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array{yyjson::detail::variant_name<std::variant_alternative_t<Is, V>>()...};
        }(std::make_index_sequence<std::variant_size_v<V>>());
        auto message = read_by_none<V>();
        for (auto i = std::size_t(0); const auto& r : reasons)
        {
            message += std::format("{} alternative {} ({}){}: {}", i == 0 ? ":" : ";", i, names[i],
                                   r.at.empty() ? std::string() : " at " + std::string(r.at), r.reason);
            ++i;
        }
        return message;
    }

    // reading `json` into T under `policies` throws `bad_cast` with exactly `message` about the value at `path`,
    // with and without a document, and from a mutable document
    template <typename T, typename... Policies>
    void expect_reasons(std::string_view json, std::string_view message, std::string_view path, Policies... policies)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_EQ(std::string(e.std::runtime_error::what()), message) << json;
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
        "yyjson::one_of_variant<int, bool> is not constructible from JSON: alternative 0 (int): the value is "
        "less than the minimum 1; alternative 1 (bool): reads no JSON number",
        "");
    // a union inside a union has the same name in the label of its alternative and in its reason
    expect_reasons<yyjson::any_of_variant<yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, bool>, bool>>(
        "0",
        "yyjson::any_of_variant<yyjson::any_of_variant<int, bool>, bool> is not constructible from JSON: alternative 0 "
        "(yyjson::any_of_variant<int, bool>): yyjson::any_of_variant<int, bool> is not constructible from JSON; "
        "alternative 1 (bool): reads no JSON number",
        "");
    expect_reasons<std::variant<std::variant<int, bool>, bool>>(
        "null",
        "std::variant<std::variant<int, bool>, bool> is not constructible from JSON: alternative 0 "
        "(std::variant<int, bool>): reads no JSON null; alternative 1 (bool): reads no JSON null",
        "");

    // each alternative that does not read the kind of the value
    using Kinds = std::variant<int, std::string, UnionPoint>;
    const auto no_bool = "reads no JSON bool";
    expect_reasons<Kinds>("true", read_by_none_because<Kinds>({no_bool, no_bool, no_bool}), "");
    const auto no_null = "reads no JSON null";
    expect_reasons<std::vector<Kinds>>(R"([1,"a",null])", read_by_none_because<Kinds>({no_null, no_null, no_null}),
                                       "/2");
    // the reason of a struct gives the path of the value within it, and the path of the variant ends the message once
    using IntOrPoint = std::variant<int, UnionPoint>;
    const auto point_reasons =
        read_by_none_because<IntOrPoint>({"reads no JSON object", {"int is not constructible from JSON string", "/x"}});
    expect_reasons<IntOrPoint>(R"({"x":"a"})", point_reasons, "");
    expect_reasons<UnionReasonField>(R"({"v":{"x":"a"}})", point_reasons, "/v");
    try
    {
        (void)yyjson::read<UnionReasonField>(R"({"v":{"x":"a"}})");
        ADD_FAILURE();
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(std::string(e.what()), point_reasons + " at /v (line 1, column 6, pos 5)");
    }

    // the setting of an alternative that the value does not satisfy
    using NonNegative = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, std::string>;
    expect_reasons<NonNegative>(
        "-1", read_by_none_because<NonNegative>({"the value is less than the minimum 0", "reads no JSON number"}), "");

    // the alternatives of yyjson::one_of_variant under the policies that count the matches, the one that the keys
    // of the object prove to fail too
    using ClosedOrXZ = yyjson::one_of_variant<OneOfClosedX, OneOfXZ>;
    expect_reasons<ClosedOrXZ>(
        R"({"z":1})",
        read_by_none_because<ClosedOrXZ>(
            {{R"(OneOfClosedX has the key "z", which yyjson::additional_properties(false) rejects)", "/z"},
             R"(OneOfXZ is not constructible from JSON without the key "x")"}),
        "");
    // the one that matches under those policies and fails under the policies of the call gives its own failure
    using XOrText = yyjson::one_of_variant<OneOfX, std::string>;
    expect_reasons<XOrText>(R"({"x":1,"k":2})", R"(OneOfX has no field of the key "k")", "/k",
                            yyjson::reject_unknown_keys);
    // also a variant within it, which gives the reasons of its alternatives
    using IntOrX = std::variant<int, OneOfX>;
    expect_reasons<UnionReasonNode>(
        R"({"v":1,"next":{"v":{"x":1,"k":2},"next":null}})",
        read_by_none_because<IntOrX>({"reads no JSON object", {R"(OneOfX has no field of the key "k")", "/k"}}),
        "/next/v", yyjson::reject_unknown_keys);

    // the path within an alternative goes through its members and elements, and a failure of the whole value has none
    using IntOrLine = std::variant<int, UnionReasonLine>;
    expect_reasons<IntOrLine>(R"({"from":{"x":1,"y":2},"to":[{"x":3,"y":4},{"x":5,"y":true}]})",
                              read_by_none_because<IntOrLine>(
                                  {"reads no JSON object", {"int is not constructible from JSON bool", "/to/1/y"}}),
                              "");
    expect_reasons<IntOrLine>("[]", read_by_none_because<IntOrLine>({"reads no JSON array", "reads no JSON array"}),
                              "");
    using IntOrMap = std::variant<int, std::map<std::string, int>>;
    expect_reasons<IntOrMap>(R"({"a/b~c":"x"})",
                             read_by_none_because<IntOrMap>(
                                 {"reads no JSON object", {"int is not constructible from JSON string", "/a~1b~0c"}}),
                             "");

    // a variant within an alternative gives no reasons of its own alternatives, which bounds the work
    using Nested = std::variant<int, std::variant<std::string, std::variant<bool, UnionPoint>>>;
    using Inner = std::variant_alternative_t<1, Nested>;
    expect_reasons<Nested>(R"({"x":"a"})",
                           read_by_none_because<Nested>({"reads no JSON object", read_by_none<Inner>()}), "");
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
    expect_rejected<Wide>("1", read_by_two<Wide>(0, 1), "");
    using Steps =
        yyjson::one_of_variant<yyjson::alt<int, yyjson::multiple_of(2)>, yyjson::alt<int, yyjson::multiple_of(3)>>;
    static_assert(one_of_overlapping<Steps, 0, 1> && !one_of_contained<Steps>);
    expect_accepted<Steps>("4", Steps(std::in_place_index<0>, 4), false);
    expect_rejected<Steps>("6", read_by_two<Steps>(0, 1), "");

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
    expect_rejected_under<OtherOrString>(R"("a")", read_by_two<OtherOrString>(0, 1));
    expect_alternative<std::variant<OneOfOtherCode, std::string>>(R"("a")", 0);
    // such a caster of a range or a map reads values that the elements or the values cannot
    expect_alternative<std::variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<std::variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeRange, std::vector<std::string>>>(R"(["x"])", 0);
    expect_alternative<yyjson::any_of_variant<OtherCodeMap, std::map<std::string, std::string>>>(R"({"a":"x"})", 0);

    // a struct reads each object of another one that requires each key that it requires and takes no key that it
    // does not name, with values that it reads
    using XAndXZ = yyjson::detail::one_of_relations<yyjson::one_of_variant<OneOfX, OneOfXZ>>;
    static_assert(XAndXZ::contained.contained == 1 && XAndXZ::contained.container == 0);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfClosedX>>);
    using ClosedByAlt =
        yyjson::alt<OneOfXZ, yyjson::property("x"), yyjson::property("z"), yyjson::additional_properties(false)>;
    static_assert(one_of_contained<yyjson::one_of_variant<ClosedByAlt, OneOfXZ>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, ClosedByAlt>>);
    static_assert(one_of_contained<
                  yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>, OneOfX>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfXMaybeY>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfXRest>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfX, OneOfWrapped<OneOfXZ>>>);
    static_assert(one_of_contained<yyjson::one_of_variant<OneOfWrapped<OneOfX>, OneOfXZ>>);

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
    expect_rejected<SignedOrUnsigned>(R"({"x":1})", read_by_two<SignedOrUnsigned>(0, 1), "");
    expect_alternative<SignedOrUnsigned>(R"({"x":-1})", 0);

    // the analysis does not prove that a struct reads each value at a key that it does not name
    using MaybeYOrZ = yyjson::one_of_variant<OneOfXMaybeY, OneOfXMaybeZ>;
    static_assert(!one_of_contained<MaybeYOrZ> && one_of_overlapping<MaybeYOrZ, 0, 1>);
    expect_rejected<MaybeYOrZ>(R"({"x":1})", read_by_two<MaybeYOrZ>(0, 1), "");
    expect_alternative<MaybeYOrZ>(R"({"x":1,"y":"a"})", 1);
    // nor what a struct reads with a setting other than the keys that it takes
    using MinimumX = yyjson::one_of_variant<yyjson::alt<OneOfX, yyjson::property("x", yyjson::minimum(0))>,
                                            yyjson::alt<OneOfXZ, yyjson::property("x", yyjson::minimum(0))>>;
    static_assert(!one_of_contained<MinimumX> && one_of_overlapping<MinimumX, 0, 1>);
    expect_rejected<MinimumX>(R"({"x":1,"z":2})", read_by_two<MinimumX>(0, 1), "");
    // nor what a struct with a map of other keys reads at those keys
    using XZOrAny = yyjson::one_of_variant<OneOfXZ, OneOfXAny>;
    static_assert(!one_of_contained<XZOrAny> && one_of_overlapping<XZOrAny, 0, 1>);
    expect_rejected<XZOrAny>(R"({"x":1,"z":2})", read_by_two<XZOrAny>(0, 1), "");
    // nor compares the keys of two structs beyond 256 pairs of them
    using Many = yyjson::one_of_variant<OneOfMany16, OneOfMany17>;
    static_assert(!one_of_contained<Many> && one_of_overlapping<Many, 0, 1>);

    // a field that is read otherwise than its type reads a value of its own may read any value: in place by a
    // caster of the user, by a caster of the user of a std::optional, or as the value of an adjacent tag
    using CodeOrText = yyjson::one_of_variant<OneOfCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<CodeOrText, 0, 1>);
    expect_rejected_under<CodeOrText>(R"({"x":"a"})", read_by_two<CodeOrText>(0, 1));
    // a caster of the user of the other document reads any value, also in a field
    using OtherCodeOrText = yyjson::one_of_variant<OneOfOtherCodeX, OneOfTextX>;
    static_assert(one_of_overlapping<OtherCodeOrText, 0, 1>);
    expect_rejected_under<OtherCodeOrText>(R"({"x":"a"})", read_by_two<OtherCodeOrText>(0, 1));
    using NumberOrText = yyjson::one_of_variant<OneOfMaybeNumberX, OneOfTextX>;
    static_assert(one_of_overlapping<NumberOrText, 0, 1>);
    expect_rejected_under<NumberOrText>(R"({"x":"a"})", read_by_two<NumberOrText>(0, 1));
    using AdjacentOrX = yyjson::one_of_variant<OneOfAdjacentX, OneOfX>;
    static_assert(one_of_overlapping<AdjacentOrX, 0, 1>);
    expect_rejected_under<AdjacentOrX>(R"({"kind":"i","x":1})", read_by_two<AdjacentOrX>(0, 1));
    // the key of the tag of an enclosing variant tells no closed struct apart, and a closed struct does not read
    // only what another one reads that names that key
    using XKind = std::variant_alternative_t<0, OneOfTaggedXKind>;
    static_assert(!one_of_overlapping<XKind, 0, 1> && yyjson::detail::one_of_variant_cast<XKind>::tagged_overlap[1][0]);
    expect_alternative<XKind>(R"({"x":1,"kind":"p"})", 1);
    expect_rejected_under<OneOfTaggedXKind>(R"({"kind":"p","x":1})", read_by_two<XKind>(0, 1));
    using MaybeKind = std::variant_alternative_t<0, OneOfTaggedMaybeKind>;
    static_assert(!one_of_contained<MaybeKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedMaybeKind>(R"({"kind":"p","x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","x":1})").cast<OneOfTaggedMaybeKind>()).index(), 0);
    // the closed struct also takes the key of the tag again with a value that the other one does not read
    using TextKind = std::variant_alternative_t<0, OneOfTaggedTextKind>;
    static_assert(!one_of_contained<TextKind>);
    EXPECT_EQ(std::get<0>(yyjson::read<OneOfTaggedTextKind>(R"({"kind":"p","kind":1,"x":1})")).index(), 0);
    EXPECT_EQ(std::get<0>(yyjson::read(R"({"kind":"p","kind":1,"x":1})").cast<OneOfTaggedTextKind>()).index(), 0);
    expect_rejected_under<OneOfTaggedTextKind>(R"({"kind":"p","x":1})", read_by_two<TextKind>(0, 1));
    // each transparent struct between an alternative and its keyed struct may have settings
    using LayeredOrXZ = yyjson::one_of_variant<OneOfWrapped<OneOfMidX>, OneOfXZ>;
    static_assert(!one_of_contained<LayeredOrXZ>);
    expect_alternative<LayeredOrXZ>(R"({"x":-1,"z":1})", 1);
    expect_rejected_under<LayeredOrXZ>(R"({"x":1,"z":1})", read_by_two<LayeredOrXZ>(0, 1));
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
        true);
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CasterFields )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of CasterFields )", key);
    };
    expect_rejected<CasterFields>(R"({"color":"green"})", value("color") + R"(is not one of "red", "blue")", "/color");
    expect_rejected<CasterFields>(R"({"name":"abcd"})", value("name") + "is longer than the maximum length 3", "/name");
    expect_rejected<CasterFields>(R"({"name":"Ab"})", value("name") + R"(does not match the pattern "^[a-z]")",
                                  "/name");
    expect_rejected<CasterFields>(R"({"label":"x"})", value("label") + "is shorter than the minimum length 2",
                                  "/label");
    expect_rejected<CasterFields>(R"({"real":-1})", value("real") + "is less than the minimum 0", "/real");
    expect_rejected<CasterFields>(R"({"real":1.5})", value("real") + "is not less than the exclusive maximum 1.5",
                                  "/real");
    expect_rejected<CasterFields>(R"({"small":11})", value("small") + "is greater than the maximum 10", "/small");
    expect_rejected<CasterFields>(R"({"flag":false})", value("flag") + "is not equal to the constant true", "/flag");
    expect_rejected<CasterFields>(R"({"list":[1,2,3]})", value("list") + "has more items than the maximum 2", "/list");
    expect_rejected<CasterFields>(R"({"list":[1,1]})", element("list") + "is equal to an earlier element", "/list/1");
    expect_rejected<CasterFields>(R"({"pos":{"x":2,"y":2}})",
                                  value("pos") + R"(is not equal to the constant {"x":1,"y":2})", "/pos");
    expect_rejected<CasterFields>(R"({"dom":[]})", value("dom") + "has fewer items than the minimum 1", "/dom");
    expect_rejected<CasterFields>(R"({"opt":"abc"})", value("opt") + "is longer than the maximum length 2", "/opt");
    expect_rejected<CasterFields>(R"({"nul":"abc"})", value("nul") + "is longer than the maximum length 2", "/nul");
    // a value that the keywords accept can still fail to convert
    EXPECT_THROW((void)yyjson::read<CasterFields>(R"({"small":1.5})"), yyjson::bad_cast);

    expect_accepted<CasterPairs>(R"({"pair":{"x":1,"y":2},"pairs":[{"y":3,"x":4}]})", {{1, 2}, {{4, 3}}});
    expect_rejected<CasterPairs>(R"({"pair":{"x":1,"y":2,"z":3}})", "CasterPair has more properties than the maximum 2",
                                 "/pair");
    expect_rejected<CasterPairs>(R"({"pairs":[{"x":1}]})", "CasterPair has fewer properties than the minimum 2",
                                 "/pairs/0");
}

TEST(Constraints, CasterJsonSameKind)
{
    // a keyword of the kind that the type writes checks the JSON value too, and not the sorted value
    const auto check = []<typename T>(std::type_identity<T>) {
        const auto value = std::format(R"(the value of the key "h" of {} )", yyjson::detail::type_name<T>());
        expect_accepted<T>(R"({"h":["a","b"]})", {CasterSorted({"a", "b"})});
        expect_rejected<T>(R"({"h":["b","a"]})", value + R"(is not one of ["a","b"])", "/h");
        expect_rejected<T>(R"({"h":["c"]})", value + R"(is not one of ["a","b"])", "/h");
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
    expect_accepted<T>(R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9\n"]})", {good});
    expect_accepted<T>(R"({"plain":["\u0061","b\u0063"],"quoted":["\u0061\"b","\u00e9\u000a"]})", {good});
    const auto plain = R"(the value of the key "plain" of CasterSortedText is not one of ["a","bc"])";
    const auto quoted =
        R"(the value of the key "quoted" of CasterSortedText is not equal to the constant ["a\"b","\u00e9\n"])";
    expect_rejected<T>(R"({"plain":["a","b"],"quoted":["a\"b","\u00e9\n"]})", plain, "/plain");
    expect_rejected<T>(R"({"plain":["a","bcd"],"quoted":["a\"b","\u00e9\n"]})", plain, "/plain");
    expect_rejected<T>(R"({"plain":["a","bc","d"],"quoted":["a\"b","\u00e9\n"]})", plain, "/plain");
    expect_rejected<T>(R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9"]})", quoted, "/quoted");
    expect_rejected<T>(R"({"plain":["a","bc"],"quoted":["a\\b","\u00e9\n"]})", quoted, "/quoted");
}

TEST(Constraints, CasterJsonMerged)
{
    // a caster replaces the value of its field, so the keywords check the member of the patch
    auto t = CasterFields();
    t.list = CasterList({1});
    t.opt = CasterName("o");
    EXPECT_EQ(
        expect_merge_rule(t, R"({"list":[2,3]})"),
        R"({"color":"","name":"","label":"","real":0.0,"small":0,"flag":false,"list":[2,3],"pos":{"x":0,"y":0},"dom":[],"opt":"o","nul":null})");
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,3,4]})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,2]})").starts_with("bad cast"));
    // the fields that the patch leaves out, and a null that gives a field its default, are not checked
    EXPECT_FALSE(expect_merge_rule(t, R"({"small":1})").starts_with("bad cast"));
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
                                 {T({"a", "b"}), T({"ab"}), CasterFirst(7), bag, {}, {}, {}});
    expect_accepted<CasterViews>(R"({"mixed":["abc"],"opt":["a"],"xy":{"x":1,"y":2},"xys":[{"x":3,"y":4}]})",
                                 {{}, T({"abc"}), {}, {}, T({"a"}), {1, 2}, {{3, 4}}});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CasterViews )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of CasterViews )", key);
    };
    expect_rejected<CasterViews>(R"({"tags":[]})", value("tags") + "has fewer items than the minimum 1", "/tags");
    expect_rejected<CasterViews>(R"({"tags":["a","b","c","d"]})", value("tags") + "has more items than the maximum 3",
                                 "/tags");
    expect_rejected<CasterViews>(R"({"tags":["a","a"]})", element("tags") + "is equal to an earlier element",
                                 "/tags/1");
    expect_rejected<CasterViews>(R"({"mixed":["a","b"]})", value("mixed") + "has more items than the maximum 1",
                                 "/mixed");
    expect_rejected<CasterViews>(R"({"mixed":"abc"})", value("mixed") + "is longer than the maximum length 2",
                                 "/mixed");
    // the caster reads the first element only, and the keywords count them all
    expect_rejected<CasterViews>(R"({"first":[1,2,3]})", value("first") + "has more items than the maximum 2",
                                 "/first");
    expect_rejected<CasterViews>(R"({"bag":{"a":1,"b":2}})", value("bag") + "has more properties than the maximum 1",
                                 "/bag");
    expect_rejected<CasterViews>(R"({"opt":["a","b","c"]})", value("opt") + "has more items than the maximum 2",
                                 "/opt");
    // the caster reads null, so the keywords check it
    expect_rejected<CasterViews>(R"({"opt":null})", value("opt") + R"(is not one of ["a"], ["a","b","c"])", "/opt");
    expect_rejected<CasterViews>(R"({"opt":["b"]})", value("opt") + R"(is not one of ["a"], ["a","b","c"])", "/opt");
    expect_rejected<CasterViews>(R"({"xy":{"x":1,"y":2,"z":3}})", "CasterXY has more properties than the maximum 2",
                                 "/xy");
    expect_rejected<CasterViews>(R"({"xys":[{"x":1,"y":2,"z":3}]})", "CasterXY has more properties than the maximum 2",
                                 "/xys/0");
    expect_rejected<CasterXY>(R"({"x":1,"y":2,"z":3})", "CasterXY has more properties than the maximum 2", "");

    expect_accepted<CasterWrapped>(R"(["a"])", {T({"a"})});
    expect_rejected<CasterWrapped>(R"(["a","b"])", "the value of CasterWrapped has more items than the maximum 1", "");

    // the in-place form fills a field that the merge resets, so the keywords count the members of the patch
    const auto t = yyjson::read<CasterViews>(R"({"bag":{"a":1}})");
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
        R"(bad cast: the value of the key "items" of CasterRemoteItems has more items than the maximum 3 at /items)");
}

TEST(Constraints, CasterJsonNull)
{
    using T = CasterTags;
    // an absent member leaves the std::optional empty, and a null gives it what the caster reads from null
    for (const auto json : {R"({})", R"({"opt":null,"text":null,"one":null})", R"({"opt":["a"]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<CasterNulls>(json)) << json;
        EXPECT_TRUE(expect_direct_read<CasterNulls>(json)) << json;
    }
    EXPECT_FALSE(yyjson::read<CasterNulls>(R"({})").opt.has_value());
    EXPECT_EQ(yyjson::read<CasterNulls>(R"({"opt":null})").opt, std::optional(T()));
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CasterNulls )", key);
    };
    expect_rejected<CasterNulls>(R"({"opt":["b"]})", value("opt") + R"(is not one of ["a"], null)", "/opt", false);
    expect_rejected<CasterNulls>(R"({"text":[]})", value("text") + "is not equal to the constant null", "/text", false);
    expect_rejected<CasterNulls>(R"({"one":[]})", value("one") + "is not equal to the constant null", "/one", false);
    // a null member of the patch gives the field its default, which the keywords do not check
    const auto t = yyjson::read<CasterNulls>(R"({"opt":["a"],"one":null})");
    EXPECT_FALSE(expect_merge_into(t, R"({"opt":null})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_into(t, R"({"one":null})").starts_with("bad cast"));
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
        EXPECT_TRUE(caster[0].first.starts_with("the value of the key") ||
                    caster[0].first.starts_with("an element of the value") ||
                    caster[0].first.starts_with("a key of the value"))
            << caster[0].first;
    }
}

TEST(Constraints, CasterJsonKeys)
{
    const auto closed = "has a key that yyjson::additional_properties(false) rejects";
    const auto names = "is longer than the maximum length 1";
    {
        using T = CasterKeysField;
        const auto value = [](std::string_view key) {
            return std::format(R"(the value of the key "{}" of CasterKeysField )", key);
        };
        const auto key = [](std::string_view name) {
            return std::format(R"(a key of the value of the key "{}" of CasterKeysField )", name);
        };
        auto bag = CasterBag();
        expect_accepted<T>(R"({"bag":{},"set":{"k":1}})", {T{bag, CasterKeySet({{"k", 1}})}});
        expect_rejected<T>(R"({"bag":{"k":1},"set":{}})", value("bag") + closed, "/bag");
        expect_rejected<T>(R"({"bag":{},"set":{"k":1,"kk":2}})", key("set") + names, "/set/kk");
        bag.set("k", 1);
        EXPECT_TRUE(expect_merge_rule(T{bag, {}}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{bag, {}}, R"({"bag":{}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
    }
    {
        using T = CasterKeysProperty;
        const auto value = [](std::string_view key) {
            return std::format(R"(the value of the key "{}" of CasterKeysProperty )", key);
        };
        const auto key = [](std::string_view name) {
            return std::format(R"(a key of the value of the key "{}" of CasterKeysProperty )", name);
        };
        auto bag = CasterBag();
        bag.set("k", 1);
        expect_accepted<T>(R"({"bag":{"k":1},"set":{}})", {T{bag, {}}});
        expect_rejected<T>(R"({"bag":{"kk":1},"set":{}})", key("bag") + names, "/bag/kk");
        expect_rejected<T>(R"({"bag":{},"set":{"k":1}})", value("set") + closed, "/set");
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"bag":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{}})").starts_with("bad cast"));
    }
}
#endif

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

TEST(Constraints, ElementAnnotations)
{
    expect_accepted<ElementsAnnotated>(
        R"({"scores":[0],"ids":[1,2],"row":[0,"a"],"names":{"a":1},"tags":["x","y"],"groups":{"g":[9]}})",
        {{0}, {1, 2}, {0, "a"}, {{"a", 1}}, {"x", "y"}, {{"g", {9}}}});
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of ElementsAnnotated )", key);
    };
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ElementsAnnotated )", key);
    };
    expect_rejected<ElementsAnnotated>(R"({"scores":[0,-1]})", element("scores") + "is less than the minimum 0",
                                       "/scores/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[1,1]})", element("ids") + "is equal to an earlier element", "/ids/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[]})", value("ids") + "has fewer items than the minimum 1", "/ids");
    expect_rejected<ElementsAnnotated>(R"({"row":[0,"ab"]})", element("row") + "is longer than the maximum length 1",
                                       "/row/1");
    expect_rejected<ElementsAnnotated>(
        R"({"names":{"A":1}})",
        R"(a key of the value of the key "names" of ElementsAnnotated does not match the pattern "^[a-z]+$")",
        "/names/A");
    expect_rejected<ElementsAnnotated>(
        R"({"tags":["x","x"]})",
        value("tags") + "has more than 1 item that satisfies the constraints of yyjson::contains", "/tags");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[10]}})", element("groups") + "is greater than the maximum 9",
                                       "/groups/g/0");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[],"h":[]}})",
                                       value("groups") + "has more properties than the maximum 1", "/groups");
}

TEST(Constraints, CombinatorAnnotations)
{
    expect_accepted<CombinedAnnotated>(R"({"n":2,"even":12,"digit":9,"s":"bc","one":4})", {2, 12, 9, "bc", 4, {}});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CombinedAnnotated )", key);
    };
    expect_rejected<CombinedAnnotated>(R"({"n":0})", value("n") + "satisfies the settings of yyjson::not_", "/n");
    expect_rejected<CombinedAnnotated>(
        R"({"even":11})", value("even") + "satisfies the settings of yyjson::if_ and not those of yyjson::then",
        "/even");
    expect_rejected<CombinedAnnotated>(
        R"({"digit":10})", value("digit") + "does not satisfy all the subschemas of yyjson::all_of", "/digit");
    expect_rejected<CombinedAnnotated>(R"({"s":"c"})",
                                       value("s") + "satisfies none of the subschemas of yyjson::any_of", "/s");
    expect_rejected<CombinedAnnotated>(R"({"one":6})",
                                       value("one") + "satisfies more than one subschema of yyjson::one_of", "/one");
    expect_rejected<CombinedAnnotated>(
        R"({"ints":[1,"a"]})",
        R"(an element of the value of the key "ints" of CombinedAnnotated is not read as int by yyjson::matches)",
        "/ints/1");
}

TEST(Constraints, StructConditionAnnotations)
{
    static_assert(yyjson::detail::has_struct_conditions<ExclusiveAnnotated>);
    expect_accepted<ExclusiveAnnotated>(R"({"first":1})", {1, {}});
    expect_accepted<ExclusiveAnnotated>(R"({"b":0})", {{}, 0});
    expect_rejected<ExclusiveAnnotated>(R"({"first":1,"b":2})",
                                        "ExclusiveAnnotated has the keys and the values that yyjson::not_ rejects", "");
    expect_rejected<ExclusiveAnnotated>(R"({"b":-1})",
                                        "ExclusiveAnnotated satisfies none of the subschemas of yyjson::any_of", "");
}

TEST(Constraints, PatternAnnotations)
{
    expect_accepted<PatternAnnotated>(R"({"x1":2,"a":1})", {1, {{"x1", 2}}});
    expect_rejected<PatternAnnotated>(
        R"({"x1":-1})", R"(the value of the key "x1" of PatternAnnotated is less than the minimum 0)", "/x1");
    EXPECT_EQ(std::string(yyjson::write(PatternAnnotated{1, {{"x1", 2}}})), R"({"a":1,"x1":2})");
}

TEST(Constraints, CasterAnnotations)
{
    expect_accepted<CasterAnnotated>(R"({"list":[1,2],"name":"abc"})", {CasterList({1, 2}), CasterName("abc")});
    expect_rejected<CasterAnnotated>(R"({"list":[1,1]})",
                                     R"(the value of the key "list" of CasterAnnotated has duplicate items)", "/list");
    expect_rejected<CasterAnnotated>(
        R"({"name":"abcd"})", R"(the value of the key "name" of CasterAnnotated is longer than the maximum length 3)",
        "/name");
}
#endif

// NOLINTEND
