#pragma once

#include <gtest/gtest.h>
#include <any>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <complex>
#include <deque>
#include <format>
#include <forward_list>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include "cpp_yyjson.hpp"

#if defined(CPPYYJSON_TEST_REQUIRE_STD_REFLECTION) && !FIELD_RFL_STD_REFLECTION
#error "the C++26 reflection backend of field-reflection is not enabled"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

// NOLINTBEGIN

namespace
{
    // what writing `t` gives, or the error it throws
    template <typename F>
    std::string written_or_error(F&& write)
    {
        try
        {
            return write();
        }
        catch (const yyjson::write_error& e)
        {
            return std::format("error {}: {}", static_cast<int>(e.code()), e.what());
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::format("bad_cast: {}", e.what());
        }
    }

    const auto direct_write_flags = std::vector<yyjson::WriteFlag>{
        yyjson::WriteFlag::NoFlag,
        yyjson::WriteFlag::Pretty,
#if YYJSON_VERSION_HEX >= 0x000700
        yyjson::WriteFlag::PrettyTwoSpaces,
#endif
        yyjson::WriteFlag::EscapeUnicode,
        yyjson::WriteFlag::EscapeSlashes,
        yyjson::WriteFlag::AllowInfAndNan,
        yyjson::WriteFlag::InfAndNanAsNull,
        yyjson::WriteFlag::AllowInvalidUnicode,
#if YYJSON_VERSION_HEX >= 0x000900
        yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::Pretty | yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::EscapeSlashes |
            yyjson::WriteFlag::NewlineAtEnd,
#endif
#if YYJSON_VERSION_HEX >= 0x000D00
        yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::LowercaseHex,
#endif
        yyjson::WriteFlag::AllowInvalidUnicode | yyjson::WriteFlag::EscapeUnicode,
#if YYJSON_VERSION_HEX >= 0x000B00
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(3)),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(2) | YYJSON_WRITE_PRETTY),
#endif
    };

    // `yyjson::write` gives the bytes, or the error, that writing a mutable document of `t` gives
    template <typename T>
    void expect_direct_write(const T& t)
    {
        for (const auto flag : direct_write_flags)
        {
            const auto expected = written_or_error([&] { return std::string(yyjson::value(t).write(flag)); });
            EXPECT_EQ(written_or_error([&] { return std::string(yyjson::write(t, flag)); }), expected)
                << "flag " << static_cast<int>(flag);
            auto out = std::string("previous");
            EXPECT_EQ(written_or_error([&] {
                          yyjson::write(t, out, flag);
                          return out;
                      }),
                      expected)
                << "flag " << static_cast<int>(flag);
        }
    }
}  // namespace


namespace
{
    // The elements of an unordered container in order.  Their order otherwise depends on the buckets, which
    // differ with libc++ between the cast of a document, which reserves the elements first, and the reader
    // without a document, which does not know their number.
    template <typename T>
    decltype(auto) in_order(const T& t)
    {
        if constexpr (requires { typename T::hasher; typename T::mapped_type; })
            return std::multimap<typename T::key_type, typename T::mapped_type>(t.begin(), t.end());
        else if constexpr (requires { typename T::hasher; })
            return std::multiset<typename T::value_type>(t.begin(), t.end());
        else
            return t;
    }

    // what reading gives, written back, or the error it throws
    template <typename F>
    std::string read_or_error(F&& read)
    {
        try
        {
            return std::string(yyjson::write(in_order(read()), yyjson::WriteFlag::AllowInfAndNan |
                                                                   yyjson::WriteFlag::AllowInvalidUnicode));
        }
        catch (const yyjson::read_error& e)
        {
            return std::format("read error {}: {}", static_cast<int>(e.code()), e.what());
        }
        catch (const yyjson::bad_cast& e)
        {
            // a document cast knows no text, so the position that `yyjson::read<T>` adds is left out
            auto what = std::string_view(e.what());
            if (const auto pos = e.position())
            {
                const auto suffix = std::format(" (line {}, column {}, pos {})", pos->line, pos->column, pos->pos);
                EXPECT_TRUE(what.ends_with(suffix)) << what;
                what.remove_suffix(std::min(suffix.size(), what.size()));
            }
            return std::format("bad cast: {}", what);
        }
    }

    // what the reader without a document gives, or nothing when it fails, which leaves the text to the
    // document
#if YYJSON_VERSION_HEX >= 0x000C00
    constexpr auto reads_directly = true;
#else
    // `yyjson::read<T>` casts a document before yyjson 0.12
    constexpr auto reads_directly = false;
#endif

#if YYJSON_VERSION_HEX >= 0x000C00
    // Calls `f` with the reader without a document of the root of `json`, as `yyjson::read<T>` makes it, and
    // returns what `f` returns, or nothing when that reader does not read the text or fails.
    template <typename F>
    std::optional<std::string> read_root_directly(std::string_view json, yyjson::ReadFlag flag, F&& f)
    {
        using yyjson::reader::detail::direct_reader;
        const char* first;
        const char* last;
        const auto root = direct_reader::find_root(json, flag, first, last);
        if (root == direct_reader::root_kind::none) return std::nullopt;
        try
        {
            auto reader = direct_reader(root, first, last, flag);
            return f(reader);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
#endif

    template <typename T>
    std::optional<std::string> read_directly(std::string_view json, yyjson::ReadFlag flag)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        return read_root_directly(json, flag, [](auto& reader) {
            const auto t = reader.template read_root<T>();
            return read_or_error([&]() -> const T& { return t; });
        });
#else
        static_cast<void>(json);
        static_cast<void>(flag);
        return std::nullopt;
#endif
    }

    // `yyjson::read<T>` gives what casting a document gives, and so does the reader without a document when it
    // reads the text; returns whether it did
    template <typename T>
    bool expect_direct_read(std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
    {
        const auto expected = read_or_error([&] { return yyjson::read(json, flag).template cast<T>(); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, flag); }), expected) << json;
        const auto direct = read_directly<T>(json, flag);
        if (direct)
        {
            EXPECT_EQ(*direct, expected) << json;
        }
        return direct.has_value() || !reads_directly;
    }
}  // namespace

namespace
{
    // `yyjson::read_into` gives what `yyjson::read<T>` gives, whatever `t` holds before, and so does the reader
    // without a document; returns whether that reader read the text
    template <typename T>
    bool expect_read_into(T t, std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
    {
        const auto expected = read_or_error([&] { return yyjson::read<T>(json, flag); });
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto direct = read_root_directly(json, flag, [&](auto& reader) {
            auto u = t;
            reader.read_root_into(u);
            return read_or_error([&] { return u; });
        });
        if (direct)
        {
            EXPECT_EQ(*direct, expected) << json;
        }
#else
        const auto direct = std::optional<std::string>();
#endif
        EXPECT_EQ(read_or_error([&] {
                      yyjson::read_into(json, t, flag);
                      return t;
                  }),
                  expected)
            << json;
        return direct.has_value() || !reads_directly;
    }
}  // namespace

namespace
{
    template <typename T>
    std::string merged(T t, std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
    {
        return read_or_error([&] {
            yyjson::merge_into(json, t, flag);
            return t;
        });
    }

    // `yyjson::merge_into` gives the same result with and without a document, and merging the same text again
    // changes nothing; returns the result
    template <typename T>
    std::string expect_merge_into(const T& t, std::string_view json, bool* merged_directly = nullptr)
    {
        // what merging through a document gives, which `merge_into` falls back to
        const auto expected = read_or_error([&] {
            auto u = t;
            yyjson::detail::merge_text(json, u, yyjson::ReadFlag::NoFlag);
            return u;
        });
        EXPECT_EQ(merged(t, json), expected) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        auto trimmed = json;
        const auto is_ws = [](char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; };
        while (!trimmed.empty() && is_ws(trimmed.front())) trimmed.remove_prefix(1);
        while (!trimmed.empty() && is_ws(trimmed.back())) trimmed.remove_suffix(1);
        if (trimmed.size() >= 2)
        {
            auto u = t;
            try
            {
                yyjson::reader::detail::direct_reader(trimmed.data(), trimmed.data() + trimmed.size() - 1,
                                                      yyjson::ReadFlag::NoFlag)
                    .merge_root(u);
                EXPECT_EQ(read_or_error([&] { return u; }), expected) << json;
                if (merged_directly != nullptr) *merged_directly = true;
            }
            catch (...)
            {
            }
        }
#else
        if (merged_directly != nullptr) *merged_directly = true;
#endif
        auto twice = t;
        try
        {
            yyjson::merge_into(json, twice);
            EXPECT_EQ(merged(twice, json), expected) << json;
        }
        catch (...)
        {
        }
        return expected;
    }

    std::string written(const auto& t) { return std::string(yyjson::write(t)); }

    // reading `json` as `T` gives the same through a document, without one, into an empty `T` and merged into
    // one, and writing the result gives the same with and without a document
    template <typename T>
    void expect_recursive(std::string_view json)
    {
        const auto expected = yyjson::read(json).cast<T>();
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_TRUE(expect_direct_read<T>(json)) << json;
        auto into = T();
        yyjson::read_into(json, into);
        EXPECT_EQ(into, expected) << json;
        auto merged = T();
        yyjson::merge_into(json, merged);
        EXPECT_EQ(merged, expected) << json;
        EXPECT_EQ(std::string_view(yyjson::write(expected)), std::string_view(yyjson::value(expected).write()))
            << json;
        EXPECT_EQ(yyjson::read(yyjson::write(expected)).template cast<T>(), expected) << json;
    }
}  // namespace


struct X
{
    int a;
    std::optional<double> b;
    std::string c = "x";
    bool operator==(const X&) const = default;
};

struct Y : public X
{
};

template <>
struct yyjson::caster<Y>
{
    inline static auto to_json(const Y& y)
    {
        return std::format("{} {} {}", y.a, (y.b ? std::format("{}", *y.b) : "null"), y.c);
    }
};

struct Z : public X
{
};

VISITABLE_STRUCT(Z, a, b, c);

struct ReflectionRecord
{
    int i;
    double j;
    std::string k;
    bool operator==(const ReflectionRecord&) const = default;
};

struct VisitableRecord
{
    int i;
    double j;
    std::string k;
    bool operator==(const VisitableRecord&) const = default;
};

VISITABLE_STRUCT(VisitableRecord, i, j, k);

struct ManyFields
{
    int f01;
    int f02;
    int f03;
    int f04;
    int f05;
    int f06;
    int f07;
    int f08;
    int f09;
    int f10;
    int f11;
    int f12;
    int f13;
    int f14;
    int f15;
    int f16;
    int f17;
    int f18;
    int f19;
    int f20;
    int f21;
    int f22;
    int f23;
    int f24;
    int f25;
    int f26;
    int f27;
    int f28;
    int f29;
    int f30;
    bool operator==(const ManyFields&) const = default;
};

VISITABLE_STRUCT(ManyFields, f01, f02, f03, f04, f05, f06, f07, f08, f09, f10, f11, f12, f13, f14, f15, f16, f17, f18,
                 f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30);

struct NullableField
{
    std::optional<int> optional_value;
    std::shared_ptr<int> nullable_value;
    std::optional<std::shared_ptr<int>> tri_state;
};

template <typename T>
class maybe
{
    std::optional<T> value_;

public:
    maybe() = default;
    maybe(T value) : value_(std::move(value)) {}  // NOLINT
    [[nodiscard]] explicit operator bool() const { return value_.has_value(); }
    [[nodiscard]] const T& operator*() const { return *value_; }
};

template <typename T>
struct yyjson::caster<maybe<T>>
{
    static bool should_omit(const maybe<T>& m) { return !m; }
    template <typename... Ts>
    static auto to_json(const maybe<T>& m, Ts...)
    {
        return *m;
    }
};

struct OmittableRecord
{
    int port = 8080;
    maybe<std::string> host;
};

VISITABLE_STRUCT(OmittableRecord, port, host);

template <>
struct yyjson::caster<std::atomic<int>>
{
    template <typename Json>
    static void from_json(std::atomic<int>& field, const Json& json)
    {
        field = yyjson::cast<int>(json);
    }
    template <typename... Ts>
    static auto to_json(const std::atomic<int>& field, Ts...)
    {
        return field.load();
    }
};

struct CounterRecord
{
    std::string name;
    std::atomic<int> hits;

    CounterRecord() = default;
    CounterRecord(const CounterRecord& other) : name(other.name), hits(other.hits.load()) {}
    CounterRecord& operator=(const CounterRecord& other)
    {
        if (this != &other)
        {
            name = other.name;
            hits = other.hits.load();
        }
        return *this;
    }
};

VISITABLE_STRUCT(CounterRecord, name, hits);

// the compile-time reflection writes this type, and the caster only decides whether it appears
struct Coordinate
{
    double x = 0.0;
    double y = 0.0;
    bool valid = false;
};

template <>
struct yyjson::caster<Coordinate>
{
    static bool should_omit(const Coordinate& c) { return !c.valid; }
};

struct Shape
{
    std::string name;
    Coordinate origin;
};

struct AddressedName
{
    std::string name;
};

// the objects that the caster below was given, in order
inline std::vector<const AddressedName*> addressed_names;

template <>
struct yyjson::caster<AddressedName>
{
    // refers to the string without copying it, so the object it was given has to outlive the document
    template <typename... Ts>
    static void to_json(writer::value_ref& v, const AddressedName& n, Ts...)
    {
        addressed_names.push_back(&n);
        v = n.name;
    }
};

struct AddressedPlain
{
    AddressedName n;
    int k;
};

struct AddressedOptional
{
    AddressedName n;
    std::optional<int> o;
};

struct ObjectCasterPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ObjectCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const ObjectCasterPoint& p, Ts... ts)
    {
        if (p.x != 0) obj.emplace("x", p.x, ts...);
        if (p.y != 0) obj.emplace("y", p.y, ts...);
    }
};

struct ArrayCasterPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ArrayCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const ArrayCasterPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

struct ValueCasterName
{
    std::string name;
};

template <>
struct yyjson::caster<ValueCasterName>
{
    template <typename... Ts>
    static void to_json(writer::value_ref& v, const ValueCasterName& n, Ts...)
    {
        if (n.name.empty())
            v = nullptr;
        else
            v = n.name;
    }
};

struct VisitableCasterPoint
{
    int x;
    int y;
};

VISITABLE_STRUCT(VisitableCasterPoint, x, y);

template <>
struct yyjson::caster<VisitableCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const VisitableCasterPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

struct TranslatedPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<TranslatedPoint>
{
    static auto to_json(const TranslatedPoint& p) { return std::format("{},{}", p.x, p.y); }
};

struct TranslatedPointHolder
{
    TranslatedPoint point;
    std::vector<TranslatedPoint> points;
};

struct NestedWriteInner
{
    std::string name;
    std::vector<int> ids;
};

struct NestedWriteRecord
{
    NestedWriteInner inner;
    std::vector<std::vector<int>> grid;
    std::map<std::string, std::vector<int>> groups;
    ObjectCasterPoint object_point;
    ArrayCasterPoint array_point;
    ValueCasterName value_name;
    std::vector<int> empty;
    std::optional<std::vector<int>> maybe_ids;
    std::tuple<int, std::string> pair;
};

struct KeyLengths
{
    int a = 0;
    int bb = 0;
    int fifteen_chars_x = 0;
    int sixteen_chars_xx = 0;
    int seventeen_chars_x = 0;
    int thirty_one_chars_xxxxxxxxxxxxxx = 0;
    int thirty_two_chars_xxxxxxxxxxxxxxx = 0;
    std::vector<int> v;
};

struct ReadPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ReadPoint>
{
    template <typename Json>
    static ReadPoint from_json(const Json& json)
    {
        const auto arr = json.as_array();
        if (!arr || arr->size() != 2) throw yyjson::bad_cast("not a point");
        return {cast<int>((*arr)[0]), cast<int>((*arr)[1])};
    }
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const ReadPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

struct ReadRecord
{
    std::string name;
    std::vector<ReadPoint> points;
    std::variant<std::monostate, int, std::string> tag;
    std::map<std::string, std::vector<double>> series;
    std::optional<KeyLengths> keys;
    std::shared_ptr<std::string> note;
    std::tuple<int, std::string> pair;
};

struct EmptyArrays
{
    std::vector<int> v = {1, 2};
    std::set<int> s = {3};
    std::list<std::string> l = {"x"};
    std::map<std::string, int> m = {{"k", 4}};
    std::vector<std::vector<int>> nested = {{5}};
    bool operator==(const EmptyArrays&) const = default;
};

struct BoolFields
{
    bool a = false;
    bool b = true;
    std::optional<bool> c;
    bool operator==(const BoolFields&) const = default;
};

struct ReadChain
{
    ReadPoint point;
    std::shared_ptr<ReadChain> next;

    bool operator==(const ReadChain& other) const
    {
        return point.x == other.point.x && point.y == other.point.y &&
               (next == nullptr ? other.next == nullptr : other.next != nullptr && *next == *other.next);
    }
};

// A chain of types in which each type is reached through the two before it, so that the number of paths to the
// first one grows as the Fibonacci numbers.
template <std::size_t N>
struct DiamondChain
{
    std::vector<DiamondChain<N - 1>> a;
    std::vector<DiamondChain<N - 2>> b;
    bool operator==(const DiamondChain&) const = default;
};
template <>
struct DiamondChain<1>
{
    int a = 0;
    bool operator==(const DiamondChain&) const = default;
};
template <>
struct DiamondChain<0>
{
    int a = 0;
    bool operator==(const DiamondChain&) const = default;
};
// the depth that the direct reader walks; MSVC runs out of heap space at 20 in this file, and clang-cl optimizing
// for Windows takes a time and memory that grow with the number of paths, running out of memory at 30
#if defined(_MSC_VER)
constexpr std::size_t diamond_depth = 10;
#else
constexpr std::size_t diamond_depth = 40;
#endif

#if YYJSON_VERSION_HEX >= 0x000C00
template <typename T>
struct DocumentOf
{
    T value;
    bool operator==(const DocumentOf&) const = default;
};

template <typename T>
struct yyjson::caster<DocumentOf<T>>
{
    template <typename Json>
    static DocumentOf<T> from_json(const Json& json)
    {
        return {cast<T>(json)};
    }
};
#endif

// Empty structs, which are read and written as empty JSON objects
struct EmptyStruct
{
    bool operator==(const EmptyStruct&) const = default;
};

// Enumerations, which are read and written by the names of their enumerators only
enum class Color
{
    red,
    green,
    blue
};

// NOLINTEND
