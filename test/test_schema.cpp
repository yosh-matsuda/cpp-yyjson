#include <algorithm>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <memory>
#include <regex>
#include "cpp_yyjson_schema.hpp"
#include "test_common.hpp"

// NOLINTBEGIN

// The schemas of `yyjson::json_schema<T>` are checked with the validator of jsoncons: a document that the read
// schema accepts is one that `yyjson::read<T>` reads, with or without `yyjson::strict`, and the text that
// `yyjson::write` writes is valid against the write schema.

template <>
struct yyjson::pattern_matcher<>
{
    std::regex re;
    explicit pattern_matcher(std::string_view p) : re(std::string(p)) {}
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), re); }
};

enum class Paint { red, green, blue };
struct Address { std::string city; std::optional<std::string> zip; };
struct Node { int value = 0; std::vector<Node> children; };
struct Circle { double r = 0; };
struct Rect { double w = 0, h = 0; };
using Tagged = std::variant<Circle, Rect>;
template <>
inline constexpr auto yyjson::annotations<Tagged> = yyjson::annotate(yyjson::internal_tag("kind", {"circle", "rect"}));
using Untagged = std::variant<int, std::string, std::vector<int>>;
struct Person
{
    std::string name;
    std::uint8_t age = 0;
    std::vector<std::string> tags;
    std::optional<Address> address;
    Paint color = Paint::green;
    std::map<std::string, double> scores;
    std::set<int> ids;
    std::tuple<int, std::string> pair;
    std::array<double, 3> pos{};
    Tagged shape;
    Untagged u;
    std::shared_ptr<Node> tree;
    std::optional<int> opt;
    std::int64_t big = 0;
};
template <>
inline constexpr auto yyjson::annotations<Person> = yyjson::annotate(
    yyjson::field("name", yyjson::required, yyjson::min_length(1), yyjson::max_length(32)),
    yyjson::field("age", yyjson::maximum(150)),
    yyjson::field("tags", yyjson::items(yyjson::pattern("^[a-z]+$")), yyjson::unique_items, yyjson::max_items(8)),
    yyjson::field("scores", yyjson::additional_properties(yyjson::minimum(0), yyjson::maximum(100)),
                  yyjson::property_names(yyjson::max_length(16))));


enum class Perm : unsigned { read = 1, write = 2 };
template <> inline constexpr auto yyjson::annotations<Perm> = yyjson::annotate(yyjson::enum_format::integer);
enum class Level { low, high };
template <> inline constexpr auto yyjson::annotations<Level> = yyjson::annotate(yyjson::enum_format::name_or_integer, yyjson::enumerator("high", yyjson::rename("HIGH")));
using Adj = std::variant<std::monostate, Circle, Rect>;
struct Msg
{
    std::string id;
    Adj body;
    Perm perm = Perm::read;
    Level level = Level::low;
    std::map<Level, int> by_level;
    std::vector<int> nums;
    std::vector<int> c{10};
    float f = 0;
    char ch = 0;
    int hidden = 0;
    std::vector<std::optional<int>> holes;
    std::vector<std::pair<std::string, int>> kv;
    std::map<int, std::string> imap;
    int ok = 0;
};
template <>
inline constexpr auto yyjson::annotations<Msg> = yyjson::annotate(
    yyjson::field("id", yyjson::rename("ID")),
    yyjson::field("body", yyjson::adjacent_tag("type", {"circle", "rect"})),
    yyjson::field("nums", yyjson::prefix_items({yyjson::minimum(10)}, {}), yyjson::items(yyjson::maximum(5))),
    yyjson::field("c", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(1)),
    yyjson::field("hidden", yyjson::skip),
    yyjson::field("ok", yyjson::optional_key),
    yyjson::additional_properties(false));
struct Loose { int a = 0; };
template <> inline constexpr auto yyjson::annotations<Loose> = yyjson::annotate(yyjson::additional_properties(true));
struct St
{
    int a = 0;
    std::optional<int> b;
    std::optional<std::optional<int>> bb;
    std::set<int> s;
    std::map<std::string, int> m;
    Tagged t;
    std::vector<std::pair<std::string, int>> kv;
    std::optional<Untagged> u;
};

// fields whose keys are left out while they hold their value in `T()`, one of which violates the constraint
struct OmitD
{
    int a = 0;
    int limit = -1;
    std::string name = "x";
};
template <>
inline constexpr auto yyjson::annotations<OmitD> =
    yyjson::annotate(yyjson::field("limit", yyjson::omit_default, yyjson::minimum(0)),
                     yyjson::field("name", yyjson::omit_default), yyjson::additional_properties(false));

// fields whose defaults functions make, one of which holds the struct itself and one of which violates the constraint
struct DefaultFromD;
DefaultFromD default_from_d();
struct DefaultFromD
{
    std::string name;
    std::optional<std::unique_ptr<DefaultFromD>> child;
    std::vector<int> sizes;
    int limit = 0;
};
template <>
inline constexpr auto yyjson::annotations<DefaultFromD> =
    yyjson::annotate(yyjson::field("child", yyjson::default_from<&default_from_d>),
                     yyjson::field("sizes", yyjson::default_from<[] { return std::vector{1, 2}; }>),
                     yyjson::field("limit", yyjson::default_from<[] { return -1; }>, yyjson::minimum(0)));
DefaultFromD default_from_d()
{
    auto d = DefaultFromD();
    d.name = "leaf";
    return d;
}

// groups of keys of which exactly one or at least one is in the object
struct Groups
{
    std::optional<std::string> env;
    std::optional<double> value;
    std::optional<std::string> unit;
    std::optional<std::string> email;
    std::optional<std::string> phone;
};
template <>
inline constexpr auto yyjson::annotations<Groups> =
    yyjson::annotate(yyjson::one_of_required({"env"}, {"value", "unit"}), yyjson::field("unit", yyjson::rename("u")),
                     yyjson::any_of_required({"email"}, {"phone"}), yyjson::any_of_required({"env"}, {"phone"}));

// conditions on the keys: a circle has a radius of at least 0, another kind a point, and no kind starts with "x";
// a single condition goes into the schema of the struct itself
struct CondPoint
{
    int x = 0;
};
struct Conditioned
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<yyjson::value> p;
};
template <>
inline constexpr auto yyjson::annotations<Conditioned> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
                         yyjson::then(yyjson::field("r", yyjson::required, yyjson::minimum(0))),
                         yyjson::else_(yyjson::field("p", yyjson::is_type<CondPoint>))),
    yyjson::not_(yyjson::field("kind", yyjson::pattern("^x"))), yyjson::field("r", yyjson::rename("radius")));
struct CondOpen
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<CondOpen> = yyjson::annotate(yyjson::additional_properties(true));
struct OpenCondition
{
    std::optional<yyjson::value> p;
};
template <>
inline constexpr auto yyjson::annotations<OpenCondition> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::field("p", yyjson::required)), yyjson::then(yyjson::field("p", yyjson::is_type<CondOpen>))));
struct OneCondition
{
    std::optional<int> a;
    std::optional<std::vector<int>> b;
};
template <>
inline constexpr auto yyjson::annotations<OneCondition> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::field("a", yyjson::required, yyjson::maximum(5))),
                         yyjson::then(yyjson::field("b", yyjson::required, yyjson::items(yyjson::minimum(1))),
                                      yyjson::field("b", yyjson::min_items(2)))));
// conditions on a plain field and a field with yyjson::omit_default, which tell their keys by the object
struct CondKeys
{
    int b = 0;
    int e = 0;
    std::optional<int> d;
};
template <>
inline constexpr auto yyjson::annotations<CondKeys> =
    yyjson::annotate(yyjson::field("e", yyjson::omit_default),
                     yyjson::if_then_else(yyjson::if_(yyjson::field("e", yyjson::required)),
                                          yyjson::then(yyjson::field("d", yyjson::required))),
                     yyjson::not_(yyjson::field("b", yyjson::const_(-1))));

// keys of no field kept in a map, whose values are constrained, also in an alternative of an internally tagged variant
struct Extra
{
    int a = 0;
    std::map<std::string, std::vector<int>> more;
};
template <>
inline constexpr auto yyjson::annotations<Extra> = yyjson::annotate(
    yyjson::field("more", yyjson::extra_keys, yyjson::additional_properties(yyjson::items(yyjson::maximum(3)))));
struct ExtraCircle
{
    int r = 0;
    std::map<std::string, yyjson::value> more;
};
template <>
inline constexpr auto yyjson::annotations<ExtraCircle> = yyjson::annotate(yyjson::field("more", yyjson::extra_keys));
using ExtraShape = std::variant<ExtraCircle, Groups>;
// keys of no field whose names are constrained; `propertyNames` lets the keys of the fields pass
struct ExtraNamed
{
    int long_name = 0;
    std::map<std::string, int> more;
};
template <>
inline constexpr auto yyjson::annotations<ExtraNamed> = yyjson::annotate(
    yyjson::field("more", yyjson::extra_keys, yyjson::property_names(yyjson::pattern("^x-"))));
template <>
inline constexpr auto yyjson::annotations<ExtraShape> = yyjson::annotate(yyjson::internal_tag("type", {"c", "g"}));

// structs read and written as the value of their only field, whose constraints and metadata they take, also as the
// fields, elements and alternatives of others
struct TId
{
    int value = 0;
};
template <>
inline constexpr auto yyjson::annotations<TId> =
    yyjson::annotate(yyjson::transparent, yyjson::description("A user"), yyjson::field("value", yyjson::minimum(1)));
struct TTags
{
    std::vector<std::string> v;
};
template <>
inline constexpr auto yyjson::annotations<TTags> = yyjson::annotate(
    yyjson::transparent, yyjson::field("v", yyjson::min_items(1), yyjson::items(yyjson::max_length(3))));
struct TLimit
{
    std::optional<int> o = 7;
};
template <>
inline constexpr auto yyjson::annotations<TLimit> =
    yyjson::annotate(yyjson::transparent, yyjson::field("o", yyjson::maximum(10)));
struct TBag
{
    std::map<std::string, int> m;
};
template <>
inline constexpr auto yyjson::annotations<TBag> =
    yyjson::annotate(yyjson::transparent, yyjson::field("m", yyjson::min_items(1)));
struct TPoint
{
    int x = 0;
};
struct TWrap
{
    TPoint in;
};
template <>
inline constexpr auto yyjson::annotations<TWrap> = yyjson::annotate(yyjson::transparent);
using TShape = std::variant<TPoint, TWrap, TBag>;
template <>
inline constexpr auto yyjson::annotations<TShape> = yyjson::annotate(yyjson::internal_tag("t", {"p", "w", "b"}));
struct TAccount
{
    TId id{1};
    TTags tags{{"a"}};
    std::vector<TId> ids;
    TLimit limit;
    TShape shape;
};

struct TkC
{
    std::tuple<std::pair<std::string, int>, std::pair<std::string, int>> t{{"a", 1}, {"b", 2}};
};
template <> inline constexpr auto yyjson::annotations<TkC> = yyjson::annotate(
    yyjson::field("t", yyjson::prefix_items({yyjson::prefix_items({yyjson::max_length(1)}, {yyjson::minimum(0)})},
                                            {yyjson::prefix_items({}, {yyjson::maximum(9)})})));

struct Kw
{
    int version = 2;
    std::string kind = "a";
    float ratio = 0.5f;
    bool on = true;
    std::string when = "2024-01-02T03:04:05Z";
    std::vector<std::string> codes;
    std::optional<std::string> card;
    std::optional<std::string> billing;
    std::optional<std::string> zip;
};
template <> inline constexpr auto yyjson::annotations<Kw> = yyjson::annotate(
    yyjson::field("version", yyjson::const_(2)),
    yyjson::field("kind", yyjson::enum_("a", "b")),
    yyjson::field("ratio", yyjson::enum_(0.5, 0.25)),
    yyjson::field("on", yyjson::const_(true)),
    yyjson::field("when", yyjson::format("date-time")),
    yyjson::field("codes", yyjson::items(yyjson::enum_("x", "y"))),
    yyjson::dependent_required("card", {"billing", "zip"}),
    yyjson::dependent_required("billing", {"zip"}));
// const_ and enum_ of an enumeration take the JSON names of its enumerators
enum class KwTone
{
    low,
    high
};
template <>
inline constexpr auto yyjson::annotations<KwTone> =
    yyjson::annotate(yyjson::enumerator("high", yyjson::rename("HIGH")));
struct KwEnum
{
    KwTone tone = KwTone::high;
    std::vector<KwTone> tones;
    std::optional<KwTone> mode;
    std::optional<int> level;
};
template <>
inline constexpr auto yyjson::annotations<KwEnum> = yyjson::annotate(
    yyjson::field("tone", yyjson::const_("HIGH")),
    yyjson::field("tones", yyjson::items(yyjson::enum_("low"))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("mode", yyjson::const_("low"), yyjson::required)),
                         yyjson::then(yyjson::field("level", yyjson::required))));
struct Nul
{
    std::variant<std::monostate, std::string> code;
    std::variant<std::string, std::monostate> kind = std::monostate();
    std::unique_ptr<int> count;
    std::shared_ptr<std::vector<int>> list;
    std::optional<std::unique_ptr<int>> level;
    std::vector<std::unique_ptr<std::string>> tags;
    std::vector<std::shared_ptr<int>> ids;
};
template <> inline constexpr auto yyjson::annotations<Nul> = yyjson::annotate(
    yyjson::field("code", yyjson::max_length(3)),
    yyjson::field("kind", yyjson::enum_("a", "b")),
    yyjson::field("count", yyjson::minimum(0)),
    yyjson::field("list", yyjson::min_items(1), yyjson::items(yyjson::maximum(9))),
    yyjson::field("level", yyjson::maximum(5)),
    yyjson::field("tags", yyjson::items(yyjson::max_length(3))),
    yyjson::field("ids", yyjson::unique_items));
struct Fm
{
    std::optional<std::string> date_time;
    std::optional<std::string> date;
    std::optional<std::string> time;
    std::optional<std::string> duration;
    std::optional<std::string> email;
    std::optional<std::string> hostname;
    std::optional<std::string> ipv4;
    std::optional<std::string> ipv6;
    std::optional<std::string> uri;
    std::optional<std::string> uri_reference;
    std::optional<std::string> uuid;
    std::optional<std::string> uri_template;
    std::optional<std::string> json_pointer;
    std::optional<std::string> relative_json_pointer;
};
template <> inline constexpr auto yyjson::annotations<Fm> = yyjson::annotate(
    yyjson::field("date_time", yyjson::rename("date-time"), yyjson::format("date-time")),
    yyjson::field("date", yyjson::rename("date"), yyjson::format("date")),
    yyjson::field("time", yyjson::rename("time"), yyjson::format("time")),
    yyjson::field("duration", yyjson::rename("duration"), yyjson::format("duration")),
    yyjson::field("email", yyjson::rename("email"), yyjson::format("email")),
    yyjson::field("hostname", yyjson::rename("hostname"), yyjson::format("hostname")),
    yyjson::field("ipv4", yyjson::rename("ipv4"), yyjson::format("ipv4")),
    yyjson::field("ipv6", yyjson::rename("ipv6"), yyjson::format("ipv6")),
    yyjson::field("uri", yyjson::rename("uri"), yyjson::format("uri")),
    yyjson::field("uri_reference", yyjson::rename("uri-reference"), yyjson::format("uri-reference")),
    yyjson::field("uuid", yyjson::rename("uuid"), yyjson::format("uuid")),
    yyjson::field("uri_template", yyjson::rename("uri-template"), yyjson::format("uri-template")),
    yyjson::field("json_pointer", yyjson::rename("json-pointer"), yyjson::format("json-pointer")),
    yyjson::field("relative_json_pointer", yyjson::rename("relative-json-pointer"), yyjson::format("relative-json-pointer")));

struct AdM
{
    std::variant<std::monostate, Circle, Rect> data;
    int n = 0;
};
template <> inline constexpr auto yyjson::annotations<AdM> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
struct AdD
{
    std::variant<Circle, Rect> data = Rect{};
};
template <> inline constexpr auto yyjson::annotations<AdD> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
enum class AKind { circle, rect };
struct AdF
{
    AKind kind = AKind::circle;
    std::variant<Circle, Rect> data;
};
template <> inline constexpr auto yyjson::annotations<AdF> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
// std::monostate named by the tag, which a field of the tag needs
struct AdN
{
    std::variant<std::monostate, Circle> data;
};
template <> inline constexpr auto yyjson::annotations<AdN> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"none", "circle"})));
struct AdNF
{
    std::string kind = "none";
    std::variant<std::monostate, Circle> data;
};
template <> inline constexpr auto yyjson::annotations<AdNF> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"none", "circle"})));
struct Celsius { double v; };
template <> struct yyjson::caster<Celsius> {
    static Celsius from_json(double d) { return {d}; }
    static std::string to_json(const Celsius& c) { return std::to_string(c.v) + "C"; }
};
struct Meters { double v; };
template <> struct yyjson::caster<Meters> {
    static Meters from_json(double d) { return {d}; }
    static double to_json(const Meters& m) { return m.v; }
};
// A caster that takes any value, whose read schema the library cannot derive; `json_view` needs yyjson 0.12.
#if YYJSON_VERSION_HEX >= 0x000C00
using any_value = yyjson::json_view;
#else
using any_value = const yyjson::reader::const_value_ref&;
#endif
struct Stamp { std::int64_t epoch; };
template <> struct yyjson::caster<Stamp> {
    static Stamp from_json(any_value v) { return {v.cast<std::int64_t>()}; }
    static std::string to_json(const Stamp& s) { return std::to_string(s.epoch); }
};
struct Pt { int x, y; };
template <> struct yyjson::caster<Pt> {
    static Pt from_json(const std::array<int, 2>& a) { return {a[0], a[1]}; }
    static void to_json(yyjson::object_emitter& o, const Pt& p) { o.emplace("x", p.x); o.emplace("y", p.y); }
};
struct Uc
{
    Celsius temp{};
    Meters len{};
    Stamp at{};
    Pt pt{};
    std::vector<Celsius> temps;
};
template <> inline constexpr auto yyjson::annotations<Celsius> =
    yyjson::annotate(yyjson::description("degrees Celsius"), yyjson::examples("20.5"));
struct Pt2 { int x = 0; int y = 0; };
template <> inline constexpr auto yyjson::annotations<Pt2> =
    yyjson::annotate(yyjson::title("Point"), yyjson::description("a point"), yyjson::examples(R"({"x":1,"y":2})"));
enum class Hue { red, green };
template <> inline constexpr auto yyjson::annotations<Hue> = yyjson::annotate(yyjson::deprecated, yyjson::title("Hue"));
struct Md
{
    int count = 1;
    std::optional<std::string> name;
    Pt2 at;
    Hue color = Hue::red;
    std::vector<int> few{1};
    Celsius temp{};
    Celsius temp2{};
    std::vector<std::pair<std::string, int>> kv;
};
template <> inline constexpr auto yyjson::annotations<Md> = yyjson::annotate(
    yyjson::description("metadata test"),
    yyjson::field("count", yyjson::minimum(0), yyjson::description("how many"), yyjson::examples("3", "4")),
    yyjson::field("name", yyjson::title("Name"), yyjson::examples(R"("bob")")),
    yyjson::field("at", yyjson::description("where")),
    yyjson::field("color", yyjson::deprecated),
    yyjson::field("few", yyjson::min_items(2)),
    yyjson::field("temp", yyjson::description("air")),
    yyjson::field("kv", yyjson::examples(R"({"a":1})")));
struct Stamp2 { std::int64_t epoch; };
template <> struct yyjson::caster<Stamp2> {
    static Stamp2 from_json(any_value v) { return {v.cast<std::int64_t>()}; }
    static std::int64_t to_json(const Stamp2& s) { return s.epoch; }
};
template <> inline constexpr auto yyjson::annotations<Stamp2> =
    yyjson::annotate(yyjson::schema(R"({"type":"integer"})"), yyjson::description("seconds"));
struct Ext { int a = 0; };
template <> inline constexpr auto yyjson::annotations<Ext> = yyjson::annotate(
    yyjson::schema(R"({"type":"object","required":["a"],"properties":{"a":{"type":"integer","minimum":0}}})"),
    yyjson::schema_name("Ext"), yyjson::title("external"));
struct Ds
{
    int n = 4;
    Celsius temp{};
    Stamp2 at{};
    std::optional<int> never;
    int any = 0;
    Ext ext;
    std::optional<Stamp2> maybe;
};
template <> inline constexpr auto yyjson::annotations<Ds> = yyjson::annotate(
    yyjson::field("n", yyjson::minimum(0), yyjson::schema(R"({"type":"integer","minimum":0,"maximum":100,"title":"x"})"),
                  yyjson::title("N")),
    yyjson::field("temp", yyjson::schema({.write = R"({"type":"string","pattern":"C$"})"})),
    yyjson::field("never", yyjson::schema("false")),
    yyjson::field("any", yyjson::schema("true"), yyjson::description("anything")));

// Types with casters that `yyjson::schema_name` puts in `$defs`: a translator, a declared schema, and a caster for
// reading only, whose write schema the library derives from the struct.
struct Kelvin { double v; };
template <> struct yyjson::caster<Kelvin> {
    static Kelvin from_json(double d) { return {d}; }
    static std::string to_json(const Kelvin& k) { return std::to_string(k.v) + "K"; }
};
template <> inline constexpr auto yyjson::annotations<Kelvin> =
    yyjson::annotate(yyjson::schema_name("Kelvin"), yyjson::description("kelvins"));
struct Epoch { std::int64_t s; };
template <> struct yyjson::caster<Epoch> {
    static Epoch from_json(any_value v) { return {v.cast<std::int64_t>()}; }
    static std::int64_t to_json(const Epoch& e) { return e.s; }
};
template <> inline constexpr auto yyjson::annotations<Epoch> =
    yyjson::annotate(yyjson::schema(R"({"type":"integer"})"), yyjson::schema_name("Epoch"));
struct Half { int v = 0; };
template <> struct yyjson::caster<Half> {
    static Half from_json(int v) { return {v}; }
};
template <> inline constexpr auto yyjson::annotations<Half> = yyjson::annotate(yyjson::schema_name("Half"));
struct NamedCasters
{
    Kelvin k{};
    std::vector<Kelvin> ks;
    Epoch at{};
    Half h{};
};

// Counts the objects that the generator constructs for the defaults.
inline int counted_made = 0;
struct Counted
{
    int x = ++counted_made;
};

struct Order
{
    int id;
    std::string item;
};
template <> inline constexpr auto yyjson::annotations<Order> = yyjson::annotate(yyjson::schema_name("Order"));
struct Receipt
{
    Order order;
    Stamp2 at;
};
template <> inline constexpr auto yyjson::annotations<Receipt> = yyjson::annotate(
    yyjson::schema_name("Receipt"), yyjson::field("at", yyjson::schema({.write = R"({"type":"integer","minimum":0})"})));
struct Same1 { int a; };
template <> inline constexpr auto yyjson::annotations<Same1> = yyjson::annotate(yyjson::schema_name("Same"));
struct Same2 { int b; };
template <> inline constexpr auto yyjson::annotations<Same2> = yyjson::annotate(yyjson::schema_name("Same"));
struct BothSame
{
    Same1 x;
    Same2 y;
};
struct BadExample { int n; };
template <> inline constexpr auto yyjson::annotations<BadExample> =
    yyjson::annotate(yyjson::field("n", yyjson::minimum(0), yyjson::examples("-1")));
struct BadText { int n; };
template <> inline constexpr auto yyjson::annotations<BadText> =
    yyjson::annotate(yyjson::field("n", yyjson::schema(R"({"type":)")));

// A bound wider than the type gives way to the type's, and the other bound stays.
struct Narrow { std::int8_t x = 0; };
template <> inline constexpr auto yyjson::annotations<Narrow> =
    yyjson::annotate(yyjson::field("x", yyjson::minimum(0), yyjson::maximum(1000)));

// min_items and max_items of a map count its properties.
struct Sized
{
    std::map<std::string, int> m;
    std::vector<std::pair<std::string, int>> kv;
};
template <> inline constexpr auto yyjson::annotations<Sized> =
    yyjson::annotate(yyjson::field("m", yyjson::min_items(1), yyjson::max_items(2)),
                     yyjson::field("kv", yyjson::min_items(1), yyjson::max_items(2)));

// A fixed-size array narrowed by min_items and max_items, and a multiset, which keeps repeated elements.
struct Fixed
{
    std::array<int, 3> a{};
    std::array<int, 2> b{};
    std::array<int, 0> none{};
    std::multiset<int> bag;
};
template <> inline constexpr auto yyjson::annotations<Fixed> =
    yyjson::annotate(yyjson::field("a", yyjson::min_items(2), yyjson::max_items(5)),
                     yyjson::field("b", yyjson::min_items(3)));

// An empty tuple, written as an empty object.
struct Empty
{
    std::tuple<> t;
};

// An alternative with a field of the tag key, which is read from the tag.
struct Cat
{
    std::string kind = "cat";
    int lives = 9;
};
struct Dog
{
    std::string kind = "dog";
};
template <> inline constexpr auto yyjson::annotations<Dog> =
    yyjson::annotate(yyjson::field("kind", yyjson::max_length(2)));
using Pet = std::variant<Cat, Dog>;
template <> inline constexpr auto yyjson::annotations<Pet> = yyjson::annotate(yyjson::internal_tag("kind", {"cat", "dog"}));
struct Owner
{
    Pet pet;
};

// Floats, compared with the bounds rounded to a float and written in their shortest text; a bound past the largest
// float leaves the floats up to it.
struct Floats
{
    float tenth = 0;
    float positive = 1;
    float below = 0;
    float large = 3.4028235e38F;
    float pick = 0.1F;
    std::optional<float> huge;
};
template <> inline constexpr auto yyjson::annotations<Floats> = yyjson::annotate(
    yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("positive", yyjson::exclusive_minimum(0)),
    yyjson::field("below", yyjson::exclusive_maximum(0.1), yyjson::minimum(-1)),
    yyjson::field("pick", yyjson::enum_(0.1, 0.2)), yyjson::field("huge", yyjson::maximum(1e300)));

// A recursive variant tagged by `internal_tag`, whose alternatives are written in place: it needs a name, which puts
// it in `$defs`.
struct TLeaf { int v = 0; };
struct TBranch;
using TTree = std::variant<TLeaf, std::shared_ptr<TBranch>>;
struct TBranch { std::vector<TTree> kids; };
template <> inline constexpr auto yyjson::annotations<TTree> =
    yyjson::annotate(yyjson::internal_tag("t", {"leaf", "branch"}), yyjson::schema_name("Tree"));
// Recursive fields through `yyjson::indirect`, which holds a value always, and the nullable form of one
struct IExpr
{
    std::string op;
    std::optional<yyjson::indirect<IExpr>> lhs;
    std::variant<std::monostate, yyjson::indirect<IExpr>> rhs;
    std::vector<yyjson::indirect<IExpr>> args;
};
struct IBranch;
using ITree = std::variant<TLeaf, yyjson::indirect<IBranch>>;
struct IBranch { std::vector<ITree> kids; };
template <> inline constexpr auto yyjson::annotations<ITree> =
    yyjson::annotate(yyjson::internal_tag("t", {"leaf", "branch"}), yyjson::schema_name("ITree"));
struct UBranch;
using UTree = std::variant<TLeaf, std::shared_ptr<UBranch>>;
struct UBranch { std::vector<UTree> kids; };
template <> inline constexpr auto yyjson::annotations<UTree> =
    yyjson::annotate(yyjson::internal_tag("t", {"leaf", "branch"}));

// Alternatives that take the tag through another type: a variant tagged in the same object, a translator to a
// struct, a std::optional; and a variant whose alternative is tagged with the same key.
struct GaugeJson { int level = 0; };
struct Gauge { int level = 0; };
template <> struct yyjson::caster<Gauge> {
    static Gauge from_json(const GaugeJson& g) { return {g.level}; }
    static GaugeJson to_json(const Gauge& g) { return {g.level}; }
};
using Nested = std::variant<Tagged, Gauge, std::optional<Circle>>;
template <> inline constexpr auto yyjson::annotations<Nested> =
    yyjson::annotate(yyjson::internal_tag("type", {"shape", "gauge", "circle"}));
using Twice = std::variant<Tagged>;
template <> inline constexpr auto yyjson::annotations<Twice> = yyjson::annotate(yyjson::internal_tag("kind", {"circle"}));

// `yyjson::any_of` and `yyjson::one_of`: the settings and the metadata of each alternative, a one_of whose alternatives
// overlap, a null alternative, and a named one_of in `$defs`.  A value in `T()` that breaks the settings of the
// alternative that it is held in is no default, also where another alternative reads it back.
struct AltPoint { int x = 0; };
using AltCount = yyjson::any_of<
    yyjson::alt<int, yyjson::minimum(0), yyjson::description("a count"), yyjson::examples("1")>,
    yyjson::alt<std::string, yyjson::max_length(3)>>;
using AltShape = yyjson::one_of<yyjson::alt<AltPoint, yyjson::title("point")>,
                                yyjson::alt<std::vector<int>, yyjson::items(yyjson::enum_(1, 2))>>;
template <> inline constexpr auto yyjson::annotations<AltShape> =
    yyjson::annotate(yyjson::schema_name("Shape"), yyjson::description("a shape"));
using AltSentinel = yyjson::one_of<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
struct Alternatives
{
    AltCount a;
    yyjson::one_of<std::int64_t, std::uint64_t> b = std::int64_t{-1};
    std::vector<yyjson::any_of<std::monostate, yyjson::alt<double, yyjson::maximum(1)>>> c;
    AltShape n;
    AltSentinel s{std::in_place_index<0>, -1};
    std::vector<AltSentinel> t{AltSentinel{std::in_place_index<0>, -1}};
};

// A DOM object or array takes the settings of a map or a range.
struct DomFields
{
    std::optional<yyjson::object> names;
    std::optional<yyjson::array> list;
};
template <>
inline constexpr auto yyjson::annotations<DomFields> = yyjson::annotate(
    yyjson::field("names", yyjson::min_items(1), yyjson::property_names(yyjson::max_length(2))),
    yyjson::field("list", yyjson::max_items(1), yyjson::unique_items));

// `yyjson::contains(yyjson::is_type<T>)` on DOM elements: `contains` has the read schema of T, beside the other
// settings of the element, also in `yyjson::then` and in an alternative.
using TypedMixed =
    yyjson::any_of<yyjson::alt<std::vector<yyjson::value>, yyjson::contains(yyjson::is_type<std::string>)>, int>;
struct TypedContains
{
    std::optional<std::vector<yyjson::value>> events;
    std::optional<std::vector<yyjson::object>> objects;
    std::optional<yyjson::array> numbers;
    std::optional<std::string> kind;
    std::optional<std::vector<yyjson::value>> points;
    TypedMixed mixed = 0;
    std::optional<std::vector<std::vector<yyjson::value>>> nest;
};
template <>
inline constexpr auto yyjson::annotations<TypedContains> = yyjson::annotate(
    yyjson::field("events", yyjson::contains(yyjson::is_type<CondPoint>), yyjson::min_contains(2)),
    yyjson::field("objects", yyjson::contains(yyjson::is_type<CondPoint>, yyjson::min_items(2)),
                  yyjson::max_contains(1)),
    yyjson::field("numbers", yyjson::contains(yyjson::is_type<int>), yyjson::min_items(1)),
    yyjson::field("nest", yyjson::items(yyjson::contains(yyjson::is_type<bool>))),
    yyjson::if_then_else(
        yyjson::if_(yyjson::field("kind", yyjson::const_("point"), yyjson::required)),
        yyjson::then(yyjson::field("points", yyjson::required, yyjson::contains(yyjson::is_type<CondPoint>)))));

// `yyjson::contains` in `yyjson::if_` and `yyjson::not_`, with the read schema of `yyjson::is_type<T>` under `not`.
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
    yyjson::not_(yyjson::field("deps", yyjson::required, yyjson::contains(yyjson::is_type<CondPoint>),
                               yyjson::min_contains(2))));

// `yyjson::not_` of values as `not`, of `anyOf` when a value has several.
struct NegatedValues
{
    int n = 0;
    std::optional<std::string> s;
    std::vector<int> v;
    yyjson::value any = 0;
    std::optional<yyjson::value> point;
    float f = 0;
    std::map<std::string, int> mp;
    std::variant<std::monostate, int> nv;
    yyjson::any_of<yyjson::alt<int, yyjson::not_(yyjson::const_(7))>, std::string> u = 0;
    std::optional<int> m;
};
template <>
inline constexpr auto yyjson::annotations<NegatedValues> = yyjson::annotate(
    yyjson::field("n", yyjson::not_(yyjson::minimum(10), yyjson::maximum(19)), yyjson::not_(yyjson::const_(3))),
    yyjson::field("s", yyjson::not_(yyjson::pattern("^x"))),
    yyjson::field("v", yyjson::items(yyjson::not_(yyjson::enum_(1, 2)))),
    yyjson::field("any", yyjson::not_(yyjson::is_type<std::nullptr_t>)),
    yyjson::field("point", yyjson::not_(yyjson::is_type<CondPoint>)),
    yyjson::field("f", yyjson::not_(yyjson::const_(0.1))),
    yyjson::field("mp", yyjson::not_(yyjson::min_items(2))),
    yyjson::field("nv", yyjson::not_(yyjson::const_(3))),
    yyjson::if_then_else(yyjson::if_(yyjson::field("n", yyjson::const_(1))),
                         yyjson::then(yyjson::field("m", yyjson::not_(yyjson::const_(0))))));

// A field that its caster leaves out, which only `yyjson::strict` requires: the struct that holds it is a default of
// the schema without `yyjson::strict`.
struct MaybeInt
{
    bool has = false;
    int value = 0;
};
template <>
struct yyjson::caster<MaybeInt>
{
    static bool should_omit(const MaybeInt& m) { return !m.has; }
    static int to_json(const MaybeInt& m) { return m.value; }
    static MaybeInt from_json(const yyjson::json_view& v) { return {true, v.cast<int>()}; }
};
struct MaybeInner { MaybeInt m; };
struct MaybeOuter { MaybeInner in; };

// Written only: a borrowed string is not read.
struct Borrowed { std::string_view s = "x"; const char* p = "y"; };

namespace
{
    namespace jss = jsoncons::jsonschema;
    using jsoncons::json;

    // Where the validator and the library are known to differ on a document, for the read schema without and with
    // `yyjson::strict`.
    enum difference : unsigned
    {
        same = 0,
        // the library reads a document that the schema does not describe, as validation.md lists
        lenient_read = 1,
        lenient_strict = 2,
        lenient = lenient_read | lenient_strict,
        // the validator and the library differ on a string with a format: jsoncons does not check uuid, duration,
        // uri-template and relative-json-pointer, checks hostname and ipv4 less than JSON-Schema-Test-Suite does,
        // and rejects the empty uri-reference
        format_read = 4,
        format_strict = 8,
        format = format_read | format_strict,
        // the schema describes a document that the library does not read, one of the exceptions that json-schema.md
        // lists
        loose_read = 16,
        loose_strict = 32,
        loose = loose_read | loose_strict,
    };

    struct doc
    {
        doc(const char* json, unsigned diff = same) : json(json), diff(diff) {}
        std::string_view json;
        unsigned diff;
    };

    json to_jsoncons(const yyjson::object& schema) { return json::parse(std::string(schema.write())); }

    jss::json_schema<json> compile(const json& schema)
    {
        return jss::make_json_schema(schema, jss::evaluation_options{}.require_format_validation(true));
    }

    const jss::json_schema<json>& meta_schema()
    {
        static const auto meta = compile(json::parse(R"({"$ref":"https://json-schema.org/draft/2020-12/schema"})"));
        return meta;
    }

    // Every `default` and every example in `node` is valid against the schema that holds it.
    void check_annotations(const json& node, const json& root, const std::string& path)
    {
        if (node.is_object())
        {
            if (node.contains("default") || node.contains("examples"))
            {
                auto sub = node;
                sub.erase("default");
                sub.erase("examples");
                if (root.contains("$defs")) sub["$defs"] = root["$defs"];
                const auto validator = compile(sub);
                if (node.contains("default"))
                {
                    EXPECT_TRUE(validator.is_valid(node["default"])) << "default at " << path;
                }
                if (node.contains("examples"))
                {
                    for (const auto& e : node["examples"].array_range())
                        EXPECT_TRUE(validator.is_valid(e)) << "example " << e.to_string() << " at " << path;
                }
            }
            for (const auto& m : node.object_range()) check_annotations(m.value(), root, path + "/" + m.key());
        }
        else if (node.is_array())
        {
            auto i = 0;
            for (const auto& e : node.array_range()) check_annotations(e, root, path + "/" + std::to_string(i++));
        }
    }

    template <typename T>
    bool reads(std::string_view json, bool strict)
    {
        try
        {
            if (strict)
                (void)yyjson::read<T>(json, yyjson::strict);
            else
                (void)yyjson::read<T>(json);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    }

    // The schemas of T are valid 2020-12 schemas, and the validator agrees with `yyjson::read<T>` on `docs` but
    // where a document is marked with a difference.  What `write` writes of a document that T reads is valid
    // against the write schema.
    template <typename T>
    void check(std::initializer_list<doc> docs)
    {
        const auto schemas = std::array{
            to_jsoncons(yyjson::json_schema<T>()), to_jsoncons(yyjson::json_schema<T>(yyjson::strict)),
            to_jsoncons(yyjson::json_schema<T>({.direction = yyjson::schema_direction::write}))};
        for (const auto& s : schemas)
        {
            EXPECT_TRUE(meta_schema().is_valid(s)) << s.to_string();
            check_annotations(s, s, "");
        }
        const auto validators = std::array{compile(schemas[0]), compile(schemas[1]), compile(schemas[2])};
        for (const auto& d : docs)
        {
            const auto j = json::parse(d.json);
            for (const auto strict : {false, true})
            {
                const auto mode = strict ? "strict" : "read";
                const auto valid = validators[strict].is_valid(j);
                const auto accepted = reads<T>(d.json, strict);
                const auto lenient_bit = strict ? lenient_strict : lenient_read;
                const auto format_bit = strict ? format_strict : format_read;
                const auto loose_bit = strict ? loose_strict : loose_read;
                if (valid && !accepted)
                    EXPECT_TRUE(d.diff & (loose_bit | format_bit)) << mode << ": valid, but not read: " << d.json;
                else if (!valid && accepted)
                    EXPECT_TRUE(d.diff & (lenient_bit | format_bit)) << mode << ": read, but not valid: " << d.json;
                else
                    EXPECT_FALSE(d.diff & (lenient_bit | format_bit | loose_bit))
                        << mode << ": marked, but agree: " << d.json;
            }
            if (!(d.diff & format) && reads<T>(d.json, false))
            {
                const auto written = std::string(yyjson::write(yyjson::read<T>(d.json)));
                EXPECT_TRUE(validators[2].is_valid(json::parse(written))) << "write: " << written;
            }
        }
    }
}  // namespace

TEST(Schema, Person)
{
    check<Person>({
        R"({"name":"a"})",
        R"({})",
        R"({"name":""})",
        R"({"name":"a","age":151})",
        R"({"name":"a","age":2.0})",
        R"({"name":"a","age":2.5})",
        R"({"name":"a","age":-1})",
        R"({"name":"a","tags":["x","Y"]})",
        R"({"name":"a","tags":["x","x"]})",
        R"({"name":"a","unknown":1})",
        R"({"name":"a","address":null})",
        R"({"name":"a","address":{"city":"k"}})",
        R"({"name":"a","address":{"zip":null}})",
        R"({"name":"a","opt":null})",
        R"({"name":"a","color":"red"})",
        R"({"name":"a","color":"pink"})",
        R"({"name":"a","scores":{"x":50}})",
        R"({"name":"a","scores":{"x":500}})",
        R"({"name":"a","scores":{"xxxxxxxxxxxxxxxxxxxxxxxxxx":5}})",
        {R"({"name":"a","ids":[1,1]})", lenient_read},
        R"({"name":"a","pair":[1,"x"]})",
        R"({"name":"a","pair":[1]})",
        R"({"name":"a","pair":[1,"x",3]})",
        R"({"name":"a","pos":[1,2]})",
        R"({"name":"a","pos":[1,2,3]})",
        R"({"name":"a","shape":{"kind":"rect","w":1,"h":2}})",
        R"({"name":"a","shape":{"w":1,"h":2}})",
        R"({"name":"a","shape":{"kind":"circle","r":1,"extra":1}})",
        R"({"name":"a","u":[1,2]})",
        R"({"name":"a","u":1.5})",
        R"({"name":"a","tree":null})",
        R"({"name":"a","tree":{"value":1,"children":[{"value":2}]}})",
        R"({"name":"a","big":1e30})",
    });
}

TEST(Schema, Msg)
{
    check<Msg>({
        R"({})",
        R"({"id":"x"})",
        R"({"ID":"x"})",
        R"({"hidden":1})",
        R"({"type":"rect","body":{"w":1,"h":1}})",
        R"({"type":"circle","body":{"w":1}})",
        R"({"type":"rect"})",
        R"({"body":{"r":1}})",
        R"({"body":null})",
        R"({"type":"oval","body":{}})",
        R"({"perm":1})",
        R"({"perm":"read"})",
        R"({"perm":7})",
        R"({"perm":-1})",
        R"({"level":"HIGH"})",
        R"({"level":"high"})",
        R"({"level":1})",
        R"({"level":5})",
        R"({"by_level":{"low":1}})",
        R"({"by_level":{"x":1}})",
        R"({"by_level":{"0":1}})",
        R"({"nums":[10,1,2]})",
        R"({"nums":[9]})",
        R"({"nums":[10,100,6]})",
        R"({"nums":[10,100,5]})",
        R"({"c":[1,10]})",
        R"({"c":[1,2]})",
        R"({"c":[10,11]})",
        R"({"c":[]})",
        R"({"f":1e39})",
        R"({"f":3.4e38})",
        R"({"ch":200})",
        R"({"ch":65})",
        R"({"holes":[1,null]})",
        R"({"kv":{"a":1}})",
        {R"({"kv":[["a",1]]})", lenient_read},
        R"({"imap":[[1,"a"]]})",
        R"({"imap":{"1":"a"}})",
    });
}

TEST(Schema, Uc)
{
    check<Uc>({
        R"({})",
        R"({"temp":20,"len":1,"at":5,"pt":[1,2],"temps":[1,2]})",
        R"({"temp":-300})",
        R"({"len":-1})",
        R"({"temp":"20C"})",
        R"({"pt":{"x":1,"y":2}})",
    });
}

TEST(Schema, Kw)
{
    check<Kw>({
        R"({})",
        R"({"version":2,"kind":"b","ratio":0.25,"on":true,"when":"2024-01-02T03:04:05Z","codes":["x"]})",
        R"({"version":3})",
        R"({"kind":"c"})",
        R"({"ratio":0.3})",
        R"({"on":false})",
        R"({"when":"x"})",
        R"({"codes":["z"]})",
        R"({"card":"1"})",
        R"({"card":"1","billing":"b","zip":"z"})",
        R"({"billing":"b"})",
        R"({"billing":"b","zip":"z"})",
        R"({"version":2,"kind":"a","ratio":0.5,"on":true,"when":"2024-01-02T03:04:05Z","codes":[],"card":"1"})",
    });
}

TEST(Schema, KwEnum)
{
    check<KwEnum>({
        R"({})",
        R"({"tone":"HIGH","tones":["low"]})",
        R"({"tone":"high"})",
        R"({"tone":"low"})",
        R"({"tones":["HIGH"]})",
        R"({"mode":"low"})",
        R"({"mode":"low","level":1})",
        R"({"mode":"HIGH"})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<KwEnum>())["$defs"]["KwEnum"];
    EXPECT_EQ(s["properties"]["tone"]["const"], json("HIGH"));
    EXPECT_EQ(s["properties"]["tones"]["items"]["enum"], json::parse(R"(["low"])"));
    EXPECT_EQ(s["if"]["properties"]["mode"]["const"], json("low"));
}

// The constraints hold for the value of a pointer or of a variant with std::monostate, and null passes them.
TEST(Schema, Nul)
{
    check<Nul>({
        R"({})",
        R"({"code":null,"kind":null,"count":null,"list":null,"level":null,"tags":[null],"ids":[null,1]})",
        R"({"code":"abc","kind":"a","count":0,"list":[9],"level":5,"tags":["abc"],"ids":[1,2]})",
        R"({"code":"abcd"})",
        R"({"kind":"c"})",
        R"({"count":-1})",
        R"({"list":[]})",
        R"({"list":[10]})",
        R"({"level":6})",
        R"({"tags":["abcd"]})",
        R"({"ids":[1,1]})",
        R"({"ids":[null,null]})",
    });
}

TEST(Schema, Fm)
{
    check<Fm>({
        R"__({"date-time": "2024-01-02T03:04:05Z"})__",
        R"__({"date-time": "2024-01-02t03:04:05z"})__",
        R"__({"date-time": "2024-01-02T03:04:05.123+09:00"})__",
        R"__({"date-time": "2024-02-30T00:00:00Z"})__",
        R"__({"date-time": "2024-01-02T24:00:00Z"})__",
        R"__({"date-time": "2024-01-02T23:59:60Z"})__",
        R"__({"date-time": "2024-01-02 03:04:05Z"})__",
        R"__({"date-time": "2024-01-02T03:04:05"})__",
        R"__({"date": "2024-01-02"})__",
        R"__({"date": "2024-02-29"})__",
        R"__({"date": "2023-02-29"})__",
        R"__({"date": "2024-1-02"})__",
        R"__({"date": "20240102"})__",
        R"__({"time": "03:04:05Z"})__",
        R"__({"time": "03:04:05+09:00"})__",
        R"__({"time": "03:04:05.5Z"})__",
        R"__({"time": "03:04:05"})__",
        R"__({"time": "23:59:60Z"})__",
        R"__({"time": "24:00:00Z"})__",
        R"__({"duration": "P1D"})__",
        R"__({"duration": "PT1H"})__",
        R"__({"duration": "P1W"})__",
        R"__({"duration": "P1Y2M"})__",
        {R"__({"duration": "PT"})__", format},
        {R"__({"duration": "P"})__", format},
        {R"__({"duration": "P1DT"})__", format},
        {R"__({"duration": "P1.5D"})__", format},
        {R"__({"duration": "PT0.5S"})__", format},
        R"__({"email": "a@example.com"})__",
        R"__({"email": "a@b"})__",
        R"__({"email": "a.@b.c"})__",
        R"__({"email": "\"a b\"@example.com"})__",
        R"__({"email": "a@[127.0.0.1]"})__",
        R"__({"email": "@b.c"})__",
        R"__({"email": "a@b..c"})__",
        R"__({"hostname": "example.com"})__",
        R"__({"hostname": "a"})__",
        R"__({"hostname": "-a.com"})__",
        {R"__({"hostname": "a-.com"})__", format},
        R"__({"hostname": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.com"})__",
        R"__({"hostname": "xn--abc.com"})__",
        R"__({"hostname": "a_b.com"})__",
        R"__({"hostname": "example.com."})__",
        R"__({"ipv4": "127.0.0.1"})__",
        {R"__({"ipv4": "256.0.0.1"})__", format},
        R"__({"ipv4": "01.0.0.1"})__",
        R"__({"ipv4": "1.2.3"})__",
        R"__({"ipv4": "1.2.3.4.5"})__",
        R"__({"ipv6": "::1"})__",
        R"__({"ipv6": "1::"})__",
        R"__({"ipv6": "::ffff:1.2.3.4"})__",
        R"__({"ipv6": "1:2:3:4:5:6:7:8"})__",
        R"__({"ipv6": "1:2:3:4:5:6:7:8:9"})__",
        R"__({"ipv6": "fe80::1%eth0"})__",
        R"__({"ipv6": "::g"})__",
        R"__({"uri": "http://a.b/c?d#e"})__",
        R"__({"uri": "urn:isbn:1"})__",
        R"__({"uri": "a"})__",
        R"__({"uri": "http://a b"})__",
        R"__({"uri": "http://[::1]/"})__",
        R"__({"uri": "mailto:a@b.c"})__",
        R"__({"uri": "http://a/%zz"})__",
        {R"__({"uri-reference": ""})__", format},
        R"__({"uri-reference": "a/b"})__",
        R"__({"uri-reference": "#x"})__",
        R"__({"uri-reference": "http://a"})__",
        R"__({"uri-reference": "a b"})__",
        R"__({"uri-reference": "%zz"})__",
        R"__({"uuid": "123e4567-e89b-12d3-a456-426614174000"})__",
        R"__({"uuid": "123E4567-E89B-12D3-A456-426614174000"})__",
        {R"__({"uuid": "123e4567e89b12d3a456426614174000"})__", format},
        {R"__({"uuid": "123e4567-e89b-12d3-a456-42661417400"})__", format},
        R"__({"uri-template": "http://a/{b}"})__",
        {R"__({"uri-template": "http://a/{b"})__", format},
        R"__({"uri-template": "{+x}"})__",
        R"__({"uri-template": "{x,y}"})__",
        {R"__({"uri-template": "{a b}"})__", format},
        R"__({"json-pointer": ""})__",
        R"__({"json-pointer": "/a"})__",
        R"__({"json-pointer": "/a~0b~1c"})__",
        R"__({"json-pointer": "a"})__",
        R"__({"json-pointer": "/a~2"})__",
        R"__({"json-pointer": "/a~"})__",
        R"__({"relative-json-pointer": "0"})__",
        R"__({"relative-json-pointer": "1/a"})__",
        R"__({"relative-json-pointer": "0#"})__",
        {R"__({"relative-json-pointer": "01"})__", format},
        {R"__({"relative-json-pointer": "-1"})__", format},
        {R"__({"relative-json-pointer": "#"})__", format},
    });
}

TEST(Schema, AdM)
{
    check<AdM>({
        R"({})",
        R"({"kind":"circle"})",
        R"({"kind":"rect"})",
        R"({"data":null})",
        R"({"data":{"r":1}})",
        R"({"kind":"circle","data":{"r":1}})",
        R"({"kind":"rect","data":{"w":1,"h":2}})",
        R"({"kind":"rect","data":{"r":1}})",
        R"({"kind":"x","data":{"r":1}})",
        R"({"kind":"circle","data":null})",
        R"({"kind":null})",
        R"({"extra":1})",
        R"({"kind":"rect","data":{"w":1,"h":2},"extra":1})",
    });
}

TEST(Schema, AdD)
{
    check<AdD>({
        R"({})",
        R"({"kind":"circle"})",
        R"({"kind":"rect"})",
        R"({"data":null})",
        R"({"data":{"r":1}})",
        R"({"kind":"circle","data":{"r":1}})",
        R"({"kind":"rect","data":{"w":1,"h":2}})",
        R"({"kind":"rect","data":{"r":1}})",
        R"({"kind":"x","data":{"r":1}})",
        R"({"kind":"circle","data":null})",
        R"({"kind":null})",
        R"({"extra":1})",
        R"({"kind":"rect","data":{"w":1,"h":2},"extra":1})",
    });
}

TEST(Schema, AdF)
{
    check<AdF>({
        R"({})",
        R"({"kind":"circle"})",
        R"({"kind":"rect"})",
        R"({"data":null})",
        R"({"data":{"r":1}})",
        R"({"kind":"circle","data":{"r":1}})",
        R"({"kind":"rect","data":{"w":1,"h":2}})",
        R"({"kind":"rect","data":{"r":1}})",
        R"({"kind":"x","data":{"r":1}})",
        R"({"kind":"circle","data":null})",
        R"({"kind":null})",
        R"({"extra":1})",
        R"({"kind":"rect","data":{"w":1,"h":2},"extra":1})",
    });
}

TEST(Schema, AdN)
{
    check<AdN>({
        R"({})",
        R"({"kind":"none"})",
        R"({"kind":"circle"})",
        R"({"data":null})",
        R"({"kind":"none","data":null})",
        R"({"kind":"none","data":{"r":1}})",
        R"({"kind":"circle","data":{"r":1}})",
        R"({"kind":"circle","data":null})",
        R"({"data":{"r":1}})",
    });
}

TEST(Schema, AdNF)
{
    check<AdNF>({
        R"({})",
        R"({"kind":"none"})",
        R"({"kind":"circle"})",
        R"({"data":null})",
        R"({"kind":"none","data":null})",
        R"({"kind":"none","data":{"r":1}})",
        R"({"kind":"circle","data":{"r":1}})",
        R"({"kind":"circle","data":null})",
        R"({"data":{"r":1}})",
    });
}

TEST(Schema, TupKvMixed)
{
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, double>>>({
        R"({"a":1,"b":2.5})",
        {R"({"a":2.5,"b":1})", loose},
        R"({"a":1,"b":2})",
        R"({"a":1})",
        {R"([["a",1],["b",2.5]])", lenient},
        R"({"a":1,"b":"x"})",
    });
}

TEST(Schema, TupKv)
{
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>>({
        R"({"a":1,"b":2})",
        R"({"a":1})",
        R"({"a":1,"b":2,"c":3})",
        {R"([["a",1],["b",2]])", lenient},
    });
}

TEST(Schema, TupIntKey)
{
    check<std::tuple<std::pair<int, int>>>({
        R"([[1,2]])",
        R"({"1":2})",
    });
}

TEST(Schema, TupEnumKey)
{
    check<std::tuple<std::pair<Level, int>>>({
        R"([["HIGH",2]])",
        R"({"HIGH":2})",
        R"([[1,2]])",
    });
}

TEST(Schema, TkC)
{
    check<TkC>({
        R"({"t":{"a":1,"b":2}})",
        R"({"t":{"a":-1,"b":2}})",
        R"({"t":{"a":1,"b":10}})",
        R"({"t":{"aa":1,"b":2}})",
        {R"({"t":{"a":1,"bb":2}})", lenient},
        R"({"t":{"b":1,"a":2}})",
    });
}

TEST(Schema, Md)
{
    check<Md>({
        R"({"few":[1,2]})",
        R"({"count":-1,"few":[1,2]})",
        R"({"few":[1]})",
        R"({"few":[1,2],"temp":3})",
    });
}

TEST(Schema, Ds)
{
    check<Ds>({
        R"({})",
        R"({"n":5,"temp":1.5,"at":3,"ext":{"a":1}})",
        R"({"n":-1})",
        R"({"at":"x"})",
        R"({"maybe":2})",
        {R"({"ext":{}})", lenient_read},
        R"({"temp":"1C"})",
    });
}

TEST(Schema, Loose)
{
    check<Loose>({
        R"({"a":1,"b":2})",
        R"({})",
    });
}

// A field annotated with `yyjson::omit_default` is not required in any schema, and the write output, which leaves
// out a value that violates the constraint, is valid.
TEST(Schema, OmitDefault)
{
    check<OmitD>({
        R"({})",
        R"({"a":1})",
        R"({"a":1,"limit":3,"name":"y"})",
        R"({"a":1,"limit":-1})",
        R"({"a":1,"limit":-2})",
    });
    for (const auto& s : {yyjson::json_schema<OmitD>(yyjson::strict),
                          yyjson::json_schema<OmitD>({.direction = yyjson::schema_direction::write})})
        EXPECT_EQ(to_jsoncons(s)["$defs"]["OmitD"]["required"], json::parse(R"(["a"])")) << s.write();
}

TEST(Schema, DefaultFrom)
{
    check<DefaultFromD>({
        R"({"name":"a"})",
        R"({"name":"a","child":{"name":"b","child":{"name":"c"}},"sizes":[],"limit":3})",
        R"({"name":"a","limit":-1})",
        R"({"name":"a","child":null})",
        R"({})",
    });
    // the defaults are what the functions make, except one that violates the constraint; the value that a function
    // makes is not filled, so the fields of the child that differ from their defaults are written
    const auto s = to_jsoncons(yyjson::json_schema<DefaultFromD>())["$defs"]["DefaultFromD"]["properties"];
    EXPECT_EQ(s["child"]["default"], json::parse(R"({"name":"leaf","sizes":[],"limit":0})"));
    EXPECT_EQ(s["sizes"]["default"], json::parse("[1,2]"));
    EXPECT_FALSE(s["limit"].contains("default"));
    for (const auto& t : {yyjson::json_schema<DefaultFromD>(yyjson::strict),
                          yyjson::json_schema<DefaultFromD>({.direction = yyjson::schema_direction::write})})
        EXPECT_EQ(to_jsoncons(t)["$defs"]["DefaultFromD"]["required"], json::parse(R"(["name"])")) << t.write();
}

TEST(Schema, RequiredGroups)
{
    check<Groups>({
        R"({"env":"e","email":"a"})",
        R"({"value":1,"u":"m","phone":"p"})",
        R"({"env":"e","phone":"p","email":"a"})",
        R"({"value":1,"phone":"p"})",
        R"({"env":"e","value":1,"u":"m","phone":"p"})",
        R"({"env":"e"})",
        R"({"value":1,"u":"m","email":"a"})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<Groups>())["$defs"]["Groups"];
    EXPECT_EQ(s["oneOf"], json::parse(R"([{"required":["env"]},{"required":["value","u"]}])"));
    // the keyword that the struct already has goes into allOf
    EXPECT_EQ(s["anyOf"], json::parse(R"([{"required":["email"]},{"required":["phone"]}])"));
    EXPECT_EQ(s["allOf"], json::parse(R"([{"anyOf":[{"required":["env"]},{"required":["phone"]}]}])"));
}

TEST(Schema, Conditions)
{
    check<Conditioned>({
        R"({"kind":"circle","radius":1})",
        R"({"kind":"circle"})",
        R"({"kind":"circle","radius":-1})",
        R"({"kind":"sq","p":{"x":1}})",
        R"({"kind":"sq","p":{"x":"a"}})",
        R"({"kind":"sq","radius":-1})",
        R"({"kind":"xx"})",
        R"({})",
        R"({"p":3})",
        R"({"kind":"circle","radius":2,"p":5})",
    });
    check<OneCondition>({
        R"({})",
        R"({"a":1})",
        R"({"a":1,"b":[1]})",
        R"({"a":1,"b":[1,2]})",
        R"({"a":1,"b":[0,2]})",
        R"({"a":9})",
        R"({"b":[0]})",
    });
    // a value of is_type is written as it is read, so the write schema takes the read schema of its type, here
    // with a key that CondOpen ignores
    check<OpenCondition>({
        R"({"p":{"x":1,"z":2}})",
        R"({"p":{"x":"a"}})",
        R"({})",
    });
    EXPECT_NO_THROW(yyjson::validate(yyjson::read<OpenCondition>(R"({"p":{"x":1,"z":2}})", yyjson::strict)));
    check<CondKeys>({
        R"({"b":1})",
        R"({"b":-1})",
        R"({"b":1,"e":0})",
        R"({"b":1,"e":2})",
        R"({"b":1,"e":2,"d":3})",
        R"({"e":0,"d":3,"b":1})",
        R"({"b":1,"d":3})",
    });
    // validate reads the value of is_type under strict, as the read of the output does
    const auto loose = yyjson::read<Conditioned>(R"({"kind":"sq","p":{"x":1,"y":2}})");
    EXPECT_THROW(yyjson::validate(loose), yyjson::bad_cast);
    const auto s = to_jsoncons(yyjson::json_schema<Conditioned>())["$defs"]["Conditioned"];
    ASSERT_EQ(s["allOf"].size(), 2);
    EXPECT_EQ(s["allOf"][0]["if"],
              json::parse(R"({"properties":{"kind":{"type":"string","const":"circle"}},"required":["kind"]})"));
    EXPECT_EQ(s["allOf"][0]["then"]["required"], json::parse(R"(["radius"])"));
    EXPECT_EQ(s["allOf"][1], json::parse(R"({"not":{"properties":{"kind":{"type":"string","pattern":"^x"}}}})"));
    const auto one = to_jsoncons(yyjson::json_schema<OneCondition>())["$defs"]["OneCondition"];
    EXPECT_TRUE(one.contains("if") && one.contains("then") && !one.contains("else") && !one.contains("allOf"));
}

TEST(Schema, ExtraKeys)
{
    check<Extra>({
        R"({"a":1})",
        R"({"a":1,"x":[1,3],"y":[]})",
        R"({"x":[4]})",
        R"({"a":1,"x":2})",
    });
    for (const auto& s :
         {to_jsoncons(yyjson::json_schema<Extra>()), to_jsoncons(yyjson::json_schema<Extra>(yyjson::strict))})
        EXPECT_EQ(s["$defs"]["Extra"]["additionalProperties"],
                  json::parse(R"({"type":"array","items":{"type":"integer","minimum":-2147483648,"maximum":3}})"));
    check<ExtraShape>({
        R"({"type":"c","r":1,"x":{"y":null}})",
        R"({"type":"c","x":1})",
        R"({"type":"g","env":"e","email":"a"})",
    });
    check<ExtraNamed>({
        R"({"long_name":1})",
        R"({"long_name":1,"x-a":2})",
        R"({"x-a":2,"y":3})",
        R"({"long_name":"s"})",
    });
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<ExtraNamed>())["$defs"]["ExtraNamed"]["propertyNames"],
              json::parse(R"({"anyOf":[{"enum":["long_name"]},{"pattern":"^x-"}]})"));
}

TEST(Schema, Transparent)
{
    check<TAccount>({
        R"({"id":3,"tags":["a"],"ids":[],"shape":{"t":"p","x":1}})",
        R"({"id":3,"tags":["a"],"ids":[1,2],"limit":10,"shape":{"t":"w","x":1}})",
        R"({"id":3,"tags":["a"],"ids":[],"shape":{"t":"b","k":1}})",
        R"({"id":3,"tags":["a"],"ids":[],"shape":{"t":"b"}})",
        R"({"id":0,"tags":["a"],"ids":[],"shape":{"t":"p"}})",
        R"({"id":3,"tags":[],"ids":[],"shape":{"t":"p"}})",
        R"({"id":3,"tags":["long"],"ids":[],"shape":{"t":"p"}})",
        R"({"id":3,"tags":["a"],"ids":[0],"shape":{"t":"p"}})",
        R"({"id":3,"tags":["a"],"ids":[],"limit":11,"shape":{"t":"p"}})",
        R"({"id":3,"tags":["a"],"ids":[],"limit":null,"shape":{"t":"p"}})",
        R"({"id":"3","tags":["a"],"ids":[],"shape":{"t":"p"}})",
    });
    // a transparent struct is a definition of the schema of its field, with its metadata
    const auto read = to_jsoncons(yyjson::json_schema<TAccount>());
    EXPECT_EQ(read["$defs"]["TId"],
              json::parse(R"({"type":"integer","maximum":2147483647,"minimum":1,"description":"A user"})"));
    // a field of an optional value is defined by the value type, and a field of the struct takes no null, as a
    // `std::optional` field does
    EXPECT_EQ(read["$defs"]["TLimit"], json::parse(R"({"type":"integer","minimum":-2147483648,"maximum":10})"));
    EXPECT_EQ(read["$defs"]["TAccount"]["properties"]["limit"], json::parse(R"({"$ref":"#/$defs/TLimit","default":7})"));
    // elsewhere null empties the field
    check<std::vector<TLimit>>({
        R"([null,3])",
        R"([11])",
    });
    check<std::map<std::string, TLimit>>({
        R"({"a":null,"b":3})",
        R"({"a":11})",
    });
    check<TLimit>({
        R"(null)",
        R"(10)",
        R"(11)",
    });
    // a field of an optional value is not required by `yyjson::strict`, nor by the write schema
    for (const auto& s : {to_jsoncons(yyjson::json_schema<TAccount>(yyjson::strict)),
                          to_jsoncons(yyjson::json_schema<TAccount>({.direction = yyjson::schema_direction::write}))})
        EXPECT_EQ(s["$defs"]["TAccount"]["required"], json::parse(R"(["id","tags","ids","shape"])"));
    // and the root refers to it, as to a struct
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<TId>())["$ref"], json("#/$defs/TId"));
}

TEST(Schema, VecOpt)
{
    check<std::vector<std::optional<int>>>({
        R"([1,null])",
        R"([1.5])",
    });
}

TEST(Schema, Set)
{
    check<std::set<std::string>>({
        {R"(["a","a"])", lenient_read},
        R"(["a","b"])",
    });
}

TEST(Schema, UMap)
{
    check<std::unordered_map<std::string, std::vector<double>>>({
        R"({"a":[1.5]})",
        R"({"a":1})",
    });
}

TEST(Schema, St)
{
    check<St>({
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{}})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"b":1})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"b":null})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"bb":null})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"x":null})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"u":null})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"u":[1]})",
        {R"({"a":1,"s":[1,1],"m":{},"t":{"kind":"circle","r":1},"kv":{}})", lenient_read},
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1,"w":2},"kv":{}})",
        {R"({"a":1,"s":[],"m":{},"t":{"kind":"rect","w":1,"h":2},"kv":[["a",1],["a",2]]})", lenient},
        R"({"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{}})",
        R"({"a":1,"s":[],"m":{},"t":{"r":1},"kv":{}})",
    });
}

// The read schemas of a generation take the defaults from one `S()`, and the write schemas from none.
TEST(Schema, DefaultsOnce)
{
    using enum yyjson::schema_direction;
    counted_made = 0;
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<Counted>())["$defs"]["Counted"]["properties"]["x"]["default"], 1);
    EXPECT_EQ(counted_made, 1);
    (void)yyjson::json_schema<Counted>({.direction = write});
    EXPECT_EQ(counted_made, 1);
    (void)yyjson::schema_collection().add<Counted>().add<std::vector<Counted>>().add<Counted>(write).build();
    EXPECT_EQ(counted_made, 2);
}

// A collection names a type once while its read and write schemas are the same, and splits it otherwise; its
// references resolve in a document that holds the schemas at the prefix (jsoncons resolves them only under a keyword
// that it knows, such as `$defs`).
TEST(Schema, Collection)
{
    auto names = [](const yyjson::schema_collection::result& r) {
        const auto schemas = to_jsoncons(r.schemas());
        std::vector<std::string> v;
        for (const auto& m : schemas.object_range()) v.emplace_back(m.key());
        return v;
    };
    using enum yyjson::schema_direction;

    // A result keeps the names settled when it was built.
    auto lax = yyjson::schema_collection();
    const auto first = lax.add<Order>().build();
    const auto second = lax.add<Order>(write).build();
    EXPECT_EQ(names(first), (std::vector<std::string>{"Order"}));
    EXPECT_EQ(first.schema<Order>().write(), R"({"$ref":"#/components/schemas/Order"})");
    EXPECT_THROW((void)first.schema<Order>(write), std::logic_error);
    EXPECT_EQ(names(second), (std::vector<std::string>{"Order-Input", "Order-Output"}));
    EXPECT_EQ(second.schema<Order>().write(), R"({"$ref":"#/components/schemas/Order-Input"})");
    EXPECT_EQ(second.schema<Order>(write).write(), R"({"$ref":"#/components/schemas/Order-Output"})");

    const auto strict =
        yyjson::schema_collection(yyjson::strict, {.ref_prefix = "#/$defs/"}).add<Receipt>().add<Receipt>(write).build();
    EXPECT_EQ(names(strict), (std::vector<std::string>{"Order", "Receipt-Input", "Receipt-Output"}));
    EXPECT_THROW((void)strict.schema<Same1>(), std::logic_error);

    auto document = json(jsoncons::json_object_arg);
    document["$defs"] = to_jsoncons(strict.schemas());
    for (const auto direction : {read, write})
    {
        document["$ref"] = to_jsoncons(strict.schema<Receipt>(direction))["$ref"];
        const auto validator = compile(document);
        EXPECT_TRUE(validator.is_valid(json::parse(R"({"order":{"id":1,"item":"a"},"at":2})")));
        EXPECT_FALSE(validator.is_valid(json::parse(R"({"order":{"id":1},"at":2})")));
        EXPECT_EQ(validator.is_valid(json::parse(R"({"order":{"id":1,"item":"a"},"at":-1})")), direction == read);
    }
}

TEST(Schema, Narrow)
{
    check<Narrow>({
        R"({"x":0})",
        R"({"x":-1})",
        R"({"x":127})",
        R"({"x":128})",
    });
}

TEST(Schema, Sized)
{
    check<Sized>({
        R"({"m":{"a":1},"kv":{"a":1}})",
        R"({"m":{"a":1,"b":2},"kv":{"a":1,"b":2}})",
        R"({"m":{},"kv":{"a":1}})",
        R"({"m":{"a":1},"kv":{}})",
        R"({"m":{"a":1,"b":2,"c":3},"kv":{"a":1}})",
        R"({"m":{"a":1},"kv":{"a":1,"b":2,"c":3}})",
    });
}

TEST(Schema, Fixed)
{
    const auto schema = to_jsoncons(yyjson::json_schema<Fixed>());
    const auto& props = schema["$defs"]["Fixed"]["properties"];
    EXPECT_EQ(props["a"]["minItems"].as<int>(), 3);
    EXPECT_EQ(props["a"]["maxItems"].as<int>(), 3);
    EXPECT_EQ(props["b"]["minItems"].as<int>(), 3);
    EXPECT_EQ(props["b"]["maxItems"].as<int>(), 2);
    EXPECT_EQ(props["none"]["maxItems"].as<int>(), 0);
    EXPECT_FALSE(props["bag"].contains("uniqueItems"));
    check<Fixed>({
        R"({"a":[1,2,3],"b":[1,2],"none":[],"bag":[1,1]})",
        R"({"a":[1,2],"b":[1,2],"none":[],"bag":[]})",
        R"({"a":[1,2,3,4],"b":[1,2],"none":[],"bag":[]})",
        R"({"a":[1,2,3],"b":[1,2],"none":[1],"bag":[]})",
    });
}

TEST(Schema, EmptyTuple)
{
    check<Empty>({
        R"({"t":{}})",
        {R"({"t":[]})", lenient},
        R"({"t":{"a":1}})",
        R"({"t":[1]})",
    });
}

TEST(Schema, TagField)
{
    check<Owner>({
        R"({"pet":{"kind":"cat"}})",
        R"({"pet":{"kind":"cat","lives":3}})",
        R"({"pet":{"lives":3}})",
        R"({"pet":{"kind":"dog"}})",
        R"({"pet":{"kind":"bird"}})",
    });
}

TEST(Schema, Floats)
{
    check<Floats>({
        R"({"tenth":0.1,"positive":1e-45,"below":0.099999994,"large":3.4028235e38,"pick":0.2})",
        R"({"tenth":0.10000001})",
        R"({"positive":0})",
        R"({"positive":-0.0})",
        R"({"below":0.1})",
        R"({"below":-1})",
        R"({"large":-3.4028235e38})",
        R"({"large":3.4028236e38})",
        R"({"pick":0.3})",
        R"({"huge":0})",
        R"({"huge":3.4028236e38})",
    });
}

TEST(Schema, RecursiveTag)
{
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<TTree>())["$ref"].as_string(), "#/$defs/Tree");
    check<TTree>({
        R"({"t":"leaf","v":1})",
        R"({"t":"branch","kids":[{"t":"leaf","v":1},{"t":"branch","kids":[]}]})",
        R"({"t":"branch","kids":[{"t":"leaf","v":"x"}]})",
        R"({"t":"branch","kids":[{"v":1}]})",
        R"({"t":"branch"})",
        R"({"t":"branch","kids":[],"v":1})",
        "null",
    });
}

TEST(Schema, Indirect)
{
    check<IExpr>({
        R"({"op":"+","lhs":{"op":"x","rhs":null,"args":[]},"rhs":{"op":"y","rhs":null,"args":[]},"args":[]})",
        R"({"op":"+","rhs":null,"args":[{"op":"a","rhs":null,"args":[]}]})",
        R"({"op":"+","lhs":null,"rhs":null,"args":[]})",
        R"({"op":"+","rhs":null,"args":[null]})",
        R"({"op":"+","rhs":1,"args":[]})",
        R"({"op":"+"})",
    });
    // the value that an indirect holds has the schema of its type, without null
    const auto s = to_jsoncons(yyjson::json_schema<IExpr>())["$defs"]["IExpr"]["properties"];
    EXPECT_EQ(s["lhs"], json::parse(R"({"$ref":"#/$defs/IExpr"})"));
    EXPECT_EQ(s["args"]["items"], json::parse(R"({"$ref":"#/$defs/IExpr"})"));
    check<ITree>({
        R"({"t":"leaf","v":1})",
        R"({"t":"branch","kids":[{"t":"leaf","v":1},{"t":"branch","kids":[]}]})",
        R"({"t":"branch","kids":[{"v":1}]})",
        R"({"t":"branch","kids":null})",
        "null",
    });
}

TEST(Schema, NestedTag)
{
    check<Nested>({
        R"({"type":"shape","kind":"circle","r":1})",
        R"({"type":"shape","kind":"rect","w":1,"h":2})",
        R"({"type":"shape","kind":"rect","r":1})",
        R"({"type":"shape"})",
        R"({"kind":"circle","r":1})",
        R"({"type":"gauge","level":3})",
        R"({"type":"gauge","level":"x"})",
        R"({"type":"gauge"})",
        R"({"type":"circle","r":2})",
        R"({"type":"circle"})",
        "null",
    });
    check<Twice>({
        R"({"kind":"circle","r":1})",
        R"({"kind":"rect","w":1,"h":2})",
        R"({"r":1})",
    });
}

TEST(Schema, Alternatives)
{
    check<Alternatives>({
        R"({"a":1,"b":-1,"c":[null,0.5],"n":{"x":1}})",
        R"({"a":"abc","b":9223372036854775808,"c":[],"n":[1,2]})",
        R"({"a":-1})",
        R"({"a":"abcd"})",
        R"({"a":1.5})",
        R"({"b":1})",
        R"({"c":[2]})",
        R"({"n":[3]})",
        R"({"n":{"x":"y"}})",
        R"({"a":0,"b":0,"c":[],"n":{"x":0},"d":1})",
        "{}",
    });
    const auto s = to_jsoncons(yyjson::json_schema<Alternatives>());
    const auto& a = s["$defs"]["Alternatives"]["properties"]["a"]["anyOf"];
    ASSERT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0]["description"].as_string(), "a count");
    EXPECT_EQ(a[0]["examples"], json::parse("[1]"));
    EXPECT_EQ(a[1]["maxLength"].as<int>(), 3);
    EXPECT_EQ(s["$defs"]["Alternatives"]["properties"]["b"]["oneOf"].size(), 2u);
    EXPECT_EQ(s["$defs"]["Alternatives"]["properties"]["b"]["default"].as<int>(), -1);
    EXPECT_EQ(s["$defs"]["Alternatives"]["properties"]["n"]["$ref"].as_string(), "#/$defs/Shape");
    EXPECT_FALSE(s["$defs"]["Alternatives"]["properties"]["s"].contains("default"));
    EXPECT_FALSE(s["$defs"]["Alternatives"]["properties"]["t"].contains("default"));
    EXPECT_EQ(s["$defs"]["Shape"]["description"].as_string(), "a shape");
    EXPECT_EQ(s["$defs"]["Shape"]["oneOf"][0], json::parse(R"({"$ref":"#/$defs/AltPoint","title":"point"})"));
}

TEST(Schema, DomFields)
{
    check<DomFields>({
        R"({"names":{"a":1},"list":[1]})",
        R"({"names":{}})",
        R"({"names":{"abc":1}})",
        R"({"list":[1,2]})",
        R"({"list":[]})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<DomFields>());
    const auto& p = s["$defs"]["DomFields"]["properties"];
    EXPECT_EQ(p["names"]["minProperties"].as<int>(), 1);
    EXPECT_FALSE(p["names"].contains("minItems"));
    EXPECT_EQ(p["names"]["propertyNames"]["maxLength"].as<int>(), 2);
    EXPECT_EQ(p["list"]["maxItems"].as<int>(), 1);
    EXPECT_TRUE(p["list"]["uniqueItems"].as<bool>());
}

TEST(Schema, TypedContains)
{
    check<TypedContains>({
        R"({"events":[{"x":1},{"x":2},3],"objects":[{"x":1,"y":2},{}],"numbers":[1,"a"],)"
        R"("kind":"point","points":[{"x":0}],"mixed":["s"]})",
        R"({"events":[{"x":1},3]})",
        R"({"events":[{"x":1,"z":0},{"x":2}]})",
        R"({"objects":[{"x":1,"y":0},{"x":2,"y":0}]})",
        R"({"objects":[{"x":1}]})",
        R"({"numbers":["a"]})",
        R"({"numbers":[]})",
        R"({"kind":"point"})",
        R"({"kind":"point","points":["a"]})",
        R"({"kind":"line","points":["a"]})",
        R"({"mixed":[1]})",
        R"({"mixed":3})",
        R"({"nest":[[1,true],[false]]})",
        R"({"nest":[[1,true],[0]]})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<TypedContains>());
    const auto& p = s["$defs"]["TypedContains"]["properties"];
    EXPECT_EQ(p["events"]["contains"], json::parse(R"({"$ref":"#/$defs/CondPoint"})"));
    EXPECT_EQ(p["objects"]["contains"]["minProperties"].as<int>(), 2);
    EXPECT_EQ(p["objects"]["contains"]["allOf"], json::parse(R"([{"$ref":"#/$defs/CondPoint"}])"));
    EXPECT_EQ(p["numbers"]["contains"]["type"].as_string(), "integer");
    EXPECT_EQ(p["mixed"]["anyOf"][0]["contains"]["type"].as_string(), "string");
    EXPECT_EQ(p["nest"]["items"]["contains"]["type"].as_string(), "boolean");
}

TEST(Schema, ContainsConditions)
{
    check<ContainsConditions>({
        R"({"tags":[]})",
        R"({"tags":["a","deprecated"]})",
        R"({"tags":["a","deprecated"],"reason":"old"})",
        R"({"tags":["deprecated","deprecated"]})",
        R"({"tags":[],"deps":[{"x":1},"a"]})",
        R"({"tags":[],"deps":[{"x":1},"a",{"x":2}]})",
        R"({"tags":[],"deps":[{"x":1},{"x":"b"}]})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<ContainsConditions>());
    const auto& all = s["$defs"]["ContainsConditions"]["allOf"];
    const auto& tags = all[0]["if"]["properties"]["tags"];
    EXPECT_EQ(tags["contains"], json::parse(R"({"const":"deprecated","type":"string"})"));
    EXPECT_EQ(tags["maxContains"].as<int>(), 1);
    const auto& deps = all[1]["not"]["properties"]["deps"];
    EXPECT_EQ(deps["contains"], json::parse(R"({"$ref":"#/$defs/CondPoint"})"));
    EXPECT_EQ(deps["minContains"].as<int>(), 2);
}

TEST(Schema, NegatedValues)
{
    check<NegatedValues>({
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"point":"a"})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"point":{"x":1}})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"point":{"x":"a"}})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"s":"ab"})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"s":"xb"})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"m":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"m":1})",
        R"({"n":1,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"m":0})",
        R"({"n":1,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0,"m":2})",
        R"({"n":3,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":15,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":20,"v":[3,0],"any":0,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[3,2],"any":0,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":null,"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":[null],"f":0,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0.1,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0.2,"mp":{},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{"a":1},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{"a":1,"b":2},"nv":null,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":3,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":4,"u":0})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":7})",
        R"({"n":0,"v":[],"any":0,"f":0,"mp":{},"nv":null,"u":"7"})",
    });
    const auto s = to_jsoncons(yyjson::json_schema<NegatedValues>());
    const auto& p = s["$defs"]["NegatedValues"]["properties"];
    EXPECT_EQ(p["n"]["not"], json::parse(R"({"anyOf":[{"minimum":10,"maximum":19},{"const":3}]})"));
    EXPECT_EQ(p["s"]["not"], json::parse(R"({"pattern":"^x"})"));
    EXPECT_EQ(p["v"]["items"]["not"], json::parse(R"({"enum":[1,2]})"));
    EXPECT_EQ(p["any"]["not"], json::parse(R"({"type":"null"})"));
    EXPECT_EQ(p["point"]["not"], json::parse(R"({"$ref":"#/$defs/CondPoint"})"));
    EXPECT_EQ(p["f"]["not"]["const"].as<double>(), 0.1);
    EXPECT_EQ(p["mp"]["not"], json::parse(R"({"minProperties":2})"));
    EXPECT_EQ(p["nv"]["anyOf"][1]["not"], json::parse(R"({"const":3})"));
    EXPECT_FALSE(p["nv"]["anyOf"][0].contains("not"));
    EXPECT_EQ(p["u"]["anyOf"][0]["not"], json::parse(R"({"const":7})"));
    EXPECT_EQ(s["$defs"]["NegatedValues"]["then"]["properties"]["m"]["not"], json::parse(R"({"const":0})"));
}

TEST(Schema, DefaultLeavingKeyOut)
{
    const auto s = to_jsoncons(yyjson::json_schema<MaybeOuter>());
    const auto& in = s["$defs"]["MaybeOuter"]["properties"]["in"];
    ASSERT_TRUE(in.contains("default"));
    EXPECT_EQ(in["default"], json::parse("{}"));
}

TEST(Schema, WriteOnly)
{
    const auto schema = to_jsoncons(yyjson::json_schema<Borrowed>({.direction = yyjson::schema_direction::write}));
    EXPECT_TRUE(meta_schema().is_valid(schema));
    EXPECT_TRUE(compile(schema).is_valid(json::parse(std::string(yyjson::write(Borrowed{})))));
}

// `yyjson::schema_name` on a type with a caster puts the schema of the caster in `$defs`; a collection splits the name
// while the read and the write schemas differ.
TEST(Schema, NamedCaster)
{
    using enum yyjson::schema_direction;
    for (const auto direction : {read, write})
    {
        const auto schema = to_jsoncons(yyjson::json_schema<NamedCasters>({.direction = direction}));
        const auto& defs = schema["$defs"];
        const auto& props = defs["NamedCasters"]["properties"];
        EXPECT_EQ(props["k"]["$ref"].as_string(), "#/$defs/Kelvin");
        EXPECT_EQ(props["ks"]["items"]["$ref"].as_string(), "#/$defs/Kelvin");
        EXPECT_EQ(props["at"]["$ref"].as_string(), "#/$defs/Epoch");
        EXPECT_EQ(props["h"]["$ref"].as_string(), "#/$defs/Half");
        EXPECT_EQ(defs["Kelvin"]["type"].as_string(), direction == read ? "number" : "string");
        EXPECT_EQ(defs["Kelvin"]["description"].as_string(), "kelvins");
        EXPECT_EQ(defs["Epoch"]["type"].as_string(), "integer");
        EXPECT_EQ(defs["Half"]["type"].as_string(), direction == read ? "integer" : "object");
    }
    check<NamedCasters>({
        R"({"k":1.5,"ks":[2],"at":3,"h":4})",
        R"({"k":"1.5K"})",
        R"({"h":{"v":1}})",
    });

    const auto schemas = to_jsoncons(
        yyjson::schema_collection().add<NamedCasters>().add<NamedCasters>(write).build().schemas());
    std::vector<std::string> names;
    for (const auto& m : schemas.object_range()) names.emplace_back(m.key());
    std::ranges::sort(names);
    EXPECT_EQ(names, (std::vector<std::string>{"Epoch", "Half-Input", "Half-Output", "Kelvin-Input", "Kelvin-Output",
                                               "NamedCasters-Input", "NamedCasters-Output"}));
}

// Two types of the same name, an example that the field does not read and a schema text that is not JSON are
// errors of the declarations.
TEST(Schema, Errors)
{
    EXPECT_THROW((void)yyjson::json_schema<BothSame>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::schema_collection().add<Same1>().add<Same2>().build(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<BadExample>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<BadText>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<UTree>(), std::invalid_argument);
}

// NOLINTEND
