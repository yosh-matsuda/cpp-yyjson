#include <map>
#include <memory>
#include <tuple>
#include <variant>

#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::validate` checks a C++ value as `yyjson::read<T>(text, yyjson::strict)` checks what `yyjson::write`
// writes of it
template <typename T>
struct ValidateMaybe
{
    bool has = false;
    T value{};
};
template <typename T>
struct yyjson::caster<ValidateMaybe<T>>
{
    static bool should_omit(const ValidateMaybe<T>& m) { return !m.has; }
    static T to_json(const ValidateMaybe<T>& m) { return m.value; }
    static ValidateMaybe<T> from_json(const yyjson::json_view& v) { return {true, v.cast<T>()}; }
};

struct ValidateFields
{
    int x = 0;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::optional<int> o;
};
template <>
inline constexpr auto yyjson::annotations<ValidateFields> = yyjson::annotate(
    yyjson::field("x", yyjson::minimum(0)),
    yyjson::field("v", yyjson::items(yyjson::maximum(5)), yyjson::unique_items),
    yyjson::field("m", yyjson::additional_properties(yyjson::minimum(1)),
                  yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("o", yyjson::maximum(9)));

struct ValidateElements
{
    std::tuple<int, std::string> t;
    std::vector<int> r;
    std::vector<int> c = {10};
    std::map<int, int> im;
    std::vector<std::optional<int>> oo;
    std::vector<std::pair<std::string, int>> vp;
};
template <>
inline constexpr auto yyjson::annotations<ValidateElements> =
    yyjson::annotate(yyjson::field("t", yyjson::prefix_items(yyjson::minimum(0), yyjson::max_length(2))),
                     yyjson::field("r", yyjson::prefix_items({yyjson::minimum(0)}, {}),
                                   yyjson::items(yyjson::maximum(9))),
                     yyjson::field("c", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(1)),
                     yyjson::field("im", yyjson::additional_properties(yyjson::maximum(3))),
                     yyjson::field("oo", yyjson::items(yyjson::minimum(0))),
                     yyjson::field("vp", yyjson::unique_items));

struct ValidateKeys
{
    std::optional<int> a;
    std::optional<int> b;
    ValidateMaybe<int> c;
    ValidateMaybe<int> d;
};
template <>
inline constexpr auto yyjson::annotations<ValidateKeys> =
    yyjson::annotate(yyjson::field("d", yyjson::optional_key), yyjson::dependent_required("a", {"b", "d"}));
struct ValidateGroups
{
    std::optional<int> a;
    ValidateMaybe<int> b;
};
template <>
inline constexpr auto yyjson::annotations<ValidateGroups> =
    yyjson::annotate(yyjson::one_of_required({"a"}, {"b"}), yyjson::field("b", yyjson::optional_key));
struct ValidateRequired
{
    ValidateMaybe<int> c;
};
template <>
inline constexpr auto yyjson::annotations<ValidateRequired> = yyjson::annotate(yyjson::field("c", yyjson::required));

struct ValidateCircle
{
    double r = 0;
};
struct ValidateRect
{
    double w = 0;
};
struct ValidateEnvelope
{
    std::string kind;
    std::variant<ValidateCircle, ValidateRect> data;
};
template <>
inline constexpr auto yyjson::annotations<ValidateEnvelope> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));

struct ValidateTree
{
    int n = 0;
    std::vector<ValidateTree> kids;
    std::unique_ptr<ValidateTree> next;
};
template <>
inline constexpr auto yyjson::annotations<ValidateTree> = yyjson::annotate(yyjson::field("n", yyjson::maximum(9)));

struct ValidateVisitable
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<ValidateVisitable> = yyjson::annotate(yyjson::field("a", yyjson::minimum(1)));
VISITABLE_STRUCT(ValidateVisitable, a);

struct ValidateDuplicates
{
    std::multimap<std::string, int> mm;
    std::vector<std::pair<std::string, int>> vp;
};

struct ValidateOrder
{
    ValidateMaybe<int> c;
    int y = 0;
    std::vector<ValidateFields> list;
};
template <>
inline constexpr auto yyjson::annotations<ValidateOrder> =
    yyjson::annotate(yyjson::field("y", yyjson::minimum(0)), yyjson::field("list", yyjson::max_items(1)));

enum class ValidateColor
{
    red,
    green
};
inline bool validate_set(const std::shared_ptr<int>& p) { return p != nullptr; }
inline bool validate_red(ValidateColor c) { return c == ValidateColor::red; }
struct ValidateNulls
{
    std::shared_ptr<int> p = std::make_shared<int>(0);
    std::map<ValidateColor, int> colors;
};
template <>
inline constexpr auto yyjson::annotations<ValidateNulls> =
    yyjson::annotate(yyjson::field("p", yyjson::satisfies<validate_set>),
                     yyjson::field("colors", yyjson::property_names(yyjson::satisfies<validate_red>)));

struct ValidateNullables
{
    std::variant<std::monostate, std::string> code;
    std::variant<double, std::monostate> ratio = std::monostate();
    std::unique_ptr<int> count;
    std::optional<std::unique_ptr<int>> level;
    std::vector<std::unique_ptr<std::string>> tags;
    std::vector<std::shared_ptr<int>> ids;
};
template <>
inline constexpr auto yyjson::annotations<ValidateNullables> =
    yyjson::annotate(yyjson::field("code", yyjson::max_length(3)), yyjson::field("ratio", yyjson::maximum(1)),
                     yyjson::field("count", yyjson::minimum(0)), yyjson::field("level", yyjson::maximum(5)),
                     yyjson::field("tags", yyjson::items(yyjson::max_length(3))),
                     yyjson::field("ids", yyjson::unique_items));

using ValidateSignedOrUnsigned = yyjson::one_of<std::int64_t, std::uint64_t>;
// some alternatives other than the last one may read its values and some may not
using ValidatePartlyOverlapping =
    yyjson::one_of<yyjson::alt<double, yyjson::maximum(0.5)>, std::string,
                   yyjson::alt<std::int64_t, yyjson::minimum(-100), yyjson::maximum(10)>, std::uint64_t>;
using ValidateLowOrShort =
    yyjson::any_of<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<std::string, yyjson::max_length(3)>>;
using ValidateItemsOrText =
    yyjson::any_of<yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(0))>, std::string>;
struct ValidateAlternatives
{
    ValidateLowOrShort a = 0;
    std::vector<ValidateSignedOrUnsigned> b;
    ValidateItemsOrText c = std::string();
};

namespace
{
    // a value of T that `set` changes
    template <typename T, typename F>
    T with(F&& set)
    {
        auto t = T();
        set(t);
        return t;
    }

    template <typename F>
    yyjson::bad_cast validate_error(F&& f)
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

    // `yyjson::validate(t)` throws `message` at `path`, and so does the strict read of what `t` is written as
    template <typename T>
    void expect_invalid(const T& t, std::string_view message, std::string_view path)
    {
        const auto e = validate_error([&] { yyjson::validate(t); });
        EXPECT_EQ(message_of(e), std::string(message) + (path.empty() ? "" : " at " + std::string(path)));
        EXPECT_EQ(e.path(), path);
        EXPECT_FALSE(e.position().has_value());
        const auto json = std::string(yyjson::write(t));
        const auto r = validate_error([&] { (void)yyjson::read<T>(json, yyjson::strict); });
        EXPECT_TRUE(message_of(r).starts_with(message)) << json << "\n" << r.what();
        EXPECT_EQ(r.path(), path) << json;
        const auto w = validate_error([&] { (void)yyjson::write(t, yyjson::strict); });
        EXPECT_EQ(std::string_view(w.what()), std::string_view(e.what()));
    }

    // `yyjson::validate(t)` throws `message` at `path`, and so does `write(t, strict)`; the strict read of what `t` is
    // written as throws at `read_path`
    template <typename T>
    void expect_invalid_alternative(const T& t, std::string_view message, std::string_view path,
                                    std::string_view read_path)
    {
        const auto e = validate_error([&] { yyjson::validate(t); });
        EXPECT_EQ(message_of(e), std::string(message) + " at " + std::string(path));
        EXPECT_EQ(e.path(), path);
        const auto json = std::string(yyjson::write(t));
        const auto r = validate_error([&] { (void)yyjson::read<T>(json, yyjson::strict); });
        EXPECT_EQ(r.path(), read_path) << json;
        const auto w = validate_error([&] { (void)yyjson::write(t, yyjson::strict); });
        EXPECT_EQ(std::string_view(w.what()), std::string_view(e.what()));
    }

    template <typename V>
    std::string type_of()
    {
        return without_type_keywords(std::string(yyjson::detail::type_name<V>()));
    }

    template <typename T>
    void expect_valid(const T& t)
    {
        EXPECT_NO_THROW(yyjson::validate(t));
        EXPECT_NO_THROW((void)yyjson::read<T>(std::string(yyjson::write(t)), yyjson::strict));
        EXPECT_EQ(std::string(yyjson::write(t, yyjson::strict)), std::string(yyjson::write(t)));
    }
}  // namespace

TEST(Validate, Fields)
{
    expect_valid(ValidateFields{1, {1, 2}, {{"a", 1}}, 3});
    expect_valid(ValidateFields{});
    expect_invalid(with<ValidateFields>([](auto& t) { t.x = -1; }),
                   R"(the value of the key "x" of ValidateFields is less than the minimum 0)", "/x");
    expect_invalid(with<ValidateFields>([](auto& t) { t.o = 10; }),
                   R"(the value of the key "o" of ValidateFields is greater than the maximum 9)", "/o");
    expect_invalid(with<ValidateFields>([](auto& t) { t.v = {1, 9}; }),
                   R"(an element of the value of the key "v" of ValidateFields is greater than the maximum 5)", "/v/1");
    expect_invalid(with<ValidateFields>([](auto& t) { t.v = {1, 2, 1}; }),
                   R"(an element of the value of the key "v" of ValidateFields is equal to an earlier element)",
                   "/v/2");
    expect_invalid(with<ValidateFields>([](auto& t) { t.m = {{"a/b~", 0}}; }),
                   R"(an element of the value of the key "m" of ValidateFields is less than the minimum 1)",
                   "/m/a~1b~0");
    expect_invalid(with<ValidateFields>([](auto& t) { t.m = {{"long", 1}}; }),
                   R"(a key of the value of the key "m" of ValidateFields is longer than the maximum length 3)",
                   "/m/long");
}

TEST(Validate, Elements)
{
    expect_valid(ValidateElements{{0, "ab"}, {0, 9, 1}, {10, 1}, {{1, 3}}, {1, std::nullopt}, {{"a", 1}, {"a", 2}}});
    expect_invalid(with<ValidateElements>([](auto& t) { t.t = {0, "abc"}; }),
                   R"(an element of the value of the key "t" of ValidateElements is longer than the maximum length 2)",
                   "/t/1");
    expect_invalid(with<ValidateElements>([](auto& t) { t.r = {-1}; }),
                   R"(an element of the value of the key "r" of ValidateElements is less than the minimum 0)", "/r/0");
    // `items` applies after the prefix
    expect_invalid(with<ValidateElements>([](auto& t) { t.r = {0, 10, 10}; }),
                   R"(an element of the value of the key "r" of ValidateElements is greater than the maximum 9)",
                   "/r/2");
    expect_invalid(with<ValidateElements>([](auto& t) { t.c = {10, 11}; }),
                   R"(the value of the key "c" of ValidateElements has more than 1 item that satisfies the )"
                   R"(constraints of yyjson::contains)",
                   "/c");
    // a map whose keys are not strings is written as an array of pairs
    expect_invalid(with<ValidateElements>([](auto& t) { t.im = {{1, 2}, {5, 7}}; }),
                   R"(an element of the value of the key "im" of ValidateElements is greater than the maximum 3)",
                   "/im/1/1");
    expect_invalid(with<ValidateElements>([](auto& t) { t.oo = {1, std::nullopt, -3}; }),
                   R"(an element of the value of the key "oo" of ValidateElements is less than the minimum 0)",
                   "/oo/2");
    // a range of pairs is written as an object, in which a repeated key cannot be pointed to
    expect_invalid(with<ValidateElements>([](auto& t) { t.vp = {{"a", 1}, {"b", 2}, {"a", 1}}; }),
                   R"(the value of the key "vp" of ValidateElements has duplicate items)", "/vp");
}

TEST(Validate, Keys)
{
    expect_valid(ValidateKeys{{}, {}, {true, 1}, {}});
    expect_valid(ValidateKeys{1, 2, {true, 1}, {true, 2}});
    // a field that its caster leaves out is missing under `yyjson::strict` unless it is optional
    expect_invalid(ValidateKeys{}, R"(ValidateKeys is not constructible from JSON without the key "c")", "");
    expect_invalid(ValidateRequired{}, R"(ValidateRequired is not constructible from JSON without the key "c")", "");
    expect_invalid(ValidateKeys{1, 2, {true, 1}, {}},
                   R"(ValidateKeys is not constructible from JSON with the key "a" without the key "d")", "");
    expect_invalid(std::vector<ValidateKeys>{{{}, {}, {true, 1}, {}}, {1, {}, {true, 1}, {true, 1}}},
                   R"(ValidateKeys is not constructible from JSON with the key "a" without the key "b")", "/1");
    // the keys that the object has are the fields that are written
    expect_valid(ValidateGroups{1, {}});
    expect_valid(ValidateGroups{{}, {true, 1}});
    expect_invalid(ValidateGroups{},
                   R"(ValidateGroups is not constructible from JSON without all the keys of one of ["a"], ["b"])", "");
    expect_invalid(
        ValidateGroups{1, {true, 1}},
        R"(ValidateGroups is not constructible from JSON with all the keys of more than one of ["a"], ["b"])", "");
}

TEST(Validate, AdjacentTag)
{
    expect_valid(ValidateEnvelope{"rect", ValidateRect{}});
    const auto e = validate_error([] { yyjson::validate(ValidateEnvelope{"rect", ValidateCircle{}}); });
    EXPECT_EQ(message_of(e),
              R"(the field "kind" of ValidateEnvelope is "rect", but the field "data" holds the alternative "circle")");
    EXPECT_THROW((void)yyjson::write(ValidateEnvelope{"rect", ValidateCircle{}}), std::invalid_argument);
}

TEST(Validate, Nested)
{
    auto tree = ValidateTree();
    tree.kids.resize(2);
    expect_valid(tree);
    tree.kids[1].next = std::make_unique<ValidateTree>();
    tree.kids[1].next->n = 10;
    expect_invalid(tree, R"(the value of the key "n" of ValidateTree is greater than the maximum 9)",
                   "/kids/1/next/n");

    const auto negative = with<ValidateFields>([](auto& t) { t.x = -1; });
    expect_invalid(std::map<std::string, std::vector<ValidateFields>>{{"k", {{}, negative}}},
                   R"(the value of the key "x" of ValidateFields is less than the minimum 0)", "/k/1/x");
    expect_invalid(std::array<ValidateFields, 2>{ValidateFields{}, negative},
                   R"(the value of the key "x" of ValidateFields is less than the minimum 0)", "/1/x");
    expect_invalid(std::optional<ValidateVisitable>(ValidateVisitable{0}),
                   R"(the value of the key "a" of ValidateVisitable is less than the minimum 1)", "/a");
    expect_invalid(std::tuple<int, ValidateVisitable>{1, {0}},
                   R"(the value of the key "a" of ValidateVisitable is less than the minimum 1)", "/1/a");
    // a tuple of key-value pairs is written as an object
    using members = std::tuple<std::pair<std::string, int>, std::pair<std::string, ValidateVisitable>>;
    expect_invalid(members{{"i", 1}, {"v", {0}}},
                   R"(the value of the key "a" of ValidateVisitable is less than the minimum 1)", "/v/a");
    // but one with enumeration keys as an array of pairs
    expect_invalid(std::tuple<std::pair<ValidateColor, ValidateVisitable>>{{ValidateColor::green, {0}}},
                   R"(the value of the key "a" of ValidateVisitable is less than the minimum 1)", "/0/1/a");

    // an untagged variant is read by trying each alternative, so only `validate` names the field
    const auto e = validate_error([] { yyjson::validate(std::variant<int, ValidateVisitable>(ValidateVisitable{0})); });
    EXPECT_EQ(e.path(), "/a");
}

TEST(Validate, Order)
{
    // the read checks the values in a field before the field, and the missing keys after all fields
    expect_invalid(with<ValidateOrder>([](auto& t) { t.list = {with<ValidateFields>([](auto& f) { f.x = -1; }), {}}; }),
                   R"(the value of the key "x" of ValidateFields is less than the minimum 0)", "/list/0/x");
    expect_invalid(with<ValidateOrder>([](auto& t) { t.y = -1; }),
                   R"(the value of the key "y" of ValidateOrder is less than the minimum 0)", "/y");
    expect_invalid(ValidateOrder{}, R"(ValidateOrder is not constructible from JSON without the key "c")", "");
}

TEST(Validate, Nulls)
{
    // the read does not check the constraints of a field that is written as null
    expect_valid(with<ValidateNulls>([](auto& t) { t.p = nullptr; }));
    // a key that is not read from its text is reported at the field
    expect_invalid(with<ValidateNulls>([](auto& t) { t.colors = {{ValidateColor::green, 1}}; }),
                   R"(the value of the key "colors" of ValidateNulls has a key that does not satisfy the )"
                   R"(constraints of yyjson::property_names)",
                   "/colors");
}

TEST(Validate, Nullables)
{
    // the constraints check the value that a pointer or a variant with std::monostate holds, and null passes
    expect_valid(ValidateNullables{});
    expect_valid(with<ValidateNullables>([](auto& t) {
        t.code = "abc";
        t.ratio = 1.0;
        t.count = std::make_unique<int>(0);
        t.level = std::make_unique<int>(5);
        t.level.emplace();
        t.tags.push_back(nullptr);
        t.tags.push_back(std::make_unique<std::string>("abc"));
        t.ids = {nullptr, std::make_shared<int>(1), std::make_shared<int>(2)};
    }));
    expect_invalid(with<ValidateNullables>([](auto& t) { t.code = "abcd"; }),
                   R"(the value of the key "code" of ValidateNullables is longer than the maximum length 3)", "/code");
    expect_invalid(with<ValidateNullables>([](auto& t) { t.ratio = 2.0; }),
                   R"(the value of the key "ratio" of ValidateNullables is greater than the maximum 1)", "/ratio");
    expect_invalid(with<ValidateNullables>([](auto& t) { t.count = std::make_unique<int>(-1); }),
                   R"(the value of the key "count" of ValidateNullables is less than the minimum 0)", "/count");
    expect_invalid(with<ValidateNullables>([](auto& t) { t.level = std::make_unique<int>(6); }),
                   R"(the value of the key "level" of ValidateNullables is greater than the maximum 5)", "/level");
    expect_invalid(with<ValidateNullables>([](auto& t) {
                       t.tags.push_back(nullptr);
                       t.tags.push_back(std::make_unique<std::string>("abcd"));
                   }),
                   R"(an element of the value of the key "tags" of ValidateNullables is longer than the maximum )"
                   R"(length 3)",
                   "/tags/1");
    expect_invalid(with<ValidateNullables>([](auto& t) { t.ids = {nullptr, std::make_shared<int>(1), nullptr}; }),
                   R"(an element of the value of the key "ids" of ValidateNullables is equal to an earlier element)",
                   "/ids/2");
}

TEST(Validate, WriteStrict)
{
    const auto valid = ValidateFields{1, {1, 2}, {{"a", 1}}, 3};
    EXPECT_EQ(std::string(yyjson::write(valid, yyjson::WriteFlag::Pretty, yyjson::strict)),
              std::string(yyjson::write(valid, yyjson::WriteFlag::Pretty)));
    auto out = std::string("old");
    yyjson::write(valid, out, yyjson::strict);
    EXPECT_EQ(out, std::string(yyjson::write(valid)));
    yyjson::write(valid, out, yyjson::WriteFlag::Pretty, yyjson::strict);
    EXPECT_EQ(out, std::string(yyjson::write(valid, yyjson::WriteFlag::Pretty)));

    // the value is checked before anything is written, and `out` is left empty
    const auto invalid = with<ValidateFields>([](auto& t) { t.x = -1; });
    EXPECT_NO_THROW((void)yyjson::write(invalid));
    const auto e = validate_error([&] { yyjson::write(invalid, out, yyjson::strict); });
    EXPECT_EQ(e.path(), "/x");
    EXPECT_TRUE(out.empty());
    out = "old";
    EXPECT_THROW(yyjson::write(invalid, out, yyjson::WriteFlag::Pretty, yyjson::strict), yyjson::bad_cast);
    EXPECT_TRUE(out.empty());
    // the adjacent tag that `write` rejects with `std::invalid_argument` is a `yyjson::bad_cast` here
    EXPECT_THROW((void)yyjson::write(ValidateEnvelope{"rect", ValidateCircle{}}, yyjson::strict), yyjson::bad_cast);
}

TEST(Validate, Alternatives)
{
    static_assert(!yyjson::detail::needs_validation<yyjson::any_of<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<yyjson::one_of<int, std::string>>);
    static_assert(yyjson::detail::needs_validation<ValidateSignedOrUnsigned>);
    static_assert(yyjson::detail::needs_validation<ValidateLowOrShort>);

    expect_valid(ValidateAlternatives{});
    expect_valid(with<ValidateAlternatives>([](auto& t) {
        t.a = std::string("abc");
        t.b = {ValidateSignedOrUnsigned(std::int64_t{-5}), ValidateSignedOrUnsigned(std::uint64_t{1} << 63)};
        t.c = std::vector<int>{0, 1};
    }));

    // the settings of the alternative that the value holds, named in the message
    const auto low = type_of<ValidateLowOrShort>();
    expect_invalid_alternative(with<ValidateAlternatives>([](auto& t) { t.a = -1; }),
                               "the value of the alternative 0 of " + low + " is less than the minimum 0", "/a",
                               "/a");
    expect_invalid_alternative(with<ValidateAlternatives>([](auto& t) { t.a = std::string("abcd"); }),
                               "the value of the alternative 1 of " + low + " is longer than the maximum length 3",
                               "/a", "/a");
    // an element inside the alternative is located, while the read reports the alternatives at the value
    expect_invalid_alternative(with<ValidateAlternatives>([](auto& t) { t.c = std::vector<int>{1, -2}; }),
                               "an element of the value of the alternative 0 of " + type_of<ValidateItemsOrText>() +
                                   " is less than the minimum 0",
                               "/c/1", "/c");

    // a value of one_of that another alternative reads too
    expect_invalid(with<ValidateAlternatives>([](auto& t) {
                       t.b = {ValidateSignedOrUnsigned(std::int64_t{-5}), ValidateSignedOrUnsigned(std::uint64_t{5})};
                   }),
                   "the value matches the alternatives 0 and 1 of " + type_of<ValidateSignedOrUnsigned>(), "/b/1");
    expect_invalid(ValidateSignedOrUnsigned(std::int64_t{5}),
                   "the value matches the alternatives 0 and 1 of " + type_of<ValidateSignedOrUnsigned>(), "");
    expect_valid(ValidatePartlyOverlapping(-300.0));
    expect_valid(ValidatePartlyOverlapping(std::uint64_t{20}));
    const auto partly = type_of<ValidatePartlyOverlapping>();
    expect_invalid(ValidatePartlyOverlapping(std::int64_t{0}),
                   "the value matches the alternatives 0 and 2 of " + partly, "");
    expect_invalid(ValidatePartlyOverlapping(std::uint64_t{5}),
                   "the value matches the alternatives 2 and 3 of " + partly, "");

    // the plain write does not check
    EXPECT_EQ(std::string(yyjson::write(with<ValidateAlternatives>([](auto& t) { t.a = -1; }))),
              R"({"a":-1,"b":[],"c":""})");
}

TEST(Validate, Unchecked)
{
    // duplicate keys of a range that is written as an object are read back by the strict read
    expect_valid(ValidateDuplicates{{{"a", 1}, {"a", 2}}, {{"a", 1}, {"a", 1}}});
    expect_valid(42);
    expect_valid(std::string("text"));
    expect_valid(std::vector<std::map<std::string, int>>{{{"a", 1}}});
}

// NOLINTEND
