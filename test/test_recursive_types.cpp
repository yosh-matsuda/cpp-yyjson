#include "test_common.hpp"

// NOLINTBEGIN

// Recursive types, which reach themselves through their fields
struct RecTree
{
    int v = 0;
    std::vector<RecTree> kids;
    bool operator==(const RecTree&) const = default;
};
struct RecList
{
    int v = 0;
    std::shared_ptr<RecList> next;
    bool operator==(const RecList& other) const
    {
        return v == other.v &&
               (next == nullptr ? other.next == nullptr : other.next != nullptr && *next == *other.next);
    }
};
struct RecUList
{
    int v = 0;
    std::unique_ptr<RecUList> next;
    std::vector<std::unique_ptr<RecUList>> kids;
    bool operator==(const RecUList& other) const
    {
        const auto same = [](const auto& a, const auto& b) {
            return a == nullptr ? b == nullptr : b != nullptr && *a == *b;
        };
        return v == other.v && same(next, other.next) &&
               std::ranges::equal(kids, other.kids, [&](const auto& a, const auto& b) { return same(a, b); });
    }
};
// with clang and libstdc++, `std::is_default_constructible_v<RecDict>` is false, because the defaulted
// `operator==` instantiates it before the default member initializer is complete; `merge_into` still merges into it
struct RecDict
{
    int v = 0;
    std::map<std::string, RecDict> children;
    bool operator==(const RecDict&) const = default;
};
struct RecB;
struct RecA
{
    int a = 0;
    std::vector<RecB> bs;
    bool operator==(const RecA&) const = default;
};
struct RecB
{
    std::optional<std::vector<RecA>> as;
    bool operator==(const RecB&) const = default;
};
struct RecVisitable
{
    int v = 0;
    std::vector<RecVisitable> kids;
    bool operator==(const RecVisitable&) const = default;
};
VISITABLE_STRUCT(RecVisitable, v, kids);
// any JSON value, read and written by a caster of the user through a variant
struct RecJson
{
    std::variant<std::nullptr_t, bool, double, std::string, std::vector<RecJson>, std::map<std::string, RecJson>> v;
    bool operator==(const RecJson&) const = default;
};
template <>
struct yyjson::caster<RecJson>
{
    template <typename Json>
    static RecJson from_json(const Json& json)
    {
        return {yyjson::cast<decltype(RecJson::v)>(json)};
    }
    template <typename... Ts>
    static const decltype(RecJson::v)& to_json(const RecJson& j, Ts...)
    {
        return j.v;
    }
};
// nested arrays, read by a translator from the arrays of the items and assigned in place by another
struct RecNest
{
    std::vector<RecNest> items;
    bool operator==(const RecNest&) const = default;
};
template <>
struct yyjson::caster<RecNest>
{
    static RecNest from_json(std::vector<RecNest> items) { return {std::move(items)}; }
    template <typename... Ts>
    static const std::vector<RecNest>& to_json(const RecNest& n, Ts...)
    {
        return n.items;
    }
};
struct RecNestInPlace
{
    std::vector<RecNestInPlace> items;
    bool operator==(const RecNestInPlace&) const = default;
};
template <>
struct yyjson::caster<RecNestInPlace>
{
    static void from_json(RecNestInPlace& n, std::vector<RecNestInPlace> items) { n.items = std::move(items); }
    template <typename... Ts>
    static const std::vector<RecNestInPlace>& to_json(const RecNestInPlace& n, Ts...)
    {
        return n.items;
    }
};
struct RecNestHolder
{
    RecNestInPlace nest;
    bool operator==(const RecNestHolder&) const = default;
};
// a type that reaches an incomplete type through a template argument, which is not recursive
struct PimplImpl;
// whose special members are defined where `PimplImpl` is complete, as those of a pimpl class are
struct PimplHandle
{
    std::unique_ptr<PimplImpl> impl;
    PimplHandle();
    explicit PimplHandle(std::unique_ptr<PimplImpl> p);
    PimplHandle(PimplHandle&&) noexcept;
    PimplHandle& operator=(PimplHandle&&) noexcept;
    ~PimplHandle();
};
template <>
struct yyjson::caster<PimplHandle>
{
    template <typename Json>
    static PimplHandle from_json(const Json& json)
    {
        return PimplHandle(std::make_unique<PimplImpl>(yyjson::cast<int>(json)));
    }
    template <typename... Ts>
    static int to_json(const PimplHandle& h, Ts...);
};
struct PimplOwner
{
    int x = 0;
    PimplHandle handle;
};
// a variant of two recursive alternatives that both read an object
struct RecExpr;
struct RecAdd
{
    std::vector<RecExpr> args;
    bool operator==(const RecAdd&) const = default;
};
struct RecMul
{
    std::vector<RecExpr> args;
    bool operator==(const RecMul&) const = default;
};
struct RecExpr
{
    std::variant<RecAdd, RecMul> op;
    bool operator==(const RecExpr&) const = default;
};
// the same, told apart by the type of `x`, which follows the nested values
struct RecTagged;
struct RecIntX
{
    std::vector<RecTagged> args;
    int x = 0;
    bool operator==(const RecIntX&) const = default;
};
struct RecStrX
{
    std::vector<RecTagged> args;
    std::string x;
    bool operator==(const RecStrX&) const = default;
};
struct RecTagged
{
    std::variant<RecIntX, RecStrX> op;
    bool operator==(const RecTagged&) const = default;
};

static_assert(yyjson::detail::recursive_type<RecTree>);
static_assert(yyjson::detail::recursive_type<RecList>);
static_assert(yyjson::detail::recursive_type<RecUList>);
static_assert(yyjson::detail::recursive_type<RecDict>);
static_assert(std::default_initializable<RecDict>);
static_assert(yyjson::detail::recursive_type<RecA> && yyjson::detail::recursive_type<RecB>);
static_assert(yyjson::detail::recursive_type<RecVisitable>);
static_assert(yyjson::detail::recursive_type<RecJson>);
static_assert(yyjson::detail::recursive_type<std::vector<RecTree>>);
static_assert(!yyjson::detail::recursive_type<ReflectionRecord>);
static_assert(!yyjson::detail::recursive_type<DiamondChain<40>>);
static_assert(!yyjson::detail::recursive_type<PimplOwner>);
// GCC 16 warns that `PimplImpl` is completed after the walk above found it incomplete, which is what this test checks
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 16
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsfinae-incomplete"
#endif
struct PimplImpl
{
    int v = 0;
};
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 16
#pragma GCC diagnostic pop
#endif
PimplHandle::PimplHandle() = default;
PimplHandle::PimplHandle(std::unique_ptr<PimplImpl> p) : impl(std::move(p)) {}
PimplHandle::PimplHandle(PimplHandle&&) noexcept = default;
PimplHandle& PimplHandle::operator=(PimplHandle&&) noexcept = default;
PimplHandle::~PimplHandle() = default;
template <typename... Ts>
int yyjson::caster<PimplHandle>::to_json(const PimplHandle& h, Ts...)
{
    return h.impl->v;
}

namespace
{
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

    // `depth` values of `T` nested in the text that `make` returns are read up to the limit, and each way of
    // reading throws `bad_cast` beyond it
    template <typename T>
    void expect_depth_limit(std::string (*make)(std::size_t depth))
    {
        static_assert(CPPYYJSON_RECURSION_DEPTH_LIMIT == 256);
        const auto read_all = [](const std::string& json) {
            EXPECT_EQ(yyjson::detail::recursion_depth, 0);
            (void)yyjson::read(json).template cast<T>();
            (void)yyjson::read<T>(json);
            auto into = T();
            yyjson::read_into(json, into);
            yyjson::merge_into(json, into);
            EXPECT_EQ(yyjson::detail::recursion_depth, 0);
        };
        EXPECT_NO_THROW(read_all(make(256))) << typeid(T).name();
        for (const auto depth : {std::size_t{257}, std::size_t{100'000}})
        {
            const auto json = make(depth);
            EXPECT_THROW((void)yyjson::read(json).template cast<T>(), yyjson::bad_cast) << depth;
            EXPECT_THROW((void)yyjson::read<T>(json), yyjson::bad_cast) << depth;
            auto into = T();
            EXPECT_THROW(yyjson::read_into(json, into), yyjson::bad_cast) << depth;
            EXPECT_THROW(yyjson::merge_into(json, into), yyjson::bad_cast) << depth;
            EXPECT_EQ(yyjson::detail::recursion_depth, 0) << depth;
        }
    }

    std::string nested(std::size_t depth, std::string_view open, std::string_view innermost, std::string_view close)
    {
        auto json = std::string();
        for (std::size_t i = 0; i < depth; ++i) json += open;
        json += innermost;
        for (std::size_t i = 0; i < depth; ++i) json += close;
        return json;
    }
}  // namespace

TEST(Reader, RecursiveTypes)
{
    expect_recursive<RecTree>(R"({"v":1,"kids":[{"v":2,"kids":[{"v":3,"kids":[]}]},{"v":4}]})");
    expect_recursive<std::vector<RecTree>>(R"([{"v":1,"kids":[{"v":2}]},{"v":3}])");
    expect_recursive<RecList>(R"({"v":1,"next":{"v":2,"next":{"v":3,"next":null}}})");
    expect_recursive<RecUList>(R"({"v":1,"next":{"v":2,"next":{"v":3,"next":null}},"kids":[{"v":4},null]})");
    expect_recursive<RecDict>(R"({"v":1,"children":{"x":{"v":2,"children":{"y":{"v":3}}},"z":{}}})");
    expect_recursive<RecA>(R"({"a":1,"bs":[{"as":[{"a":2,"bs":[{}]}]},{"as":null}]})");
    expect_recursive<RecVisitable>(R"({"v":1,"kids":[{"v":2,"kids":[{"v":3}]}]})");
    expect_recursive<RecJson>(R"({"a":[1,true,null,"s",{"b":[[],{}]}]})");
    expect_recursive<RecNest>(R"([[],[[[]],[]]])");
    expect_recursive<RecNestHolder>(R"({"nest":[[],[[[]],[]]]})");

    // a value of a recursive type that does not match throws as any other
    EXPECT_THROW((void)yyjson::read<RecTree>(R"({"v":1,"kids":[{"v":"x"}]})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<RecNest>(R"([[],[1]])"), yyjson::bad_cast);

    // merging replaces arrays and merges objects at every level
    auto tree = RecTree{1, {{2, {}}, {3, {{4, {}}}}}};
    yyjson::merge_into(R"({"kids":[{"v":5},{"kids":[{"v":6}]}]})", tree);
    EXPECT_EQ(tree, (RecTree{1, {{5, {}}, {0, {{6, {}}}}}}));
    auto dict = RecDict{1, {{"a", {2, {{"b", {3, {}}}}}}}};
    yyjson::merge_into(R"({"children":{"a":{"children":{"b":{"v":4}}}}})", dict);
    EXPECT_EQ(dict, (RecDict{1, {{"a", {2, {{"b", {4, {}}}}}}}}));

    // an incomplete type reached through a template argument does not make a type recursive
    const auto owner = yyjson::read<PimplOwner>(R"({"x":1,"handle":2})");
    EXPECT_EQ(owner.x, 1);
    EXPECT_EQ(std::string_view(yyjson::write(owner)), R"({"x":1,"handle":2})");
}

TEST(Reader, RecursionDepthLimit)
{
    expect_depth_limit<RecTree>([](std::size_t depth) {
        return nested(depth - 1, R"({"v":1,"kids":[)", R"({"v":2})", "]}");
    });
    expect_depth_limit<RecList>([](std::size_t depth) { return nested(depth, R"({"next":)", "null", "}"); });
    expect_depth_limit<RecUList>([](std::size_t depth) { return nested(depth - 1, R"({"kids":[)", "{}", "]}"); });
    expect_depth_limit<RecDict>([](std::size_t depth) {
        return nested(depth - 1, R"({"children":{"a":)", "{}", "}}");
    });
    // a pair of `RecA` and `RecB` counts twice
    expect_depth_limit<RecA>([](std::size_t depth) {
        if (depth % 2 == 1) return nested(depth / 2, R"({"bs":[{"as":[)", "{}", "]}]}");
        return nested(depth / 2 - 1, R"({"bs":[{"as":[)", R"({"bs":[{}]})", "]}]}");
    });
    expect_depth_limit<RecVisitable>([](std::size_t depth) {
        return nested(depth - 1, R"({"kids":[)", "{}", "]}");
    });
    expect_depth_limit<RecJson>([](std::size_t depth) { return nested(depth - 1, "[", "0", "]"); });
    expect_depth_limit<RecNest>([](std::size_t depth) { return nested(depth - 1, "[", "[]", "]"); });
    expect_depth_limit<RecNestHolder>([](std::size_t depth) {
        return R"({"nest":)" + nested(depth - 1, "[", "[]", "]") + "}";
    });

    // the limit counts the values one inside the other, not all of them
    auto wide = std::string(R"({"kids":[)");
    for (auto i = 0; i < 1000; ++i) wide += std::string(i == 0 ? "" : ",") + R"({"kids":[{}]})";
    wide += "]}";
    EXPECT_EQ(yyjson::read<RecTree>(wide).kids.size(), 1000);
    EXPECT_EQ(yyjson::read(wide).cast<RecTree>().kids.size(), 1000);

    // writing has no limit
    auto deep = RecTree();
    for (auto i = 0; i < 1000; ++i) deep = RecTree{i, {std::move(deep)}};
    EXPECT_EQ(std::string_view(yyjson::write(deep)), std::string_view(yyjson::value(deep).write()));
}

TEST(Reader, RecursiveVariantLimit)
{
    const auto read_all = []<typename T>(std::type_identity<T>, const std::string& json) {
        EXPECT_THROW((void)yyjson::read(json).template cast<T>(), yyjson::bad_cast);
        EXPECT_THROW((void)yyjson::read<T>(json), yyjson::bad_cast);
        auto into = T();
        EXPECT_THROW(yyjson::read_into(json, into), yyjson::bad_cast);
        EXPECT_THROW(yyjson::merge_into(json, into), yyjson::bad_cast);
        EXPECT_EQ(yyjson::detail::recursion_depth, 0);
        EXPECT_EQ(yyjson::detail::variant_rereads, 1);
    };
    // both alternatives read the nested values before the innermost fails, which would take 2^100 reads of it
    read_all(std::type_identity<RecExpr>(),
             nested(100, R"({"op":{"args":[)", R"({"op":{"args":1}})", "]}}"));
    read_all(std::type_identity<RecExpr>(),
             nested(5, R"({"op":{"args":[)", R"({"op":{"args":1}})", "]}}"));

    // the second alternative is read after the first fails in up to 8 variants with 2 such alternatives each
    const auto tagged = [](std::size_t depth, std::string_view x) {
        return nested(depth - 1, R"({"op":{"args":[)", std::format(R"({{"op":{{"x":{}}}}})", x),
                      std::format(R"(],"x":{}}}}})", x));
    };
    expect_recursive<RecTagged>(tagged(8, R"("s")"));
    EXPECT_EQ(yyjson::read<RecTagged>(tagged(8, R"("s")")).op.index(), 1);
    read_all(std::type_identity<RecTagged>(), tagged(9, R"("s")"));
    // and the first alternative is read at any depth
    expect_recursive<RecTagged>(tagged(100, "1"));
    EXPECT_EQ(yyjson::read<RecTagged>(tagged(100, "1")).op.index(), 0);
}

// NOLINTEND
