#include "test_common.hpp"

// NOLINTBEGIN

// C array fields, which are read and written as JSON arrays by the rules of `std::array` fields
struct CArrayInner
{
    int x = 7;
    std::string s;
};

struct CArrays
{
    int a[3] = {1, 2, 3};
    double m[2][3] = {};
    char c[4] = {};
    std::string s[2];
    CArrayInner in[2];
    std::optional<int> o[2];
};

// the same fields as `std::array`
struct StdArrays
{
    std::array<int, 3> a = {1, 2, 3};
    std::array<std::array<double, 3>, 2> m = {};
    std::array<char, 4> c = {};
    std::array<std::string, 2> s;
    std::array<CArrayInner, 2> in;
    std::array<std::optional<int>, 2> o;
};

struct VisitableCArrays
{
    int a[2] = {};
    char c[2][2] = {};
};
VISITABLE_STRUCT(VisitableCArrays, a, c);

struct RecursiveCArray
{
    std::vector<RecursiveCArray> v[2];
};
static_assert(yyjson::detail::recursive_type<RecursiveCArray>);

TEST(Reflection, CArrayFields)
{
    using yyjson::bad_cast;
    const auto* full_json =
        R"({"a":[4,5,6],"m":[[1.5,2.0,3.0],[4.0,5.0,6.0]],"c":[97,98,0,100],"s":["x","y"],)"
        R"("in":[{"x":1,"s":"p"},{"x":2,"s":""}],"o":[1,null]})";
    const auto full = yyjson::read<CArrays>(full_json);
    const auto full_std = yyjson::read<StdArrays>(full_json);
    EXPECT_EQ(full.m[1][2], 6.0);
    EXPECT_EQ(full.c[3], 'd');
    EXPECT_EQ(full.in[0].s, "p");
    EXPECT_FALSE(full.o[1].has_value());

    // `char[N]` is an array of integers, as `std::array<char, N>` is, not a string
    EXPECT_EQ(written(full), full_json);
    EXPECT_EQ(written(full), written(full_std));
    EXPECT_EQ(std::string(yyjson::value(full).write()), full_json);
    EXPECT_EQ(std::string(yyjson::object(full).write()), full_json);
    EXPECT_EQ(std::string(yyjson::value(CArrays(full)).write()), full_json);
    EXPECT_EQ(std::string(yyjson::write(CArrays(full))), full_json);
    EXPECT_EQ(std::string(yyjson::value(std::vector<CArrays>{full}).write()), std::format("[{}]", full_json));
    expect_direct_write(full);
    expect_direct_write(std::vector<CArrays>{full, CArrays()});
    EXPECT_EQ(written(yyjson::read(full_json).cast<CArrays>()), full_json);
    EXPECT_TRUE(expect_direct_read<CArrays>(full_json));
    EXPECT_TRUE(expect_direct_read<std::vector<CArrays>>(std::format("[{},{{}}]", full_json)));

    // a JSON array of another length or any other value throws `bad_cast`, and `read_into` and `merge_into`
    // replace the array, all as for `std::array`
    const auto same_errors = [](std::string s) { return s.starts_with("bad cast") ? std::string("bad cast") : s; };
    for (const auto* json :
         {R"({})", R"({"a":[9],"m":[[1],[2,3]],"c":[97],"s":["x"],"in":[{"x":1}],"o":[1]})",
          R"({"a":[],"m":[],"c":[],"s":[],"in":[],"o":[]})", R"({"m":[[1,2,3]]})", R"({"m":[[1,2,3],[4,5]]})",
          R"({"a":[9,8,7],"m":[[1,2,3],[4,5,6]],"c":[97,98,99,100],"s":["x","y"],"in":[{"x":1},{}],"o":[1,null]})",
          R"({"a":[1,2,3,4]})", R"({"m":[[1,2,3,4]]})",
          R"({"m":[[1],[2],[3]]})", R"({"a":1})", R"({"a":null})", R"({"a":"abc"})", R"({"c":"abc"})",
          R"({"a":{"0":1}})", R"({"m":[1,2]})", R"({"a":["x"]})", R"({"c":[300]})", R"({"in":[1]})",
          R"({"s":null,"o":null})"})
    {
        EXPECT_EQ(same_errors(read_or_error([&] { return yyjson::read(json).cast<CArrays>(); })),
                  same_errors(read_or_error([&] { return yyjson::read(json).cast<StdArrays>(); })))
            << json;
        EXPECT_EQ(same_errors(read_or_error([&] { return yyjson::read<CArrays>(json); })),
                  same_errors(read_or_error([&] { return yyjson::read<StdArrays>(json); })))
            << json;
        expect_direct_read<CArrays>(json);
        expect_read_into(full, json);
        EXPECT_EQ(same_errors(read_or_error([&] {
                      auto t = full;
                      yyjson::read_into(json, t);
                      return t;
                  })),
                  same_errors(read_or_error([&] {
                      auto t = full_std;
                      yyjson::read_into(json, t);
                      return t;
                  })))
            << json;
        EXPECT_EQ(same_errors(expect_merge_into(full, json)), same_errors(expect_merge_into(full_std, json)))
            << json;
    }

    auto t = full;
    yyjson::read_into(R"({"a":[9,8,7],"m":[[8,7,6],[5,4,3]]})", t);
    EXPECT_EQ(written(t), R"({"a":[9,8,7],"m":[[8.0,7.0,6.0],[5.0,4.0,3.0]],"c":[0,0,0,0],"s":["",""],)"
                          R"("in":[{"x":7,"s":""},{"x":7,"s":""}],"o":[null,null]})");
    t = full;
    yyjson::merge_into(R"({"a":[9,8,7],"s":null,"in":[{"s":"q"},{}]})", t);
    EXPECT_EQ(written(t), R"({"a":[9,8,7],"m":[[1.5,2.0,3.0],[4.0,5.0,6.0]],"c":[97,98,0,100],"s":["",""],)"
                          R"("in":[{"x":7,"s":"q"},{"x":7,"s":""}],"o":[1,null]})");
    EXPECT_THROW((void)yyjson::read<CArrays>(R"({"a":[1,2,3,4]})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<CArrays>(R"({"a":[1,2]})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<CArrays>(R"({"m":[[1,2,3],[4,5]]})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"m":[[1,2,3],[4,5]]})").cast<CArrays>(), bad_cast);
    EXPECT_THROW(yyjson::read_into(R"({"a":[9]})", t), bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"a":[9]})", t), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"m":[[1],[2],[3]]})").cast<CArrays>(), bad_cast);
    EXPECT_THROW(yyjson::read_into(R"({"a":1})", t), bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"c":"abc"})", t), bad_cast);

    // a struct of `VISITABLE_STRUCT`
    const auto* visitable_json = R"({"a":[1,2],"c":[[97,98],[99,0]]})";
    const auto visitable = yyjson::read<VisitableCArrays>(visitable_json);
    EXPECT_EQ(visitable.c[1][0], 'c');
    EXPECT_EQ(written(visitable), visitable_json);
    EXPECT_EQ(std::string(yyjson::value(visitable).write()), visitable_json);
    EXPECT_EQ(written(yyjson::read(visitable_json).cast<VisitableCArrays>()), visitable_json);
    expect_direct_write(visitable);
    EXPECT_TRUE(expect_direct_read<VisitableCArrays>(R"({"a":[1,2],"c":[[97,98],[99,100]]})"));
    expect_direct_read<VisitableCArrays>(R"({"a":[1,2,3]})");
    expect_direct_read<VisitableCArrays>(R"({"a":[1]})");
    expect_direct_read<VisitableCArrays>(R"({"c":[[1,2],[3]]})");
    expect_read_into(visitable, R"({"c":[[1,2],[3,4]]})");
    expect_read_into(visitable, R"({"c":[[1]]})");
    EXPECT_EQ(expect_merge_into(visitable, R"({"c":[[1,2],[3,4]]})"), R"({"a":[1,2],"c":[[1,2],[3,4]]})");
    EXPECT_EQ(expect_merge_into(visitable, R"({"c":[[1]]})").substr(0, 8), "bad cast");

    // the element type of a C array leads back to the struct
    const auto* recursive_json = R"({"v":[[{"v":[[],[{"v":[[],[]]}]]}],[]]})";
    EXPECT_EQ(written(yyjson::read<RecursiveCArray>(recursive_json)), recursive_json);
    EXPECT_TRUE(expect_direct_read<RecursiveCArray>(recursive_json));
}

// A struct whose caster writes a key and a value from C arrays of `char` without a NUL
struct EmittedCharArrays
{
    char key[2] = {'k', 'y'};
    char value[3] = {'v', 'a', 'l'};
};
template <>
struct yyjson::caster<EmittedCharArrays>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const EmittedCharArrays& e, Ts... ts)
    {
        out.emplace(e.key, e.value, ts...);
        out.emplace_array("a", [&](yyjson::array_emitter& a) { a.emplace_back(e.value, ts...); });
    }
};

// A C array of `char` that is an element of an array or a range is written as integers, as a field is.  A
// 1-dimensional one written as a value or a key of its own, such as a string literal, is a string that ends at its
// first NUL, or at its end when it has none.
TEST(Writer, CharArrays)
{
    const char nested[2][3] = {{'a', 'b', 'c'}, {'d', 'e', 'f'}};
    EXPECT_EQ(std::string(yyjson::write(nested)), "[[97,98,99],[100,101,102]]");
    EXPECT_EQ(std::string(yyjson::value(nested).write()), "[[97,98,99],[100,101,102]]");
    EXPECT_EQ(std::string(yyjson::array(nested).write()), "[[97,98,99],[100,101,102]]");
    expect_direct_write(nested);
    char deeper[2][1][2] = {{{'a', 'b'}}, {{'c', 'd'}}};
    EXPECT_EQ(std::string(yyjson::write(deeper)), "[[[97,98]],[[99,100]]]");
    expect_direct_write(deeper);
    const char names[][6] = {"alpha", "beta"};
    EXPECT_EQ(std::string(yyjson::write(names)), "[[97,108,112,104,97,0],[98,101,116,97,0,0]]");

    const std::array<char[2], 2> elements = {{{'x', 'y'}, {'z', 'w'}}};
    EXPECT_EQ(std::string(yyjson::write(elements)), "[[120,121],[122,119]]");
    EXPECT_EQ(std::string(yyjson::value(elements).write()), "[[120,121],[122,119]]");
    expect_direct_write(elements);
    EXPECT_EQ(std::string(yyjson::write(std::span(elements))), "[[120,121],[122,119]]");
    EXPECT_EQ(std::string(yyjson::value(std::span(deeper)).write()), "[[[97,98]],[[99,100]]]");
    const auto vectors = std::vector<std::array<char, 2>>{{'a', 'b'}};
    EXPECT_EQ(std::string(yyjson::write(vectors)), "[[97,98]]");
    expect_direct_write(vectors);

    char unterminated[3] = {'x', 'y', 'z'};
    const char early[5] = {'a', 'b', '\0', 'c', '\0'};
    EXPECT_EQ(std::string(yyjson::write("abc")), R"("abc")");
    EXPECT_EQ(std::string(yyjson::value("abc").write()), R"("abc")");
    EXPECT_EQ(std::string(yyjson::write(unterminated)), R"("xyz")");
    EXPECT_EQ(std::string(yyjson::value(unterminated).write()), R"("xyz")");
    EXPECT_EQ(std::string(yyjson::value(unterminated, yyjson::copy_string).write()), R"("xyz")");
    EXPECT_EQ(std::string(yyjson::write(early)), R"("ab")");
    EXPECT_EQ(std::string(yyjson::value(early).write()), R"("ab")");
    expect_direct_write(unterminated);
    expect_direct_write(early);
    EXPECT_EQ(std::string(yyjson::write(std::forward_as_tuple(unterminated, 1))), R"(["xyz",1])");
    EXPECT_EQ(std::string(yyjson::value(std::forward_as_tuple(unterminated, 1)).write()), R"(["xyz",1])");
    EXPECT_EQ(std::string(yyjson::array{unterminated, early}.write()), R"(["xyz","ab"])");

    EXPECT_EQ(std::string(yyjson::object{{unterminated, 1}, {early, 2}, {"k", "v"}}.write()),
              R"({"xyz":1,"ab":2,"k":"v"})");
    auto obj = yyjson::object();
    obj.emplace(unterminated, unterminated);
    obj.emplace(early, early, yyjson::copy_string);
    EXPECT_EQ(std::string(obj.write()), R"({"xyz":"xyz","ab":"ab"})");
    EXPECT_EQ(std::string(yyjson::write(EmittedCharArrays())), R"({"ky":"val","a":["val"]})");
    EXPECT_EQ(std::string(yyjson::value(EmittedCharArrays()).write()), R"({"ky":"val","a":["val"]})");
    expect_direct_write(EmittedCharArrays());

    // the other character types are integers anywhere
    const signed char s[2] = {1, -2};
    const unsigned char u[2][1] = {{3}, {4}};
    const char8_t u8[2] = {u8'a', u8'b'};
    EXPECT_EQ(std::string(yyjson::write(s)), "[1,-2]");
    EXPECT_EQ(std::string(yyjson::write(u)), "[[3],[4]]");
    EXPECT_EQ(std::string(yyjson::write(u8)), "[97,98]");
    expect_direct_write(s);
    expect_direct_write(u);
    expect_direct_write(u8);
}

// Structs whose fields cannot all be assigned, which are written but cannot be read
struct ConstField
{
    const int a;
    int b;
};
struct ConstFieldInit
{
    const int a = 1;
    int b = 2;
};
struct ConstArrayField
{
    const int a[2] = {1, 2};
    int b = 3;
};
struct NestedConstField
{
    ConstFieldInit c;
    int d = 4;
};
struct VisitableConstField
{
    const int a = 5;
    int b = 6;
};
VISITABLE_STRUCT(VisitableConstField, a, b);
// a caster of the user reads it anyway, though not as a field, which cannot be assigned
struct UserConstField
{
    const int a = 0;
    int b = 0;
};
template <>
struct yyjson::caster<UserConstField>
{
    template <typename Json>
    static UserConstField from_json(const Json& json)
    {
        return UserConstField{0, yyjson::cast<int>(json)};
    }
};
struct UserConstFieldField
{
    UserConstField u;
};
#if FIELD_RFL_STD_REFLECTION
// only the C++26 backend of field-reflection reflects references and volatile fields
inline int referenced_int = 7;
struct RefField
{
    int& r = referenced_int;
    int b = 8;
};
struct RvalueRefField
{
    int&& r = std::move(referenced_int);
    int b = 10;
};
struct VolatileField
{
    volatile int a = 0;
    int b = 0;
};
#endif

namespace
{
    template <typename T>
    concept readable_in_any_way =
        yyjson::castable<yyjson::reader::const_value_ref, T> || yyjson::castable<yyjson::value, T> ||
        yyjson::readable<T> || requires(std::string_view json, T& t) { yyjson::read<T>(json); } ||
        requires(std::string_view json, T& t) { yyjson::read_into(json, t); } ||
        requires(std::string_view json, T& t) { yyjson::merge_into(json, t); };
}

TEST(Reflection, UnassignableFields)
{
    static_assert(!readable_in_any_way<ConstField>);
    static_assert(!readable_in_any_way<ConstFieldInit>);
    static_assert(!readable_in_any_way<ConstArrayField>);
    static_assert(!readable_in_any_way<NestedConstField>);
    static_assert(!readable_in_any_way<VisitableConstField>);
    static_assert(!readable_in_any_way<std::vector<ConstFieldInit>>);
    static_assert(!readable_in_any_way<std::map<std::string, ConstFieldInit>>);
    static_assert(!readable_in_any_way<std::optional<ConstFieldInit>>);
    static_assert(!readable_in_any_way<std::shared_ptr<ConstFieldInit>>);
    static_assert(!readable_in_any_way<std::variant<ConstFieldInit, int>>);
    static_assert(!readable_in_any_way<std::tuple<ConstFieldInit, int>>);
    static_assert(!readable_in_any_way<UserConstFieldField>);

    EXPECT_EQ(yyjson::read<UserConstField>("5").b, 5);
    EXPECT_EQ(yyjson::read<std::vector<UserConstField>>("[5]").at(0).b, 5);

    EXPECT_EQ(std::string_view(yyjson::write(ConstField{1, 2})), R"({"a":1,"b":2})");
    EXPECT_EQ(std::string_view(yyjson::write(ConstArrayField())), R"({"a":[1,2],"b":3})");
    EXPECT_EQ(std::string_view(yyjson::write(NestedConstField())), R"({"c":{"a":1,"b":2},"d":4})");
    EXPECT_EQ(std::string_view(yyjson::write(VisitableConstField())), R"({"a":5,"b":6})");
    EXPECT_EQ(yyjson::value(NestedConstField()).write(), R"({"c":{"a":1,"b":2},"d":4})");
    expect_direct_write(ConstField{1, 2});
    expect_direct_write(NestedConstField());
    expect_direct_write(std::vector<ConstFieldInit>(2));
    expect_direct_write(VisitableConstField());

#if FIELD_RFL_STD_REFLECTION
    static_assert(!readable_in_any_way<RefField>);
    static_assert(!readable_in_any_way<RvalueRefField>);
    static_assert(!readable_in_any_way<std::vector<RefField>>);
    EXPECT_EQ(std::string_view(yyjson::write(RefField())), R"({"r":7,"b":8})");
    EXPECT_EQ(std::string_view(yyjson::write(RvalueRefField())), R"({"r":7,"b":10})");
    expect_direct_write(RefField());

    const auto v = yyjson::read<VolatileField>(R"({"a":11,"b":12})");
    EXPECT_EQ(static_cast<int>(v.a), 11);
    EXPECT_EQ(v.b, 12);
    auto into = VolatileField();
    yyjson::read_into(R"({"a":13})", into);
    EXPECT_EQ(static_cast<int>(into.a), 13);
#endif
}

enum Unscoped : std::uint8_t
{
    u_zero,
    u_one,
    u_max = 128
};
// `big` is out of the range of values that the C++20 backend of field-reflection searches for names
enum class Wide : std::int64_t
{
    minus = -5,
    zero = 0,
    big = 1000
};
enum class Aliased
{
    first = 1,
    second = 1,
    other = 2
};
enum class Flags
{
    a = 1,
    b = 2
};
enum class Ranged
{
    low = 0,
    high = 1000
};
template <>
struct field_reflection::enum_range<Ranged>
{
    static constexpr int min = 0;
    static constexpr int max = 1000;
};
// names longer than an integer, and names that `WriteFlag::EscapeUnicode` escapes
enum class LongNames
{
    a_name_longer_than_twenty_characters,
    b
};
enum class Kana
{
    あ,
    い
};
// a value without a name is the integer of the underlying type
template <>
inline constexpr auto yyjson::annotations<Unscoped> = yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<Wide> = yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<Flags> = yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<Kana> = yyjson::annotate(yyjson::enum_format::name_or_integer);

struct EnumFields
{
    Color c = Color::red;
    Unscoped u = u_zero;
    Wide w = Wide::zero;
    Flags f = Flags::a;
    std::vector<Color> v;
    std::optional<Color> o;
    std::map<std::string, Color> m;
    Color arr[2] = {};
    LongNames l = LongNames::b;
};

struct VisitableEnumFields
{
    Color c = Color::red;
    std::vector<Wide> v;
};
VISITABLE_STRUCT(VisitableEnumFields, c, v);

// enumerations with casters of the user, which take precedence over the names
enum class UserColor
{
    red,
    green
};
template <>
struct yyjson::caster<UserColor>
{
    static UserColor from_json(std::string_view s)
    {
        if (s == "R") return UserColor::red;
        if (s == "G") return UserColor::green;
        throw yyjson::bad_cast("unknown color");
    }
    template <typename... Ts>
    static std::string_view to_json(UserColor c, Ts...)
    {
        return c == UserColor::red ? "R" : "G";
    }
};
enum class UserNumber
{
    one = 1
};
template <>
struct yyjson::caster<UserNumber>
{
    template <typename Json>
    static UserNumber from_json(const Json& json)
    {
        return static_cast<UserNumber>(yyjson::cast<int>(json) - 100);
    }
    static void to_json(yyjson::writer::value_ref& v, UserNumber n) { v = static_cast<int>(n) + 100; }
};

TEST(Reflection, Enums)
{
    using yyjson::bad_cast;
    constexpr auto std_reflection = bool(FIELD_RFL_STD_REFLECTION);

    // the root, and the values written for each kind of value of an enumeration
    EXPECT_EQ(std::string(yyjson::write(Color::green)), R"("green")");
    EXPECT_EQ(std::string(yyjson::value(Color::green).write()), R"("green")");
    EXPECT_EQ(yyjson::read(R"("blue")").cast<Color>(), Color::blue);
    EXPECT_EQ(yyjson::read<Color>(R"("blue")"), Color::blue);
    EXPECT_EQ(written(std::vector<Unscoped>{u_zero, u_max, static_cast<Unscoped>(255)}), R"(["u_zero","u_max",255])");
    EXPECT_EQ(written(std::vector<Wide>{Wide::minus, static_cast<Wide>(std::numeric_limits<std::int64_t>::min()),
                                        Wide::big}),
              std_reflection ? R"(["minus",-9223372036854775808,"big"])" : R"(["minus",-9223372036854775808,1000])");
    EXPECT_EQ(written(std::vector<Ranged>{Ranged::low, Ranged::high}), R"(["low","high"])");
    // a combination of flags has no name, and an alias is written by the name declared first
    EXPECT_EQ(written(std::vector<Flags>{Flags::a, static_cast<Flags>(3)}), R"(["a",3])");
    EXPECT_EQ(written(std::vector<Aliased>{Aliased::second, Aliased::other}), R"(["first","other"])");
    EXPECT_EQ(yyjson::read<std::vector<Flags>>("[3]")[0], static_cast<Flags>(3));
    expect_direct_write(std::vector<Unscoped>{u_zero, u_max, static_cast<Unscoped>(255)});
    expect_direct_write(std::vector<Wide>{Wide::minus, Wide::big, static_cast<Wide>(7)});
    expect_direct_write(std::vector<Flags>{Flags::b, static_cast<Flags>(3)});
    expect_direct_write(std::vector<LongNames>{LongNames::a_name_longer_than_twenty_characters, LongNames::b});
    expect_direct_write(std::vector<Kana>{Kana::あ, Kana::い, static_cast<Kana>(5)});
    EXPECT_EQ(yyjson::read<std::vector<Kana>>(written(std::vector<Kana>{Kana::い, Kana::あ})),
              (std::vector<Kana>{Kana::い, Kana::あ}));

    // a struct, with the enumerations in the containers that hold other values
    auto fields = EnumFields{Color::blue,
                             u_one,
                             Wide::minus,
                             static_cast<Flags>(3),
                             {Color::green, Color::red},
                             Color::red,
                             {{"x", Color::blue}},
                             {Color::green, Color::blue},
                             LongNames::a_name_longer_than_twenty_characters};
    const auto* fields_json = R"({"c":"blue","u":"u_one","w":"minus","f":3,"v":["green","red"],"o":"red",)"
                              R"("m":{"x":"blue"},"arr":["green","blue"],"l":"a_name_longer_than_twenty_characters"})";
    EXPECT_EQ(written(fields), fields_json);
    EXPECT_EQ(std::string(yyjson::value(fields).write()), fields_json);
    EXPECT_EQ(std::string(yyjson::object(fields).write()), fields_json);
    expect_direct_write(fields);
    expect_direct_write(std::vector<EnumFields>{fields, EnumFields()});
    EXPECT_EQ(written(yyjson::read(fields_json).cast<EnumFields>()), fields_json);
    EXPECT_EQ(written(yyjson::read<EnumFields>(fields_json)), fields_json);
    EXPECT_TRUE(expect_direct_read<EnumFields>(fields_json));
    EXPECT_TRUE(expect_direct_read<std::vector<EnumFields>>(std::format("[{},{{}}]", fields_json)));
    const auto visitable = VisitableEnumFields{Color::green, {Wide::zero, static_cast<Wide>(3)}};
    EXPECT_EQ(written(visitable), R"({"c":"green","v":["zero",3]})");
    expect_direct_write(visitable);
    EXPECT_TRUE(expect_direct_read<VisitableEnumFields>(R"({"c":"blue","v":["minus",-7]})"));

    // a name, or with `enum_format::name_or_integer` an integer that the underlying type holds, which is read as
    // that type reads it from any number
    EXPECT_EQ(written(yyjson::read<EnumFields>(R"({"u":128,"w":-9223372036854775808,"f":2e0,"v":["blue"]})")),
              R"({"c":"red","u":"u_max","w":-9223372036854775808,"f":"b","v":["blue"],"m":{},)"
              R"("arr":["red","red"],"l":"b"})");
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"c":1})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"c":1})").cast<EnumFields>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Color>>("[0.0]"), bad_cast);
    for (const auto* json :
         {R"({"c":"green"})", R"({"c":"gr\u0065en"})", R"({"c":2})", R"({"c":-3})", R"({"c":1.5})", R"({"u":255})",
          R"({"u":"u_max"})", R"({"w":"big"})", R"({"w":1000})", R"({"w":18446744073709551615})", R"({"c":"Green"})",
          R"({"c":"purple"})", R"({"c":""})", R"({"c":true})", R"({"c":null})", R"({"c":[]})", R"({"c":{}})",
          R"({"u":256})", R"({"u":-1})", R"({"u":-0.5})", R"({"f":2.9})", R"({"u":256.0})", R"({"c":1e10})",
          R"({"f":"a"})", R"({"v":["red",3,"x"]})", R"({"o":null})", R"({"o":"blue"})", R"({"m":{"y":"red","z":0}})",
          R"({"arr":["blue"]})", R"({"arr":["blue",1,2]})"})
    {
        expect_direct_read<EnumFields>(json);
        expect_read_into(fields, json);
        expect_merge_into(fields, json);
    }
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"c":"purple"})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"c":"purple"})").cast<EnumFields>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"c":"Green"})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"c":true})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"c":null})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"u":256})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"u":-1})").cast<EnumFields>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Color>>(R"([[]])"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"("purple")").cast<Color>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<EnumFields>(R"({"u":-0.5})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"f":2.9})").cast<EnumFields>(), bad_cast);
    EXPECT_EQ(yyjson::read<EnumFields>(R"({"u":-0.0})").u, u_zero);
    auto t = fields;
    yyjson::read_into(R"({"c":"green","v":["red"]})", t);
    EXPECT_EQ(written(t), R"({"c":"green","u":"u_zero","w":"zero","f":"a","v":["red"],"m":{},"arr":["red","red"],)"
                          R"("l":"b"})");
    t = fields;
    yyjson::merge_into(R"({"c":"green","o":null,"m":{"y":"red"}})", t);
    EXPECT_EQ(written(t),
              R"({"c":"green","u":"u_one","w":"minus","f":3,"v":["green","red"],)"
              R"("m":{"x":"blue","y":"red"},"arr":["green","blue"],"l":"a_name_longer_than_twenty_characters"})");
    EXPECT_THROW(yyjson::merge_into(R"({"c":"purple"})", t), bad_cast);

    // a name out of the range that the C++20 backend searches, and an alias, which is read by neither backend
    EXPECT_EQ(yyjson::read<std::vector<Ranged>>(R"(["high","low"])"), (std::vector<Ranged>{Ranged::high, Ranged::low}));
    EXPECT_EQ(yyjson::read<std::vector<Aliased>>(R"(["first","other"])"),
              (std::vector<Aliased>{Aliased::first, Aliased::other}));
    EXPECT_THROW((void)yyjson::read<std::vector<Aliased>>(R"(["second"])"), bad_cast);
    if constexpr (std_reflection)
        EXPECT_EQ(yyjson::read<std::vector<Wide>>(R"(["big"])")[0], Wide::big);
    else
        EXPECT_THROW((void)yyjson::read<std::vector<Wide>>(R"(["big"])"), bad_cast);

    // a variant takes the first alternative that accepts the kind of the value, which for an enumeration is a
    // string, and with `enum_format::name_or_integer` a number as well, without and with a document
    using int_or_color = std::variant<int, Color>;
    using color_or_int = std::variant<Color, int>;
    using string_or_color = std::variant<std::string, Color>;
    using string_or_unscoped = std::variant<std::string, Unscoped>;
    EXPECT_EQ(yyjson::read<std::vector<int_or_color>>(R"([1,"red"])"),
              (std::vector<int_or_color>{1, Color::red}));
    EXPECT_EQ(yyjson::read<std::vector<color_or_int>>(R"([5,"red"])"), (std::vector<color_or_int>{5, Color::red}));
    EXPECT_EQ(yyjson::read(R"([5,"red"])").cast<std::vector<color_or_int>>(),
              (std::vector<color_or_int>{5, Color::red}));
    EXPECT_THROW((void)yyjson::read<std::vector<string_or_color>>("[1]"), bad_cast);
    EXPECT_EQ(yyjson::read<std::vector<string_or_unscoped>>(R"([1,"u_one"])"),
              (std::vector<string_or_unscoped>{u_one, "u_one"}));
    EXPECT_EQ(yyjson::read(R"([1,"u_one"])").cast<std::vector<string_or_unscoped>>(),
              (std::vector<string_or_unscoped>{u_one, "u_one"}));
    EXPECT_TRUE(expect_direct_read<std::vector<color_or_int>>(R"([5,"red"])"));
    expect_direct_read<std::vector<int_or_color>>(R"([1,"red",1.5,"purple"])");
    EXPECT_TRUE(expect_direct_read<std::vector<int_or_color>>(R"([1,"red"])"));
    EXPECT_TRUE((expect_direct_read<std::vector<std::variant<Color, std::string>>>(R"(["red","purple"])")));
    expect_direct_read<std::vector<std::variant<std::monostate, Unscoped>>>(R"([null,256])");
    EXPECT_EQ(written(std::vector<int_or_color>{1, Color::red}), R"([1,"red"])");
    expect_direct_write(std::vector<string_or_color>{"x", Color::blue});
    expect_direct_write(std::vector<string_or_unscoped>{"x", u_one, static_cast<Unscoped>(9)});

    // `std::byte` has no enumerators, and is an integer from 0 to 255
    EXPECT_EQ(written(std::vector<std::byte>{std::byte{0}, std::byte{255}}), "[0,255]");
    EXPECT_EQ(yyjson::read<std::vector<std::byte>>("[0,255]"), (std::vector<std::byte>{std::byte{0}, std::byte{255}}));
    expect_direct_write(std::vector<std::byte>{std::byte{7}});
    expect_direct_read<std::vector<std::byte>>("[256]");
    expect_direct_read<std::vector<std::byte>>(R"(["a"])");

    // a caster of the user takes precedence in both APIs
    const auto users = std::vector<UserColor>{UserColor::green, UserColor::red};
    EXPECT_EQ(written(users), R"(["G","R"])");
    EXPECT_EQ(std::string(yyjson::value(users).write()), R"(["G","R"])");
    expect_direct_write(users);
    EXPECT_EQ(yyjson::read<std::vector<UserColor>>(R"(["R","G"])"),
              (std::vector<UserColor>{UserColor::red, UserColor::green}));
    EXPECT_TRUE(expect_direct_read<std::vector<UserColor>>(R"(["R","G"])"));
    EXPECT_THROW((void)yyjson::read<std::vector<UserColor>>(R"(["red"])"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"([0])").cast<std::vector<UserColor>>(), bad_cast);
    EXPECT_EQ(written(std::vector<UserNumber>{UserNumber::one}), "[101]");
    expect_direct_write(std::vector<UserNumber>{UserNumber::one});
    EXPECT_EQ(yyjson::read<std::vector<UserNumber>>("[101]"), std::vector<UserNumber>{UserNumber::one});
    EXPECT_THROW((void)yyjson::read<std::vector<UserNumber>>(R"(["one"])"), bad_cast);
}

struct EnumKeyFields
{
    std::map<Color, int> m;
    std::map<Color, std::vector<int>> nested;
    std::optional<std::map<Unscoped, std::string>> opt;
};

TEST(Reflection, EnumKeys)
{
    using yyjson::bad_cast;
    constexpr auto std_reflection = bool(FIELD_RFL_STD_REFLECTION);
    using colors = std::map<Color, int>;
    using unordered_colors = std::unordered_map<Color, int>;
    using multi_colors = std::multimap<Color, int>;
    using wides = std::map<Wide, int>;
    using bytes_map = std::map<Unscoped, int>;
    using flag_map = std::map<Flags, int>;
    using kanas = std::map<Kana, int>;
    using user_pairs = std::vector<std::pair<UserColor, int>>;

    // a map with enumeration keys is an object of the names, in both APIs
    const auto m = colors{{Color::red, 1}, {Color::blue, 3}};
    EXPECT_EQ(written(m), R"({"red":1,"blue":3})");
    EXPECT_EQ(std::string(yyjson::value(m).write()), R"({"red":1,"blue":3})");
    EXPECT_EQ(std::string(yyjson::object(m).write()), R"({"red":1,"blue":3})");
    EXPECT_EQ(yyjson::read(R"({"red":1,"blue":3})").cast<colors>(), m);
    EXPECT_EQ(yyjson::read<colors>(R"({"red":1,"blue":3})"), m);
    EXPECT_TRUE(expect_direct_read<colors>(R"({"blue":3,"red":1})"));
    expect_direct_write(m);
    expect_direct_write(colors());
    // the other containers of key-value pairs that string keys make objects of, of which a sequence also reads an
    // array of pairs
    const auto um = unordered_colors{{Color::green, 2}};
    EXPECT_EQ(written(um), R"({"green":2})");
    EXPECT_EQ(yyjson::read<unordered_colors>(R"({"green":2})"), um);
    expect_direct_write(um);
    const auto mm = multi_colors{{Color::red, 1}, {Color::red, 2}};
    EXPECT_EQ(written(mm), R"({"red":1,"red":2})");
    EXPECT_TRUE(expect_direct_read<multi_colors>(R"({"red":1,"green":0,"red":2})"));
    expect_direct_write(mm);
    using pairs = std::vector<std::pair<Color, int>>;
    const auto ps = pairs{{Color::blue, 1}, {Color::red, 2}, {Color::blue, 3}};
    EXPECT_EQ(written(ps), R"({"blue":1,"red":2,"blue":3})");
    EXPECT_EQ(std::string(yyjson::array(ps).write()), R"([["blue",1],["red",2],["blue",3]])");
    expect_direct_write(ps);
    EXPECT_EQ(yyjson::read<pairs>(R"({"blue":1,"red":2,"blue":3})"), ps);
    EXPECT_EQ(yyjson::read<pairs>(R"([["blue",1],["red",2],["blue",3]])"), ps);
    EXPECT_TRUE(expect_direct_read<pairs>(R"({"blue":1,"red":2,"blue":3})"));
    expect_direct_read<pairs>(R"([["blue",1],[0,2]])");
    // a key that the object has twice keeps its first value, as a string key does
    EXPECT_EQ(yyjson::read<colors>(R"({"red":1,"red":2})"), (colors{{Color::red, 1}}));
    EXPECT_TRUE(expect_direct_read<colors>(R"({"red":1,"red":2})"));

    // with `enum_format::name_or_integer`, a value without a name is the integer of its underlying type, which is
    // read back
    const auto unnamed = wides{{Wide::minus, 1},
                               {static_cast<Wide>(std::numeric_limits<std::int64_t>::min()), 2},
                               {static_cast<Wide>(7), 3},
                               {Wide::big, 4}};
    const auto* unnamed_json = std_reflection ? R"({"-9223372036854775808":2,"minus":1,"7":3,"big":4})"
                                              : R"({"-9223372036854775808":2,"minus":1,"7":3,"1000":4})";
    EXPECT_EQ(written(unnamed), unnamed_json);
    EXPECT_EQ(std::string(yyjson::value(unnamed).write()), unnamed_json);
    EXPECT_EQ(yyjson::read<wides>(unnamed_json), unnamed);
    EXPECT_EQ(yyjson::read(unnamed_json).cast<wides>(), unnamed);
    expect_direct_write(unnamed);
    const auto flags = flag_map{{Flags::a, 1}, {static_cast<Flags>(3), 2}};
    EXPECT_EQ(written(flags), R"({"a":1,"3":2})");
    EXPECT_EQ(yyjson::read<flag_map>(R"({"a":1,"3":2})"), flags);
    const auto bytes = bytes_map{{u_max, 1}, {static_cast<Unscoped>(255), 2}};
    EXPECT_EQ(written(bytes), R"({"u_max":1,"255":2})");
    EXPECT_EQ(yyjson::read<bytes_map>(R"({"u_max":1,"255":2})"), bytes);
    // names that need escapes, which are escaped as the document escapes them
    const auto kana = kanas{{Kana::あ, 1}, {static_cast<Kana>(5), 2}};
    EXPECT_EQ(written(kana), R"({"あ":1,"5":2})");
    EXPECT_EQ(std::string(yyjson::write(kana, yyjson::WriteFlag::EscapeUnicode)), R"({"\u3042":1,"5":2})");
    expect_direct_write(kana);
    EXPECT_EQ(yyjson::read<kanas>(R"({"\u3042":1,"い":2})"), (kanas{{Kana::あ, 1}, {Kana::い, 2}}));
    expect_direct_write(std::map<LongNames, int>{{LongNames::a_name_longer_than_twenty_characters, 1}});
    EXPECT_TRUE(expect_direct_read<kanas>(R"({"\u3042":1,"い":2})"));

    // a key is a name as it is written, and nothing else
    for (const auto* json : {R"({"purple":1})", R"({"Red":1})", R"({"":1})", R"({"1.0":1})", R"({"+1":1})",
                             R"({"01":1})", R"({"-0":1})", R"({" 1":1})", R"({"1 ":1})", R"({"-":1})", R"({"1e0":1})",
                             R"({"0x1":1})", R"({"red":1,"x":2})", R"({"0":1})", R"({"7":1})"})
    {
        EXPECT_THROW((void)yyjson::read<colors>(json), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<colors>(), bad_cast) << json;
        expect_direct_read<colors>(json);
    }
    // or with `enum_format::name_or_integer` an integer as it is written
    for (const auto* json : {R"({"300":1})", R"({"256":1})", R"({"-1":1})", R"({"18446744073709551616":1})",
                             R"({"+1":1})", R"({"01":1})", R"({"1.0":1})"})
    {
        EXPECT_THROW((void)yyjson::read<bytes_map>(json), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<bytes_map>(), bad_cast) << json;
    }
    EXPECT_EQ(yyjson::read<flag_map>(R"({"1":1,"-1":2,"7":3})"),
              (flag_map{{Flags::a, 1}, {static_cast<Flags>(-1), 2}, {static_cast<Flags>(7), 3}}));
    EXPECT_EQ(yyjson::read<wides>(R"({"-9223372036854775808":1})").begin()->first,
              static_cast<Wide>(std::numeric_limits<std::int64_t>::min()));
    EXPECT_THROW((void)yyjson::read<wides>(R"({"9223372036854775808":1})"), bad_cast);

    // as fields, nested and optional, and read into and merged into
    auto fields = EnumKeyFields{{{Color::red, 1}, {Color::green, 2}},
                                {{Color::blue, {1, 2}}, {Color::green, {}}},
                                std::map<Unscoped, std::string>{{u_one, "one"}}};
    const auto* fields_json = R"({"m":{"red":1,"green":2},"nested":{"green":[],"blue":[1,2]},"opt":{"u_one":"one"}})";
    EXPECT_EQ(written(fields), fields_json);
    EXPECT_EQ(std::string(yyjson::value(fields).write()), fields_json);
    expect_direct_write(fields);
    EXPECT_EQ(written(yyjson::read(fields_json).cast<EnumKeyFields>()), fields_json);
    EXPECT_EQ(written(yyjson::read<EnumKeyFields>(fields_json)), fields_json);
    EXPECT_TRUE(expect_direct_read<EnumKeyFields>(fields_json));
    for (const auto* json :
         {R"({"m":{"green":5,"blue":6}})", R"({"m":{"blue":6,"red":7,"green":8}})", R"({"m":{"red":1,"red":2}})",
          R"({"m":{"green":null}})", R"({"nested":{"blue":[3],"green":null,"red":[4]}})", R"({"opt":null})",
          R"({"opt":{"u_one":null,"u_max":"max","7":"seven"}})", R"({"m":{"purple":1}})", R"({"opt":{"256":""}})"})
    {
        expect_read_into(fields, json);
        expect_merge_into(fields, json);
    }
    auto t = fields;
    const auto* first_node = &*t.m.find(Color::green);
    yyjson::read_into(R"({"m":{"green":5,"blue":6}})", t);
    EXPECT_EQ(t.m, (colors{{Color::green, 5}, {Color::blue, 6}}));
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(&*t.m.find(Color::green), first_node);
#else
    static_cast<void>(first_node);
#endif
    EXPECT_EQ(expect_merge_into(fields, R"({"m":{"green":null,"blue":3},"nested":{"blue":[5]},"opt":{"u_one":null}})"),
              R"({"m":{"red":1,"blue":3},"nested":{"green":[],"blue":[5]},"opt":{}})");
    EXPECT_EQ(expect_merge_into(flag_map{{Flags::a, 1}}, R"({"1":null,"3":3})"), R"({"3":3})");
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"purple":1}})", t), bad_cast);

    // a variant tells a map with enumeration keys by its first key, which it then reads without throwing
    using color_or_string = std::variant<colors, std::map<std::string, int>>;
    EXPECT_EQ(
        yyjson::read<std::vector<color_or_string>>(R"([{"red":1},{"purple":2},{}])"),
        (std::vector<color_or_string>{colors{{Color::red, 1}}, std::map<std::string, int>{{"purple", 2}}, colors()}));
    EXPECT_EQ(yyjson::read(R"([{"red":1},{"purple":2}])").cast<std::vector<color_or_string>>(),
              (std::vector<color_or_string>{colors{{Color::red, 1}}, std::map<std::string, int>{{"purple", 2}}}));
    EXPECT_TRUE(expect_direct_read<std::vector<color_or_string>>(R"([{"red":1},{"purple":2},{"0":3,"x":4}])"));
    static_assert(yyjson::detail::may_reject_nested<std::map<Color, ReadPoint>>());
    static_assert(!yyjson::detail::may_reject_nested<std::map<std::string, ReadPoint>>());
    for (const auto* json : {R"({"purple":1})", R"({"red":1})", R"({"green":1,"purple":2})", "{}"})
    {
        const auto doc = yyjson::read(json);
        const auto value = yyjson::reader::const_value_ref(doc);
        EXPECT_EQ(yyjson::detail::may_cast<colors>(value, yyjson::detail::json_kind_of(value)), json[2] != 'p') << json;
    }
    using pairs_or_int = std::variant<int, pairs>;
    EXPECT_TRUE(expect_direct_read<std::vector<pairs_or_int>>(R"([1,{"red":1},[["blue",2]]])"));

    // an enumeration with a caster of the user, which would have to give a string, is written as before as an
    // element of an array of pairs
    const auto users = std::map<UserColor, int>{{UserColor::red, 1}};
    EXPECT_EQ(written(users), R"([["R",1]])");
    EXPECT_EQ(std::string(yyjson::value(users).write()), R"([["R",1]])");
    EXPECT_EQ(yyjson::read<user_pairs>(R"([["G",2]])"), (user_pairs{{UserColor::green, 2}}));
    // and so are integer keys
    EXPECT_EQ(written(std::map<int, int>{{1, 2}}), "[[1,2]]");
}

// enumerators renamed, and an enumeration read and written as integers, by C++26 annotations or by
// `yyjson::annotations`
#if FIELD_RFL_ANNOTATIONS
enum class Status
{
    todo,
    in_progress[[= yyjson::rename("in-progress")]],
    done[[= yyjson::rename("完了")]]
};
enum class[[= yyjson::enum_format::integer]] Perm : unsigned
{
    read = 1,
    write = 2
};
#else
enum class Status
{
    todo,
    in_progress,
    done
};
enum class Perm : unsigned
{
    read = 1,
    write = 2
};
template <>
inline constexpr auto yyjson::annotations<Status> =
    yyjson::annotate(yyjson::enumerator("in_progress", yyjson::rename("in-progress")),
                     yyjson::enumerator("done", yyjson::rename("完了")));
template <>
inline constexpr auto yyjson::annotations<Perm> = yyjson::annotate(yyjson::enum_format::integer);
#endif
// a renamed enumerator that is the name of another
enum class Swapped
{
    a,
    b
};
template <>
inline constexpr auto yyjson::annotations<Swapped> =
    yyjson::annotate(yyjson::enumerator("a", yyjson::rename("b")), yyjson::enumerator("b", yyjson::rename("a")));

struct FormatFields
{
    Status s = Status::todo;
    Perm p = Perm::read;
    std::map<Status, int> m;
    std::map<Perm, int> pm;
    bool operator==(const FormatFields&) const = default;
};

TEST(Reflection, EnumFormats)
{
    using yyjson::bad_cast;
    static_assert(yyjson::detail::enum_format_of<Color> == yyjson::enum_format::name);
    static_assert(yyjson::detail::enum_format_of<Perm> == yyjson::enum_format::integer);
    static_assert(yyjson::detail::enum_format_of<const Unscoped> == yyjson::enum_format::name_or_integer);
    static_assert(yyjson::detail::enum_format_of<std::byte> == yyjson::enum_format::integer);

    // a renamed enumerator is read and written by its new name only
    const auto statuses = std::vector<Status>{Status::todo, Status::in_progress, Status::done};
    EXPECT_EQ(written(statuses), R"(["todo","in-progress","完了"])");
    EXPECT_EQ(std::string(yyjson::value(statuses).write()), R"(["todo","in-progress","完了"])");
    expect_direct_write(statuses);
    EXPECT_EQ(yyjson::read<std::vector<Status>>(R"(["todo","in-progress","完了"])"), statuses);
    EXPECT_EQ(yyjson::read(R"(["todo","in-progress","\u5b8c\u4e86"])").cast<std::vector<Status>>(), statuses);
    EXPECT_TRUE(expect_direct_read<std::vector<Status>>(R"(["in-progress","\u5b8c\u4e86"])"));
    for (const auto* json : {R"(["in_progress"])", R"(["done"])", "[1]"})
    {
        EXPECT_THROW((void)yyjson::read<std::vector<Status>>(json), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<std::vector<Status>>(), bad_cast) << json;
    }
    EXPECT_EQ(written(std::vector<Swapped>{Swapped::a, Swapped::b}), R"(["b","a"])");
    EXPECT_EQ(yyjson::read<std::vector<Swapped>>(R"(["b","a"])"), (std::vector<Swapped>{Swapped::a, Swapped::b}));
    EXPECT_EQ(yyjson::read(R"(["b"])").cast<std::vector<Swapped>>(), std::vector<Swapped>{Swapped::a});

    // `enum_format::integer` reads and writes integers only
    const auto perms = std::vector<Perm>{Perm::read, static_cast<Perm>(3)};
    EXPECT_EQ(written(perms), "[1,3]");
    EXPECT_EQ(std::string(yyjson::value(perms).write()), "[1,3]");
    expect_direct_write(perms);
    EXPECT_EQ(yyjson::read<std::vector<Perm>>("[1,3]"), perms);
    EXPECT_EQ(yyjson::read("[1,3]").cast<std::vector<Perm>>(), perms);
    for (const auto* json : {R"(["read"])", "[-1]", "[1.5]"})
    {
        EXPECT_THROW((void)yyjson::read<std::vector<Perm>>(json), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<std::vector<Perm>>(), bad_cast) << json;
    }
    using perm_or_string = std::variant<Perm, std::string>;
    EXPECT_EQ(yyjson::read<std::vector<perm_or_string>>(R"([2,"read"])"),
              (std::vector<perm_or_string>{Perm::write, "read"}));
    EXPECT_EQ(yyjson::read(R"([2,"read"])").cast<std::vector<perm_or_string>>(),
              (std::vector<perm_or_string>{Perm::write, "read"}));

    // as fields and keys
    const auto fields =
        FormatFields{Status::done, Perm::write, {{Status::in_progress, 1}}, {{static_cast<Perm>(4), 2}}};
    const auto* fields_json = R"({"s":"完了","p":2,"m":{"in-progress":1},"pm":{"4":2}})";
    EXPECT_EQ(written(fields), fields_json);
    EXPECT_EQ(std::string(yyjson::value(fields).write()), fields_json);
    expect_direct_write(fields);
    EXPECT_EQ(std::string(yyjson::write(fields, yyjson::WriteFlag::EscapeUnicode)),
              R"({"s":"\u5B8C\u4E86","p":2,"m":{"in-progress":1},"pm":{"4":2}})");
    EXPECT_EQ(yyjson::read<FormatFields>(fields_json), fields);
    EXPECT_EQ(yyjson::read(fields_json).cast<FormatFields>(), fields);
    EXPECT_TRUE(expect_direct_read<FormatFields>(fields_json));
    for (const auto* json : {R"({"m":{"in_progress":1}})", R"({"m":{"1":1}})", R"({"pm":{"read":1}})"})
    {
        EXPECT_THROW((void)yyjson::read<FormatFields>(json), bad_cast) << json;
        expect_direct_read<FormatFields>(json);
        expect_merge_into(fields, json);
    }

    // a value without a name is not written, and the message tells how to write it
    const auto unnamed = static_cast<Color>(5);
    const auto expect_unnamed = [](auto&& write) {
        try
        {
            write();
            ADD_FAILURE() << "no exception";
        }
        catch (const bad_cast& e)
        {
            EXPECT_TRUE(std::string_view(e.what()).find("Color") != std::string_view::npos) << e.what();
            EXPECT_TRUE(std::string_view(e.what()).find("5") != std::string_view::npos) << e.what();
            EXPECT_TRUE(std::string_view(e.what()).find("enum_format") != std::string_view::npos) << e.what();
        }
    };
    expect_unnamed([&] { (void)yyjson::write(unnamed); });
    expect_unnamed([&] { (void)yyjson::value(unnamed).write(); });
    expect_unnamed([&] { (void)yyjson::write(std::vector<Color>{Color::red, unnamed}); });
    expect_unnamed([&] { (void)yyjson::value(std::vector<Color>{Color::red, unnamed}).write(); });
    expect_unnamed([&] { (void)yyjson::write(std::map<Color, int>{{unnamed, 1}}); });
    expect_unnamed([&] { (void)yyjson::value(std::map<Color, int>{{unnamed, 1}}).write(); });
    expect_unnamed([&] { (void)yyjson::write(EnumKeyFields{{}, {{unnamed, {}}}, {}}); });
    expect_unnamed([&] { (void)yyjson::value(std::map<Color, yyjson::value>{{unnamed, yyjson::value(1)}}); });
    const auto one = yyjson::read("1");
    expect_unnamed([&] {
        (void)yyjson::value(
            std::map<Color, yyjson::reader::const_value_ref>{{unnamed, yyjson::reader::const_value_ref(one)}});
    });
    EXPECT_THROW((void)yyjson::write(std::vector<Status>{static_cast<Status>(7)}), bad_cast);
    EXPECT_THROW((void)yyjson::write(std::map<Status, int>{{static_cast<Status>(7), 1}}), bad_cast);
    expect_direct_write(std::vector<Status>{static_cast<Status>(7)});
    auto unnamed_fields = EnumFields();
    unnamed_fields.f = static_cast<Flags>(3);
    unnamed_fields.v = {unnamed};
    expect_direct_write(unnamed_fields);
}

struct PairMapFields
{
    std::map<int, int> m;
    std::optional<std::unordered_map<int, std::string>> u;
    std::vector<std::multimap<int, int>> v;
};

// A map whose keys are neither strings nor enumerations, which is written as an array of [key, value] pairs, is read
// back from one
TEST(Reflection, MapsFromArraysOfPairs)
{
    using yyjson::bad_cast;

    const auto ints = std::map<int, int>{{1, 2}, {3, 4}};
    EXPECT_EQ((yyjson::read<std::map<int, int>>(written(ints))), ints);
    EXPECT_EQ((yyjson::read(written(ints)).cast<std::map<int, int>>()), ints);
    EXPECT_TRUE((expect_direct_read<std::map<int, int>>("[[1,2],[3,4]]")));
    EXPECT_TRUE((expect_direct_read<std::unordered_map<int, std::string>>(R"([[1,"a"]])")));
    EXPECT_TRUE((expect_direct_read<std::multimap<int, int>>("[[1,2],[1,3],[0,4]]")));
    EXPECT_TRUE((expect_direct_read<std::map<std::pair<int, int>, int>>("[[[1,2],3],[[0,1],4]]")));
    EXPECT_TRUE((expect_direct_read<std::map<int, std::map<int, int>>>("[[1,[[2,3]]],[0,[]]]")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::map<int, double>>>("[[[1,1.5]],[]]")));
    EXPECT_EQ((yyjson::read<std::map<UserColor, int>>(R"([["G",2],["R",1]])")),
              (std::map<UserColor, int>{{UserColor::red, 1}, {UserColor::green, 2}}));

    // a key given twice keeps its first value, as a key of an object read into a map does
    EXPECT_EQ((yyjson::read<std::map<int, int>>("[[1,2],[1,3]]")), (std::map<int, int>{{1, 2}}));
    EXPECT_EQ((yyjson::read("[[1,2],[1,3]]").cast<std::map<int, int>>()), (std::map<int, int>{{1, 2}}));

    // neither an object nor an element that is not a pair of a key and a value
    for (const auto* json : {R"({"1":2})", "[1]", R"([["1",2]])", "[[1,2,3]]", "[[1,null]]"})
    {
        EXPECT_THROW(static_cast<void>(yyjson::read<std::map<int, int>>(json)), bad_cast) << json;
        EXPECT_THROW(static_cast<void>(yyjson::read(json).cast<std::map<int, int>>()), bad_cast) << json;
    }

    // as fields, read into and merged into, which replaces the map as it does an array
    const auto fields = PairMapFields{{{1, 2}}, std::unordered_map<int, std::string>{{3, "x"}}, {{{4, 5}, {4, 6}}}};
    const auto* fields_json = R"({"m":[[1,2]],"u":[[3,"x"]],"v":[[[4,5],[4,6]]]})";
    EXPECT_EQ(written(fields), fields_json);
    EXPECT_EQ(written(yyjson::read<PairMapFields>(fields_json)), fields_json);
    EXPECT_EQ(written(yyjson::read(fields_json).cast<PairMapFields>()), fields_json);
    EXPECT_TRUE(expect_direct_read<PairMapFields>(fields_json));
    for (const auto* json : {R"({"m":[[5,6]]})", R"({"m":[],"u":null})", R"({"v":[[],[[7,8]]]})", R"({"m":{}})"})
    {
        expect_read_into(fields, json);
        expect_merge_into(fields, json);
    }
    EXPECT_EQ(expect_merge_into(fields, R"({"m":[[5,6]]})"), R"({"m":[[5,6]],"u":[[3,"x"]],"v":[[[4,5],[4,6]]]})");
    EXPECT_TRUE(expect_read_into(std::map<int, int>{{1, 1}}, "[[2,5]]"));
}

// An aggregate with begin() and end() is written into an object, and is read from one
struct RangeAggregate
{
    std::vector<int> items;
    std::string name;
    [[nodiscard]] auto begin() const { return items.begin(); }
    [[nodiscard]] auto end() const { return items.end(); }
};
struct MutableRangeAggregate
{
    std::vector<int> items;
    [[nodiscard]] auto begin() { return items.begin(); }
    [[nodiscard]] auto end() { return items.end(); }
    [[nodiscard]] auto begin() const { return items.begin(); }
    [[nodiscard]] auto end() const { return items.end(); }
};
struct RangeAggregateFields
{
    RangeAggregate r;
    std::vector<MutableRangeAggregate> m;
    std::optional<RangeAggregate> o;
};

TEST(Reflection, RangeAggregates)
{
    using yyjson::bad_cast;
    static_assert(std::ranges::input_range<RangeAggregate> && std::ranges::input_range<MutableRangeAggregate>);

    const auto json = R"({"items":[1,2],"name":"x"})";
    EXPECT_EQ(written(RangeAggregate{{1, 2}, "x"}), json);
    EXPECT_EQ(written(yyjson::read<RangeAggregate>(json)), json);
    EXPECT_EQ(written(yyjson::read(json).cast<RangeAggregate>()), json);
    EXPECT_TRUE(expect_direct_read<RangeAggregate>(json));
    EXPECT_TRUE(expect_direct_read<MutableRangeAggregate>(R"({"items":[3]})"));
    for (const auto* text : {"[1,2]", "[]"})
    {
        EXPECT_THROW((void)yyjson::read<RangeAggregate>(text), bad_cast) << text;
        EXPECT_THROW((void)yyjson::read(text).cast<RangeAggregate>(), bad_cast) << text;
        EXPECT_THROW((void)yyjson::read<MutableRangeAggregate>(text), bad_cast) << text;
        EXPECT_THROW((void)yyjson::read(text).cast<MutableRangeAggregate>(), bad_cast) << text;
    }

    const auto fields = RangeAggregateFields{{{1}, "a"}, {{{2, 3}}}, RangeAggregate{{}, "b"}};
    const auto* fields_json = R"({"r":{"items":[1],"name":"a"},"m":[{"items":[2,3]}],"o":{"items":[],"name":"b"}})";
    EXPECT_EQ(written(fields), fields_json);
    EXPECT_EQ(std::string(yyjson::value(fields).write()), fields_json);
    EXPECT_TRUE(expect_direct_read<RangeAggregateFields>(fields_json));
    for (const auto* text : {R"({"r":{"name":"c"}})", R"({"m":[{},{"items":[4]}],"o":null})", R"({"o":{"items":[5]}})"})
    {
        expect_read_into(fields, text);
        expect_merge_into(fields, text);
    }
    EXPECT_EQ(expect_merge_into(fields, R"({"r":{"name":"c"}})"),
              R"({"r":{"items":[1],"name":"c"},"m":[{"items":[2,3]}],"o":{"items":[],"name":"b"}})");
}

// No JSON value is read into these types, so they are not castable rather than failing at run time
class PrivateFields
{
    int a_ = 0;

public:
    [[nodiscard]] int a() const { return a_; }
};
static_assert(!yyjson::castable<yyjson::reader::const_value_ref, PrivateFields>);
static_assert(!yyjson::castable<yyjson::reader::const_value_ref, std::mutex>);
static_assert(!yyjson::castable<yyjson::reader::const_value_ref, std::forward_list<int>>);
static_assert(!yyjson::castable<yyjson::value, std::forward_list<int>>);
// `std::any` would hold a `std::string_view` into the JSON of a string, and no JSON value is read into it
static_assert(!yyjson::castable<yyjson::reader::const_value_ref, std::any>);
static_assert(!yyjson::castable<yyjson::value, std::any>);
static_assert(!yyjson::castable<yyjson::reader::const_value_ref, std::optional<std::any>>);

// `yyjson::readable` and `yyjson::writable` of the types that are converted only one way or not at all
static_assert(!yyjson::readable<PrivateFields> && !yyjson::writable<PrivateFields>);
static_assert(!yyjson::readable<std::mutex> && !yyjson::writable<std::mutex>);
static_assert(!yyjson::readable<std::forward_list<int>> && yyjson::writable<std::forward_list<int>>);
static_assert(!yyjson::readable<std::any> && !yyjson::writable<std::any>);
static_assert(!yyjson::readable<std::vector<std::any>>);
// a type constructible from a JSON scalar reads it
static_assert(yyjson::readable<std::chrono::seconds> && !yyjson::writable<std::chrono::seconds>);
static_assert(yyjson::readable<std::function<void()>> && !yyjson::writable<std::function<void()>>);
static_assert(!yyjson::writable<std::unique_ptr<int[]>>);
static_assert(yyjson::readable<std::unique_ptr<int>> && yyjson::writable<std::unique_ptr<int>>);
static_assert(yyjson::readable<yyjson::value> && yyjson::writable<yyjson::value>);
static_assert(yyjson::writable<yyjson::reader::const_value_ref>);

// A `VISITABLE_STRUCT` is converted only when each of its members is, as a reflected struct is
struct VisitableDuration
{
    int a = 0;
    std::chrono::seconds d;
};
VISITABLE_STRUCT(VisitableDuration, a, d);
struct VisitableMutex
{
    int a = 0;
    std::mutex m;
};
VISITABLE_STRUCT(VisitableMutex, a, m);
static_assert(yyjson::readable<VisitableDuration> && !yyjson::writable<VisitableDuration>);
static_assert(!yyjson::readable<VisitableMutex> && !yyjson::writable<VisitableMutex>);
static_assert(!yyjson::castable<yyjson::value, VisitableMutex>);

// A type that cannot be converted is rejected by a deleted overload that names the field or element that cannot
// be, and a requires-expression of the call is still false.
struct BadInner
{
    int x = 0;
    std::unique_ptr<int[]> p;
};
struct BadOuter
{
    int a = 0;
    std::vector<BadInner> items;
};
struct ConstFieldCause
{
    const int a = 0;
};
// The name of a field in a cause, built as the library builds it: MSVC keeps `name<N>{"literal"}` as another
// template argument.
template <std::size_t N>
consteval auto cause_name(const char (&s)[N])
{
    return yyjson::detail::diagnostic_name<N - 1>(std::string_view(s, N - 1));
}
static_assert(std::same_as<
              yyjson::detail::write_cause<BadOuter>,
              yyjson::diagnostic::field<
                  BadOuter, cause_name("items"),
                  yyjson::diagnostic::element<
                      std::vector<BadInner>,
                      yyjson::diagnostic::field<BadInner, cause_name("p"),
                                                yyjson::diagnostic::not_writable<std::unique_ptr<int[]>>>>>>);
static_assert(std::same_as<yyjson::detail::read_cause<ConstFieldCause>,
                           yyjson::diagnostic::field<ConstFieldCause, cause_name("a"),
                                                     yyjson::diagnostic::not_readable<const int>>>);
static_assert(std::same_as<yyjson::detail::write_cause<VisitableDuration>,
                           yyjson::diagnostic::field<VisitableDuration, cause_name("d"),
                                                     yyjson::diagnostic::not_writable<std::chrono::seconds>>>);
static_assert(
    std::same_as<yyjson::detail::write_cause<std::map<std::string, std::optional<std::chrono::seconds>>>,
                 yyjson::diagnostic::element<std::map<std::string, std::optional<std::chrono::seconds>>,
                                             yyjson::diagnostic::element<std::optional<std::chrono::seconds>,
                                                                         yyjson::diagnostic::not_writable<
                                                                             std::chrono::seconds>>>>);
static_assert(std::same_as<yyjson::detail::write_cause<std::variant<int, std::chrono::seconds>>,
                           yyjson::diagnostic::element<std::variant<int, std::chrono::seconds>,
                                                       yyjson::diagnostic::not_writable<std::chrono::seconds>>>);
static_assert(std::same_as<yyjson::detail::read_cause<std::tuple<int, PrivateFields>>,
                           yyjson::diagnostic::element<std::tuple<int, PrivateFields>,
                                                       yyjson::diagnostic::not_readable<PrivateFields>>>);
static_assert(std::same_as<yyjson::detail::write_cause<PrivateFields>,
                           yyjson::diagnostic::not_writable<PrivateFields>>);
// a field renamed in JSON is named as declared
struct RenamedCause
{
    std::chrono::seconds duration_s;
};
template <>
inline constexpr auto yyjson::annotations<RenamedCause> =
    yyjson::annotate(yyjson::field("duration_s", yyjson::rename("duration")));
struct VisitableRenamedCause
{
    std::chrono::seconds duration_s;
};
VISITABLE_STRUCT(VisitableRenamedCause, (duration_s, "duration"));
static_assert(std::same_as<yyjson::detail::write_cause<RenamedCause>,
                           yyjson::diagnostic::field<RenamedCause, cause_name("duration_s"),
                                                     yyjson::diagnostic::not_writable<std::chrono::seconds>>>);
static_assert(
    std::same_as<yyjson::detail::write_cause<VisitableRenamedCause>,
                 yyjson::diagnostic::field<VisitableRenamedCause, cause_name("duration_s"),
                                           yyjson::diagnostic::not_writable<std::chrono::seconds>>>);
namespace
{
    template <typename T>
    concept writes_through_call = requires(const T& t, std::string& out) {
        yyjson::write(t);
        yyjson::write(t, out);
    };
    template <typename T>
    concept reads_through_call = requires(std::string_view json, T& t) { yyjson::read<T>(json); } ||
                                 requires(std::string_view json, T& t) { yyjson::read_into(json, t); } ||
                                 requires(std::string_view json, T& t) { yyjson::merge_into(json, t); };
}  // namespace
static_assert(!writes_through_call<BadOuter> && writes_through_call<ConstFieldCause>);
static_assert(!reads_through_call<ConstFieldCause> && !reads_through_call<PrivateFields>);
static_assert(reads_through_call<BadOuter>);

// `std::unique_ptr` is null or the value it points to, as `std::shared_ptr` is
struct UniqueFields
{
    int a = 0;
    std::unique_ptr<int> p;
    std::optional<std::unique_ptr<std::string>> o;
    std::vector<std::unique_ptr<int>> v;
    std::map<std::string, std::unique_ptr<double>> m;
};

TEST(Reflection, UniquePointers)
{
    using yyjson::bad_cast;
    using yyjson::read;
    static_assert(yyjson::readable<UniqueFields> && yyjson::writable<UniqueFields>);

    EXPECT_EQ(read("null").cast<std::unique_ptr<int>>(), nullptr);
    EXPECT_EQ(*read("3").cast<std::unique_ptr<int>>(), 3);
    EXPECT_EQ(*yyjson::read<std::unique_ptr<std::string>>(R"("s")"), "s");
    EXPECT_TRUE(yyjson::value(std::unique_ptr<int>()).is_null());
    EXPECT_EQ(written(std::make_unique<int>(2)), "2");
    EXPECT_EQ(std::string(yyjson::value(std::make_unique<std::string>("x")).write()), R"("x")");
    EXPECT_THROW((void)read(R"("s")").cast<std::unique_ptr<int>>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<std::unique_ptr<int>>(R"("s")"), bad_cast);
    EXPECT_TRUE(expect_direct_read<std::vector<std::unique_ptr<int>>>("[null,1]"));
    EXPECT_TRUE((expect_direct_read<std::vector<std::variant<std::unique_ptr<int>, std::string>>>(R"([null,1,"s"])")));
    // a pointer to an array reads only null, and the error names its type up to the end
    try
    {
        (void)yyjson::read<std::unique_ptr<int[]>>("1");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_TRUE(
            std::string_view(e.what()).ends_with("> is not constructible from JSON integer (line 1, column 1, pos 0)"))
            << e.what();
    }

    const auto* json = R"({"a":1,"p":2,"o":"s","v":[3,null],"m":{"x":1.5,"y":null}})";
    const auto fields = yyjson::read<UniqueFields>(json);
    EXPECT_EQ(written(fields), json);
    EXPECT_EQ(std::string(yyjson::value(fields).write()), json);
    EXPECT_EQ(written(read(json).cast<UniqueFields>()), json);
    EXPECT_TRUE(expect_direct_read<UniqueFields>(json));
    expect_direct_write(fields);
    // an optional pointer keeps null as an empty pointer
    EXPECT_EQ(written(yyjson::read<UniqueFields>(R"({"p":null,"o":null})")),
              R"({"a":0,"p":null,"o":null,"v":[],"m":{}})");

    auto into = yyjson::read<UniqueFields>(json);
    yyjson::read_into(R"({"p":null,"v":[4]})", into);
    EXPECT_EQ(written(into), R"({"a":0,"p":null,"v":[4],"m":{}})");
    yyjson::merge_into(R"({"p":5,"m":{"z":2.5}})", into);
    EXPECT_EQ(written(into), R"({"a":0,"p":5,"v":[4],"m":{"z":2.5}})");
    yyjson::merge_into(R"({"p":null,"m":{"z":null}})", into);
    EXPECT_EQ(written(into), R"({"a":0,"p":null,"v":[4],"m":{}})");
}

// Empty structs, which are read and written as empty JSON objects
struct EmptyFields
{
    int a = 1;
    EmptyStruct e;
    std::vector<EmptyStruct> v;
    std::optional<EmptyStruct> o;
    std::map<std::string, EmptyStruct> m;
};
struct NestedEmpty
{
    EmptyFields f;
    EmptyStruct e;
};

// an empty struct with a caster of the user, which takes precedence over the empty object
struct UserEmpty
{
};
template <>
struct yyjson::caster<UserEmpty>
{
    static UserEmpty from_json(std::string_view s)
    {
        if (s != "user") throw yyjson::bad_cast("not user");
        return {};
    }
    template <typename... Ts>
    static std::string_view to_json(UserEmpty, Ts...)
    {
        return "user";
    }
};
struct UserEmptyField
{
    UserEmpty u;
    EmptyStruct e;
};

TEST(Reflection, EmptyStructs)
{
    using yyjson::bad_cast;

    // the classes of the standard library and of this library are no objects
    static_assert(yyjson::detail::reflectable<EmptyStruct>);
    static_assert(!yyjson::detail::reflectable<std::monostate>);
    static_assert(!yyjson::detail::reflectable<std::less<>>);
    static_assert(!yyjson::detail::reflectable<std::true_type>);
    static_assert(!yyjson::detail::reflectable<yyjson::copy_string_t>);
    static_assert(!yyjson::detail::reflectable<yyjson::empty_object_t>);
    static_assert(!yyjson::detail::reflectable<decltype([] {})>);
    static_assert(std::is_assignable_v<yyjson::value, std::tuple<int, yyjson::copy_string_t>>);
    EXPECT_TRUE(yyjson::value(std::monostate()).is_null());
    EXPECT_EQ(written(std::monostate()), "null");

    // the root
    EXPECT_EQ(written(EmptyStruct()), "{}");
    EXPECT_EQ(std::string(yyjson::value(EmptyStruct()).write()), "{}");
    EXPECT_EQ(std::string(yyjson::object(EmptyStruct()).write()), "{}");
    EXPECT_EQ(std::string(yyjson::write(EmptyStruct(), yyjson::WriteFlag::Pretty)), "{}");
    expect_direct_write(EmptyStruct());
    EXPECT_EQ(yyjson::read<EmptyStruct>("{}"), EmptyStruct());
    EXPECT_EQ(yyjson::read("{}").cast<EmptyStruct>(), EmptyStruct());
    // the members of the object are ignored, as unknown keys are, and any other value throws `bad_cast`
    EXPECT_TRUE(expect_direct_read<EmptyStruct>(R"({"a":1,"b":[{}]})"));
    for (const auto* json : {"null", "true", "0", "-1", "1.5", R"("")", "[]", "[{}]"})
    {
        expect_direct_read<EmptyStruct>(json);
        EXPECT_THROW((void)yyjson::read<EmptyStruct>(json), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).cast<EmptyStruct>(), bad_cast) << json;
    }

    // fields
    auto full = EmptyFields();
    full.v.resize(2);
    full.o.emplace();
    full.m["k"] = {};
    const auto* full_json = R"({"a":1,"e":{},"v":[{},{}],"o":{},"m":{"k":{}}})";
    EXPECT_EQ(written(full), full_json);
    EXPECT_EQ(std::string(yyjson::value(full).write()), full_json);
    EXPECT_EQ(std::string(yyjson::object(full).write()), full_json);
    EXPECT_EQ(std::string(yyjson::write(full, yyjson::WriteFlag::Pretty)),
              "{\n    \"a\": 1,\n    \"e\": {},\n    \"v\": [\n        {},\n        {}\n    ],\n    \"o\": {},\n"
              "    \"m\": {\n        \"k\": {}\n    }\n}");
    expect_direct_write(full);
    expect_direct_write(NestedEmpty{full, {}});
    EXPECT_EQ(written(NestedEmpty()), R"({"f":{"a":1,"e":{},"v":[],"m":{}},"e":{}})");
    EXPECT_EQ(written(yyjson::read<EmptyFields>(full_json)), full_json);
    EXPECT_EQ(written(yyjson::read(full_json).cast<EmptyFields>()), full_json);
    EXPECT_TRUE(expect_direct_read<EmptyFields>(full_json));
    EXPECT_TRUE(expect_direct_read<NestedEmpty>(R"({"f":{"e":{"x":1},"v":[{}]},"e":{"y":[]}})"));
    EXPECT_TRUE(expect_direct_read<std::vector<EmptyStruct>>("[{},{}]"));
    EXPECT_TRUE(expect_direct_read<std::optional<EmptyStruct>>("{}"));
    expect_direct_read<std::optional<EmptyStruct>>("null");
    for (const auto* json : {R"({})", R"({"e":{"x":1}})", R"({"e":null})", R"({"e":1})", R"({"e":[]})",
                             R"({"v":[{},1]})", R"({"o":null})", R"({"o":"x"})", R"({"m":{"k":[]}})",
                             R"({"a":2,"e":{},"o":{}})"})
    {
        expect_direct_read<EmptyFields>(json);
        expect_read_into(full, json);
        expect_merge_into(full, json);
    }
    EXPECT_THROW((void)yyjson::read<EmptyFields>(R"({"e":1})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"e":[]})").cast<EmptyFields>(), bad_cast);

    // `read_into` and `merge_into` have nothing to put into an empty struct
    auto t = full;
    yyjson::read_into(R"({"e":{"x":1}})", t);
    EXPECT_EQ(written(t), R"({"a":1,"e":{},"v":[],"m":{}})");
    t = full;
    yyjson::merge_into(R"({"a":2,"e":{"x":1},"v":[{}]})", t);
    EXPECT_EQ(written(t), R"({"a":2,"e":{},"v":[{}],"o":{},"m":{"k":{}}})");
    EXPECT_THROW(yyjson::read_into(R"({"e":null})", t), bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"e":"x"})", t), bad_cast);

    // a caster of the user takes precedence in both APIs
    EXPECT_EQ(written(UserEmptyField()), R"({"u":"user","e":{}})");
    EXPECT_EQ(std::string(yyjson::value(UserEmptyField()).write()), R"({"u":"user","e":{}})");
    expect_direct_write(UserEmptyField());
    EXPECT_NO_THROW((void)yyjson::read<UserEmptyField>(R"({"u":"user","e":{}})"));
    EXPECT_THROW((void)yyjson::read<UserEmptyField>(R"({"u":{}})"), bad_cast);
    EXPECT_THROW((void)yyjson::read(R"({"u":{}})").cast<UserEmptyField>(), bad_cast);

    // an empty struct is an alternative of a variant that a JSON object selects, and `std::monostate` the one that
    // null selects
    using monostate_or_empty = std::variant<std::monostate, EmptyStruct, int>;
    EXPECT_EQ(yyjson::read<monostate_or_empty>("{}").index(), 1);
    EXPECT_EQ(yyjson::read("{}").cast<monostate_or_empty>().index(), 1);
    EXPECT_EQ(yyjson::read<monostate_or_empty>("null").index(), 0);
    EXPECT_EQ(yyjson::read<monostate_or_empty>("2").index(), 2);
    EXPECT_TRUE(expect_direct_read<std::vector<monostate_or_empty>>(R"([null,{},{"x":1},2])"));
    expect_direct_read<std::vector<monostate_or_empty>>(R"([null,{},[]])");
    EXPECT_EQ(written(std::vector<monostate_or_empty>{std::monostate(), EmptyStruct(), 2}), "[null,{},2]");
    expect_direct_write(std::vector<monostate_or_empty>{std::monostate(), EmptyStruct(), 2});
    // the first alternative that a JSON object can be read as is read, whatever members the object has
    using empty_or_map = std::variant<EmptyStruct, std::map<std::string, int>>;
    EXPECT_EQ(yyjson::read<empty_or_map>(R"({"a":1})").index(), 0);
    EXPECT_EQ(yyjson::read(R"({"a":1})").cast<empty_or_map>().index(), 0);
    using map_or_empty = std::variant<std::map<std::string, int>, EmptyStruct>;
    EXPECT_EQ(yyjson::read<map_or_empty>(R"({"a":1})").index(), 0);
    EXPECT_EQ(yyjson::read<map_or_empty>(R"({"a":"x"})").index(), 1);
    EXPECT_EQ(yyjson::read(R"({"a":"x"})").cast<map_or_empty>().index(), 1);
    EXPECT_TRUE(expect_direct_read<std::vector<map_or_empty>>(R"([{"a":1},{"a":"x"},{}])"));
}

// NOLINTEND
