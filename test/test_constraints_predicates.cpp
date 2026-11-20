#include "test_constraints_common.hpp"

// NOLINTBEGIN

// a schema that also takes a C++ value checks the merged JSON, which the merge does not write again
struct R11Mixed
{
    R11Point f;
};
template <>
inline constexpr auto yyjson::annotations<R11Mixed> =
    yyjson::annotate(yyjson::field("f", yyjson::min_properties(2), yyjson::satisfies<r11_any>));

TEST(Constraints, MergeChecksAMixedSchemaOnTheMergedJson)
{
    expect_rejected<R11Mixed>(
        R"({"f":{"y":0}})",
        R"(the object has fewer properties than the minimum 2 at /f; reading the key "f" of R11Mixed)", "/f", false);
    auto t = R11Mixed{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":null}})", t); }).path(), "/f");
}

// a value that only writes leaves the checks of the other values in place
struct R11WriteOnly
{
    int x = 0;
};
template <>
struct yyjson::caster<R11WriteOnly>
{
    static std::map<std::string, int> to_json(const R11WriteOnly& c) { return {{"x", c.x}}; }
};
struct R11WithWriteOnly
{
    R11WriteOnly c;
    int n = 1;
};
template <>
inline constexpr auto yyjson::annotations<R11WithWriteOnly> = yyjson::annotate(yyjson::field("n", yyjson::maximum(0)));

TEST(Constraints, MergeChecksKeptValuesBesideAWriteOnlyValue)
{
    EXPECT_THROW(yyjson::validate(R11WithWriteOnly{}), yyjson::bad_cast);
    auto t = R11WithWriteOnly{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into("{}", t); }).path(), "/n");
}

// `validate` checks a value of a map with its C++ value, and does not read the JSON that it writes
struct R11Prefixed
{
    std::string s;
};
inline int r11_prefixed_reads = 0;
template <>
struct yyjson::caster<R11Prefixed>
{
    static R11Prefixed from_json(std::string s)
    {
        ++r11_prefixed_reads;
        if (s.starts_with("!")) throw std::runtime_error("unreadable prefix");
        return {std::move(s)};
    }
    static std::string to_json(const R11Prefixed& c) { return "!" + c.s; }
};
struct R11PrefixedMap
{
    std::map<std::string, R11Prefixed> rest;
};
template <>
inline constexpr auto yyjson::annotations<R11PrefixedMap> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, ValidateBindsAMapValueToItsEntry)
{
    r11_prefixed_reads = 0;
    EXPECT_NO_THROW(yyjson::validate(R11PrefixedMap{{{"a", {"ab"}}}}));
    EXPECT_EQ(bad_cast_of([] { yyjson::validate(R11PrefixedMap{{{"a", {"abcd"}}}}); }).path(), "/a");
    EXPECT_EQ(r11_prefixed_reads, 0);
    // a merge reads only the value of the patch, and binds the kept value "!ab" to its entry
    auto t = R11PrefixedMap{{{"a", {"ab"}}}};
    yyjson::merge_into(R"({"b":"cd"})", t);
    EXPECT_EQ(t.rest.size(), 2U);
}

// a member that the patch replaces keeps no binding to the value before the merge
struct R11Negative
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Negative> = yyjson::annotate(yyjson::field("x", yyjson::maximum(-1)));
struct R11Replaced
{
    R11Negative f;
};

TEST(Constraints, MergeReplacesAMemberForTheLaterDuplicates)
{
    auto t = R11Replaced{};
    yyjson::merge_into(R"({"f":null,"f":{"x":-1},"f":{"y":0}})", t);
    EXPECT_EQ(t.f.x, -1);
}

// a merge into an empty optional checks the patch before it sets the optional
struct R11Root
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<R11Root> = yyjson::annotate(yyjson::field("y", yyjson::minimum(0)));

TEST(Constraints, MergeChecksAnEmptyRootBeforeTheCommit)
{
    auto t = std::optional<R11Root>{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"x":1,"y":-1})", t); }).path(), "/y");
    EXPECT_FALSE(t.has_value());
}

// a predicate of the values of a map gives the key of the value that it rejects
struct R11Pair
{
    int a = 0;
    int b = 0;
};
inline bool r11_pair_small(const R11Pair& c) { return c.a + c.b <= 1; }
struct R11Pairs
{
    std::map<std::string, R11Pair> m;
};
template <>
inline constexpr auto yyjson::annotations<R11Pairs> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::satisfies<r11_pair_small>)));

TEST(Constraints, MergeGivesTheKeyOfARejectedMapValue)
{
    auto t = R11Pairs{{{"k", {1, 0}}}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"m":{"k":{"b":1}}})", t); }).path(), "/m/k");
}

// a check before a merge that does not depend on the merged C++ value rejects the patch before the commit
struct R12Pair
{
    int x = 0;
    int n = 0;
};
inline bool r12_yes(const R12Pair&) { return true; }
inline bool r12_small(const R12Pair& f) { return f.x <= 1; }
inline bool r12_zero(const int& n) { return n == 0; }
struct R12Counted
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12Counted> =
    yyjson::annotate(yyjson::field("f", yyjson::min_properties(2), yyjson::satisfies<r12_yes>));
struct R12Condition
{
    R12Pair f;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<R12Condition> =
    yyjson::annotate(yyjson::all_of(yyjson::property("n", yyjson::satisfies<r12_zero>)));

TEST(Constraints, MergeRejectsBeforeTheCommitWhatNoMergedValueDecides)
{
    auto t = R12Counted{{9, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":null}})", t); }).path(), "/f");
    EXPECT_EQ(t.f.x, 9);

    auto c = R12Condition{{0, 0}, 0};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":7},"n":1})", c); }).path(), "/n");
    EXPECT_EQ(c.f.x, 0);
    EXPECT_EQ(c.n, 0);
}

// a combinator of a keyword on JSON and a predicate of a merged value is left to the commit only if the
// predicate can change its result
struct R12AnyOf
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12AnyOf> =
    yyjson::annotate(yyjson::field("f", yyjson::any_of(yyjson::min_properties(3), yyjson::satisfies<r12_small>)));
struct R12AnyOfAll
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12AnyOfAll> = yyjson::annotate(
    yyjson::field("f", yyjson::any_of(yyjson::max_properties(0),
                                      yyjson::all_of(yyjson::min_properties(3), yyjson::satisfies<r12_small>))));
struct R12Not
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12Not> = yyjson::annotate(
    yyjson::field("f", yyjson::not_(yyjson::all_of(yyjson::min_properties(1), yyjson::satisfies<r12_small>))));
struct R12NotJson
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12NotJson> = yyjson::annotate(
    yyjson::field("f", yyjson::not_(yyjson::any_of(yyjson::min_properties(1), yyjson::satisfies<r12_small>))));
struct R12OneOf
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12OneOf> =
    yyjson::annotate(yyjson::field("f", yyjson::one_of(yyjson::min_properties(2), yyjson::satisfies<r12_small>)));
struct R12OneOfJson
{
    R12Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R12OneOfJson> = yyjson::annotate(yyjson::field(
    "f", yyjson::one_of(yyjson::min_properties(1), yyjson::max_properties(5), yyjson::satisfies<r12_small>)));

TEST(Constraints, MergeDecidesACombinatorBeforeTheCommitIfItCan)
{
    // the predicate decides: the merge commits, and then rejects the merged value
    auto any = R12AnyOf{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", any); }).path(), "/f");
    EXPECT_EQ(any.f.x, 5);
    yyjson::merge_into(R"({"f":{"x":1}})", any);
    EXPECT_EQ(any.f.x, 1);

    auto one = R12OneOf{{5, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":0}})", one); }).path(), "/f");
    EXPECT_EQ(one.f.x, 0);

    auto negated = R12Not{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", negated); }).path(), "/f");
    EXPECT_EQ(negated.f.x, 1);

    // the keywords on JSON decide whatever the predicate gives: the merge rejects the patch before the commit
    auto all = R12AnyOfAll{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":1}})", all); }).path(), "/f");
    EXPECT_EQ(all.f.x, 0);

    auto json = R12NotJson{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", json); }).path(), "/f");
    EXPECT_EQ(json.f.x, 0);

    auto many = R12OneOfJson{{0, 0}};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":5}})", many); }).path(), "/f");
    EXPECT_EQ(many.f.x, 0);
}

// a pattern that matches a declared field of its value type checks the field without a read of its JSON
struct R12Prefixed
{
    std::string s;
};
inline int r12_prefixed_reads = 0;
template <>
struct yyjson::caster<R12Prefixed>
{
    static R12Prefixed from_json(std::string s)
    {
        ++r12_prefixed_reads;
        if (s.starts_with("!")) throw std::runtime_error("unreadable prefix");
        return {s};
    }
    static std::string to_json(const R12Prefixed& c) { return "!" + c.s; }
};
struct R12Declared
{
    R12Prefixed a;
    std::map<std::string, R12Prefixed> p;
};
template <>
inline constexpr auto yyjson::annotations<R12Declared> = yyjson::annotate(
    yyjson::field("p", yyjson::pattern_properties("^a$"), yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, PatternOfADeclaredFieldTakesItsValue)
{
    r12_prefixed_reads = 0;
    auto t = R12Declared{{"ab"}, {}};
    EXPECT_NO_THROW(yyjson::validate(t));
    yyjson::merge_into(R"({"p":{}})", t);
    EXPECT_EQ(r12_prefixed_reads, 0);
    t.a.s = "abcd";
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/a");
    EXPECT_EQ(r12_prefixed_reads, 0);
}

// a finite value that a caster writes as a float is not read back with WriteFlag::InfAndNanAsNull
struct R12Finite
{
    double x = 5;
};
inline int r12_finite_reads = 0;
template <>
struct yyjson::caster<R12Finite>
{
    static R12Finite from_json(double)
    {
        ++r12_finite_reads;
        throw std::runtime_error("finite caster reread");
    }
    static double to_json(const R12Finite& d) { return d.x; }
};
struct R12Finites
{
    std::map<std::string, R12Finite> m;
};
template <>
inline constexpr auto yyjson::annotations<R12Finites> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));

TEST(Constraints, InfAndNanAsNullReadsNoFiniteCasterValue)
{
    r12_finite_reads = 0;
    const auto t = R12Finites{{{"k", R12Finite{5}}}};
    EXPECT_EQ(std::string_view(yyjson::write(t, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation)),
              R"({"k":5.0})");
    EXPECT_EQ(r12_finite_reads, 0);
}

// a validate that a predicate calls during the check of a merge checks its own value
struct R13Part
{
    int x = 0;
};
inline bool r13_positive(const R13Part& f) { return f.x > 0; }
struct R13Check
{
    std::shared_ptr<R13Part> f;
};
template <>
inline constexpr auto yyjson::annotations<R13Check> =
    yyjson::annotate(yyjson::field("f", yyjson::satisfies<r13_positive>));
struct R13Gated
{
    R13Part f;
    int gate = 0;
};
inline R13Gated* r13_target = nullptr;
inline bool r13_rejects(int)
{
    try
    {
        yyjson::validate(R13Check{std::shared_ptr<R13Part>(&r13_target->f, [](R13Part*) {})});
        return false;
    }
    catch (const yyjson::bad_cast&)
    {
        return true;
    }
}
template <>
inline constexpr auto yyjson::annotations<R13Gated> = yyjson::annotate(
    yyjson::field("f", yyjson::satisfies<r13_positive>), yyjson::field("gate", yyjson::satisfies<r13_rejects>));

TEST(Constraints, MergeKeepsANestedValidateIndependent)
{
    auto t = R13Gated{};
    r13_target = &t;
    EXPECT_NO_THROW((void)yyjson::read<R13Gated>(R"({"f":{"x":1},"gate":0})"));
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(t.f.x, 1);
}

// a merge calls no predicate of a branch that read does not select
inline bool r13_explode(int) { throw std::runtime_error("inactive branch"); }
struct R13Branch
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13Branch> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r13_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("x", yyjson::satisfies<r13_explode>)))));
struct R13AnyOf
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13AnyOf> = yyjson::annotate(yyjson::field(
    "f", yyjson::any_of(yyjson::satisfies<r13_positive>, yyjson::property("x", yyjson::satisfies<r13_explode>))));
struct R13Both
{
    R13Part f;
};
template <>
inline constexpr auto yyjson::annotations<R13Both> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r13_positive>), yyjson::then(yyjson::max_properties(0)),
                              yyjson::else_(yyjson::max_properties(0)))));

TEST(Constraints, MergeCallsNoPredicateOfAnInactiveBranch)
{
    EXPECT_NO_THROW((void)yyjson::read<R13Branch>(R"({"f":{"x":1}})"));
    auto branch = R13Branch{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", branch));
    EXPECT_EQ(branch.f.x, 1);
    auto any = R13AnyOf{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", any));
    EXPECT_EQ(any.f.x, 1);

    // both branches fail: the message does not tell which way the condition went
    auto both = R13Both{{-1}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"f":{"x":-2}})", both); });
    EXPECT_EQ(e.path(), "/f");
    EXPECT_EQ(std::string_view(e.what()).find("if_"), std::string_view::npos);
    EXPECT_EQ(both.f.x, -1);
}

// a pattern binds the C++ value of a declared field through a wrapper or a variant
struct R13Wrap
{
    R12Prefixed value;
};
template <>
inline constexpr auto yyjson::annotations<R13Wrap> = yyjson::annotate(yyjson::transparent);
template <typename A>
struct R13Patterned
{
    A a;
    std::map<std::string, R12Prefixed> p;
};
template <typename A>
inline constexpr auto yyjson::annotations<R13Patterned<A>> = yyjson::annotate(
    yyjson::field("p", yyjson::pattern_properties("^a$"), yyjson::additional_properties(yyjson::max_length(4))));
using R13Adjacent = std::variant<R12Prefixed, std::string>;
struct R13AdjacentField
{
    R13Adjacent v;
    std::map<std::string, R13Adjacent> p;
};
template <>
inline constexpr auto yyjson::annotations<R13AdjacentField> = yyjson::annotate(
    yyjson::field("v", yyjson::adjacent_tag("t", {"c", "s"})),
    yyjson::field("p", yyjson::pattern_properties("^v$"), yyjson::additional_properties(yyjson::max_length(4))));

TEST(Constraints, PatternOfAWrappedFieldTakesItsValue)
{
    r12_prefixed_reads = 0;
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<std::optional<R12Prefixed>>{R12Prefixed{"ab"}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<R13Wrap>{R13Wrap{{"ab"}}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13Patterned<std::variant<R12Prefixed, int>>{R12Prefixed{"ab"}, {}}));
    EXPECT_NO_THROW(yyjson::validate(R13AdjacentField{R12Prefixed{"ab"}, {}}));
    auto t = R13Patterned<std::optional<R12Prefixed>>{R12Prefixed{"ab"}, {}};
    yyjson::merge_into(R"({"p":{}})", t);
    EXPECT_EQ(r12_prefixed_reads, 0);
    t.a = R12Prefixed{"abcd"};
    EXPECT_EQ(bad_cast_of([&] { yyjson::validate(t); }).path(), "/a");
    EXPECT_EQ(r12_prefixed_reads, 0);
}

// a NaN that write gives as null is checked as the JSON of the caster, without its from_json
struct R13Real
{
    double x = 0;
};
inline int r13_real_reads = 0;
template <>
struct yyjson::caster<R13Real>
{
    static R13Real from_json(std::optional<double> x)
    {
        ++r13_real_reads;
        return {x.value_or(0)};
    }
    static std::optional<double> to_json(const R13Real& c) { return c.x; }
};
struct R13Reals
{
    std::map<std::string, R13Real> m;
};
template <>
inline constexpr auto yyjson::annotations<R13Reals> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));
struct R13Positions
{
    R12Finite a;
    std::vector<R13Real> rest;
};
template <>
inline constexpr auto yyjson::annotations<R13Positions> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::max_items(5));

TEST(Constraints, InfAndNanAsNullReadsNoCasterValue)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    r13_real_reads = 0;
    r12_finite_reads = 0;
    EXPECT_EQ(std::string_view(yyjson::write(R13Reals{{{"k", R13Real{nan}}}}, as_null, yyjson::with_validation)),
              R"({"k":null})");
    EXPECT_EQ(std::string_view(yyjson::write(R13Real{nan}, as_null, yyjson::with_validation)), "null");
    EXPECT_EQ(
        std::string_view(yyjson::write(R13Positions{R12Finite{1}, {R13Real{nan}}}, as_null, yyjson::with_validation)),
        "[1.0,null]");
    // a `double` that `from_json` takes cannot read null
    const auto e =
        bad_cast_of([&] { (void)yyjson::write(R13Positions{R12Finite{nan}, {}}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected a number, got null at /0; reading double");
    EXPECT_EQ(r13_real_reads, 0);
    EXPECT_EQ(r12_finite_reads, 0);
}

TEST(Constraints, MergeRejectsBeforeANestedValidate)
{
    EXPECT_THROW((void)yyjson::read<R14Nested>(R"({"f":{"x":1,"s":"s"}})"), yyjson::bad_cast);
    auto t = R14Nested{};
    EXPECT_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t), yyjson::bad_cast);
    EXPECT_EQ(t.f.x, 0);
}

// a merge calls no predicate after a property of the struct position that fails
struct R14Inner
{
    int x = 0;
};
inline bool r14_inner_positive(const R14Inner& a) { return a.x > 0; }
inline int r14_later_calls = 0;
inline bool r14_later(int)
{
    ++r14_later_calls;
    throw yyjson::bad_cast("later callback");
}
struct R14Pair
{
    R14Inner a;
    int b = 0;
};
struct R14Chain
{
    R14Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R14Chain> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::property("a", yyjson::satisfies<r14_inner_positive>),
                                          yyjson::property("b", yyjson::satisfies<r14_later>)),
                              yyjson::then(yyjson::min_properties(1)), yyjson::else_(yyjson::min_properties(1)))));

TEST(Constraints, MergeCallsNoPredicateAfterAFailedProperty)
{
    r14_later_calls = 0;
    EXPECT_NO_THROW((void)yyjson::read<R14Chain>(R"({"f":{"a":{"x":-1},"b":0}})"));
    auto t = R14Chain{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"a":{"x":-1}}})", t));
    EXPECT_EQ(r14_later_calls, 0);
}

// InfAndNanAsNull reads no caster value that a variant or a map holds
struct R14Key
{
    std::string s;
    explicit R14Key(std::string_view x) : s("!" + std::string(x)) {}
    operator std::string_view() const { return s; }  // NOLINT(google-explicit-constructor)
    bool operator<(const R14Key& k) const { return s < k.s; }
};
struct R14Keyed
{
    std::map<R14Key, R13Real> m;
};
template <>
inline constexpr auto yyjson::annotations<R14Keyed> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::maximum(10))));
struct R14Int
{
    static double from_json(int n) { return n; }
};
struct R14Source
{
    double a = 0;
};
struct R14Written
{
    std::optional<double> a;
};
struct R14Translated
{
    double v = 0;
};
template <>
struct yyjson::caster<R14Translated>
{
    static R14Translated from_json(R14Source s) { return {s.a}; }
    static R14Written to_json(const R14Translated& t) { return {t.v}; }
};

struct R14Accepting
{
    double v = 0;
};
template <>
struct yyjson::caster<R14Accepting>
{
    static R14Accepting from_json(R14Written w) { return {w.a.value_or(0)}; }
    static R14Source to_json(const R14Accepting& t) { return {t.v}; }
};

TEST(Constraints, InfAndNanAsNullReadsTheHeldCasterValue)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    r13_real_reads = 0;
    EXPECT_EQ(std::string_view(
                  yyjson::write(std::variant<R13Real, std::string>{R13Real{nan}}, as_null, yyjson::with_validation)),
              "null");
    EXPECT_EQ(std::string_view(yyjson::write(R14Keyed{{{R14Key("k"), R13Real{5}}}}, as_null, yyjson::with_validation)),
              R"({"!k":5.0})");
    EXPECT_EQ(
        std::string_view(yyjson::write(R14Keyed{{{R14Key("k"), R13Real{nan}}}}, as_null, yyjson::with_validation)),
        R"({"!k":null})");
    EXPECT_EQ(r13_real_reads, 0);
    // null is read as the type that `from_json` takes
    const auto e = bad_cast_of(
        [&] { (void)yyjson::write(yyjson::with_caster<double, R14Int>{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected an integer, got null; reading int");
    const auto f = bad_cast_of([&] { (void)yyjson::write(R14Translated{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(f.what()), "expected a number, got null at /a; reading double");
    // a std::optional field of the type that `from_json` takes rejects null, as read rejects it under the policy
    const auto g = bad_cast_of([&] { (void)yyjson::write(R14Accepting{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(g.what()),
              "the value must not be null at /a; reading std::optional<double> under yyjson::reject_null_as_missing");
}

// the check of a merge follows no field that write leaves out
struct R15Node;
struct R15Link
{
    std::shared_ptr<R15Node> p;
    bool operator==(const R15Link&) const { return true; }
};
template <>
struct yyjson::caster<R15Link>
{
    static const R15Node& to_json(const R15Link& l) { return *l.p; }
};
struct R15Node
{
    double x = 0;
    R15Link next;
};
inline R15Link r15_empty_link() { return {}; }
template <>
inline constexpr auto yyjson::annotations<R15Node> =
    yyjson::annotate(yyjson::field("next", yyjson::default_from<r15_empty_link>));
struct R15Cyclic
{
    std::shared_ptr<R15Node> p;
};
template <>
struct yyjson::caster<R15Cyclic>
{
    static R15Cyclic from_json(std::map<std::string, double>) { return {std::make_shared<R15Node>()}; }
    static const R15Node& to_json(const R15Cyclic& c) { return *c.p; }
};

TEST(Constraints, InfAndNanAsNullFollowsTheWrittenFields)
{
    auto p = std::make_shared<R15Node>();
    p->next.p = p;
    EXPECT_EQ(
        std::string_view(yyjson::write(R15Cyclic{p}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation)),
        R"({"x":0.0})");
    p->next.p.reset();
}

// a validate inside a speculative branch of a merge checks types that only its write reaches
inline bool r15_positive(int x) { return x > 0; }
struct R15Array
{
    int x = -1;
};
template <>
inline constexpr auto yyjson::annotations<R15Array> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::contains(yyjson::satisfies<r15_positive>));
struct R15Limited
{
    int v = 5;
};
template <>
inline constexpr auto yyjson::annotations<R15Limited> = yyjson::annotate(yyjson::field("v", yyjson::maximum(0)));
struct R15WriteOnly
{
    int v = 5;
};
template <>
struct yyjson::caster<R15WriteOnly>
{
    static R15Limited to_json(const R15WriteOnly& w) { return {w.v}; }
};
inline int r15_nested_accepted = 0;
bool r15_nested_checks(std::string_view)
{
    const auto accepts = [](const auto& v) {
        try
        {
            yyjson::validate(v);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    };
    r15_nested_accepted += static_cast<int>(accepts(R15Array{})) + static_cast<int>(accepts(R15WriteOnly{}));
    return true;
}
struct R15Gate
{
    R14Text2 f;
};
template <>
inline constexpr auto yyjson::annotations<R15Gate> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r14_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("s", yyjson::format_assertion("nested-checks"))))));

TEST(Constraints, NestedValidateInASpeculativeBranchChecksAll)
{
    r15_nested_accepted = 0;
    (void)r15_nested_checks("");
    EXPECT_EQ(r15_nested_accepted, 0);
    auto t = R15Gate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r15_nested_accepted, 0);
}

// null is read as the type that read gives `from_json` at the position
struct R15Both
{
    double x = 0;
};
template <>
struct yyjson::caster<R15Both>
{
    static R15Both from_json(std::optional<double> x) { return {x.value_or(0)}; }
    static void from_json(R15Both& c, int x) { c.x = x; }
    static std::optional<double> to_json(const R15Both& c) { return c.x; }
};
struct R15BothField
{
    R15Both b;
};
// the native positions beside a caster in the written value are read back
struct R15Opaque
{
    int x = 0;
};
template <>
struct yyjson::caster<R15Opaque>
{
    static R15Opaque from_json(int x) { return {x}; }
    static int to_json(const R15Opaque& d) { return d.x; }
};
struct R15In
{
    double a = 0;
    R15Opaque b;
};
struct R15Out
{
    double a = 0;
    int b = 0;
};
struct R15Beside
{
    double x = 0;
};
template <>
struct yyjson::caster<R15Beside>
{
    static R15Beside from_json(R15In v) { return {v.a}; }
    static R15Out to_json(const R15Beside& v) { return {v.x, 0}; }
};

TEST(Constraints, InfAndNanAsNullReadsAsTheReadPosition)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    EXPECT_NO_THROW((void)yyjson::read<R15Both>("null"));
    EXPECT_EQ(std::string_view(yyjson::write(R15Both{nan}, as_null, yyjson::with_validation)), "null");
    EXPECT_THROW((void)yyjson::read<R15BothField>(R"({"b":null})"), yyjson::bad_cast);
    const auto e = bad_cast_of([&] { (void)yyjson::write(R15BothField{{nan}}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(e.what()), "expected an integer, got null at /b; reading int");
    EXPECT_THROW((void)yyjson::read<R15Beside>(R"({"a":null,"b":0})"), yyjson::bad_cast);
    const auto f = bad_cast_of([&] { (void)yyjson::write(R15Beside{nan}, as_null, yyjson::with_validation); });
    EXPECT_EQ(std::string_view(f.what()), "expected a number, got null at /a; reading double");
}

// a key of a map that read stores in another map is found before a value of the map is read back
struct R15Routed
{
    std::map<std::string, R13Real> first, rest;
};
template <>
inline constexpr auto yyjson::annotations<R15Routed> = yyjson::annotate(
    yyjson::field("first", yyjson::pattern_properties("^x")), yyjson::field("rest", yyjson::extra_keys));

TEST(Constraints, InfAndNanAsNullChecksTheRoutesFirst)
{
    r13_real_reads = 0;
    const auto e = bad_cast_of([] {
        (void)yyjson::write(R15Routed{{}, {{"x", R13Real{5}}}}, yyjson::WriteFlag::InfAndNanAsNull,
                            yyjson::with_validation);
    });
    EXPECT_EQ(
        std::string_view(e.what()),
        R"(the value is not valid at /x; validating R15Routed: a read gives this key of the field "rest" to the field "first")");
    EXPECT_EQ(r13_real_reads, 0);
}

// a merge calls a predicate of a branch that its check cannot select only after the merge
struct R15Pair
{
    int x = 0, y = 0;
};
inline bool r15_pair_positive(const R15Pair& f) { return f.x > 0; }
inline bool r15_never(int) { return false; }
struct R15Late
{
    R15Pair f;
};
template <>
inline constexpr auto yyjson::annotations<R15Late> = yyjson::annotate(
    yyjson::field("f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r15_pair_positive>),
                                            yyjson::then(yyjson::property("y", yyjson::satisfies<r15_never>)),
                                            yyjson::else_(yyjson::max_properties(0)))));

TEST(Constraints, MergeChecksASpeculativePredicateAfterTheMerge)
{
    EXPECT_THROW((void)yyjson::read<R15Late>(R"({"f":{"x":1,"y":0}})"), yyjson::bad_cast);
    auto t = R15Late{};
    EXPECT_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t), yyjson::bad_cast);
    // documented: the predicate runs after the merge, so `t` holds the merged value
    EXPECT_EQ(t.f.x, 1);
}

// a merge checks a field that `from_json` reads in place as read gives it
struct R16InPlace
{
    std::string value;
};
template <>
struct yyjson::caster<R16InPlace>
{
    static R16InPlace from_json(int x) { return {std::to_string(x)}; }
    static void from_json(R16InPlace& c, std::string x) { c.value = std::move(x); }
    static std::string to_json(const R16InPlace& c) { return c.value; }
};
struct R16InPlaceField
{
    int n = 0;
    R16InPlace c{"old"};
};
template <>
inline constexpr auto yyjson::annotations<R16InPlaceField> =
    yyjson::annotate(yyjson::field("c", yyjson::max_length(3)));

TEST(Constraints, MergeChecksAFieldReadInPlace)
{
    const auto* json = R"({"n":1,"c":"new"})";
    EXPECT_NO_THROW((void)yyjson::read<R16InPlaceField>(json));
    auto t = R16InPlaceField{};
    EXPECT_NO_THROW(yyjson::merge_into(json, t));
    EXPECT_EQ(t.n, 1);
    EXPECT_EQ(t.c.value, "new");
}

// NOLINTEND
