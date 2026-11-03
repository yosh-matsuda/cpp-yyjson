#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::transparent` reads and writes a struct as the value of its only field, which takes the constraints that a
// caster cannot carry: an integer that is a user id, a list of tags, an object of another struct
struct UserId
{
    int value = 0;
    bool operator==(const UserId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UserId> =
    yyjson::annotate(yyjson::transparent, yyjson::description("A user"), yyjson::field("value", yyjson::minimum(1)));
struct Tags
{
    std::vector<std::string> v;
    bool operator==(const Tags&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Tags> = yyjson::annotate(
    yyjson::transparent, yyjson::field("v", yyjson::min_items(1), yyjson::items(yyjson::max_length(3))));
struct Inner
{
    int a = 1;
    std::string b = "x";
    bool operator==(const Inner&) const = default;
};
struct Wrapped
{
    Inner in;
    bool operator==(const Wrapped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Wrapped> = yyjson::annotate(yyjson::transparent);
// an optional value with a default, which a field of the struct leaves out when it is empty
struct Limit
{
    std::optional<int> o = 7;
    bool operator==(const Limit&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Limit> =
    yyjson::annotate(yyjson::transparent, yyjson::field("o", yyjson::maximum(10)));
// a skipped field keeps the value it has in `T()`
struct Cached
{
    int hits = 5;
    std::string name;
    bool operator==(const Cached&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Cached> =
    yyjson::annotate(yyjson::transparent, yyjson::field("hits", yyjson::skip));
struct Account
{
    UserId id{1};
    Tags tags{{"a"}};
    std::optional<UserId> other;
    std::vector<UserId> ids;
    std::map<std::string, UserId> by_name;
    Wrapped w;
    Limit limit;
    bool operator==(const Account&) const = default;
};

// the alternatives of variants
struct TCircle
{
    int r = 0;
    bool operator==(const TCircle&) const = default;
};
using TShape = std::variant<TCircle, Wrapped>;
template <>
inline constexpr auto yyjson::annotations<TShape> = yyjson::annotate(yyjson::internal_tag("t", {"c", "w"}));
using IdOrName = std::variant<UserId, std::string>;

struct VisitableId
{
    int v = 0;
    bool operator==(const VisitableId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VisitableId> =
    yyjson::annotate(yyjson::transparent, yyjson::field("v", yyjson::minimum(0)));
VISITABLE_STRUCT(VisitableId, v);

#if FIELD_RFL_ANNOTATIONS
struct [[= yyjson::transparent]] AnnotatedId
{
    [[= yyjson::minimum(1)]] int value = 1;
    bool operator==(const AnnotatedId&) const = default;
};
#endif

// The rules that the static assertions of `yyjson::transparent` enforce, on the error of the table of the keys
#define TRANSPARENT_RULE(NAME, ...) \
    struct NAME                     \
    {                               \
        int v = 0;                  \
        int w = 0;                  \
    };                              \
    template <>                     \
    inline constexpr auto yyjson::annotations<NAME> = yyjson::annotate(__VA_ARGS__);
TRANSPARENT_RULE(TransparentTwice, yyjson::transparent, yyjson::transparent, yyjson::field("w", yyjson::skip))
TRANSPARENT_RULE(TransparentTwoFields, yyjson::transparent)
TRANSPARENT_RULE(TransparentNoField, yyjson::transparent, yyjson::field("v", yyjson::skip),
                 yyjson::field("w", yyjson::skip))
TRANSPARENT_RULE(TransparentRenamed, yyjson::transparent, yyjson::field("w", yyjson::skip),
                 yyjson::field("v", yyjson::rename("x")))
TRANSPARENT_RULE(TransparentRequired, yyjson::transparent, yyjson::field("w", yyjson::skip),
                 yyjson::field("v", yyjson::required))
TRANSPARENT_RULE(TransparentOmitted, yyjson::transparent, yyjson::field("w", yyjson::skip),
                 yyjson::field("v", yyjson::omit_default))
TRANSPARENT_RULE(TransparentDescribedField, yyjson::transparent, yyjson::field("w", yyjson::skip),
                 yyjson::field("v", yyjson::description("d")))
TRANSPARENT_RULE(TransparentClosed, yyjson::transparent, yyjson::additional_properties(false),
                 yyjson::field("w", yyjson::skip))
TRANSPARENT_RULE(TransparentOpen, yyjson::transparent, yyjson::additional_properties(true),
                 yyjson::field("w", yyjson::skip))
TRANSPARENT_RULE(TransparentCounted, yyjson::transparent, yyjson::max_properties(1), yyjson::field("w", yyjson::skip))
TRANSPARENT_RULE(TransparentConstrained, yyjson::transparent, yyjson::description("d"),
                 yyjson::field("w", yyjson::skip), yyjson::field("v", yyjson::minimum(0), yyjson::maximum(9)))
#undef TRANSPARENT_RULE
namespace transparent_rules
{
    template <typename T>
    constexpr auto table_error =
        yyjson::detail::make_field_key_table<T, yyjson::detail::reflected_field_names<T>>().error;
    using enum yyjson::detail::field_key_error;
    static_assert(table_error<TransparentTwice> == transparent_twice);
    static_assert(table_error<TransparentTwoFields> == transparent_field_count);
    static_assert(table_error<TransparentNoField> == transparent_field_count);
    static_assert(table_error<TransparentRenamed> == transparent_field_setting);
    static_assert(table_error<TransparentRequired> == transparent_field_setting);
    static_assert(table_error<TransparentOmitted> == transparent_field_setting);
    static_assert(table_error<TransparentDescribedField> == transparent_field_setting);
    static_assert(table_error<TransparentClosed> == transparent_struct_setting);
    static_assert(table_error<TransparentOpen> == transparent_struct_setting);
    static_assert(table_error<TransparentCounted> == transparent_struct_setting);
    static_assert(table_error<TransparentConstrained> == none);

    // a field of a transparent struct of an optional value is optional, as the optional value would be
    static_assert(yyjson::detail::optional_field_type<Limit>());
    static_assert(!yyjson::detail::optional_field_type<UserId>());
}  // namespace transparent_rules

namespace
{
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

    // every read of `json`, with and without `yyjson::strict`, gives `expected`, and the reader without a document
    // reads it and merges it
    template <typename T>
    void expect_read(std::string_view json, const T& expected)
    {
        const auto result = read_or_error([&]() -> const T& { return expected; });
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read<T>(json, yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(yyjson::strict), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        auto into = T();
        yyjson::read_into(json, into, yyjson::strict);
        EXPECT_EQ(into, expected) << json;
        EXPECT_TRUE(expect_read_into(T(), json)) << json;
        auto merged_directly = false;
        EXPECT_EQ(expect_merge_into(T(), json, &merged_directly), result) << json;
#if YYJSON_VERSION_HEX >= 0x000C00
        // the reader without a document merges objects and arrays
        EXPECT_EQ(merged_directly, json.starts_with('{') || json.starts_with('[')) << json;
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_TRUE(direct.has_value()) << json;
        EXPECT_EQ(*direct, result) << json;
        const auto scope = yyjson::detail::strict_scope();
        const auto strict = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        ASSERT_TRUE(strict.has_value()) << json;
        EXPECT_EQ(*strict, result) << json;
#endif
    }

    // every read of `json`, with and without a document, throws `bad_cast` with `message` about the value at
    // `path`, and the reader without a document leaves it to the document
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view message, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            EXPECT_EQ(message_of(e).substr(0, message.size()), message) << json << "\n" << e.what();
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json); }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::read_into(json, t);
        }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(); }));
#if YYJSON_VERSION_HEX >= 0x000C00
        EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
        auto merged_directly = false;
        static_cast<void>(expect_merge_into(T(), json, &merged_directly));
        EXPECT_FALSE(merged_directly) << json;
#endif
    }

    // writing gives `json` with and without a document, and so does writing under `yyjson::strict`
    template <typename T>
    void expect_written(const T& t, std::string_view json)
    {
        EXPECT_EQ(yyjson::write(t), json);
        EXPECT_EQ(yyjson::value(t).write(), json);
        EXPECT_EQ(yyjson::write(t, yyjson::strict), json);
        expect_direct_write(t);
    }
}  // namespace

TEST(Transparent, Values)
{
    // a struct is read and written as the value of its field
    expect_read<UserId>("3", {3});
    expect_written(UserId{3}, "3");
    expect_read<Tags>(R"(["a","bc"])", {{"a", "bc"}});
    expect_written(Tags{{"a", "bc"}}, R"(["a","bc"])");
    expect_read<Wrapped>(R"({"a":2,"b":"q"})", {{2, "q"}});
    expect_written(Wrapped{{2, "q"}}, R"({"a":2,"b":"q"})");
    expect_read<Limit>("3", {3});
    expect_written(Limit{3}, "3");
    expect_read<Cached>(R"("n")", {5, "n"});
    expect_written(Cached{9, "n"}, R"("n")");
    // `yyjson::object` takes a struct whose field is written as an object
    EXPECT_EQ(yyjson::object(Wrapped{{2, "q"}}).write(), R"({"a":2,"b":"q"})");
    EXPECT_EQ(yyjson::object(Wrapped{{2, "q"}}, yyjson::copy_string).write(), R"({"a":2,"b":"q"})");
    static_assert(!std::is_constructible_v<yyjson::object, UserId>);
    static_assert(!std::is_constructible_v<yyjson::object, Tags>);

    // the field's constraints are the struct's, at the path of the struct, and its type is named in the message
    expect_rejected<UserId>("0", "the value of UserId is less than the minimum 1", "");
    expect_rejected<UserId>(R"("3")", "", "");
    expect_rejected<Tags>("[]", "the value of Tags has fewer items than the minimum 1", "");
    expect_rejected<Tags>(R"(["a","long"])", "an element of the value of Tags is longer than the maximum length 3", "/1");
    expect_rejected<Limit>("11", "the value of Limit is greater than the maximum 10", "");

    // the fields of the struct in the field
    expect_read<Account>(
        R"({"id":3,"tags":["a"],"other":4,"ids":[1,2],"by_name":{"x":4},"w":{"a":5,"b":"yy"},"limit":9})",
        {{3}, {{"a"}}, UserId{4}, {{1}, {2}}, {{"x", {4}}}, {{5, "yy"}}, {9}});
    expect_written(Account{{3}, {{"a"}}, UserId{4}, {{1}, {2}}, {{"x", {4}}}, {{5, "yy"}}, {9}},
                   R"({"id":3,"tags":["a"],"other":4,"ids":[1,2],"by_name":{"x":4},"w":{"a":5,"b":"yy"},"limit":9})");
    expect_rejected<Account>(R"({"id":0})", "the value of UserId is less than the minimum 1", "/id");
    expect_rejected<Account>(R"({"tags":["a","long"]})",
                             "an element of the value of Tags is longer than the maximum length 3", "/tags/1");
    expect_rejected<Account>(R"({"other":0})", "the value of UserId is less than the minimum 1", "/other");
    expect_rejected<Account>(R"({"ids":[1,0]})", "the value of UserId is less than the minimum 1", "/ids/1");
    expect_rejected<Account>(R"({"by_name":{"x":0}})", "the value of UserId is less than the minimum 1",
                             "/by_name/x");
    expect_rejected<Account>(R"({"limit":11})", "the value of Limit is greater than the maximum 10", "/limit");
    const auto position = bad_cast_of([] { (void)yyjson::read<Account>(R"({"id":0})"); }).position();
    ASSERT_TRUE(position.has_value());
    EXPECT_EQ(position->pos, 6);

    // a field of an empty optional value is left out, which `yyjson::strict` accepts, and null in it is read as a
    // missing key, as for a field of std::optional, which `yyjson::strict` rejects; `merge_into` resets it to the
    // default
    auto empty = Account();
    empty.limit.o.reset();
    expect_written(empty, R"({"id":1,"tags":["a"],"ids":[],"by_name":{},"w":{"a":1,"b":"x"}})");
    expect_read<Account>(R"({"id":1,"tags":["a"],"ids":[],"by_name":{},"w":{"a":1,"b":"x"}})", Account());
    EXPECT_EQ(yyjson::read<Account>(R"({"limit":null})"), Account());
    EXPECT_EQ(yyjson::read(R"({"limit":null})").cast<Account>(), Account());
    auto into = Account();
    into.limit.o = 9;
    yyjson::read_into(R"({"limit":null})", into);
    EXPECT_EQ(into, Account());
    constexpr auto strict_null = R"({"id":1,"tags":["a"],"ids":[],"by_name":{},"w":{"a":1,"b":"x"},"limit":null})";
    for (const auto& read : std::array<std::function<void()>, 2>{
             [&] { (void)yyjson::read<Account>(strict_null, yyjson::strict); },
             [&] { (void)yyjson::read(strict_null).cast<Account>(yyjson::strict); }})
    {
        const auto e = bad_cast_of(read);
        EXPECT_TRUE(message_of(e).starts_with("std::optional<int> is not constructible from null under yyjson::strict"))
            << e.what();
        EXPECT_EQ(e.path(), "/limit");
    }
    EXPECT_EQ(yyjson::read<Limit>("null"), Limit{std::nullopt});
    auto merged = Account();
    merged.limit.o = 9;
    yyjson::merge_into(R"({"limit":null})", merged);
    EXPECT_EQ(merged, Account());
    // the members that the object leaves out are kept by `merge_into`, also in the object of a transparent struct
    auto kept = Account();
    kept.w.in.b = "keep";
    kept.limit.o = 9;
    EXPECT_EQ(expect_merge_into(kept, R"({"w":{"a":8},"tags":["z"]})"),
              yyjson::write(Account{{1}, {{"z"}}, {}, {}, {}, {{8, "keep"}}, {9}}));

    // elsewhere, null empties the optional value, as for std::optional, and an empty one is written as null
    expect_read<Limit>("null", Limit{std::nullopt});
    expect_read<std::vector<Limit>>("[null,3]", {Limit{std::nullopt}, Limit{3}});
    const auto map = std::map<std::string, Limit>{{"a", Limit{std::nullopt}}};
    EXPECT_EQ((yyjson::read<std::map<std::string, Limit>>(R"({"a":null})")), map);
    EXPECT_EQ((yyjson::read(R"({"a":null})").cast<std::map<std::string, Limit>>()), map);
    expect_written(std::vector<Limit>{Limit{std::nullopt}, Limit{3}}, "[null,3]");
    auto over = std::vector<Limit>{Limit{5}};
    yyjson::read_into("[null]", over);
    EXPECT_EQ(over, std::vector<Limit>{Limit{std::nullopt}});
}

TEST(Transparent, Variants)
{
    // an alternative is read as the value of its field, also to choose the alternative
    expect_read<IdOrName>("3", UserId{3});
    expect_read<IdOrName>(R"("n")", std::string("n"));
    expect_written(IdOrName(UserId{3}), "3");
    EXPECT_THROW((void)yyjson::read<IdOrName>("0"), yyjson::bad_cast);

    // an internal tag goes in the object of the field
    expect_read<TShape>(R"({"t":"w","a":3,"b":"z"})", Wrapped{{3, "z"}});
    expect_read<TShape>(R"({"b":"z","a":3,"t":"w"})", Wrapped{{3, "z"}});
    expect_written(TShape(Wrapped{{3, "z"}}), R"({"t":"w","a":3,"b":"z"})");
    expect_read<TShape>(R"({"t":"c","r":1})", TCircle{1});
}

TEST(Transparent, Validate)
{
    // `validate` and the strict write check the field's constraints at the path where the value is written
    EXPECT_NO_THROW(yyjson::validate(Account()));
    auto bad = Account();
    bad.ids = {{1}, {0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(bad); }).path(), "/ids/1");
    EXPECT_EQ(bad_cast_of([&] { (void)yyjson::write(bad, yyjson::strict); }).path(), "/ids/1");
    EXPECT_EQ(yyjson::write(bad), R"({"id":1,"tags":["a"],"ids":[1,0],"by_name":{},"w":{"a":1,"b":"x"},"limit":7})");
    bad = Account();
    bad.tags.v.clear();
    EXPECT_EQ(message_of(bad_cast_of([&] { yyjson::validate(bad); })),
              "the value of Tags has fewer items than the minimum 1 at /tags");
    bad = Account();
    bad.limit.o = 11;
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(bad); }).path(), "/limit");
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(UserId{0}); }).path(), "");
    EXPECT_THROW((void)yyjson::write(std::vector{UserId{0}}, yyjson::strict), yyjson::bad_cast);
}

TEST(Transparent, Visitable)
{
    expect_read<VisitableId>("3", {3});
    expect_written(VisitableId{3}, "3");
    expect_rejected<VisitableId>("-1", "the value of VisitableId is less than the minimum 0", "");
    expect_read<std::vector<VisitableId>>("[1,2]", {{1}, {2}});
    expect_rejected<std::vector<VisitableId>>("[1,-2]", "the value of VisitableId is less than the minimum 0", "/1");
}

#if FIELD_RFL_ANNOTATIONS
TEST(Transparent, Annotations)
{
    expect_read<AnnotatedId>("3", {3});
    expect_written(AnnotatedId{3}, "3");
    expect_rejected<AnnotatedId>("0", "the value of AnnotatedId is less than the minimum 1", "");
}
#endif

// NOLINTEND
