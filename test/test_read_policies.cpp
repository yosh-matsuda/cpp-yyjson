#include "test_common.hpp"

// NOLINTBEGIN

// The read policies `yyjson::reject_unknown_keys`, `reject_missing_keys`, `reject_null_as_missing`,
// `reject_duplicates` and `yyjson::strict`, given to a read or declared by a struct

struct PolPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolPt&) const = default;
};
struct PolPtV
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolPtV&) const = default;
};
VISITABLE_STRUCT(PolPtV, x, y);

// more than 8 fields, which the reader without a document matches by an index of the keys
struct PolWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const PolWide&) const = default;
};
struct PolWideV
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const PolWideV&) const = default;
};
VISITABLE_STRUCT(PolWideV, a, b, c, d, e, f, g, h, i, j);

struct PolOuter
{
    PolPt p;
    std::vector<PolPt> v;
    std::map<std::string, PolPt> m;
    bool operator==(const PolOuter&) const = default;
};

// policies declared by the struct, which do not apply to the types of its fields
struct PolStrictPt
{
    int x = 0;
    std::optional<int> y;
    PolPt inner;
    bool operator==(const PolStrictPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolStrictPt> = yyjson::annotate(yyjson::strict);
struct PolClosedPt
{
    int x = 0;
    PolPt inner;
    bool operator==(const PolClosedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolClosedPt> =
    yyjson::annotate(yyjson::reject_unknown_keys, yyjson::reject_duplicates, yyjson::reject_duplicates);
struct PolClosedV
{
    int x = 0;
    bool operator==(const PolClosedV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolClosedV> = yyjson::annotate(yyjson::strict, yyjson::reject_missing_keys,
                                                                      yyjson::reject_unknown_keys);
VISITABLE_STRUCT(PolClosedV, x);
struct PolHoldsStrict
{
    PolStrictPt s;
    PolPt p;
    bool operator==(const PolHoldsStrict&) const = default;
};

// the optional fields that read null as a missing key and those that do not
struct PolNulls
{
    std::optional<double> plain;
    std::optional<std::shared_ptr<int>> ptr;
    std::optional<std::variant<std::monostate, int>> var;
    std::optional<std::optional<int>> opt;
    bool operator==(const PolNulls&) const = default;
};

// fields that may not be left out although they read null
struct PolNotOptional
{
    std::shared_ptr<int> ptr;
    std::variant<std::monostate, int> var;
    yyjson::indirect<int> ind;
    bool operator==(const PolNotOptional&) const = default;
};

struct PolSkipped
{
    int a = 0;
    int s = 0;
    bool operator==(const PolSkipped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolSkipped> = yyjson::annotate(yyjson::field("s", yyjson::skip));

struct PolMaybeId
{
    std::optional<int> v;
    bool operator==(const PolMaybeId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolMaybeId> = yyjson::annotate(yyjson::transparent);
struct PolId
{
    int v = 0;
    bool operator==(const PolId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolId> = yyjson::annotate(yyjson::transparent);
struct PolHoldsTransparent
{
    PolMaybeId maybe;
    PolId id;
    bool operator==(const PolHoldsTransparent&) const = default;
};
struct PolHoldsIndirect
{
    yyjson::indirect<PolPt> p;
    std::optional<yyjson::indirect<PolPt>> q;
};

// variants tagged by a member of the object of the alternative and by a member of the object of the struct
struct PolCircle
{
    int r = 0;
    bool operator==(const PolCircle&) const = default;
};
struct PolRect
{
    int w = 0;
    int h = 0;
    bool operator==(const PolRect&) const = default;
};
using PolShape = std::variant<PolCircle, PolRect>;
template <>
inline constexpr auto yyjson::annotations<PolShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct PolEnvelope
{
    int id = 0;
    std::variant<PolCircle, PolRect> data;
    bool operator==(const PolEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolEnvelope> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
struct PolEnvelopeV
{
    int id = 0;
    std::variant<PolCircle, PolRect> data;
    bool operator==(const PolEnvelopeV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolEnvelopeV> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
VISITABLE_STRUCT(PolEnvelopeV, id, data);

// a variant without a tag, whose last step reads an object that no candidate reads as the first alternative
struct PolOk
{
    int code = 0;
    bool operator==(const PolOk&) const = default;
};
struct PolErr
{
    int code = 0;
    std::string msg;
    bool operator==(const PolErr&) const = default;
};
using PolResult = std::variant<PolOk, PolErr>;

// casters whose casts read under the policies of the read that calls them
#if YYJSON_VERSION_HEX >= 0x000C00
using PolView = yyjson::json_view;
#else
// json_view needs yyjson 0.12, and a value of a document takes its place
using PolView = yyjson::reader::const_value_ref;
#endif
struct PolViewBox
{
    PolPt p;
    bool operator==(const PolViewBox&) const = default;
};
template <>
struct yyjson::caster<PolViewBox>
{
    static PolViewBox from_json(PolView v) { return {v.cast<PolPt>()}; }
};
struct PolClosedViewBox
{
    PolPt p;
    bool operator==(const PolClosedViewBox&) const = default;
};
template <>
struct yyjson::caster<PolClosedViewBox>
{
    static PolClosedViewBox from_json(PolView v) { return {v.cast<PolPt>(yyjson::reject_unknown_keys)}; }
};
struct PolDomBox
{
    PolPt p;
    bool operator==(const PolDomBox&) const = default;
};
template <>
struct yyjson::caster<PolDomBox>
{
    template <typename Json>
    static PolDomBox from_json(const Json& json)
    {
        return {yyjson::cast<PolPt>(json)};
    }
};
struct PolBoxes
{
    PolViewBox view;
    PolDomBox dom;
    bool operator==(const PolBoxes&) const = default;
};

#if FIELD_RFL_ANNOTATIONS
struct [[= yyjson::strict]] PolAnnotatedPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolAnnotatedPt&) const = default;
};
struct [[= yyjson::reject_unknown_keys]] PolAnnotatedClosed
{
    int x = 0;
    bool operator==(const PolAnnotatedClosed&) const = default;
};
#endif

// closed types, which `yyjson::additional_properties(false)` makes reject the keys of no field or tag
struct ClosedPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const ClosedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedPt> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedPtV
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const ClosedPtV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedPtV> = yyjson::annotate(yyjson::additional_properties(false));
VISITABLE_STRUCT(ClosedPtV, x, y);
struct ClosedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const ClosedWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedWide> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedRenamed
{
    int a = 0;
    int s = 0;
    bool operator==(const ClosedRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedRenamed> = yyjson::annotate(
    yyjson::field("a", yyjson::rename("A")), yyjson::field("s", yyjson::skip), yyjson::additional_properties(false));
struct ClosedHolder
{
    ClosedPt p;
    std::vector<ClosedPt> v;
    bool operator==(const ClosedHolder&) const = default;
};
struct ClosedCircle
{
    int r = 0;
    bool operator==(const ClosedCircle&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedCircle> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedRect
{
    int w = 0;
    int h = 0;
    bool operator==(const ClosedRect&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedRect> = yyjson::annotate(yyjson::additional_properties(false));
using ClosedShape = std::variant<ClosedCircle, ClosedRect>;
template <>
inline constexpr auto yyjson::annotations<ClosedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct ClosedEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const ClosedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})), yyjson::additional_properties(false));

// types that bound the number of the members of their objects
struct CountedPt
{
    int a = 0;
    int b = 0;
    std::optional<int> c;
    bool operator==(const CountedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedPt> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
struct CountedPtV
{
    int a = 0;
    int b = 0;
    std::optional<int> c;
    bool operator==(const CountedPtV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedPtV> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
VISITABLE_STRUCT(CountedPtV, a, b, c);
struct CountedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const CountedWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedWide> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
struct CountedHolder
{
    CountedPt p;
    std::vector<CountedPt> v;
    bool operator==(const CountedHolder&) const = default;
};
struct CountedRect
{
    int w = 0;
    int h = 0;
    bool operator==(const CountedRect&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedRect> = yyjson::annotate(yyjson::min_properties(3));
using CountedShape = std::variant<ClosedCircle, CountedRect>;
template <>
inline constexpr auto yyjson::annotations<CountedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct CountedEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const CountedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})), yyjson::max_properties(3));
struct CountedClosed
{
    int x = 0;
    bool operator==(const CountedClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedClosed> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(1));
// the same fields, with and without `yyjson::min_properties`
struct CountedDefaults
{
    int a = 0;
    int b = 5;
    std::optional<int> c;
    bool operator==(const CountedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::min_properties(0));
struct UncountedDefaults
{
    int a = 0;
    int b = 5;
    std::optional<int> c;
    bool operator==(const UncountedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UncountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::max_properties(3));

using OptionalMap = std::map<std::string, std::optional<int>>;
using IntMap = std::map<std::string, int>;
using IntHashMap = std::unordered_map<std::string, int>;
using IntMultimap = std::multimap<std::string, int>;
using IntHashMultimap = std::unordered_multimap<std::string, int>;

namespace
{
    // whether the reader without a document read the last text that `read_with` or `merge_with` read
    bool direct_read = false;

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

    // what every read of `json` under the policies gives, which is the same through a document and without one
    template <typename T, typename... Ps>
    std::string read_with(std::string_view json, Ps... ps)
    {
        const auto expected = read_or_error([&] { return yyjson::read<T>(json, ps...); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, yyjson::ReadFlag::NoFlag, ps...); }), expected)
            << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::read(json).cast<T>(ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::value(yyjson::read(json)).cast<T>(ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::cast<T>(yyjson::read(json), ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] {
                      auto t = T();
                      yyjson::read_into(json, t, ps...);
                      return t;
                  }),
                  expected)
            << json;
        EXPECT_EQ(read_or_error([&] {
                      auto t = T();
                      yyjson::read_into(json, t, yyjson::ReadFlag::NoFlag, ps...);
                      return t;
                  }),
                  expected)
            << json;
        direct_read = !reads_directly;
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto scope = yyjson::detail::read_policy_scope(yyjson::detail::policies_of<Ps...>);
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        direct_read = direct.has_value();
        if (direct)
        {
            EXPECT_EQ(*direct, expected) << json;
        }
        const auto into = read_root_directly(json, yyjson::ReadFlag::NoFlag, [&](auto& reader) {
            auto t = T();
            reader.read_root_into(t);
            return read_or_error([&] { return t; });
        });
        EXPECT_EQ(into.has_value(), direct.has_value()) << json;
        if (into)
        {
            EXPECT_EQ(*into, expected) << json;
        }
#endif
        return expected;
    }

    // what merging `json` into `t` under the policies gives, which is the same through a document and without one
    template <typename T, typename... Ps>
    std::string merge_with(const T& t, std::string_view json, Ps... ps)
    {
        const auto expected = read_or_error([&] {
            auto u = t;
            yyjson::merge_into(json, u, ps...);
            return u;
        });
        EXPECT_EQ(read_or_error([&] {
                      auto u = t;
                      yyjson::merge_into(json, u, yyjson::ReadFlag::NoFlag, ps...);
                      return u;
                  }),
                  expected)
            << json;
        const auto scope = yyjson::detail::read_policy_scope(yyjson::detail::policies_of<Ps...>);
        EXPECT_EQ(read_or_error([&] {
                      auto u = t;
                      yyjson::detail::merge_text(json, u, yyjson::ReadFlag::NoFlag);
                      return u;
                  }),
                  expected)
            << json;
        direct_read = !reads_directly;
#if YYJSON_VERSION_HEX >= 0x000C00
        direct_read = false;
        auto u = t;
        try
        {
            yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, yyjson::ReadFlag::NoFlag)
                .merge_root(u);
            direct_read = true;
        }
        catch (...)
        {
        }
        if (direct_read)
        {
            EXPECT_EQ(read_or_error([&] { return u; }), expected) << json;
        }
#endif
        return expected;
    }

    // the path of the error of reading `json` under the policies, which is the same through a document
    template <typename T, typename... Ps>
    std::string error_path(std::string_view json, Ps... ps)
    {
        const auto e = bad_cast_of([&] { (void)yyjson::read<T>(json, ps...); });
        const auto d = bad_cast_of([&] { (void)yyjson::read(json).cast<T>(ps...); });
        EXPECT_EQ(e.path(), d.path()) << json;
        EXPECT_TRUE(e.position().has_value()) << json;
        return std::string(e.path());
    }

    std::string error(std::string_view what) { return std::format("bad cast: {}", what); }
    std::string unknown(std::string_view type, std::string_view key)
    {
        return error(std::format("{} has no field of the key \"{}\"", type, key));
    }
    std::string repeated(std::string_view type, std::string_view key)
    {
        return error(std::format("the object read into {} has the key \"{}\" twice", type, key));
    }
    std::string missing(std::string_view type, std::string_view key)
    {
        return error(std::format("{} is not constructible from JSON without the key \"{}\"", type, key));
    }
    std::string closed(std::string_view type, std::string_view key)
    {
        return error(
            std::format("{} has the key \"{}\", which yyjson::additional_properties(false) rejects", type, key));
    }
    std::string fewer(std::string_view type, int n)
    {
        return error(std::format("{} has fewer properties than the minimum {}", type, n));
    }
    std::string more(std::string_view type, int n)
    {
        return error(std::format("{} has more properties than the maximum {}", type, n));
    }
    std::string null_as_missing(std::string_view type)
    {
        return error(std::format("{} is not constructible from JSON null under yyjson::reject_null_as_missing", type));
    }

    // the cases of the objects of `{"x":..,"y":..}` for a struct of <= 8 fields, both kinds of struct
    template <typename T>
    void expect_small(std::string_view name)
    {
        const auto at = [](std::string_view s, std::string_view path) { return std::format("{} at {}", s, path); };
        // keys in order and out of order
        for (const auto json : {R"({"x":1,"y":2})", R"({"y":2,"x":1})"})
        {
            EXPECT_EQ(read_with<T>(json), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys, yyjson::strict), R"({"x":1,"y":2})");
            EXPECT_TRUE(direct_read) << json;
        }
        // an unknown key before, among and after the fields
        for (const auto json : {R"({"z":0,"x":1,"y":2})", R"({"x":1,"z":0,"y":2})", R"({"x":1,"y":2,"z":0})"})
        {
            EXPECT_EQ(read_with<T>(json), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::strict), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), at(unknown(name, "z"), "/z"));
            EXPECT_EQ(error_path<T>(json, yyjson::reject_unknown_keys), "/z");
        }
        // a key twice, inside the keys in order and after them
        for (const auto& [json, key] : std::vector<std::pair<std::string_view, std::string_view>>{
                 {R"({"x":1,"x":3,"y":2})", "x"}, {R"({"x":1,"y":2,"x":3})", "x"}, {R"({"y":2,"x":1,"y":2})", "y"}})
        {
            const auto plain = read_with<T>(json);
            EXPECT_TRUE(plain.starts_with("{")) << json;
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys, yyjson::reject_missing_keys), plain);
            EXPECT_EQ(read_with<T>(json, yyjson::reject_duplicates), at(repeated(name, key), std::format("/{}", key)));
            EXPECT_EQ(read_with<T>(json, yyjson::strict), read_with<T>(json, yyjson::reject_duplicates));
        }
        // a field left out, which may be left out only when it is optional
        EXPECT_EQ(read_with<T>(R"({"y":2})"), R"({"x":0,"y":2})");
        EXPECT_EQ(read_with<T>(R"({"y":2})", yyjson::reject_missing_keys), missing(name, "x"));
        EXPECT_EQ(error_path<T>(R"({"y":2})", yyjson::strict), "");
        EXPECT_EQ(read_with<T>(R"({"x":1})", yyjson::strict), R"({"x":1})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({})"), R"({"x":0})");
        EXPECT_EQ(read_with<T>(R"({})", yyjson::strict), missing(name, "x"));
        EXPECT_EQ(read_with<T>(R"({})", yyjson::reject_unknown_keys, yyjson::reject_duplicates), R"({"x":0})");
        // null as the value of an optional field that reads it as a missing key
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})"), R"({"x":1})");
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})", yyjson::reject_missing_keys), R"({"x":1})");
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})", yyjson::reject_null_as_missing),
                  at(null_as_missing("std::optional<int>"), "/y"));
        EXPECT_EQ(read_with<T>(R"({"y":null,"x":1})", yyjson::strict), at(null_as_missing("std::optional<int>"), "/y"));
        EXPECT_EQ(error_path<T>(R"({"x":1,"y":null})", yyjson::strict), "/y");
    }

    // the same for a struct of 10 fields
    template <typename T>
    void expect_wide(std::string_view name)
    {
        const auto all = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})");
        const auto plain = read_with<T>(all);
        EXPECT_EQ(plain, all);
        EXPECT_EQ(read_with<T>(all, yyjson::strict, yyjson::reject_unknown_keys), all);
        EXPECT_TRUE(direct_read);
        const auto shuffled = std::string(R"({"j":10,"i":9,"a":1,"c":3,"b":2,"e":5,"d":4,"g":7,"f":6,"h":8})");
        EXPECT_EQ(read_with<T>(shuffled, yyjson::strict, yyjson::reject_unknown_keys), all);
        EXPECT_TRUE(direct_read);
        // a key twice, inside the keys in order and after them
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})"), all);
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})",
                               yyjson::reject_duplicates),
                  repeated(name, "b") + " at /b");
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})"), all);
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})", yyjson::strict),
                  repeated(name, "a") + " at /a");
        EXPECT_EQ(read_with<T>(R"({"j":1,"i":9,"j":10,"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})",
                               yyjson::strict),
                  repeated(name, "j") + " at /j");
        // a key left out in the middle and at the end
        const auto no_e = std::string(R"({"a":1,"b":2,"c":3,"d":4,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(no_e), R"({"a":1,"b":2,"c":3,"d":4,"e":0,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(no_e, yyjson::strict), missing(name, "e"));
        const auto no_i = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})");
        EXPECT_EQ(read_with<T>(no_i, yyjson::reject_missing_keys), missing(name, "i"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})", yyjson::strict),
                  R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})");
        EXPECT_TRUE(direct_read);
        // an unknown key
        const auto with_z = std::string(R"({"a":1,"b":2,"c":3,"d":4,"z":0,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(with_z, yyjson::strict), all);
        EXPECT_EQ(read_with<T>(with_z, yyjson::reject_unknown_keys), unknown(name, "z") + " at /z");
        // null as an optional field
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 3) + "null}", yyjson::reject_null_as_missing),
                  null_as_missing("std::optional<int>") + " at /j");
    }
}  // namespace

TEST(ReadPolicies, SmallStructs)
{
    expect_small<PolPt>("PolPt");
    expect_small<PolPtV>("PolPtV");
}

TEST(ReadPolicies, WideStructs)
{
    expect_wide<PolWide>("PolWide");
    expect_wide<PolWideV>("PolWideV");
}

TEST(ReadPolicies, Nested)
{
    // a policy of the read applies to every struct in it
    const auto json = R"({"p":{"y":1},"v":[{"x":1},{"y":2}],"m":{"k":{"x":3}}})";
    EXPECT_EQ(read_with<PolOuter>(json), R"({"p":{"x":0,"y":1},"v":[{"x":1},{"x":0,"y":2}],"m":{"k":{"x":3}}})");
    EXPECT_EQ(read_with<PolOuter>(json, yyjson::reject_missing_keys), missing("PolPt", "x") + " at /p");
    EXPECT_EQ(error_path<PolOuter>(json, yyjson::reject_missing_keys), "/p");
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[{"x":1},{"y":2}],"m":{}})", yyjson::strict),
              missing("PolPt", "x") + " at /v/1");
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1,"z":1}}})", yyjson::reject_unknown_keys),
              unknown("PolPt", "z") + " at /m/k/z");
    EXPECT_TRUE(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1},"k":{"x":2}}})", yyjson::strict)
                    .ends_with(R"(has the key "k" twice at /m/k)"));
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::strict),
              R"({"p":{"x":1},"v":[],"m":{}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::reject_unknown_keys),
              unknown("PolOuter", "q") + " at /q");
}

TEST(ReadPolicies, TypeLevel)
{
    // the struct rejects for itself, but not for the types of its fields
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})"), R"({"x":1,"inner":{"x":0}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolStrictPt>(R"({"inner":{}})"), missing("PolStrictPt", "x"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"x":1,"inner":{}})"), repeated("PolStrictPt", "x") + " at /x");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"y":null,"inner":{}})"), null_as_missing("std::optional<int>") + " at /y");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"y":1,"y":2,"w":1}})"),
              R"({"x":1,"inner":{"x":0,"y":2}})");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})", yyjson::reject_missing_keys),
              missing("PolPt", "x") + " at /inner");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"x":1}})", yyjson::reject_unknown_keys),
              unknown("PolStrictPt", "z") + " at /z");
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"x":1,"inner":{}}})"), R"({"s":{"x":1,"inner":{"x":0}},"p":{"x":0}})");
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"inner":{}},"p":{}})"), missing("PolStrictPt", "x") + " at /s");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"inner":{"x":1,"w":1}})"), R"({"x":1,"inner":{"x":1}})");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"w":1,"inner":{}})"), unknown("PolClosedPt", "w") + " at /w");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"inner":{}})"), R"({"x":0,"inner":{"x":0}})");
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolClosedV>(R"({})"), missing("PolClosedV", "x"));
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1,"y":1})"), unknown("PolClosedV", "y") + " at /y");
    EXPECT_EQ(read_with<std::vector<PolClosedV>>(R"([{"x":1},{"x":1,"x":2}])"),
              repeated("PolClosedV", "x") + " at /1/x");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"y":1})"), missing("PolAnnotatedPt", "x"));
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({"x":1,"y":1})"), unknown("PolAnnotatedClosed", "y") + " at /y");
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({})"), R"({"x":0})");
#endif
}

TEST(ReadPolicies, Null)
{
    const auto json = R"({"plain":null,"ptr":null,"var":null,"opt":null})";
    EXPECT_EQ(read_with<PolNulls>(json), R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_EQ(read_with<PolNulls>(json, yyjson::reject_null_as_missing),
              null_as_missing("std::optional<double>") + " at /plain");
    // only an optional whose value does not read null takes null as a missing key
    EXPECT_EQ(read_with<PolNulls>(R"({"ptr":null,"var":null,"opt":null})", yyjson::strict),
              R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_TRUE(direct_read);
    // elements, values of a map and the root read null as `std::nullopt`
    EXPECT_EQ(read_with<std::vector<std::optional<int>>>("[null,1]", yyjson::strict), "[null,1]");
    EXPECT_EQ(read_with<OptionalMap>(R"({"a":null})", yyjson::strict),
              R"({"a":null})");
    EXPECT_EQ(read_with<std::optional<int>>("null", yyjson::strict), "null");
    EXPECT_EQ(read_with<std::optional<PolPt>>("null", yyjson::strict), "null");
    // a transparent struct over such an optional follows the rule of the field
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})"), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})", yyjson::reject_null_as_missing),
              null_as_missing("std::optional<int>") + " at /maybe");
}

TEST(ReadPolicies, Missing)
{
    // a field that reads null is not optional, so its key may not be left out
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null,"ind":1})", yyjson::strict),
              R"({"ptr":null,"var":null,"ind":1})");
    EXPECT_EQ(read_with<PolNotOptional>(R"({"var":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ptr"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "var"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ind"));
    // a transparent struct may be left out when its field may
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"id":1})", yyjson::strict), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2})", yyjson::strict), missing("PolHoldsTransparent", "id"));
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2,"id":1,"id":1})", yyjson::strict),
              repeated("PolHoldsTransparent", "id") + " at /id");
    // a field of `yyjson::indirect`
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1}})", yyjson::strict).p->x; }),
              "1");
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"q":{"x":1}})", yyjson::strict).p->x; }),
              missing("PolHoldsIndirect", "p"));
    EXPECT_EQ(
        read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1},"q":{}})", yyjson::strict).p->x; }),
        missing("PolPt", "x") + " at /q");
    // a skipped field is not read, so its key is unknown and never missing
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::reject_unknown_keys), unknown("PolSkipped", "s") + " at /s");
}

TEST(ReadPolicies, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolShape>(R"({"w":1,"type":"rect","h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"r":1})", yyjson::reject_unknown_keys),
              unknown("PolRect", "r") + " at /r");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1})", yyjson::reject_missing_keys),
              missing("PolRect", "h"));
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})", yyjson::reject_duplicates),
              repeated("PolRect", "type") + " at /type");
    EXPECT_EQ(read_with<PolCircle>(R"({"type":"circle","r":1})", yyjson::reject_unknown_keys),
              unknown("PolCircle", "type") + " at /type");

    // the keys of an adjacent tag are keys of the struct that holds the variant
    const auto expect_adjacent = [](auto t, std::string_view name) {
        using T = decltype(t);
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"data":{"r":3},"kind":"circle","id":1})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"circle","data":{"r":3}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})", yyjson::reject_unknown_keys),
                  unknown(name, "z") + " at /z");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})"),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})",
                               yyjson::reject_duplicates),
                  repeated(name, "kind") + " at /kind");
        // the value of the variant left out with or without its tag
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})", yyjson::reject_missing_keys), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"id":1})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1})", yyjson::strict), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"kind":"circle","data":{"r":3}})", yyjson::strict), missing(name, "id"));
    };
    expect_adjacent(PolEnvelope(), "PolEnvelope");
    expect_adjacent(PolEnvelopeV(), "PolEnvelopeV");
}

TEST(ReadPolicies, Containers)
{
    // a key twice in a map
    EXPECT_EQ(read_with<IntMap>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    EXPECT_TRUE(direct_read);
    const auto map_text = read_with<IntMap>(R"({"a":1,"b":2,"a":3})", yyjson::reject_duplicates);
    EXPECT_TRUE(map_text.starts_with("bad cast: the object read into std::map<")) << map_text;
    EXPECT_TRUE(map_text.ends_with(R"(has the key "a" twice at /a)")) << map_text;
    EXPECT_EQ(read_with<IntHashMap>(R"({"a":1,"a":3})", yyjson::strict).substr(0, 37),
              "bad cast: the object read into std::u");
    EXPECT_EQ(read_with<IntMultimap>(R"({"a":1,"a":3})"), R"({"a":1,"a":3})");
    EXPECT_TRUE(read_with<IntMultimap>(R"({"a":1,"a":3})", yyjson::strict)
                    .ends_with(R"(has the key "a" twice at /a)"));
    EXPECT_TRUE(read_with<IntHashMultimap>(R"({"a":1,"b":2,"a":3})", yyjson::strict)
                    .ends_with(R"(has the key "a" twice at /a)"));
    // a map takes every key
    EXPECT_EQ(read_with<IntMap>(R"({"a":1})", yyjson::reject_unknown_keys), R"({"a":1})");

    // an element twice in a set, which would drop it
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]"), "[1,2]");
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]", yyjson::strict),
              error("the array read into std::set<int> has an element twice at /2"));
    EXPECT_EQ(read_with<std::set<int>>("[1,2,3]", yyjson::strict), "[1,2,3]");
    EXPECT_TRUE(direct_read);
    EXPECT_TRUE(read_with<std::unordered_set<int>>("[1,1]", yyjson::reject_duplicates)
                    .ends_with("has an element twice at /1"));
    EXPECT_EQ(read_with<std::multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::unordered_multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<std::set<int>>>("[[1],[2,2]]", yyjson::strict),
              error("the array read into std::set<int> has an element twice at /1/1"));
}

TEST(ReadPolicies, UntaggedVariant)
{
    // no candidate reads `{"msg":"x"}`, which the plain read gives the first alternative
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})"), R"({"code":0})");
    const auto rejected = read_with<PolResult>(R"({"msg":"x"})", yyjson::reject_missing_keys);
    EXPECT_TRUE(rejected.starts_with("bad cast: ")) << rejected;
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})", yyjson::strict), rejected);
    EXPECT_EQ(read_with<PolResult>(R"({"code":1})", yyjson::strict), R"({"code":1})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::strict), R"({"code":1,"msg":"x"})");
    // a candidate that fails under a policy gives the object to the next
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::reject_unknown_keys),
              R"({"code":1,"msg":"x"})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x","code":2})", yyjson::reject_duplicates).substr(0, 10),
              "bad cast: ");
}

TEST(ReadPolicies, Casters)
{
    // the casts of a caster read under the policies of the read that calls it
    const auto json = R"({"view":{"y":1},"dom":{"y":2}})";
    EXPECT_EQ(read_with<PolBoxes>(json), R"({"view":{"p":{"x":0,"y":1}},"dom":{"p":{"x":0,"y":2}}})");
    EXPECT_EQ(read_with<PolBoxes>(json, yyjson::reject_missing_keys), missing("PolPt", "x") + " at /view");
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"y":2}})", yyjson::reject_missing_keys),
              missing("PolPt", "x") + " at /dom");
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"x":0,"x":1}})", yyjson::strict),
              repeated("PolPt", "x") + " at /dom/x");
    // a cast in a caster adds policies, which end with it
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"x":1,"z":1})"), unknown("PolPt", "z") + " at /z");
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})"), R"({"p":{"x":0,"y":1}})");
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})", yyjson::reject_missing_keys), missing("PolPt", "x"));
    EXPECT_EQ(read_with<std::vector<PolClosedViewBox>>(R"([{"x":1},{"x":1,"z":1}])", yyjson::reject_missing_keys),
              unknown("PolPt", "z") + " at /1/z");
    // the policies end with the read
    EXPECT_EQ(read_with<PolPt>(R"({"y":1})"), R"({"x":0,"y":1})");
}

TEST(ReadPolicies, MergeInto)
{
    const auto t = PolPt{1, 2};
    // a patch leaves out the keys that it keeps
    EXPECT_EQ(merge_with(t, R"({"y":3})", yyjson::strict), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({})", yyjson::strict, yyjson::reject_unknown_keys), R"({"x":1,"y":2})");
    // null removes a key, which only an optional field may lack
    EXPECT_EQ(merge_with(t, R"({"x":null})"), R"({"x":0,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":null})", yyjson::strict),
              error(R"(PolPt is not constructible from JSON null for the key "x" under yyjson::reject_missing_keys at /x)"));
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::strict), R"({"x":1})");
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::reject_null_as_missing), R"({"x":1})");
    // unknown keys and keys twice in the patch
    EXPECT_EQ(merge_with(t, R"({"z":1})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"z":1})", yyjson::reject_unknown_keys), unknown("PolPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})"), R"({"x":4,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})", yyjson::reject_duplicates), repeated("PolPt", "x") + " at /x");
    EXPECT_EQ(merge_with(PolWide(), R"({"j":1,"a":1,"j":1})", yyjson::strict), repeated("PolWide", "j") + " at /j");
    EXPECT_EQ(merge_with(PolWideV(), R"({"j":1,"b":1})", yyjson::strict),
              R"({"a":0,"b":1,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"inner":{"y":1}})"), R"({"x":0,"inner":{"x":0,"y":1}})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"w":1})"), unknown("PolClosedPt", "w") + " at /w");

    // a struct merged in place is a patch too, while a value read whole is read under every policy
    const auto outer = PolOuter{{1, 2}, {{1, {}}}, {{"k", {1, {}}}}};
    EXPECT_EQ(merge_with(outer, R"({"p":{"y":3}})", yyjson::strict),
              R"({"p":{"x":1,"y":3},"v":[{"x":1}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})"), R"({"p":{"x":1,"y":2},"v":[{"x":0,"y":3}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})", yyjson::strict), missing("PolPt", "x") + " at /v/0");
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})"),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1},"n":{"x":0,"y":3}}})");
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})", yyjson::strict), missing("PolPt", "x") + " at /m/n");
    EXPECT_EQ(merge_with(outer, R"({"m":{"k":{"y":3}}})", yyjson::strict),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1,"y":3}}})");
    EXPECT_EQ(merge_with(outer, R"({"m":{"k":{"y":3},"k":{"y":4}}})", yyjson::strict).substr(0, 31),
              "bad cast: the object read into ");

    // the tags of a variant in the patch
    const auto env = PolEnvelope{1, PolRect{1, 2}};
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","data":{"h":3}})", yyjson::strict, yyjson::reject_unknown_keys),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})", yyjson::strict), missing("PolCircle", "r") + " at /data");
    // a new alternative is read whole, and a failure leaves the variant as it was
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})"), R"({"type":"circle","r":0})");
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})", yyjson::strict), missing("PolCircle", "r"));
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"rect","h":3})", yyjson::strict),
              R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","kind":"rect"})", yyjson::strict),
              repeated("PolEnvelope", "kind") + " at /kind");
}

TEST(ClosedTypes, Read)
{
    // the keys of the fields in any order, and an unknown key before, among and after them
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":2})"), R"({"x":1,"y":2})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"y":2,"x":1})"), R"({"x":1,"y":2})");
        EXPECT_EQ(read_with<T>(R"({})"), R"({"x":0})");
        for (const auto json : {R"({"z":0,"x":1,"y":2})", R"({"x":1,"z":0,"y":2})", R"({"x":1,"y":2,"z":0})",
                                R"({"x":1,"z":null})", R"({"z":{}})"})
        {
            EXPECT_EQ(read_with<T>(json), closed(name, "z") + " at /z") << json;
            EXPECT_EQ(error_path<T>(json), "/z") << json;
            // the closed type reports the key also under the policy that rejects it
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), closed(name, "z") + " at /z") << json;
            EXPECT_EQ(read_with<T>(json, yyjson::strict), closed(name, "z") + " at /z") << json;
        }
        EXPECT_EQ(read_with<T>(R"({"y":1})", yyjson::reject_missing_keys), missing(name, "x"));
    };
    expect(std::type_identity<ClosedPt>(), "ClosedPt");
    expect(std::type_identity<ClosedPtV>(), "ClosedPtV");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1,"k":1})"), closed("ClosedWide", "k") + " at /k");
    // the key of a renamed field is its new key, and a skipped field has no key
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1})"), R"({"A":1})");
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"a":1})"), closed("ClosedRenamed", "a") + " at /a");
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1,"s":2})"), closed("ClosedRenamed", "s") + " at /s");
    // the struct that holds a closed type is not closed
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1},"q":1})"), R"({"p":{"x":1},"v":[]})");
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1,"q":1}})"), closed("ClosedPt", "q") + " at /p/q");
    EXPECT_EQ(read_with<ClosedHolder>(R"({"v":[{},{"q":1}]})"), closed("ClosedPt", "q") + " at /v/1/q");
}

TEST(ClosedTypes, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ClosedShape>(R"({"w":1,"type":"rect","h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2,"r":1})"), closed("ClosedRect", "r") + " at /r");
    EXPECT_EQ(read_with<ClosedCircle>(R"({"type":"circle","r":1})"), closed("ClosedCircle", "type") + " at /type");
    // the keys of an adjacent tag are keys of the struct that holds the variant
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"data":{"r":3},"kind":"circle","id":1})"),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})"),
              closed("ClosedEnvelope", "z") + " at /z");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2,"z":1}})"),
              closed("ClosedRect", "z") + " at /data/z");
}

TEST(ClosedTypes, MergeInto)
{
    // the keys of the patch are checked, but an unknown key of null removes nothing
    const auto t = ClosedPt{1, 2};
    EXPECT_EQ(merge_with(t, R"({"y":3})"), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"z":1})"), closed("ClosedPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"y":3,"z":{}})", yyjson::reject_unknown_keys), closed("ClosedPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"z":null})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"y":null,"z":null})"), R"({"x":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":null,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":1})"), closed("ClosedWide", "k") + " at /k");
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":1}})"), closed("ClosedPt", "q") + " at /p/q");
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":null}})"), R"({"p":{"x":0},"v":[]})");
    // the alternatives of a variant in the patch
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","h":3})"), R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","r":3})"),
              closed("ClosedRect", "r") + " at /r");
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"circle","w":3})"),
              closed("ClosedCircle", "w") + " at /w");
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","data":{"h":3}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","z":1})"),
              closed("ClosedEnvelope", "z") + " at /z");
}

TEST(ClosedTypes, Write)
{
    // a closed type writes its fields and tags as before
    EXPECT_EQ(std::string(yyjson::write(ClosedEnvelope{1, ClosedCircle{3}})),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(std::string(yyjson::write(ClosedShape(ClosedRect{1, 2}))), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(std::string(yyjson::write(ClosedRenamed{1, 2})), R"({"A":1})");
}

TEST(PropertyCounts, Read)
{
    // every member of the object counts: unknown keys, null values and repeated keys too
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"a":1})"), R"({"a":1,"b":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"b":2,"a":1})"), R"({"a":1,"b":2})");
        EXPECT_EQ(read_with<T>(R"({"z":1})"), R"({"a":0,"b":0})");
        EXPECT_EQ(read_with<T>(R"({})"), fewer(name, 1));
        EXPECT_EQ(error_path<T>(R"({})"), "");
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})"), more(name, 2));
        for (const auto json : {R"({"a":1,"b":2,"z":3})", R"({"a":1,"c":null,"b":2})", R"({"a":1,"a":2,"a":3})",
                                R"({"z":1,"z":2,"z":3})"})
        {
            EXPECT_EQ(read_with<T>(json), more(name, 2)) << json;
        }
        // the policies check the keys first
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"z":3})", yyjson::reject_unknown_keys), unknown(name, "z") + " at /z");
        EXPECT_EQ(read_with<T>(R"({"a":1,"a":2,"a":3})", yyjson::reject_duplicates), repeated(name, "a") + " at /a");
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})", yyjson::reject_unknown_keys), more(name, 2));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    };
    expect(std::type_identity<CountedPt>(), "CountedPt");
    expect(std::type_identity<CountedPtV>(), "CountedPtV");
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedWide>(R"({})"), fewer("CountedWide", 1));
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1,"k":null})"), more("CountedWide", 2));
    // the count of a struct in a field or an element has its path
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{"a":1},"v":[{"b":1}]})"), R"({"p":{"a":1,"b":0},"v":[{"a":0,"b":1}]})");
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{}})"), fewer("CountedPt", 1) + " at /p");
    EXPECT_EQ(error_path<CountedHolder>(R"({"p":{}})"), "/p");
    EXPECT_EQ(read_with<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), more("CountedPt", 2) + " at /v/1");
    EXPECT_EQ(error_path<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), "/v/1");
    // a key that a closed type rejects comes before the count
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"x":2})"), more("CountedClosed", 1));
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"z":2})"), closed("CountedClosed", "z") + " at /z");
}

TEST(PropertyCounts, Tags)
{
    // the key of an internal tag is a member of the alternative
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1})"), fewer("CountedRect", 3));
    EXPECT_EQ(read_with<CountedRect>(R"({"w":1,"h":2})"), fewer("CountedRect", 3));
    // the keys of an adjacent tag are members of the struct that holds the variant
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":null})"),
              more("CountedEnvelope", 3));
}

TEST(PropertyCounts, MergeInto)
{
    // the members of the patch that are not null count, as for a map
    const auto t = CountedPt{1, 2, 3};
    EXPECT_EQ(merge_with(t, R"({"a":4})"), R"({"a":4,"b":2,"c":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"z":1,"c":null})"), R"({"a":1,"b":2})");
    EXPECT_EQ(merge_with(t, R"({"a":4,"z":null,"c":null,"b":5})"), R"({"a":4,"b":5})");
    EXPECT_EQ(merge_with(t, R"({})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({"c":null})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({ "c" : null })"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({ "a" : 4 , "b" : null })"), R"({"a":4,"b":0,"c":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"a":4,"b":5,"z":6})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(t, R"({"a":4,"a":5,"a":6})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(CountedPtV{1, 2, 3}, R"({"c":null})"), fewer("CountedPtV", 1));
    EXPECT_EQ(merge_with(CountedWide(), R"({"j":1,"k":null,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(CountedWide(), R"({"j":1,"k":1,"a":1})"), more("CountedWide", 2));
    EXPECT_EQ(merge_with(CountedHolder(), R"({"p":{"c":null}})"), fewer("CountedPt", 1) + " at /p");
    EXPECT_EQ(merge_with(CountedShape(CountedRect{1, 2}), R"({"type":"rect","h":3,"w":null})"),
              fewer("CountedRect", 3));
}

TEST(PropertyCounts, Write)
{
    // `yyjson::min_properties` writes the fields equal to their defaults, but not an empty optional
    EXPECT_EQ(std::string(yyjson::write(CountedDefaults())), R"({"a":0,"b":5})");
    EXPECT_EQ(std::string(yyjson::write(UncountedDefaults())), R"({"a":0})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{CountedDefaults()})), R"([{"a":0,"b":5}])");
    auto doc = yyjson::object(CountedDefaults());
    EXPECT_EQ(std::string(doc.write()), R"({"a":0,"b":5})");
    // the count is not checked when writing
    EXPECT_EQ(std::string(yyjson::write(CountedPt{1, 2, 3})), R"({"a":1,"b":2,"c":3})");
}

// NOLINTEND
