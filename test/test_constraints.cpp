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
    yyjson::field("max0", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(0)),
    yyjson::field("both0", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0), yyjson::max_contains(0)),
    yyjson::field("opts", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(1)),
    yyjson::field("unique", yyjson::unique_items), yyjson::field("head", yyjson::prefix_items(yyjson::minimum(0))),
    yyjson::field("some", yyjson::contains(yyjson::minimum(10))));

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
CONSTRAINT_RULE(ItemsInverted, std::vector<int>, yyjson::min_items(3), yyjson::max_items(2))
CONSTRAINT_RULE(ContainsMissing, std::vector<int>, yyjson::min_contains(1))
CONSTRAINT_RULE(ContainsInverted, std::vector<int>, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(2),
                yyjson::max_contains(1))
using StringMap = std::map<std::string, int>;
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
CONSTRAINT_RULE(AnyOfTooLong, Nested10,
                yyjson::any_of(yyjson::items(yyjson::items(yyjson::items(
                    yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::items(yyjson::minimum(0)))))))))))
#undef CONSTRAINT_RULE
namespace constraint_tree_rules
{
    template <typename T>
    constexpr auto tree_error = yyjson::detail::make_schema_tree<&yyjson::detail::build_field_tree<T, 0>>().error;
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
    static_assert(tree_error<ItemsInverted> == reads_nothing && tree_error<PropertiesInverted> == reads_nothing &&
                  tree_error<ContainsInverted> == reads_nothing && tree_error<ItemsInside> == reads_nothing);
    static_assert(tree_error<ContainsMissing> == contains_missing && tree_error<ItemsTwice> == keyword_twice);
    static_assert(tree_error<PrefixTooLong> == prefix_position_too_large);
    static_assert(tree_error<NotTwice> == none && tree_error<NotInverted> == reads_nothing &&
                  tree_error<BranchMissingContains> == contains_missing && tree_error<MatchesTwice> == keyword_twice &&
                  tree_error<AnyOfTooLong> == prefix_position_too_large);

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
        yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
        yyjson::then(yyjson::field("radius", yyjson::required, yyjson::minimum(0))),
        yyjson::else_(yyjson::field("p", yyjson::matches<ConditionPoint>),
                      yyjson::field("sides", yyjson::items(yyjson::minimum(1)), yyjson::min_items(3)))),
    yyjson::not_(yyjson::field("kind", yyjson::pattern("^x"))));
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
    yyjson::if_then_else(yyjson::if_(yyjson::field("level", yyjson::maximum(4))),
                         yyjson::else_(yyjson::field("note", yyjson::required, yyjson::min_length(1)))));
// VISITABLE_STRUCT
struct ConditionVisitable
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const ConditionVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ConditionVisitable> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::field("a", yyjson::required)), yyjson::then(yyjson::field("b", yyjson::required))));
VISITABLE_STRUCT(ConditionVisitable, a, b);
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
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::required)),
                                          yyjson::then(yyjson::field("d", yyjson::required))),
                     yyjson::if_then_else(yyjson::if_(yyjson::field("b", yyjson::required)),
                                          yyjson::then(yyjson::field("d", yyjson::required))),
                     yyjson::not_(yyjson::field("c", yyjson::required), yyjson::field("e", yyjson::required)));
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
    yyjson::annotate(yyjson::one_of(yyjson::field("a", yyjson::required), yyjson::field("b", yyjson::required)),
                     yyjson::all_of(yyjson::field("a", yyjson::maximum(3))),
                     yyjson::if_then_else(yyjson::if_(yyjson::field("c", yyjson::required)),
                                          yyjson::then(yyjson::field("d", yyjson::required))));
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
    yyjson::annotate(yyjson::any_of(yyjson::field("name", yyjson::required), yyjson::field("id", yyjson::required)),
                     yyjson::one_of(yyjson::field("email", yyjson::required), yyjson::field("phone", yyjson::required)),
                     yyjson::all_of({yyjson::field("age", yyjson::minimum(0))},
                                    yyjson::not_(yyjson::field("id", yyjson::required, yyjson::maximum(0)))));
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
    yyjson::if_then_else(yyjson::if_(yyjson::field("auto", yyjson::required, yyjson::matches<yyjson::object>)),
                         yyjson::then(yyjson::field("name", yyjson::required))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("point", yyjson::required, yyjson::matches<ConditionPoint>)),
                         yyjson::then(yyjson::field("label", yyjson::required))),
    yyjson::not_(yyjson::field("kind", yyjson::required, yyjson::matches<std::nullptr_t>)));
// keywords of the values in the conditions: a kind "few" when there are two or three xs, but not with four or more
struct AlignedConditions
{
    std::optional<std::string> kind;
    std::vector<int> xs;
    std::vector<int> ys;
    std::vector<std::vector<int>> rows;
};
template <>
inline constexpr auto yyjson::annotations<AlignedConditions> =
    yyjson::annotate(yyjson::field("ys", yyjson::not_(yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2))),
                     yyjson::field("rows", yyjson::items(yyjson::not_(yyjson::contains(yyjson::const_(0))))),
                     yyjson::if_then_else(yyjson::if_(yyjson::field("xs", yyjson::min_items(2), yyjson::max_items(3))),
                                          yyjson::then(yyjson::field("kind", yyjson::required, yyjson::const_("few")))),
                     yyjson::not_(yyjson::field("xs", yyjson::min_items(4)),
                                  yyjson::field("kind", yyjson::required, yyjson::const_("few"))));
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
    yyjson::if_then_else(yyjson::if_(yyjson::field("d", yyjson::required, yyjson::prefix_items(yyjson::const_("a")))),
                         yyjson::then(yyjson::field("n", yyjson::required))));

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
    yyjson::field("m", yyjson::additional_properties(yyjson::field("id", yyjson::required))),
    yyjson::field("v", yyjson::not_(yyjson::field("bad", yyjson::required))),
    // an absent key satisfies yyjson::field, so that yyjson::if_ tests it with yyjson::required
    yyjson::field("o", yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::required, yyjson::const_("x"))),
                                            yyjson::then(yyjson::field("x", yyjson::required, yyjson::minimum(1))))),
    yyjson::field("e", yyjson::items(yyjson::field("id", yyjson::required, yyjson::minimum(0)))));

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

namespace struct_condition_rules
{
    // the settings that a condition of a struct takes, which the types alone tell
    template <typename... As>
    concept conditions_accepted = requires { yyjson::not_(std::declval<As>()...); };
    static_assert(conditions_accepted<decltype(yyjson::field("k", yyjson::required, yyjson::minimum(0)))>);
    static_assert(yyjson::detail::has_struct_conditions<ConditionShape> &&
                  yyjson::detail::has_struct_conditions<ConditionVisitable> &&
                  !yyjson::detail::has_struct_conditions<Bounded> && !yyjson::detail::has_constraints<Contact>);
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
    = yyjson::not_(yyjson::field("first", yyjson::required), yyjson::field("b", yyjson::required)),
    = yyjson::any_of(yyjson::field("first", yyjson::required), yyjson::field("b", yyjson::minimum(0)))
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
    // a value that the map keeps is not in the JSON, which leaves the error at the map
    auto t = Elements();
    t.limits = {{"old", 200}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"limits":{"a":1}})", t); });
    EXPECT_TRUE(message_of(e).starts_with(R"(the value of the key "limits" of Elements has a value that does not )"
                                          R"(satisfy the constraints of yyjson::additional_properties)"))
        << e.what();
    EXPECT_EQ(e.path(), "/limits");
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

TEST(Constraints, MergeKept)
{
    // a key that merge_into keeps is not in the JSON, which leaves the error at the map
    auto keyed = Keyed();
    keyed.m = {{"ABC", 1}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"ab":1}})", keyed); });
    EXPECT_TRUE(message_of(e).starts_with(R"(the value of the key "m" of Keyed has a key that does not satisfy )"
                                          R"(the constraints of yyjson::property_names)"))
        << e.what();
    EXPECT_EQ(e.path(), "/m");
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
    // min_contains(0) alone accepts any array, max_contains(0) alone still needs one item, and both reject one
    expect_accepted<Counted>(R"({"min0":[],"both0":[1]})", {.both0 = {1}});
    expect_accepted<Counted>(R"({"min0":[1],"both0":[]})", {.min0 = {1}});
    const auto prefix = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Counted )", key);
    };
    expect_rejected<Counted>(
        R"({"max0":[1]})", prefix("max0") + "has no item that satisfies the constraints of yyjson::contains", "/max0");
    expect_rejected<Counted>(R"({"max0":[10]})",
                             prefix("max0") + "has more than 0 items that satisfy the constraints of yyjson::contains",
                             "/max0");
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
    // every read of `json` into T, alone and as an element, and `merge_into` unless `merged` is false: accepted, or
    // rejected with `message` at `path`; the reader without a document reads the accepted ones as the document does
    template <typename T>
    void expect_conditions(std::string_view json, std::string_view message, std::string_view path = "",
                           bool merged = true)
    {
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
    expect_conditions<Contact>(R"({"email":"e"})", "Contact satisfies none of the subschemas of yyjson::any_of");
    expect_conditions<Contact>(R"({"name":"a"})", "Contact satisfies none of the subschemas of yyjson::one_of");
    expect_conditions<Contact>(R"({"name":"a","email":"e","phone":"p"})",
                               "Contact satisfies more than one subschema of yyjson::one_of");
    expect_conditions<Contact>(R"({"name":"a","email":"e","age":-1})",
                               R"(the value of the key "age" of Contact is less than the minimum 0)", "/age");
    expect_conditions<Contact>(R"({"id":0,"email":"e"})",
                               "Contact has the keys and the values that yyjson::not_ rejects");
}

TEST(Constraints, StructConditionsOfNull)
{
    // a null that the read takes as a missing key is no key, and the conditions do not check the value that the
    // field keeps
    expect_conditions<NullKeys>(R"({"a":null,"b":1})", "");
    expect_conditions<NullKeys>(R"({"a":null})", "NullKeys satisfies none of the subschemas of yyjson::one_of");
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
    // merge_into checks the patch: the keys of the patch but those with a null, which removes a value, with the
    // values that the merge gives
    auto c = Contact{.name = "a", .email = "e"};
    yyjson::merge_into(R"({"name":"a","email":"e","age":3})", c);
    EXPECT_EQ(c.age, 3);
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"name":"a","email":"e","age":-1})", c); });
    EXPECT_TRUE(message_of(e).starts_with(R"(the value of the key "age" of Contact is less than the minimum 0)"))
        << e.what();
    EXPECT_EQ(e.path(), "/age");
    e = bad_cast_of([&] { yyjson::merge_into(R"({"age":3})", c); });
    EXPECT_TRUE(message_of(e).starts_with("Contact satisfies none of the subschemas of yyjson::any_of")) << e.what();
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
                   R"(that yyjson::field gives)";
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
    // the reader without a document leaves the elements of the JSON array to the document
    expect_accepted<MultiKindWritten>(R"({"p":"x","q":[5]})", {"x", {5, {}, {}}}, false);
    expect_accepted<MultiKindWritten>(R"({"p":[1,2],"q":[1,5]})", {MultiKindPair{1, 2, {}}, {1, 5, {}}}, false);
    expect_accepted<MultiKindWritten>(R"({"p":[1,4,3],"q":[5,6]})", {MultiKindPair{1, 4, {3}}, {5, 6, {}}}, false);
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
#endif

// NOLINTEND
