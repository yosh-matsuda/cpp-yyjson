#include "test_constraints_common.hpp"

// NOLINTBEGIN

namespace
{
    // whether the validated write accepts the value exactly when read accepts its plain write
    template <typename T>
    void expect_read_agrees()
    {
        const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
        const auto text = std::string(std::string_view(yyjson::write(T{}, as_null)));
        bool read_accepts = true;
        try
        {
            (void)yyjson::read<T>(text);
        }
        catch (const std::exception&)
        {
            read_accepts = false;
        }
        bool validate_accepts = true;
        try
        {
            (void)yyjson::write(T{}, as_null, yyjson::with_validation);
        }
        catch (const std::exception&)
        {
            validate_accepts = false;
        }
        EXPECT_EQ(validate_accepts, read_accepts) << text;
    }
}  // namespace

TEST(Constraints, InfAndNanAsNullReadsTheSourceOfTheCaster)
{
    expect_read_agrees<r16::Transparent>();
    expect_read_agrees<r16::Translated>();
    expect_read_agrees<r16::ToRow>();
    expect_read_agrees<r16::ToValues>();
    expect_read_agrees<r16::ToArray>();
    expect_read_agrees<r16::ToMap>();
    expect_read_agrees<r16::ToRowIn>();
    EXPECT_THROW((void)yyjson::write(r16::ToArray{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(r16::ToMap{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::write(r16::ToRowIn{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation),
                 yyjson::bad_cast);
}

TEST(Constraints, InfAndNanAsNullCallsOnlyTheNestedCasters)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    r16::outer = 0;
    EXPECT_NO_THROW((void)yyjson::write(r16::ToCounted{}, as_null, yyjson::with_validation));
    EXPECT_EQ(r16::outer, 0);
    expect_read_agrees<r16::ToExtra>();
    r16::outer = 0;
    EXPECT_NO_THROW((void)yyjson::write(r16::ToExtra{}, as_null, yyjson::with_validation));
    EXPECT_EQ(r16::outer, 0);
}

// a merge does not call the predicate of a field of `yyjson::with_caster` in a branch that it cannot select
struct R16Unread
{
    static int from_json(int x) { return x; }
};
inline int r16_invoked_calls = 0;
inline bool r16_invoked(int)
{
    ++r16_invoked_calls;
    return true;
}
struct R16WithCaster
{
    int x = 0;
    yyjson::with_caster<int, R16Unread> y{1};
};
inline bool r16_with_caster_positive(const R16WithCaster& f) { return f.x > 0; }
struct R16WithCasterGate
{
    R16WithCaster f;
};
template <>
inline constexpr auto yyjson::annotations<R16WithCasterGate> = yyjson::annotate(
    yyjson::field("f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r16_with_caster_positive>),
                                            yyjson::then(yyjson::min_properties(1)),
                                            yyjson::else_(yyjson::property("y", yyjson::satisfies<r16_invoked>)))));

TEST(Constraints, MergeDefersThePredicateOfAWithCaster)
{
    r16_invoked_calls = 0;
    EXPECT_NO_THROW((void)yyjson::read<R16WithCasterGate>(R"({"f":{"x":1,"y":1}})"));
    EXPECT_EQ(r16_invoked_calls, 0);
    auto t = R16WithCasterGate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r16_invoked_calls, 0);
}

// a nested read in a speculative branch checks the keywords of the type that it reads with an opaque write
inline bool r16_positive(int x) { return x > 0; }
struct R16Entry
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<R16Entry> =
    yyjson::annotate(yyjson::field("x", yyjson::satisfies<r16_positive>));
struct R16ReadOnly
{
    std::map<std::string, R16Entry> rest;
};
template <>
inline constexpr auto yyjson::annotations<R16ReadOnly> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
template <>
struct yyjson::caster<R16ReadOnly>
{
    static int to_json(const R16ReadOnly&) { return 0; }
};
inline int r16_nested_accepted = 0;
bool r16_nested_reads(std::string_view)
{
    try
    {
        (void)yyjson::read<R16ReadOnly>(R"({"k":{"x":-1}})");
        ++r16_nested_accepted;
    }
    catch (const yyjson::bad_cast&)
    {
    }
    return true;
}
struct R16ReadGate
{
    R14Text2 f;
};
template <>
inline constexpr auto yyjson::annotations<R16ReadGate> = yyjson::annotate(yyjson::field(
    "f", yyjson::if_then_else(yyjson::if_(yyjson::satisfies<r14_positive>), yyjson::then(yyjson::min_properties(1)),
                              yyjson::else_(yyjson::property("s", yyjson::format_assertion("nested-reads"))))));

TEST(Constraints, NestedReadInASpeculativeBranchChecksAll)
{
    EXPECT_TRUE(yyjson::detail::speculation_sensitive<R16ReadOnly>::value);
    r16_nested_accepted = 0;
    (void)r16_nested_reads("");
    EXPECT_EQ(r16_nested_accepted, 0);
    auto t = R16ReadGate{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"f":{"x":1}})", t));
    EXPECT_EQ(r16_nested_accepted, 0);
}

TEST(Constraints, RepeatedKeysNeedNoSpeculationState)
{
    EXPECT_TRUE((std::is_empty_v<yyjson::detail::independent_checks_for<std::multimap<std::string, int>>>));
}

// a present value of an in-place caster field with `yyjson::default_from` starts from the field of `T()`, as read
struct R17InPlace
{
    int x = 10;
    bool operator==(const R17InPlace&) const = default;
};
template <>
struct yyjson::caster<R17InPlace>
{
    static R17InPlace from_json(int x) { return {x + 100}; }
    static void from_json(R17InPlace& c, int x) { c.x += x; }
    static int to_json(const R17InPlace& c) { return c.x; }
};
inline R17InPlace r17_twenty() { return {20}; }
inline bool r17_is21(const R17InPlace& c) { return c.x == 21; }
struct R17InPlaceField
{
    R17InPlace c;
};
template <>
inline constexpr auto yyjson::annotations<R17InPlaceField> =
    yyjson::annotate(yyjson::field("c", yyjson::default_from<r17_twenty>, yyjson::satisfies<r17_is21>));
struct R17InPlaceProperty
{
    R17InPlace c;
};
template <>
inline constexpr auto yyjson::annotations<R17InPlaceProperty> = yyjson::annotate(
    yyjson::field("c", yyjson::default_from<r17_twenty>), yyjson::property("c", yyjson::satisfies<r17_is21>));

template <typename T>
void expect_in_place_from_object()
{
    EXPECT_ANY_THROW((void)yyjson::read<T>(R"({"c":1})"));
    auto t = T{};
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":1})", t));
    EXPECT_EQ(t.c.x, 10);
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":1})", t, yyjson::strict));
    EXPECT_EQ(t.c.x, 10);
    EXPECT_EQ(yyjson::read<T>(R"({"c":11})").c.x, 21);
    t.c.x = 5;
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":11})", t));
    EXPECT_EQ(t.c.x, 21);
    t.c.x = 5;
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":11})", t, yyjson::strict));
    EXPECT_EQ(t.c.x, 21);
}

TEST(Constraints, MergeReadsAPresentInPlaceFieldFromTheObject)
{
    expect_in_place_from_object<R17InPlaceField>();
    expect_in_place_from_object<R17InPlaceProperty>();
}

// a transparent struct with its own caster is read by that caster, also at a field
struct R17Transparent
{
    int x = 10;
};
template <>
inline constexpr auto yyjson::annotations<R17Transparent> = yyjson::annotate(yyjson::transparent);
template <>
struct yyjson::caster<R17Transparent>
{
    static R17Transparent from_json(int x) { return {x + 100}; }
    static void from_json(R17Transparent& c, int x) { c.x += x; }
    static int to_json(const R17Transparent& c) { return c.x; }
};
inline bool r17_eleven(const R17Transparent& c) { return c.x == 11; }
struct R17TransparentField
{
    R17Transparent c;
};
template <>
inline constexpr auto yyjson::annotations<R17TransparentField> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<r17_eleven>));

TEST(Constraints, MergeReadsATransparentCasterAsRead)
{
    EXPECT_EQ(yyjson::read<R17TransparentField>(R"({"c":1})").c.x, 11);
    auto t = R17TransparentField{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t));
    EXPECT_EQ(t.c.x, 11);
}

// a new value of a map is an element, which read gives with the returning `from_json`
struct R17Element
{
    std::string value = "old";
};
template <>
struct yyjson::caster<R17Element>
{
    static R17Element from_json(int x) { return {std::to_string(x)}; }
    static void from_json(R17Element& c, std::string x) { c.value = std::move(x); }
    static std::string to_json(const R17Element& c) { return c.value; }
};
struct R17ExtraElements
{
    int n = 0;
    std::map<std::string, R17Element> m;
};
template <>
inline constexpr auto yyjson::annotations<R17ExtraElements> =
    yyjson::annotate(yyjson::field("m", yyjson::extra_keys, yyjson::additional_properties(yyjson::max_length(3))));
struct R17PatternElements
{
    int n = 0;
    std::map<std::string, R17Element> m;
};
template <>
inline constexpr auto yyjson::annotations<R17PatternElements> =
    yyjson::annotate(yyjson::field("m", yyjson::pattern_properties("^a")));
struct R17MapElements
{
    std::map<std::string, R17Element> m;
};

TEST(Constraints, MergeInsertsAMapValueAsAnElement)
{
    auto extra = R17ExtraElements{};
    EXPECT_EQ(yyjson::read<R17ExtraElements>(R"({"n":1,"a":5})").m.at("a").value, "5");
    EXPECT_NO_THROW(yyjson::merge_into(R"({"n":1,"a":5})", extra));
    EXPECT_EQ(extra.n, 1);
    EXPECT_EQ(extra.m.at("a").value, "5");
    auto pattern = R17PatternElements{};
    EXPECT_NO_THROW(yyjson::merge_into(R"({"n":1,"a":5})", pattern));
    EXPECT_EQ(pattern.m.at("a").value, "5");
    auto map = R17MapElements{};
    EXPECT_EQ(yyjson::read<R17MapElements>(R"({"m":{"a":5}})", yyjson::strict).m.at("a").value, "5");
    EXPECT_NO_THROW(yyjson::merge_into(R"({"m":{"a":5}})", map, yyjson::strict));
    EXPECT_EQ(map.m.at("a").value, "5");
}

// the null that write gives for a caster is read as the type of its `from_json` with the casters in that type
inline int r17_inner_calls = 0;
inline int r17_outer_calls = 0;
struct R17Inner
{
};
template <>
struct yyjson::caster<R17Inner>
{
    static R17Inner from_json(std::optional<double> x)
    {
        ++r17_inner_calls;
        if (!x) throw yyjson::bad_cast("R17Inner rejects null");
        return {};
    }
    static double to_json(const R17Inner&) { return 0; }
};
struct R17Outer
{
    double x = std::numeric_limits<double>::infinity();
};
template <>
struct yyjson::caster<R17Outer>
{
    static R17Outer from_json(R17Inner)
    {
        ++r17_outer_calls;
        return {0};
    }
    static double to_json(const R17Outer& c) { return c.x; }
};

TEST(Constraints, ValidatedNullCallsTheNestedCasters)
{
    EXPECT_ANY_THROW((void)yyjson::read<R17Outer>("null"));
    r17_inner_calls = r17_outer_calls = 0;
    EXPECT_ANY_THROW((void)yyjson::write(R17Outer{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
    EXPECT_GT(r17_inner_calls, 0);
    EXPECT_EQ(r17_outer_calls, 0);
}

// a field that write leaves out is not written for the check of an infinity or a NaN
inline int r17_omitted_calls = 0;
struct R17OmittedWritten
{
    double x = 0;
};
struct R17Omitted
{
    int x = 0;
    bool operator==(const R17Omitted&) const = default;
};
template <>
struct yyjson::caster<R17Omitted>
{
    static R17Omitted from_json(std::map<std::string, double>) { return {}; }
    static R17OmittedWritten to_json(const R17Omitted&)
    {
        ++r17_omitted_calls;
        throw std::runtime_error("omitted caster");
    }
};
struct R17OmittedField
{
    std::optional<R17Omitted> c = R17Omitted{};
};
template <>
inline constexpr auto yyjson::annotations<R17OmittedField> = yyjson::annotate(yyjson::field("c", yyjson::default_));

TEST(Constraints, ValidationWritesNoOmittedField)
{
    r17_omitted_calls = 0;
    EXPECT_NO_THROW(
        (void)yyjson::write(R17OmittedField{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
    EXPECT_EQ(r17_omitted_calls, 0);
}

// a `from_json` that borrows its value from the document is checked with null too
struct R17Borrowed
{
    double x = std::numeric_limits<double>::infinity();
};
template <>
struct yyjson::caster<R17Borrowed>
{
    static R17Borrowed from_json(std::string_view) { return {0}; }
    static double to_json(const R17Borrowed& c) { return c.x; }
};

TEST(Constraints, ValidatedNullReadsABorrowedType)
{
    const auto json = yyjson::write(R17Borrowed{}, yyjson::WriteFlag::InfAndNanAsNull);
    EXPECT_EQ(json, "null");
    EXPECT_ANY_THROW((void)yyjson::read<R17Borrowed>(json));
    EXPECT_ANY_THROW((void)yyjson::write(R17Borrowed{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation));
}

// the layout flags do not hide the null of an infinity in a `std::optional` field
struct R17OptionalInf
{
    std::optional<double> x = std::numeric_limits<double>::infinity();
};

TEST(Constraints, NullAsMissingCheckIgnoresTheLayout)
{
    for (const auto flag : {
#if YYJSON_VERSION_HEX >= 0x000900
             yyjson::WriteFlag::NewlineAtEnd,
#endif
             yyjson::WriteFlag::Pretty})
    {
        const auto f = yyjson::WriteFlag::InfAndNanAsNull | flag;
        const auto json = yyjson::write(R17OptionalInf{}, f);
        EXPECT_ANY_THROW((void)yyjson::read<R17OptionalInf>(json, yyjson::reject_null_as_missing));
        EXPECT_ANY_THROW((void)yyjson::write(R17OptionalInf{}, f, yyjson::with_validation));
    }
}

// an untagged variant with an alternative that borrows from the JSON is read back as read reads it
struct R18Walked
{
    int x = -1;
};
template <>
inline constexpr auto yyjson::annotations<R18Walked> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
using R18Fallback = yyjson::any_of_variant<R18Walked, std::map<std::string, int>, std::string_view>;
using R18Violating = yyjson::any_of_variant<yyjson::alt<std::string, yyjson::max_length(1)>,
                                            yyjson::alt<std::string_view, yyjson::max_length(1)>>;
using R18Ambiguous = yyjson::one_of_variant<yyjson::alt<std::string, yyjson::min_length(2)>,
                                            yyjson::alt<std::string_view, yyjson::max_length(3)>>;
template <typename V>
struct R18Borrowing
{
};
template <typename V>
struct yyjson::caster<R18Borrowing<V>>
{
    static R18Borrowing<V> from_json(V) { return {}; }
    static V to_json(const R18Borrowing<V>&)
    {
        if constexpr (std::same_as<V, R18Fallback>)
            return V(std::in_place_index<0>, R18Walked{});
        else
            return V(std::in_place_index<0>, "ab");
    }
};

template <typename T>
bool r18_reads_written()
{
    try
    {
        (void)yyjson::read<T>(yyjson::write(T{}));
        return true;
    }
    catch (const yyjson::bad_cast&)
    {
        return false;
    }
}

TEST(Constraints, ValidationReadsABorrowedAlternativeBack)
{
    EXPECT_TRUE(r18_reads_written<R18Borrowing<R18Fallback>>());
    EXPECT_NO_THROW((void)yyjson::write(R18Borrowing<R18Fallback>{}, yyjson::with_validation));
    EXPECT_FALSE(r18_reads_written<R18Borrowing<R18Violating>>());
    EXPECT_ANY_THROW((void)yyjson::write(R18Borrowing<R18Violating>{}, yyjson::with_validation));
    EXPECT_FALSE(r18_reads_written<R18Borrowing<R18Ambiguous>>());
    EXPECT_ANY_THROW((void)yyjson::write(R18Borrowing<R18Ambiguous>{}, yyjson::with_validation));
}

// a `yyjson::nullable` or a `yyjson::indirect` field reads its value in place as read does, from the field of `T()`
inline bool r18_eleven(const R17Transparent& c) { return c.x == 11; }
template <template <typename> typename W>
struct R18WrappedField
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedField<W>> =
    yyjson::annotate(yyjson::field("c", yyjson::satisfies<r18_eleven>));
template <template <typename> typename W>
struct R18WrappedProperty
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedProperty<W>> =
    yyjson::annotate(yyjson::property("c", yyjson::satisfies<r18_eleven>));
template <template <typename> typename W>
struct R18WrappedMinimum
{
    W<R17Transparent> c{R17Transparent{}};
};
template <template <typename> typename W>
inline constexpr auto yyjson::annotations<R18WrappedMinimum<W>> =
    yyjson::annotate(yyjson::field("c", yyjson::minimum(0)));

template <typename T>
void expect_wrapped_in_place()
{
    EXPECT_EQ(yyjson::read<T>(R"({"c":1})").c->x, 11);
    auto read = T{};
    read.c = R17Transparent{99};
    yyjson::read_into(R"({"c":1})", read);
    EXPECT_EQ(read.c->x, 11);
    for (const auto strict : {false, true})
    {
        auto t = T{};
        t.c = R17Transparent{99};
        if (strict)
            EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t, yyjson::strict));
        else
            EXPECT_NO_THROW(yyjson::merge_into(R"({"c":1})", t));
        EXPECT_EQ(t.c->x, 11);
    }
}

TEST(Constraints, MergeReadsAWrappedInPlaceCasterAsRead)
{
    expect_wrapped_in_place<R18WrappedField<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedField<yyjson::indirect>>();
    expect_wrapped_in_place<R18WrappedProperty<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedProperty<yyjson::indirect>>();
    expect_wrapped_in_place<R18WrappedMinimum<yyjson::nullable>>();
    expect_wrapped_in_place<R18WrappedMinimum<yyjson::indirect>>();
    auto t = R18WrappedField<yyjson::nullable>{};
    t.c = R17Transparent{99};
    EXPECT_ANY_THROW(yyjson::merge_into(R"({"c":2})", t));
    EXPECT_EQ(t.c->x, 99);
}

// a default that borrows from the JSON is a sentinel if the strict read of its written text fails
struct R18BorrowedDefault
{
    std::string_view s = "ok";
    bool operator==(const R18BorrowedDefault&) const = default;
};
template <>
struct yyjson::caster<R18BorrowedDefault>
{
    static int to_json(const R18BorrowedDefault&) { return 0; }
};
struct R18BorrowedDefaultField
{
    R18BorrowedDefault r;
};
template <>
inline constexpr auto yyjson::annotations<R18BorrowedDefaultField> =
    yyjson::annotate(yyjson::field("r", yyjson::default_));
struct R18BorrowedDefaultHolder
{
};
template <>
struct yyjson::caster<R18BorrowedDefaultHolder>
{
    static R18BorrowedDefaultHolder from_json(R18BorrowedDefaultField) { return {}; }
    static R18BorrowedDefaultField to_json(const R18BorrowedDefaultHolder&) { return {}; }
};

TEST(Constraints, BorrowedDefaultIsASentinel)
{
    const auto json = yyjson::write(R18BorrowedDefaultHolder{}, yyjson::emit_defaults);
    EXPECT_EQ(json, "{}");
    EXPECT_NO_THROW((void)yyjson::read<R18BorrowedDefaultHolder>(json));
    EXPECT_NO_THROW((void)yyjson::write(R18BorrowedDefaultHolder{}, yyjson::emit_defaults, yyjson::with_validation));
}

// the values of a range of pairs read from the array form are checked as in the object form
struct PairSetValues
{
    std::vector<std::pair<std::string, std::set<int>>> m = {};
};
template <>
inline constexpr auto yyjson::annotations<PairSetValues> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::min_items(2))));
struct PairVectorValues
{
    std::vector<std::pair<std::string, std::vector<int>>> m = {};
};
template <>
inline constexpr auto yyjson::annotations<PairVectorValues> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::min_items(2))));

template <typename T>
void expect_pair_values()
{
    for (const auto* json : {R"({"m":{"k":[1]}})", R"({"m":[["k",[1]]]})"})
    {
        EXPECT_THROW((void)yyjson::read<T>(json), yyjson::bad_cast) << json;
        EXPECT_THROW((void)yyjson::read(json).template cast<T>(), yyjson::bad_cast) << json;
    }
    for (const auto* json : {R"({"m":{"k":[1,2]}})", R"({"m":[["k",[1,2]]]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<T>(json)) << json;
        EXPECT_NO_THROW((void)yyjson::read(json).template cast<T>()) << json;
    }
    EXPECT_THROW(yyjson::validate(T{{{"k", {1}}}}), yyjson::bad_cast);
    EXPECT_NO_THROW(yyjson::validate(T{{{"k", {1, 2}}}}));
}

TEST(Constraints, PairValuesFromTheArrayForm)
{
    expect_pair_values<PairSetValues>();
    expect_pair_values<PairVectorValues>();
    try
    {
        (void)yyjson::read<PairSetValues>(R"({"m":[["k",[1]]]})");
        FAIL();
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(e.path(), "/m");
    }
}

// NOLINTEND
