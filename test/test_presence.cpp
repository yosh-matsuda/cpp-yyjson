#include "test_common.hpp"

// NOLINTBEGIN

// The presence of the key of a field: `yyjson::required`, `yyjson::default_` on a field and on a struct, and
// `yyjson::default_from<f>`

struct PreReq
{
    int a = 1;
    int b = 2;
    std::optional<int> c;
    bool operator==(const PreReq&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReq> = yyjson::annotate(yyjson::field("a", yyjson::required));

struct PreReqV
{
    int a = 1;
    int b = 2;
    std::optional<int> c;
    bool operator==(const PreReqV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqV> = yyjson::annotate(yyjson::field("a", yyjson::required));
VISITABLE_STRUCT(PreReqV, a, b, c);

// more than 8 fields, which the reader without a document matches by an index of the keys
struct PreReqWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    int j = 10;
    bool operator==(const PreReqWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqWide> = yyjson::annotate(yyjson::field("j", yyjson::required));

// `yyjson::required("key", ...)` on the struct requires the key as `yyjson::field("member", yyjson::required)` does
struct PreReqKeys
{
    int a = 1;
    int b = 2;
    std::optional<int> c;
    bool operator==(const PreReqKeys&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqKeys> = yyjson::annotate(yyjson::required("a"));
struct PreReqAllOf
{
    int a = 1;
    int b = 2;
    std::optional<int> c;
    bool operator==(const PreReqAllOf&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqAllOf> =
    yyjson::annotate(yyjson::all_of(yyjson::required("a"), yyjson::property("b", yyjson::minimum(0))));
// the key after `yyjson::rename`, and a key in a `std::string_view`
inline constexpr std::string_view pre_req_key = "y";
struct PreReqRenamed
{
    int a = 1;
    int b = 2;
    bool operator==(const PreReqRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqRenamed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("x")), yyjson::field("b", yyjson::rename("y")),
                     yyjson::required("x"), yyjson::required(pre_req_key));
// a variant takes the alternative whose required keys the object has
struct PreReqA
{
    int a = 0;
    std::optional<int> n;
    bool operator==(const PreReqA&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreReqA> = yyjson::annotate(yyjson::required("a"));
struct PreReqB
{
    std::optional<int> b;
    std::optional<int> n;
    bool operator==(const PreReqB&) const = default;
};

struct PreDef
{
    int a = 1;
    int b = 2;
    std::optional<int> c = 3;
    bool operator==(const PreDef&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreDef> = yyjson::annotate(yyjson::field("a", yyjson::default_));

// the default of the struct applies to the fields without their own setting
struct PreStructDef
{
    int a = 1;
    int b = 2;
    std::string s = "s";
    bool operator==(const PreStructDef&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreStructDef> =
    yyjson::annotate(yyjson::default_, yyjson::field("b", yyjson::required));

namespace
{
    std::string make_name() { return "anon"; }
    int make_seven() { return 7; }
}  // namespace

struct PreFrom
{
    std::string s = "init";
    std::optional<std::string> o;
    yyjson::nullable<int> n;
    std::shared_ptr<int> p;
    yyjson::indirect<int> i;
    int plain = 5;
    bool operator==(const PreFrom& r) const
    {
        return s == r.s && o == r.o && n == r.n && (p == nullptr ? r.p == nullptr : r.p != nullptr && *p == *r.p) &&
               *i == *r.i && plain == r.plain;
    }
};
template <>
inline constexpr auto yyjson::annotations<PreFrom> = yyjson::annotate(
    yyjson::field("s", yyjson::default_from<make_name>), yyjson::field("o", yyjson::default_from<make_name>),
    yyjson::field("n", yyjson::default_from<make_seven>), yyjson::field("p", yyjson::default_from<make_seven>),
    yyjson::field("i", yyjson::default_from<make_seven>));

struct PreFromV
{
    std::string s = "init";
    std::optional<std::string> o;
    int plain = 5;
    bool operator==(const PreFromV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreFromV> = yyjson::annotate(
    yyjson::field("s", yyjson::default_from<make_name>), yyjson::field("o", yyjson::default_from<make_name>));
VISITABLE_STRUCT(PreFromV, s, o, plain);

struct PreFromWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<std::string> o;
    bool operator==(const PreFromWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreFromWide> =
    yyjson::annotate(yyjson::field("o", yyjson::default_from<make_name>));

// a field that is not copyable
struct PreFromUnique
{
    std::unique_ptr<int> p;
    std::optional<std::unique_ptr<int>> q;
};
template <>
inline constexpr auto yyjson::annotations<PreFromUnique> = yyjson::annotate(
    yyjson::field("p", yyjson::default_from<make_seven>), yyjson::field("q", yyjson::default_from<make_seven>));

#if FIELD_RFL_ANNOTATIONS
struct PreAnnotated
{
    [[= yyjson::required]] int a = 1;
    [[= yyjson::default_]] int b = 2;
    [[= yyjson::default_from<make_name>]] std::optional<std::string> o;
    int c = 3;
    bool operator==(const PreAnnotated&) const = default;
};
struct PreAnnotatedZip
{
    std::optional<std::string> zip;
};
struct[[= yyjson::required("a")]][[= yyjson::property("b", yyjson::minimum(0))]] PreAnnotatedKeys
{
    int a = 0;
    int b = 0;
    [[= yyjson::required("zip")]] PreAnnotatedZip addr;
};
#endif

// a std::optional field with its own `default_`, and a field that writes its default
struct PreOptDef
{
    std::optional<int> c = 3;
    int d = 4;
    int e = 5;
    bool operator==(const PreOptDef&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreOptDef> =
    yyjson::annotate(yyjson::field("c", yyjson::default_), yyjson::field("d", yyjson::default_, yyjson::emit_defaults),
                     yyjson::field("e", yyjson::default_from<make_seven>));

struct PreZero
{
    double z = 0.0;
    float f = -0.0f;
};
template <>
inline constexpr auto yyjson::annotations<PreZero> = yyjson::annotate(yyjson::default_);

struct PreInner
{
    int x = 1;
    std::string y = "y";
    bool operator==(const PreInner&) const = default;
};

// fields of each kind under the default of the struct
struct PreKinds
{
    std::vector<int> v = {1, 2};
    std::map<std::string, int> m = {{"k", 1}};
    std::unordered_set<int> u = {1, 2, 3};
    std::variant<int, std::string> x = "s";
    char c[4] = "ab";
    std::shared_ptr<int> p;
    yyjson::nullable<int> n = 3;
    yyjson::indirect<PreInner> i;
    PreInner s;
    std::tuple<int, std::optional<int>> t = {1, std::nullopt};
};
template <>
inline constexpr auto yyjson::annotations<PreKinds> = yyjson::annotate(yyjson::default_);

struct PreDefV
{
    int a = 1;
    std::string b = "b";
};
template <>
inline constexpr auto yyjson::annotations<PreDefV> = yyjson::annotate(yyjson::field("a", yyjson::default_));
VISITABLE_STRUCT(PreDefV, a, b);

// more than 8 fields, all of them left out when they equal their defaults
struct PreDefWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::string j = "j";
};
template <>
inline constexpr auto yyjson::annotations<PreDefWide> = yyjson::annotate(yyjson::default_);

// the members of a struct with defaults in another object
struct PreMembers
{
    PreDef def;
    int last = 0;
};
template <>
struct yyjson::caster<PreMembers>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const PreMembers& r, Ts... ts)
    {
        out.emplace_members(r.def, ts...);
        out.emplace("last", r.last);
    }
};

// sentinel defaults: a default of `default_` or `default_from` that violates the keywords of its field
struct PreSentinel
{
    int x = -1;
    int y = 2;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinel> =
    yyjson::annotate(yyjson::field("x", yyjson::default_, yyjson::minimum(0)), yyjson::field("y", yyjson::default_));
struct PreSentinelEmit
{
    int x = -1;
    int y = 2;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelEmit> =
    yyjson::annotate(yyjson::field("x", yyjson::default_, yyjson::emit_defaults, yyjson::minimum(0)),
                     yyjson::field("y", yyjson::default_, yyjson::emit_defaults));
// the struct counts its members, which writes every default but the sentinel
struct PreSentinelCounted
{
    int x = -1;
    int y = 2;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelCounted> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::field("x", yyjson::default_, yyjson::minimum(0)),
                     yyjson::field("y", yyjson::default_));
namespace
{
    int make_negative() { return -5; }
}  // namespace
struct PreSentinelFrom
{
    int x = 3;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelFrom> =
    yyjson::annotate(yyjson::field("x", yyjson::default_from<make_negative>, yyjson::minimum(0)));
struct PreSentinelOpt
{
    std::optional<int> x = -1;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelOpt> =
    yyjson::annotate(yyjson::field("x", yyjson::default_, yyjson::minimum(0)));
// no `default_`: the initializer is written and checked
struct PreInitOpt
{
    std::optional<int> x = -1;
};
template <>
inline constexpr auto yyjson::annotations<PreInitOpt> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
struct PreInitPlain
{
    int x = -1;
};
template <>
inline constexpr auto yyjson::annotations<PreInitPlain> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
// a struct default that violates the keywords of the inner struct, which a merge reads as absent
struct PreSentinelInner
{
    int a = -1;
    std::optional<int> b;
    bool operator==(const PreSentinelInner&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelInner> =
    yyjson::annotate(yyjson::field("a", yyjson::required, yyjson::minimum(0)));
struct PreSentinelOuter
{
    PreSentinelInner in;
    int n = 0;
    bool operator==(const PreSentinelOuter&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelOuter> =
    yyjson::annotate(yyjson::field("in", yyjson::default_, yyjson::emit_defaults));
// a default that the write does not give back without a flag
struct PreSentinelInf
{
    double x = std::numeric_limits<double>::infinity();
    int y = 1;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelInf> = yyjson::annotate(yyjson::default_);
// a valid default whose first test runs near the recursion limit of the walk that meets it
struct PreSentinelNode
{
    int v = 0;
    std::unique_ptr<PreSentinelNode> child;
};
namespace
{
    PreSentinelNode make_leaf() { return PreSentinelNode{1, nullptr}; }
}  // namespace
template <>
inline constexpr auto yyjson::annotations<PreSentinelNode> =
    yyjson::annotate(yyjson::field("child", yyjson::default_from<make_leaf>));
// an array keeps the position of each field, so that it writes a sentinel
struct PreSentinelArray
{
    int a = 1;
    int b = -1;
};
template <>
inline constexpr auto yyjson::annotations<PreSentinelArray> =
    yyjson::annotate(yyjson::as_array, yyjson::field("b", yyjson::default_, yyjson::minimum(0)));

// The rules that the table of the keys of a struct of the members `v` and `w` enforces
namespace presence_rules
{
    template <typename... As>
    consteval auto table_error(const As&... annotations)
    {
        constexpr auto members = std::array<std::string_view, 2>{"v", "w"};
        auto builder = yyjson::detail::field_key_builder<2>{members, {members}, false};
        (builder.apply_annotation(annotations), ...);
        return builder.result().error;
    }
    using enum yyjson::detail::field_key_error;
    using yyjson::field, yyjson::required, yyjson::default_, yyjson::emit_defaults, yyjson::default_from;
    static_assert(table_error(field("v", required), field("w", default_)) == none);
    static_assert(table_error(field("v", required, default_)) == required_with_default);
    static_assert(table_error(field("v", default_from<make_seven>, required)) == required_with_default);
    static_assert(table_error(field("v", required, required)) == required_twice);
    static_assert(table_error(field("v", default_, default_from<make_seven>)) == default_twice);
    static_assert(table_error(field("v", default_, default_)) == default_twice);
    static_assert(table_error(field("v", default_, emit_defaults, emit_defaults)) == emit_defaults_twice);
    static_assert(table_error(field("v", emit_defaults)) == emit_defaults_without_default);
    static_assert(table_error(field("v", required, emit_defaults), default_) == emit_defaults_without_default);
    static_assert(table_error(field("v", emit_defaults), default_) == none);
    static_assert(table_error(field("v", default_from<make_seven>, emit_defaults)) == none);
    static_assert(table_error(default_, default_) == struct_default_twice);
    static_assert(table_error(field("v", yyjson::skip, required)) == skipped_with_setting);
    static_assert(table_error(field("v", required), field("v", yyjson::skip)) == skipped_with_setting);
    static_assert(table_error(field("v", default_), field("w", yyjson::adjacent_tag("t", {"x"}))) == none);
    static_assert(table_error(field("w", default_, yyjson::adjacent_tag("t", {"x"}))) == tagged_presence);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), field("v", required)) ==
                  transparent_presence);
    static_assert(table_error(yyjson::transparent, field("w", yyjson::skip), default_) == transparent_presence);

    // `yyjson::property` and `yyjson::required("key", ...)` on the struct, which name the keys of its fields
    template <typename... As>
    consteval auto type_key_error(const As&... annotations)
    {
        constexpr auto members = std::array<std::string_view, 3>{"v", "w", "t"};
        auto builder = yyjson::detail::field_key_builder<3>{members, {members}, false};
        (builder.apply_annotation(annotations), ...);
        (builder.apply_type_key(annotations), ...);
        return builder.result().error;
    }
    using yyjson::property, yyjson::minimum;
    static_assert(type_key_error(required("v", "w"), required("v"), field("v", required)) == none);
    static_assert(type_key_error(property("v", minimum(0)), field("v", yyjson::maximum(1))) == none);
    static_assert(type_key_error(required("v"), field("v", default_)) == required_with_default);
    static_assert(type_key_error(yyjson::all_of(required("v")), field("v", default_)) == required_with_default);
    static_assert(type_key_error(yyjson::any_of(required("v")), field("v", default_)) == none);
    static_assert(type_key_error(yyjson::not_(required("v")), field("v", default_)) == none);
    // a default of the struct is no default of a field that `required` names
    static_assert(type_key_error(default_, required("v")) == none);
    static_assert(type_key_error(required("x")) == unknown_property_key);
    static_assert(type_key_error(property("x")) == unknown_property_key);
    static_assert(type_key_error(field("v", yyjson::rename("x")), required("x")) == none);
    static_assert(type_key_error(field("v", yyjson::rename("x")), required("v")) == unknown_property_key);
    static_assert(type_key_error(field("v", yyjson::skip), property("v")) == unknown_property_key);
    static_assert(type_key_error(field("v", yyjson::adjacent_tag("k", {"x"})), required("k")) == unknown_property_key);
    static_assert(type_key_error(field("t", yyjson::extra_keys), required("t")) == unknown_property_key);
    static_assert(type_key_error(yyjson::as_array, required("v")) == as_array_object_keyword);
    static_assert(type_key_error(yyjson::as_array, property("v")) == as_array_object_keyword);
}  // namespace presence_rules

namespace
{
    // what reading `json` gives, which is the same through a document, without one, and into a value of T that
    // holds other values
    template <typename T, typename... Ps>
    std::string read_with(std::string_view json, Ps... ps)
    {
        const auto expected = read_or_error([&] { return yyjson::read(json).template cast<T>(ps...); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, ps...); }), expected) << json;
        if constexpr (sizeof...(Ps) == 0)
        {
            // the reader without a document leaves a text that it rejects to the document
            const bool direct = expect_direct_read<T>(json);
            const bool into = expect_read_into(T(), json);
            if (!expected.starts_with("bad cast"))
            {
                EXPECT_TRUE(direct) << json;
                EXPECT_TRUE(into) << json;
            }
        }
        return expected;
    }

    std::string error(std::string_view what) { return std::format("bad cast: {}", what); }
    std::string missing(std::string_view type, std::string_view key)
    {
        return error(std::format("{} is not constructible from JSON without the key \"{}\"", type, key));
    }

    template <typename T>
    void expect_required(std::string_view name)
    {
        EXPECT_EQ(read_with<T>(R"({"a":4,"b":5})"), R"({"a":4,"b":5})");
        EXPECT_EQ(read_with<T>(R"({"b":5,"a":4})"), R"({"a":4,"b":5})");
        // `required` rejects a missing key without a policy, and the other fields keep their initializers
        EXPECT_EQ(read_with<T>(R"({"b":5})"), missing(name, "a"));
        EXPECT_EQ(read_with<T>(R"({"a":4})"), R"({"a":4,"b":2})");
        EXPECT_EQ(read_with<T>(R"({"c":6,"a":4})"), R"({"a":4,"b":2,"c":6})");
        EXPECT_EQ(read_with<T>(R"({})"), missing(name, "a"));
        EXPECT_EQ(read_with<T>(R"({"z":1,"b":5})"), missing(name, "a"));
        // the policies add the fields without a default
        EXPECT_EQ(read_with<T>(R"({"a":4})", yyjson::strict), missing(name, "b"));
        EXPECT_EQ(read_with<T>(R"({"a":4,"b":5})", yyjson::strict), R"({"a":4,"b":5})");
        EXPECT_EQ(read_with<T>(R"({"b":5})", yyjson::reject_unknown_keys), missing(name, "a"));
        // a merge patch may leave out the key
        EXPECT_EQ(expect_merge_into(T{}, R"({"b":5})"), R"({"a":1,"b":5})");
        EXPECT_EQ(expect_merge_into(T{}, R"({})"), R"({"a":1,"b":2})");
    }
}  // namespace

TEST(Presence, Required)
{
    expect_required<PreReq>("PreReq");
    expect_required<PreReqV>("PreReqV");
    const auto all = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":11})");
    EXPECT_EQ(read_with<PreReqWide>(all), all);
    EXPECT_EQ(read_with<PreReqWide>(R"({"j":11,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":11})");
    EXPECT_EQ(read_with<PreReqWide>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})"),
              missing("PreReqWide", "j"));
    EXPECT_EQ(read_with<PreReqWide>(R"({"b":2,"a":1})"), missing("PreReqWide", "j"));
    EXPECT_EQ(expect_merge_into(PreReqWide{}, R"({"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":10})");
    // a vector of them, whose elements are read whole
    EXPECT_EQ(read_with<std::vector<PreReq>>(R"([{"a":1},{"b":2}])"), missing("PreReq", "a") + " at /1");
}

TEST(Presence, RequiredKeys)
{
    static_assert(yyjson::detail::presence_of<PreReqKeys, 0>.required &&
                  !yyjson::detail::presence_of<PreReqKeys, 1>.required);
    static_assert(yyjson::detail::presence_of<PreReqAllOf, 0>.required &&
                  !yyjson::detail::has_struct_conditions<PreReqKeys>);
    expect_required<PreReqKeys>("PreReqKeys");
    expect_required<PreReqAllOf>("PreReqAllOf");
    EXPECT_EQ(read_with<PreReqAllOf>(R"({"a":4,"b":-1})"),
              error(R"(the value of the key "b" of PreReqAllOf is less than the minimum 0)") + " at /b");
    EXPECT_EQ(read_with<PreReqRenamed>(R"({"x":4,"y":5})"), R"({"x":4,"y":5})");
    EXPECT_EQ(read_with<PreReqRenamed>(R"({"y":5})"), missing("PreReqRenamed", "x"));
    EXPECT_EQ(read_with<PreReqRenamed>(R"({"x":4})"), missing("PreReqRenamed", "y"));
    using V = std::variant<PreReqA, PreReqB>;
    EXPECT_EQ(yyjson::read<V>(R"({"a":1,"n":2})").index(), 0);
    EXPECT_EQ(yyjson::read<V>(R"({"b":1,"n":2})").index(), 1);
    EXPECT_EQ(yyjson::read<V>(R"({"n":2})").index(), 1);
}

TEST(Presence, Default)
{
    // a field of `default_` and a std::optional field may lack their keys under strict
    EXPECT_EQ(read_with<PreDef>(R"({"b":5})", yyjson::strict), R"({"a":1,"b":5,"c":3})");
    EXPECT_EQ(read_with<PreDef>(R"({"a":4})", yyjson::strict), missing("PreDef", "b"));
    EXPECT_EQ(read_with<PreDef>(R"({"a":4})"), R"({"a":4,"b":2,"c":3})");
    EXPECT_EQ(read_with<PreDef>(R"({"c":null,"b":5})"), R"({"a":1,"b":5,"c":3})");
    // the default of the struct
    EXPECT_EQ(read_with<PreStructDef>(R"({"b":5})", yyjson::strict), R"({"a":1,"b":5,"s":"s"})");
    EXPECT_EQ(read_with<PreStructDef>(R"({"a":4,"s":"t"})"), missing("PreStructDef", "b"));
    EXPECT_EQ(read_with<PreStructDef>(R"({"s":"t","b":5,"a":4})"), R"({"a":4,"b":5,"s":"t"})");
    // null into a field of `default_` resets it in a merge patch
    auto t = PreDef{4, 5, 6};
    EXPECT_EQ(expect_merge_into(t, R"({"a":null,"c":null})"), R"({"a":1,"b":5,"c":3})");
    EXPECT_EQ(read_or_error([&] {
                  auto u = t;
                  yyjson::merge_into(R"({"a":null})", u, yyjson::strict);
                  return u;
              }),
              R"({"a":1,"b":5,"c":6})");
}

namespace
{
    template <typename T>
    void expect_default_from()
    {
        // a missing key takes the value that f makes, and a key read over it keeps the value read
        EXPECT_EQ(read_with<T>(R"({})"), R"({"s":"anon","o":"anon","plain":5})");
        EXPECT_EQ(read_with<T>(R"({"plain":1})"), R"({"s":"anon","o":"anon","plain":1})");
        EXPECT_EQ(read_with<T>(R"({"s":"a","o":"b"})"), R"({"s":"a","o":"b","plain":5})");
        EXPECT_EQ(read_with<T>(R"({"o":"b","s":"a","z":1})"), R"({"s":"a","o":"b","plain":5})");
        EXPECT_EQ(read_with<T>(R"({"plain":1,"o":"b"})"), R"({"s":"anon","o":"b","plain":1})");
        // null into a std::optional field reads as a missing key, and strict rejects it
        EXPECT_EQ(read_with<T>(R"({"s":"a","o":null})"), R"({"s":"a","o":"anon","plain":5})");
        EXPECT_EQ(read_with<T>(R"({"o":null,"s":"a"})"), R"({"s":"a","o":"anon","plain":5})");
        EXPECT_TRUE(read_with<T>(R"({"o":null,"plain":1})", yyjson::strict)
                        .ends_with(" is not constructible from JSON null under yyjson::reject_null_as_missing at /o"));
        // the keys may be missing under strict
        EXPECT_EQ(read_with<T>(R"({"plain":1})", yyjson::strict), R"({"s":"anon","o":"anon","plain":1})");
        // a merge patch keeps a field whose key it lacks, and null resets the field to the value that f makes
        auto t = T();
        t.s = "a";
        t.o = "b";
        EXPECT_EQ(expect_merge_into(t, R"({"plain":1})"), R"({"s":"a","o":"b","plain":1})");
        EXPECT_EQ(expect_merge_into(t, R"({"s":null,"o":null})"), R"({"s":"anon","o":"anon","plain":5})");
        // T() does not call f
        EXPECT_EQ(written(T()), R"({"s":"init","plain":5})");
    }
}  // namespace

TEST(Presence, DefaultFrom)
{
    expect_default_from<PreFromV>();
    EXPECT_EQ(read_with<PreFrom>(R"({})"), R"({"s":"anon","o":"anon","n":7,"p":7,"i":7,"plain":5})");
    EXPECT_EQ(read_with<PreFrom>(R"({"s":"a","o":"b","n":null,"p":null,"i":1,"plain":1})"),
              R"({"s":"a","o":"b","n":null,"p":null,"i":1,"plain":1})");
    EXPECT_EQ(read_with<PreFrom>(R"({"o":null,"plain":1})"), R"({"s":"anon","o":"anon","n":7,"p":7,"i":7,"plain":1})");
    EXPECT_EQ(read_with<PreFrom>(R"({"i":2,"n":3,"plain":5})", yyjson::strict),
              R"({"s":"anon","o":"anon","n":3,"p":7,"i":2,"plain":5})");
    EXPECT_EQ(written(PreFrom()), R"({"s":"init","n":null,"p":null,"i":0,"plain":5})");
    auto t = yyjson::read<PreFrom>(R"({"n":1,"p":2,"i":3})");
    EXPECT_EQ(expect_merge_into(t, R"({"n":null,"p":null,"i":null})"),
              R"({"s":"anon","o":"anon","n":7,"p":7,"i":7,"plain":5})");
    // more than 8 fields
    EXPECT_EQ(read_with<PreFromWide>(R"({"b":1,"a":2})"),
              R"({"a":2,"b":1,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"o":"anon"})");
    EXPECT_EQ(read_with<PreFromWide>(R"({"o":null,"i":1})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":1,"o":"anon"})");
    EXPECT_EQ(read_with<PreFromWide>(R"({"o":"x"})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"o":"x"})");
    // fields that are not copyable
    const auto u = yyjson::read<PreFromUnique>("{}");
    EXPECT_EQ(*u.p, 7);
    EXPECT_EQ(**u.q, 7);
    // std::unique_ptr reads null, so the std::optional over it holds an empty pointer
    const auto all = [](const PreFromUnique& v) { return std::string(yyjson::write(v, yyjson::emit_defaults)); };
    EXPECT_EQ(all(yyjson::read<PreFromUnique>(R"({"q":null})")), R"({"p":7,"q":null})");
    EXPECT_EQ(all(yyjson::read(R"({"p":1})").cast<PreFromUnique>()), R"({"p":1,"q":7})");
    // a value equal to the value that f makes is left out
    EXPECT_EQ(written(yyjson::read<PreFromUnique>(R"({"q":null})")), R"({"q":null})");
    EXPECT_EQ(written(yyjson::read(R"({"p":1})").cast<PreFromUnique>()), R"({"p":1})");
}

namespace
{
    // what writing `t` gives, which is the same through a document and without one
    template <typename T>
    std::string write_both(const T& t)
    {
        expect_direct_write(t);
        return std::string(yyjson::write(t));
    }

    // what writing `t` with `yyjson::emit_defaults` gives, which all the forms of `yyjson::write` give
    template <typename T>
    std::string write_all(const T& t)
    {
        const auto json = std::string(yyjson::write(t, yyjson::emit_defaults));
        EXPECT_EQ(std::string(yyjson::write(t, yyjson::WriteFlag::NoFlag, yyjson::emit_defaults)), json);
        auto out = std::string("previous");
        yyjson::write(t, out, yyjson::emit_defaults);
        EXPECT_EQ(out, json);
        yyjson::write(t, out, yyjson::WriteFlag::NoFlag, yyjson::emit_defaults);
        EXPECT_EQ(out, json);
        // the call leaves the next write as it was
        EXPECT_EQ(std::string(yyjson::write(t)), write_both(t));
        return json;
    }
}  // namespace

TEST(Presence, WriteDefaults)
{
    // a field equal to its default is left out, and a std::optional field without `default_` only when empty
    EXPECT_EQ(write_both(PreDef()), R"({"b":2,"c":3})");
    EXPECT_EQ(write_both(PreDef{4, 2, std::nullopt}), R"({"a":4,"b":2})");
    EXPECT_EQ(write_all(PreDef()), R"({"a":1,"b":2,"c":3})");
    EXPECT_EQ(write_all(PreDef{1, 2, std::nullopt}), R"({"a":1,"b":2})");
    // `default_` on a std::optional field also leaves out a value equal to its initializer
    EXPECT_EQ(write_both(PreOptDef()), R"({"d":4,"e":5})");
    EXPECT_EQ(write_both(PreOptDef{std::nullopt, 1, 7}), R"({"d":1})");
    EXPECT_EQ(write_both(PreOptDef{6, 4, 8}), R"({"c":6,"d":4,"e":8})");
    EXPECT_EQ(write_all(PreOptDef{std::nullopt, 4, 7}), R"({"d":4,"e":7})");
    EXPECT_EQ(write_all(PreOptDef()), R"({"c":3,"d":4,"e":5})");
    // the default of the struct, and `default_from`
    EXPECT_EQ(write_both(PreStructDef()), R"({"b":2})");
    EXPECT_EQ(write_both(PreStructDef{1, 2, "t"}), R"({"b":2,"s":"t"})");
    EXPECT_EQ(write_both(PreFromV{"anon", "anon", 5}), R"({"plain":5})");
    EXPECT_EQ(write_both(PreFromV{"init", "x", 5}), R"({"s":"init","o":"x","plain":5})");
    EXPECT_EQ(write_all(PreFromV{"anon", "anon", 5}), R"({"s":"anon","o":"anon","plain":5})");
    auto from = yyjson::read<PreFrom>("{}");
    EXPECT_EQ(write_both(from), R"({"plain":5})");
    *from.p = 8;
    EXPECT_EQ(write_both(from), R"({"p":8,"plain":5})");
    // -0.0 equals a default of 0.0, as JSON Schema compares numbers; emit_defaults writes the exact values
    EXPECT_EQ(write_both(PreZero()), R"({})");
    EXPECT_EQ(write_both(PreZero{-0.0, 0.0f}), R"({})");
    EXPECT_EQ(write_both(PreZero{1.0, 0.0f}), R"({"z":1.0})");
    EXPECT_EQ(write_all(PreZero()), R"({"z":0.0,"f":-0.0})");
    EXPECT_EQ(write_all(PreZero{-0.0, 0.0f}), R"({"z":-0.0,"f":0.0})");
    // fields of each kind
    EXPECT_EQ(write_both(PreKinds()), R"({})");
    auto kinds = PreKinds();
    kinds.v = {2, 1};
    kinds.m["k"] = 2;
    kinds.u = {3, 2};
    kinds.x = 1;
    kinds.c[1] = 'c';
    kinds.p = std::make_shared<int>(0);
    kinds.n = std::nullopt;
    kinds.i->x = 2;
    kinds.s.y = "z";
    std::get<1>(kinds.t) = 0;
    EXPECT_EQ(write_both(kinds), R"({"v":[2,1],"m":{"k":2},"u":)" + written(kinds.u) +
                                     R"(,"x":1,"c":[97,99,0,0],"p":0,"n":null,"i":{"x":2,"y":"y"},"s":{"x":1,"y":"z"},)"
                                     R"("t":[1,0]})");
    // the elements of an unordered set equal in any order
    kinds = PreKinds();
    kinds.u = {3, 1, 2};
    EXPECT_EQ(write_both(kinds), R"({})");
    // VISITABLE_STRUCT, more than 8 fields, elements of a vector and of a map, and members of another object
    EXPECT_EQ(write_both(PreDefV()), R"({"b":"b"})");
    EXPECT_EQ(write_both(PreDefV{2, "b"}), R"({"a":2,"b":"b"})");
    EXPECT_EQ(write_both(PreDefWide()), R"({})");
    EXPECT_EQ(write_both(PreDefWide{.i = 1, .j = "k"}), R"({"i":1,"j":"k"})");
    EXPECT_EQ(write_all(PreDefWide()), R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":"j"})");
    EXPECT_EQ(write_both(std::vector<PreDef>{{}, {4, 2, 3}}), R"([{"b":2,"c":3},{"a":4,"b":2,"c":3}])");
    EXPECT_EQ(write_both(std::map<std::string, PreStructDef>{{"x", {}}}), R"({"x":{"b":2}})");
    EXPECT_EQ(write_all(std::vector<PreDef>{{}}), R"([{"a":1,"b":2,"c":3}])");
    EXPECT_EQ(write_both(PreMembers()), R"({"b":2,"c":3,"last":0})");
    EXPECT_EQ(write_all(PreMembers()), R"({"a":1,"b":2,"c":3,"last":0})");
    EXPECT_EQ(std::string(yyjson::object(PreDef()).write()), R"({"b":2,"c":3})");
}

TEST(Presence, Sentinel)
{
    using yyjson::emit_defaults;
    // a field equal to a sentinel is left out under every tag, and a missing key reads as it
    EXPECT_EQ(write_both(PreSentinel()), R"({})");
    EXPECT_EQ(write_all(PreSentinel()), R"({"y":2})");
    EXPECT_EQ(write_all(PreSentinel{0, 2}), R"({"x":0,"y":2})");
    EXPECT_EQ(write_both(PreSentinelEmit()), R"({"y":2})");
    EXPECT_EQ(write_all(PreSentinelEmit()), R"({"y":2})");
    EXPECT_EQ(write_both(PreSentinelCounted()), R"({"y":2})");
    EXPECT_EQ(write_all(PreSentinelCounted()), R"({"y":2})");
    EXPECT_EQ(yyjson::read<PreSentinel>("{}", yyjson::strict).x, -1);
    EXPECT_EQ(yyjson::read<PreSentinelEmit>("{}", yyjson::strict).x, -1);
    EXPECT_EQ(yyjson::read<PreSentinelCounted>(R"({"y":2})", yyjson::strict).x, -1);
    for (const auto& validate :
         {+[] { yyjson::validate(PreSentinel()); }, +[] { yyjson::validate(PreSentinel(), emit_defaults); },
          +[] { yyjson::validate(PreSentinelEmit()); }, +[] { yyjson::validate(PreSentinelCounted(), emit_defaults); }})
        EXPECT_NO_THROW(validate());
    EXPECT_THROW(yyjson::validate(PreSentinel{-2, 2}), yyjson::bad_cast);

    // the value of `default_from`, which is not checked when it is read
    EXPECT_EQ(write_all(PreSentinelFrom()), R"({"x":3})");
    EXPECT_EQ(write_all(PreSentinelFrom{-5}), R"({})");
    EXPECT_EQ(yyjson::read<PreSentinelFrom>("{}", yyjson::strict).x, -5);
    EXPECT_NO_THROW(yyjson::validate(PreSentinelFrom{-5}, emit_defaults));

    // a std::optional with `default_` leaves out the sentinel and an empty value
    EXPECT_EQ(write_all(PreSentinelOpt()), R"({})");
    EXPECT_EQ(write_all(PreSentinelOpt{std::nullopt}), R"({})");
    EXPECT_EQ(write_all(PreSentinelOpt{3}), R"({"x":3})");
    EXPECT_NO_THROW(yyjson::validate(PreSentinelOpt(), emit_defaults));
    // an initializer without `default_` is no sentinel
    EXPECT_EQ(write_all(PreInitOpt()), R"({"x":-1})");
    EXPECT_THROW(yyjson::validate(PreInitOpt()), yyjson::bad_cast);
    EXPECT_EQ(write_all(PreInitPlain()), R"({"x":-1})");
    EXPECT_THROW(yyjson::validate(PreInitPlain()), yyjson::bad_cast);

    // a merge patch into a field that the write leaves out reads the object of the patch alone
    const auto merge = [](const PreSentinelOuter& t, std::string_view patch) {
        return expect_merge_rule<PreSentinelOuter, true>(t, patch);
    };
    EXPECT_EQ(write_all(PreSentinelOuter()), R"({"n":0})");
    EXPECT_EQ(merge(PreSentinelOuter(), R"({"in":{"b":1}})"), missing("PreSentinelInner", "a") + " at /in");
    EXPECT_EQ(merge(PreSentinelOuter(), R"({"in":{"a":1}})"), R"({"in":{"a":1},"n":0})");
    auto held = PreSentinelOuter{{2, std::nullopt}, 0};
    EXPECT_EQ(merge(held, R"({"in":{"b":1}})"), R"({"in":{"a":2,"b":1},"n":0})");

    // the round trip without flags decides, also for a write that allows infinity
    EXPECT_EQ(std::string(yyjson::write(PreSentinelInf(), yyjson::WriteFlag::AllowInfAndNan, emit_defaults)),
              R"({"y":1})");

    // the test starts from the state of a new call
    auto deep = PreSentinelNode{0, std::make_unique<PreSentinelNode>(make_leaf())};
    for (auto i = 0; i < CPPYYJSON_RECURSION_DEPTH_LIMIT - 1; ++i)
        deep = PreSentinelNode{0, std::make_unique<PreSentinelNode>(std::move(deep))};
    EXPECT_THROW(yyjson::validate(deep, emit_defaults), yyjson::bad_cast);
    const auto shallow = PreSentinelNode{0, std::make_unique<PreSentinelNode>(make_leaf())};
    EXPECT_EQ(std::string(yyjson::write(shallow, emit_defaults)), R"({"v":0,"child":{"v":1,"child":null}})");

    EXPECT_EQ(write_all(PreSentinelArray()), R"([1,-1])");
    EXPECT_THROW(yyjson::validate(PreSentinelArray()), yyjson::bad_cast);
}

#if FIELD_RFL_ANNOTATIONS
TEST(Presence, Annotations)
{
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4,"c":5})", yyjson::strict), R"({"a":4,"b":2,"o":"anon","c":5})");
    EXPECT_EQ(read_with<PreAnnotated>(R"({"c":5})"), missing("PreAnnotated", "a"));
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4})", yyjson::strict), missing("PreAnnotated", "c"));
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4,"o":null})"), R"({"a":4,"b":2,"o":"anon","c":3})");
}

TEST(Presence, AnnotatedKeys)
{
    EXPECT_EQ(read_with<PreAnnotatedKeys>(R"({"a":1,"b":2,"addr":{"zip":"z"}})"),
              R"({"a":1,"b":2,"addr":{"zip":"z"}})");
    EXPECT_EQ(read_with<PreAnnotatedKeys>(R"({"b":2,"addr":{"zip":"z"}})"), missing("PreAnnotatedKeys", "a"));
    EXPECT_EQ(read_with<PreAnnotatedKeys>(R"({"a":1,"b":-1,"addr":{"zip":"z"}})"),
              error("the value of the key \"b\" of PreAnnotatedKeys is less than the minimum 0 at /b"));
    EXPECT_EQ(read_with<PreAnnotatedKeys>(R"({"a":1,"b":2,"addr":{}})"),
              error("the value of the key \"addr\" of PreAnnotatedKeys does not have the keys and the values that "
                    "yyjson::property and required give at /addr"));
}
#endif

// NOLINTEND
