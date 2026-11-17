#include "test_common.hpp"
#include "test_validation.hpp"

// NOLINTBEGIN

// `yyjson::transparent` reads and writes a struct as the value of its only field: an integer that is a user id, a
// list of tags, an object of another struct
struct UserId
{
    int value = 0;
    bool operator==(const UserId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UserId> = yyjson::annotate(yyjson::transparent);
struct Tags
{
    std::vector<std::string> v;
    bool operator==(const Tags&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Tags> = yyjson::annotate(yyjson::transparent);
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
// an optional value with a default, which a field of a struct leaves out when it is empty
struct Limit
{
    std::optional<int> o = 7;
    bool operator==(const Limit&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Limit> = yyjson::annotate(yyjson::transparent);
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
inline constexpr auto yyjson::annotations<VisitableId> = yyjson::annotate(yyjson::transparent);
VISITABLE_STRUCT(VisitableId, v);

#if FIELD_RFL_ANNOTATIONS
struct [[= yyjson::transparent]] AnnotatedId
{
    int value = 1;
    bool operator==(const AnnotatedId&) const = default;
};
#endif

// The rules that the static assertions of `yyjson::transparent` enforce, on the error of the table of the keys of a
// struct of the members `v` and `w`
namespace transparent_rules
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
    using yyjson::field, yyjson::skip, yyjson::transparent;
    static_assert(table_error(transparent, transparent, field("w", skip)) == transparent_twice);
    static_assert(table_error(transparent) == transparent_field_count);
    static_assert(table_error(transparent, field("v", skip), field("w", skip)) == transparent_field_count);
    static_assert(table_error(transparent, field("w", skip), field("v", yyjson::rename("x"))) ==
                  transparent_field_setting);
    static_assert(table_error(transparent, field("w", skip), field("v", yyjson::adjacent_tag("t", {"x"}))) ==
                  transparent_field_setting);
    static_assert(table_error(field("w", skip), transparent) == none);
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

    // every read of `json` gives `expected`, and the reader without a document reads it and merges it
    template <typename T>
    void expect_read(std::string_view json, const T& expected)
    {
        const auto result = read_or_error([&]() -> const T& { return expected; });
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        auto into = T();
        yyjson::read_into(json, into);
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
#endif
    }

    // every read of `json`, with and without a document, throws `bad_cast` about the value at `path`, and the
    // reader without a document leaves it to the document
    template <typename T>
    void expect_rejected(std::string_view json, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) { EXPECT_EQ(e.path(), path) << json << "\n" << e.what(); };
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

    // writing gives `json` with and without a document
    template <typename T>
    void expect_written(const T& t, std::string_view json)
    {
        EXPECT_EQ(yyjson::write(t), json);
        EXPECT_EQ(yyjson::value(t).write(), json);
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

    // an error in the field is at the path of the struct
    expect_rejected<UserId>(R"("3")", "");
    expect_rejected<Tags>(R"(["a",1])", "/1");

    // the fields of the struct in the field
    expect_read<Account>(
        R"({"id":3,"tags":["a"],"other":4,"ids":[1,2],"by_name":{"x":4},"w":{"a":5,"b":"yy"},"limit":9})",
        {{3}, {{"a"}}, UserId{4}, {{1}, {2}}, {{"x", {4}}}, {{5, "yy"}}, {9}});
    expect_written(Account{{3}, {{"a"}}, UserId{4}, {{1}, {2}}, {{"x", {4}}}, {{5, "yy"}}, {9}},
                   R"({"id":3,"tags":["a"],"other":4,"ids":[1,2],"by_name":{"x":4},"w":{"a":5,"b":"yy"},"limit":9})");
    expect_rejected<Account>(R"({"id":"3"})", "/id");
    expect_rejected<Account>(R"({"tags":["a",1]})", "/tags/1");
    expect_rejected<Account>(R"({"other":"4"})", "/other");
    expect_rejected<Account>(R"({"ids":[1,"2"]})", "/ids/1");
    expect_rejected<Account>(R"({"by_name":{"x":"4"}})", "/by_name/x");
    expect_rejected<Account>(R"({"w":{"a":"5"}})", "/w/a");
    expect_rejected<Account>(R"({"limit":"9"})", "/limit");
    const auto position = bad_cast_of([] { (void)yyjson::read<Account>(R"({"id":"3"})"); }).position();
    ASSERT_TRUE(position.has_value());
    EXPECT_EQ(position->pos, 6);

    // a field of an empty optional value is left out, and null in it is read as a missing key, as for a field of
    // std::optional; `merge_into` resets it to the default
    auto empty = Account();
    empty.limit.o.reset();
    expect_written(empty, R"({"id":1,"tags":["a"],"ids":[],"by_name":{},"w":{"a":1,"b":"x"}})");
    expect_read<Account>(R"({"id":1,"tags":["a"],"ids":[],"by_name":{},"w":{"a":1,"b":"x"}})", Account());
    expect_read<Account>(R"({"limit":null})", Account());
    auto into = Account();
    into.limit.o = 9;
    yyjson::read_into(R"({"limit":null})", into);
    EXPECT_EQ(into, Account());
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
    EXPECT_THROW((void)yyjson::read<IdOrName>("true"), yyjson::bad_cast);

    // an internal tag goes in the object of the field
    expect_read<TShape>(R"({"t":"w","a":3,"b":"z"})", Wrapped{{3, "z"}});
    expect_read<TShape>(R"({"b":"z","a":3,"t":"w"})", Wrapped{{3, "z"}});
    expect_written(TShape(Wrapped{{3, "z"}}), R"({"t":"w","a":3,"b":"z"})");
    expect_read<TShape>(R"({"t":"c","r":1})", TCircle{1});
}

TEST(Transparent, Visitable)
{
    expect_read<VisitableId>("3", {3});
    expect_written(VisitableId{3}, "3");
    expect_rejected<VisitableId>("true", "");
    expect_read<std::vector<VisitableId>>("[1,2]", {{1}, {2}});
    expect_rejected<std::vector<VisitableId>>("[1,true]", "/1");
}

// the keywords of the fields of transparent structs, which `yyjson::validate` checks where the value is written
struct CheckedId
{
    int value = 1;
    bool operator==(const CheckedId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CheckedId> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::minimum(1)));
struct CheckedTags
{
    std::vector<std::string> v = {"a"};
    bool operator==(const CheckedTags&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CheckedTags> = yyjson::annotate(
    yyjson::transparent, yyjson::field("v", yyjson::min_items(1), yyjson::items(yyjson::max_length(3))));
struct CheckedLimit
{
    std::optional<int> o = 7;
    bool operator==(const CheckedLimit&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CheckedLimit> =
    yyjson::annotate(yyjson::transparent, yyjson::field("o", yyjson::maximum(10)));
struct CheckedAccount
{
    CheckedId id;
    CheckedTags tags;
    std::optional<CheckedId> other;
    std::vector<CheckedId> ids;
    std::map<std::string, CheckedId> by_name;
    CheckedLimit limit;
    bool operator==(const CheckedAccount&) const = default;
};

TEST(Transparent, Validate)
{
    expect_valid(CheckedAccount());
    auto bad = CheckedAccount();
    bad.ids = {{1}, {0}};
    expect_invalid(bad, "the number is less than the minimum 1 at /ids/1; reading CheckedId", "/ids/1");
    bad = CheckedAccount();
    bad.tags.v.clear();
    expect_invalid(bad, "the array has fewer items than the minimum 1 at /tags; reading CheckedTags", "/tags");
    bad = CheckedAccount();
    bad.tags.v = {"a", "long"};
    expect_invalid(bad, "the string is longer than the maximum length 3 at /tags/1; reading an element of CheckedTags",
                   "/tags/1");
    bad = CheckedAccount();
    bad.limit.o = 11;
    expect_invalid(bad, "the number is greater than the maximum 10 at /limit; reading CheckedLimit", "/limit");
    bad = CheckedAccount();
    bad.by_name = {{"k", {0}}};
    expect_invalid(bad, "the number is less than the minimum 1 at /by_name/k; reading CheckedId", "/by_name/k");
    expect_invalid(CheckedId{0}, "the number is less than the minimum 1; reading CheckedId", "");
    expect_invalid(std::vector{CheckedId{0}}, "the number is less than the minimum 1 at /0; reading CheckedId", "/0");
}

#if FIELD_RFL_ANNOTATIONS
TEST(Transparent, Annotations)
{
    expect_read<AnnotatedId>("3", {3});
    expect_written(AnnotatedId{3}, "3");
    expect_rejected<AnnotatedId>("true", "");
}
#endif

// NOLINTEND
