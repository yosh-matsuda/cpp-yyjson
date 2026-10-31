#include <regex>
#include "test_common.hpp"

// NOLINTBEGIN

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

// bounds of a float field, which compares with them rounded to a float
struct FloatBounds
{
    float tenth = 0;
    float under = 0;
    float big = 16777216.0F;
    bool operator==(const FloatBounds&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FloatBounds> =
    yyjson::annotate(yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("under", yyjson::exclusive_maximum(0.1)),
                     yyjson::field("big", yyjson::minimum(16777217)));

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

// the elements of ranges and the values of maps
bool is_small_pair(const std::pair<const int, int>& p) { return p.first < p.second; }
struct Elements
{
    std::vector<int> scores;
    std::array<int, 2> pair = {};
    std::list<std::string> names;
    std::set<int> set;
    std::unordered_set<int> uset;
    std::map<std::string, int> limits;
    std::unordered_map<std::string, std::vector<int>> groups;
    std::map<int, int> by_id;  // read from an array of [key, value] pairs
    std::map<int, int> ordered;
    std::vector<std::optional<int>> holes;
    std::optional<std::vector<std::string>> tags;
    std::vector<std::vector<std::string>> nested;
    bool operator==(const Elements&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Elements> = yyjson::annotate(
    yyjson::field("scores", yyjson::items(yyjson::minimum(0))),
    yyjson::field("pair", yyjson::items(yyjson::maximum(9))),
    yyjson::field("names", yyjson::items(yyjson::pattern("^[a-z]+$"))),
    yyjson::field("set", yyjson::items(yyjson::maximum(5))),
    yyjson::field("uset", yyjson::items(yyjson::satisfies<is_even>)),
    yyjson::field("limits", yyjson::max_items(3), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field("groups", yyjson::additional_properties(yyjson::min_items(1), yyjson::items(yyjson::minimum(1)))),
    yyjson::field("by_id", yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("ordered", yyjson::items(yyjson::satisfies<is_small_pair>)),
    yyjson::field("holes", yyjson::items(yyjson::maximum(5))),
    yyjson::field("tags", yyjson::items(yyjson::max_length(3))),
    yyjson::field("nested", yyjson::items(yyjson::min_items(1), yyjson::items(yyjson::max_length(3)))));

struct ElementsVisitable
{
    std::vector<int> v;
    bool operator==(const ElementsVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ElementsVisitable> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::exclusive_minimum(0))));
VISITABLE_STRUCT(ElementsVisitable, v);

// the values that smart pointers and a std::variant of std::monostate hold, as those of std::optional
bool fits_in_three(const std::string& s) { return s.size() < 4; }
struct Nullables
{
    std::variant<std::monostate, std::string> code;
    std::variant<double, std::monostate> ratio = std::monostate();
    std::shared_ptr<std::string> name;
    std::shared_ptr<std::vector<int>> list;
    std::vector<std::shared_ptr<int>> ids;
    bool operator==(const Nullables& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<Nullables> = yyjson::annotate(
    yyjson::field("code", yyjson::max_length(3)), yyjson::field("ratio", yyjson::maximum(1)),
    yyjson::field("name", yyjson::satisfies<fits_in_three>),
    yyjson::field("list", yyjson::min_items(1), yyjson::items(yyjson::maximum(9))),
    yyjson::field("ids", yyjson::unique_items));
struct Owned
{
    std::unique_ptr<int> count;
    std::optional<std::unique_ptr<int>> level;
    std::vector<std::unique_ptr<std::string>> tags;
};
template <>
inline constexpr auto yyjson::annotations<Owned> = yyjson::annotate(
    yyjson::field("count", yyjson::minimum(0)),
    yyjson::field("level", yyjson::maximum(5), yyjson::satisfies<[](const int& v) { return v != 3; }>),
    yyjson::field("tags", yyjson::items(yyjson::max_length(3)), yyjson::unique_items));

// const_ and enum_ reject JSON null unless they list nullptr, as JSON Schema's const and enum
struct NullableListed
{
    std::variant<std::monostate, std::string> unit;
    std::variant<std::monostate, int> level;
    std::shared_ptr<int> count;
    std::vector<std::optional<int>> steps;
    std::optional<std::string> note;
    std::optional<std::variant<std::monostate, int>> mode;
    std::shared_ptr<std::vector<int>> none;
    bool operator==(const NullableListed& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<NullableListed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm")), yyjson::field("level", yyjson::enum_(1, 2, nullptr)),
    yyjson::field("count", yyjson::const_(1)), yyjson::field("steps", yyjson::items(yyjson::enum_(1, 2))),
    yyjson::field("note", yyjson::const_("x")), yyjson::field("mode", yyjson::const_(nullptr)),
    yyjson::field("none", yyjson::const_(nullptr)));

// the elements of a range that are equal to an earlier one, the positions of a range or of a tuple, the keys of a
// map, and the elements of a range that satisfy settings
struct Unordered
{
    int v = 0;
    bool operator==(const Unordered&) const = default;
};
struct Unique
{
    std::vector<int> ints;
    std::vector<std::string> strs;
    std::vector<double> reals;
    std::vector<Unordered> plain;
    std::vector<std::optional<int>> opts;
    std::vector<std::vector<int>> nested;
    bool operator==(const Unique&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Unique> = yyjson::annotate(
    yyjson::field("ints", yyjson::unique_items), yyjson::field("strs", yyjson::unique_items),
    yyjson::field("reals", yyjson::unique_items), yyjson::field("plain", yyjson::unique_items),
    yyjson::field("opts", yyjson::unique_items), yyjson::field("nested", yyjson::items(yyjson::unique_items)));

struct UniqueJson
{
    std::vector<yyjson::value> any;
};
template <>
inline constexpr auto yyjson::annotations<UniqueJson> = yyjson::annotate(yyjson::field("any", yyjson::unique_items));

// elements that are compared by the values that they are written with, and not by `operator==`
struct UniqueBox
{
    std::shared_ptr<int> p;
};
struct UniqueLoose
{
    int v = 0;
    bool operator==(const UniqueLoose&) const { return true; }
};
struct UniqueTree
{
    int v = 0;
    std::vector<UniqueTree> kids;
};
struct UniqueParts
{
    std::vector<UniqueBox> boxes;
    std::vector<UniqueLoose> loose;
    std::vector<UniqueTree> trees;
    std::vector<std::unordered_map<std::string, int>> maps;
    std::vector<std::vector<std::pair<std::string, int>>> pairs;
    std::vector<std::tuple<int, std::string>> tuples;
};
template <>
inline constexpr auto yyjson::annotations<UniqueParts> = yyjson::annotate(
    yyjson::field("boxes", yyjson::unique_items), yyjson::field("loose", yyjson::unique_items),
    yyjson::field("trees", yyjson::unique_items), yyjson::field("maps", yyjson::unique_items),
    yyjson::field("pairs", yyjson::unique_items), yyjson::field("tuples", yyjson::unique_items));

bool is_lower(const std::string& s)
{
    return std::ranges::all_of(s, [](char c) { return 'a' <= c && c <= 'z'; });
}
struct Prefixed
{
    std::tuple<int, std::string, double> row;
    std::pair<int, int> range;
    std::vector<int> head;
    std::array<int, 3> arr = {};
    std::vector<std::vector<int>> grid;
    bool operator==(const Prefixed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Prefixed> = yyjson::annotate(
    yyjson::field("row",
                  yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(2), yyjson::satisfies<is_lower>}, {})),
    yyjson::field("range", yyjson::prefix_items(yyjson::maximum(5), yyjson::minimum(5))),
    yyjson::field("head", yyjson::prefix_items({yyjson::minimum(0)}, {}), yyjson::items(yyjson::maximum(9))),
    yyjson::field("arr", yyjson::prefix_items({}, yyjson::minimum(1))),
    yyjson::field("grid", yyjson::prefix_items(yyjson::items(yyjson::minimum(1)), {yyjson::min_items(2)})));

// the string settings written as string literals, which hold their characters before C++26, and a position of
// prefix_items that is made of them as temporaries
struct Literals
{
    std::tuple<std::string, std::vector<std::string>> pair;
    std::string unit;
    std::vector<std::string> days;
    bool operator==(const Literals&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Literals> = yyjson::annotate(
    yyjson::field("pair", yyjson::prefix_items({yyjson::pattern("^[a-z]+$"), yyjson::enum_("ab", "Ab", "abc")},
                                               yyjson::prefix_items(yyjson::const_("yz")))),
    yyjson::field("unit", yyjson::enum_("mm", "cm"), yyjson::title("Unit"), yyjson::examples(R"("mm")")),
    yyjson::field("days", yyjson::items(yyjson::format("date"))));
// they are template arguments, as of yyjson::alt
template <auto... Settings>
struct setting_arguments
{
};
template struct setting_arguments<yyjson::pattern("^a"), yyjson::format("date"), yyjson::title("t"),
                                  yyjson::description("d"), yyjson::const_("c"), yyjson::enum_("e", 1),
                                  yyjson::examples("1", "2")>;

bool is_short(const std::string& s) { return s.size() < 3; }
struct Keyed
{
    std::map<std::string, int> m;
    std::unordered_map<std::string, int> u;
    std::map<std::string, std::map<std::string, int>> deep;
    bool operator==(const Keyed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Keyed> = yyjson::annotate(
    yyjson::field("m", yyjson::property_names(yyjson::max_length(3), yyjson::pattern("^[a-z]+$"))),
    yyjson::field("u", yyjson::property_names(yyjson::satisfies<is_short>)),
    yyjson::field("deep", yyjson::additional_properties(yyjson::property_names(yyjson::min_length(2)))));

// several patterns at one level, which a value must all match, between other settings too
struct Patterns
{
    std::string s;
    std::optional<std::string> o;
    std::vector<std::string> list;
    std::map<std::string, int> m;
    std::string n;
    bool operator==(const Patterns&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Patterns> = yyjson::annotate(
    yyjson::field("s", yyjson::pattern("^a"), yyjson::pattern("z$")),
    yyjson::field("o", yyjson::pattern("^o"), yyjson::min_length(2), yyjson::pattern("o$"), yyjson::pattern("x")),
    yyjson::field("list", yyjson::items(yyjson::pattern("^a"), yyjson::enum_("ab", "az", "b"),
                                        yyjson::pattern("b$"))),
    yyjson::field("m", yyjson::property_names(yyjson::pattern("^k"), yyjson::pattern("[0-9]$"))),
    yyjson::field("n", yyjson::not_(yyjson::pattern("^a"), yyjson::pattern("z$"))));

struct Containing
{
    std::vector<std::string> roles;
    std::vector<int> v;
    std::vector<int> none;
    std::vector<std::vector<int>> rows;
    bool operator==(const Containing&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Containing> = yyjson::annotate(
    yyjson::field("roles", yyjson::contains(yyjson::pattern("^admin$"))),
    yyjson::field("v", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2), yyjson::max_contains(3)),
    yyjson::field("none", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0), yyjson::max_contains(1)),
    yyjson::field("rows", yyjson::contains(yyjson::contains(yyjson::minimum(5)))));

// the corner cases of the settings of the values: counts of yyjson::contains that are 0, null elements, a
// std::optional range, a user caster, keys that are not strings, ranges of pairs, and sequences that are not vectors
struct Counted
{
    std::vector<int> min0;
    std::vector<int> max0;
    std::vector<int> both0;
    std::vector<std::optional<int>> opts;
    std::optional<std::vector<int>> unique;
    std::optional<std::vector<int>> head;
    std::optional<std::vector<int>> some;
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

struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
};
template <>
struct yyjson::caster<Point>
{
    static Point from_json(std::array<int, 2> a) { return {a[0], a[1]}; }
    template <typename... Ts>
    static std::array<int, 2> to_json(const Point& p, Ts...)
    {
        return {p.x, p.y};
    }
};
enum class Side
{
    left,
    right
};
bool is_left(const Side& s) { return s == Side::left; }
// a range that its caster reads from a string such as "1,2"
struct Csv
{
    std::vector<int> v;
    auto begin() const { return v.begin(); }
    auto end() const { return v.end(); }
    bool operator==(const Csv&) const = default;
};
template <>
struct yyjson::caster<Csv>
{
    static Csv from_json(std::string_view s)
    {
        auto result = Csv();
        for (std::size_t i = 0; i <= s.size();)
        {
            const auto j = std::min(s.find(',', i), s.size());
            result.v.push_back(std::stoi(std::string(s.substr(i, j - i))));
            i = j + 1;
        }
        return result;
    }
    template <typename... Ts>
    static std::string to_json(const Csv& c, Ts...)
    {
        auto result = std::string();
        for (const auto i : c.v) result += std::format("{}{}", result.empty() ? "" : ",", i);
        return result;
    }
};
struct Unusual
{
    std::vector<Point> pts;
    std::map<Side, int> sides;
    std::multimap<std::string, int> mm;
    std::vector<std::pair<std::string, int>> kv;
    std::deque<int> dq;
    std::list<int> ls;
    std::vector<double> reals;
    std::optional<Csv> csv;
    bool operator==(const Unusual&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Unusual> = yyjson::annotate(
    yyjson::field("pts", yyjson::unique_items),
    yyjson::field("sides", yyjson::property_names(yyjson::satisfies<is_left>)),
    yyjson::field("mm", yyjson::unique_items), yyjson::field("kv", yyjson::property_names(yyjson::max_length(1))),
    yyjson::field("dq", yyjson::prefix_items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(9))),
    yyjson::field("ls", yyjson::prefix_items({}, yyjson::minimum(1)), yyjson::items(yyjson::maximum(9))),
    yyjson::field("reals", yyjson::unique_items), yyjson::field("csv", yyjson::items(yyjson::minimum(0))));

// the settings of the values in the values in the values
struct Deep
{
    std::vector<std::vector<std::vector<int>>> v;
    std::map<std::string, std::vector<int>> m;
    std::vector<std::map<std::string, int>> k;
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
    std::vector<int> u;
    std::tuple<int, int> p;
    std::vector<int> c;
    std::map<std::string, int> k;
    bool operator==(const Visited&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Visited> = yyjson::annotate(
    yyjson::field("u", yyjson::unique_items), yyjson::field("p", yyjson::prefix_items(yyjson::minimum(0))),
    yyjson::field("c", yyjson::contains(yyjson::minimum(5))),
    yyjson::field("k", yyjson::property_names(yyjson::max_length(1))));
VISITABLE_STRUCT(Visited, u, p, c, k);

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
    std::vector<std::string> tags;
    std::map<std::string, int> named;
    std::tuple<int, std::string> pair = {1, "x"};
    bool operator==(const Listed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Listed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm", "m")), yyjson::field("version", yyjson::const_(2)),
    yyjson::field("ratio", yyjson::enum_(0.1, 0.5)), yyjson::field("flag", yyjson::const_(true)),
    yyjson::field("big", yyjson::enum_(-1, 0, 18446744073709551615ULL)),
    yyjson::field("level", yyjson::enum_(1, 2, 3)),
    yyjson::field("odd", yyjson::const_(1), yyjson::enum_(1, 3)),
    yyjson::field("tags", yyjson::items(yyjson::enum_("a", "b"))),
    yyjson::field("named", yyjson::property_names(yyjson::enum_("x", "y"))),
    yyjson::field("pair", yyjson::prefix_items({yyjson::const_(1)}, {yyjson::enum_("x", "y")})));

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
    std::vector<Tone> tones;
    bool operator==(const ListedEnums&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedEnums> =
    yyjson::annotate(yyjson::field("first", yyjson::const_("low")),
                     yyjson::field("loud", yyjson::enum_("mid", "HIGH")),
                     yyjson::field("maybe", yyjson::const_("mid")),
                     yyjson::field("tones", yyjson::items(yyjson::enum_("low", "HIGH"))));

// const_ and enum_ of JSON texts, read into the type of the value that they check
struct Spot
{
    int x = 0;
    int y = 0;
    bool operator==(const Spot&) const = default;
};
// a setting made from a std::string_view is given to yyjson::alt as a pointer
inline constexpr auto origin = yyjson::const_(yyjson::json_text(std::string_view(R"({"x":0,"y":0})")));
struct ListedTexts
{
    Spot at;
    std::vector<int> pair = {1, 2};
    std::optional<std::vector<double>> scale;
    std::variant<int, std::vector<int>> mixed;
    std::map<std::string, int> counts = {{"a", 1}};
    std::vector<std::pair<std::string, int>> order = {{"a", 2}, {"b", 1}};
    std::vector<std::vector<int>> rows;
    yyjson::any_of<yyjson::alt<Spot, &origin>, yyjson::alt<std::vector<int>, yyjson::const_(yyjson::json_text("[7]"))>,
                   std::string>
        place = "here";
    bool operator==(const ListedTexts&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedTexts> = yyjson::annotate(
    yyjson::field("at",
                  yyjson::enum_(yyjson::json_text(R"({ "y": 0, "x": 0 })"), yyjson::json_text(R"({"x":1,"y":1})"))),
    yyjson::field("pair", yyjson::const_(yyjson::json_text("[1, 2]"))),
    yyjson::field("scale", yyjson::enum_(yyjson::json_text("[1]"), yyjson::json_text("[2.5, 2.5]"))),
    yyjson::field("mixed", yyjson::enum_(yyjson::json_text("0"), yyjson::json_text("[0]"), yyjson::json_text("{}"))),
    yyjson::field("counts", yyjson::not_(yyjson::const_(yyjson::json_text(R"({"a":0})")))),
    yyjson::field("order", yyjson::const_(yyjson::json_text(R"({"a":2,"b":1})"))),
    yyjson::field("rows", yyjson::items(yyjson::enum_(yyjson::json_text("[]"), yyjson::json_text("[0,0]")))));
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

// the formats that the library checks, "regex" with the engine of pattern_matcher<>, and one that the program checks
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
struct Formatted
{
    std::string when = "2024-02-29T23:59:60Z";
    std::optional<std::string> mail;
    std::vector<std::string> hosts;
    std::string code = "X";
    std::optional<std::string> re;
    bool operator==(const Formatted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Formatted> =
    yyjson::annotate(yyjson::field("when", yyjson::format("date-time")),
                     yyjson::field("mail", yyjson::format("email")),
                     yyjson::field("hosts", yyjson::items(yyjson::format("hostname"))),
                     yyjson::field("code", yyjson::format("upper")), yyjson::field("re", yyjson::format("regex")));

// dependent_required names members, whose keys are renamed
struct Billing
{
    std::string name;
    std::optional<std::string> card;
    std::optional<std::string> address;
    int zip = 0;
    bool operator==(const Billing&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Billing> =
    yyjson::annotate(yyjson::dependent_required("card", {"address", "zip"}), yyjson::field("zip", yyjson::rename("postal")));

// one_of_required names members, whose keys are renamed
struct Measure
{
    std::string name;
    std::optional<std::string> env;
    std::optional<double> value;
    std::optional<std::string> unit;
};
template <>
inline constexpr auto yyjson::annotations<Measure> = yyjson::annotate(
    yyjson::one_of_required({"env"}, {"value", "unit"}), yyjson::field("unit", yyjson::rename("u")));
// a group whose field reads null
struct NullableMeasure
{
    std::optional<std::variant<std::monostate, std::string>> env;
    std::optional<double> value;
};
template <>
inline constexpr auto yyjson::annotations<NullableMeasure> = yyjson::annotate(yyjson::one_of_required({"env"}, {"value"}));
// any_of_required and one_of_required together
struct Reachable
{
    std::optional<std::string> email;
    std::optional<std::string> phone;
    std::optional<std::string> fax;
};
template <>
inline constexpr auto yyjson::annotations<Reachable> =
    yyjson::annotate(yyjson::any_of_required({"email"}, {"phone"}), yyjson::one_of_required({"phone"}, {"fax"}));
// an object of VISITABLE_STRUCT that has the keys of all the fields in their order
struct ExclusiveVisitable
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<ExclusiveVisitable> =
    yyjson::annotate(yyjson::one_of_required({"a"}, {"b"}));
VISITABLE_STRUCT(ExclusiveVisitable, a, b);

// min_properties and max_properties count the keys of the object, unknown keys too
struct Sized
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<Sized> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
// a struct without fields
struct SizedEmpty
{
};
template <>
inline constexpr auto yyjson::annotations<SizedEmpty> = yyjson::annotate(yyjson::min_properties(1));
// the keys that a field of yyjson::extra_keys receives
struct SizedExtra
{
    std::optional<int> a;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<SizedExtra> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::field("rest", yyjson::extra_keys));
// VISITABLE_STRUCT
struct SizedVisitable
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<SizedVisitable> = yyjson::annotate(yyjson::max_properties(1));
VISITABLE_STRUCT(SizedVisitable, a, b);
// the tag of an internally tagged variant is a key of the object of the alternative
struct SizedTagged
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<SizedTagged> = yyjson::annotate(yyjson::max_properties(2));
struct SizedOther
{
    int x = 0;
};
using SizedVariant = std::variant<SizedTagged, SizedOther>;
template <>
inline constexpr auto yyjson::annotations<SizedVariant> = yyjson::annotate(yyjson::internal_tag("kind", {"t", "o"}));

// if_then_else and not_: a circle has a radius of at least 0, another shape a point and three sides or more, and no
// kind starts with "x"; the radius is renamed
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
    yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
                         yyjson::then(yyjson::field("r", yyjson::required, yyjson::minimum(0))),
                         yyjson::else_(yyjson::field("p", yyjson::is_type<ConditionPoint>),
                                       yyjson::field("sides", yyjson::items(yyjson::minimum(1)),
                                                     yyjson::min_items(3)))),
    yyjson::not_(yyjson::field("kind", yyjson::pattern("^x"))), yyjson::field("r", yyjson::rename("radius")));
// is_type in if_ and not_: an object in "auto" needs a name, and "kind" is not null; "point" read as ConditionPoint,
// which strict reading does not do with another key, needs a label
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
    yyjson::if_then_else(yyjson::if_(yyjson::field("auto_", yyjson::required, yyjson::is_type<yyjson::object>)),
                         yyjson::then(yyjson::field("name", yyjson::required))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("point", yyjson::required, yyjson::is_type<ConditionPoint>)),
                         yyjson::then(yyjson::field("label", yyjson::required))),
    yyjson::not_(yyjson::field("kind", yyjson::required, yyjson::is_type<std::nullptr_t>)));
// a field with yyjson::required in a condition, and an if_then_else without then
struct Leveled
{
    int level = 0;
    std::optional<std::string> note;
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
};
template <>
inline constexpr auto yyjson::annotations<ConditionVisitable> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::field("a", yyjson::required)), yyjson::then(yyjson::field("b", yyjson::required))));
VISITABLE_STRUCT(ConditionVisitable, a, b);
// fields whose keys a condition tests while their values cannot tell: a std::optional with an initializer, a plain
// field, and fields with yyjson::optional_key and yyjson::omit_default; reading tells by the keys of the object
struct ConditionPresence
{
    std::optional<int> a = 5;
    int b = 0;
    int c = 7;
    int e = 0;
    std::optional<int> d;
};
template <>
inline constexpr auto yyjson::annotations<ConditionPresence> = yyjson::annotate(
    yyjson::field("c", yyjson::optional_key), yyjson::field("e", yyjson::omit_default),
    yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::required)),
                         yyjson::then(yyjson::field("d", yyjson::required))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("b", yyjson::required)),
                         yyjson::then(yyjson::field("d", yyjson::required))),
    yyjson::not_(yyjson::field("c", yyjson::required), yyjson::field("e", yyjson::required)));

// contains(is_type<T>) on the elements of DOM values: two events read as ConditionPoint, at most one object that
// is one, an element of a yyjson::array, and in then
struct TypedContains
{
    std::vector<yyjson::value> events;
    std::vector<yyjson::object> objects;
    yyjson::array numbers;
    std::optional<std::string> kind;
    std::optional<std::vector<yyjson::value>> points;
};
template <>
inline constexpr auto yyjson::annotations<TypedContains> = yyjson::annotate(
    yyjson::field("events", yyjson::contains(yyjson::is_type<ConditionPoint>), yyjson::min_contains(2)),
    yyjson::field("objects", yyjson::contains(yyjson::is_type<ConditionPoint>), yyjson::max_contains(1)),
    yyjson::field("numbers", yyjson::contains(yyjson::is_type<int>)),
    yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("point"), yyjson::required)),
                         yyjson::then(yyjson::field("points", yyjson::contains(yyjson::is_type<ConditionPoint>)))));
// conditions on the elements of arrays: a reason when "deprecated" is among the tags once, and no two integers
// among the dependencies
struct ContainsConditions
{
    std::vector<std::string> tags;
    std::optional<std::string> reason;
    std::optional<std::vector<yyjson::value>> deps;
};
template <>
inline constexpr auto yyjson::annotations<ContainsConditions> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::field("tags", yyjson::required,
                                                   yyjson::contains(yyjson::const_("deprecated")),
                                                   yyjson::max_contains(1))),
                         yyjson::then(yyjson::field("reason", yyjson::required))),
    yyjson::not_(
        yyjson::field("deps", yyjson::required, yyjson::contains(yyjson::is_type<int>), yyjson::min_contains(2))));

// additional_properties(false) in then and else_: a circle has only a kind and a radius, another shape only a kind
// and a width, even among the keys that would go to the map of yyjson::extra_keys
struct ClosedShape
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<int> w;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<ClosedShape> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys),
    yyjson::if_then_else(
        yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
        yyjson::then(yyjson::field("kind"), yyjson::field("r", yyjson::required, yyjson::minimum(0)),
                     yyjson::additional_properties(false)),
        yyjson::else_(yyjson::field("kind"), yyjson::field("w"), yyjson::additional_properties(false))));
// the keys of a map of yyjson::pattern_properties, and then without fields; additional_properties(true) changes nothing
struct ClosedPatterns
{
    std::optional<int> a;
    std::map<std::string, int> xs;
};
template <>
inline constexpr auto yyjson::annotations<ClosedPatterns> = yyjson::annotate(
    yyjson::field("xs", yyjson::pattern_properties("^x")),
    yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::maximum(0))),
                         yyjson::then(yyjson::additional_properties(false)),
                         yyjson::else_(yyjson::field("a"), yyjson::additional_properties(true))));
// the keys of tags are allowed: an adjacent tag, and the internal tag of an alternative
struct ClosedAdjacent
{
    std::variant<int, std::string> data;
    std::optional<int> n;
};
template <>
inline constexpr auto yyjson::annotations<ClosedAdjacent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})),
                     yyjson::if_then_else(yyjson::if_(yyjson::field("n", yyjson::required)),
                                          yyjson::then(yyjson::field("n"), yyjson::field("data"),
                                                       yyjson::additional_properties(false))));
struct ClosedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<ClosedAlternative> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::required)),
                                          yyjson::then(yyjson::field("a"), yyjson::additional_properties(false))));
using ClosedVariant = std::variant<ClosedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<ClosedVariant> = yyjson::annotate(yyjson::internal_tag("type", {"c", "p"}));

// the nodes of the settings are counted from the types, and a struct without settings of the values in its fields
// takes none
static_assert(yyjson::detail::node_count<decltype(yyjson::items(yyjson::items(yyjson::minimum(0))))>.nodes == 2);
static_assert(yyjson::detail::value_node_count<Bounded> == yyjson::detail::tree_size{});

struct CountsInverted
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<CountsInverted> =
    yyjson::annotate(yyjson::min_properties(3), yyjson::max_properties(2));
struct CountsTwice
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<CountsTwice> =
    yyjson::annotate(yyjson::max_properties(3), yyjson::max_properties(2));
struct CountsEqual
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<CountsEqual> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::max_properties(2));

// The rules that the static assertions enforce, tested on the conditions that they assert: the error of the table of
// the keys of a struct that is never read, and the concepts of the types that a setting constrains.
template <typename T>
constexpr auto table_error = yyjson::detail::make_field_key_table<T, yyjson::detail::reflected_field_names<T>>().error;
#define CONSTRAINT_RULE(NAME, ...)       \
    struct NAME                          \
    {                                    \
        std::vector<std::vector<int>> v; \
    };                                   \
    template <>                          \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(yyjson::field("v", __VA_ARGS__));
// min_contains and max_contains need contains at their level, also inside items and in a position
CONSTRAINT_RULE(MinContainsAlone, yyjson::min_contains(1))
CONSTRAINT_RULE(MaxContainsInItems, yyjson::items(yyjson::max_contains(1)))
CONSTRAINT_RULE(MaxContainsInPosition, yyjson::prefix_items({yyjson::max_contains(1)}))
CONSTRAINT_RULE(ContainsBounded, yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(0), yyjson::max_contains(0))
// a setting twice at one level, also inside items, and the same setting at two levels
CONSTRAINT_RULE(UniqueTwice, yyjson::unique_items, yyjson::unique_items)
CONSTRAINT_RULE(MinimumTwiceInItems, yyjson::items(yyjson::minimum(0), yyjson::minimum(1)))
CONSTRAINT_RULE(ContainsTwice, yyjson::contains(yyjson::minimum(0)), yyjson::contains(yyjson::maximum(1)))
CONSTRAINT_RULE(ItemsTwice, yyjson::items(yyjson::unique_items), yyjson::items(yyjson::minimum(0)))
CONSTRAINT_RULE(UniqueAtTwoLevels, yyjson::unique_items, yyjson::items(yyjson::unique_items))
// is_type is a setting of the elements of contains once
CONSTRAINT_RULE(IsTypeTwiceInContains, yyjson::contains(yyjson::is_type<int>, yyjson::is_type<long>))
// a position holds up to 8 nodes: one for itself and one for each items, contains, property_names and
// additional_properties in it, counted here in breadth because MSVC 19.44 limits the depth of the initializers
#define POSITION_OF_8                                                                      \
    yyjson::items(yyjson::items(yyjson::minimum(0)), yyjson::contains(yyjson::minimum(0)), \
                  yyjson::property_names(yyjson::min_length(1))),                          \
        yyjson::contains(yyjson::items(yyjson::minimum(0))), yyjson::property_names(yyjson::min_length(1))
CONSTRAINT_RULE(PositionOf8, yyjson::prefix_items({POSITION_OF_8}))
CONSTRAINT_RULE(PositionOf9, yyjson::prefix_items({POSITION_OF_8, yyjson::additional_properties(yyjson::minimum(0))}))
#undef POSITION_OF_8
CONSTRAINT_RULE(MultipleOfZeroInItems, yyjson::items(yyjson::multiple_of(0)))
// const_ and enum_ once at a level, and up to 16 values in a position
CONSTRAINT_RULE(ConstTwice, yyjson::items(yyjson::items(yyjson::const_(1), yyjson::const_(2))))
CONSTRAINT_RULE(EnumTwice, yyjson::items(yyjson::items(yyjson::enum_(1), yyjson::enum_(2))))
CONSTRAINT_RULE(FormatTwice, yyjson::items(yyjson::format("date"), yyjson::format("time")))
CONSTRAINT_RULE(PositionOf16Values,
                yyjson::prefix_items({yyjson::items(yyjson::const_(0),
                                                    yyjson::enum_(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                                                        14, 15))}))
CONSTRAINT_RULE(PositionOf17Values,
                yyjson::prefix_items({yyjson::items(yyjson::const_(0),
                                                    yyjson::enum_(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                                                        14, 15, 16))}))
#undef CONSTRAINT_RULE
#if !defined(__cpp_lib_define_static)
// before C++26 a position copies up to 256 characters of the strings written as string literals
#define STRING_RULE(NAME, ...)        \
    struct NAME                       \
    {                                 \
        std::vector<std::string> v;   \
    };                                \
    template <>                       \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(yyjson::field("v", __VA_ARGS__));
#define CHARS_64 "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
STRING_RULE(PositionOf256Chars, yyjson::prefix_items({yyjson::pattern(CHARS_64 CHARS_64),
                                                      yyjson::enum_(CHARS_64, CHARS_64)}))
STRING_RULE(PositionOf257Chars, yyjson::prefix_items({yyjson::pattern(CHARS_64 CHARS_64),
                                                      yyjson::enum_(CHARS_64, CHARS_64 "x")}))
inline constexpr std::string_view chars_257 = CHARS_64 CHARS_64 CHARS_64 CHARS_64 "x";
STRING_RULE(PositionOfLongView, yyjson::prefix_items(yyjson::const_(chars_257)))
#undef CHARS_64
#undef STRING_RULE
#endif
// dependent_required names a member that is read, and once
struct DependsOnUnknown
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<DependsOnUnknown> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}));
struct DependentUnknown
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<DependentUnknown> =
    yyjson::annotate(yyjson::dependent_required("b", {"a"}));
struct DependsOnSkipped
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<DependsOnSkipped> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}), yyjson::field("b", yyjson::skip));
struct DependsTwice
{
    int a = 0;
    int b = 0;
    int c = 0;
};
template <>
inline constexpr auto yyjson::annotations<DependsTwice> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}), yyjson::dependent_required("b", {"a", "c"}));
// one_of_required and any_of_required name members that are read
struct GroupOfUnknown
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<GroupOfUnknown> = yyjson::annotate(yyjson::one_of_required({"a"}, {"b"}));
struct GroupOfSkipped
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<GroupOfSkipped> =
    yyjson::annotate(yyjson::any_of_required({"a"}, {"b"}), yyjson::field("b", yyjson::skip));
// a field that a condition names has a key of its own, and is_type is a setting of then and else_
#define CONDITION_RULE(NAME, SETTING, ...)                                                                        \
    struct NAME                                                                                                   \
    {                                                                                                             \
        std::optional<int> a;                                                                                     \
        std::map<std::string, int> m;                                                                             \
    };                                                                                                            \
    template <>                                                                                                   \
    inline constexpr auto yyjson::annotations<NAME> =                                                             \
        yyjson::annotate(yyjson::field("a", SETTING), __VA_ARGS__ __VA_OPT__(, )                                  \
                         yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::required)),                  \
                                              yyjson::then(yyjson::field("a", yyjson::minimum(1)))));
CONDITION_RULE(ConditionOnSkipped, yyjson::skip)
CONDITION_RULE(ConditionOnExtraKeys, yyjson::rename("b"), yyjson::field("m", yyjson::extra_keys),
               yyjson::not_(yyjson::field("m", yyjson::required)))
CONDITION_RULE(ConditionWithIsType, yyjson::is_type<int>)
CONDITION_RULE(ConditionTransparent, yyjson::minimum(0), yyjson::field("m", yyjson::skip), yyjson::transparent)
#undef CONDITION_RULE
struct ConditionOnUnknown
{
    std::optional<int> a;
};
template <>
inline constexpr auto yyjson::annotations<ConditionOnUnknown> =
    yyjson::annotate(yyjson::not_(yyjson::field("b", yyjson::required)));
struct ConditionTwice
{
    std::optional<int> a;
};
template <>
inline constexpr auto yyjson::annotations<ConditionTwice> =
    yyjson::annotate(yyjson::not_(yyjson::field("a", yyjson::minimum(0), yyjson::minimum(1))));
namespace compile_rules
{
    using enum yyjson::detail::field_key_error;
    static_assert(table_error<MinContainsAlone> == contains_missing);
    static_assert(table_error<MaxContainsInItems> == contains_missing);
    static_assert(table_error<MaxContainsInPosition> == contains_missing);
    static_assert(table_error<ContainsBounded> == none);
    static_assert(table_error<UniqueTwice> == same_setting_twice);
    static_assert(table_error<MinimumTwiceInItems> == same_setting_twice);
    static_assert(table_error<ContainsTwice> == same_setting_twice);
    static_assert(table_error<ItemsTwice> == same_setting_twice);
    static_assert(table_error<UniqueAtTwoLevels> == none);
    static_assert(table_error<IsTypeTwiceInContains> == same_setting_twice);
    static_assert(table_error<PositionOf8> == none);
    static_assert(table_error<PositionOf9> == prefix_position_too_large);
    static_assert(table_error<MultipleOfZeroInItems> == multiple_of_not_positive);
    static_assert(table_error<ConstTwice> == same_setting_twice);
    static_assert(table_error<EnumTwice> == same_setting_twice);
    static_assert(table_error<FormatTwice> == same_setting_twice);
    static_assert(table_error<PositionOf16Values> == none);
    static_assert(table_error<PositionOf17Values> == prefix_position_too_large);
#if !defined(__cpp_lib_define_static)
    static_assert(table_error<PositionOf256Chars> == none);
    static_assert(table_error<PositionOf257Chars> == prefix_position_too_large);
    static_assert(table_error<PositionOfLongView> == none);
#endif
    static_assert(table_error<DependsOnUnknown> == unknown_member);
    static_assert(table_error<DependentUnknown> == unknown_member);
    static_assert(table_error<DependsOnSkipped> == dependency_skipped);
    static_assert(table_error<DependsTwice> == none);
    static_assert(table_error<GroupOfUnknown> == unknown_member);
    static_assert(table_error<GroupOfSkipped> == dependency_skipped);
    static_assert(table_error<Reachable> == none);
    static_assert(table_error<ConditionOnSkipped> == condition_field_kind);
    static_assert(table_error<ConditionPresence> == none);
    static_assert(table_error<CountsInverted> == min_properties_above_max);
    static_assert(table_error<CountsTwice> == additional_twice);
    static_assert(table_error<CountsEqual> == none);
    static_assert(table_error<ConditionOnExtraKeys> == condition_field_kind);
    static_assert(table_error<ConditionWithIsType> == is_type_outside_condition);
    static_assert(table_error<ConditionTransparent> == condition_transparent);
    static_assert(table_error<ConditionOnUnknown> == unknown_member);
    static_assert(table_error<ConditionShape> == none && table_error<Leveled> == none);
    static_assert(yyjson::detail::make_condition_table<ConditionTwice>().tree.error == same_setting_twice);

    using namespace yyjson::detail;
    using string_map = std::map<std::string, int>;
    using pairs = std::vector<std::pair<std::string, int>>;

    // unique_items: the ranges whose elements may be equal
    static_assert(unique_keys<std::set<int>> && unique_keys<string_map> && unique_keys<std::unordered_set<int>> &&
                  unique_keys<std::unordered_map<std::string, int>>);
    static_assert(!unique_keys<std::multiset<int>> && !unique_keys<std::multimap<std::string, int>> &&
                  !unique_keys<std::vector<int>>);
    static_assert(!constraint_items<std::string> && constraint_items<std::vector<int>>);

    // items and contains: the ranges read from JSON arrays, including the maps whose keys are not strings
    static_assert(constraint_item_range<std::vector<int>> && constraint_item_range<std::set<int>> &&
                  constraint_item_range<std::map<int, int>>);
    static_assert(!constraint_item_range<string_map> && !constraint_item_range<pairs> &&
                  !constraint_item_range<std::string> && !constraint_item_range<std::tuple<int, int>>);

    // prefix_items: the tuples, and the ranges read from JSON arrays in their order
    static_assert(constraint_prefix_target<std::tuple<int, std::string>> &&
                  constraint_prefix_target<std::pair<int, int>> && constraint_prefix_target<std::vector<int>> &&
                  constraint_prefix_target<std::array<int, 2>> && constraint_prefix_target<std::list<int>>);
    static_assert(!constraint_prefix_target<std::set<int>> && !constraint_prefix_target<std::map<int, int>> &&
                  !constraint_prefix_target<string_map> && !constraint_prefix_target<std::string>);

    // additional_properties(settings...): the maps, and the ranges of pairs read from JSON objects
    static_assert(constraint_value_range<string_map> && constraint_value_range<std::map<int, int>> &&
                  constraint_value_range<pairs>);
    static_assert(!constraint_value_range<std::vector<std::pair<int, int>>> &&
                  !constraint_value_range<std::vector<int>>);

    // property_names: the maps and the ranges of pairs read from JSON objects
    static_assert(constraint_key_range<string_map> && constraint_key_range<std::unordered_map<std::string, int>> &&
                  constraint_key_range<pairs>);
    static_assert(!constraint_key_range<std::map<int, int>> && !constraint_key_range<std::vector<int>>);

    // the settings inside items and the others: those of a value, not those of a key
    static_assert(element_setting<yyjson::minimum> && element_setting<yyjson::unique_items_t> &&
                  element_setting<decltype(yyjson::items(yyjson::minimum(0)))>);
    static_assert(!element_setting<yyjson::rename> && !element_setting<yyjson::required_t> &&
                  !element_setting<yyjson::skip_t>);

    // if_ and not_ test one value or the presence of a key; then and else_ take every setting of the values too
    static_assert(condition_test_setting<yyjson::required_t> && condition_test_setting<yyjson::minimum> &&
                  condition_test_setting<yyjson::pattern<>> && condition_test_setting<yyjson::format<>>);
    static_assert(condition_test_setting<decltype(yyjson::contains(yyjson::minimum(0)))> &&
                  condition_test_setting<yyjson::min_contains> && condition_test_setting<yyjson::max_contains>);
    static_assert(!condition_test_setting<yyjson::min_items> && !condition_test_setting<yyjson::unique_items_t> &&
                  !condition_test_setting<yyjson::satisfies_t> &&
                  !condition_test_setting<decltype(yyjson::items(yyjson::minimum(0)))> &&
                  condition_test_setting<yyjson::is_type_t<int>> && !condition_test_setting<yyjson::rename>);
    static_assert(condition_assert_setting<yyjson::min_items> && condition_assert_setting<yyjson::is_type_t<int>> &&
                  condition_assert_setting<decltype(yyjson::items(yyjson::minimum(0)))>);
    static_assert(!condition_assert_setting<yyjson::rename> && !condition_assert_setting<yyjson::skip_t>);

    // not_ of settings holds the settings of one value, without element trees, and stands where they stand except
    // in the tests of if_ and not_
    using not_minimum = decltype(yyjson::not_(yyjson::minimum(0)));
    static_assert(not_setting<yyjson::minimum> && not_setting<decltype(yyjson::const_(3))> &&
                  not_setting<decltype(yyjson::pattern("^x"))> && not_setting<yyjson::min_items> &&
                  not_setting<yyjson::is_type_t<int>>);
    static_assert(!not_setting<yyjson::satisfies_t> && !not_setting<yyjson::unique_items_t> &&
                  !not_setting<yyjson::min_contains> && !not_setting<not_minimum> &&
                  !not_setting<decltype(yyjson::items(yyjson::minimum(0)))> && !not_setting<yyjson::rename>);
    static_assert(element_setting<not_minimum> && condition_assert_setting<not_minimum> &&
                  !condition_test_setting<not_minimum>);
    static_assert(holds_type_test<decltype(yyjson::items(yyjson::not_(yyjson::is_type<int>)))>::value);

    // contains(is_type<T>) tests the DOM values, which items does not take, and no position of prefix_items holds it at any depth
    static_assert(dom_kind_of<yyjson::value> == dom_kind::value && dom_kind_of<yyjson::object> == dom_kind::object &&
                  dom_kind_of<yyjson::writer::const_value_ref> == dom_kind::value_ref &&
                  dom_kind_of<std::optional<yyjson::value>> == dom_kind::none && dom_kind_of<int> == dom_kind::none);
    static_assert(!element_setting<yyjson::is_type_t<int>>);
    static_assert(holds_type_test<decltype(yyjson::items(yyjson::contains(yyjson::is_type<int>)))>::value &&
                  !holds_type_test<decltype(yyjson::items(yyjson::contains(yyjson::minimum(0))))>::value);
}  // namespace compile_rules

// Members of an object routed to map fields by the patterns of their keys
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
// every field is a map, and the keys that no pattern matches go to the field of `yyjson::extra_keys`
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
// a struct that accepts unknown keys under strict ignores the keys that no pattern matches
struct PatternOpen
{
    int a = 0;
    std::map<std::string, int> p;
    bool operator==(const PatternOpen&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternOpen> =
    yyjson::annotate(yyjson::additional_properties(true), yyjson::field("p", yyjson::pattern_properties("^p")));
// a pattern that matches the key of a tag checks the value of the tag
struct PatternTagShort
{
    int x = 0;
    std::map<std::string, std::string> m;
    bool operator==(const PatternTagShort&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternTagShort> = yyjson::annotate(yyjson::field(
    "m", yyjson::pattern_properties("^k"), yyjson::additional_properties(yyjson::max_length(1))));
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

// The rules that the static assertions of `yyjson::pattern_properties` enforce
#define PATTERN_RULE(NAME, ...)       \
    struct NAME                       \
    {                                 \
        int v = 0;                    \
        std::map<std::string, int> m; \
        std::map<std::string, int> n; \
    };                                \
    template <>                       \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(__VA_ARGS__);
PATTERN_RULE(PatternRenamed, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::rename("k")))
PATTERN_RULE(PatternRequired, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::required))
PATTERN_RULE(PatternCounted, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::min_items(1)))
PATTERN_RULE(PatternDescribed, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::description("d")))
PATTERN_RULE(PatternExtra, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::extra_keys))
PATTERN_RULE(PatternTwice, yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::pattern_properties("^b")))
PATTERN_RULE(PatternSkipped, yyjson::field("m", yyjson::skip, yyjson::pattern_properties("^a")))
PATTERN_RULE(PatternDependency, yyjson::dependent_required("v", {"m"}),
             yyjson::field("m", yyjson::pattern_properties("^a")))
PATTERN_RULE(PatternSettings,
             yyjson::field("m", yyjson::pattern_properties("^a"), yyjson::additional_properties(yyjson::minimum(0)),
                           yyjson::property_names(yyjson::max_length(3))),
             yyjson::field("n", yyjson::pattern_properties("^a")))
#undef PATTERN_RULE
namespace pattern_rules
{
    template <typename T>
    constexpr auto table_error =
        yyjson::detail::make_field_key_table<T, yyjson::detail::reflected_field_names<T>>().error;
    using enum yyjson::detail::field_key_error;
    static_assert(table_error<PatternRenamed> == pattern_with_setting);
    static_assert(table_error<PatternRequired> == pattern_with_setting);
    static_assert(table_error<PatternCounted> == pattern_with_setting);
    static_assert(table_error<PatternDescribed> == pattern_with_setting);
    static_assert(table_error<PatternExtra> == pattern_with_setting);
    static_assert(table_error<PatternTwice> == same_setting_twice);
    static_assert(table_error<PatternSkipped> == skipped_with_setting);
    static_assert(table_error<PatternDependency> == dependency_skipped);
    static_assert(table_error<PatternSettings> == none);
}  // namespace pattern_rules

#if FIELD_RFL_ANNOTATIONS
struct ElementsAnnotated
{
    [[= yyjson::items(yyjson::minimum(0))]] std::vector<int> v;
    [[= yyjson::additional_properties(
        yyjson::items(yyjson::max_length(2)))]] std::map<std::string, std::vector<std::string>>
        m;
    bool operator==(const ElementsAnnotated&) const = default;
};

struct ElementsAnnotated2
{
    [[= yyjson::prefix_items({yyjson::minimum(0)}, {})]] std::vector<int> p;
    [[= yyjson::unique_items]] std::vector<int> u;
    [[= yyjson::property_names(yyjson::max_length(2))]] std::map<std::string, int> k;
    [[ = yyjson::contains(yyjson::minimum(5)), = yyjson::min_contains(2) ]] std::vector<int> c;
    bool operator==(const ElementsAnnotated2&) const = default;
};

struct TypedContainsAnnotated
{
    [[ = yyjson::contains(yyjson::is_type<ConditionPoint>), = yyjson::min_contains(2) ]] std::vector<yyjson::value> v;
};

struct NotAnnotated
{
    [[ = yyjson::not_(yyjson::minimum(10), yyjson::maximum(19)), = yyjson::not_(yyjson::const_(3)) ]] int n = 0;
    [[= yyjson::items(yyjson::not_(yyjson::is_type<std::nullptr_t>))]] std::vector<yyjson::value> d;
};

struct [[= yyjson::dependent_required("card", {"address"})]] KeywordsAnnotated
{
    [[= yyjson::const_(2)]] int version = 2;
    [[= yyjson::enum_("a", "b")]] std::string unit = "a";
    [[= yyjson::format("date")]] std::string day = "2024-01-01";
    [[= yyjson::items(yyjson::enum_(1.5, 2))]] std::vector<double> steps;
    std::optional<std::string> card;
    std::optional<std::string> address;
    bool operator==(const KeywordsAnnotated&) const = default;
};

struct [[ = yyjson::one_of_required({"a"}, {"b"}), = yyjson::any_of_required({"a", "c"}, {"b"}) ]] GroupsAnnotated
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
};

struct [[ = yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
                                 yyjson::then(yyjson::field("r", yyjson::required, yyjson::minimum(0)))),
           = yyjson::not_(yyjson::field("kind", yyjson::enum_("x", "y"))) ]] ConditionsAnnotated
{
    std::optional<std::string> kind;
    std::optional<double> r;
};

struct [[ = yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
                                 yyjson::then(yyjson::field("kind"), yyjson::field("r"),
                                              yyjson::additional_properties(false))) ]] ClosedAnnotated
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<double> w;
};

struct ConstrainedAnnotated
{
    [[ = yyjson::minimum(1), = yyjson::maximum(3) ]] int n = 1;
    [[ = yyjson::pattern("^p"), = yyjson::pattern("q$") ]] std::string p = "pq";
    [[= yyjson::pattern("^x")]] std::string s = "x";
    [[= yyjson::satisfies<is_even>]] int even = 0;
    bool operator==(const ConstrainedAnnotated&) const = default;
};

struct PatternAnnotated
{
    int ab = 0;
    [[ = yyjson::pattern_properties("^a"), = yyjson::property_names(yyjson::max_length(3)) ]] std::map<std::string, int> a;
    [[ = yyjson::pattern_properties("b$"), = yyjson::additional_properties(yyjson::minimum(10)) ]] std::map<std::string, int> b;
    bool operator==(const PatternAnnotated&) const = default;
};
#endif

// a bound is compared as a double, so a long double that it would round does not compile; Clang 17 and older
// take no floating-point template argument
#if !defined(__clang__) || __clang_major__ >= 18
template <auto V>
concept bound_accepted = requires { typename std::integral_constant<bool, (yyjson::minimum(V), true)>; };
static_assert(bound_accepted<1.5L> && bound_accepted<9007199254740992.0L>);
static_assert(!bound_accepted<9007199254740993.0L> || sizeof(long double) == sizeof(double));
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
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
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

TEST(Constraints, FloatBounds)
{
    expect_accepted<FloatBounds>(R"({"tenth":0.1,"under":0.099999994,"big":16777217})",
                                 {0.1F, 0.099999994F, 16777216.0F});
    expect_rejected<FloatBounds>(
        R"({"tenth":0.10000001})",
        R"(the value of the key "tenth" of FloatBounds is greater than the maximum 0.1)", "/tenth");
    expect_rejected<FloatBounds>(
        R"({"under":0.1})",
        R"(the value of the key "under" of FloatBounds is not less than the exclusive maximum 0.1)", "/under");
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

TEST(Constraints, Patterns)
{
    expect_accepted<Patterns>(R"({"s":"abz","o":"oxo","list":["ab"],"m":{"k1":1},"n":"ab"})",
                              {"abz", "oxo", {"ab"}, {{"k1", 1}}, "ab"});
    expect_accepted<Patterns>(R"({"s":"az"})", {"az", {}, {}, {}, {}});

    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Patterns does not match all of the patterns )", key);
    };
    expect_rejected<Patterns>(R"({"s":"ab"})", value("s") + R"("^a", "z$")", "/s");
    expect_rejected<Patterns>(R"({"s":"bz"})", value("s") + R"("^a", "z$")", "/s");
    expect_rejected<Patterns>(R"({"s":"az","o":"oo"})", value("o") + R"("^o", "o$", "x")", "/o");
    expect_rejected<Patterns>(R"({"s":"az","o":"xo"})", value("o") + R"("^o", "o$", "x")", "/o");
    expect_rejected<Patterns>(R"({"s":"az","o":"o"})",
                              R"(the value of the key "o" of Patterns is shorter than the minimum length 2)", "/o");
    expect_rejected<Patterns>(
        R"({"s":"az","list":["ab","az"]})",
        R"(an element of the value of the key "list" of Patterns does not match all of the patterns "^a", "b$")",
        "/list/1");
    expect_rejected<Patterns>(R"({"s":"az","list":["b"]})",
                              R"(an element of the value of the key "list" of Patterns does not match all of the )"
                              R"(patterns "^a", "b$")",
                              "/list/0");
    expect_rejected<Patterns>(
        R"({"s":"az","m":{"k1":1,"kx":2}})",
        R"(a key of the value of the key "m" of Patterns does not match all of the patterns "^k", "[0-9]$")", "/m/kx");
    expect_rejected<Patterns>(R"({"s":"az","n":"abz"})", R"(the value of the key "n" of Patterns )", "/n");

    // validate and write with yyjson::strict check them as well
    const auto rejected = [](const Patterns& t, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of([&] { yyjson::validate(t); });
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
        EXPECT_EQ(e.path(), path);
        EXPECT_THROW((void)yyjson::write(t, yyjson::strict), yyjson::bad_cast);
    };
    EXPECT_NO_THROW(yyjson::validate(Patterns{"abz", "oxo", {"ab"}, {{"k1", 1}}, "ab"}));
    rejected(Patterns{"ab"}, value("s") + R"("^a", "z$")", "/s");
    rejected(Patterns{"az", "oxx"}, value("o") + R"("^o", "o$", "x")", "/o");
    rejected(Patterns{"az", {}, {"ab", "b"}},
             R"(an element of the value of the key "list" of Patterns does not match all of the patterns)", "/list/1");
    rejected(Patterns{"az", {}, {}, {{"k", 1}}},
             R"(a key of the value of the key "m" of Patterns does not match all of the patterns)", "/m/k");
}

TEST(Constraints, Validate)
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
    expect_rejected<Elements>(R"({"uset":[2,4,6,3,8]})", prefix("uset") + "is rejected by yyjson::satisfies", "/uset/3");
    // the values of a map, and the constraints of the map itself
    expect_rejected<Elements>(R"({"limits":{"a":1,"b":101}})", prefix("limits") + "is greater than the maximum 100",
                              "/limits/b");
    expect_rejected<Elements>(R"({"limits":{"a":1,"b":2,"c":3,"d":4}})",
                              R"(the value of the key "limits" of Elements has more items than the maximum 3)",
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
}

TEST(Constraints, Nullables)
{
    auto expected = Nullables();
    expected.ids = {nullptr, std::make_shared<int>(1)};
    expect_accepted<Nullables>(R"({"code":null,"ratio":null,"name":null,"list":null,"ids":[null,1]})", expected);
    expected.code = "abc";
    expected.ratio = 1.0;
    expected.name = std::make_shared<std::string>("abc");
    expected.list = std::make_shared<std::vector<int>>(std::vector{9});
    expected.ids = {std::make_shared<int>(1), std::make_shared<int>(2)};
    expect_accepted<Nullables>(R"({"code":"abc","ratio":1.0,"name":"abc","list":[9],"ids":[1,2]})", expected);

    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Nullables )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Nullables )", key);
    };
    expect_rejected<Nullables>(R"({"code":"abcd"})", field("code") + "is longer than the maximum length 3", "/code");
    expect_rejected<Nullables>(R"({"ratio":1.5})", field("ratio") + "is greater than the maximum 1", "/ratio");
    expect_rejected<Nullables>(R"({"name":"abcd"})", field("name") + "is rejected by yyjson::satisfies", "/name");
    expect_rejected<Nullables>(R"({"list":[]})", field("list") + "has fewer items than the minimum 1", "/list");
    expect_rejected<Nullables>(R"({"list":[1,10]})", element("list") + "is greater than the maximum 9", "/list/1");
    // smart pointers compare the values that they hold, and null is equal to null
    expect_rejected<Nullables>(R"({"ids":[1,2,1]})", element("ids") + "is equal to an earlier element", "/ids/2");
    expect_rejected<Nullables>(R"({"ids":[null,1,null]})", element("ids") + "is equal to an earlier element",
                               "/ids/2");

    // std::unique_ptr, in a field, in a std::optional and in the elements
    const auto owned = yyjson::read<Owned>(R"({"count":null,"level":null,"tags":[null,"abc"]})");
    // null into the std::optional gives an empty pointer, which the pointer reads null as
    EXPECT_EQ(owned.count, nullptr);
    EXPECT_EQ(owned.level, std::optional<std::unique_ptr<int>>(nullptr));
    EXPECT_EQ(written(owned), R"({"count":null,"level":null,"tags":[null,"abc"]})");
    EXPECT_EQ(written(yyjson::read<Owned>(R"({"count":0,"level":5,"tags":["a","b"]})")),
              R"({"count":0,"level":5,"tags":["a","b"]})");
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
    expect_owned_rejected(R"({"tags":[null,"abcd"]})",
                          R"(an element of the value of the key "tags" of Owned is longer than the maximum length 3)",
                          "/tags/1");
    expect_owned_rejected(R"({"tags":["a",null,"a"]})",
                          R"(an element of the value of the key "tags" of Owned is equal to an earlier element)",
                          "/tags/2");
}

TEST(Constraints, NullableListed)
{
    // a key that the object leaves out is not checked
    expect_accepted<NullableListed>("{}", {});
    auto expected = NullableListed();
    expected.unit = "mm";
    expected.count = std::make_shared<int>(1);
    expected.steps = {1, 2};
    expected.note = "x";
    expect_accepted<NullableListed>(
        R"({"unit":"mm","level":null,"count":1,"steps":[1,2],"note":"x","none":null})", expected);
    // merge_into takes null as the removal of the key, and the others read monostate
    EXPECT_EQ(yyjson::read<NullableListed>(R"({"mode":null})").mode,
              std::make_optional(std::variant<std::monostate, int>()));
    expected = NullableListed();
    expected.level = 2;
    expect_accepted<NullableListed>(R"({"level":2})", expected);

    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of NullableListed )", key);
    };
    expect_rejected<NullableListed>(R"({"level":3})", field("level") + "is not one of 1, 2, null", "/level");
    expect_rejected<NullableListed>(R"({"steps":[1,null]})",
                                    R"(an element of the value of the key "steps" of NullableListed is not one of 1, 2)",
                                    "/steps/1");
    expect_rejected<NullableListed>(R"({"mode":1})", field("mode") + "is not equal to the constant null", "/mode");
    expect_rejected<NullableListed>(R"({"none":[]})", field("none") + "is not equal to the constant null", "/none");
    // null of a field, which merge_into takes as the removal of the key
    const auto expect_null_rejected = [](std::string_view json, std::string_view message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<NullableListed>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<NullableListed>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = NullableListed();
            yyjson::read_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<NullableListed>(); }));
        auto t = NullableListed();
        yyjson::merge_into(json, t);
        EXPECT_EQ(t, NullableListed()) << json;
    };
    expect_null_rejected(R"({"unit":null})", field("unit") + R"(is not one of "mm", "cm")", "/unit");
    expect_null_rejected(R"({"count":null})", field("count") + "is not equal to the constant 1", "/count");
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

    // JSON values compare as JSON: numbers by value, objects regardless of the order of the members
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
                                        R"("maps":[{"a":1,"b":2},{"a":1}]})")
                  .boxes.size(),
              3);
    expect_rejected<UniqueParts>(R"({"boxes":[{"p":1},{"p":1}]})", parts("boxes"), "/boxes/1");
    expect_rejected<UniqueParts>(R"({"boxes":[{"p":null},{"p":2},{"p":null}]})", parts("boxes"), "/boxes/2");
    expect_rejected<UniqueParts>(R"({"loose":[{"v":1},{"v":1}]})", parts("loose"), "/loose/1");
    expect_rejected<UniqueParts>(
        R"({"trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[{"v":2,"kids":[]}]}]})", parts("trees"),
        "/trees/1");
    expect_rejected<UniqueParts>(R"({"maps":[{"a":1,"b":2,"c":3},{"c":3,"b":2,"a":1}]})", parts("maps"), "/maps/1");
    // a range of key-value pairs is written as an object, whose members have no order
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"a":1,"b":3}]})").pairs.size(), 2);
    expect_rejected<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"b":2,"a":1}]})", parts("pairs"), "/pairs/1");
    // a range of tuples is sorted from 17 elements on
    auto tuples = std::string(R"({"tuples":[)");
    for (auto i = 0; i < 20; ++i) tuples += std::format(R"([{},"{}"],)", i % 2, i);
    EXPECT_EQ(yyjson::read<UniqueParts>(tuples.substr(0, tuples.size() - 1) + "]}").tuples.size(), 20);
    expect_rejected<UniqueParts>(tuples + R"([1,"7"]]})", parts("tuples"), "/tuples/20");
}

TEST(Constraints, PrefixItems)
{
    expect_accepted<Prefixed>(R"({"row":[0,"ab",1.5],"range":[5,5],"head":[0,10],"arr":[0,1,0],)"
                              R"("grid":[[1],[0,0],[0]]})",
                              {{0, "ab", 1.5}, {5, 5}, {0, 10}, {0, 1, 0}, {{1}, {0, 0}, {0}}});
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
}

TEST(Constraints, StringLiterals)
{
    expect_accepted<Literals>(R"({"pair":["ab",["yz"]],"unit":"cm","days":["2024-01-31"]})",
                              {{"ab", {"yz"}}, "cm", {"2024-01-31"}});
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Literals )", key);
    };
    expect_rejected<Literals>(R"({"pair":["Ab",[]]})", element("pair") + R"(does not match the pattern "^[a-z]+$")",
                              "/pair/0");
    expect_rejected<Literals>(R"({"pair":["abd",[]]})", element("pair") + R"(is not one of "ab", "Ab", "abc")",
                              "/pair/0");
    expect_rejected<Literals>(R"({"pair":["ab",["y"]]})", element("pair") + R"(is not equal to the constant "yz")",
                              "/pair/1/0");
    expect_rejected<Literals>(R"({"unit":"m"})", R"(the value of the key "unit" of Literals is not one of "mm", "cm")",
                              "/unit");
    expect_rejected<Literals>(R"({"days":["2024-02-30"]})", element("days") + R"(does not have the format "date")",
                              "/days/0");
}

TEST(Constraints, PropertyNames)
{
    expect_accepted<Keyed>(R"({"m":{"ab":1},"u":{"x":1},"deep":{"x":{"ab":1}}})",
                           {{{"ab", 1}}, {{"x", 1}}, {{"x", {{"ab", 1}}}}});

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
}

TEST(Constraints, Contains)
{
    expect_accepted<Containing>(R"({"roles":["user","admin"],"v":[10,1,11],"none":[1],"rows":[[1],[5]]})",
                                {{"user", "admin"}, {10, 1, 11}, {1}, {{1}, {5}}});
    expect_accepted<Containing>(R"({"none":[]})", {});

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
    // a null element satisfies the settings of its value, as a null field does
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

TEST(Constraints, Strict)
{
    // yyjson::strict checks the settings of the values as a read without it does
    const auto rejected = [](auto read, std::string_view message, std::string_view path) {
        const auto e = bad_cast_of(read);
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
        EXPECT_EQ(e.path(), path);
    };
    rejected([] { (void)yyjson::read<Unique>(R"({"ints":[1,1]})", yyjson::strict); },
             R"(an element of the value of the key "ints" of Unique is equal to an earlier element)", "/ints/1");
    rejected([] { (void)yyjson::read<Prefixed>(R"({"range":[6,5]})", yyjson::strict); },
             R"(an element of the value of the key "range" of Prefixed is greater than the maximum 5)", "/range/0");
    rejected(
        [] {
            auto t = Keyed();
            yyjson::read_into(R"({"m":{"A":1}})", t, yyjson::strict);
        },
        R"(a key of the value of the key "m" of Keyed does not match the pattern "^[a-z]+$")", "/m/A");
    rejected(
        [] {
            auto t = Containing();
            yyjson::merge_into(R"({"v":[1]})", t, yyjson::strict);
        },
        R"(the value of the key "v" of Containing has fewer than 2 items)", "/v");
    rejected([] { (void)yyjson::read(R"({"nested":[["a",""],[]]})").cast<Elements>(yyjson::strict); },
             R"(an element of the value of the key "nested" of Elements has fewer items than the minimum 1)",
             "/nested/1");
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
    expect_accepted<Unusual>(R"({"pts":[[0,0],[0,1]],"sides":{"left":1},"mm":{"a":1,"a":2},"kv":{"a":1},)"
                             R"("dq":[0,-1,9],"ls":[0,1,9]})",
                             {.pts = {{0, 0}, {0, 1}},
                              .sides = {{Side::left, 1}},
                              .mm = {{"a", 1}, {"a", 2}},
                              .kv = {{"a", 1}},
                              .dq = {0, -1, 9},
                              .ls = {0, 1, 9}});
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Unusual )", key);
    };
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Unusual )", key);
    };
    // the elements that a user caster reads are converted again to find the equal one
    expect_rejected<Unusual>(R"({"pts":[[0,1],[0,1]]})", element("pts") + "is equal to an earlier element", "/pts/1");
    // a key that is not a string, and an element that is a pair, leave the error at the field
    expect_rejected<Unusual>(
        R"({"sides":{"left":1,"right":2}})",
        value("sides") + "has a key that does not satisfy the constraints of yyjson::property_names", "/sides");
    expect_rejected<Unusual>(R"({"mm":{"a":1,"a":1}})", value("mm") + "has duplicate items", "/mm");
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
    // a range that is not read from a JSON array leaves the error at the field
    expect_accepted<Unusual>(R"({"csv":"1,0"})", {.csv = Csv{{1, 0}}});
    expect_rejected<Unusual>(R"({"csv":"1,-1"})",
                             value("csv") + "has an element that does not satisfy the constraints of yyjson::items",
                             "/csv");
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

TEST(Constraints, ConstAndEnum)
{
    expect_accepted<Listed>(R"({"unit":"mm","version":2,"ratio":0.5,"flag":true,"big":18446744073709551615,"level":2,)"
                            R"("tags":["a","b"],"named":{"x":1},"pair":[1,"y"]})",
                            {.unit = "mm",
                             .ratio = 0.5F,
                             .big = 18446744073709551615ULL,
                             .level = 2,
                             .tags = {"a", "b"},
                             .named = {{"x", 1}},
                             .pair = {1, "y"}});
    // a float equals 0.1 when it holds 0.1F, whatever the JSON writes
    expect_accepted<Listed>(R"({"ratio":0.100000001,"level":null})", {});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Listed )", key);
    };
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Listed )", key);
    };
    expect_rejected<Listed>(R"({"unit":"km"})", value("unit") + R"(is not one of "mm", "cm", "m")", "/unit");
    expect_rejected<Listed>(R"({"version":3})", value("version") + "is not equal to the constant 2", "/version");
    expect_rejected<Listed>(R"({"ratio":0.3})", value("ratio") + "is not one of 0.1, 0.5", "/ratio");
    expect_rejected<Listed>(R"({"flag":false})", value("flag") + "is not equal to the constant true", "/flag");
    expect_rejected<Listed>(R"({"big":18446744073709551614})",
                            value("big") + "is not one of -1, 0, 18446744073709551615", "/big");
    expect_rejected<Listed>(R"({"level":4})", value("level") + "is not one of 1, 2, 3", "/level");
    // the constant is checked first; settings that leave no value, as with enum_(2, 3), do not compile
    expect_rejected<Listed>(R"({"odd":3})", value("odd") + "is not equal to the constant 1", "/odd");
    expect_accepted<Listed>(R"({"odd":1})", {});
    expect_rejected<Listed>(R"({"tags":["a","c"]})", element("tags") + R"(is not one of "a", "b")", "/tags/1");
    expect_rejected<Listed>(R"({"named":{"x":1,"z":2}})",
                            R"(a key of the value of the key "named" of Listed is not one of "x", "y")", "/named/z");
    expect_rejected<Listed>(R"({"pair":[2,"x"]})", element("pair") + "is not equal to the constant 1", "/pair/0");
    expect_rejected<Listed>(R"({"pair":[1,"z"]})", element("pair") + R"(is not one of "x", "y")", "/pair/1");
}

TEST(Constraints, ConstAndEnumNames)
{
    expect_accepted<ListedEnums>(R"({"first":"low","loud":"mid","maybe":"mid","tones":["HIGH","low"]})",
                                 {.loud = Tone::mid, .maybe = Tone::mid, .tones = {Tone::high, Tone::low}});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ListedEnums )", key);
    };
    expect_rejected<ListedEnums>(R"({"first":"mid"})", value("first") + R"(is not equal to the constant "low")",
                                 "/first");
    expect_rejected<ListedEnums>(R"({"loud":"low"})", value("loud") + R"(is not one of "mid", "HIGH")", "/loud");
    expect_rejected<ListedEnums>(R"({"maybe":"HIGH"})", value("maybe") + R"(is not equal to the constant "mid")",
                                 "/maybe");
    expect_rejected<ListedEnums>(R"({"tones":["low","mid"]})",
                                 R"(an element of the value of the key "tones" of ListedEnums is not one of "low", )"
                                 R"("HIGH")",
                                 "/tones/1");
}

TEST(Constraints, ConstAndEnumTexts)
{
    const auto written = ListedTexts{.at = {}, .pair = {1, 2}, .mixed = {}, .counts = {{"a", 1}}, .rows = {}};
    expect_accepted<ListedTexts>(
        R"({"at":{"y":1,"x":1},"pair":[1,2],"scale":[2.5,2.5],"mixed":[0],"counts":{"a":2},"rows":[[],[0,0]],)"
        R"("place":{"x":0,"y":0}})",
        {.at = {1, 1},
         .scale = std::vector{2.5, 2.5},
         .mixed = std::vector{0},
         .counts = {{"a", 2}},
         .rows = {{}, {0, 0}},
         .place = Spot{}});
    // a double reads 1 as 1.0, and an integer is a number of the variant, which reads it into its alternative
    expect_accepted<ListedTexts>(R"({"scale":[1.0],"mixed":0})", {.scale = std::vector{1.0}});
    expect_accepted<ListedTexts>("{}", written);
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
    expect_rejected<ListedTexts>(R"({"counts":{"a":0}})", value("counts"), "/counts");
    expect_rejected<ListedTexts>(R"({"rows":[[0],[]]})",
                                 R"(an element of the value of the key "rows" of ListedTexts is not one of [], [0,0])",
                                 "/rows/0");
    expect_accepted<ListedTexts>(R"({"place":[7]})", {.place = std::vector{7}});
    // no alternative reads the value
    expect_rejected<ListedTexts>(R"({"place":{"x":1,"y":0}})", "yyjson::any_of<", "/place");
    expect_rejected<ListedTexts>(R"({"place":[8]})", "yyjson::any_of<", "/place");
    const auto checked = [](const ListedTexts& t) {
        try
        {
            yyjson::validate(t);
            return std::string();
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::string(e.path());
        }
    };
    EXPECT_EQ(checked(written), "");
    EXPECT_EQ(checked({.at = {2, 2}}), "/at");
    EXPECT_EQ(checked({.pair = {}}), "/pair");
    EXPECT_EQ(checked({.rows = {{1}}}), "/rows/0");
    EXPECT_EQ(checked({.place = Spot{1, 1}}), "/place");
    EXPECT_THROW((void)yyjson::write(ListedTexts{.mixed = 3}, yyjson::strict), yyjson::bad_cast);

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
    expect_accepted<Formatted>(R"({"when":"1985-04-12T23:20:50.52+01:00","mail":"a.b@[IPv6:::1]",)"
                               R"("hosts":["example.com","xn--4gbwdl.xn--wgbh1c"],"code":"AB"})",
                               {"1985-04-12T23:20:50.52+01:00", "a.b@[IPv6:::1]",
                                {"example.com", "xn--4gbwdl.xn--wgbh1c"}, "AB", std::nullopt});
    expect_accepted<Formatted>(R"({"mail":null})", {});
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Formatted )", key);
    };
    expect_rejected<Formatted>(R"({"when":"2023-02-29T00:00:00Z"})",
                               value("when") + R"(does not have the format "date-time")", "/when");
    expect_rejected<Formatted>(R"({"when":"2024-01-01T23:59:60+01:00"})",
                               value("when") + R"(does not have the format "date-time")", "/when");
    expect_rejected<Formatted>(R"({"mail":"a..b@example.com"})", value("mail") + R"(does not have the format "email")",
                               "/mail");
    expect_rejected<Formatted>(
        R"({"hosts":["example.com","-a.com"]})",
        R"(an element of the value of the key "hosts" of Formatted does not have the format "hostname")", "/hosts/1");
    expect_rejected<Formatted>(R"({"code":"Ab"})", value("code") + R"(does not have the format "upper")", "/code");
    expect_accepted<Formatted>(R"({"re":"([abc])+\\s+$"})",
                               {"2024-02-29T23:59:60Z", std::nullopt, {}, "X", R"(([abc])+\s+$)"});
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

    using any = yyjson::any_of<yyjson::alt<std::string, yyjson::format("regex")>, std::string>;
    using one = yyjson::one_of<yyjson::alt<std::string, yyjson::format("regex")>, int>;
    constexpr auto value = R"("<out of memory>")";
    EXPECT_THROW((void)yyjson::read<any>(value), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(value).cast<any>(), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read<one>(value), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(value).cast<one>(), std::bad_alloc);
}

TEST(Constraints, Formats)
{
    using namespace yyjson::detail;
    const auto check = [](std::string_view format, std::string_view s) { return builtin_format(format)(s); };
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
    EXPECT_TRUE(check("uri-template", "http://example.com/dictionary/{term:1}/{term}") &&
                check("uri-template", "") && !check("uri-template", "http://example.com/dictionary/{term:1}/{term"));
    EXPECT_TRUE(check("uuid", "2EB8AA08-AA98-11EA-B4AA-73B441D16380") &&
                !check("uuid", "2eb8aa08-aa98-11ea-b4aa73b441d16380"));
    EXPECT_TRUE(check("json-pointer", "") && check("json-pointer", "/~0~1") && !check("json-pointer", "/~2") &&
                !check("json-pointer", "a"));
    EXPECT_TRUE(check("relative-json-pointer", "0#") && check("relative-json-pointer", "1/a") &&
                !check("relative-json-pointer", "01") && !check("relative-json-pointer", "/a"));
}

TEST(Constraints, DependentRequired)
{
    const auto missing = [](std::string_view dependent, std::string_view key) {
        return std::format(R"(Billing is not constructible from JSON with the key "{}" without the key "{}")",
                           dependent, key);
    };
    const auto rejected = [](auto read, std::string_view message) {
        const auto e = bad_cast_of(read);
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
        EXPECT_EQ(e.path(), "");
    };
    // an object without the key of the member, or with every key that it needs, in any order
    for (const auto json : {R"({"name":"a"})", R"({"name":"a","address":"b"})", R"({"name":"a","card":"c","address":"b","postal":1})",
                            R"({"postal":1,"card":"c","address":null})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<Billing>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).cast<Billing>()) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_TRUE(read_directly<Billing>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
#endif
    }
    for (const auto& [json, key] : {std::pair{R"({"name":"a","card":"c"})", "address"},
                                    std::pair{R"({"card":"c","address":"b"})", "postal"},
                                    std::pair{R"({"postal":1,"name":"a","card":"c"})", "address"}})
    {
        rejected([&] { (void)yyjson::read<Billing>(json); }, missing("card", key));
        rejected([&] { (void)yyjson::read(json).cast<Billing>(); }, missing("card", key));
        rejected(
            [&] {
                auto t = Billing();
                yyjson::read_into(json, t);
            },
            missing("card", key));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(read_directly<Billing>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
#endif
    }
    // yyjson::strict reports a missing key of a field that is not optional first
    rejected([] { (void)yyjson::read<Billing>(R"({"postal":1,"name":"a","card":"c"})", yyjson::strict); },
             missing("card", "address"));
    rejected([] { (void)yyjson::read<Billing>(R"({"card":"c","address":"b"})", yyjson::strict); },
             R"(Billing is not constructible from JSON without the key "name")");
    // merge_into keeps the fields that the object leaves out
    auto t = Billing();
    yyjson::merge_into(R"({"card":"c"})", t);
    EXPECT_EQ(t.card, "c");
    // in a field
    const auto e = bad_cast_of([] { (void)yyjson::read<std::vector<Billing>>(R"([{},{"card":"c"}])"); });
    EXPECT_EQ(e.path(), "/1");
}

TEST(Constraints, RequiredGroups)
{
    const auto none_of = [](std::string_view type, std::string_view groups) {
        return std::format("{} is not constructible from JSON without all the keys of one of {}", type, groups);
    };
    const auto many_of = [](std::string_view type, std::string_view groups) {
        return std::format("{} is not constructible from JSON with all the keys of more than one of {}", type,
                           groups);
    };
    // every read of `json`: accepted, or rejected at the object with `message`
    const auto check = []<typename T>(std::type_identity<T>, const char* json, std::string_view message) {
        const auto reads = std::array<std::function<void()>, 3>{
            [&] { (void)yyjson::read<T>(json); }, [&] { (void)yyjson::read(json).template cast<T>(); },
            [&] {
                auto t = T();
                yyjson::read_into(json, t);
            }};
        for (const auto& read : reads)
        {
            if (message.empty())
                EXPECT_NO_THROW(read()) << json;
            else
            {
                const auto e = bad_cast_of(read);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), "") << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty()) << json;
#endif
    };
    constexpr auto measure = std::type_identity<Measure>{};
    const auto measure_none = none_of("Measure", R"(["env"], ["value", "u"])");
    const auto measure_many = many_of("Measure", R"(["env"], ["value", "u"])");
    check(measure, R"({"name":"a","env":"e"})", "");
    check(measure, R"({"name":"a","value":1,"u":"m"})", "");
    check(measure, R"({"u":"m","value":1})", "");
    check(measure, R"({"name":"a","value":1})", measure_none);
    check(measure, R"({"name":"a"})", measure_none);
    check(measure, R"({})", measure_none);
    // the keys of all the fields in their order, and out of order
    check(measure, R"({"name":"a","env":"e","value":1,"u":"m"})", measure_many);
    check(measure, R"({"u":"m","env":"e","value":1})", measure_many);
    // a key whose value is null is a key that the object has
    check(std::type_identity<NullableMeasure>{}, R"({"env":null})", "");
    check(std::type_identity<NullableMeasure>{}, R"({"env":null,"value":1})",
          many_of("NullableMeasure", R"(["env"], ["value"])"));

    constexpr auto reachable = std::type_identity<Reachable>{};
    check(reachable, R"({"email":"a","fax":"f"})", "");
    check(reachable, R"({"phone":"p"})", "");
    check(reachable, R"({"email":"a"})", none_of("Reachable", R"(["phone"], ["fax"])"));
    check(reachable, R"({"fax":"f"})", none_of("Reachable", R"(["email"], ["phone"])"));
    check(reachable, R"({"email":"a","phone":"p","fax":"f"})", many_of("Reachable", R"(["phone"], ["fax"])"));

    constexpr auto visitable = std::type_identity<ExclusiveVisitable>{};
    check(visitable, R"({"b":1})", "");
    check(visitable, R"({"a":1,"b":2})", many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    check(visitable, R"({"b":2,"a":1})", many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    check(visitable, R"({})", none_of("ExclusiveVisitable", R"(["a"], ["b"])"));

    // yyjson::strict reports a missing key of a field that is not optional first
    const auto strict = bad_cast_of([] { (void)yyjson::read<Measure>(R"({"env":"e","value":1})", yyjson::strict); });
    EXPECT_TRUE(message_of(strict).starts_with(R"(Measure is not constructible from JSON without the key "name")"))
        << strict.what();
    EXPECT_THROW((void)yyjson::read<Measure>(R"({"name":"a"})", yyjson::strict), yyjson::bad_cast);
    // merge_into keeps the fields that the object leaves out
    auto t = Measure{"a", "e"};
    yyjson::merge_into(R"({"name":"b","value":1})", t);
    EXPECT_EQ(t.name, "b");
    // in a field
    const auto e = bad_cast_of([] { (void)yyjson::read<std::vector<Measure>>(R"([{"env":"e"},{}])"); });
    EXPECT_EQ(e.path(), "/1");
}

TEST(Constraints, PropertyCounts)
{
    const auto fewer = [](std::string_view type, std::size_t n, std::size_t min) {
        return std::format("{} is not constructible from JSON with {} keys, fewer than the minimum {}", type, n, min);
    };
    const auto more = [](std::string_view type, std::size_t n, std::size_t max) {
        return std::format("{} is not constructible from JSON with {} keys, more than the maximum {}", type, n, max);
    };
    // every read of `json` but merge_into: accepted, or rejected at the object with `message`
    const auto check = []<typename T>(std::type_identity<T>, const char* json, std::string_view message) {
        const auto reads = std::array<std::function<void()>, 3>{
            [&] { (void)yyjson::read<T>(json); }, [&] { (void)yyjson::read(json).template cast<T>(); },
            [&] {
                auto t = T();
                yyjson::read_into(json, t);
            }};
        for (const auto& read : reads)
        {
            if (message.empty())
                EXPECT_NO_THROW(read()) << json;
            else
            {
                const auto e = bad_cast_of(read);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), "") << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty()) << json;
#endif
    };
    constexpr auto sized = std::type_identity<Sized>{};
    check(sized, R"({"a":1})", "");
    check(sized, R"({"a":1,"b":2})", "");
    check(sized, R"({"c":1,"a":2})", "");
    check(sized, R"({})", fewer("Sized", 0, 1));
    // the keys of all the fields in their order, the first fields and a key out of order
    check(sized, R"({"a":1,"b":2,"c":3})", more("Sized", 3, 2));
    check(sized, R"({"a":1,"b":2,"x":3})", more("Sized", 3, 2));
    check(sized, R"({"c":1,"b":2,"a":3})", more("Sized", 3, 2));
    // unknown keys count, and a repeated key counts each time
    check(sized, R"({"x":1})", "");
    check(sized, R"({"x":1,"y":2,"z":3})", more("Sized", 3, 2));
    check(sized, R"({"a":1,"a":2,"a":3})", more("Sized", 3, 2));
    EXPECT_THROW((void)yyjson::read<Sized>(R"({"x":1})", yyjson::strict), yyjson::bad_cast);
    EXPECT_NO_THROW((void)yyjson::read<Sized>(R"({"b":1})", yyjson::strict));

    constexpr auto empty = std::type_identity<SizedEmpty>{};
    check(empty, R"({})", fewer("SizedEmpty", 0, 1));
    check(empty, R"({"x":1})", "");

    constexpr auto extra = std::type_identity<SizedExtra>{};
    check(extra, R"({"a":1,"p":2})", "");
    check(extra, R"({"p":1,"q":2,"a":3})", more("SizedExtra", 3, 2));

    constexpr auto visitable = std::type_identity<SizedVisitable>{};
    check(visitable, R"({"b":1})", "");
    check(visitable, R"({"a":1,"b":2})", more("SizedVisitable", 2, 1));

    constexpr auto tagged = std::type_identity<SizedVariant>{};
    check(tagged, R"({"kind":"t","a":1})", "");
    check(tagged, R"({"a":1,"kind":"t"})", "");
    const auto e = bad_cast_of([] { (void)yyjson::read<SizedVariant>(R"({"kind":"t","a":1,"b":2})"); });
    EXPECT_TRUE(message_of(e).find(more("SizedTagged", 3, 2)) != std::string::npos) << e.what();

    // merge_into does not count the keys
    auto t = Sized{1, 2, std::nullopt};
    yyjson::merge_into(R"({})", t);
    yyjson::merge_into(R"({"c":3})", t);
    EXPECT_EQ(t.c, 3);
    // in a field
    const auto f = bad_cast_of([] { (void)yyjson::read<std::vector<Sized>>(R"([{"a":1},{}])"); });
    EXPECT_EQ(f.path(), "/1");
}

TEST(Constraints, Conditions)
{
    // every read of `json` but merge_into: accepted, or rejected with `message` at `path`
    const auto check = []<typename T>(std::type_identity<T>, const char* json, std::string_view message,
                                      std::string_view path = "") {
        const auto reads = std::array<std::function<void()>, 4>{
            [&] { (void)yyjson::read<T>(json); }, [&] { (void)yyjson::read(json).template cast<T>(); },
            [&] {
                auto t = T();
                yyjson::read_into(json, t);
            },
            [&] { (void)yyjson::read<std::vector<T>>(std::format("[{}]", json)); }};
        for (std::size_t i = 0; i < reads.size(); ++i)
        {
            if (message.empty())
                EXPECT_NO_THROW(reads[i]()) << json;
            else
            {
                const auto e = bad_cast_of(reads[i]);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), i == 3 ? "/0" + std::string(path) : path) << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty()) << json;
#endif
    };
    constexpr auto shape = std::type_identity<ConditionShape>{};
    const auto rejected = "ConditionShape has the keys and the values that yyjson::not_ rejects";
    check(shape, R"({"kind":"circle","radius":1})", "");
    check(shape, R"({"kind":"square","p":{"x":1},"sides":[1,2,3]})", "");
    // the else of another kind, whose fields of else_ have no keys, leaves the radius unconstrained
    check(shape, R"({"kind":"square","radius":-1})", "");
    check(shape, R"({"kind":"circle"})",
          R"(ConditionShape is not constructible from JSON without the key "radius", which )"
          R"(yyjson::if_then_else requires)");
    check(shape, R"({"kind":"circle","radius":-1})",
          R"(the value of the key "radius" of ConditionShape is less than the minimum 0)", "/radius");
    check(shape, R"({"kind":"square","p":{"x":"a"}})", "int is not constructible from JSON string", "/p/x");
    check(shape, R"({"kind":"square","sides":[1,0,2]})",
          R"(an element of the value of the key "sides" of ConditionShape is less than the minimum 1)", "/sides/1");
    check(shape, R"({"kind":"square","sides":[1,2]})",
          R"(the value of the key "sides" of ConditionShape has fewer items than the minimum 3)", "/sides");
    // not_ holds for an object without the key, as JSON Schema's `not` of `properties`
    check(shape, R"({"kind":"xy"})", rejected);
    check(shape, R"({})", rejected);
    // under yyjson::strict, is_type reads strictly too
    EXPECT_NO_THROW((void)yyjson::read<ConditionShape>(R"({"kind":"square","p":{"x":1}})", yyjson::strict));
    EXPECT_THROW((void)yyjson::read<ConditionShape>(R"({"kind":"square","p":{"x":1,"y":2}})", yyjson::strict),
                 yyjson::bad_cast);
    EXPECT_NO_THROW((void)yyjson::read<ConditionShape>(R"({"kind":"square","p":{"x":1,"y":2}})"));
    // merge_into does not check the conditions, as it keeps the fields that the object leaves out
    auto merged = ConditionShape{"circle", 1.0, {}, {}};
    yyjson::merge_into(R"({"radius":-1})", merged);
    EXPECT_EQ(merged.r, -1.0);

    constexpr auto leveled = std::type_identity<Leveled>{};
    check(leveled, R"({"level":4})", "");
    check(leveled, R"({"level":5,"note":"n"})", "");
    check(leveled, R"({"level":5})",
          R"(Leveled is not constructible from JSON without the key "note", which yyjson::if_then_else requires)");
    check(leveled, R"({"level":5,"note":""})",
          R"(the value of the key "note" of Leveled is shorter than the minimum length 1)", "/note");

    constexpr auto visitable = std::type_identity<ConditionVisitable>{};
    check(visitable, R"({"a":1,"b":2})", "");
    check(visitable, R"({"b":2})", "");
    check(visitable, R"({"a":1})",
          R"(ConditionVisitable is not constructible from JSON without the key "b", which yyjson::if_then_else )"
          R"(requires)");

    // the keys of the object tell which fields it has, whatever values the fields have in T()
    constexpr auto presence = std::type_identity<ConditionPresence>{};
    const auto missing_d =
        R"(ConditionPresence is not constructible from JSON without the key "d", which yyjson::if_then_else requires)";
    check(presence, R"({})", "");
    check(presence, R"({"c":7})", "");
    check(presence, R"({"a":5,"b":0,"c":7,"d":1})", "");
    check(presence, R"({"d":1,"a":5})", "");
    check(presence, R"({"a":5})", missing_d);
    check(presence, R"({"b":0})", missing_d);
    check(presence, R"({"c":7,"b":0})", missing_d);
    check(presence, R"({"a":5,"b":0,"c":7,"e":0,"d":1})",
          "ConditionPresence has the keys and the values that yyjson::not_ rejects");
    check(presence, R"({"e":0,"c":7})", "ConditionPresence has the keys and the values that yyjson::not_ rejects");
}

TEST(Constraints, ClosedConditions)
{
    // every read of `json` but merge_into: accepted, or rejected at the key `key`; the strict read rejects the key
    // `unknown`, which is not a key of T, first
    const auto check = []<typename T>(std::type_identity<T>, const char* json, std::string_view key = "",
                                      std::string_view unknown = "") {
        const auto reads = std::array<std::function<void()>, 5>{
            [&] { (void)yyjson::read<T>(json); }, [&] { (void)yyjson::read(json).template cast<T>(); },
            [&] {
                auto t = T();
                yyjson::read_into(json, t);
            },
            [&] { (void)yyjson::read<std::vector<T>>(std::format("[{}]", json)); },
            [&] { (void)yyjson::read<T>(json, yyjson::strict); }};
        const auto message = std::format(R"( has the key "{}", which yyjson::additional_properties(false) of )"
                                         R"(yyjson::if_then_else rejects)",
                                         key);
        for (std::size_t i = 0; i < reads.size(); ++i)
        {
            if (i == 4 && !unknown.empty())
            {
                const auto e = bad_cast_of(reads[i]);
                EXPECT_TRUE(message_of(e).find(std::format(R"(has no field of the key "{}")", unknown)) !=
                            std::string::npos)
                    << json << "\n"
                    << e.what();
                EXPECT_EQ(e.path(), "/" + std::string(unknown)) << json;
            }
            else if (key.empty())
                EXPECT_NO_THROW(reads[i]()) << json;
            else
            {
                const auto e = bad_cast_of(reads[i]);
                EXPECT_TRUE(message_of(e).find(message) != std::string::npos) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), (i == 3 ? "/0/" : "/") + std::string(key)) << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value(), key.empty()) << json;
#endif
    };
    constexpr auto shape = std::type_identity<ClosedShape>{};
    check(shape, R"({"kind":"circle","r":1})");
    check(shape, R"({"r":1,"kind":"circle"})");
    check(shape, R"({})");
    check(shape, R"({"kind":"square","w":1})");
    check(shape, R"({"kind":"circle","r":1,"w":2})", "w");
    check(shape, R"({"w":2,"kind":"circle","r":1})", "w");
    check(shape, R"({"kind":"circle","r":1,"x":1})", "x");
    check(shape, R"({"kind":"square","r":1})", "r");
    check(shape, R"({"x":1})", "x");
    // the other conditions are checked first
    EXPECT_TRUE(message_of(bad_cast_of([] { (void)yyjson::read<ClosedShape>(R"({"kind":"circle","r":-1,"w":2})"); }))
                    .find("less than the minimum 0") != std::string::npos);

    constexpr auto patterns = std::type_identity<ClosedPatterns>{};
    check(patterns, R"({})");
    check(patterns, R"({"a":1,"x1":2})");
    check(patterns, R"({"x1":2})", "x1");
    check(patterns, R"({"a":0})", "a");
    check(patterns, R"({"z":0})", "z", "z");

    constexpr auto adjacent = std::type_identity<ClosedAdjacent>{};
    check(adjacent, R"({"kind":"i","data":1,"n":1})");
    check(adjacent, R"({"n":1,"data":"a","kind":"s"})");
    check(adjacent, R"({"kind":"i","data":1,"n":1,"z":0})", "z", "z");
    check(adjacent, R"({"kind":"i","data":1,"z":0})", "", "z");

    EXPECT_NO_THROW((void)yyjson::read<ClosedVariant>(R"({"type":"c","a":1})"));
    EXPECT_NO_THROW((void)yyjson::read<ClosedVariant>(R"({"a":1,"type":"c"})"));
    EXPECT_NO_THROW((void)yyjson::read(R"({"type":"c","a":1})").cast<ClosedVariant>());
    auto e = bad_cast_of([] { (void)yyjson::read<ClosedVariant>(R"({"type":"c","a":1,"b":2})"); });
    EXPECT_EQ(e.path(), "/b");
    e = bad_cast_of([] { (void)yyjson::read(R"({"type":"c","a":1,"b":2})").cast<ClosedVariant>(); });
    EXPECT_EQ(e.path(), "/b");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_TRUE(read_directly<ClosedVariant>(R"({"type":"c","a":1})", yyjson::ReadFlag::NoFlag).has_value());
    EXPECT_FALSE(read_directly<ClosedVariant>(R"({"type":"c","a":1,"b":2})", yyjson::ReadFlag::NoFlag).has_value());
#endif

    // merge_into does not check the conditions
    auto merged = ClosedShape{"circle", 1.0, {}, {}};
    yyjson::merge_into(R"({"w":1})", merged);
    EXPECT_EQ(merged.w, 1);

    // validate and write(strict) test the keys that writing writes
    const auto rejects = [](const auto& t, std::string_view path) {
        auto f = bad_cast_of([&] { yyjson::validate(t); });
        EXPECT_EQ(f.path(), path);
        EXPECT_TRUE(message_of(f).find("which yyjson::additional_properties(false) of yyjson::if_then_else rejects") !=
                    std::string::npos)
            << f.what();
        f = bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); });
        EXPECT_EQ(f.path(), path);
    };
    auto t = ClosedShape{"circle", 1.0, {}, {}};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.w = 2;
    rejects(t, "/w");
    t.w.reset();
    t.rest = {{"x", 1}};
    rejects(t, "/x");
    t.kind = "square";
    t.r.reset();
    t.rest.clear();
    EXPECT_NO_THROW(yyjson::validate(t));
    t.r = 1.0;
    rejects(t, "/r");
    rejects(ClosedPatterns{{}, {{"x1", 1}}}, "/x1");
    EXPECT_NO_THROW(yyjson::validate(ClosedPatterns{1, {{"x1", 1}}}));
    EXPECT_NO_THROW(yyjson::validate(ClosedAdjacent{1, 2}));
    EXPECT_NO_THROW(yyjson::validate(ClosedVariant(ClosedAlternative{1, {}})));
    rejects(ClosedVariant(ClosedAlternative{1, 2}), "/b");
    EXPECT_EQ(yyjson::write(ClosedVariant(ClosedAlternative{1, {}}), yyjson::strict), R"({"type":"c","a":1})");
}

TEST(Constraints, IsTypeFunction)
{
    const auto doc = yyjson::read(R"({"p":{"x":1,"y":2},"n":1.5,"s":"a"})");
    const auto obj = *doc.as_object();
    EXPECT_TRUE(yyjson::is_type<ConditionPoint>(obj["p"]));
    EXPECT_FALSE(yyjson::is_type<ConditionPoint>(obj["p"], yyjson::strict));
    EXPECT_TRUE(yyjson::is_type<double>(obj["n"]));
    EXPECT_FALSE(yyjson::is_type<int>(obj["n"]));
    EXPECT_FALSE(yyjson::is_type<int>(obj["s"]));
    EXPECT_TRUE(yyjson::is_type<std::string>(obj["s"], yyjson::strict));
    EXPECT_TRUE((yyjson::is_type<std::map<std::string, yyjson::value>>(doc)));
    EXPECT_TRUE(yyjson::is_type<ConditionPoint>(yyjson::value(doc)));
    EXPECT_FALSE(yyjson::is_type<ConditionPoint>(yyjson::object(obj), yyjson::strict));
}

TEST(Constraints, IsTypeConditions)
{
    const auto reads = [](const char* json, bool strict = false) {
        try
        {
            if (strict)
                (void)yyjson::read<TypeTested>(json, yyjson::strict);
            else
                (void)yyjson::read<TypeTested>(json);
            (void)(strict ? yyjson::read(json).cast<TypeTested>(yyjson::strict) : yyjson::read(json).cast<TypeTested>());
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    };
    for (const auto strict : {false, true})
    {
        EXPECT_TRUE(reads(R"({})", strict));
        EXPECT_FALSE(reads(R"({"auto":{}})", strict));
        EXPECT_TRUE(reads(R"({"auto":{},"name":"a"})", strict));
        EXPECT_TRUE(reads(R"({"auto":[]})", strict));
        EXPECT_TRUE(reads(R"({"auto":null})", strict));
        EXPECT_FALSE(reads(R"({"kind":null})", strict));
        EXPECT_TRUE(reads(R"({"kind":1})", strict));
        EXPECT_FALSE(reads(R"({"point":{"x":1}})", strict));
        EXPECT_TRUE(reads(R"({"point":"a"})", strict));
    }
    // {"x":1,"y":2} is a ConditionPoint only without yyjson::strict
    EXPECT_FALSE(reads(R"({"point":{"x":1,"y":2}})"));
    EXPECT_TRUE(reads(R"({"point":{"x":1,"y":2}})", true));
    const auto e = bad_cast_of([] { (void)yyjson::read<TypeTested>(R"({"kind":null})"); });
    EXPECT_TRUE(message_of(e).starts_with("TypeTested has the keys and the values that yyjson::not_ rejects"))
        << e.what();
    auto t = TypeTested{.auto_ = yyjson::read("{}"), .name = std::nullopt};
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.auto_ = yyjson::read("1");
    EXPECT_NO_THROW(yyjson::validate(t));
    t.kind = yyjson::read("null");
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
}

TEST(Constraints, TypedContains)
{
    const auto json = [](std::string_view events, std::string_view rest = "") {
        return std::format(R"({{"events":{},"objects":[{{"x":0}},{{"x":"a"}}],"numbers":["a",1]{}}})", events, rest);
    };
    const auto points = R"([{"x":1},"a",{"x":2,"y":0}])";
    EXPECT_NO_THROW((void)yyjson::read<TypedContains>(json(points)));
    EXPECT_NO_THROW((void)yyjson::read(json(points)).cast<TypedContains>());
    const auto prefix = std::string(R"(the value of the key "events" of TypedContains )");
    // under yyjson::strict, {"x":2,"y":0} is not read as ConditionPoint
    auto e = bad_cast_of([&] { (void)yyjson::read<TypedContains>(json(points), yyjson::strict); });
    EXPECT_TRUE(message_of(e).starts_with(prefix + "has fewer than 2 items that satisfy")) << e.what();
    EXPECT_EQ(e.path(), "/events");
    e = bad_cast_of([&] { (void)yyjson::read<TypedContains>(json(R"([{"x":"1"},{"x":2}])")); });
    EXPECT_TRUE(message_of(e).starts_with(prefix + "has fewer than 2 items that satisfy")) << e.what();
    EXPECT_THROW((void)yyjson::read<TypedContains>(R"({"events":[{"x":1},{"x":2}],"objects":[{"x":1},{"x":2}],)"
                                                   R"("numbers":[1]})"),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<TypedContains>(R"({"events":[{"x":1},{"x":2}],"objects":[],"numbers":[1.5]})"),
                 yyjson::bad_cast);
    // then checks the elements only when the condition holds
    EXPECT_NO_THROW((void)yyjson::read<TypedContains>(json(points, R"(,"points":[1])")));
    e = bad_cast_of([&] { (void)yyjson::read<TypedContains>(json(points, R"(,"kind":"point","points":[1])")); });
    EXPECT_EQ(e.path(), "/points");
    EXPECT_NO_THROW((void)yyjson::read<TypedContains>(json(points, R"(,"kind":"point","points":[1,{"x":0}])")));

    // validate and write(strict) read the elements as the strict read does
    const auto t = yyjson::read<TypedContains>(json(points));
    e = bad_cast_of([&] { yyjson::validate(t); });
    EXPECT_EQ(e.path(), "/events");
    EXPECT_THROW((void)yyjson::write(t, yyjson::strict), yyjson::bad_cast);
    const auto u = yyjson::read<TypedContains>(json(R"([{"x":1},{"x":2}])"), yyjson::strict);
    EXPECT_NO_THROW(yyjson::validate(u));
    EXPECT_EQ(yyjson::read<TypedContains>(yyjson::write(u, yyjson::strict), yyjson::strict).events.size(), 2);
}

TEST(Constraints, ContainsConditions)
{
    const auto check = [](const char* json, std::string_view message) {
        const auto reads = std::array<std::function<void()>, 3>{
            [&] { (void)yyjson::read<ContainsConditions>(json); },
            [&] { (void)yyjson::read(json).cast<ContainsConditions>(); },
            [&] { (void)yyjson::read<ContainsConditions>(json, yyjson::strict); }};
        for (const auto& read : reads)
        {
            if (message.empty())
                EXPECT_NO_THROW(read()) << json;
            else
            {
                const auto e = bad_cast_of(read);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), "") << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<ContainsConditions>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty())
            << json;
#endif
    };
    const auto missing = R"(ContainsConditions is not constructible from JSON without the key "reason", which )"
                         R"(yyjson::if_then_else requires)";
    const auto rejected = "ContainsConditions has the keys and the values that yyjson::not_ rejects";
    check(R"({"tags":[]})", "");
    check(R"({"tags":["a","b"]})", "");
    check(R"({"tags":["a","deprecated"],"reason":"old"})", "");
    check(R"({"tags":["a","deprecated"]})", missing);
    // twice is more than max_contains, so the condition does not hold
    check(R"({"tags":["deprecated","deprecated"]})", "");
    check(R"({"tags":[],"deps":[1,"a",{"x":1}]})", "");
    check(R"({"tags":[],"deps":[1,"a",2]})", rejected);
    check(R"({"tags":[],"deps":[1.5,2]})", "");

    // validate and write(strict) test the values that writing writes
    auto t = ContainsConditions{{"deprecated"}, {}, {}};
    auto e = bad_cast_of([&] { yyjson::validate(t); });
    EXPECT_TRUE(message_of(e).starts_with(missing)) << e.what();
    EXPECT_THROW((void)yyjson::write(t, yyjson::strict), yyjson::bad_cast);
    t.reason = "old";
    EXPECT_NO_THROW(yyjson::validate(t));
    t.deps = yyjson::read<std::vector<yyjson::value>>("[1,2]");
    e = bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); });
    EXPECT_TRUE(message_of(e).starts_with(rejected)) << e.what();
}

// not_ of settings rejects a value for which all of them hold
struct ValueNot
{
    int n = 0;
    std::optional<std::string> s;
    std::vector<int> v;
    yyjson::value any = 0;
    std::optional<int> m;
    yyjson::any_of<yyjson::alt<int, yyjson::not_(yyjson::const_(7))>, std::string> u = 0;
    std::map<std::string, int> keys;
};
template <>
inline constexpr auto yyjson::annotations<ValueNot> = yyjson::annotate(
    yyjson::field("n", yyjson::not_(yyjson::minimum(10), yyjson::maximum(19)), yyjson::not_(yyjson::const_(3))),
    yyjson::field("s", yyjson::not_(yyjson::pattern("^x"))),
    yyjson::field("v", yyjson::items(yyjson::not_(yyjson::enum_(1, 2)))),
    yyjson::field("any", yyjson::not_(yyjson::is_type<std::nullptr_t>)),
    yyjson::field("keys",
                  yyjson::property_names(yyjson::pattern("^[a-z]"), yyjson::not_(yyjson::enum_("ab", "cd")))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("n", yyjson::const_(1))),
                         yyjson::then(yyjson::field("m", yyjson::not_(yyjson::const_(0))))));

TEST(Constraints, ValueNot)
{
    const auto check = [](const char* json, std::string_view message, std::string_view path) {
        const auto reads = std::array<std::function<void()>, 3>{
            [&] { (void)yyjson::read<ValueNot>(json); }, [&] { (void)yyjson::read(json).cast<ValueNot>(); },
            [&] { (void)yyjson::read<ValueNot>(json, yyjson::strict); }};
        for (const auto& read : reads)
        {
            if (message.empty())
                EXPECT_NO_THROW(read()) << json;
            else
            {
                const auto e = bad_cast_of(read);
                EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
                EXPECT_EQ(e.path(), path) << json;
            }
        }
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_EQ(read_directly<ValueNot>(json, yyjson::ReadFlag::NoFlag).has_value(), message.empty()) << json;
#endif
    };
    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ValueNot satisfies the settings of yyjson::not_)", key);
    };
    // the keys that strict reading requires, and the members that the documents below change
    const auto doc = [](std::string_view members) {
        auto result = std::string("{") + std::string(members);
        for (const auto& [key, value] : {std::pair{"n", "0"}, {"v", "[]"}, {"any", "1"}, {"u", "1"}, {"keys", "{}"}})
            if (members.find(std::format(R"("{}":)", key)) == std::string_view::npos)
                result += std::format(R"({}"{}":{})", result.size() == 1 ? "" : ",", key, value);
        return result + "}";
    };
    const auto check_doc = [&](std::string_view members, std::string_view message, std::string_view path = "") {
        check(doc(members).c_str(), message, path);
    };
    check_doc(R"("n":9,"v":[3])", "");
    check_doc(R"("n":20,"s":"ab","any":"a","u":"x")", "");
    // both settings of the first not_ hold, the single setting of the second
    check_doc(R"("n":15)", field("n"), "/n");
    check_doc(R"("n":3)", field("n"), "/n");
    check_doc(R"("s":"xa")", field("s"), "/s");
    check_doc(R"("v":[3,2])",
              R"(an element of the value of the key "v" of ValueNot satisfies the settings of yyjson::not_)",
              "/v/1");
    // null is a value of a yyjson::value field, which not_ tests
    check_doc(R"("any":null)", field("any"), "/any");
    // 7 is the value that the alternative int rejects, and no other alternative reads it
    check_doc(R"("u":7)", "yyjson::any_of<", "/u");
    check_doc(R"("n":1,"m":0)", field("m"), "/m");
    check_doc(R"("n":2,"m":0)", "");
    check_doc(R"("n":1)", "");
    // not_ in property_names rejects the keys that it lists
    check_doc(R"("keys":{"ac":1,"x":2})", "");
    check_doc(R"("keys":{"ac":1,"cd":2})",
              R"(a key of the value of the key "keys" of ValueNot satisfies the settings of yyjson::not_)", "/keys/cd");
    check_doc(R"("keys":{"A":1})",
              R"(a key of the value of the key "keys" of ValueNot does not match the pattern)", "/keys/A");

    // validate and write(strict) test the values that writing writes
    auto t = ValueNot{};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.any = yyjson::value(nullptr);
    auto e = bad_cast_of([&] { yyjson::validate(t); });
    EXPECT_TRUE(message_of(e).starts_with(field("any"))) << e.what();
    EXPECT_EQ(e.path(), "/any");
    t.any = 1;
    t.v = {3, 1};
    e = bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); });
    EXPECT_EQ(e.path(), "/v/1") << e.what();
    t.v = {};
    t.u = 7;
    e = bad_cast_of([&] { yyjson::validate(t); });
    EXPECT_TRUE(message_of(e).starts_with("the value of the alternative 0 of ")) << e.what();
    EXPECT_EQ(e.path(), "/u");
    t.u = 0;
    t.keys = {{"ab", 1}};
    e = bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); });
    EXPECT_EQ(e.path(), "/keys/ab") << e.what();
}

// not_ of a value that may be null tests null as JSON Schema does: the settings other than const_ and enum_
// hold for it, and is_type<T> reads it
struct NullNot
{
    std::variant<std::monostate, int> above = 0;
    std::variant<std::monostate, int> listed = 0;
    std::variant<std::monostate, int> unlisted = 0;
    std::vector<std::optional<yyjson::value>> nulls;
    std::vector<std::optional<yyjson::value>> ints;
};
template <>
inline constexpr auto yyjson::annotations<NullNot> =
    yyjson::annotate(yyjson::field("above", yyjson::not_(yyjson::minimum(5))),
                     yyjson::field("listed", yyjson::not_(yyjson::enum_(1, nullptr))),
                     yyjson::field("unlisted", yyjson::not_(yyjson::enum_(1, 2))),
                     yyjson::field("nulls", yyjson::items(yyjson::not_(yyjson::is_type<std::nullptr_t>))),
                     yyjson::field("ints", yyjson::items(yyjson::not_(yyjson::is_type<int>))));
// null is no longer read by the first alternative, so it does not contain the second
using NullOrAbove = yyjson::one_of<yyjson::alt<std::optional<int>, yyjson::not_(yyjson::minimum(5))>, std::nullptr_t>;

TEST(Constraints, ValueNotOfNull)
{
    const auto read = [](std::string_view members) {
        const auto json = std::format(R"({{"above":0,"listed":0,"unlisted":0,"nulls":[],"ints":[],{}}})", members);
        return yyjson::read<NullNot>(json);
    };
    const auto rejected = [&](std::string_view members, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)read(members); });
        EXPECT_EQ(e.path(), path) << members << "\n" << e.what();
    };
    EXPECT_NO_THROW((void)read(R"("unlisted":null,"nulls":[1],"ints":[null,"a"])"));
    rejected(R"("above":null)", "/above");
    rejected(R"("listed":null)", "/listed");
    rejected(R"("nulls":[1,null])", "/nulls/1");
    rejected(R"("ints":[null,2])", "/ints/1");

    auto t = NullNot{};
    t.unlisted = std::monostate{};
    t.ints = {std::nullopt};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.above = std::monostate{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/above");
    t.above = 0;
    t.listed = std::monostate{};
    EXPECT_EQ(bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); }).path(), "/listed");
    t.listed = 0;
    t.nulls = {std::nullopt};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/nulls/0");

    EXPECT_EQ(yyjson::read<NullOrAbove>("null").index(), 1);
    EXPECT_EQ(yyjson::read<NullOrAbove>("4").index(), 0);
    EXPECT_THROW((void)yyjson::read<NullOrAbove>("5"), yyjson::bad_cast);
}

// value settings on values that can be of several JSON kinds: each setting constrains the values of its kind
enum class MultiShade { light, dark };
struct MultiPoint
{
    int x;
};
struct MultiKind
{
    yyjson::value v;
    std::variant<std::monostate, int, std::string> w;
    std::optional<yyjson::any_of<bool, double, std::string, std::vector<yyjson::value>, MultiPoint>> x;
    yyjson::one_of<int, std::string> y;
    std::variant<MultiShade, int> s;
    yyjson::value list;
    std::vector<yyjson::value> elements;
};
template <>
inline constexpr auto yyjson::annotations<MultiKind> = yyjson::annotate(
    yyjson::field("v", yyjson::enum_(1, "x", nullptr), yyjson::max_length(3)),
    yyjson::field("w", yyjson::minimum(0), yyjson::max_length(2)), yyjson::field("x", yyjson::const_("on")),
    yyjson::field("y", yyjson::not_(yyjson::const_(3))), yyjson::field("s", yyjson::enum_("dark", 3)),
    yyjson::field("list", yyjson::min_items(2), yyjson::pattern("^a")),
    yyjson::field("elements", yyjson::items(yyjson::exclusive_maximum(2), yyjson::min_length(1))));
static_assert(yyjson::detail::value_kinds<yyjson::value>() == yyjson::detail::kind_any);
static_assert(yyjson::detail::value_kinds<std::variant<MultiShade, int>>() ==
              (yyjson::detail::kind_string | yyjson::detail::kind_number));

TEST(Constraints, MultiKindValues)
{
    // the members of a valid object that `members` does not give
    const auto json = [](std::string_view members) {
        auto text = std::string("{") + std::string(members);
        for (const auto& [key, value] : {std::pair{"v", "1"}, {"w", "1"}, {"y", "1"}, {"s", "\"dark\""}})
            if (members.find(std::format(R"("{}":)", key)) == std::string_view::npos)
                text += std::format(R"(,"{}":{})", key, value);
        return text + "}";
    };
    for (const auto members : {R"("v":1.0,"w":null)", R"("v":"x","w":"ab","y":"3")", R"("v":null,"x":"on")",
                               R"("s":3,"list":"abc","elements":[1,"a",null,{},[]])", R"("list":{"k":1})",
                               R"("list":[1,2],"w":5)"})
    {
        EXPECT_NO_THROW((void)yyjson::read<MultiKind>(json(members))) << members;
        EXPECT_NO_THROW((void)yyjson::read(json(members)).cast<MultiKind>()) << members;
    }
    expect_rejected<MultiKind>(json(R"("v":2)"), R"(the value of the key "v" of MultiKind is not one of)", "/v");
    expect_rejected<MultiKind>(json(R"("v":[])"), R"(the value of the key "v" of MultiKind is not one of)", "/v");
    expect_rejected<MultiKind>(json(R"("w":-1)"), "the value of the key \"w\" of MultiKind is less than", "/w");
    expect_rejected<MultiKind>(json(R"("w":"abc")"), "the value of the key \"w\" of MultiKind is longer", "/w");
    expect_rejected<MultiKind>(json(R"("x":true)"), "the value of the key \"x\" of MultiKind is not equal", "/x");
    expect_rejected<MultiKind>(json(R"("x":{"x":1})"), "the value of the key \"x\" of MultiKind is not equal",
                               "/x");
    expect_rejected<MultiKind>(json(R"("y":3)"), "the value of the key \"y\" of MultiKind satisfies", "/y");
    expect_rejected<MultiKind>(json(R"("s":"light")"), "the value of the key \"s\" of MultiKind is not one of",
                               "/s");
    expect_rejected<MultiKind>(json(R"("s":4)"), "the value of the key \"s\" of MultiKind is not one of", "/s");
    expect_rejected<MultiKind>(json(R"("list":[1])"), "the value of the key \"list\" of MultiKind has fewer", "/list");
    expect_rejected<MultiKind>(json(R"("list":"b")"), "the value of the key \"list\" of MultiKind does not match",
                               "/list");
    expect_rejected<MultiKind>(json(R"("elements":[1,2])"), "an element of the value of the key \"elements\"",
                               "/elements/1");
    expect_rejected<MultiKind>(json(R"("elements":[""])"), "an element of the value of the key \"elements\"",
                               "/elements/0");

    auto t = yyjson::read<MultiKind>(json(R"("x":"on")"));
    EXPECT_NO_THROW(yyjson::validate(t));
    t.w = std::string("abc");
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/w");
    t.w = std::monostate{};
    t.x = true;
    EXPECT_EQ(bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); }).path(), "/x");
    t.x = std::nullopt;
    t.s = MultiShade::light;
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/s");
    t.s = 3;
    t.v = yyjson::value("x");
    EXPECT_NO_THROW(yyjson::validate(t));
    t.v = yyjson::value(1.5);
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/v");
}

// settings of strings on an enumeration written by name check the name, and the numbers of items on a struct written
// as an array count the elements that it is written as
struct MultiKindPair
{
    int a;
    std::optional<int> b;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindPair> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items));
struct MultiKindWritten
{
    std::variant<MultiShade, int> s;
    std::variant<MultiKindPair, std::string> p;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindWritten> =
    yyjson::annotate(yyjson::field("s", yyjson::pattern("^l"), yyjson::max_length(5)),
                     yyjson::field("p", yyjson::min_items(2), yyjson::max_items(3)));

TEST(Constraints, MultiKindWrittenForms)
{
    for (const auto json : {R"({"s":"light","p":"x"})", R"({"s":5,"p":[1,2]})", R"({"s":"light","p":[1,2,3]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<MultiKindWritten>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).cast<MultiKindWritten>()) << json;
    }
    const auto message = [](std::string_view key, std::string_view text) {
        return std::format(R"(the value of the key "{}" of MultiKindWritten {})", key, text);
    };
    expect_rejected<MultiKindWritten>(R"({"s":"dark","p":"x"})", message("s", "does not match"), "/s");
    expect_rejected<MultiKindWritten>(R"({"s":5,"p":[1]})", message("p", "has fewer"), "/p");
    expect_rejected<MultiKindWritten>(R"({"s":5,"p":[1,2,3,4]})", message("p", "has more"), "/p");
    auto t = MultiKindWritten{.s = MultiShade::dark, .p = std::string("x")};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/s");
    t.s = MultiShade::light;
    t.p = MultiKindPair{.a = 1, .b = std::nullopt, .rest = {}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/p");
    t.p = MultiKindPair{.a = 1, .b = 2, .rest = {3}};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.p = MultiKindPair{.a = 1, .b = 2, .rest = {3, 4}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/p");
}

// alternatives of yyjson::any_of and yyjson::one_of, with settings given as values and through pointers
constexpr std::string_view alternative_id_regex = "^[a-z]+$";
constexpr auto alternative_id_pattern = yyjson::pattern(alternative_id_regex);
constexpr bool alternative_even(int v) { return v % 2 == 0; }
using Alternatives = yyjson::any_of<
    yyjson::alt<int, yyjson::minimum(0), yyjson::maximum(9), yyjson::satisfies<alternative_even>,
                yyjson::description("small")>,
    yyjson::alt<int, yyjson::minimum(100), yyjson::enum_(100, 200)>,
    yyjson::alt<std::string, &alternative_id_pattern, yyjson::max_length(8), yyjson::title("id")>,
    yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(1)), yyjson::contains(yyjson::maximum(3)),
                yyjson::min_contains(1)>,
    std::tuple<int, std::string>>;
using ExactlyOne = yyjson::one_of<yyjson::alt<std::string, yyjson::pattern("^x")>, double>;
// one_of whose alternatives may both read a value, which is checked when the value is read
using SignedOrUnsigned = yyjson::one_of<std::int64_t, std::uint64_t>;
using LowOrHigh = yyjson::one_of<yyjson::alt<int, yyjson::maximum(10)>, yyjson::alt<int, yyjson::minimum(0)>>;
// some alternatives before the last one may read its values and some may not
using PartlyOverlapping = yyjson::one_of<yyjson::alt<double, yyjson::maximum(0.5)>, std::string,
                                         yyjson::alt<std::int64_t, yyjson::minimum(-100), yyjson::maximum(10)>,
                                         std::uint64_t>;
struct AlternativePoint
{
    int x;
    bool operator==(const AlternativePoint&) const = default;
};
using PointOrMap = yyjson::any_of<AlternativePoint, std::map<std::string, int>>;
// a struct with annotations, which are not those of a variant
struct AlternativeClosed
{
    int x;
    bool operator==(const AlternativeClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<AlternativeClosed> = yyjson::annotate(yyjson::additional_properties(false));
using NameOrClosed = yyjson::any_of<std::string, std::optional<AlternativeClosed>>;
struct AlternativeFields
{
    Alternatives a;
    std::vector<LowOrHigh> l;
    std::optional<ExactlyOne> e;
    bool operator==(const AlternativeFields&) const = default;
};
struct AlternativeNode;
using AlternativeTree = yyjson::any_of<yyjson::alt<int, yyjson::minimum(0)>, std::vector<AlternativeNode>>;
struct AlternativeNode
{
    AlternativeTree t;
    bool operator==(const AlternativeNode&) const = default;
};
namespace compile_rules
{
    template <typename... As>
    constexpr auto alternatives_error = yyjson::detail::alternative_table<As...>::tree.error;
    using enum yyjson::detail::field_key_error;
    static_assert(alternatives_error<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::minimum(0)>> ==
                  none);
    static_assert(alternatives_error<int, yyjson::alt<int, yyjson::minimum(0), yyjson::minimum(1)>> ==
                  same_setting_twice);
    static_assert(alternatives_error<yyjson::alt<int, yyjson::title("a"), yyjson::title("b")>> == same_setting_twice);
    static_assert(alternatives_error<yyjson::alt<int, yyjson::multiple_of(0)>> == multiple_of_not_positive);
    static_assert(alternatives_error<yyjson::alt<std::vector<int>, yyjson::max_contains(2)>> == contains_missing);
    static_assert(alternatives_error<yyjson::alt<std::vector<int>, yyjson::items(yyjson::min_contains(1))>> ==
                  contains_missing);

    static_assert(std::variant_size_v<Alternatives> == 5 && std::variant_size_v<const ExactlyOne> == 2);
    static_assert(std::same_as<std::variant_alternative_t<2, Alternatives>, std::string>);
    static_assert(std::same_as<std::variant_alternative_t<4, Alternatives>, std::tuple<int, std::string>>);
    static_assert(!std::is_aggregate_v<Alternatives>);
    static_assert(yyjson::detail::constrained_alternative<Alternatives, 0>);
    static_assert(!yyjson::detail::constrained_alternative<Alternatives, 4>);
    static_assert(!yyjson::detail::combined_alternatives<ExactlyOne>::table::tree.nodes[1].constraints.set);
    static_assert(yyjson::detail::combined_alternatives<ExactlyOne>::exactly_one);
    static_assert(!yyjson::detail::combined_alternatives<Alternatives>::exactly_one);
    static_assert(yyjson::detail::alternative_level<Alternatives, 1>::values.size() == 2);
    static_assert(yyjson::detail::alternative_level<Alternatives, 2>::constraints.pattern_text.view() ==
                  "^[a-z]+$");
    static_assert(yyjson::detail::alternative_level<Alternatives, 3>::node.contains != 0);

    // the JSON values that each alternative reads: `contained` is the first alternative that is never read
    // (any_of) or never the only one to read a value (one_of), `container` the one that reads them first
    enum class Shade { light, dark };
    using yyjson::alt, yyjson::any_of, yyjson::one_of;
    using yyjson::minimum, yyjson::maximum, yyjson::exclusive_minimum, yyjson::exclusive_maximum, yyjson::const_, yyjson::enum_;
    template <typename V>
    using relations = yyjson::detail::alternative_relations<V>;
    template <typename V>
    constexpr auto contained = relations<V>::contained;
    static_assert(yyjson::detail::check_alternative_bounds<Alternatives>());
    static_assert(yyjson::detail::check_alternative_bounds<ExactlyOne>());
    static_assert(contained<any_of<double, int>> == 1 && relations<any_of<double, int>>::container == 0);
    static_assert(contained<any_of<int, double>> == 2);
    static_assert(contained<any_of<double, std::uint64_t>> == 1);
    static_assert(contained<any_of<float, double>> == 2 && contained<any_of<double, float>> == 1);
    static_assert(contained<any_of<std::string, Shade>> == 1 && contained<any_of<Shade, std::string>> == 2);
    static_assert(contained<any_of<int, std::optional<int>>> == 2 && contained<any_of<std::optional<int>, int>> == 1);
    // a nullable alternative reads null unless const_ or enum_ leave nullptr out
    static_assert(contained<any_of<alt<std::optional<int>, enum_(1, nullptr)>, std::monostate>> == 1);
    static_assert(contained<any_of<alt<std::optional<int>, enum_(1, 2)>, std::monostate>> == 2);
    static_assert(contained<any_of<alt<std::optional<int>, const_(nullptr)>, std::monostate>> == 1);
    static_assert(contained<one_of<alt<std::unique_ptr<int>, const_(1)>, std::monostate>> == 2);
    static_assert(contained<any_of<yyjson::value, std::map<std::string, int>>> == 1);
    static_assert(contained<any_of<std::vector<yyjson::value>, std::vector<int>>> == 1);
    static_assert(contained<any_of<std::vector<int>, std::vector<yyjson::value>>> == 2);
    static_assert(contained<any_of<alt<int, minimum(0)>, alt<int, minimum(5)>>> == 1);
    static_assert(contained<any_of<alt<int, minimum(5)>, alt<int, minimum(0)>>> == 2);
    static_assert(contained<any_of<std::int8_t, alt<int, minimum(-128.5), exclusive_maximum(127.5)>>> == 1);
    static_assert(contained<any_of<std::int8_t, alt<int, minimum(-129), maximum(127)>>> == 2);
    static_assert(contained<any_of<alt<double, exclusive_maximum(1)>, alt<int, maximum(0)>>> == 1);
    static_assert(contained<any_of<alt<double, exclusive_maximum(1)>, alt<int, maximum(1)>>> == 2);
    static_assert(contained<any_of<alt<int, enum_(1, 2, 3)>, alt<int, const_(2)>>> == 1);
    static_assert(contained<any_of<alt<int, const_(2.0)>, alt<double, const_(2)>>> == 1);
    static_assert(contained<any_of<alt<int, enum_(1, 2)>, alt<double, enum_(2, 2.5)>>> == 2);
    static_assert(contained<any_of<bool, alt<bool, const_(true)>>> == 1);
    static_assert(contained<any_of<alt<std::string, enum_("light", "dark")>, Shade>> == 1);
    static_assert(contained<any_of<alt<std::string, enum_("light")>, Shade>> == 2);
    static_assert(contained<any_of<alt<std::string, enum_("a", "b")>, alt<std::string, const_("b")>>> ==
                  1);
    // pattern, satisfies and the like make a bound partial: such an alternative contains no other one
    static_assert(contained<any_of<alt<std::string, yyjson::pattern("^a")>, alt<std::string, const_("b")>>> ==
                  2);
    static_assert(contained<any_of<std::string, alt<std::string, yyjson::pattern("^a")>>> == 1);
    static_assert(contained<any_of<alt<int, yyjson::satisfies<alternative_even>>, int>> == 2);
    static_assert(contained<any_of<alt<int, yyjson::multiple_of(2)>, alt<int, const_(4)>>> == 2);
    // a float reads the doubles that round to the floats within its settings, which are rounded to floats; a
    // long double reads the doubles within its settings, but is described in part, as are several float values
    static_assert(contained<any_of<alt<double, minimum(0)>, alt<float, minimum(1)>>> == 1);
    static_assert(contained<any_of<alt<float, minimum(0)>, alt<float, minimum(1)>>> == 1);
    static_assert(contained<any_of<alt<double, minimum(0.1)>, alt<float, minimum(0.1)>>> == 2);
    static_assert(contained<any_of<alt<float, const_(2)>, alt<float, enum_(2)>>> == 1);
    static_assert(contained<any_of<alt<float, enum_(1, 2)>, alt<float, const_(2)>>> == 2);
    static_assert(contained<any_of<alt<double, minimum(0)>, alt<long double, minimum(1)>>> == 1);
    static_assert(contained<any_of<alt<long double, minimum(0)>, alt<long double, minimum(1)>>> == 2);
    // an alternative whose settings leave no value of its type, also the first one, and also if null is read, as
    // of a field
    static_assert(relations<any_of<alt<int, minimum(0), maximum(-1)>, std::string>>::empty == 0);
    static_assert(relations<any_of<int, alt<std::uint8_t, maximum(-1)>>>::empty == 1);
    static_assert(relations<any_of<alt<std::optional<int>, minimum(1), maximum(0)>, std::string>>::empty == 0);
    static_assert(relations<any_of<alt<std::uint8_t, maximum(0)>, std::string>>::empty == 2);
    template <typename V>
    constexpr bool reads_nothing =
        yyjson::detail::settings_read_nothing<yyjson::detail::alternative_level<V, 0>,
                                              yyjson::detail::constrained_value_t<std::variant_alternative_t<0, V>>>();
    static_assert(reads_nothing<any_of<alt<int, exclusive_minimum(0), exclusive_maximum(1)>>>);
    static_assert(!reads_nothing<any_of<alt<double, exclusive_minimum(0), exclusive_maximum(1)>>>);
    static_assert(reads_nothing<any_of<alt<std::int8_t, minimum(200)>>>);
    static_assert(reads_nothing<any_of<alt<int, const_(1.5)>>>);
    static_assert(reads_nothing<any_of<alt<int, minimum(0), const_(-1)>>>);
    static_assert(reads_nothing<any_of<alt<bool, const_(true), enum_(false)>>>);
    static_assert(reads_nothing<any_of<alt<std::string, const_("x"), enum_("a")>>>);
    static_assert(reads_nothing<any_of<alt<std::string, const_("abc"), yyjson::max_length(1)>>>);
    static_assert(!reads_nothing<any_of<alt<std::string, enum_("a", "abc"), yyjson::min_length(2)>>>);
    static_assert(reads_nothing<any_of<alt<std::string, yyjson::min_length(5), yyjson::max_length(3)>>>);
    static_assert(reads_nothing<any_of<alt<std::vector<int>, yyjson::min_items(3), yyjson::max_items(1)>>>);
    static_assert(!reads_nothing<any_of<alt<int, const_("a")>>>);
    static_assert(reads_nothing<any_of<alt<float, minimum(1), maximum(0)>>>);
    static_assert(reads_nothing<any_of<alt<float, const_(1), enum_(2, 3)>>>);
    static_assert(reads_nothing<any_of<alt<float, exclusive_minimum(0), exclusive_maximum(1e-45)>>>);
    static_assert(!reads_nothing<any_of<alt<float, minimum(0.100000001), maximum(0.1)>>>);
    static_assert(reads_nothing<any_of<alt<long double, const_(5), maximum(0)>>>);
    // an enumeration read by name reads the names of const_ and enum_; a name that is not the JSON name
    // of an enumerator does not compile
    static_assert(contained<any_of<alt<Shade, enum_("light", "dark")>, alt<Shade, const_("dark")>>> == 1);
    static_assert(contained<one_of<alt<Shade, const_("light")>, alt<Shade, const_("dark")>>> == 2);
    static_assert(contained<any_of<alt<std::string, enum_("light")>, alt<Shade, const_("light")>>> == 1);
    static_assert(reads_nothing<any_of<alt<Shade, const_("light"), enum_("dark")>>>);
    static_assert(yyjson::detail::listed_enumerators<yyjson::detail::alternative_level<any_of<alt<Tone,
                      enum_("HIGH", "low")>>, 0>, Tone> == std::array{Tone::high, Tone::low});
    static_assert(!yyjson::detail::listed_enumerators<
                  yyjson::detail::alternative_level<any_of<alt<Tone, const_("high")>>, 0>, Tone>);
    // the lengths of strings in code points, and the numbers of items of the arrays that a vector reads
    using yyjson::min_length, yyjson::max_length, yyjson::min_items, yyjson::max_items;
    static_assert(contained<any_of<alt<std::string, max_length(5)>, alt<std::string, max_length(3)>>> == 1);
    static_assert(contained<any_of<alt<std::string, max_length(3)>, alt<std::string, max_length(5)>>> == 2);
    static_assert(contained<any_of<alt<std::string, max_length(3)>, alt<std::string, const_("abc")>>> == 1);
    static_assert(contained<any_of<alt<std::string, max_length(3)>, alt<std::string, const_("abcd")>>> == 2);
    static_assert(
        contained<any_of<alt<std::string, max_length(2)>, alt<std::string, const_("\xC3\xA9\xC3\xA9")>>> == 1);
    static_assert(
        contained<any_of<alt<std::string, yyjson::pattern("^a"), max_length(3)>, alt<std::string, max_length(1)>>> ==
        2);
    using Lengths = one_of<alt<std::string, max_length(3)>, alt<std::string, min_length(4)>>;
    static_assert(contained<Lengths> == 2 && !relations<Lengths>::overlaps<0, 1>);
    using Values = std::vector<yyjson::value>;
    static_assert(contained<any_of<alt<Values, max_items(5)>, alt<Values, max_items(2)>>> == 1);
    static_assert(contained<any_of<alt<std::vector<int>, max_items(5)>, alt<std::vector<int>, max_items(2)>>> == 2);
    using Counts = one_of<alt<std::vector<int>, max_items(1)>, alt<std::vector<std::string>, min_items(2)>>;
    static_assert(contained<Counts> == 2 && !relations<Counts>::overlaps<0, 1>);
    // a map or a set counts its C++ elements, of which a JSON value may hold more
    using Maps = std::map<std::string, yyjson::value>;
    using MapCounts = one_of<alt<Maps, max_items(1)>, alt<Maps, min_items(2)>>;
    static_assert(contained<MapCounts> == 2 && relations<MapCounts>::overlaps<0, 1>);
    using SetCounts = one_of<alt<std::set<int>, max_items(1)>, alt<std::set<int>, min_items(2)>>;
    static_assert(contained<SetCounts> == 2 && relations<SetCounts>::overlaps<0, 1>);
    // nested alternatives: any_of and std::variant read the union, one_of a part of it
    static_assert(contained<any_of<std::variant<int, std::string>, std::string>> == 1);
    static_assert(contained<any_of<one_of<int, std::string>, std::string>> == 2);
    // one_of: containment either way, and overlap without containment
    static_assert(contained<one_of<int, alt<int, minimum(0)>>> == 1);
    static_assert(contained<one_of<alt<int, minimum(0)>, int>> == 0 &&
                  relations<one_of<alt<int, minimum(0)>, int>>::container == 1);
    using Signedness = one_of<std::int64_t, std::uint64_t>;
    static_assert(contained<Signedness> == 2 && relations<Signedness>::overlaps<0, 1>);
    using Split = one_of<alt<double, maximum(0.5)>, alt<int, minimum(1)>, std::string>;
    static_assert(contained<Split> == 3 && !relations<Split>::overlaps<0, 1> && !relations<Split>::overlaps<1, 2>);
    using Touching = one_of<alt<double, maximum(1)>, alt<int, minimum(1)>>;
    static_assert(contained<Touching> == 2 && relations<Touching>::overlaps<1, 0>);
}  // namespace compile_rules

TEST(Constraints, AlternativeTypes)
{
    // the alternatives of the same type are told apart by the index
    auto a = Alternatives(std::in_place_index<1>, 200);
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

    auto e = ExactlyOne(1.5);
    EXPECT_EQ(e.index(), 1);
    e = "x";
    EXPECT_EQ(std::get<std::string>(e), "x");
}

namespace
{
    // the messages of a value that no alternative of V reads, and that the alternatives i and j of V read
    template <typename V>
    std::string read_by_none()
    {
        return without_type_keywords(std::format("{} is not constructible from JSON", yyjson::detail::type_name<V>()));
    }
    template <typename V>
    std::string read_by_two(int i, int j)
    {
        return without_type_keywords(
            std::format("the value matches the alternatives {} and {} of {}", i, j, yyjson::detail::type_name<V>()));
    }
}  // namespace

TEST(Constraints, AlternativeReading)
{
    // the first alternative whose type reads the value and whose settings it satisfies
    expect_accepted<Alternatives>("4", Alternatives(std::in_place_index<0>, 4));
    expect_accepted<Alternatives>("200", Alternatives(std::in_place_index<1>, 200));
    expect_accepted<Alternatives>(R"("abc")", Alternatives(std::string("abc")));
    expect_accepted<Alternatives>("[3,1]", Alternatives(std::vector{3, 1}));
    expect_accepted<Alternatives>(R"([5,"a"])", Alternatives(std::tuple(5, std::string("a"))));
    for (const auto json : {"5", "150", R"("ABC")", R"("abcdefghi")", "[5,6]", "1.5", "null"})
        expect_rejected<Alternatives>(json, read_by_none<Alternatives>(), "");

    expect_accepted<ExactlyOne>(R"("xy")", ExactlyOne(std::string("xy")));
    expect_accepted<ExactlyOne>("2", ExactlyOne(2.0));
    expect_rejected<ExactlyOne>(R"("y")", read_by_none<ExactlyOne>(), "");

    expect_accepted<NameOrClosed>(R"({"x":1})", NameOrClosed(AlternativeClosed{1}));
    expect_accepted<NameOrClosed>("null", NameOrClosed(std::optional<AlternativeClosed>()));
    EXPECT_THROW((void)yyjson::read<NameOrClosed>(R"({"x":1,"y":2})"), yyjson::bad_cast);

    // a nullable alternative whose enum_ leave nullptr out does not read null
    using ListedOrNull = yyjson::one_of<yyjson::alt<std::optional<int>, yyjson::enum_(1, 2)>, std::monostate>;
    expect_accepted<ListedOrNull>("1", ListedOrNull(std::in_place_index<0>, 1));
    expect_accepted<ListedOrNull>("null", ListedOrNull(std::monostate()));
    expect_rejected<ListedOrNull>("3", read_by_none<ListedOrNull>(), "");

    // one_of fails for a value that two alternatives read
    expect_accepted<SignedOrUnsigned>("-1", SignedOrUnsigned(std::int64_t{-1}));
    expect_accepted<SignedOrUnsigned>("9223372036854775808", SignedOrUnsigned(std::uint64_t{1} << 63));
    expect_rejected<SignedOrUnsigned>("1", read_by_two<SignedOrUnsigned>(0, 1), "");
    expect_accepted<LowOrHigh>("-1", LowOrHigh(std::in_place_index<0>, -1));
    expect_accepted<LowOrHigh>("11", LowOrHigh(std::in_place_index<1>, 11));
    expect_rejected<LowOrHigh>("10", read_by_two<LowOrHigh>(0, 1), "");
    expect_accepted<PartlyOverlapping>("-300", PartlyOverlapping(-300.0));
    expect_accepted<PartlyOverlapping>(R"("x")", PartlyOverlapping(std::string("x")));
    expect_accepted<PartlyOverlapping>("20", PartlyOverlapping(std::uint64_t{20}));
    expect_rejected<PartlyOverlapping>("0", read_by_two<PartlyOverlapping>(0, 2), "");
    expect_rejected<PartlyOverlapping>("5", read_by_two<PartlyOverlapping>(2, 3), "");

    // in fields and elements
    expect_accepted<AlternativeFields>(R"({"a":"id","l":[-5,20],"e":"x"})",
                                       {Alternatives(std::string("id")),
                                        {LowOrHigh(std::in_place_index<0>, -5), LowOrHigh(std::in_place_index<1>, 20)},
                                        ExactlyOne(std::string("x"))});
    expect_rejected<AlternativeFields>(R"({"a":4,"l":[-5,3]})", read_by_two<LowOrHigh>(0, 1), "/l/1");
    expect_rejected<AlternativeFields>(R"({"a":4,"e":"y"})", read_by_none<ExactlyOne>(), "/e");

    // recursive
    expect_accepted<AlternativeNode>(
        R"({"t":[{"t":1},{"t":[{"t":2}]}]})",
        {AlternativeTree(std::vector{AlternativeNode{AlternativeTree(std::in_place_index<0>, 1)},
                                     AlternativeNode{AlternativeTree(std::vector{
                                         AlternativeNode{AlternativeTree(std::in_place_index<0>, 2)}})}})});
    expect_rejected<AlternativeNode>(R"({"t":[{"t":-1}]})", read_by_none<AlternativeTree>(), "/t");

    // an alternative reads in the mode of the read: strict rejects the unknown key of the struct
    const auto json = R"({"x":1,"y":2})";
    EXPECT_EQ(yyjson::read<PointOrMap>(json), PointOrMap(AlternativePoint{1}));
    EXPECT_EQ(yyjson::read(json).cast<PointOrMap>(), PointOrMap(AlternativePoint{1}));
    const auto map = PointOrMap(std::map<std::string, int>{{"x", 1}, {"y", 2}});
    EXPECT_EQ(yyjson::read<PointOrMap>(json, yyjson::strict), map);
    EXPECT_EQ(yyjson::read(json).cast<PointOrMap>(yyjson::strict), map);

    // merge_into replaces the value
    auto merged = AlternativeFields{Alternatives(std::vector{1}), {}, {}};
    yyjson::merge_into(R"({"a":[2]})", merged);
    EXPECT_EQ(merged.a, Alternatives(std::vector{2}));

    // written as the alternative held, in both writers
    const auto fields = AlternativeFields{Alternatives(std::in_place_index<1>, 100),
                                          {LowOrHigh(std::in_place_index<1>, 20)},
                                          ExactlyOne(std::string("x"))};
    EXPECT_EQ(yyjson::write(fields), R"({"a":100,"l":[20],"e":"x"})");
    EXPECT_EQ(yyjson::object(fields).write(), R"({"a":100,"l":[20],"e":"x"})");
    EXPECT_EQ(yyjson::value(Alternatives(std::string("id"))).write(), R"("id")");
}

TEST(Constraints, PatternProperties)
{
    const auto check = [](std::string_view json, std::string_view message, std::string_view path) {
        return [=](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
    };
    // every read of `json` but merge_into throws `message` at `path`; merge_into into `t` checks a value only against
    // the field that receives it
    const auto rejected_but_merged = [&]<typename T>(T t, std::string_view json, std::string_view message,
                                                     std::string_view path) {
        const auto c = check(json, message, path);
        c(bad_cast_of([&] { (void)yyjson::read<T>(json); }));
        c(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::strict); }));
        c(bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); }));
        c(bad_cast_of([&] {
            auto u = T();
            yyjson::read_into(json, u);
        }));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
#endif
        EXPECT_NO_THROW(yyjson::merge_into(json, t)) << json;
        return t;
    };
    const auto strict_rejects = [&]<typename T>(std::type_identity<T>, std::string_view json,
                                                std::string_view message, std::string_view path) {
        const auto c = check(json, message, path);
        c(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::strict); }));
        c(bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(yyjson::strict); }));
    };
    constexpr auto maps = std::type_identity<PatternMaps>{};

    // a key goes to the first field whose pattern matches it, in the order of the members
    expect_accepted<PatternMaps>(R"({"ab":12,"ax":1,"xb":10,"sv":"t"})", {12, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}});
    expect_accepted<PatternMaps>(R"({"ax":1,"ab":12})", {12, {{"ax", 1}}, {}, {}});
    expect_rejected<PatternMaps>(R"({"sb":"x"})", "int is not constructible from JSON string", "/sb");
    expect_rejected<PatternMaps>(R"({"ab":12,"xb":9})",
                                 R"(the value of the key "xb" of PatternMaps is less than the minimum 10)", "/xb");
    expect_rejected<PatternMaps>(R"({"aaaa":1})", R"(the key "aaaa" of PatternMaps is longer than the maximum length 3)",
                                 "/aaaa");
    // a key that no pattern matches is unknown
    expect_accepted<PatternMaps>(R"({"ab":12,"zz":1})", {12, {}, {}, {}});
    strict_rejects(maps, R"({"ab":12,"zz":1})", R"(PatternMaps has no field of the key "zz")", "/zz");
    strict_rejects(maps, R"({"ab":12,"ax":1,"ax":2})", R"(the object read into PatternMaps has the key "ax" twice)", "/ax");
    // a key that no field receives may appear twice, as in a struct without pattern fields
    for (const auto json : {R"({"a":1,"zz":1,"zz":2})", R"({"zz":1,"a":1,"zz":2,"p1":3})"})
    {
        const auto expected = std::string_view(json).find("p1") != std::string_view::npos ? PatternOpen{1, {{"p1", 3}}} : PatternOpen{1, {}};
        EXPECT_EQ(yyjson::read<PatternOpen>(json, yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<PatternOpen>(yyjson::strict), expected) << json;
        auto t = PatternOpen();
        yyjson::merge_into(json, t, yyjson::strict);
        EXPECT_EQ(t, expected) << json;
    }
    strict_rejects(std::type_identity<PatternOpen>{}, R"({"a":1,"p1":1,"p1":2})",
                   R"(the object read into PatternOpen has the key "p1" twice)", "/p1");

    // the value of a key that a later pattern or a field matches satisfies that pattern too
    EXPECT_EQ(rejected_but_merged(PatternMaps(), R"({"ab":12,"axb":9})",
                                  R"(the value of the key "axb" of PatternMaps is less than the minimum 10)", "/axb")
                  .a,
              (std::map<std::string, int>{{"axb", 9}}));
    EXPECT_EQ(rejected_but_merged(PatternMaps(), R"({"ab":9})",
                                  R"(the value of the key "ab" of PatternMaps is less than the minimum 10)", "/ab")
                  .ab,
              9);
    expect_accepted<PatternMaps>(R"({"axb":10,"ab":10})", {10, {{"axb", 10}}, {}, {}});

    // the keys that no pattern matches go to the field of `yyjson::extra_keys`, or are rejected
    expect_accepted<PatternRest>(R"({"ax":1,"z":true})", {{{"ax", 1}}, {{"z", true}}});
    EXPECT_EQ(yyjson::read<PatternRest>(R"({"ax":1,"z":true})", yyjson::strict),
              (PatternRest{{{"ax", 1}}, {{"z", true}}}));
    expect_rejected<PatternRest>(R"({"ax":true})", "int is not constructible from JSON bool", "/ax");
    expect_accepted<PatternClosed>(R"({"x1":[1]})", {{{"x1", yyjson::read("[1]")}}});
    expect_rejected<PatternClosed>(R"({"x1":1,"y":1})", R"(PatternClosed has no field of the key "y")", "/y");

    // the value of a tag whose key a pattern matches satisfies that pattern
    expect_accepted<PatternTagged>(R"({"kind":"s","x":1})", PatternTagShort{1, {}});
    expect_accepted<PatternTagged>(R"({"kind":"longer","y":1})", PatternTagOther{1});
    // merge_into reads an alternative that the variant does not hold, and merges into the one that it holds
    rejected_but_merged(PatternTaggedLong(PatternTagShort()), R"({"kind":"st","x":1})",
                        R"(the value of the key "kind" of PatternTagShort is longer than the maximum length 1)",
                        "/kind");
    // merge_into reads the struct that a pointer holds anew
    expect_rejected<PatternTaggedShared>(R"({"kind":"i","x":1})", "int is not constructible from JSON string", "/kind");
    for (const auto json : {R"({"kind":"i","data":1})", R"({"data":1,"kind":"i"})"})
        rejected_but_merged(PatternAdjacent(), json, "int is not constructible from JSON string",
                            "/kind");
}

TEST(Constraints, PatternPropertiesWrite)
{
    // validate and write(t, strict) throw `message` at `path`; the plain write throws std::invalid_argument for a key
    // that it would write twice and writes the rest
    const auto rejected = []<typename T>(const T& t, std::string_view message, std::string_view path,
                                         bool written_twice = false) {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
            EXPECT_EQ(e.path(), path) << message;
        };
        check(bad_cast_of([&] { yyjson::validate(t); }));
        check(bad_cast_of([&] { (void)yyjson::write(t, yyjson::strict); }));
        if (written_twice)
        {
            EXPECT_THROW((void)yyjson::write(t), std::invalid_argument) << message;
            EXPECT_THROW((void)yyjson::value(t), std::invalid_argument) << message;
        }
        else
        {
            EXPECT_EQ(std::string(yyjson::write(t)), std::string(yyjson::value(t).write())) << message;
        }
    };
    const auto maps = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}};
    const auto text = R"({"ab":12,"ax":1,"xb":10,"sv":"t"})";
    EXPECT_EQ(std::string(yyjson::write(maps)), text);
    EXPECT_EQ(std::string(yyjson::write(maps, yyjson::strict)), text);
    EXPECT_EQ(std::string(yyjson::value(maps).write()), text);
    EXPECT_NO_THROW(yyjson::validate(maps));
    EXPECT_EQ(yyjson::read<PatternMaps>(text, yyjson::strict), maps);

    auto t = maps;
    t.a["ab"] = 3;
    rejected(t, R"(a field of yyjson::pattern_properties of PatternMaps has the key "ab", which PatternMaps writes for a field or a tag)",
             "", true);
    t = maps;
    t.b["ax"] = 30;
    rejected(t, R"(the fields of yyjson::pattern_properties of PatternMaps have the key "ax" more than once)", "",
             true);
    t = maps;
    t.b["qq"] = 30;
    rejected(t, R"(the key "qq" of PatternMaps does not match the pattern "b$" of the field of yyjson::pattern_properties that has it)",
             "/qq");
    t = maps;
    t.b["axb"] = 30;
    rejected(t, R"(the key "axb" of PatternMaps matches the pattern "^a" of a field of yyjson::pattern_properties before the field that has it)",
             "/axb");
    t = maps;
    t.a["aab"] = 3;
    rejected(t, R"(the value of the key "aab" of PatternMaps is less than the minimum 10)", "/aab");
    t = maps;
    t.a["aaaa"] = 30;
    rejected(t, R"(the key "aaaa" of PatternMaps is longer than the maximum length 3)", "/aaaa");
    t = maps;
    t.ab = 3;
    rejected(t, R"(the value of the key "ab" of PatternMaps is less than the minimum 10)", "/ab");
    rejected(PatternRest{{{"ax", 1}}, {{"ay", true}}},
             R"(the key "ay" of PatternRest matches the pattern "^a" of a field of yyjson::pattern_properties, but the field of yyjson::extra_keys has it)",
             "/ay");

    // tags
    EXPECT_NO_THROW(yyjson::validate(PatternTagged(PatternTagShort())));
    rejected(PatternTaggedLong(PatternTagShort()),
             R"(the value of the key "kind" of PatternTagShort is longer than the maximum length 1)", "/kind");
    rejected(PatternTaggedShared(std::make_shared<PatternTagInt>()), "int is not constructible from JSON string",
             "/kind");
    rejected(PatternAdjacent{1, {}}, "int is not constructible from JSON string", "/kind");
    auto adjacent = PatternAdjacent{1, {{"kind", 1}}};
    rejected(adjacent, R"(a field of yyjson::pattern_properties of PatternAdjacent has the key "kind", which PatternAdjacent writes for a field or a tag)",
             "", true);

    // object_emitter::emplace_members
    EXPECT_EQ(std::string(yyjson::write(PatternEmitted{maps})), R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    EXPECT_EQ(std::string(yyjson::value(PatternEmitted{maps}).write()),
              R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    t = maps;
    t.s["ab"] = "x";
    EXPECT_THROW((void)yyjson::write(PatternEmitted{t}), std::invalid_argument);
    EXPECT_THROW((void)yyjson::value(PatternEmitted{t}), std::invalid_argument);
}

#if FIELD_RFL_ANNOTATIONS
TEST(Constraints, Annotations)
{
    expect_accepted<ElementsAnnotated>(R"({"v":[0],"m":{"a":["xy"]}})", {{0}, {{"a", {"xy"}}}});
    expect_rejected<ElementsAnnotated>(
        R"({"v":[0,-1]})", R"(an element of the value of the key "v" of ElementsAnnotated is less than the minimum 0)",
        "/v/1");
    expect_rejected<ElementsAnnotated>(
        R"({"m":{"a":["xy","xyz"]}})",
        R"(an element of the value of the key "m" of ElementsAnnotated is longer than the maximum length 2)", "/m/a/1");

    expect_accepted<ElementsAnnotated2>(R"({"p":[0,-1],"u":[1,2],"k":{"ab":1},"c":[5,6]})",
                                        {{0, -1}, {1, 2}, {{"ab", 1}}, {5, 6}});
    expect_rejected<ElementsAnnotated2>(
        R"({"p":[-1]})", R"(an element of the value of the key "p" of ElementsAnnotated2 is less than the minimum 0)",
        "/p/0");
    expect_rejected<ElementsAnnotated2>(
        R"({"u":[1,1]})",
        R"(an element of the value of the key "u" of ElementsAnnotated2 is equal to an earlier element)", "/u/1");
    expect_rejected<ElementsAnnotated2>(
        R"({"k":{"abc":1}})",
        R"(a key of the value of the key "k" of ElementsAnnotated2 is longer than the maximum length 2)", "/k/abc");
    expect_rejected<ElementsAnnotated2>(
        R"({"c":[5]})",
        R"(the value of the key "c" of ElementsAnnotated2 has fewer than 2 items that satisfy the constraints of yyjson::contains)",
        "/c");
    EXPECT_NO_THROW((void)yyjson::read<TypedContainsAnnotated>(R"({"v":[{"x":1},1,{"x":2}]})"));
    EXPECT_THROW((void)yyjson::read<TypedContainsAnnotated>(R"({"v":[{"x":1},{"x":"2"}]})"), yyjson::bad_cast);

    EXPECT_NO_THROW((void)yyjson::read<NotAnnotated>(R"({"n":20,"d":[1,"a"]})"));
    for (const auto* json : {R"({"n":15})", R"({"n":3})", R"({"d":[1,null]})"})
    {
        const auto e = bad_cast_of([&] { (void)yyjson::read<NotAnnotated>(json); });
        EXPECT_TRUE(message_of(e).find("satisfies the settings of yyjson::not_") != std::string::npos) << e.what();
    }

    expect_accepted<KeywordsAnnotated>(R"({"version":2,"unit":"b","day":"2024-02-29","steps":[2,1.5]})",
                                       {2, "b", "2024-02-29", {2, 1.5}, {}, {}});
    const auto keywords = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of KeywordsAnnotated )", key);
    };
    expect_rejected<KeywordsAnnotated>(R"({"version":1})", keywords("version") + "is not equal to the constant 2",
                                       "/version");
    expect_rejected<KeywordsAnnotated>(R"({"unit":"c"})", keywords("unit") + R"(is not one of "a", "b")", "/unit");
    expect_rejected<KeywordsAnnotated>(R"({"day":"2024-02-30"})", keywords("day") + R"(does not have the format "date")",
                                       "/day");
    expect_rejected<KeywordsAnnotated>(
        R"({"steps":[1]})", R"(an element of the value of the key "steps" of KeywordsAnnotated is not one of 1.5, 2)",
        "/steps/0");
    EXPECT_NO_THROW((void)yyjson::read<KeywordsAnnotated>(R"({"card":"c","address":"a"})"));
    EXPECT_THROW((void)yyjson::read<KeywordsAnnotated>(R"({"card":"c"})"), yyjson::bad_cast);

    EXPECT_NO_THROW((void)yyjson::read<GroupsAnnotated>(R"({"a":1,"c":1})"));
    EXPECT_NO_THROW((void)yyjson::read<GroupsAnnotated>(R"({"b":1})"));
    for (const auto& [json, message] :
         {std::pair{R"({"a":1})", R"(without all the keys of one of ["a", "c"], ["b"])"},
          std::pair{R"({"a":1,"b":1,"c":1})", R"(with all the keys of more than one of ["a"], ["b"])"}})
    {
        const auto e = bad_cast_of([&] { (void)yyjson::read<GroupsAnnotated>(json); });
        const auto expected = std::format("GroupsAnnotated is not constructible from JSON {}", message);
        EXPECT_TRUE(message_of(e).starts_with(expected)) << e.what();
    }

    EXPECT_NO_THROW((void)yyjson::read<ConditionsAnnotated>(R"({"kind":"circle","r":0})"));
    EXPECT_NO_THROW((void)yyjson::read<ConditionsAnnotated>(R"({"kind":"square"})"));
    for (const auto& [json, message] :
         {std::pair{R"({"kind":"circle"})",
                    R"(ConditionsAnnotated is not constructible from JSON without the key "r", which )"
                    R"(yyjson::if_then_else requires)"},
          std::pair{R"({"kind":"circle","r":-1})",
                    R"(the value of the key "r" of ConditionsAnnotated is less than the minimum 0)"},
          std::pair{R"({"kind":"y"})", "ConditionsAnnotated has the keys and the values that yyjson::not_ rejects"}})
    {
        const auto e = bad_cast_of([&] { (void)yyjson::read<ConditionsAnnotated>(json); });
        EXPECT_TRUE(message_of(e).starts_with(message)) << e.what();
    }

    EXPECT_NO_THROW((void)yyjson::read<ClosedAnnotated>(R"({"kind":"circle","r":1})"));
    EXPECT_NO_THROW((void)yyjson::read<ClosedAnnotated>(R"({"kind":"square","w":1})"));
    const auto closed = bad_cast_of([] { (void)yyjson::read<ClosedAnnotated>(R"({"kind":"circle","w":1})"); });
    EXPECT_TRUE(message_of(closed).starts_with(R"(ClosedAnnotated has the key "w", which )"
                                               R"(yyjson::additional_properties(false) of yyjson::if_then_else rejects)"))
        << closed.what();
    EXPECT_EQ(closed.path(), "/w");

    expect_accepted<PatternAnnotated>(R"({"ab":10,"ax":1,"xb":10,"zz":1})", {10, {{"ax", 1}}, {{"xb", 10}}});
    expect_rejected<PatternAnnotated>(
        R"({"xb":9})", R"(the value of the key "xb" of PatternAnnotated is less than the minimum 10)", "/xb");
    expect_rejected<PatternAnnotated>(
        R"({"aaaa":1})", R"(the key "aaaa" of PatternAnnotated is longer than the maximum length 3)", "/aaaa");
    EXPECT_EQ(yyjson::write(PatternAnnotated{10, {{"ax", 1}}, {{"xb", 10}}}), R"({"ab":10,"ax":1,"xb":10})");

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
}
#endif

// NOLINTEND
