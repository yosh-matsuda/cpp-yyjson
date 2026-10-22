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
    yyjson::field("uset", yyjson::items(yyjson::validate<is_even>)),
    yyjson::field("limits", yyjson::max_items(3), yyjson::additional_properties(yyjson::maximum(100))),
    yyjson::field("groups", yyjson::additional_properties(yyjson::min_items(1), yyjson::items(yyjson::minimum(1)))),
    yyjson::field("by_id", yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("ordered", yyjson::items(yyjson::validate<is_small_pair>)),
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
                  yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(2), yyjson::validate<is_lower>}, {})),
    yyjson::field("range", yyjson::prefix_items(yyjson::maximum(5), yyjson::minimum(5))),
    yyjson::field("head", yyjson::prefix_items({yyjson::minimum(0)}, {}), yyjson::items(yyjson::maximum(9))),
    yyjson::field("arr", yyjson::prefix_items({}, yyjson::minimum(1))),
    yyjson::field("grid", yyjson::prefix_items(yyjson::items(yyjson::minimum(1)), {yyjson::min_items(2)})));

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
    yyjson::field("u", yyjson::property_names(yyjson::validate<is_short>)),
    yyjson::field("deep", yyjson::additional_properties(yyjson::property_names(yyjson::min_length(2)))));

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
    yyjson::field("sides", yyjson::property_names(yyjson::validate<is_left>)),
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

// the nodes of the settings are counted from the types, and a struct without settings of the values in its fields
// takes none
static_assert(yyjson::detail::node_count<decltype(yyjson::items(yyjson::items(yyjson::minimum(0))))> == 2);
static_assert(yyjson::detail::value_node_count<Bounded> == 0);

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
#undef CONSTRAINT_RULE
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
    static_assert(table_error<PositionOf8> == none);
    static_assert(table_error<PositionOf9> == prefix_position_too_large);
    static_assert(table_error<MultipleOfZeroInItems> == multiple_of_not_positive);

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
}  // namespace compile_rules

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

struct ConstrainedAnnotated
{
    [[ = yyjson::minimum(1), = yyjson::maximum(3) ]] int n = 1;
    [[= yyjson::pattern("^x")]] std::string s = "x";
    [[= yyjson::validate<is_even>]] int even = 0;
    bool operator==(const ConstrainedAnnotated&) const = default;
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
    expect_rejected<Elements>(R"({"uset":[2,4,6,3,8]})", prefix("uset") + "is rejected by yyjson::validate", "/uset/3");
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
    expect_rejected<Elements>(R"({"ordered":[[1,2],[4,3]]})", prefix("ordered") + "is rejected by yyjson::validate",
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
    expect_rejected<Prefixed>(R"({"row":[0,"AB",0]})", prefix("row") + "is rejected by yyjson::validate", "/row/1");
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
    expect_rejected<Keyed>(R"({"u":{"abc":1}})", prefix("u") + "is rejected by yyjson::validate", "/u/abc");
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
        prefix("none") + "has more than 1 items that satisfy the constraints of yyjson::contains", "/none");
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
                             prefix("opts") + "has more than 1 items that satisfy the constraints of yyjson::contains",
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
