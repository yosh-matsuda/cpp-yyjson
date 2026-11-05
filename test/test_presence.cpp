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
#endif

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
    static_assert(table_error(field("v", required, required)) == same_setting_twice);
    static_assert(table_error(field("v", default_, default_from<make_seven>)) == default_twice);
    static_assert(table_error(field("v", default_, default_)) == default_twice);
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
    EXPECT_EQ(written(yyjson::read<PreFromUnique>(R"({"q":null})")), R"({"p":7,"q":null})");
    EXPECT_EQ(written(yyjson::read(R"({"p":1})").cast<PreFromUnique>()), R"({"p":1,"q":7})");
}

#if FIELD_RFL_ANNOTATIONS
TEST(Presence, Annotations)
{
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4,"c":5})", yyjson::strict), R"({"a":4,"b":2,"o":"anon","c":5})");
    EXPECT_EQ(read_with<PreAnnotated>(R"({"c":5})"), missing("PreAnnotated", "a"));
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4})", yyjson::strict), missing("PreAnnotated", "c"));
    EXPECT_EQ(read_with<PreAnnotated>(R"({"a":4,"o":null})"), R"({"a":4,"b":2,"o":"anon","c":3})");
}
#endif

// NOLINTEND
