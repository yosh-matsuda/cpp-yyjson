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

// Floats, compared with the bounds rounded to a float and written in their shortest text.
struct Floats
{
    float tenth = 0;
    float positive = 1;
    float below = 0;
    float large = 3.4028235e38F;
    float pick = 0.1F;
    std::optional<float> none;
};
template <> inline constexpr auto yyjson::annotations<Floats> = yyjson::annotate(
    yyjson::field("tenth", yyjson::maximum(0.1)), yyjson::field("positive", yyjson::exclusive_minimum(0)),
    yyjson::field("below", yyjson::exclusive_maximum(0.1), yyjson::minimum(-1)),
    yyjson::field("pick", yyjson::enum_(0.1, 0.2)), yyjson::field("none", yyjson::minimum(1e300)));

// A recursive variant tagged by `internal_tag`, whose alternatives are written in place: it needs a name, which puts
// it in `$defs`.
struct TLeaf { int v = 0; };
struct TBranch;
using TTree = std::variant<TLeaf, std::shared_ptr<TBranch>>;
struct TBranch { std::vector<TTree> kids; };
template <> inline constexpr auto yyjson::annotations<TTree> =
    yyjson::annotate(yyjson::internal_tag("t", {"leaf", "branch"}), yyjson::schema_name("Tree"));
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
        {R"({"name":"a","address":null})", lenient_read},
        R"({"name":"a","address":{"city":"k"}})",
        {R"({"name":"a","address":{"zip":null}})", lenient_read},
        {R"({"name":"a","opt":null})", lenient_read},
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
        {R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"b":null})", lenient_read},
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"bb":null})",
        R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"x":null})",
        {R"({"a":1,"s":[],"m":{},"t":{"kind":"circle","r":1},"kv":{},"u":null})", lenient_read},
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
        R"({"none":0})",
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
