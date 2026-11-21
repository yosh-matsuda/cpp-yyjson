#pragma once

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

inline bool is_even(int v) { return v % 2 == 0; }
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

struct ConstrainedOuter
{
    std::vector<Bounded> items;
    Texts texts;
    bool operator==(const ConstrainedOuter&) const = default;
};

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
inline bool fits_in_three(const std::string& s) { return s.size() < 4; }
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

// The keywords of arrays and objects: the elements of ranges, the values and the keys of maps, the positions of
// tuples and the elements that an array contains
inline bool is_small_pair(const std::pair<const int, int>& p) { return p.first < p.second; }
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

inline bool is_short(const std::string& s) { return s.size() < 3; }
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

// the keywords of the elements of a struct that is transparent
struct Scores
{
    std::vector<int> values = {};
    bool operator==(const Scores&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Scores> = yyjson::annotate(
    yyjson::transparent, yyjson::field("values", yyjson::min_items(1), yyjson::items(yyjson::minimum(0))));

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

namespace
{
    // `e.what()` without the position of the read
    std::string message_of(const std::exception& e)
    {
        const auto* cast = dynamic_cast<const yyjson::bad_cast*>(&e);
        return cast != nullptr ? what_without_position(*cast) : std::string(e.what());
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

// The alternatives of `yyjson::any_of_variant`, with settings given as values and through a pointer
inline constexpr char union_id_chars[] = "^[a-z]+$";
inline constexpr std::string_view union_id_regex = union_id_chars;
inline constexpr auto union_id_pattern = yyjson::pattern(union_id_regex);
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
inline constexpr char union_key_x[] = "x";
inline constexpr char union_key_y[] = "y";
inline constexpr auto union_y_needs_x = yyjson::dependent_required(union_key_y, {union_key_x});
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

// a validate inside a format of a merge checks its value on its own
struct R14Text
{
    std::string text;
};
inline bool r14_has_s(const std::string& s) { return s.find('S') != std::string::npos; }
template <>
inline constexpr auto yyjson::annotations<R14Text> =
    yyjson::annotate(yyjson::field("text", yyjson::satisfies<r14_has_s>));
inline bool r14_valid_text(std::string_view s)
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

// NOLINTEND
