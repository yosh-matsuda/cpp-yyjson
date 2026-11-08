#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::with_caster<T, X>`, whose value X converts instead of `caster<T>`

// an integer as a hexadecimal string, in the translator form
struct Hex
{
    static std::string to_json(int v) { return std::format("{:x}", v); }
    static int from_json(std::string_view s)
    {
        auto v = 0;
        const auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v, 16);
        if (ec != std::errc() || p != s.data() + s.size()) throw std::invalid_argument("not hexadecimal");
        return v;
    }
};
using HexInt = yyjson::with_caster<int, Hex>;

// the in-place form, and the in-place form together with the translator form
struct HexInPlace
{
    static void from_json(int& v, std::string_view s) { v = Hex::from_json(s); }
    static std::string to_json(const int& v, yyjson::copy_string_t) { return Hex::to_json(v); }
};
struct HexBoth
{
    static int from_json(std::string_view s) { return Hex::from_json(s); }
    static void from_json(int& v, std::string_view s) { v = Hex::from_json(s) + 1000; }
    static std::string to_json(int v) { return Hex::to_json(v); }
};

// a converter that only writes, and one that only reads
struct Mask
{
    static std::string_view to_json(const std::string&) { return "***"; }
};
struct Lenient
{
    static int from_json(const std::string& s) { return std::stoi(s); }
};

struct WcPoint
{
    int x = 0;
    int y = 0;
    bool operator==(const WcPoint&) const = default;
    auto operator<=>(const WcPoint&) const = default;
};

// a point as [x, y] through emitters, and as {"X": x, "Y": y} through the DOM API
struct PairPoint
{
    template <typename... Ts>
    static void to_json(yyjson::array_emitter& out, const WcPoint& p, Ts...)
    {
        out.emplace_back(p.x);
        out.emplace_back(p.y);
    }
    static WcPoint from_json(const std::array<int, 2>& a) { return {a[0], a[1]}; }
};
struct UpperPoint
{
    template <typename Json>
    static WcPoint from_json(const Json& json)
    {
        const auto m = json.template cast<std::map<std::string, int>>();
        return {m.at("X"), m.at("Y")};
    }
    template <typename... Ts>
    static void to_json(yyjson::writer::object_ref& obj, const WcPoint& p, Ts... ts)
    {
        obj.emplace("X", p.x, ts...);
        obj.emplace("Y", p.y, ts...);
    }
};
// the DOM form as overloads, and a `value_ref` that the converter assigns
struct NegPoint
{
    static WcPoint from_json(const yyjson::reader::const_value_ref& v)
    {
        const auto m = v.cast<std::map<std::string, int>>();
        return {-m.at("x"), -m.at("y")};
    }
    static WcPoint from_json(const yyjson::writer::detail::const_value_ref& v)
    {
        const auto m = v.cast<std::map<std::string, int>>();
        return {-m.at("x"), -m.at("y")};
    }
    static void to_json(yyjson::writer::value_ref& v, const WcPoint& p)
    {
        v = std::map<std::string, int>{{"x", -p.x}, {"y", -p.y}};
    }
};
// a value left out of an object when it is zero
struct OmitZero
{
    static int to_json(int v) { return v; }
    static bool should_omit(int v) { return v == 0; }
};
// an optional value that the converter reads from null too
struct NullAsZero
{
    static std::optional<int> from_json(const std::optional<int>& v) { return v.value_or(0); }
};

#if YYJSON_VERSION_HEX >= 0x000C00
// a point as "x,y", or as [x, y], read through a view
struct ViewPoint
{
    static WcPoint from_json(yyjson::json_view v)
    {
        if (const auto s = v.as_string())
        {
            const auto comma = s->find(',');
            return {std::stoi(std::string(s->substr(0, comma))), std::stoi(std::string(s->substr(comma + 1)))};
        }
        const auto a = *v.as_array();
        return {a[0].cast<int>(), a[1].cast<int>()};
    }
    static std::string to_json(const WcPoint& p) { return std::format("{},{}", p.x, p.y); }
};
#endif

struct WcFields
{
    HexInt id;
    yyjson::with_caster<std::string, Mask> secret = "s";
    std::vector<HexInt> ids;
    std::map<std::string, HexInt> named;
    yyjson::with_caster<int, Lenient> n;
    bool operator==(const WcFields&) const = default;
};

struct WcForms
{
    yyjson::with_caster<int, HexInPlace> in_place;
    yyjson::with_caster<int, HexBoth> both;
    yyjson::with_caster<WcPoint, PairPoint> pair;
    yyjson::with_caster<WcPoint, UpperPoint> upper;
    yyjson::with_caster<WcPoint, NegPoint> neg;
    bool operator==(const WcForms&) const = default;
};

struct WcOmit
{
    yyjson::with_caster<int, OmitZero> z;
    int a = 1;
    bool operator==(const WcOmit&) const = default;
};

// the outer forms of presence, and an optional that the converter receives whole
struct WcPresence
{
    std::optional<HexInt> o;
    yyjson::nullable<HexInt> n;
    yyjson::with_caster<std::optional<int>, NullAsZero> z;
    bool operator==(const WcPresence&) const = default;
};

struct WcArray
{
    HexInt a;
    int b = 0;
    bool operator==(const WcArray&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WcArray> = yyjson::annotate(yyjson::as_array);

struct WcTransparent
{
    HexInt v;
    bool operator==(const WcTransparent&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WcTransparent> = yyjson::annotate(yyjson::transparent);

struct WcExtra
{
    int a = 0;
    std::map<std::string, HexInt> rest;
    bool operator==(const WcExtra&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WcExtra> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));

struct WcTaggedA
{
    HexInt id;
    bool operator==(const WcTaggedA&) const = default;
};
struct WcTaggedB
{
    yyjson::with_caster<WcPoint, PairPoint> p;
    bool operator==(const WcTaggedB&) const = default;
};
using WcTagged = std::variant<WcTaggedA, WcTaggedB>;
template <>
inline constexpr auto yyjson::annotations<WcTagged> = yyjson::annotate(yyjson::internal_tag("kind", {"a", "b"}));

// defaults compare the values, which a masking converter writes alike
struct HexReal
{
    static std::string to_json(double v) { return std::format("{}", v); }
    static double from_json(std::string_view s) { return std::stod(std::string(s)); }
};
struct WcZero
{
    yyjson::with_caster<double, HexReal> z = 0.0;
    yyjson::with_caster<std::string, Mask> secret = "s";
    bool operator==(const WcZero&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WcZero> =
    yyjson::annotate(yyjson::field("z", yyjson::default_), yyjson::field("secret", yyjson::default_));

bool wc_even(const HexInt& v) { return v.value % 2 == 0; }
struct WcChecked
{
    HexInt even;
    HexInt small;
    std::optional<HexInt> opt;
    bool operator==(const WcChecked&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<WcChecked> =
    yyjson::annotate(yyjson::field("even", yyjson::satisfies<wc_even>),
                     yyjson::field("small", yyjson::satisfies<[](const int& v) { return v < 16; }>),
                     yyjson::field("opt", yyjson::satisfies<[](const int& v) { return v != 1; }>));

#if FIELD_RFL_ANNOTATIONS
struct WcAnnotated
{
    [[= yyjson::rename("t")]] HexInt id;
    bool operator==(const WcAnnotated&) const = default;
};
#endif

// the wrapper is no larger than its value, and copies and compares as its value
static_assert(sizeof(HexInt) == sizeof(int));
static_assert(sizeof(yyjson::with_caster<std::string, Mask>) == sizeof(std::string));
static_assert(std::is_trivially_copyable_v<HexInt> && std::is_standard_layout_v<HexInt>);
static_assert(std::is_trivially_default_constructible_v<HexInt>);
static_assert(!std::is_trivially_copyable_v<yyjson::with_caster<std::string, Mask>>);
static_assert(!std::is_aggregate_v<HexInt>);
static_assert(std::regular<HexInt> && std::totally_ordered<HexInt>);
static_assert(std::regular<yyjson::with_caster<std::string, Mask>>);
static_assert(std::same_as<HexInt::value_type, int> && std::same_as<HexInt::caster_type, Hex>);
static_assert(field_reflection::field_count<WcFields> == 5);
static_assert(field_reflection::field_count<WcTransparent> == 1);
static_assert(std::is_default_constructible_v<std::hash<HexInt>>);
static_assert(!std::is_default_constructible_v<std::hash<yyjson::with_caster<WcPoint, PairPoint>>>);
#if __cplusplus > 202002L
static_assert(std::formattable<HexInt, char>);
static_assert(!std::formattable<yyjson::with_caster<WcPoint, PairPoint>, char>);
#endif
// conversions: as those of T (as std::optional), and not tuple-like
static_assert(std::convertible_to<int, HexInt> && std::convertible_to<HexInt, int>);
static_assert(std::convertible_to<double, HexInt>);
static_assert(std::convertible_to<int, yyjson::with_caster<std::uint64_t, Hex>>);
static_assert(std::convertible_to<int, yyjson::with_caster<double, Hex>>);
static_assert(!std::convertible_to<std::size_t, yyjson::with_caster<std::vector<double>, Hex>>);
static_assert(std::constructible_from<yyjson::with_caster<std::vector<double>, Hex>, std::size_t>);
static_assert(std::convertible_to<const char*, yyjson::with_caster<std::string, Mask>>);
static_assert(!std::convertible_to<std::string_view, yyjson::with_caster<std::string, Mask>>);
static_assert(std::constructible_from<yyjson::with_caster<std::string, Mask>, std::string_view>);
static_assert(std::constructible_from<yyjson::with_caster<std::vector<int>, Hex>, std::in_place_t, int, int>);
static_assert(!std::constructible_from<HexInt, std::initializer_list<int>>);
template <typename T>
concept wc_tuple_like = requires { std::tuple_size<T>::value; };
static_assert(!wc_tuple_like<HexInt>);
// the comparisons with the value, and none with a wrapper of another converter
template <typename A, typename B>
concept wc_equal = requires(const A& a, const B& b) { a == b; };
template <typename A, typename B>
concept wc_less = requires(const A& a, const B& b) { a < b; };
static_assert(wc_equal<HexInt, int> && wc_equal<int, HexInt> && wc_less<HexInt, int> && wc_less<int, HexInt>);
static_assert(!wc_equal<HexInt, yyjson::with_caster<int, HexBoth>> &&
              !wc_less<HexInt, yyjson::with_caster<int, HexBoth>>);
static_assert(!wc_equal<HexInt, yyjson::with_caster<long, Hex>>);
static_assert(wc_equal<yyjson::with_caster<std::string, Mask>, const char*> &&
              wc_equal<const char*, yyjson::with_caster<std::string, Mask>>);
static_assert(wc_equal<yyjson::with_caster<std::string, Mask>, std::string_view> &&
              wc_less<std::string_view, yyjson::with_caster<std::string, Mask>>);
// the concepts of the standard see the common reference of the value
static_assert(std::equality_comparable_with<HexInt, int> && std::totally_ordered_with<int, HexInt>);
static_assert(std::equality_comparable_with<yyjson::with_caster<std::string, Mask>, std::string>);
static_assert(std::same_as<std::common_reference_t<HexInt&, int&>, int&>);
// T without <=>, or with == only
struct WcEqOnly
{
    int v = 0;
    bool operator==(const WcEqOnly&) const = default;
};
struct WcNoCompare
{
    int v = 0;
};
static_assert(std::equality_comparable<yyjson::with_caster<WcEqOnly, Hex>> &&
              !wc_less<yyjson::with_caster<WcEqOnly, Hex>, yyjson::with_caster<WcEqOnly, Hex>>);
static_assert(!std::equality_comparable<yyjson::with_caster<WcNoCompare, Hex>> &&
              std::copyable<yyjson::with_caster<WcNoCompare, Hex>>);
// the forms of the converter are those of the caster of the wrapper
static_assert(yyjson::detail::from_json_translator<HexInt>::returns);
static_assert(!yyjson::detail::from_json_translator<HexInt>::in_place);
static_assert(yyjson::detail::from_json_translator<yyjson::with_caster<int, HexInPlace>>::in_place);
static_assert(yyjson::detail::from_json_translator<yyjson::with_caster<int, HexBoth>>::returns &&
              yyjson::detail::from_json_translator<yyjson::with_caster<int, HexBoth>>::in_place);
static_assert(yyjson::writer::detail::to_json_arr_emit<yyjson::with_caster<WcPoint, PairPoint>>);
static_assert(yyjson::writer::detail::to_json_obj_usr<yyjson::with_caster<WcPoint, UpperPoint>>);
static_assert(yyjson::writer::detail::to_json_val_usr<yyjson::with_caster<WcPoint, NegPoint>>);
static_assert(yyjson::detail::omittable<yyjson::with_caster<int, OmitZero>>);
static_assert(!yyjson::detail::omittable<HexInt>);
// X with neither direction for T, or with a form that cannot be called
struct NoConversion
{
};
struct WrongReturn
{
    static long from_json(std::string_view) { return 0; }
};
struct WrongWrite
{
    static std::string to_json(const std::string&) { return {}; }
};
static_assert(!yyjson::detail::converter_names_from_json<NoConversion> &&
              !yyjson::detail::converter_names_to_json<NoConversion> &&
              !yyjson::detail::converter_reads<int, NoConversion> &&
              !yyjson::detail::converter_writes<int, NoConversion>);
static_assert(yyjson::detail::converter_names_from_json<WrongReturn> &&
              !yyjson::detail::converter_reads<int, WrongReturn>);
static_assert(yyjson::detail::converter_names_to_json<WrongWrite> &&
              !yyjson::detail::converter_writes<int, WrongWrite>);
static_assert(yyjson::detail::converter_names_from_json<UpperPoint> &&
              yyjson::detail::converter_reads<WcPoint, UpperPoint>);

namespace
{
    // reads `json` as T in each way and writes the result back in each way; returns what it writes
    template <typename T>
    std::string expect_with_caster(std::string_view json)
    {
        EXPECT_TRUE(expect_direct_read<T>(json)) << json;
        EXPECT_TRUE(expect_read_into(T(), json)) << json;
        const auto t = yyjson::read<T>(json);
        EXPECT_EQ(yyjson::read(json).cast<T>(), t) << json;
        expect_direct_write(t);
        const auto written = std::string(yyjson::write(t));
        EXPECT_EQ(written, std::string(yyjson::value(t).write())) << json;
        EXPECT_EQ(yyjson::read<T>(written), t) << json;
        return written;
    }
}  // namespace

TEST(WithCaster, Fields)
{
    const auto json = R"({"id":"ff","secret":"abc","ids":["1","a"],"named":{"k":"10"},"n":"42"})";
    const auto t = yyjson::read<WcFields>(json);
    EXPECT_EQ(t.id, 255);
    EXPECT_EQ(t.secret, "abc");
    EXPECT_EQ(t.ids, (std::vector<HexInt>{1, 10}));
    EXPECT_EQ(t.named.at("k"), 16);
    EXPECT_EQ(t.n, 42);
    // the write-only converter masks the secret, and the read-only one writes an integer
    EXPECT_EQ(written(t), R"({"id":"ff","secret":"***","ids":["1","a"],"named":{"k":"10"},"n":42})");
    EXPECT_EQ(std::string(yyjson::value(t).write()), written(t));
    EXPECT_EQ(std::string(yyjson::object(t).write()), written(t));
    EXPECT_EQ(yyjson::read(json).cast<WcFields>(), t);
    EXPECT_TRUE(expect_direct_read<WcFields>(json));
    EXPECT_TRUE(expect_read_into(WcFields(), json));
    expect_direct_write(t);
    expect_direct_write(WcFields());
}

TEST(WithCaster, Forms)
{
    const auto json = R"({"in_place":"a","both":"b","pair":[1,2],"upper":{"X":3,"Y":4},"neg":{"x":5,"y":6}})";
    const auto t = yyjson::read<WcForms>(json);
    EXPECT_EQ(t.in_place, 10);
    // a field takes the in-place form, as for a caster that has both
    EXPECT_EQ(t.both, 1011);
    EXPECT_EQ((yyjson::read<yyjson::with_caster<int, HexBoth>>(R"("b")")), 11);
    EXPECT_EQ(t.pair, (WcPoint{1, 2}));
    EXPECT_EQ(t.upper, (WcPoint{3, 4}));
    EXPECT_EQ(t.neg, (WcPoint{-5, -6}));
    EXPECT_EQ(yyjson::read(json).cast<WcForms>(), t);
    EXPECT_TRUE(expect_direct_read<WcForms>(json));
    EXPECT_TRUE(expect_read_into(WcForms(), json));
    EXPECT_EQ(written(t), R"({"in_place":"a","both":"3f3","pair":[1,2],"upper":{"X":3,"Y":4},"neg":{"x":5,"y":6}})");
    EXPECT_EQ(std::string(yyjson::value(t).write()), written(t));
    expect_direct_write(t);
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(WithCaster, JsonView)
{
    using V = yyjson::with_caster<WcPoint, ViewPoint>;
    EXPECT_EQ(yyjson::read<V>(R"("1,2")"), (WcPoint{1, 2}));
    EXPECT_EQ(yyjson::read<V>("[3,4]"), (WcPoint{3, 4}));
    EXPECT_EQ(expect_with_caster<std::vector<V>>(R"(["1,2",[3,4]])"), R"(["1,2","3,4"])");
    EXPECT_EQ((expect_with_caster<std::map<std::string, V>>(R"({"a":[5,6]})")), R"({"a":"5,6"})");
}
#endif

TEST(WithCaster, Positions)
{
    // root, elements, map values and keys, and tuples
    EXPECT_EQ((expect_with_caster<HexInt>(R"("1f")")), R"("1f")");
    EXPECT_EQ((expect_with_caster<std::vector<HexInt>>(R"(["1","2"])")), R"(["1","2"])");
    EXPECT_EQ((expect_with_caster<std::map<std::string, HexInt>>(R"({"a":"b"})")), R"({"a":"b"})");
    EXPECT_EQ((expect_with_caster<std::map<HexInt, int>>(R"([["ff",1],["a",2]])")), R"([["a",2],["ff",1]])");
    EXPECT_EQ((expect_with_caster<std::unordered_map<HexInt, int>>(R"([["ff",1]])")), R"([["ff",1]])");
    EXPECT_EQ((expect_with_caster<std::tuple<HexInt, int>>(R"(["c",3])")), R"(["c",3])");
    // as_array, transparent, extra_keys and tagged structs
    EXPECT_EQ(expect_with_caster<WcArray>(R"(["ff",2])"), R"(["ff",2])");
    EXPECT_EQ(expect_with_caster<WcTransparent>(R"("ff")"), R"("ff")");
    EXPECT_EQ(expect_with_caster<std::vector<WcTransparent>>(R"(["1","2"])"), R"(["1","2"])");
    EXPECT_EQ(expect_with_caster<WcExtra>(R"({"a":1,"x":"ff","y":"0"})"), R"({"a":1,"x":"ff","y":"0"})");
    EXPECT_EQ(expect_with_caster<WcTagged>(R"({"kind":"a","id":"ff"})"), R"({"kind":"a","id":"ff"})");
    EXPECT_EQ(expect_with_caster<WcTagged>(R"({"p":[1,2],"kind":"b"})"), R"({"kind":"b","p":[1,2]})");
}

TEST(WithCaster, Variants)
{
    // the alternatives see the JSON that X reads: a string for the converter, a number for int
    using V = std::variant<HexInt, int>;
    EXPECT_EQ(yyjson::read<V>(R"("ff")").index(), 0u);
    EXPECT_EQ(yyjson::read<V>("5").index(), 1u);
    EXPECT_EQ(expect_with_caster<std::vector<V>>(R"(["ff",5])"), R"(["ff",5])");
    using A = yyjson::any_of_variant<HexInt, int>;
    EXPECT_EQ(yyjson::read<A>(R"("ff")").index(), 0u);
    EXPECT_EQ(yyjson::read<A>("5").index(), 1u);
    EXPECT_EQ(expect_with_caster<std::vector<A>>(R"(["10",7])"), R"(["10",7])");
    using O = yyjson::one_of_variant<HexInt, int>;
    EXPECT_EQ(yyjson::read<O>(R"("ff")").index(), 0u);
    EXPECT_EQ(yyjson::read<O>("5").index(), 1u);
    // the reader without a document leaves a one_of_variant of a caster to the document, as without the wrapper
    static_cast<void>(expect_direct_read<std::vector<O>>(R"(["10",7])"));
    static_cast<void>(expect_read_into(std::vector<O>(), R"(["10",7])"));
    EXPECT_EQ(written(yyjson::read<std::vector<O>>(R"(["10",7])")), R"(["10",7])");
    expect_direct_write(yyjson::read<std::vector<O>>(R"(["10",7])"));
}

TEST(WithCaster, Presence)
{
    EXPECT_EQ(expect_with_caster<WcPresence>(R"({"n":null,"z":null})"), R"({"n":null,"z":0})");
    EXPECT_EQ(expect_with_caster<WcPresence>(R"({"o":"a","n":"b","z":3})"), R"({"o":"a","n":"b","z":3})");
    const auto t = yyjson::read<WcPresence>(R"({"o":null,"n":null,"z":null})");
    EXPECT_FALSE(t.o.has_value());
    EXPECT_FALSE(t.n.has_value());
    EXPECT_EQ(t.z.value, 0);
    // the key of an optional that the converter receives whole is required, and its empty value is written
    EXPECT_EQ(read_or_error([] { return yyjson::read<WcPresence>(R"({"n":null})", yyjson::strict); }),
              read_or_error([] { return yyjson::read(R"({"n":null})").cast<WcPresence>(yyjson::strict); }));
    EXPECT_EQ(read_or_error([] { return yyjson::read<WcPresence>(R"({"n":null})", yyjson::strict); }),
              "bad cast: WcPresence is not constructible from JSON without the key \"z\"");
    EXPECT_EQ(written(WcPresence()), R"({"n":null,"z":null})");
    // the converter decides whether a field is left out
    EXPECT_EQ(expect_with_caster<WcOmit>(R"({"z":0,"a":2})"), R"({"a":2})");
    EXPECT_EQ(expect_with_caster<WcOmit>(R"({"z":3})"), R"({"z":3,"a":1})");
}

TEST(WithCaster, Merge)
{
    auto t = WcFields();
    t.id = 1;
    t.named.emplace("a", 2);
    EXPECT_EQ(expect_merge_into(t, R"({"id":"ff","named":{"b":"3"}})"),
              R"({"id":"ff","secret":"***","ids":[],"named":{"a":"2","b":"3"},"n":0})");
    EXPECT_EQ(expect_merge_into(WcForms(), R"({"both":"1","pair":[7,8]})"),
              R"({"in_place":"0","both":"3e9","pair":[7,8],"upper":{"X":0,"Y":0},"neg":{"x":0,"y":0}})");
}

TEST(WithCaster, Errors)
{
    // a JSON value that X does not take, at the path of the field or the element
    const auto id = R"({"id":1})";
    EXPECT_EQ(read_or_error([&] { return yyjson::read<WcFields>(id); }),
              read_or_error([&] { return yyjson::read(id).cast<WcFields>(); }));
    EXPECT_TRUE(read_or_error([&] { return yyjson::read<WcFields>(id); }).find("/id") != std::string::npos)
        << read_or_error([&] { return yyjson::read<WcFields>(id); });
    const auto ids = R"({"ids":["1",2]})";
    EXPECT_EQ(read_or_error([&] { return yyjson::read<WcFields>(ids); }),
              read_or_error([&] { return yyjson::read(ids).cast<WcFields>(); }));
    EXPECT_TRUE(read_or_error([&] { return yyjson::read<WcFields>(ids); }).find("/ids/1") != std::string::npos)
        << read_or_error([&] { return yyjson::read<WcFields>(ids); });
    // the position of the text, as for any caster
    try
    {
        static_cast<void>(yyjson::read<WcFields>(ids));
        ADD_FAILURE();
    }
    catch (const yyjson::bad_cast& e)
    {
        if (reads_directly)
        {
            EXPECT_TRUE(e.position().has_value());
        }
    }
    // an exception of X passes through
    EXPECT_THROW(static_cast<void>(yyjson::read<WcFields>(R"({"id":"zz"})")), std::invalid_argument);
    // read policies apply as to any field
    EXPECT_EQ(read_or_error([] { return yyjson::read<WcOmit>(R"({"a":1})", yyjson::strict); }),
              "bad cast: WcOmit is not constructible from JSON without the key \"z\"");
}

TEST(WithCaster, Defaults)
{
    // the secret that differs from the default is written, although X writes it as the default
    EXPECT_EQ(written(WcZero()), "{}");
    EXPECT_EQ(written(WcZero{0.0, "x"}), R"({"secret":"***"})");
    EXPECT_EQ(std::string(yyjson::value(WcZero{0.0, "x"}).write()), R"({"secret":"***"})");
    // -0.0 equals the default 0.0, as JSON Schema compares numbers
    EXPECT_EQ(written(WcZero{-0.0, "s"}), "{}");
    EXPECT_EQ(std::string(yyjson::value(WcZero{-0.0, "s"}).write()), "{}");
    EXPECT_EQ(std::string(yyjson::write(WcZero{-0.0, "s"}, yyjson::emit_defaults)), R"({"z":"-0","secret":"***"})");
    EXPECT_EQ(std::string(yyjson::write(WcZero(), yyjson::emit_defaults)), R"({"z":"0","secret":"***"})");
    EXPECT_TRUE((yyjson::detail::json_equal<yyjson::with_caster<std::string, Mask>>("a", "a")));
    EXPECT_FALSE((yyjson::detail::json_equal<yyjson::with_caster<std::string, Mask>>("a", "b")));
    EXPECT_TRUE((yyjson::detail::json_equal<yyjson::with_caster<double, HexReal>>(0.0, -0.0)));
}

TEST(WithCaster, Satisfies)
{
    EXPECT_EQ(expect_with_caster<WcChecked>(R"({"even":"a","small":"f"})"), R"({"even":"a","small":"f"})");
    EXPECT_EQ(read_or_error([] { return yyjson::read<WcChecked>(R"({"even":"b","small":"f"})"); }),
              read_or_error([] { return yyjson::read(R"({"even":"b","small":"f"})").cast<WcChecked>(); }));
    EXPECT_NE(read_or_error([] { return yyjson::read<WcChecked>(R"({"even":"b","small":"f"})"); }).find("/even"),
              std::string::npos);
    EXPECT_NE(read_or_error([] { return yyjson::read<WcChecked>(R"({"even":"a","small":"10"})"); }).find("/small"),
              std::string::npos);
    EXPECT_NE(
        read_or_error([] { return yyjson::read<WcChecked>(R"({"even":"a","small":"1","opt":"1"})"); }).find("/opt"),
        std::string::npos);
    EXPECT_EQ(expect_with_caster<WcChecked>(R"({"even":"a","small":"1","opt":"2"})"),
              R"({"even":"a","small":"1","opt":"2"})");
}

#if FIELD_RFL_ANNOTATIONS
TEST(WithCaster, Annotations) { EXPECT_EQ(expect_with_caster<WcAnnotated>(R"({"t":"ff"})"), R"({"t":"ff"})"); }
#endif

TEST(WithCaster, Value)
{
    auto w = HexInt(3);
    int& r = w;
    r = 4;
    EXPECT_EQ(*w, 4);
    EXPECT_EQ(w.value, 4);
    const auto& c = w;
    EXPECT_EQ(static_cast<const int&>(c), 4);
    auto s = yyjson::with_caster<std::string, Mask>("abc");
    EXPECT_EQ(s->size(), 3u);
    EXPECT_EQ(std::as_const(s)->size(), 3u);
    // a temporary gives its value to a parameter of T, and moves it out
    const auto size = [](const std::string& x) { return x.size(); };
    EXPECT_EQ(size(yyjson::with_caster<std::string, Mask>("abcd")), 4u);
    auto moved = std::string(std::move(s));
    EXPECT_EQ(moved, "abc");
    auto star = *yyjson::with_caster<std::string, Mask>("xy");
    EXPECT_EQ(star, "xy");
    // initializer lists and in-place construction
    const auto v = yyjson::with_caster<std::vector<int>, Hex>{1, 2, 3};
    EXPECT_EQ(v->size(), 3u);
    const auto five = yyjson::with_caster<std::vector<int>, Hex>(std::in_place, 5, 1);
    EXPECT_EQ(five->size(), 5u);
    // comparisons with the value and with the same wrapper
    EXPECT_TRUE(w == 4 && 4 == w && w < 5 && 3 < w && w != HexInt(3) && HexInt(3) < w);
    const auto hexes = std::vector<HexInt>{1, 2, 3};
    EXPECT_EQ(std::ranges::find(hexes, 2), hexes.begin() + 1);
    EXPECT_EQ(std::hash<HexInt>()(w), std::hash<int>()(4));
    EXPECT_EQ(std::format("{:>3}", w), "  4");
}

// NOLINTEND
