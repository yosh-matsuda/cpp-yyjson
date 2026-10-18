#include "test_common.hpp"

// NOLINTBEGIN

#if FIELD_RFL_STD_REFLECTION
// types that only the C++26 backend of field-reflection reflects
struct ReflectedBase
{
    int id = 0;
    std::string name;
    bool operator==(const ReflectedBase&) const = default;
};
struct ReflectedDerived : ReflectedBase
{
    std::vector<int> list;
    std::optional<double> score;
    bool operator==(const ReflectedDerived&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ReflectedDerived> =
    yyjson::annotate(yyjson::field("name", yyjson::rename("@name")));
struct ReflectedWide
{
    int f0 = 0;
    int f1 = 1;
    int f2 = 2;
    int f3 = 3;
    int f4 = 4;
    int f5 = 5;
    int f6 = 6;
    int f7 = 7;
    int f8 = 8;
    int f9 = 9;
    int f10 = 10;
    int f11 = 11;
    int f12 = 12;
    int f13 = 13;
    int f14 = 14;
    int f15 = 15;
    int f16 = 16;
    int f17 = 17;
    int f18 = 18;
    int f19 = 19;
    int f20 = 20;
    int f21 = 21;
    int f22 = 22;
    int f23 = 23;
    int f24 = 24;
    int f25 = 25;
    int f26 = 26;
    int f27 = 27;
    int f28 = 28;
    int f29 = 29;
    int f30 = 30;
    int f31 = 31;
    int f32 = 32;
    int f33 = 33;
    int f34 = 34;
    int f35 = 35;
    int f36 = 36;
    int f37 = 37;
    int f38 = 38;
    int f39 = 39;
    int f40 = 40;
    int f41 = 41;
    int f42 = 42;
    int f43 = 43;
    int f44 = 44;
    int f45 = 45;
    int f46 = 46;
    int f47 = 47;
    int f48 = 48;
    int f49 = 49;
    int f50 = 50;
    int f51 = 51;
    int f52 = 52;
    int f53 = 53;
    int f54 = 54;
    int f55 = 55;
    int f56 = 56;
    int f57 = 57;
    int f58 = 58;
    int f59 = 59;
    int f60 = 60;
    int f61 = 61;
    int f62 = 62;
    int f63 = 63;
    int f64 = 64;
    int f65 = 65;
    int f66 = 66;
    int f67 = 67;
    int f68 = 68;
    int f69 = 69;
    int f70 = 70;
    int f71 = 71;
    int f72 = 72;
    int f73 = 73;
    int f74 = 74;
    int f75 = 75;
    int f76 = 76;
    int f77 = 77;
    int f78 = 78;
    int f79 = 79;
    int f80 = 80;
    int f81 = 81;
    int f82 = 82;
    int f83 = 83;
    int f84 = 84;
    int f85 = 85;
    int f86 = 86;
    int f87 = 87;
    int f88 = 88;
    int f89 = 89;
    int f90 = 90;
    int f91 = 91;
    int f92 = 92;
    int f93 = 93;
    int f94 = 94;
    int f95 = 95;
    int f96 = 96;
    int f97 = 97;
    int f98 = 98;
    int f99 = 99;
    int f100 = 100;
    int f101 = 101;
    int f102 = 102;
    int f103 = 103;
    int f104 = 104;
    int f105 = 105;
    int f106 = 106;
    int f107 = 107;
    int f108 = 108;
    int f109 = 109;
    int f110 = 110;
    int f111 = 111;
    int f112 = 112;
    int f113 = 113;
    int f114 = 114;
    int f115 = 115;
    int f116 = 116;
    int f117 = 117;
    int f118 = 118;
    int f119 = 119;
    bool operator==(const ReflectedWide&) const = default;
};
struct EmptyDerived : EmptyStruct
{
};
struct EmptyBased : EmptyStruct
{
    int x = 3;
};
struct EmptyBasedFields
{
    EmptyDerived d;
    EmptyBased b;
};

TEST(Reflection, StdReflection)
{
    static_assert(field_reflection::field_count<ReflectedDerived> == 4);
    static_assert(field_reflection::field_count<ReflectedWide> == 120);

    const auto* json = R"({"id":1,"@name":"a","list":[1,2],"score":0.5})";
    expect_recursive<ReflectedDerived>(json);
    auto derived = ReflectedDerived{};
    derived.id = 1;
    derived.name = "a";
    derived.list = {1, 2};
    derived.score = 0.5;
    EXPECT_EQ(yyjson::read<ReflectedDerived>(json), derived);
    EXPECT_EQ(std::string_view(yyjson::write(derived)), json);
    expect_direct_write(derived);
    expect_direct_write(std::vector<ReflectedDerived>{derived, ReflectedDerived{}});

    auto wide_json = std::string("{");
    for (auto i = 0; i < 120; ++i) wide_json += std::format("{}\"f{}\":{}", i == 0 ? "" : ",", i, i * 2);
    wide_json += "}";
    expect_recursive<ReflectedWide>(wide_json);
    const auto wide = yyjson::read<ReflectedWide>(wide_json);
    EXPECT_EQ(wide.f0, 0);
    EXPECT_EQ(wide.f119, 238);
    EXPECT_EQ(std::string_view(yyjson::write(wide)), wide_json);
    expect_direct_write(wide);

    // a struct whose only base is empty is an empty struct, and one with fields of its own is not
    static_assert(field_reflection::field_count<EmptyDerived> == 0);
    static_assert(field_reflection::field_count<EmptyBased> == 1);
    const auto* based_json = R"({"d":{},"b":{"x":3}})";
    EXPECT_EQ(written(EmptyBasedFields()), based_json);
    expect_direct_write(EmptyBasedFields());
    EXPECT_EQ(written(yyjson::read<EmptyBasedFields>(R"({"d":{"y":1},"b":{"x":3}})")), based_json);
    EXPECT_TRUE(expect_direct_read<EmptyBasedFields>(R"({"d":{"y":1},"b":{"x":5}})"));
    expect_direct_read<EmptyBasedFields>(R"({"d":[]})");
}
#endif

// Fields skipped by `[[=yyjson::skip]]` on the member with C++26 annotations, by `yyjson::annotations` otherwise
#if FIELD_RFL_ANNOTATIONS
#define SKIPPED_FIELD [[= yyjson::skip]]
#else
#define SKIPPED_FIELD
#endif
struct SkippedFields
{
    int id = 0;
    SKIPPED_FIELD std::string cache = "default";
    std::vector<int> list = {};
    SKIPPED_FIELD std::function<int()> callback = {};  // neither read nor written by this library
    SKIPPED_FIELD int* pointer = nullptr;
    std::optional<int> opt = {};
    bool operator==(const SkippedFields& other) const
    {
        return id == other.id && cache == other.cache && list == other.list && pointer == other.pointer &&
               opt == other.opt;
    }
};
// a skipped field leaves its key to another field
struct SkippedKeyReused
{
    SKIPPED_FIELD int a = 7;
    int b = 0;
    bool operator==(const SkippedKeyReused&) const = default;
};
struct AllSkipped
{
    SKIPPED_FIELD int a = 1;
    SKIPPED_FIELD std::string b = "b";
    bool operator==(const AllSkipped&) const = default;
};
struct SkippedConst
{
    int x = 0;
    SKIPPED_FIELD const int version = 3;
    bool operator==(const SkippedConst&) const = default;
};
struct SkippedMutex
{
    int x = 0;
    SKIPPED_FIELD std::mutex mutex = {};
};
#if FIELD_RFL_ANNOTATIONS
// the C++20 backend of field-reflection does not reflect a reference member
struct SkippedReference
{
    int x = 0;
    SKIPPED_FIELD const int& ref;
};
#endif
// not a recursive type, as the field of its own type is skipped
struct SkippedRecursion
{
    int v = 0;
    SKIPPED_FIELD std::vector<SkippedRecursion> children;
    bool operator==(const SkippedRecursion&) const = default;
};
#if FIELD_RFL_ANNOTATIONS
template <>
inline constexpr auto yyjson::annotations<SkippedKeyReused> = yyjson::annotate(yyjson::field("b", yyjson::rename("a")));
#else
template <>
inline constexpr auto yyjson::annotations<SkippedFields> =
    yyjson::annotate(yyjson::field("cache", yyjson::skip), yyjson::field("callback", yyjson::skip),
                     yyjson::field("pointer", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<SkippedKeyReused> =
    yyjson::annotate(yyjson::field("a", yyjson::skip), yyjson::field("b", yyjson::rename("a")));
template <>
inline constexpr auto yyjson::annotations<AllSkipped> =
    yyjson::annotate(yyjson::field("a", yyjson::skip), yyjson::field("b", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<SkippedConst> = yyjson::annotate(yyjson::field("version", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<SkippedMutex> = yyjson::annotate(yyjson::field("mutex", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<SkippedRecursion> = yyjson::annotate(yyjson::field("children", yyjson::skip));
#endif

TEST(Reflection, AnnotationSkip)
{
    using yyjson::detail::reflected_fields;
    static_assert(std::same_as<reflected_fields<SkippedFields>, std::index_sequence<0, 2, 5>>);
    static_assert(std::same_as<reflected_fields<AllSkipped>, std::index_sequence<>>);
    static_assert(!yyjson::detail::recursive_type<SkippedRecursion>);

    auto value = 5;
    // reading ignores the keys of the skipped fields, and the fields keep their values
    const auto* json = R"({"id":1,"cache":"x","list":[1,2],"callback":3,"pointer":4,"opt":5})";
    auto expected = SkippedFields{.id = 1, .list = {1, 2}, .opt = 5};
    EXPECT_EQ(yyjson::read<SkippedFields>(json), expected);
    EXPECT_EQ(yyjson::read(json).cast<SkippedFields>(), expected);
    EXPECT_TRUE(expect_direct_read<SkippedFields>(json));
    EXPECT_TRUE(expect_direct_read<SkippedFields>(R"({"cache":{"a":[1,{}]},"id":2})"));
    EXPECT_TRUE(expect_direct_read<std::vector<SkippedFields>>(std::string("[") + json + ",{}]"));
    expect_direct_read<SkippedFields>(R"({"id":"x","cache":1})");
    expect_direct_read<SkippedFields>("[]");
    EXPECT_EQ(std::get<1>(yyjson::read<std::variant<int, SkippedFields>>(json)), expected);

    auto t = SkippedFields{.id = 9, .cache = "kept", .list = {9}, .pointer = &value, .opt = 9};
    t.callback = [] { return 1; };
    // `read_into` gives the skipped fields their values in `T()` as `t = read<T>(json)` does, and `merge_into`
    // keeps them
    yyjson::read_into(R"({"id":2,"cache":"x","pointer":null})", t);
    EXPECT_EQ(t, (SkippedFields{.id = 2}));
    EXPECT_FALSE(t.callback);
    t.cache = "kept";
    t.pointer = &value;
    t.callback = [] { return 1; };
    yyjson::merge_into(R"({"list":[3],"cache":"y","callback":null,"opt":1})", t);
    EXPECT_EQ(t, (SkippedFields{.id = 2, .cache = "kept", .list = {3}, .pointer = &value, .opt = 1}));
    EXPECT_TRUE(t.callback);

    // writing leaves the skipped fields out
    const auto written_json = R"({"id":1,"list":[1,2],"opt":5})";
    EXPECT_EQ(std::string(yyjson::write(expected)), written_json);
    EXPECT_EQ(std::string(yyjson::value(expected).write()), written_json);
    EXPECT_EQ(std::string(yyjson::object(expected).write()), written_json);
    EXPECT_EQ(std::string(yyjson::object(SkippedFields(expected)).write()), written_json);
    EXPECT_EQ(std::string(yyjson::write(expected, yyjson::WriteFlag::Pretty)),
              std::string(yyjson::value(expected).write(yyjson::WriteFlag::Pretty)));
    EXPECT_EQ(std::string(yyjson::write(SkippedFields{.id = 1})), R"({"id":1,"list":[]})");
    expect_direct_write(expected);
    expect_direct_write(SkippedFields{});
    expect_direct_write(std::vector<SkippedFields>{expected, SkippedFields{}});
    EXPECT_EQ(yyjson::read<SkippedFields>(std::string(yyjson::write(expected))), expected);

    EXPECT_EQ(yyjson::read<SkippedKeyReused>(R"({"a":1})"), (SkippedKeyReused{7, 1}));
    EXPECT_EQ(yyjson::read(R"({"a":1})").cast<SkippedKeyReused>(), (SkippedKeyReused{7, 1}));
    EXPECT_EQ(std::string(yyjson::write(SkippedKeyReused{1, 2})), R"({"a":2})");
    EXPECT_EQ(std::string(yyjson::value(SkippedKeyReused{1, 2}).write()), R"({"a":2})");

    // a struct whose fields are all skipped is an empty object
    EXPECT_EQ(std::string(yyjson::write(AllSkipped{2, "x"})), "{}");
    EXPECT_EQ(std::string(yyjson::write(AllSkipped{2, "x"}, yyjson::WriteFlag::Pretty)), "{}");
    EXPECT_EQ(std::string(yyjson::value(AllSkipped{2, "x"}).write()), "{}");
    EXPECT_EQ(yyjson::read<AllSkipped>(R"({"a":2,"b":"x"})"), AllSkipped());
    EXPECT_EQ(yyjson::read(R"({"a":2,"b":"x"})").cast<AllSkipped>(), AllSkipped());
    EXPECT_TRUE(expect_direct_read<AllSkipped>(R"({"a":2})"));
    expect_direct_read<AllSkipped>("1");
    auto all = AllSkipped{2, "x"};
    yyjson::merge_into(R"({"a":3})", all);
    EXPECT_EQ(all, (AllSkipped{2, "x"}));
    yyjson::read_into(R"({"a":3})", all);
    EXPECT_EQ(all, AllSkipped());
    expect_direct_write(AllSkipped{});

    // skipped fields of types that cannot be read or assigned
    EXPECT_EQ(yyjson::read<SkippedConst>(R"({"x":1,"version":4})"), (SkippedConst{1, 3}));
    EXPECT_EQ(std::string(yyjson::write(SkippedConst{1, 3})), R"({"x":1})");
    expect_direct_write(SkippedConst{1, 3});
    auto mutex = SkippedMutex{.x = 1};
    EXPECT_EQ(std::string(yyjson::write(mutex)), R"({"x":1})");
    EXPECT_EQ(std::string(yyjson::value(mutex).write()), R"({"x":1})");
#if FIELD_RFL_ANNOTATIONS
    const auto reference = SkippedReference{2, value};
    EXPECT_EQ(std::string(yyjson::write(reference)), R"({"x":2})");
    EXPECT_EQ(std::string(yyjson::value(reference).write()), R"({"x":2})");
#endif

    const auto recursion = SkippedRecursion{1, {{2, {}}}};
    EXPECT_EQ(std::string(yyjson::write(recursion)), R"({"v":1})");
    EXPECT_EQ(yyjson::read<SkippedRecursion>(R"({"v":1,"children":[{"v":2}]})"), (SkippedRecursion{1, {}}));
}

#if FIELD_RFL_ANNOTATIONS
// keys given by C++26 annotations
struct AnnotatedProject
{
    [[= yyjson::rename("@context")]] std::string context;
    [[= yyjson::rename("a/b\"c, longer than 16")]] std::string type;
    int id = 0;
    bool operator==(const AnnotatedProject&) const = default;
};
// keys given by `yyjson::annotations` to a struct that C++26 reflects
struct TypeAnnotatedProject
{
    std::string context;
    std::string type;
    int id = 0;
    bool operator==(const TypeAnnotatedProject&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TypeAnnotatedProject> =
    yyjson::annotate(yyjson::field("context", yyjson::rename("@context")),
                     yyjson::field("type", yyjson::rename("a/b\"c, longer than 16")));
// keys given by an annotation of the member and by `yyjson::annotations`
struct CombinedRenames
{
    [[= yyjson::rename("@a")]] int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    bool operator==(const CombinedRenames&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CombinedRenames> =
    yyjson::annotate(yyjson::field("b", yyjson::rename("@b")), yyjson::field("c", yyjson::rename("@c")));
struct AnnotatedSwappedKeys
{
    [[= yyjson::rename("b")]] int a = 0;
    [[= yyjson::rename("a")]] int b = 0;
    bool operator==(const AnnotatedSwappedKeys&) const = default;
};

struct HoldsAnnotated
{
    AnnotatedProject project;
    std::vector<SkippedFields> items;
    std::map<std::string, TypeAnnotatedProject> by_name;
    std::optional<CombinedRenames> combined;
    AllSkipped none = {};
    bool operator==(const HoldsAnnotated&) const = default;
};

TEST(Reflection, AnnotationRename)
{
    static_assert(std::same_as<yyjson::detail::reflected_fields<AnnotatedProject>, std::index_sequence<0, 1, 2>>);
    static_assert(yyjson::detail::field_key<AnnotatedProject, 0> == "@context");
    static_assert(yyjson::detail::field_key<AnnotatedProject, 2> == "id");
    static_assert(yyjson::detail::field_key<TypeAnnotatedProject, 1> == "a/b\"c, longer than 16");
    static_assert(yyjson::detail::field_key<CombinedRenames, 0> == "@a");
    static_assert(yyjson::detail::field_key<CombinedRenames, 1> == "@b");
    static_assert(yyjson::detail::field_key<CombinedRenames, 2> == "@c");
    static_assert(yyjson::detail::field_key<CombinedRenames, 3> == "d");

    const auto* json = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1,"context":"x","type":"y"})";
    const auto* written_json = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1})";
    const auto check = [&]<typename T>(std::type_identity<T>) {
        const auto expected = T{"c", "t", 1};
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected);
        EXPECT_EQ(yyjson::read<T>(json), expected);
        EXPECT_TRUE(expect_direct_read<T>(json));
        EXPECT_TRUE(expect_direct_read<T>(R"({"\u0040context":"c","a\/b\"c, longer than 16":"t"})"));
        EXPECT_TRUE(expect_direct_read<std::vector<T>>(std::string("[") + json + "," + json + "]"));
        auto t = T{"old", "old", 9};
        yyjson::read_into(R"({"@context":"c"})", t);
        EXPECT_EQ(t, (T{"c", "", 0}));
        yyjson::merge_into(R"({"id":2,"type":"x"})", t);
        EXPECT_EQ(t, (T{"c", "", 2}));

        EXPECT_EQ(std::string(yyjson::value(expected).write()), written_json);
        EXPECT_EQ(std::string(yyjson::object(expected).write()), written_json);
        EXPECT_EQ(std::string(yyjson::object(T(expected)).write()), written_json);
        EXPECT_EQ(std::string(yyjson::write(expected)), written_json);
        expect_direct_write(expected);
        expect_direct_write(std::vector<T>{expected, T{}});
        EXPECT_EQ(yyjson::read<T>(std::string(yyjson::write(expected, yyjson::WriteFlag::Pretty))), expected);
    };
    check(std::type_identity<AnnotatedProject>{});
    check(std::type_identity<TypeAnnotatedProject>{});

    const auto combined = CombinedRenames{1, 2, 3, 4};
    const auto* combined_json = R"({"@a":1,"@b":2,"@c":3,"d":4})";
    EXPECT_EQ(std::string(yyjson::write(combined)), combined_json);
    EXPECT_EQ(std::string(yyjson::value(combined).write()), combined_json);
    EXPECT_EQ(yyjson::read<CombinedRenames>(combined_json), combined);
    EXPECT_EQ(yyjson::read(combined_json).cast<CombinedRenames>(), combined);
    EXPECT_EQ(yyjson::read<CombinedRenames>(R"({"a":1,"b":2,"c":3,"d":4})"), (CombinedRenames{0, 0, 0, 4}));
    EXPECT_TRUE(expect_direct_read<CombinedRenames>(combined_json));

    const auto* swapped = R"({"a":1,"b":2})";
    EXPECT_EQ(yyjson::read<AnnotatedSwappedKeys>(swapped), (AnnotatedSwappedKeys{2, 1}));
    EXPECT_EQ(yyjson::read(swapped).cast<AnnotatedSwappedKeys>(), (AnnotatedSwappedKeys{2, 1}));
    EXPECT_EQ(std::string(yyjson::write(AnnotatedSwappedKeys{1, 2})), R"({"b":1,"a":2})");
    EXPECT_TRUE(expect_direct_read<AnnotatedSwappedKeys>(swapped));
}

TEST(Reflection, AnnotationNested)
{
    auto holds = HoldsAnnotated{
        .project = {"c", "t", 1},
        .items = {SkippedFields{.id = 1, .list = {2}}, SkippedFields{.id = 3, .opt = 4}},
        .by_name = {{"k", {"x", "y", 5}}},
        .combined = CombinedRenames{1, 2, 3, 4},
    };
    const auto* json = R"({"project":{"@context":"c","a/b\"c, longer than 16":"t","id":1},)"
                       R"("items":[{"id":1,"list":[2]},{"id":3,"list":[],"opt":4}],)"
                       R"("by_name":{"k":{"@context":"x","a/b\"c, longer than 16":"y","id":5}},)"
                       R"("combined":{"@a":1,"@b":2,"@c":3,"d":4},"none":{}})";
    EXPECT_EQ(std::string(yyjson::write(holds)), json);
    EXPECT_EQ(std::string(yyjson::value(holds).write()), json);
    expect_direct_write(holds);
    EXPECT_EQ(yyjson::read<HoldsAnnotated>(json), holds);
    EXPECT_EQ(yyjson::read(json).cast<HoldsAnnotated>(), holds);
    EXPECT_TRUE(expect_direct_read<HoldsAnnotated>(json));
    EXPECT_EQ(yyjson::read<HoldsAnnotated>(std::string(yyjson::write(holds, yyjson::WriteFlag::Pretty))), holds);

    // the skipped fields of the elements read over get their values in `T()`
    auto into = holds;
    into.items[0].cache = "changed";
    into.items[1].cache = "changed";
    yyjson::read_into(json, into);
    EXPECT_EQ(into, holds);
    EXPECT_EQ(into.items[0].cache, "default");
    EXPECT_EQ(into.items[1].cache, "default");
    into.items[0].cache = "kept";
    yyjson::merge_into(R"({"items":[{"id":7,"cache":"x"}],"none":{"a":2}})", into);
    EXPECT_EQ(into.items.size(), 1);
    EXPECT_EQ(into.items[0].id, 7);
    EXPECT_EQ(into.none, AllSkipped());
}
#endif

// NOLINTEND
