#include <cmath>
#include <cstdlib>
#include <memory>
#include <new>
#include "test_common.hpp"
#include "test_pattern_matcher.hpp"
#include "test_validation.hpp"

// NOLINTBEGIN

// `yyjson::validate(t)` and `yyjson::write(t, ..., yyjson::with_validation)`, which check a value against the
// keywords of its type before it is written

// the keywords of the values of the fields
enum class VdColor
{
    red,
    green
};
struct VdFields
{
    int a = 0;
    double d = 0;
    std::string s = "ab";
    unsigned u = 0;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::tuple<int, std::string> t = {0, "x"};
    std::string e = "mm";
    int c = 1;
    std::string day = "2024-01-01";
};
template <>
inline constexpr auto yyjson::annotations<VdFields> = yyjson::annotate(
    yyjson::field("a", yyjson::minimum(0)), yyjson::field("d", yyjson::exclusive_maximum(10)),
    yyjson::field("s", yyjson::min_length(2), yyjson::max_length(4)), yyjson::field("u", yyjson::multiple_of(3)),
    yyjson::field("v", yyjson::items(yyjson::maximum(5)), yyjson::max_items(3), yyjson::unique_items),
    yyjson::field("m", yyjson::additional_properties(yyjson::minimum(0)),
                  yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("t", yyjson::prefix_items(yyjson::minimum(0), yyjson::min_length(1))),
    yyjson::field("e", yyjson::enum_("mm", "cm")), yyjson::field("c", yyjson::const_(1)),
    yyjson::field("day", yyjson::format_assertion("date")));

TEST(Validate, ValueKeywordsOfFields)
{
    expect_valid(VdFields{});
    expect_valid(VdFields{5, 9.5, "abcd", 6, {1, 2, 5}, {{"abc", 0}}, {3, "y"}, "cm", 1, "2000-02-29"});
    expect_invalid(with<VdFields>([](auto& t) { t.a = -1; }),
                   R"(the number is less than the minimum 0 at /a; reading the key "a" of VdFields)", "/a");
    expect_invalid(with<VdFields>([](auto& t) { t.d = 10; }),
                   R"(the number is not less than the exclusive maximum 10 at /d; reading the key "d" of VdFields)",
                   "/d");
    expect_invalid(with<VdFields>([](auto& t) { t.s = "a"; }),
                   R"(the string is shorter than the minimum length 2 at /s; reading the key "s" of VdFields)", "/s");
    expect_invalid(with<VdFields>([](auto& t) { t.u = 4; }),
                   R"(the number is not a multiple of 3 at /u; reading the key "u" of VdFields)", "/u");
    expect_invalid(
        with<VdFields>([](auto& t) { t.v = {1, 9}; }),
        R"(the number is greater than the maximum 5 at /v/1; reading an element of the value at the key "v" of VdFields)",
        "/v/1");
    expect_invalid(with<VdFields>([](auto& t) { t.v = {1, 2, 3, 4}; }),
                   R"(the array has more items than the maximum 3 at /v; reading the key "v" of VdFields)", "/v");
    expect_invalid(
        with<VdFields>([](auto& t) { t.v = {1, 1}; }),
        R"(the item is equal to an earlier item at /v/1; reading an element of the value at the key "v" of VdFields)",
        "/v/1");
    expect_invalid(
        with<VdFields>([](auto& t) { t.m = {{"a", 0}, {"b", -1}}; }),
        R"(the number is less than the minimum 0 at /m/b; reading an element of the value at the key "m" of VdFields)",
        "/m/b");
    expect_invalid(
        with<VdFields>([](auto& t) { t.m = {{"abcd", 0}}; }),
        R"(the key is longer than the maximum length 3 at /m/abcd; reading a key of the value at the key "m" of VdFields)",
        "/m/abcd");
    expect_invalid(
        with<VdFields>([](auto& t) { t.t = {-1, "x"}; }),
        R"(the number is less than the minimum 0 at /t/0; reading an element of the value at the key "t" of VdFields)",
        "/t/0");
    expect_invalid(
        with<VdFields>([](auto& t) { t.t = {0, ""}; }),
        R"(the string is shorter than the minimum length 1 at /t/1; reading an element of the value at the key "t" of VdFields)",
        "/t/1");
    expect_invalid(with<VdFields>([](auto& t) { t.e = "km"; }),
                   R"(the value is not one of "mm", "cm" at /e; reading the key "e" of VdFields)", "/e");
    expect_invalid(with<VdFields>([](auto& t) { t.c = 2; }),
                   R"(the value is not equal to the constant 1 at /c; reading the key "c" of VdFields)", "/c");
    expect_invalid(with<VdFields>([](auto& t) { t.day = "2023-02-29"; }),
                   R"(the string does not have the format "date" at /day; reading the key "day" of VdFields)", "/day");
}

// a float takes the keywords on numbers as the number of the text that write gives for it
struct VdFloat
{
    float f = 0.1F;
    std::vector<float> v = {0.1F};
};
template <>
inline constexpr auto yyjson::annotations<VdFloat> =
    yyjson::annotate(yyjson::field("f", yyjson::maximum(0.1)), yyjson::field("v", yyjson::items(yyjson::maximum(0.1))));
struct VdFloatBelow
{
    float f = 0.1F;
};
template <>
inline constexpr auto yyjson::annotations<VdFloatBelow> =
    yyjson::annotate(yyjson::field("f", yyjson::maximum(0.099999999)));
struct VdFloatRow
{
    int id = 0;
    std::vector<float> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdFloatRow> = yyjson::annotate(
    yyjson::as_array, yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::minimum(0.12))));

// a double takes the keywords on numbers as its value, which write gives with the shortest digits
struct VdDecimal
{
    double tenth = 0.3;
    std::vector<double> v = {9007199254740992.0};
};
template <>
inline constexpr auto yyjson::annotations<VdDecimal> =
    yyjson::annotate(yyjson::field("tenth", yyjson::multiple_of(0.1)),
                     yyjson::field("v", yyjson::items(yyjson::maximum(9007199254740991LL))));

// reals that a format of write changes, and a long double that a double does not hold
struct VdFormatted
{
    double d = 0.12;
    long double l = 0.12L;
};
template <>
inline constexpr auto yyjson::annotations<VdFormatted> =
    yyjson::annotate(yyjson::field("d", yyjson::minimum(0.12)), yyjson::field("l", yyjson::minimum(0.12)));
struct VdWide
{
    long double w = std::numeric_limits<long double>::max();
};
template <>
inline constexpr auto yyjson::annotations<VdWide> = yyjson::annotate(yyjson::field("w", yyjson::minimum(0)));

TEST(Validate, RealNumberKeywords)
{
    expect_valid(VdDecimal{0.3, {}});
    expect_invalid(VdDecimal{0.1 + 0.2, {}},
                   R"(the number is not a multiple of 0.1 at /tenth; reading the key "tenth" of VdDecimal)", "/tenth");
    expect_invalid(
        VdDecimal{},
        R"(the number is greater than the maximum 9007199254740991 at /v/0; reading an element of the value at the key "v" of VdDecimal)",
        "/v/0");
}

TEST(Validate, FloatNumberKeywords)
{
#if YYJSON_VERSION_HEX >= 0x000B00
    // with a format of the flag, the value and the written JSON must both pass, as read checks the JSON
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    const auto to_float = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT);
    expect_valid(VdFormatted{});
    EXPECT_EQ(std::string(yyjson::write(VdFormatted{}, yyjson::with_validation)), R"({"d":0.12,"l":0.12})");
    const auto e = validation_error([&] { (void)yyjson::write(VdFormatted{}, one_place, yyjson::with_validation); });
    EXPECT_EQ(
        std::string(e.what()),
        R"(the number is less than the minimum 0.12 at /d (line 1, column 6, pos 5); reading the key "d" of VdFormatted)");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(
        std::string(validation_error([&] {
                        (void)yyjson::write(VdFormatted{0.2, 0.12L}, one_place, yyjson::with_validation);
                    }).what()),
        R"(the number is less than the minimum 0.12 at /l (line 1, column 14, pos 13); reading the key "l" of VdFormatted)");
    const auto below = VdFormatted{std::nextafter(0.12, 0.0), 0.12L};
    const auto message = R"(the number is less than the minimum 0.12 at /d; reading the key "d" of VdFormatted)";
    expect_invalid(below, message, "/d");
    EXPECT_EQ(std::string(yyjson::write(below, to_float)), R"({"d":0.12,"l":0.12})");
    EXPECT_EQ(
        std::string(validation_error([&] { (void)yyjson::write(below, to_float, yyjson::with_validation); }).what()),
        message);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdDecimal{0.1 + 0.2, {}}, one_place, yyjson::with_validation);
                          }).what()),
              R"(the number is not a multiple of 0.1 at /tenth; reading the key "tenth" of VdDecimal)");
    EXPECT_EQ(std::string(yyjson::write(VdDecimal{0.3, {}}, one_place, yyjson::with_validation)),
              R"({"tenth":0.3,"v":[]})");
#endif
    // a long double above the range of a double is written as null
    if constexpr (sizeof(long double) > sizeof(double))
    {
        const auto wide = validation_error(
            [] { (void)yyjson::write(VdWide{}, yyjson::WriteFlag::InfAndNanAsNull, yyjson::with_validation); });
        EXPECT_EQ(std::string(wide.what()), "expected a number, got null at /w; reading long double");
        EXPECT_EQ(wide.path(), "/w");
    }
#if YYJSON_VERSION_HEX >= 0x000B00
    expect_valid(VdFloat{});
    expect_invalid(
        with<VdFloat>([](auto& t) { t.v = {0.1F, std::nextafter(0.1F, 1.0F)}; }),
        R"(the number is greater than the maximum 0.1 at /v/1; reading an element of the value at the key "v" of VdFloat)",
        "/v/1");
#endif
    expect_valid(VdFloatBelow{std::nextafter(0.1F, 0.0F)});
    expect_invalid(VdFloatBelow{},
                   R"(the number is greater than the maximum 0.099999999 at /f; reading the key "f" of VdFloatBelow)",
                   "/f");
#if YYJSON_VERSION_HEX >= 0x000C00
    // the extra items take the numbers as the flags write them
    const auto fixed = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    expect_valid(VdFloatRow{0, {0.5F, 0.14F}});
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdFloatRow{0, {0.5F, 0.14F}}, fixed, yyjson::with_validation);
                          }).what()),
              "the number is less than the minimum 0.12 at /2; reading an element of VdFloatRow");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(std::vector{VdFloatRow{0, {0.14F}}}, fixed, yyjson::with_validation);
                          }).what()),
              "the number is less than the minimum 0.12 at /0/1; reading an element of VdFloatRow");
#endif
}

// the values in containers, in tuples and in other structs, with the path to each
struct VdInner
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdInner> = yyjson::annotate(yyjson::field("n", yyjson::minimum(0)));

// the reals of a document, a raw number and a nested validate under a format of write
struct VdDomReal
{
    yyjson::value d = yyjson::read("0.12");
};
template <>
inline constexpr auto yyjson::annotations<VdDomReal> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0.12)));
struct VdRawReal
{
    yyjson::value d = yyjson::read("0.12", yyjson::ReadFlag::NumberAsRaw);
};
template <>
inline constexpr auto yyjson::annotations<VdRawReal> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0.12)));
inline bool vd_dom_valid(int)
{
    yyjson::validate(VdDomReal{});
    return true;
}
struct VdCallsValidate
{
    int i = 0;
    double x = 1;
};
template <>
inline constexpr auto yyjson::annotations<VdCallsValidate> =
    yyjson::annotate(yyjson::field("i", yyjson::satisfies<vd_dom_valid>));

TEST(Validate, WrittenFormatOfOtherNumbers)
{
#if YYJSON_VERSION_HEX >= 0x000B00
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    expect_valid(VdDomReal{});
    EXPECT_EQ(
        std::string(
            validation_error([&] { (void)yyjson::write(VdDomReal{}, one_place, yyjson::with_validation); }).what()),
        R"(the number is less than the minimum 0.12 at /d (line 1, column 6, pos 5); reading the key "d" of VdDomReal)");
    EXPECT_EQ(std::string(yyjson::write(VdCallsValidate{}, one_place, yyjson::with_validation)), R"({"i":0,"x":1.0})");
    // the writer copies a raw number as it is
    EXPECT_EQ(std::string(yyjson::write(VdRawReal{}, one_place, yyjson::with_validation)), R"({"d":0.12})");
    // a nested float above the range of a float is written as null
    const auto to_float = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT);
    const auto e = validation_error([&] {
        (void)yyjson::write(std::tuple{VdInner{}, 1e100}, to_float | yyjson::WriteFlag::InfAndNanAsNull,
                            yyjson::with_validation);
    });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /1; reading double");
    EXPECT_EQ(e.path(), "/1");
#endif
}
// a check of a read in a function of the user, which the format of an outer write does not reach
struct VdReadsDom
{
};
template <>
struct yyjson::caster<VdReadsDom>
{
    static VdReadsDom from_json(const std::string& s)
    {
        (void)yyjson::read<VdDomReal>(s);
        return {};
    }
};
struct VdUserReads
{
    yyjson::value by_caster = R"({"d":0.12})";
    std::string by_matcher = R"({"d":0.12})";
};
template <>
inline constexpr auto yyjson::annotations<VdUserReads> = yyjson::annotate(
    yyjson::field("by_caster", yyjson::matches<VdReadsDom>), yyjson::field("by_matcher", yyjson::pattern("<test>")));

TEST(Validate, FormatOutsideFunctionsOfUser)
{
    test_pattern = [](std::string_view s) {
        (void)yyjson::read<VdDomReal>(s);
        return true;
    };
#if YYJSON_VERSION_HEX >= 0x000B00
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    EXPECT_EQ(std::string(yyjson::write(VdUserReads{}, one_place, yyjson::with_validation)),
              R"({"by_caster":"{\"d\":0.12}","by_matcher":"{\"d\":0.12}"})");
#endif
    expect_valid(VdUserReads{});
    test_pattern = nullptr;
}

// `yyjson::validate(t)` and each checked write throw `message`, which the read of the written JSON gives
template <typename T>
void expect_invalid_text(const T& t, std::string_view message)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(t); }).what()), message);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(t, with_validation); }).what()), message);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(t, WriteFlag::NoFlag, with_validation); }).what()),
              message);
    auto out = std::string("previous");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::write(t, out, with_validation); }).what()), message);
    EXPECT_EQ(out, "previous");
}

// the reals of a document as write gives them: by the format of the value, and as null
struct VdFloatDom
{
    yyjson::value d = yyjson::value(0.1F);
};
template <>
inline constexpr auto yyjson::annotations<VdFloatDom> =
    yyjson::annotate(yyjson::field("d", yyjson::exclusive_minimum(0.1)));
struct VdNotNumber
{
    yyjson::value d = yyjson::value(-1e100);
};
template <>
inline constexpr auto yyjson::annotations<VdNotNumber> =
    yyjson::annotate(yyjson::field("d", yyjson::not_(yyjson::minimum(0))));

TEST(Validate, WrittenRealsOfDocument)
{
#if YYJSON_VERSION_HEX >= 0x000B00
    // a float in a value of the DOM is written with the shortest digits of the float
    EXPECT_EQ(std::string(yyjson::write(VdFloatDom{})), R"({"d":0.1})");
    expect_invalid_text(
        VdFloatDom{},
        R"(the number is not greater than the exclusive minimum 0.1 at /d (line 1, column 6, pos 5); reading the key "d" of VdFloatDom)");
#else
    expect_valid(VdFloatDom{});
#endif
    expect_valid(VdFloatDom{yyjson::value(0.2F)});
    expect_valid(VdNotNumber{});
    const auto as_null = [](const VdNotNumber& t, yyjson::WriteFlag flag) {
        return std::string(validation_error([&] { (void)yyjson::write(t, flag, yyjson::with_validation); }).what());
    };
    const auto expected =
        R"(the value satisfies the subschema of not at /d (line 1, column 6, pos 5); reading the key "d" of VdNotNumber)";
#if YYJSON_VERSION_HEX >= 0x000B00
    const auto to_float = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT);
    EXPECT_EQ(std::string(yyjson::write(VdNotNumber{}, to_float | yyjson::WriteFlag::InfAndNanAsNull)),
              R"({"d":null})");
    EXPECT_EQ(
        as_null(VdNotNumber{}, to_float | yyjson::WriteFlag::InfAndNanAsNull),
        R"(the value satisfies the subschema of not at /d (line 1, column 6, pos 5); reading the key "d" of VdNotNumber)");
#endif
    EXPECT_EQ(as_null(VdNotNumber{yyjson::value(-std::numeric_limits<double>::infinity())},
                      yyjson::WriteFlag::InfAndNanAsNull),
              expected);
    EXPECT_EQ(as_null(VdNotNumber{yyjson::value(std::nan(""))}, yyjson::WriteFlag::InfAndNanAsNull), expected);
}

// the listed values and the unique items take the reals as the format of write gives them
struct VdListedReal
{
    double d = 0.12;
    std::vector<double> v = {0.12, 0.14};
};
template <>
inline constexpr auto yyjson::annotations<VdListedReal> =
    yyjson::annotate(yyjson::field("d", yyjson::const_(0.1)), yyjson::field("v", yyjson::unique_items));

TEST(Validate, ListedWrittenReals)
{
    expect_invalid(VdListedReal{},
                   R"(the value is not equal to the constant 0.1 at /d; reading the key "d" of VdListedReal)", "/d");
#if YYJSON_VERSION_HEX >= 0x000B00
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    EXPECT_EQ(std::string(yyjson::write(VdListedReal{0.1, {0.12, 0.24}}, one_place, yyjson::with_validation)),
              R"({"d":0.1,"v":[0.1,0.2]})");
    const auto e = validation_error(
        [&] { (void)yyjson::write(VdListedReal{0.1, {0.12, 0.14}}, one_place, yyjson::with_validation); });
    EXPECT_EQ(
        std::string(e.what()),
        R"(the item is equal to an earlier item at /v/1; reading an element of the value at the key "v" of VdListedReal)");
    EXPECT_EQ(e.path(), "/v/1");
#endif
}

// the keywords that compare a written value of the DOM or a value of another type
struct VdTextConst
{
    yyjson::value c = yyjson::read("0.12");
    yyjson::value e = yyjson::read("0.12");
};
template <>
inline constexpr auto yyjson::annotations<VdTextConst> =
    yyjson::annotate(yyjson::field("c", yyjson::const_(yyjson::json_text("0.12"))),
                     yyjson::field("e", yyjson::enum_(yyjson::json_text("0.12"), yyjson::json_text("5"))));
struct VdPart
{
    double d = 0.12;
};
template <>
inline constexpr auto yyjson::annotations<VdPart> =
    yyjson::annotate(yyjson::field("d", yyjson::exclusive_minimum(0.1)));
struct VdMatchesDom
{
    yyjson::value n = 1.0;
    yyjson::value p = yyjson::read(R"({"d":0.12})");
};
template <>
inline constexpr auto yyjson::annotations<VdMatchesDom> =
    yyjson::annotate(yyjson::field("n", yyjson::matches<double>), yyjson::field("p", yyjson::matches<VdPart>));
struct VdUniqueDom
{
    yyjson::value a = yyjson::read("[[0.12],[0.14]]");
};
template <>
inline constexpr auto yyjson::annotations<VdUniqueDom> = yyjson::annotate(yyjson::field("a", yyjson::unique_items));
inline bool vd_above(double d) { return d > 0.11; }
struct VdPredicate
{
    double d = 0.12;
};
template <>
inline constexpr auto yyjson::annotations<VdPredicate> =
    yyjson::annotate(yyjson::field("d", yyjson::satisfies<vd_above>));

TEST(Validate, WrittenJsonOfFormats)
{
    const auto as_null = yyjson::WriteFlag::InfAndNanAsNull;
    const auto error = [](const auto& t, yyjson::WriteFlag flag) {
        auto out = std::string("previous");
        const auto e = validation_error([&] { yyjson::write(t, out, flag, yyjson::with_validation); });
        EXPECT_EQ(out, "previous");
        EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(t, flag, yyjson::with_validation); }).what()),
                  e.what());
        return std::string(e.what());
    };
    // without a format, a real of the DOM that write gives as null is checked as null
    auto infinite = yyjson::value(-std::numeric_limits<double>::infinity());
    EXPECT_EQ(
        error(VdMatchesDom{infinite}, as_null),
        R"(the value does not satisfy matches at /n (line 1, column 6, pos 5); reading the key "n" of VdMatchesDom)");
    yyjson::value reals = yyjson::read("[]");
    reals.as_array()->emplace_back(std::nan(""));
    reals.as_array()->emplace_back(std::numeric_limits<double>::infinity());
    EXPECT_EQ(
        error(VdUniqueDom{reals}, as_null),
        R"(the item is equal to an earlier item at /a/1 (line 1, column 12, pos 11); reading an element of the value at the key "a" of VdUniqueDom)");
#if YYJSON_VERSION_HEX >= 0x000B00
    yyjson::value floats = yyjson::read("[]");
    floats.as_array()->emplace_back(0.1F);
    floats.as_array()->emplace_back(0.1);
    expect_invalid_text(
        VdUniqueDom{floats},
        R"(the item is equal to an earlier item at /a/1 (line 1, column 11, pos 10); reading an element of the value at the key "a" of VdUniqueDom)");
    // with a format, read checks the written JSON
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    expect_valid(VdTextConst{});
    EXPECT_EQ(
        error(VdTextConst{}, one_place),
        R"(the value is not equal to the constant 0.12 at /c (line 1, column 6, pos 5); reading the key "c" of VdTextConst)");
    EXPECT_EQ(error(VdTextConst{yyjson::read("0.12"), yyjson::read("0.14")}, yyjson::WriteFlag::NoFlag),
              R"(the value is not one of 0.12, 5 at /e; reading the key "e" of VdTextConst)");
    EXPECT_EQ(
        error(VdTextConst{yyjson::read("0.12", yyjson::ReadFlag::NumberAsRaw), yyjson::read("0.12")}, one_place),
        R"(the value is not one of 0.12, 5 at /e (line 1, column 15, pos 14); reading the key "e" of VdTextConst)");
    expect_valid(VdMatchesDom{});
    EXPECT_EQ(
        error(VdMatchesDom{}, one_place),
        R"(the value does not satisfy matches at /p (line 1, column 14, pos 13); reading the key "p" of VdMatchesDom)");
    expect_valid(VdUniqueDom{});
    EXPECT_EQ(
        error(VdUniqueDom{}, one_place),
        R"(the item is equal to an earlier item at /a/1 (line 1, column 13, pos 12); reading an element of the value at the key "a" of VdUniqueDom)");
    expect_valid(VdPredicate{});
    EXPECT_EQ(
        error(VdPredicate{}, one_place),
        R"(the value does not satisfy the predicate at /d (line 1, column 6, pos 5); reading the key "d" of VdPredicate)");
    EXPECT_EQ(std::string(yyjson::write(VdPredicate{0.2}, one_place, yyjson::with_validation)), R"({"d":0.2})");
#endif
}

// a cast compares the stored number of a real of the DOM, whatever format write gives it
struct VdCastReal
{
    yyjson::value d;
};
template <>
inline constexpr auto yyjson::annotations<VdCastReal> =
    yyjson::annotate(yyjson::field("d", yyjson::exclusive_minimum(0.1)));

TEST(Validate, CastComparesStoredReal)
{
    auto obj = yyjson::object();
    obj.emplace("d", 0.1F);
    EXPECT_EQ(*yyjson::value(obj).cast<VdCastReal>().d.as_real(), static_cast<double>(0.1F));
}

// the check reads the written JSON of a value of the DOM, also through a C array, a caster and a variant
struct VdDomArray
{
    yyjson::value a[2];
};
template <>
inline constexpr auto yyjson::annotations<VdDomArray> = yyjson::annotate(yyjson::field("a", yyjson::unique_items));
struct VdTwoReaders
{
    yyjson::value v;
};
template <>
struct yyjson::caster<VdTwoReaders>
{
    // a read of a field takes the second overload
    static VdTwoReaders from_json(const yyjson::reader::const_value_ref&)
    {
        throw std::logic_error("the returning from_json");
    }
    static void from_json(VdTwoReaders& t, const yyjson::reader::const_value_ref& v) { t.v = yyjson::value(v); }
    static yyjson::value to_json(const VdTwoReaders& t) { return t.v; }
};
using VdDomOrText = yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::exclusive_minimum(0.1)>, std::string>;
struct VdDomHolders
{
    VdTwoReaders r{yyjson::value(1)};
    VdDomOrText x = std::string("a");
};
template <>
inline constexpr auto yyjson::annotations<VdDomHolders> =
    yyjson::annotate(yyjson::field("r", yyjson::exclusive_minimum(0.1)));

// a caster that checks the number in `from_json` only
struct VdReadChecked
{
    double d = 0.12;
};
template <>
struct yyjson::caster<VdReadChecked>
{
    static VdReadChecked from_json(double d)
    {
        if (d < 0.11) throw std::range_error("below 0.11");
        return {d};
    }
    static double to_json(const VdReadChecked& t) { return t.d; }
};

// a caster that writes a real through an emitter, and checks it in `from_json` only
struct VdEmittedReal
{
    double d = 0.12;
};
template <>
struct yyjson::caster<VdEmittedReal>
{
    static void to_json(yyjson::object_emitter& e, const VdEmittedReal& t) { e.emplace("d", t.d); }
    static VdEmittedReal from_json(const yyjson::reader::const_value_ref& v)
    {
        const auto d = v.as_object()->operator[]("d").as_real().value();
        if (d <= 0.11) throw std::range_error("not above 0.11");
        return {d};
    }
};

// the same writer for a const value only, as write calls it
struct VdConstEmitted
{
    double d = 0.12;
};
template <>
struct yyjson::caster<VdConstEmitted>
{
    static void to_json(yyjson::object_emitter& e, const VdConstEmitted& t) { e.emplace("d", t.d); }
    static void to_json(yyjson::object_emitter&, VdConstEmitted&&) = delete;
    static VdConstEmitted from_json(const yyjson::reader::const_value_ref& v)
    {
        const auto d = v.as_object()->operator[]("d").as_real().value();
        if (d <= 0.11) throw std::range_error("not above 0.11");
        return {d};
    }
};

// a caster that copies a value of the DOM through an emitter, and reads it with a declared constraint
struct VdCopiedInner
{
    double d;
};
template <>
inline constexpr auto yyjson::annotations<VdCopiedInner> =
    yyjson::annotate(yyjson::field("d", yyjson::exclusive_minimum(0.1)));
struct VdCopiesValue
{
    yyjson::value v;
};
template <>
struct yyjson::caster<VdCopiesValue>
{
    static void to_json(yyjson::object_emitter& e, const VdCopiesValue& t) { e.emplace("d", t.v); }
    static VdCopiesValue from_json(const VdCopiedInner& i) { return {yyjson::value(i.d)}; }
};

// a converter of `yyjson::with_caster` that only reads, which a write must not call
struct VdReadOnlyInt
{
    static int from_json(int) { throw std::logic_error("from_json of VdReadOnlyInt"); }
};

// a chain of casters whose moved values lead to new types, which write never reaches
template <int N>
struct VdChainA
{
};
template <int N>
struct VdChainB
{
};
template <int N>
struct yyjson::caster<VdChainA<N>>
{
    static int to_json(const VdChainA<N>&) { return 7; }
    static VdChainB<N> to_json(VdChainA<N>&&) { return {}; }
    static VdChainA<N> from_json(int) { return {}; }
};
template <int N>
struct yyjson::caster<VdChainB<N>>
{
    static VdChainA<N + 1> to_json(const VdChainB<N>&) { return {}; }
    static int to_json(VdChainB<N>&&) { return 7; }
};

// a caster whose `from_json` calls write, which must not take `yyjson::emit_defaults` of the outer write
struct VdInnerDefault
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdInnerDefault> = yyjson::annotate(yyjson::field("n", yyjson::default_));
inline std::string vd_inner_text;
struct VdWritesInRead
{
    int n = 0;
};
template <>
struct yyjson::caster<VdWritesInRead>
{
    static VdWritesInRead from_json(int n)
    {
        vd_inner_text = std::string(yyjson::write(VdInnerDefault{}));
        return {n};
    }
    static int to_json(const VdWritesInRead& t) { return t.n; }
};
struct VdReadWrites
{
    double d = 0.5;
    VdWritesInRead w;
    VdInnerDefault e;
};

// casters whose `from_json` cannot read: the check before a write does not instantiate them
struct VdTemplatedReader
{
    int n = 1;
};
template <>
struct yyjson::caster<VdTemplatedReader>
{
    template <typename J>
    static auto from_json(const J& j)
    {
        return VdTemplatedReader{j.no_such_member()};
    }
    static int to_json(const VdTemplatedReader& t) { return t.n; }
};
struct VdRealTemplatedReader
{
    double d = 0.5;
};
template <>
struct yyjson::caster<VdRealTemplatedReader>
{
    template <typename J>
    static VdRealTemplatedReader from_json(const J& j)
    {
        return {j.no_such_member()};
    }
    static double to_json(const VdRealTemplatedReader& t) { return t.d; }
};

TEST(Validate, WrittenJsonOfDom)
{
    using yyjson::with_validation;
    EXPECT_EQ(
        std::string(validation_error([] {
                        (void)yyjson::write(VdDomArray{{yyjson::value(0.1F), yyjson::value(0.1F)}}, with_validation);
                    }).what()),
        R"(the item is equal to an earlier item at /a/1; reading an element of the value at the key "a" of VdDomArray)");
    const auto low = validation_error([] {
        (void)yyjson::write(VdDomHolders{VdTwoReaders{yyjson::value(1)}, VdDomOrText{yyjson::value(0.05)}},
                            with_validation);
    });
    EXPECT_TRUE(std::string_view(low.what()).starts_with("no alternative accepts the value at /x;")) << low.what();
#if YYJSON_VERSION_HEX >= 0x000B00
    // write gives each `float` as `0.1`
    const auto floats = VdDomArray{{yyjson::value(0.1F), yyjson::value(0.1)}};
    EXPECT_EQ(
        std::string(validation_error([&] { yyjson::validate(floats); }).what()),
        R"(the item is equal to an earlier item at /a/1; reading an element of the value at the key "a" of VdDomArray)");
    EXPECT_EQ(
        std::string(validation_error([&] { (void)yyjson::write(floats, with_validation); }).what()),
        R"(the item is equal to an earlier item at /a/1; reading an element of the value at the key "a" of VdDomArray)");
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdDomHolders{VdTwoReaders{yyjson::value(0.1F)}}, with_validation);
                          }).what()),
              R"(the number is not greater than the exclusive minimum 0.1 at /r; reading the key "r" of VdDomHolders)");
    // the read of the written JSON gives the error of the variant
    const auto written = validation_error([] {
        (void)yyjson::write(VdDomHolders{VdTwoReaders{yyjson::value(1)}, VdDomOrText{yyjson::value(0.1F)}},
                            with_validation);
    });
    EXPECT_TRUE(
        std::string_view(written.what()).starts_with("no alternative accepts the value at /x (line 1, column 12"))
        << written.what();
#endif
    EXPECT_EQ(std::string(yyjson::write(VdDomHolders{}, with_validation)), R"({"r":1,"x":"a"})");
#if YYJSON_VERSION_HEX >= 0x000C00
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    EXPECT_EQ(std::string(yyjson::write(VdReadChecked{}, with_validation)), "0.12");
    try
    {
        (void)yyjson::write(VdReadChecked{}, one_place, with_validation);
        ADD_FAILURE() << "no exception";
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "below 0.11");
    }
#endif
    EXPECT_EQ(std::string(yyjson::write(VdTemplatedReader{}, yyjson::WriteFlag::NoFlag)), "1");
    EXPECT_EQ(std::string(yyjson::write(VdTemplatedReader{}, with_validation)), "1");
    EXPECT_EQ(std::string(yyjson::write(VdTemplatedReader{}, yyjson::WriteFlag::NoFlag, with_validation)), "1");
    EXPECT_NO_THROW(yyjson::validate(VdTemplatedReader{}));
    auto out = std::string();
    EXPECT_EQ(std::string(yyjson::write(VdRealTemplatedReader{}, with_validation)), "0.5");
    EXPECT_EQ(std::string(yyjson::write(VdRealTemplatedReader{}, yyjson::emit_defaults, with_validation)), "0.5");
    yyjson::write(VdRealTemplatedReader{}, out, with_validation);
    EXPECT_EQ(out, "0.5");
    yyjson::write(VdRealTemplatedReader{}, out, yyjson::emit_defaults, with_validation);
    EXPECT_EQ(out, "0.5");
    EXPECT_NO_THROW(yyjson::validate(VdRealTemplatedReader{}));
}

TEST(Validate, WrittenJsonOfCasters)
{
    using yyjson::with_validation;
    EXPECT_EQ(std::string(yyjson::write(VdChainA<0>{}, with_validation)), "7");
#if YYJSON_VERSION_HEX >= 0x000C00
    const auto one_place = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    EXPECT_EQ(std::string(yyjson::write(VdEmittedReal{}, with_validation)), R"({"d":0.12})");
    try
    {
        (void)yyjson::write(VdEmittedReal{}, one_place, with_validation);
        ADD_FAILURE() << "no exception";
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "not above 0.11");
    }
    // the read of the written JSON calls `from_json`, and the write in it leaves out the default
    vd_inner_text.clear();
    EXPECT_EQ(std::string(yyjson::write(VdReadWrites{}, one_place, yyjson::emit_defaults, with_validation)),
              R"({"d":0.5,"w":0,"e":{"n":0}})");
    EXPECT_EQ(vd_inner_text, "{}");
    // the writer that takes a const value only
    try
    {
        (void)yyjson::write(VdConstEmitted{}, one_place, with_validation);
        ADD_FAILURE() << "no exception";
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "not above 0.11");
    }
    // the library writes the value of `with_caster`, and the converter only reads
    EXPECT_EQ(std::string(yyjson::write(yyjson::with_caster<int, VdReadOnlyInt>{7}, one_place, with_validation)), "7");
    // a chain of casters that the check follows as write does, without a moved value
    EXPECT_EQ(std::string(yyjson::write(VdChainA<0>{}, one_place, with_validation)), "7");
#endif
}

TEST(Validate, WrittenJsonOfCopiedDom)
{
    using yyjson::with_validation;
#if YYJSON_VERSION_HEX >= 0x000B00
    expect_invalid_text(
        VdCopiesValue{yyjson::value(0.1F)},
        R"(the number is not greater than the exclusive minimum 0.1 at /d (line 1, column 6, pos 5); reading the key "d" of VdCopiedInner)");
#endif
    EXPECT_NO_THROW((void)yyjson::write(VdCopiesValue{yyjson::value(0.2F)}, with_validation));
    const auto inf = VdCopiesValue{yyjson::value(-std::numeric_limits<double>::infinity())};
    try
    {
        (void)yyjson::write(inf, yyjson::WriteFlag::InfAndNanAsNull, with_validation);
        ADD_FAILURE() << "no exception";
    }
    catch (const std::exception& e)
    {
        EXPECT_EQ(std::string(e.what()), "expected a number, got null at /d (line 1, column 6, pos 5); reading double");
    }
}

struct VdNested
{
    std::vector<VdInner> list;
    std::map<std::string, VdInner> by_name;
    std::map<int, VdInner> by_id;
    std::map<VdColor, VdInner> by_color;
    std::tuple<int, VdInner> pair;
    VdInner inner;
    std::array<VdInner, 2> fixed;
};

TEST(Validate, NestedValues)
{
    const auto bad = VdInner{-1};
    expect_valid(VdNested{{{1}}, {{"a/b", {2}}}, {{3, {3}}}, {{VdColor::green, {4}}}, {1, {5}}, {6}, {}});
    expect_invalid(with<VdNested>([&](auto& t) { t.list = {{}, bad}; }),
                   R"(the number is less than the minimum 0 at /list/1/n; reading the key "n" of VdInner)",
                   "/list/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"a/b", bad}}; }),
                   R"(the number is less than the minimum 0 at /by_name/a~1b/n; reading the key "n" of VdInner)",
                   "/by_name/a~1b/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_name = {{"m~n", bad}}; }),
                   R"(the number is less than the minimum 0 at /by_name/m~0n/n; reading the key "n" of VdInner)",
                   "/by_name/m~0n/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_id = {{7, bad}}; }),
                   R"(the number is less than the minimum 0 at /by_id/0/1/n; reading the key "n" of VdInner)",
                   "/by_id/0/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.by_color = {{VdColor::green, bad}}; }),
                   R"(the number is less than the minimum 0 at /by_color/green/n; reading the key "n" of VdInner)",
                   "/by_color/green/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.pair = {0, bad}; }),
                   R"(the number is less than the minimum 0 at /pair/1/n; reading the key "n" of VdInner)",
                   "/pair/1/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.inner = bad; }),
                   R"(the number is less than the minimum 0 at /inner/n; reading the key "n" of VdInner)", "/inner/n");
    expect_invalid(with<VdNested>([&](auto& t) { t.fixed = {VdInner{}, bad}; }),
                   R"(the number is less than the minimum 0 at /fixed/1/n; reading the key "n" of VdInner)",
                   "/fixed/1/n");
    expect_invalid(std::vector<VdInner>{{}, {}, bad},
                   R"(the number is less than the minimum 0 at /2/n; reading the key "n" of VdInner)", "/2/n");
    expect_invalid(bad, R"(the number is less than the minimum 0 at /n; reading the key "n" of VdInner)", "/n");
}

// the values that pointers, `std::optional`, `yyjson::nullable` and `yyjson::indirect` hold, and the null that
// they are written as when they hold none
struct VdHolders
{
    std::optional<int> o;
    yyjson::nullable<int> n;
    std::shared_ptr<VdInner> p;
    std::unique_ptr<int> u;
    yyjson::indirect<VdInner> i = yyjson::indirect<VdInner>();
};
template <>
inline constexpr auto yyjson::annotations<VdHolders> =
    yyjson::annotate(yyjson::field("o", yyjson::minimum(0)), yyjson::field("n", yyjson::minimum(0)),
                     yyjson::field("u", yyjson::maximum(5)));
// a null passes `yyjson::minimum` and fails its negation
struct VdNotMinimum
{
    std::shared_ptr<int> p;
};
template <>
inline constexpr auto yyjson::annotations<VdNotMinimum> =
    yyjson::annotate(yyjson::field("p", yyjson::not_(yyjson::minimum(0))));
// a null that a keyword rejects
struct VdConstNull
{
    yyjson::nullable<int> n;
};
template <>
inline constexpr auto yyjson::annotations<VdConstNull> = yyjson::annotate(yyjson::field("n", yyjson::const_(1)));

TEST(Validate, Holders)
{
    expect_valid(VdHolders{});
    expect_valid(with<VdHolders>([](auto& t) {
        t.o = 1;
        t.n = 2;
        t.p = std::make_shared<VdInner>();
        t.u = std::make_unique<int>(5);
    }));
    expect_invalid(with<VdHolders>([](auto& t) { t.o = -1; }),
                   R"(the number is less than the minimum 0 at /o; reading the key "o" of VdHolders)", "/o");
    expect_invalid(with<VdHolders>([](auto& t) { t.n = -1; }),
                   R"(the number is less than the minimum 0 at /n; reading the key "n" of VdHolders)", "/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.p = std::make_shared<VdInner>(-1); }),
                   R"(the number is less than the minimum 0 at /p/n; reading the key "n" of VdInner)", "/p/n");
    expect_invalid(with<VdHolders>([](auto& t) { t.u = std::make_unique<int>(6); }),
                   R"(the number is greater than the maximum 5 at /u; reading the key "u" of VdHolders)", "/u");
    expect_invalid(with<VdHolders>([](auto& t) { t.i = yyjson::indirect<VdInner>(VdInner{-1}); }),
                   R"(the number is less than the minimum 0 at /i/n; reading the key "n" of VdInner)", "/i/n");

    expect_valid(VdNotMinimum{std::make_shared<int>(-1)});
    expect_invalid(VdNotMinimum{},
                   R"(the value satisfies the subschema of not at /p; reading the key "p" of VdNotMinimum)", "/p");
    expect_invalid(VdNotMinimum{std::make_shared<int>(0)},
                   R"(the value satisfies the subschema of not at /p; reading the key "p" of VdNotMinimum)", "/p");
    expect_valid(VdConstNull{1});
    expect_invalid(VdConstNull{},
                   R"(the value is not equal to the constant 1 at /n; reading the key "n" of VdConstNull)", "/n");
}

// `yyjson::satisfies` and the combinators
bool vd_is_even(int v) { return v % 2 == 0; }
struct VdLogic
{
    int even = 0;
    int any = 0;
    int one = 6;
    int all = 1;
    int cond = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdLogic> = yyjson::annotate(
    yyjson::field("even", yyjson::satisfies<vd_is_even>),
    yyjson::field("any", yyjson::any_of(yyjson::maximum(0), yyjson::minimum(10))),
    yyjson::field("one", yyjson::one_of(yyjson::maximum(5), yyjson::minimum(0))),
    yyjson::field("all", yyjson::all_of(yyjson::minimum(1), yyjson::maximum(3))),
    yyjson::field("cond", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                               yyjson::else_(yyjson::maximum(5)))));

TEST(Validate, SatisfiesAndCombinators)
{
    expect_valid(VdLogic{});
    expect_valid(VdLogic{2, 10, 6, 3, 12});
    expect_invalid(with<VdLogic>([](auto& t) { t.even = 1; }),
                   R"(the value does not satisfy the predicate at /even; reading the key "even" of VdLogic)", "/even");
    expect_invalid(with<VdLogic>([](auto& t) { t.any = 5; }),
                   R"(the value satisfies none of the subschemas of anyOf at /any; reading the key "any" of VdLogic)",
                   "/any");
    expect_invalid(with<VdLogic>([](auto& t) { t.one = 3; }),
                   R"(the value satisfies more than one subschema of oneOf at /one; reading the key "one" of VdLogic)",
                   "/one");
    expect_invalid(
        with<VdLogic>([](auto& t) { t.all = 4; }),
        R"(the value does not satisfy all the subschemas of allOf at /all; reading the key "all" of VdLogic)", "/all");
    expect_invalid(with<VdLogic>([](auto& t) { t.cond = 11; }),
                   R"(the value satisfies if and not then at /cond; reading the key "cond" of VdLogic)", "/cond");
    expect_invalid(with<VdLogic>([](auto& t) { t.cond = 6; }),
                   R"(the value satisfies neither if nor else at /cond; reading the key "cond" of VdLogic)", "/cond");
}

// a transparent struct, which is written as its field, and the fields of DOM values
struct VdPercent
{
    int value = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPercent> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::minimum(0), yyjson::maximum(100)));
struct VdDom
{
    VdPercent pct;
    yyjson::value any;
};
template <>
inline constexpr auto yyjson::annotations<VdDom> =
    yyjson::annotate(yyjson::field("any", yyjson::min_items(2), yyjson::items(yyjson::minimum(0))));

TEST(Validate, TransparentAndDom)
{
    expect_valid(VdPercent{100});
    expect_invalid(VdPercent{101}, "the number is greater than the maximum 100; reading VdPercent", "");
    expect_valid(VdDom{{5}, yyjson::read("[1,2]")});
    expect_invalid(VdDom{{-1}, yyjson::read("[1,2]")},
                   "the number is less than the minimum 0 at /pct; reading VdPercent", "/pct");
    expect_invalid(VdDom{{1}, yyjson::read("[1]")},
                   R"(the array has fewer items than the minimum 2 at /any; reading the key "any" of VdDom)", "/any");
    expect_invalid(
        VdDom{{1}, yyjson::read("[1,-1]")},
        R"(the number is less than the minimum 0 at /any/1; reading an element of the value at the key "any" of VdDom)",
        "/any/1");
}

// `yyjson::as_array`, whose positions and extra items are checked as they are written
struct VdRow
{
    int id = 0;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdRow> =
    yyjson::annotate(yyjson::as_array, yyjson::field("id", yyjson::minimum(0)),
                     yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::maximum(5))));

TEST(Validate, AsArray)
{
    expect_valid(VdRow{1, {2, 5}});
    expect_invalid(VdRow{-1, {}}, "the number is less than the minimum 0 at /0; reading an element of VdRow", "/0");
    expect_invalid(VdRow{1, {1, 6}}, "the number is greater than the maximum 5 at /2; reading an element of VdRow",
                   "/2");
}

// the keywords of the whole array of `yyjson::as_array`, counted on the array that write gives: the positions up
// to the first that it leaves out, then the extra items
struct VdArrayKeys
{
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
    std::vector<std::optional<int>> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayKeys> =
    yyjson::annotate(yyjson::as_array, yyjson::min_items(3), yyjson::max_items(4), yyjson::unique_items,
                     yyjson::contains(yyjson::const_(7)), yyjson::field("rest", yyjson::extra_items));
struct VdArrayShort
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayShort> = yyjson::annotate(yyjson::as_array, yyjson::min_items(2));
struct VdArrayAny
{
    int a = 0;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayAny> =
    yyjson::annotate(yyjson::as_array, yyjson::any_of(yyjson::contains(yyjson::const_(7)), yyjson::min_items(3)),
                     yyjson::field("rest", yyjson::extra_items));
struct VdArrayCell
{
    int x = 0;
    std::optional<int> y;
};
// `yyjson::max_properties` in `yyjson::contains` counts the members of the JSON object of an element
struct VdArrayCells
{
    VdArrayCell a;
    VdArrayCell b;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayCells> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::not_(yyjson::contains(yyjson::max_properties(1))));
struct VdArrayHolder
{
    VdArrayKeys row;
    VdArrayCells cells;
};
template <>
inline constexpr auto yyjson::annotations<VdArrayHolder> = yyjson::annotate(yyjson::additional_properties(false));

TEST(Validate, AsArrayWholeArray)
{
    static_assert(yyjson::detail::needs_validation<VdArrayShort>);
    const auto keys = VdArrayKeys{7, 2, 3, {}};
    expect_valid(keys);
    expect_invalid(VdArrayKeys{7, 2, std::nullopt, {}},
                   "the array has fewer items than the minimum 3; reading VdArrayKeys", "");
    expect_invalid(VdArrayKeys{1, 1, 7, {}},
                   "the item is equal to an earlier item at /1; reading an element of VdArrayKeys", "/1");
    expect_invalid(VdArrayKeys{1, 2, 3, {}}, "the array has no item that satisfies contains; reading VdArrayKeys", "");
    expect_invalid(VdArrayKeys{1, 2, 7, {8, 9}}, "the array has more items than the maximum 4; reading VdArrayKeys",
                   "");
    expect_valid(VdArrayKeys{1, 2, 3, {7}});
    expect_invalid(VdArrayKeys{7, 2, 3, {2}},
                   "the item is equal to an earlier item at /3; reading an element of VdArrayKeys", "/3");
    expect_valid(VdArrayShort{1, 2});
    expect_invalid(VdArrayShort{1, std::nullopt}, "the array has fewer items than the minimum 2; reading VdArrayShort",
                   "");

    // write throws for a position that it leaves out before an extra item, and the keywords are not checked
    const auto gap = VdArrayKeys{7, 2, std::nullopt, {3}};
    EXPECT_NO_THROW(yyjson::validate(gap));
    EXPECT_THROW((void)yyjson::write(gap, yyjson::with_validation), yyjson::write_error);

    expect_valid(VdArrayAny{7, {}});
    expect_valid(VdArrayAny{1, {2, 3}});
    expect_invalid(VdArrayAny{1, {2}}, "the value satisfies none of the subschemas of anyOf; reading VdArrayAny", "");

    const auto cells = VdArrayCells{{1, 2}, {1, 2}};
    expect_valid(cells);
    expect_invalid(VdArrayCells{{1, 2}, {}}, "the value satisfies the subschema of not; reading VdArrayCells", "");

    expect_valid(VdArrayHolder{keys, cells});
    expect_invalid(VdArrayHolder{{1, 1, 7, {}}, cells},
                   "the item is equal to an earlier item at /row/1; reading an element of VdArrayKeys", "/row/1");
    expect_invalid(VdArrayHolder{keys, {}}, "the value satisfies the subschema of not at /cells; reading VdArrayCells",
                   "/cells");
    expect_invalid(std::vector<VdArrayKeys>{keys, {7, 2, std::nullopt, {}}},
                   "the array has fewer items than the minimum 3 at /1; reading VdArrayKeys", "/1");
    expect_invalid(std::vector<VdArrayCells>{cells, {}},
                   "the value satisfies the subschema of not at /1; reading VdArrayCells", "/1");
}

// a field that write leaves out is not checked, and a field that a read gives another value is a violation; each
// default here violates the keywords of its field, so that write leaves out a field equal to it in every form
struct VdWeight
{
    int a = 0;
    int note = 7;
};
template <>
inline constexpr auto yyjson::annotations<VdWeight> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("note", yyjson::skip));
struct VdDefaults
{
    int d = 0;
    int e = 0;
    VdWeight w;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaults> =
    yyjson::annotate(yyjson::field("d", yyjson::minimum(1), yyjson::default_),
                     yyjson::field("e", yyjson::minimum(1), yyjson::default_, yyjson::emit_defaults),
                     yyjson::field("w", yyjson::default_));
// defaults that satisfy their keywords, which `yyjson::emit_defaults` writes past the maximum of the struct
struct VdCapped
{
    int a = 1;
    int b = 2;
};
template <>
inline constexpr auto yyjson::annotations<VdCapped> =
    yyjson::annotate(yyjson::max_properties(1), yyjson::field("a", yyjson::default_, yyjson::minimum(1)),
                     yyjson::field("b", yyjson::default_));
int vd_three() { return 3; }
struct VdReadBack
{
    std::optional<int> x = 3;
    std::optional<int> y;
    std::optional<int> z = 3;
};
template <>
inline constexpr auto yyjson::annotations<VdReadBack> =
    yyjson::annotate(yyjson::field("z", yyjson::default_from<vd_three>));
struct VdOptionalPercent
{
    std::optional<int> value;
};
template <>
inline constexpr auto yyjson::annotations<VdOptionalPercent> = yyjson::annotate(yyjson::transparent);
struct VdReadBackTransparent
{
    VdOptionalPercent p = {4};
};

// elements that the keywords compare as the JSON values that write gives
struct VdZeroed
{
    int x = 0;
    bool operator==(const VdZeroed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VdZeroed> = yyjson::annotate(yyjson::field("x", yyjson::default_));
struct VdUniqueWritten
{
    std::vector<VdZeroed> defaulted;
    std::vector<double> reals;
    std::vector<std::optional<std::optional<int>>> nulls;
};
template <>
inline constexpr auto yyjson::annotations<VdUniqueWritten> =
    yyjson::annotate(yyjson::field("defaulted", yyjson::unique_items), yyjson::field("reals", yyjson::unique_items),
                     yyjson::field("nulls", yyjson::unique_items));
struct VdUniquePair
{
    double a = 0;
    double b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUniquePair> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::unique_items);

constexpr bool above_one(double x) { return x > 1; }
struct VdWrittenItems
{
    double s = 1.0001;
    std::vector<double> v = {1.0001};
    std::map<std::string, std::vector<double>> m = {{"a", {1.0001}}};
};
template <>
inline constexpr auto yyjson::annotations<VdWrittenItems> = yyjson::annotate(
    yyjson::field("s", yyjson::const_(1), yyjson::satisfies<above_one>),
    yyjson::field("v", yyjson::items(yyjson::const_(1), yyjson::satisfies<above_one>)),
    yyjson::field("m", yyjson::additional_properties(yyjson::items(yyjson::const_(1), yyjson::satisfies<above_one>))));

struct VdEscapedKey
{
    std::map<std::string, std::vector<double>> m;
};
template <>
inline constexpr auto yyjson::annotations<VdEscapedKey> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::items(yyjson::const_(1)))));

TEST(Validate, UniqueWrittenJson)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    const auto at = [](std::string_view key, std::string_view path) {
        return std::format(R"(the item is equal to an earlier item at {}; reading an element of the value at the )"
                           R"(key "{}" of VdUniqueWritten)",
                           path, key);
    };
    // both elements write {}, or {"x":0} with `yyjson::emit_defaults`
    const auto defaulted = VdUniqueWritten{.defaulted = {{0}, {0}}, .reals = {}, .nulls = {}};
    expect_invalid(defaulted, at("defaulted", "/defaulted/1"), "/defaulted/1");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(defaulted, emit_defaults); }).what()),
              at("defaulted", "/defaulted/1"));
    // both optionals write null
    expect_invalid(VdUniqueWritten{.defaulted = {}, .reals = {}, .nulls = {std::nullopt, std::optional<int>{}}},
                   at("nulls", "/nulls/1"), "/nulls/1");
#if YYJSON_VERSION_HEX >= 0x000C00
    // the flags write the two doubles as the same number
    const auto fixed = static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(3));
    const auto reals = VdUniqueWritten{.defaulted = {}, .reals = {1.0, 1.0000000000000002}, .nulls = {}};
    expect_valid(reals);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(reals, fixed, with_validation); }).what()),
              at("reals", "/reals/1"));
    const auto pair = VdUniquePair{1.0, 1.0000000000000002};
    expect_valid(pair);
    EXPECT_EQ(std::string(validation_error([&] { (void)yyjson::write(pair, fixed, with_validation); }).what()),
              "the item is equal to an earlier item at /1; reading an element of VdUniquePair");
    // the predicate takes the number that the flags write, as `yyjson::read` gives it
    const auto rounded = std::string(
        R"(the value does not satisfy the predicate at /s (line 1, column 6, pos 5); reading the key "s" of )"
        R"(VdWrittenItems)");
    EXPECT_EQ(
        std::string(validation_error([&] { (void)yyjson::write(VdWrittenItems{}, fixed, with_validation); }).what()),
        rounded);
    // the flags write another key, and the member still takes the constant of its entry
    const auto invalid_key = std::string(1, static_cast<char>(0xff));
    const auto escaped = fixed | yyjson::WriteFlag::AllowInvalidUnicode | yyjson::WriteFlag::EscapeUnicode;
    EXPECT_EQ(std::string(yyjson::write(VdEscapedKey{{{invalid_key, {1.0001}}}}, escaped, with_validation)),
              R"({"m":{"\uFFFD":[1.0]}})");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdEscapedKey{{{invalid_key, {1.1}}}}, escaped, with_validation);
                          }).what()),
              "the value is not equal to the constant 1 at /m/\xEF\xBF\xBD/0; reading an element of the value at the "
              "key \"m\" of "
              "VdEscapedKey");
#endif
}

TEST(Validate, Presence)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    // the defaults of "d", "e" and "w" are left out also under `yyjson::emit_defaults` and are not checked
    const auto e1 = VdDefaults{0, 1, {0, 3}};
    expect_valid(e1);
    expect_valid(VdDefaults{0, 0, {0, 7}});
    EXPECT_NO_THROW(yyjson::validate(e1, emit_defaults));
    EXPECT_EQ(std::string(yyjson::write(e1, emit_defaults, with_validation)), R"({"e":1})");
    EXPECT_EQ(std::string(yyjson::write(VdDefaults{0, 0, {0, 7}}, emit_defaults, with_validation)), "{}");
    // a value other than the default is checked
    expect_invalid(VdDefaults{-1, 1, {0, 3}},
                   R"(the number is less than the minimum 1 at /d; reading the key "d" of VdDefaults)", "/d");
    expect_invalid(VdDefaults{1, -1, {0, 3}},
                   R"(the number is less than the minimum 1 at /e; reading the key "e" of VdDefaults)", "/e");
    expect_invalid(VdDefaults{1, 1, {-1, 3}},
                   R"(the number is less than the minimum 1 at /w/a; reading the key "a" of VdWeight)", "/w/a");

    // `yyjson::emit_defaults` of the call writes and checks the other defaults
    expect_valid(VdCapped{});
    const auto capped = std::string("the object has more properties than the maximum 1; reading VdCapped");
    EXPECT_EQ(validation_error([&] { yyjson::validate(VdCapped{}, emit_defaults); }).what(), capped);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(VdCapped{}, emit_defaults, with_validation); }).what(),
              capped);
    auto out = std::string("previous");
    EXPECT_EQ(validation_error([&] { yyjson::write(VdCapped{}, out, emit_defaults, with_validation); }).what(), capped);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(validation_error([&] {
                  (void)yyjson::write(VdCapped{}, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              capped);
    EXPECT_EQ(validation_error([&] {
                  yyjson::write(VdCapped{}, out, yyjson::WriteFlag::NoFlag, emit_defaults, with_validation);
              }).what(),
              capped);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(yyjson::write(VdCapped{}, emit_defaults)), R"({"a":1,"b":2})");

    // an empty `std::optional`, whose default holds a value, is read back as that value
    expect_valid(VdReadBack{});
    expect_valid(VdReadBack{4, std::nullopt, 5});
    expect_invalid(
        VdReadBack{std::nullopt, {}, 1},
        R"(the value is not valid at /x; validating VdReadBack: the field "x" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/x", read_leg::skipped);
    expect_invalid(
        VdReadBack{3, {}, std::nullopt},
        R"(the value is not valid at /z; validating VdReadBack: the field "z" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/z", read_leg::skipped);
    expect_valid(VdReadBackTransparent{});
    expect_invalid(
        VdReadBackTransparent{VdOptionalPercent{}},
        R"(the value is not valid at /p; validating VdReadBackTransparent: the field "p" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/p", read_leg::skipped);
}

// an error of write itself takes the place of a violation in a checked write
struct VdCircle
{
    int r = 0;
};
struct VdEnvelope
{
    std::string kind = "circle";
    std::variant<VdCircle, int> data;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "num"})), yyjson::field("n", yyjson::minimum(0)));
struct VdReal
{
    double d = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdReal> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0)));

TEST(Validate, Precedence)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    // the error of write comes before the violation
    const auto violation_and_mismatch = VdEnvelope{"num", VdCircle{}, -1};
    EXPECT_THROW(yyjson::validate(violation_and_mismatch), std::invalid_argument);
    const auto expect_write_error = [](const auto& t) {
        try
        {
            (void)yyjson::write(t, with_validation);
            ADD_FAILURE() << "no exception";
        }
        catch (const yyjson::bad_cast&)
        {
            ADD_FAILURE() << "the violation is thrown in the place of the error of write";
        }
        catch (const std::invalid_argument&)
        {
        }
    };
    expect_write_error(violation_and_mismatch);
    const auto mismatch = VdEnvelope{"num", VdCircle{}, 0};
    EXPECT_THROW(yyjson::validate(mismatch), std::invalid_argument);
    expect_write_error(mismatch);

    // a NaN that the write rejects, and one that it writes
    const auto nan = VdReal{std::nan("")};
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(nan); }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    EXPECT_THROW((void)yyjson::write(nan, with_validation), yyjson::write_error);
    auto out = std::string("previous");
    EXPECT_THROW(yyjson::write(nan, out, with_validation), yyjson::write_error);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(nan, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    EXPECT_EQ(std::string(yyjson::write(std::vector{std::nan("")}, WriteFlag::AllowInfAndNan, with_validation)),
              "[NaN]");
    const auto negative = VdReal{-INFINITY};
    EXPECT_THROW((void)yyjson::write(negative, with_validation), yyjson::write_error);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(negative, WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdReal)");
    // a non-finite number written as null is checked as the null that a read gets
    const auto e = validation_error([&] { (void)yyjson::write(nan, WriteFlag::InfAndNanAsNull, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /d; reading double");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_FALSE(e.position().has_value());
    out = "previous";
    EXPECT_THROW(yyjson::write(nan, out, WriteFlag::InfAndNanAsNull, with_validation), yyjson::bad_cast);
    EXPECT_EQ(out, "previous");
    EXPECT_EQ(std::string(yyjson::write(VdReal{1}, WriteFlag::InfAndNanAsNull, with_validation)), R"({"d":1.0})");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(std::vector{std::nan("")}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "expected a number, got null at /0; reading double");
    const auto optional_nan = std::vector<std::optional<double>>{std::nan("")};
    EXPECT_EQ(std::string(yyjson::write(optional_nan, WriteFlag::InfAndNanAsNull, with_validation)), "[null]");
}

// a recursive type, whose depth the check limits as a read does
struct VdTree
{
    int v = 0;
    std::vector<VdTree> kids;
};
template <>
inline constexpr auto yyjson::annotations<VdTree> = yyjson::annotate(yyjson::field("v", yyjson::minimum(0)));
// holds itself through a translation
struct VdLoop
{
    std::shared_ptr<VdLoop> next;
};
struct VdLoopWire
{
    std::vector<VdLoop> next;
    int n = 1;
};
template <>
inline constexpr auto yyjson::annotations<VdLoopWire> = yyjson::annotate(yyjson::field("n", yyjson::minimum(1)));
template <>
struct yyjson::caster<VdLoop>
{
    static VdLoop from_json(const VdLoopWire&) { return {}; }
    static VdLoopWire to_json(const VdLoop& x)
    {
        auto w = VdLoopWire();
        if (x.next) w.next.push_back(*x.next);
        return w;
    }
};

TEST(Validate, DepthLimit)
{
    expect_invalid(VdTree{0, {VdTree{}, VdTree{0, {VdTree{-1, {}}}}}},
                   R"(the number is less than the minimum 0 at /kids/1/kids/0/v; reading the key "v" of VdTree)",
                   "/kids/1/kids/0/v");
    auto tree = VdTree();
    for (auto i = 0; i < 300; ++i) tree = VdTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(e.path(), "");
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    EXPECT_THROW((void)yyjson::write(tree, yyjson::with_validation), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);

    auto loop = std::make_shared<VdLoop>();
    loop->next = loop;
    const auto l = validation_error([&] { yyjson::validate(*loop); });
    EXPECT_TRUE(std::string_view(l.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << l.what();
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    loop->next.reset();
}

// the keywords of a string, of the items of an array and of the keys of a map
struct VdMore
{
    std::string code = "ab";
    std::string word = "é";
    std::vector<int> v = {10, 11};
    std::vector<int> pair = {1, 2};
    std::map<VdColor, int> colors;
    std::shared_ptr<std::string> unit = std::make_shared<std::string>("mm");
};
template <>
inline constexpr auto yyjson::annotations<VdMore> = yyjson::annotate(
    yyjson::field("code", yyjson::pattern("^a")), yyjson::field("word", yyjson::max_length(1)),
    yyjson::field("v", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2), yyjson::max_contains(3)),
    yyjson::field("pair", yyjson::const_(yyjson::json_text("[1, 2]"))),
    yyjson::field("colors", yyjson::property_names(yyjson::enum_("red"))),
    yyjson::field("unit", yyjson::enum_("mm", "cm")));

TEST(Validate, MoreValueKeywords)
{
    expect_valid(VdMore{});
    expect_valid(with<VdMore>([](auto& t) {
        t.colors = {{VdColor::red, 0}};
        t.v = {10, 1, 11, 12};
    }));
    expect_invalid(with<VdMore>([](auto& t) { t.code = "ba"; }),
                   R"(the string does not match the pattern "^a" at /code; reading the key "code" of VdMore)", "/code");
    expect_invalid(with<VdMore>([](auto& t) { t.word = "日本"; }),
                   R"(the string is longer than the maximum length 1 at /word; reading the key "word" of VdMore)",
                   "/word");
    expect_invalid(with<VdMore>([](auto& t) { t.v = {10, 1}; }),
                   R"(the array has fewer than 2 items that satisfy contains at /v; reading the key "v" of VdMore)",
                   "/v");
    expect_invalid(with<VdMore>([](auto& t) { t.v = {10, 11, 12, 13}; }),
                   R"(the array has more than 3 items that satisfy contains at /v; reading the key "v" of VdMore)",
                   "/v");
    expect_invalid(with<VdMore>([](auto& t) { t.pair = {2, 1}; }),
                   R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of VdMore)",
                   "/pair");
    expect_invalid(
        with<VdMore>([](auto& t) { t.colors = {{VdColor::green, 0}}; }),
        R"(the key is not one of "red" at /colors/green; reading a key of the value at the key "colors" of VdMore)",
        "/colors/green");
    expect_invalid(with<VdMore>([](auto& t) { t.unit = std::make_shared<std::string>("km"); }),
                   R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of VdMore)", "/unit");
}

// a non-finite number that the write gives as null is checked as a read under yyjson::reject_null_as_missing
// reads that null at its position
struct VdOptionalReal
{
    std::optional<double> o = 1.0;
};
struct VdNullableReals
{
    std::optional<double> o;
    std::vector<std::optional<double>> v;
    yyjson::nullable<double> n;
};
struct VdUnreadable
{
    std::string_view s = "x";
    double d = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnreadable> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0)));

TEST(Validate, NonFiniteAsNull)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    const auto optional_null =
        "the value must not be null at /o; reading std::optional<double> under yyjson::reject_null_as_missing";
    auto e = validation_error([&] { (void)yyjson::write(VdOptionalReal{nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    EXPECT_EQ(e.path(), "/o");
    e = validation_error([&] { (void)yyjson::write(VdNullableReals{nan, {}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), optional_null);
    const auto nulls = VdNullableReals{std::nullopt, {nan, 1}, nan};
    EXPECT_EQ(std::string(yyjson::write(nulls, as_null, with_validation)), R"({"v":[null,1.0],"n":null})");

    static_assert(!yyjson::readable<VdUnreadable>);
    e = validation_error([&] { (void)yyjson::write(VdUnreadable{"x", nan}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /d; reading double");
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(VdUnreadable{"x", nan}); }).what()),
              R"(the number is less than the minimum 0 at /d; reading the key "d" of VdUnreadable)");
}

// a field that the write rejects alone is still checked against its keywords
struct VdUnwritable
{
    std::vector<double> v;
    std::string s;
};
template <>
inline constexpr auto yyjson::annotations<VdUnwritable> =
    yyjson::annotate(yyjson::field("v", yyjson::max_items(1)), yyjson::field("s", yyjson::max_length(1)));

TEST(Validate, FieldThatWriteRejects)
{
    using yyjson::with_validation;
    const auto many = VdUnwritable{{1, std::nan("")}, ""};
    auto e = validation_error([&] { yyjson::validate(many); });
    EXPECT_EQ(std::string(e.what()),
              R"(the array has more items than the maximum 1 at /v; reading the key "v" of VdUnwritable)");
    EXPECT_EQ(e.path(), "/v");
    EXPECT_THROW((void)yyjson::write(many, with_validation), yyjson::write_error);
    const auto invalid = VdUnwritable{{}, "\xff\xfe"};
    e = validation_error([&] { yyjson::validate(invalid); });
    EXPECT_EQ(std::string(e.what()),
              R"(the string is longer than the maximum length 1 at /s; reading the key "s" of VdUnwritable)");
    EXPECT_THROW((void)yyjson::write(invalid, with_validation), yyjson::write_error);
    EXPECT_NO_THROW(yyjson::validate(VdUnwritable{{std::nan("")}, "\xff"}));
}

// an empty std::optional under transparent structs in a chain
struct VdChainIn
{
    std::optional<int> v = 3;
};
template <>
inline constexpr auto yyjson::annotations<VdChainIn> = yyjson::annotate(yyjson::transparent);
struct VdChainOut
{
    VdChainIn in;
};
template <>
inline constexpr auto yyjson::annotations<VdChainOut> = yyjson::annotate(yyjson::transparent);
struct VdChainTop
{
    VdChainOut out;
};

TEST(Validate, TransparentChain)
{
    expect_valid(VdChainTop{});
    expect_invalid(
        VdChainTop{VdChainOut{VdChainIn{std::nullopt}}},
        R"(the value is not valid at /out; validating VdChainTop: the field "out" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/out", read_leg::skipped);
}

// an enumerator without a name is an error of write, which the check reports as write does
struct VdNamedColor
{
    VdColor c = VdColor::red;
};
template <>
inline constexpr auto yyjson::annotations<VdNamedColor> = yyjson::annotate(yyjson::field("c", yyjson::enum_("red")));

TEST(Validate, UnnamedEnumerator)
{
    const auto unnamed = VdNamedColor{static_cast<VdColor>(7)};
    const auto write_error = [](auto&& f) {
        try
        {
            f();
        }
        catch (const yyjson::write_error& e)
        {
            EXPECT_EQ(e.code(), YYJSON_WRITE_ERROR_INVALID_VALUE_TYPE);
            return std::string(e.what());
        }
        ADD_FAILURE() << "no yyjson::write_error";
        return std::string();
    };
    const auto plain = write_error([&] { (void)yyjson::write(unnamed); });
    EXPECT_EQ(write_error([&] { (void)yyjson::write(unnamed, yyjson::with_validation); }), plain);
    EXPECT_EQ(write_error([&] { yyjson::validate(unnamed); }), plain);
    EXPECT_EQ(write_error([&] { yyjson::validate(static_cast<VdColor>(7)); }),
              write_error([&] { (void)yyjson::write(static_cast<VdColor>(7)); }));
    using unnamed_keys = std::map<VdColor, int>;
    const auto keys = unnamed_keys{{static_cast<VdColor>(7), 1}};
    EXPECT_EQ(write_error([&] { yyjson::validate(keys); }), write_error([&] { (void)yyjson::write(keys); }));
    EXPECT_NO_THROW(yyjson::validate(VdNamedColor{}));
    EXPECT_NO_THROW(yyjson::validate(unnamed_keys{{VdColor::red, 1}}));
}

// the keywords of each kind let the types be checked and written
struct VdClosed
{
    int a = 0;
    std::optional<int> b = 1;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdClosed> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::max_properties(3), yyjson::dependent_required("a", {"b"}));
struct VdMatches
{
    yyjson::value v;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdMatches> =
    yyjson::annotate(yyjson::field("v", yyjson::contains(yyjson::matches<VdInner>)));

struct VdClosedStruct
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedStruct> = yyjson::annotate(yyjson::additional_properties(false));
struct VdPatterned
{
    int a = 0;
    std::map<std::string, int> xs = {{"x1", 1}};
};
template <>
inline constexpr auto yyjson::annotations<VdPatterned> =
    yyjson::annotate(yyjson::field("xs", yyjson::pattern_properties("^x")));
struct VdShortKeys
{
    int a = 0;
    int bb = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdShortKeys> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
struct VdSquare
{
    int side = 1;
};
using VdShape = std::variant<VdCircle, VdSquare>;
template <>
inline constexpr auto yyjson::annotations<VdShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square"}));
using VdOneOf = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdAnyOf = yyjson::any_of_variant<int, std::string>;
struct VdPoint
{
    int x = 0;
    int y = 0;
};
template <>
struct yyjson::caster<VdPoint>
{
    static VdPoint from_json(std::array<int, 2> a) { return {a[0], a[1]}; }
    template <typename... Ts>
    static std::array<int, 2> to_json(const VdPoint& p, Ts...)
    {
        return {p.x, p.y};
    }
};
struct VdPair
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPair> = yyjson::annotate(yyjson::as_array, yyjson::min_items(2));
struct VdKinds
{
    VdShape shape = VdSquare{};
    VdOneOf one;
    VdAnyOf any = std::string("x");
    std::vector<VdPoint> points = {{1, 2}};
    std::multimap<int, int> pairs = {{1, 2}, {1, 3}};
    VdPair row;
};
template <>
inline constexpr auto yyjson::annotations<VdKinds> =
    yyjson::annotate(yyjson::field("points", yyjson::max_items(3)), yyjson::field("pairs", yyjson::max_items(2)));

TEST(Validate, OtherKeywordsCompile)
{
    EXPECT_NO_THROW(yyjson::validate(VdClosed{}));
    EXPECT_EQ(std::string(yyjson::write(VdClosed{}, yyjson::with_validation)), std::string(yyjson::write(VdClosed{})));
    EXPECT_NO_THROW(yyjson::validate(VdMatches{yyjson::read(R"([{"n":1}])"), 1}));
    EXPECT_NO_THROW(yyjson::validate(VdEnvelope{}));
    expect_valid(VdClosedStruct{});
    expect_valid(VdPatterned{});
    expect_valid(VdShortKeys{});
    expect_valid(VdKinds{});
}

// the object that the write gives for a struct, with the keys of its maps and of the tag that holds it
struct VdPatternOnly
{
    int a = 0;
    std::map<std::string, int> ps;
};
template <>
inline constexpr auto yyjson::annotations<VdPatternOnly> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("ps", yyjson::pattern_properties("^p")));
struct VdTagCircle
{
    int r = 0;
    std::optional<int> label;
};
template <>
inline constexpr auto yyjson::annotations<VdTagCircle> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(2));
struct VdTagSquare
{
    int side = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagSquare> = yyjson::annotate(yyjson::min_properties(2));
struct VdTagLine
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagLine> =
    yyjson::annotate(yyjson::property_names(yyjson::pattern("^[a-n]")));
struct VdTagNamed
{
    std::string type = "named";
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagNamed> = yyjson::annotate(yyjson::additional_properties(false));
using VdTagShape = std::variant<VdTagCircle, VdTagSquare, VdTagLine, VdTagNamed>;
template <>
inline constexpr auto yyjson::annotations<VdTagShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square", "line", "named"}));
struct VdTagHolder
{
    std::vector<VdTagShape> shapes;
};
struct VdShortNames
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdShortNames> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::field("rest", yyjson::extra_keys));
struct VdLongField
{
    int abc = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdLongField> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));

// the number of members, with the defaults that the write leaves out
struct VdCount
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCount> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::default_, yyjson::field("rest", yyjson::extra_keys));
struct VdCountEmit
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountEmit> = yyjson::annotate(
    yyjson::max_properties(2), yyjson::default_, yyjson::field("a", yyjson::default_, yyjson::emit_defaults),
    yyjson::field("rest", yyjson::extra_keys));
struct VdCountMin
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountMin> =
    yyjson::annotate(yyjson::min_properties(3), yyjson::field("rest", yyjson::extra_keys));

// the presence rules and the conditions, on the keys that the write gives
struct VdPresence
{
    std::optional<int> a;
    std::optional<int> b;
    int v = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPresence> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}), yyjson::property("v", yyjson::minimum(0)));
struct VdDroppedDefault
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdDroppedDefault> =
    yyjson::annotate(yyjson::field("a", yyjson::default_), yyjson::dependent_required("b", {"a"}));
struct VdConditions
{
    int kind = 0;
    std::optional<int> x;
    std::optional<int> y;
    std::optional<int> z;
};
template <>
inline constexpr auto yyjson::annotations<VdConditions> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::property("kind", yyjson::const_(1))), yyjson::then(yyjson::required("x")),
                         yyjson::else_(yyjson::not_(yyjson::required("x")))),
    yyjson::one_of(yyjson::required("y"), yyjson::required("z")),
    yyjson::any_of(yyjson::max_properties(2), yyjson::required("z")),
    yyjson::all_of(yyjson::property("kind", yyjson::maximum(1))));

// object keywords at a field of struct type and in a condition of the struct
struct VdInnerObj
{
    int a = 0;
    std::optional<int> b;
};
struct VdOuterObj
{
    VdInnerObj in;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<VdOuterObj> =
    yyjson::annotate(yyjson::field("in", yyjson::max_properties(1)), yyjson::all_of(yyjson::max_properties(1)));

// the values of the maps of the keys
struct VdKeyMaps
{
    std::map<std::string, int> ab;
    std::map<std::string, int> a;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdKeyMaps> = yyjson::annotate(
    yyjson::field("ab", yyjson::pattern_properties("^ab"), yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("a", yyjson::pattern_properties("^a"), yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(1))));

// a non-finite number written as null beside a struct
struct VdNanInner
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdNanInner> = yyjson::annotate(yyjson::max_properties(1));
struct VdNanBeside
{
    std::tuple<double, VdNanInner> t;
    std::vector<std::pair<VdNanInner, yyjson::nullable<double>>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNanBeside> = yyjson::annotate(yyjson::field("v", yyjson::max_items(1)));

TEST(Validate, ObjectKeys)
{
    expect_valid(VdPatternOnly{0, {{"p1", 1}}});
    expect_invalid(VdPatternOnly{0, {{"q", 1}}},
                   "the object has a key that is not allowed at /q; reading VdPatternOnly, which "
                   "yyjson::additional_properties(false) closes",
                   "/q");
    // the key of the internal tag is a key of the object of the alternative
    expect_valid(VdTagShape{VdTagCircle{1, std::nullopt}});
    expect_valid(VdTagShape{VdTagSquare{}});
    expect_valid(VdTagShape{VdTagNamed{}});
    expect_invalid(VdTagShape{VdTagLine{}},
                   R"(the key does not match the pattern "^[a-n]" at /type; reading a key of VdTagLine)", "/type");
    expect_invalid(VdTagHolder{{VdTagCircle{}, VdTagCircle{1, 2}}},
                   "the object has more properties than the maximum 2 at /shapes/1; reading VdTagCircle", "/shapes/1");
    expect_valid(VdShortNames{0, {{"xy", 1}}});
    expect_invalid(VdShortNames{0, {{"xyz", 1}}},
                   "the key is longer than the maximum length 2 at /xyz; reading a key of VdShortNames", "/xyz");
    expect_invalid(VdLongField{}, "the key is longer than the maximum length 2 at /abc; reading a key of VdLongField",
                   "/abc");
}

TEST(Validate, ObjectCounts)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    expect_valid(VdCount{});
    expect_valid(VdCount{0, 0, {{"x", 1}}});
    expect_invalid(VdCount{1, 1, {{"x", 1}}}, "the object has more properties than the maximum 2; reading VdCount", "");
    // the defaults that `yyjson::emit_defaults` writes are counted
    const auto message = std::string("the object has more properties than the maximum 2; reading VdCount");
    const auto defaults = VdCount{0, 0, {{"x", 1}}};
    EXPECT_EQ(validation_error([&] { yyjson::validate(defaults, emit_defaults); }).what(), message);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(defaults, emit_defaults, with_validation); }).what(), message);
    EXPECT_THROW((void)yyjson::read<VdCount>(std::string(yyjson::write(defaults, emit_defaults))), yyjson::bad_cast);
    expect_valid(VdCountEmit{0, 0, {{"x", 1}}});
    expect_invalid(VdCountEmit{0, 0, {{"x", 1}, {"y", 1}}},
                   "the object has more properties than the maximum 2; reading VdCountEmit", "");
    expect_valid(VdCountMin{0, {{"x", 1}, {"y", 2}}});
    expect_invalid(VdCountMin{0, {{"x", 1}}}, "the object has fewer properties than the minimum 3; reading VdCountMin",
                   "");
    // at a field of struct type, and in a condition of the struct
    expect_valid(VdOuterObj{});
    expect_invalid(VdOuterObj{VdInnerObj{0, 1}, std::nullopt},
                   R"(the object has more properties than the maximum 1 at /in; reading the key "in" of VdOuterObj)",
                   "/in");
    expect_invalid(VdOuterObj{{}, 1},
                   "the value does not satisfy all the subschemas of allOf; reading VdOuterObj in yyjson::all_of", "");
}

TEST(Validate, Conditions)
{
    expect_valid(VdPresence{1, 1, 0});
    expect_valid(VdPresence{});
    expect_invalid(
        VdPresence{1, std::nullopt, 0},
        R"(the object does not have all the keys that dependentRequired requires; reading the key "b" of VdPresence, which yyjson::dependent_required requires)",
        "");
    expect_invalid(VdPresence{std::nullopt, std::nullopt, -1},
                   R"(the number is less than the minimum 0 at /v; reading the key "v" of VdPresence)", "/v");
    // a default that the write leaves out is a missing key
    expect_valid(VdDroppedDefault{1, 1});
    expect_invalid(
        VdDroppedDefault{0, 1},
        R"(the object does not have all the keys that dependentRequired requires; reading the key "a" of VdDroppedDefault, which yyjson::dependent_required requires)",
        "");
    EXPECT_NO_THROW(yyjson::validate(VdDroppedDefault{0, 1}, yyjson::emit_defaults));

    expect_valid(VdConditions{0, std::nullopt, 1, std::nullopt});
    expect_valid(VdConditions{1, 1, std::nullopt, 1});
    expect_invalid(
        VdConditions{1, std::nullopt, 1, std::nullopt},
        R"(a required key is missing; reading the key "x" of VdConditions, which yyjson::if_then_else requires)", "");
    expect_invalid(VdConditions{0, 1, 1, std::nullopt},
                   "the value satisfies neither if nor else; reading VdConditions in yyjson::not_", "");
    expect_invalid(
        VdConditions{0, std::nullopt, 1, 1},
        R"(the value satisfies more than one subschema of oneOf; reading VdConditions with all the keys of more than one of ["y"], ["z"])",
        "");
    expect_invalid(
        VdConditions{0, std::nullopt, std::nullopt, std::nullopt},
        R"(the value satisfies none of the subschemas of oneOf; reading VdConditions without all the keys of one of ["y"], ["z"])",
        "");
    expect_invalid(VdConditions{1, 1, 1, std::nullopt},
                   "the value satisfies none of the subschemas of anyOf; reading VdConditions", "");
    expect_invalid(
        VdConditions{2, std::nullopt, 1, std::nullopt},
        R"(the value does not satisfy all the subschemas of allOf at /kind; reading the key "kind" of VdConditions)",
        "/kind");
}

TEST(Validate, KeyMaps)
{
    expect_valid(VdKeyMaps{{{"ab", 1}}, {{"a", 0}}, {{"x", 1}}});
    // a key is checked by each pattern that matches it
    expect_invalid(VdKeyMaps{{{"ab", -1}}, {}, {}},
                   "the number is less than the minimum 0 at /ab; reading a member value of VdKeyMaps", "/ab");
    expect_invalid(VdKeyMaps{{{"ab", 6}}, {}, {}},
                   "the number is greater than the maximum 5 at /ab; reading a member value of VdKeyMaps", "/ab");
    expect_invalid(VdKeyMaps{{}, {}, {{"x", 0}}},
                   "the number is less than the minimum 1 at /x; reading a member value of VdKeyMaps", "/x");
    // a key that the read stores in another field
    expect_invalid(
        VdKeyMaps{{}, {{"abc", 1}}, {}},
        R"(the value is not valid at /abc; validating VdKeyMaps: a read gives this key of the field "a" to the field "ab")",
        "/abc", read_leg::skipped);
    expect_invalid(
        VdKeyMaps{{}, {}, {{"ax", 1}}},
        R"(the value is not valid at /ax; validating VdKeyMaps: a read gives this key of the field "rest" to the field "a")",
        "/ax", read_leg::skipped);
    expect_invalid(
        VdKeyMaps{{{"q", 1}}, {}, {}},
        R"(the value is not valid at /q; validating VdKeyMaps: a read gives this key of the field "ab" to the field "rest")",
        "/q", read_leg::skipped);
}

template <typename T>
void expect_write_error(const T& t, std::string_view message)
{
    const auto expect_message = [&](auto&& f) {
        try
        {
            f();
            ADD_FAILURE() << "no exception";
        }
        catch (const std::invalid_argument& e)
        {
            EXPECT_EQ(std::string_view(e.what()), message);
        }
    };
    expect_message([&] { yyjson::validate(t); });
    expect_message([&] { (void)yyjson::write(t); });
    expect_message([&] { (void)yyjson::write(t, yyjson::with_validation); });
}

TEST(Validate, InternalTagMismatch)
{
    expect_write_error(VdTagShape{VdTagNamed{"other", 0}}, R"(the alternative "named" of )" +
                                                               std::string(yyjson::detail::type_name<VdTagShape>()) +
                                                               R"( is written with another "type")");
}

using VdTagScalar = std::variant<VdTagCircle, int>;
template <>
inline constexpr auto yyjson::annotations<VdTagScalar> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "int"}));
using VdTagExtra = std::variant<VdShortNames, VdTagCircle>;
template <>
inline constexpr auto yyjson::annotations<VdTagExtra> =
    yyjson::annotate(yyjson::internal_tag("type", {"extra", "circle"}));
struct VdTagScalarHolder
{
    VdTagScalar s;
};

TEST(Validate, WriteErrors)
{
    const auto scalar = R"(the alternative "int" of )" + std::string(yyjson::detail::type_name<VdTagScalar>()) +
                        ", which yyjson::internal_tag tags, is not written as a JSON object";
    expect_write_error(VdTagScalar{1}, scalar);
    expect_write_error(VdTagScalarHolder{VdTagScalar{1}}, scalar);
    expect_valid(VdTagScalar{VdTagCircle{}});
    expect_write_error(VdTagExtra{VdShortNames{0, {{"type", 1}}}},
                       R"(the field of yyjson::extra_keys of VdShortNames has the key "type", which VdShortNames )"
                       "writes for a field or a tag");
    expect_write_error(VdShortNames{0, {{"a", 1}}},
                       R"(the field of yyjson::extra_keys of VdShortNames has the key "a", which VdShortNames )"
                       "writes for a field or a tag");
}

TEST(Validate, NonFiniteBesideStruct)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    expect_valid(VdNanBeside{});
    auto e = validation_error([&] { (void)yyjson::write(VdNanBeside{{nan, {}}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /t/0; reading double");
    EXPECT_EQ(e.path(), "/t/0");
    const auto nullable = VdNanBeside{{0, {}}, {{{}, nan}}};
    EXPECT_EQ(std::string(yyjson::write(nullable, as_null, with_validation)),
              R"({"t":[0.0,{"a":0}],"v":[[{"a":0},null]]})");
    // the struct beside the null is still checked
    e = validation_error([&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{0, 1}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "the object has more properties than the maximum 1 at /v/0/0; reading VdNanInner");
    e = validation_error(
        [&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{}, nan}, {{}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()),
              R"(the array has more items than the maximum 1 at /v; reading the key "v" of VdNanBeside)");
}

// a non-finite number in a map of keys, written as null
struct VdNanExtra
{
    int x = 0;
    std::map<std::string, double> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdNanExtra> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct VdNanPattern
{
    int x = 0;
    std::map<std::string, double> p;
};
template <>
inline constexpr auto yyjson::annotations<VdNanPattern> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^k")));

// a keyword of a field that holds null in the place of a NaN
struct VdNotInner
{
    int a = 0;
};
struct VdNotNull
{
    std::vector<std::variant<std::optional<double>, VdNotInner>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNotNull> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::not_(yyjson::minimum(0)))));

TEST(Validate, NonFiniteInKeyMapsAndFields)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    auto e = validation_error([&] { (void)yyjson::write(VdNanExtra{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /k; reading double");
    e = validation_error([&] { (void)yyjson::write(VdNanPattern{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /k; reading double");
    EXPECT_EQ(std::string(yyjson::write(VdNanPattern{0, {{"k", 1.5}}}, as_null, with_validation)),
              R"({"x":0,"k":1.5})");

    // read checks the null, which `yyjson::not_` accepts, and not the NaN
    const auto not_null = VdNotNull{{std::optional<double>(nan)}};
    e = validation_error([&] { (void)yyjson::write(not_null, as_null, with_validation); });
    EXPECT_EQ(
        std::string(e.what()),
        R"(the value satisfies the subschema of not at /v/0; reading an element of the value at the key "v" of VdNotNull)");
    EXPECT_THROW((void)yyjson::read<VdNotNull>(std::string(yyjson::write(not_null, as_null))), yyjson::bad_cast);
}

// a struct with object keywords inside another, checked on the JSON of the outer
struct VdLink
{
    int v = 0;
    std::optional<int> w;
    std::unique_ptr<VdLink> next;
};
template <>
inline constexpr auto yyjson::annotations<VdLink> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, NestedObjectKeywords)
{
    const auto chain = [](int depth, std::optional<int> w, int v) {
        auto head = std::unique_ptr<VdLink>();
        for (auto i = 0; i < depth; ++i)
            head = std::make_unique<VdLink>(VdLink{i == 0 ? v : 0, i == 0 ? w : std::nullopt, std::move(head)});
        return head;
    };
    const auto valid = VdLink{0, std::nullopt, chain(3, std::nullopt, 0)};
    EXPECT_NO_THROW(yyjson::validate(valid));
    const auto counted = VdLink{0, std::nullopt, chain(3, 1, 0)};
    auto e = validation_error([&] { yyjson::validate(counted); });
    EXPECT_EQ(std::string(e.what()),
              "the object has more properties than the maximum 2 at /next/next/next; reading VdLink");
    e = validation_error([&] { (void)yyjson::write(counted, yyjson::with_validation); });
    EXPECT_EQ(e.path(), "/next/next/next");
    const auto negative = VdLink{0, std::nullopt, chain(2, std::nullopt, -1)};
    e = validation_error([&] { yyjson::validate(negative); });
    EXPECT_EQ(std::string(e.what()),
              R"(the number is less than the minimum 0 at /next/next/v; reading the key "v" of VdLink)");
}

// a key of a pattern map that read stores in no field
struct VdUnmatched
{
    int x = 0;
    std::map<std::string, int> p;
};
template <>
inline constexpr auto yyjson::annotations<VdUnmatched> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^p")));

TEST(Validate, KeyReadIntoNoField)
{
    expect_valid(VdUnmatched{0, {{"p1", 1}}});
    expect_invalid(
        VdUnmatched{0, {{"q", 1}}},
        R"(the value is not valid at /q; validating VdUnmatched: a read gives this key of the field "p" to no field)",
        "/q", read_leg::skipped);
}

// the keys that read needs and that write leaves out
struct VdOmitZero
{
    static int to_json(int v) { return v; }
    static int from_json(int v) { return v; }
    static bool should_omit(int v) { return v == 0; }
};
struct VdRequiredOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOmitted> = yyjson::annotate(yyjson::field("a", yyjson::required));
struct VdMissingOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdMissingOmitted> = yyjson::annotate(yyjson::reject_missing_keys);

// a check of the keys of a struct without object keywords allocates no memory
struct VdUnknownOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnknownOnly> = yyjson::annotate(yyjson::reject_unknown_keys);
struct VdRequiredOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOnly> = yyjson::annotate(yyjson::field("a", yyjson::required));

namespace
{
    thread_local std::size_t vd_allocations = 0;

    template <typename T>
    std::size_t validate_allocations(const T& t)
    {
        const auto before = vd_allocations;
        yyjson::validate(t);
        return vd_allocations - before;
    }
}  // namespace

void* operator new(std::size_t size)
{
    ++vd_allocations;
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    ++vd_allocations;
    return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }

TEST(Validate, MissingKeys)
{
    expect_valid(VdRequiredOmitted{1});
    expect_invalid(VdRequiredOmitted{0}, R"(a required key is missing; reading the key "a" of VdRequiredOmitted)", "");
    expect_valid(VdMissingOmitted{1, 0});
    expect_invalid(VdMissingOmitted{0, 1}, R"(a required key is missing; reading the key "a" of VdMissingOmitted)", "");
    EXPECT_EQ(validate_allocations(VdUnknownOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOmitted{1}), 0U);
}

// a key that write gives twice in one object, which a strict read rejects and a plain read takes
namespace
{
    // `Object` is the type of the object in `t` that gives `key` twice.
    template <typename T>
    void expect_repeated_key(const T& t, const expected_text& message, std::string_view path)
    {
        expect_invalid(t, message, path, read_leg::skipped);
        if constexpr (yyjson::readable<T>)
        {
            const auto json = std::string(yyjson::write(t));
            EXPECT_NO_THROW((void)yyjson::read<T>(json));
            const auto r = validation_error([&] { (void)yyjson::read<T>(json, yyjson::strict); });
            EXPECT_EQ(what_without_position(r), message.text) << json;
            EXPECT_EQ(r.path(), path) << json;
        }
    }
}  // namespace

enum class VdSide
{
    left,
    right
};
// a key whose `<=>` compares more than its text
struct VdTagged
{
    std::string s;
    int n = 0;
    VdTagged() = default;
    VdTagged(std::string_view v, int k = 0) : s(v), n(k) {}
    operator std::string_view() const { return s; }
    auto operator<=>(const VdTagged&) const = default;
};
struct VdMulti
{
    std::multimap<std::string, int> m;
};

TEST(Validate, RepeatedKeys)
{
    using pairs = std::vector<std::pair<std::string, int>>;
    static_assert(yyjson::detail::needs_validation<std::multimap<std::string, int>>);
    static_assert(yyjson::detail::needs_validation<VdMulti>);
    expect_valid(std::multimap<std::string, int>{{"a", 1}, {"b", 1}});
    expect_repeated_key(std::multimap<std::string, int>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::multimap<std::string, int>>()),
                        "/a");
    expect_repeated_key(std::unordered_multimap<std::string, int>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::unordered_multimap<std::string, int>>()),
                        "/a");
    expect_repeated_key(pairs{{"a", 1}, {"b", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::list<std::pair<std::string, int>>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::list<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::deque<std::pair<std::string, int>>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::deque<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::vector<std::pair<VdSide, int>>{{VdSide::left, 1}, {VdSide::left, 2}},
                        std::format("the object has a duplicate key at /left; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<VdSide, int>>>()),
                        "/left");
    using keyed = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    expect_repeated_key(
        keyed{{"a", 1}, {"a", 2}},
        std::format("the object has a duplicate key at /a; reading {}",
                    yyjson::detail::type_name<std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>>()),
        "/a");
    static_assert(!yyjson::detail::needs_validation<std::tuple<std::pair<std::string, int>>>);
    const auto first = std::string("k"), second = std::string("k");
    expect_repeated_key(std::map<const char*, int>{{first.c_str(), 1}, {second.c_str(), 2}},
                        std::format("the object has a duplicate key at /k; reading {}",
                                    yyjson::detail::type_name<std::map<const char*, int>>()),
                        "/k");
    // keys that compare apart in C++ but are written as one text
    expect_repeated_key(std::map<VdTagged, int>{{VdTagged("a", 0), 1}, {VdTagged("a", 1), 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::map<VdTagged, int>>()),
                        "/a");
    // a null key has no JSON text, so write throws for it and the check of the keys skips it
    using c_pairs = std::vector<std::pair<const char*, int>>;
    EXPECT_NO_THROW(yyjson::validate(c_pairs{{nullptr, 1}, {nullptr, 2}, {"", 3}}));
    EXPECT_NO_THROW(yyjson::validate(
        std::tuple<std::pair<const char*, int>, std::pair<const char*, int>>{{nullptr, 1}, {nullptr, 2}}));

    // more keys than the pairwise comparison takes
    auto many = pairs();
    for (int i = 0; i < 20; ++i) many.emplace_back(std::to_string(i), i);
    expect_valid(many);
    many.emplace_back("7", 0);
    many.emplace_back("3", 0);
    expect_repeated_key(many,
                        std::format("the object has a duplicate key at /7; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/7");
    auto nulls = c_pairs(20, {nullptr, 0});
    EXPECT_NO_THROW(yyjson::validate(nulls));
    nulls.emplace_back("z", 1);
    nulls.emplace_back("z", 2);
    EXPECT_EQ(validation_error([&] { yyjson::validate(nulls); }).path(), "/z");
    auto sides = std::vector<std::pair<VdSide, int>>();
    for (int i = 0; i < 20; ++i) sides.emplace_back(i % 2 == 0 ? VdSide::right : VdSide::left, i);
    expect_repeated_key(sides,
                        std::format("the object has a duplicate key at /right; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<VdSide, int>>>()),
                        "/right");

    expect_repeated_key(VdMulti{{{"a", 1}, {"a", 2}}},
                        std::format("the object has a duplicate key at /m/a; reading {}",
                                    yyjson::detail::type_name<std::multimap<std::string, int>>()),
                        "/m/a");
    expect_repeated_key(std::vector<pairs>{{{"a", 1}}, {{"a", 1}, {"a", 2}}},
                        std::format("the object has a duplicate key at /1/a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/1/a");
    expect_repeated_key(std::map<std::string, pairs>{{"k", {{"a", 1}, {"a", 2}}}},
                        std::format("the object has a duplicate key at /k/a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/k/a");

    // the key is reported before the values, as a strict read does; a plain read reports the value
    using rows = std::multimap<std::string, VdRow>;
    const auto bad_rows = rows{{"a", VdRow{-1, {}}}, {"a", VdRow{1, {}}}};
    expect_invalid(bad_rows,
                   std::format("the object has a duplicate key at /a; reading {}",
                               yyjson::detail::type_name<std::multimap<std::string, VdRow>>()),
                   "/a", read_leg::skipped);
    EXPECT_EQ(validation_error([&] { (void)yyjson::read<rows>(yyjson::write(bad_rows), yyjson::strict); }).path(),
              "/a");

    static_assert(!yyjson::detail::needs_validation<std::map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::unordered_map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::multimap<int, int>>);
    expect_valid(std::multimap<int, int>{{1, 1}, {1, 2}});
    auto map = std::map<std::string, int>();
    for (int i = 0; i < 100; ++i) map.emplace(std::to_string(i), i);
    EXPECT_EQ(validate_allocations(map), 0U);
    EXPECT_EQ(validate_allocations(std::multimap<std::string, int>{{"a", 1}, {"b", 2}}), 0U);
}

// a union: the settings of the alternative that it holds, the count of a one_of, and the read of its JSON when the
// alternative fails, which passes if another alternative reads it
struct VdUnionA
{
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionA> = yyjson::annotate(yyjson::field("x", yyjson::minimum(0)));
struct VdUnionB
{
    int x = 0;
};
struct VdUnionC
{
    int x = 0;
    std::optional<int> y;
};
struct VdUnionD
{
    int y = 0;
};
struct VdUnionE
{
    std::string msg;
};
using VdLowOrInt = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, int>;
using VdLowOrHigh =
    yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdLowOrShort =
    yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<std::string, yyjson::max_length(3)>>;
using VdItemsOrText =
    yyjson::any_of_variant<yyjson::alt<std::vector<int>, yyjson::items(yyjson::minimum(0))>, std::string>;
using VdSignedOrUnsigned = yyjson::one_of_variant<std::int64_t, std::uint64_t>;
// some alternatives other than the last one may read its values and some may not
using VdPartlyOverlapping =
    yyjson::one_of_variant<yyjson::alt<double, yyjson::maximum(0.5)>, std::string,
                           yyjson::alt<std::int64_t, yyjson::minimum(-100), yyjson::maximum(10)>, std::uint64_t>;
using VdAB = std::variant<VdUnionA, VdUnionB>;
using VdAnyAB = yyjson::any_of_variant<VdUnionA, VdUnionB>;
using VdOneCD = yyjson::one_of_variant<VdUnionC, VdUnionD>;
using VdOneAD = yyjson::one_of_variant<VdUnionA, VdUnionD>;
struct VdUnionFields
{
    VdOneCD u = VdUnionD{};
    VdLowOrShort n = 0;
};
struct VdCounted
{
    int a = 0;
    int b = 0;
};
using VdCountedOrMap =
    yyjson::any_of_variant<yyjson::alt<VdCounted, yyjson::max_properties(1)>, std::map<std::string, int>>;
struct VdDefaulted
{
    int a = 0;
    int b = 5;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaulted> = yyjson::annotate(yyjson::field("b", yyjson::default_));
using VdDefaultedOrInt = yyjson::any_of_variant<yyjson::alt<VdDefaulted, yyjson::max_properties(1)>, int>;
struct VdDisjointField
{
    int a = 0;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdDisjointField> = yyjson::annotate(yyjson::field("a", yyjson::minimum(0)));

namespace
{
    template <typename V>
    std::string vd_name()
    {
        return std::string(yyjson::detail::variant_name<V>());
    }
}  // namespace

TEST(Validate, Unions)
{
    static_assert(yyjson::detail::needs_validation<VdLowOrInt>);
    static_assert(yyjson::detail::needs_validation<VdOneCD>);
    static_assert(!yyjson::detail::needs_validation<yyjson::any_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<yyjson::one_of_variant<int, std::string>>);
    static_assert(!yyjson::detail::needs_validation<std::variant<VdUnionB, int>>);

    // the settings of the alternative, where another alternative may read the JSON
    expect_valid(VdLowOrInt(std::in_place_index<0>, 5));
    expect_valid(VdLowOrInt(std::in_place_index<0>, -5));
    expect_valid(VdLowOrHigh(std::in_place_index<0>, 3));
    expect_valid(VdLowOrHigh(std::in_place_index<1>, -20));
    expect_invalid(
        VdLowOrHigh(std::in_place_index<0>, -5),
        "no alternative accepts the value; reading yyjson::one_of_variant<int, int>: alternative 0 (int): the number "
        "is less than the minimum 0; alternative 1 (int): the number is greater than the maximum -10",
        "");
    expect_valid(VdLowOrShort(std::string("abc")));
    expect_invalid(VdLowOrShort(-1),
                   std::format("no alternative accepts the value; reading {}: alternative 0 (int): the number is less "
                               "than the minimum 0; alternative 1 ({}): expected a string, got a number",
                               yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                               yyjson::detail::type_name<std::string>()),
                   "");
    expect_invalid(
        VdLowOrShort(std::string("abcd")),
        std::format("no alternative accepts the value; reading {}: alternative 0 (int): expected an integer, got a "
                    "string; alternative 1 ({}): the string is longer than the maximum length 3",
                    yyjson::detail::variant_name<yyjson::any_of_variant<int, std::string>>(),
                    yyjson::detail::type_name<std::string>()),
        "");
    expect_invalid(
        VdItemsOrText(std::vector<int>{1, -2}),
        std::format("no alternative accepts the value; reading {}: alternative 0 (std::vector<int>): the array has an "
                    "item that does not satisfy items; alternative 1 ({}): expected a string, got an array",
                    yyjson::detail::variant_name<yyjson::any_of_variant<std::vector<int>, std::string>>(),
                    yyjson::detail::type_name<std::string>()),
        "");
    // a keyword that counts the JSON of the alternative
    expect_valid(VdCountedOrMap(VdCounted{1, 2}));
    expect_valid(VdDefaultedOrInt(VdDefaulted{1, 5}));
    const auto defaulted = "no alternative accepts the value; reading " + vd_name<VdDefaultedOrInt>() +
                           ": alternative 0 (VdDefaulted): the object has more properties than the maximum 1; "
                           "alternative 1 (int): expected an integer, got an object";
    EXPECT_EQ(std::string(validation_error([] {
                              yyjson::validate(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults);
                          }).what()),
              defaulted);
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdDefaultedOrInt(VdDefaulted{1, 5}), yyjson::emit_defaults,
                                                  yyjson::with_validation);
                          }).what()),
              defaulted);

    // a value whose alternative fails passes if another reads its JSON
    expect_valid(VdAB(VdUnionA{-1}));
    expect_valid(VdAnyAB(VdUnionA{-1}));
    // another alternative that reads the JSON only without its required keys
    expect_invalid(
        std::variant<VdUnionA, VdUnionE>(VdUnionA{-1}),
        R"(no alternative accepts the value; reading std::variant<VdUnionA, VdUnionE>: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 (VdUnionE): a required key is missing (reading the key "msg" of VdUnionE))",
        "", read_leg::skipped);
    expect_invalid(
        VdOneAD(VdUnionA{-1}),
        R"(no alternative accepts the value; reading yyjson::one_of_variant<VdUnionA, VdUnionD>: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 (VdUnionD): a required key is missing (reading the key "y" of VdUnionD))",
        "");
    // a walk that fails for the alternative that read gives too, which read does not check
    expect_invalid(
        std::variant<VdReadBack, int>(VdReadBack{3, {}, std::nullopt}),
        R"(the value is not valid at /z; validating VdReadBack: the field "z" holds an empty std::optional, which write leaves out and a read gives the default value)",
        "/z", read_leg::skipped);

    // the count of a one_of, which a disjoint alternative skips
    expect_invalid(VdOneCD(VdUnionC{1, 2}),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdUnionC, VdUnionD>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_valid(VdOneCD(VdUnionC{1, {}}));
    expect_valid(VdOneCD(VdUnionD{1}));
    expect_valid(yyjson::one_of_variant<int, std::string>(1));
    expect_valid(VdSignedOrUnsigned(std::int64_t{-5}));
    expect_invalid(
        VdSignedOrUnsigned(std::int64_t{5}),
        std::format("the value matches more than one alternative; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "");
    expect_invalid(
        std::vector<VdSignedOrUnsigned>{std::int64_t{-5}, std::uint64_t{5}},
        std::format("the value matches more than one alternative at /1; reading {}: the alternatives 0 and 1 match",
                    yyjson::detail::variant_name<yyjson::one_of_variant<std::int64_t, std::uint64_t>>()),
        "/1");
    expect_valid(VdPartlyOverlapping(-300.0));
    expect_valid(VdPartlyOverlapping(std::uint64_t{20}));
    expect_invalid(
        VdPartlyOverlapping(std::int64_t{0}),
        std::format(
            "the value matches more than one alternative; reading {}: the alternatives 0 and 2 match",
            yyjson::detail::variant_name<yyjson::one_of_variant<double, std::string, std::int64_t, std::uint64_t>>()),
        "");
    expect_invalid(
        VdPartlyOverlapping(std::uint64_t{5}),
        std::format(
            "the value matches more than one alternative; reading {}: the alternatives 2 and 3 match",
            yyjson::detail::variant_name<yyjson::one_of_variant<double, std::string, std::int64_t, std::uint64_t>>()),
        "");

    // nested unions, and a union in an array, a map and a struct
    expect_invalid(yyjson::any_of_variant<VdOneCD, int>(VdOneCD(VdUnionC{1, 2})),
                   "no alternative accepts the value; reading yyjson::any_of_variant<yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>, int>: alternative 0 (yyjson::one_of_variant<VdUnionC, VdUnionD>): the value matches "
                   "more than one alternative (reading yyjson::one_of_variant<VdUnionC, VdUnionD>: the alternatives 0 "
                   "and 1 match); alternative 1 (int): expected an integer, got an object",
                   "");
    expect_valid(yyjson::any_of_variant<VdOneAD, VdUnionB>(VdOneAD(VdUnionA{-1})));
    expect_valid(yyjson::one_of_variant<VdAB, VdUnionD>(VdAB(VdUnionA{-1})));
    expect_invalid(std::vector<VdOneCD>{VdUnionD{1}, VdUnionD{2}, VdUnionD{3}, VdUnionC{1, 2}},
                   "the value matches more than one alternative at /3; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/3");
    expect_invalid(std::map<std::string, VdOneCD>{{"k", VdUnionC{1, 2}}},
                   "the value matches more than one alternative at /k; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/k");
    expect_invalid(VdUnionFields{VdUnionC{1, 2}, 1},
                   "the value matches more than one alternative at /u; reading yyjson::one_of_variant<VdUnionC, "
                   "VdUnionD>: the alternatives 0 and 1 match",
                   "/u");
    expect_valid(VdUnionFields{VdUnionC{1, {}}, std::string("ab")});
    // the settings of an alternative that is a variant check the alternative that the inner variant holds
    using VdNested = yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<int, std::string>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>;
    using VdInner = std::variant<std::variant<int, std::string>, bool>;
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(7))));
    expect_valid(VdNested(VdInner(std::variant<int, std::string>(std::string("ab")))));
    expect_valid(VdNested(VdInner(true)));
    expect_invalid(
        VdNested(VdInner(std::variant<int, std::string>(1))),
        std::format(
            "no alternative accepts the value; reading {}: alternative 0 ({}): the number is less than the minimum 5",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");
    expect_invalid(
        VdNested(VdInner(std::variant<int, std::string>(std::string("a")))),
        std::format(
            "no alternative accepts the value; reading {}: alternative 0 ({}): the string is shorter than the minimum "
            "length 2",
            yyjson::detail::variant_name<yyjson::any_of_variant<std::variant<std::variant<int, std::string>, bool>>>(),
            yyjson::detail::variant_name<std::variant<std::variant<int, std::string>, bool>>()),
        "");

    // a passing check of a disjoint union, or of an alternative that satisfies its settings, allocates nothing
    EXPECT_EQ(validate_allocations(std::vector<VdLowOrInt>(100, VdLowOrInt(std::in_place_index<0>, 1))), 0U);
    EXPECT_EQ(validate_allocations(yyjson::one_of_variant<int, std::string>(1)), 0U);
    EXPECT_EQ(validate_allocations(VdDisjointField{1, std::string("x")}), 0U);
}

// a union in a tagged variant, and a variant that is tagged
struct VdClosedA
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedA> = yyjson::annotate(yyjson::additional_properties(false));
struct VdTypedA
{
    std::string type;
    int a = 0;
};
using VdOneClosed = yyjson::one_of_variant<VdClosedA, VdTypedA>;
struct VdOtherZ
{
    int z = 0;
};
using VdTaggedOne = std::variant<VdOneClosed, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedOne> = yyjson::annotate(yyjson::internal_tag("type", {"one", "z"}));
using VdTaggedAD = std::variant<VdUnionA, VdUnionD>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedAD> = yyjson::annotate(yyjson::internal_tag("kind", {"a", "d"}));
// a union that a wrapper holds in a tagged variant
struct VdOneWrapped
{
    VdOneClosed u;
};
template <>
inline constexpr auto yyjson::annotations<VdOneWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedWrapped = std::variant<VdOneWrapped, std::unique_ptr<VdOneClosed>, std::optional<VdOneClosed>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedWrapped> =
    yyjson::annotate(yyjson::internal_tag("type", {"w", "p", "o", "z"}));
// a variant that is not tagged, whose map alternative reads the written object only without the tag
using VdUnionOrMap = std::variant<VdUnionA, std::map<std::string, int>>;
struct VdMapWrapped
{
    VdUnionOrMap u;
};
template <>
inline constexpr auto yyjson::annotations<VdMapWrapped> = yyjson::annotate(yyjson::transparent);
using VdTaggedMap =
    std::variant<VdUnionOrMap, VdMapWrapped, std::unique_ptr<VdUnionOrMap>, std::optional<VdUnionOrMap>, VdOtherZ>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedMap> =
    yyjson::annotate(yyjson::internal_tag("type", {"u", "w", "p", "o", "z"}));
struct VdAdjacentAB
{
    std::string kind = "a";
    std::variant<VdUnionA, VdUnionB> data;
};
template <>
inline constexpr auto yyjson::annotations<VdAdjacentAB> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"a", "b"})));

TEST(Validate, TaggedUnions)
{
    // the key of the tag lets the alternatives of the one_of read the same object
    expect_valid(VdOneClosed(VdClosedA{0}));
    expect_invalid(VdTaggedOne(VdOneClosed(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedOne(VdOneClosed(VdTypedA{"one", 0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(std::vector<VdTaggedOne>{VdOtherZ{}, VdOneClosed(VdClosedA{0})},
                   "the value matches more than one alternative at /1; reading yyjson::one_of_variant<VdClosedA, "
                   "VdTypedA>: the alternatives 0 and 1 match",
                   "/1");
    expect_valid(VdTaggedOne(VdOtherZ{1}));

    // the tag reaches a union through a transparent struct, a pointer and an optional
    expect_invalid(VdTaggedWrapped(VdOneWrapped{VdClosedA{0}}),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedWrapped(std::make_unique<VdOneClosed>(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_invalid(VdTaggedWrapped(std::optional<VdOneClosed>(VdClosedA{0})),
                   "the value matches more than one alternative; reading yyjson::one_of_variant<VdClosedA, VdTypedA>: "
                   "the alternatives 0 and 1 match",
                   "");
    expect_valid(VdTaggedWrapped(VdOtherZ{1}));
    expect_invalid(
        VdTaggedMap(VdUnionOrMap(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(VdMapWrapped{VdUnionA{-1}}),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(std::make_unique<VdUnionOrMap>(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_invalid(
        VdTaggedMap(std::optional<VdUnionOrMap>(VdUnionA{-1})),
        std::format(
            R"(no alternative accepts the value; reading {}: alternative 0 (VdUnionA) at /x: the number is less than the minimum 0 (reading the key "x" of VdUnionA); alternative 1 ({}) at /type: expected an integer, got a string (reading int))",
            yyjson::detail::variant_name<std::variant<VdUnionA, std::map<std::string, int>>>(),
            yyjson::detail::type_name<std::map<std::string, int>>()),
        "");
    expect_valid(VdTaggedMap(VdUnionOrMap(VdUnionA{1})));
    expect_valid(VdTaggedMap(VdMapWrapped{VdUnionA{1}}));

    // a tagged variant reads only the alternative of its tag
    expect_invalid(VdTaggedAD(VdUnionA{-1}),
                   R"(the number is less than the minimum 0 at /x; reading the key "x" of VdUnionA)", "/x");
    expect_invalid(VdAdjacentAB{"a", VdUnionA{-1}},
                   R"(the number is less than the minimum 0 at /data/x; reading the key "x" of VdUnionA)", "/data/x");
    expect_valid(VdAdjacentAB{"b", VdUnionB{-1}});
}

// a union whose alternative write gives a repeated key or null for a NaN
using VdRepeatedOrMap = std::variant<std::multimap<std::string, int>, std::map<std::string, int>>;
using VdLowOrNull = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, std::nullptr_t>;
using VdLowDouble = yyjson::any_of_variant<yyjson::alt<double, yyjson::minimum(0)>, int>;
// a tree whose depth the check limits through a union
struct VdUnionTree
{
    int v = 0;
    std::vector<std::variant<VdUnionTree, int>> kids;
};
template <>
inline constexpr auto yyjson::annotations<VdUnionTree> = yyjson::annotate(yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, UnionWrittenForms)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    // the other alternative does not read the key twice either, as a strict read
    const auto repeated = VdRepeatedOrMap(std::multimap<std::string, int>{{"a", 1}, {"a", 2}});
    const auto twice = [](int i, std::string_view type) {
        return std::format("alternative {} ({}) at /a: the object has a duplicate key (reading {})", i, type, type);
    };
    const auto message = std::format("no alternative accepts the value; reading {}: {}; {}", vd_name<VdRepeatedOrMap>(),
                                     twice(0, yyjson::detail::type_name<std::multimap<std::string, int>>()),
                                     twice(1, yyjson::detail::type_name<std::map<std::string, int>>()));
    expect_invalid(repeated, message, "", read_leg::skipped);
    EXPECT_NO_THROW((void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated)));
    EXPECT_EQ(what_without_position(validation_error(
                  [&] { (void)yyjson::read<VdRepeatedOrMap>(yyjson::write(repeated), yyjson::strict); })),
              message);

    // a NaN that write gives as null, which another alternative reads
    const auto nan = std::nan("");
    EXPECT_EQ(std::string(yyjson::write(VdLowOrNull(nan), WriteFlag::InfAndNanAsNull, with_validation)), "null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::InfAndNanAsNull, with_validation);
                          }).what()),
              "no alternative accepts the value; reading " + vd_name<VdLowDouble>() +
                  ": alternative 0 (double): expected a number, got null; alternative 1 (int): expected an integer, "
                  "got null");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdLowDouble(nan), WriteFlag::AllowInfAndNan, with_validation);
                          }).what()),
              "no alternative accepts the value; reading " + vd_name<VdLowDouble>() +
                  ": alternative 0 (double): the number is less than the minimum 0; alternative 1 (int): expected an "
                  "integer, got a number");

    // the depth of a walk and of the read of a failing union
    expect_invalid(
        VdUnionTree{0, {VdUnionTree{-1, {}}}},
        R"(no alternative accepts the value at /kids/0; reading std::variant<VdUnionTree, int>: alternative 0 (VdUnionTree) at /v: the number is less than the minimum 0 (reading the key "v" of VdUnionTree); alternative 1 (int): expected an integer, got an object)",
        "/kids/0");
    auto tree = VdUnionTree();
    for (auto i = 0; i < 300; ++i) tree = VdUnionTree{0, {std::move(tree)}};
    const auto e = validation_error([&] { yyjson::validate(tree); });
    EXPECT_TRUE(std::string_view(e.what()).find("CPPYYJSON_RECURSION_DEPTH_LIMIT") != std::string_view::npos)
        << e.what();
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
    auto failing = VdUnionTree{-1, {}};
    for (auto i = 0; i < 100; ++i) failing = VdUnionTree{0, {std::move(failing)}};
    EXPECT_THROW(yyjson::validate(failing), yyjson::bad_cast);
    EXPECT_EQ(yyjson::detail::recursion_depth, 0);
}

// the keywords on the JSON of the casters of the user, which validate checks on the JSON that to_json writes
namespace
{
    int vd_to_json_calls = 0;
}
class VdName
{
public:
    VdName() = default;
    explicit VdName(std::string s) : s_(std::move(s)) {}
    const std::string& str() const { return s_; }
    bool operator==(const VdName&) const = default;

private:
    std::string s_;
};
template <>
struct yyjson::caster<VdName>
{
    static VdName from_json(std::string s) { return VdName(std::move(s)); }
    static std::string to_json(const VdName& n)
    {
        ++vd_to_json_calls;
        return n.str();
    }
};
// a translator to an object that leaves out y for a negative x
struct VdXY
{
    int x = 0;
    int y = 0;
};
template <>
struct yyjson::caster<VdXY>
{
    static VdXY from_json(const std::map<std::string, int>& m) { return {m.at("x"), m.contains("y") ? m.at("y") : 0}; }
    static std::map<std::string, int> to_json(const VdXY& p)
    {
        ++vd_to_json_calls;
        if (p.x < 0) return {{"x", p.x}};
        return {{"x", p.x}, {"y", p.y}};
    }
};
template <>
inline constexpr auto yyjson::annotations<VdXY> = yyjson::annotate(yyjson::min_properties(2));
// an emitter that writes n members
struct VdEmit
{
    int n = 0;
};
template <>
struct yyjson::caster<VdEmit>
{
    static VdEmit from_json(const std::map<std::string, int>& m) { return {static_cast<int>(m.size())}; }
    static void to_json(yyjson::object_emitter& e, const VdEmit& v)
    {
        ++vd_to_json_calls;
        for (int i = 0; i < v.n; ++i) e.emplace(std::string(1, static_cast<char>('a' + i)), i);
    }
};
template <>
inline constexpr auto yyjson::annotations<VdEmit> = yyjson::annotate(yyjson::max_properties(2));
struct VdDigits
{
    static std::string to_json(int v) { return std::to_string(v); }
    static int from_json(const std::string& s) { return std::stoi(s); }
};
struct VdEmitInt
{
    static void to_json(yyjson::object_emitter& e, int v)
    {
        for (int i = 0; i < v; ++i) e.emplace(std::string(1, static_cast<char>('a' + i)), i);
    }
    static int from_json(const std::map<std::string, int>& m) { return static_cast<int>(m.size()); }
};
struct VdCasters
{
    VdName name;
    VdXY xy;
    VdEmit em;
    yyjson::with_caster<int, VdDigits> w{};
    yyjson::with_caster<int, VdEmitInt> e{};
    VdEmit fem;
};
template <>
inline constexpr auto yyjson::annotations<VdCasters> =
    yyjson::annotate(yyjson::field("name", yyjson::max_length(2)), yyjson::field("w", yyjson::max_length(2)),
                     yyjson::field("e", yyjson::max_properties(1)), yyjson::field("fem", yyjson::min_properties(1)));
struct VdCasterPlaces
{
    std::optional<VdName> c;
    std::vector<VdXY> ps;
    std::map<std::string, VdXY> mp;
    yyjson::any_of_variant<VdXY, int> v = 0;
    std::tuple<int, VdXY> t;
    std::unique_ptr<VdEmit> p;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterPlaces> = yyjson::annotate(yyjson::field("c", yyjson::max_length(2)));
struct VdCasterRow
{
    int id = 0;
    VdName name;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterRow> =
    yyjson::annotate(yyjson::as_array, yyjson::field("name", yyjson::pattern("^[a-z]+$")));
struct VdCasterBox
{
    VdName name;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterBox> =
    yyjson::annotate(yyjson::transparent, yyjson::field("name", yyjson::min_length(1)));
struct VdCasterRest
{
    int a = 0;
    std::map<std::string, VdXY> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterRest> =
    yyjson::annotate(yyjson::max_properties(3), yyjson::field("rest", yyjson::extra_keys));
// the order of the fields: the first violation of read
struct VdNameAndList
{
    VdName c;
    std::vector<VdName> cs;
};
template <>
inline constexpr auto yyjson::annotations<VdNameAndList> =
    yyjson::annotate(yyjson::field("c", yyjson::pattern("^a")), yyjson::field("cs", yyjson::max_items(1)));
// a caster type without keywords in a struct with keywords
struct VdUncheckedName
{
    VdName name;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUncheckedName> = yyjson::annotate(yyjson::field("n", yyjson::minimum(0)));
// a translator to a number, which write gives as null for a NaN with `WriteFlag::InfAndNanAsNull`
struct VdCelsius
{
    double v = 0;
};
template <>
struct yyjson::caster<VdCelsius>
{
    static VdCelsius from_json(double d) { return {d}; }
    static double to_json(const VdCelsius& c)
    {
        ++vd_to_json_calls;
        return c.v;
    }
};
struct VdTemperatures
{
    VdCelsius low;
    VdCelsius high;
};
template <>
inline constexpr auto yyjson::annotations<VdTemperatures> =
    yyjson::annotate(yyjson::field("low", yyjson::maximum(100)), yyjson::field("high", yyjson::enum_(1.5, 2.5)));
// casters in an internally tagged variant, which read with the key of the tag: n members besides the tag
struct VdLetters
{
    int n = 0;
};
template <>
struct yyjson::caster<VdLetters>
{
    static VdLetters from_json(const std::map<std::string, std::string>& m)
    {
        return {static_cast<int>(m.size() - m.count("type"))};
    }
    static std::map<std::string, std::string> to_json(const VdLetters& v)
    {
        auto m = std::map<std::string, std::string>();
        for (int i = 0; i < v.n; ++i) m.emplace(std::string(1, static_cast<char>('a' + i)), "x");
        return m;
    }
};
template <>
inline constexpr auto yyjson::annotations<VdLetters> = yyjson::annotate(yyjson::max_properties(2));
struct VdLettersBox
{
    VdLetters e;
};
template <>
inline constexpr auto yyjson::annotations<VdLettersBox> =
    yyjson::annotate(yyjson::transparent, yyjson::field("e", yyjson::min_properties(2)));
struct VdWidth
{
    int w = 0;
};
using VdTaggedCasters = std::variant<VdLetters, VdLettersBox, VdWidth>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedCasters> =
    yyjson::annotate(yyjson::internal_tag("type", {"l", "box", "w"}));
// casters that check the keys of their objects, also as alternatives of a variant tagged in the object
struct VdClosedKeys
{
    int n = 0;
};
template <>
struct yyjson::caster<VdClosedKeys>
{
    static VdClosedKeys from_json(const std::map<std::string, std::string>& m)
    {
        return {static_cast<int>(m.size() - m.count("type"))};
    }
    static std::map<std::string, std::string> to_json(const VdClosedKeys& v)
    {
        auto m = std::map<std::string, std::string>();
        for (int i = 0; i < v.n; ++i) m.emplace(std::string(1, static_cast<char>('a' + i)), "x");
        return m;
    }
};
template <>
inline constexpr auto yyjson::annotations<VdClosedKeys> = yyjson::annotate(yyjson::additional_properties(false));
struct VdNamedKey
{
    std::string key;
};
template <>
struct yyjson::caster<VdNamedKey>
{
    static VdNamedKey from_json(const std::map<std::string, std::string>& m)
    {
        for (const auto& [k, v] : m)
            if (k != "type") return {k};
        return {};
    }
    static std::map<std::string, std::string> to_json(const VdNamedKey& v)
    {
        if (v.key.empty()) return {};
        return {{v.key, "x"}};
    }
};
template <>
inline constexpr auto yyjson::annotations<VdNamedKey> = yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
using VdTaggedKeys = std::variant<VdClosedKeys, VdNamedKey>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedKeys> = yyjson::annotate(yyjson::internal_tag("type", {"c", "s"}));
// a closed type whose caster only reads, so that write reflects it and gives the keys of its fields
struct VdReadClosed
{
    int a = 0;
};
template <>
struct yyjson::caster<VdReadClosed>
{
    static VdReadClosed from_json(const std::map<std::string, int>& m) { return {m.at("x")}; }
};
template <>
inline constexpr auto yyjson::annotations<VdReadClosed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("x")), yyjson::additional_properties(false));
struct VdReadClosedHolder
{
    VdReadClosed s;
};
// translators to an integer and to a bool
struct VdLevel
{
    int v = 0;
};
template <>
struct yyjson::caster<VdLevel>
{
    static VdLevel from_json(long v) { return {static_cast<int>(v)}; }
    static long to_json(const VdLevel& l) { return l.v; }
};
struct VdSwitch
{
    bool on = false;
};
template <>
struct yyjson::caster<VdSwitch>
{
    static VdSwitch from_json(bool b) { return {b}; }
    static bool to_json(const VdSwitch& s) { return s.on; }
};
struct VdPanel
{
    std::optional<VdLevel> level;
    VdSwitch power;
};
template <>
inline constexpr auto yyjson::annotations<VdPanel> = yyjson::annotate(
    yyjson::field("level", yyjson::minimum(0), yyjson::maximum(9)), yyjson::field("power", yyjson::const_(true)));

TEST(Validate, CasterJson)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto ok = with<VdCasters>([](auto& v) {
        v.xy = {1, 2};
        v.fem = {1};
    });
    expect_valid(ok);

    // field and type keywords on translators, emitters and `yyjson::with_caster`
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.name = VdName("abc");
                   }),
                   R"(the string is longer than the maximum length 2 at /name; reading the key "name" of VdCasters)",
                   "/name");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.xy = {-1, 0};
                   }),
                   "the object has fewer properties than the minimum 2 at /xy; reading VdXY", "/xy");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.em = {3};
                   }),
                   "the object has more properties than the maximum 2 at /em; reading VdEmit", "/em");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.w = yyjson::with_caster<int, VdDigits>{123};
                   }),
                   R"(the string is longer than the maximum length 2 at /w; reading the key "w" of VdCasters)", "/w");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.e = yyjson::with_caster<int, VdEmitInt>{2};
                   }),
                   R"(the object has more properties than the maximum 1 at /e; reading the key "e" of VdCasters)",
                   "/e");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.fem = {0};
                   }),
                   R"(the object has fewer properties than the minimum 1 at /fem; reading the key "fem" of VdCasters)",
                   "/fem");
    // the type keywords first, then the field keywords, as read checks them
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.fem = {3};
                   }),
                   "the object has more properties than the maximum 2 at /fem; reading VdEmit", "/fem");

    // root caster types
    expect_valid(VdXY{1, 2});
    expect_invalid(VdXY{-1, 0}, "the object has fewer properties than the minimum 2; reading VdXY", "");
    expect_invalid(VdEmit{3}, "the object has more properties than the maximum 2; reading VdEmit", "");
    expect_valid(yyjson::with_caster<int, VdDigits>{5});

    // the positions in the containers, the alternatives and the pointers
    const auto places = [](auto f) {
        auto v = VdCasterPlaces();
        v.t = {1, VdXY{1, 2}};
        f(v);
        return v;
    };
    expect_valid(places([](auto&) {}));
    expect_invalid(places([](auto& v) { v.c = VdName("abc"); }),
                   R"(the string is longer than the maximum length 2 at /c; reading the key "c" of VdCasterPlaces)",
                   "/c");
    expect_invalid(places([](auto& v) { v.ps = {{1, 1}, {-1, 0}}; }),
                   "the object has fewer properties than the minimum 2 at /ps/1; reading VdXY", "/ps/1");
    expect_invalid(places([](auto& v) { v.mp = {{"k", {-1, 0}}}; }),
                   "the object has fewer properties than the minimum 2 at /mp/k; reading VdXY", "/mp/k");
    expect_invalid(places([](auto& v) { v.t = {1, VdXY{-1, 0}}; }),
                   "the object has fewer properties than the minimum 2 at /t/1; reading VdXY", "/t/1");
    expect_invalid(places([](auto& v) { v.p = std::make_unique<VdEmit>(VdEmit{3}); }),
                   "the object has more properties than the maximum 2 at /p; reading VdEmit", "/p");
    expect_invalid(places([](auto& v) { v.v = VdXY{-1, 0}; }),
                   "no alternative accepts the value at /v; reading yyjson::any_of_variant<VdXY, int>: alternative 0 "
                   "(VdXY): the object has fewer properties than the minimum 2 (reading VdXY); alternative 1 (int): "
                   "expected an integer, got an object",
                   "/v");
    // `yyjson::as_array` positions, a transparent field and the values of `yyjson::extra_keys`
    expect_valid(VdCasterRow{1, VdName("ab")});
    expect_invalid(VdCasterRow{1, VdName("A")},
                   R"(the string does not match the pattern "^[a-z]+$" at /1; reading an element of VdCasterRow)",
                   "/1");
    expect_valid(VdCasterBox{VdName("a")});
    expect_invalid(VdCasterBox{VdName("")}, "the string is shorter than the minimum length 1; reading VdCasterBox", "");
    expect_valid(VdCasterRest{1, {{"k", {1, 2}}}});
    expect_invalid(VdCasterRest{1, {{"k", {-1, 0}}}},
                   "the object has fewer properties than the minimum 2 at /k; reading VdXY", "/k");

    // the first violation in the order of the fields
    expect_invalid(VdNameAndList{VdName("b"), {VdName("a"), VdName("a")}},
                   R"(the string does not match the pattern "^a" at /c; reading the key "c" of VdNameAndList)", "/c");

    // to_json runs only for a position with keywords on its JSON: once for validate, twice for a checked write
    const auto unchecked = VdUncheckedName{VdName("abc"), 1};
    vd_to_json_calls = 0;
    yyjson::validate(unchecked);
    EXPECT_EQ(vd_to_json_calls, 0);
    expect_invalid(VdUncheckedName{VdName("abc"), -1},
                   R"(the number is less than the minimum 0 at /n; reading the key "n" of VdUncheckedName)", "/n");
    const auto checked = with<VdCasters>([&](auto& v) { v = ok; });
    vd_to_json_calls = 0;
    yyjson::validate(checked.name);
    EXPECT_EQ(vd_to_json_calls, 0);
    yyjson::validate(VdXY{1, 2});
    EXPECT_EQ(vd_to_json_calls, 1);
    const auto plain = std::string(yyjson::write(VdXY{1, 2}));
    vd_to_json_calls = 0;
    EXPECT_EQ(std::string(yyjson::write(VdXY{1, 2}, with_validation)), plain);
    EXPECT_EQ(vd_to_json_calls, 2);

    // a NaN that a translator gives as a number: null with `WriteFlag::InfAndNanAsNull`, which the caster cannot read
    const auto nan = std::nan("");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{nan}, {1.5}}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "expected a number, got null at /low; reading double");
    // the read back fails before the enum, as for a field of `double`
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{1}, {nan}}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "expected a number, got null at /high; reading double");
    const auto too_high =
        std::string(R"(the number is greater than the maximum 100 at /low; reading the key "low" of VdTemperatures)");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(VdTemperatures{{nan}, {1.5}}); }).what()), too_high);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{nan}, {1.5}}, WriteFlag::AllowInfAndNan,
                                                  with_validation);
                          }).what()),
              too_high);
    expect_invalid(VdTemperatures{{200}, {1.5}},
                   R"(the number is greater than the maximum 100 at /low; reading the key "low" of VdTemperatures)",
                   "/low");
#if YYJSON_VERSION_HEX >= 0x000C00
    // the keywords check the real as the flags write it
    const auto fixed = static_cast<WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    vd_to_json_calls = 0;
    EXPECT_EQ(std::string(yyjson::write(VdTemperatures{{100.04}, {1.5}}, fixed, with_validation)),
              R"({"low":100.0,"high":1.5})");
    EXPECT_EQ(vd_to_json_calls, 4);
#endif

    // the members of the caster count the key of the tag, also under a struct that checks its object
    expect_valid(VdTaggedCasters{VdLetters{1}});
    expect_invalid(VdTaggedCasters{VdLetters{2}},
                   "the object has more properties than the maximum 2; reading VdLetters", "");
    expect_invalid(std::vector<VdTaggedCasters>{VdLetters{2}},
                   "the object has more properties than the maximum 2 at /0; reading VdLetters", "/0");
    expect_valid(VdTaggedCasters{VdLettersBox{{1}}});
    expect_invalid(VdTaggedCasters{VdLettersBox{{0}}},
                   "the object has fewer properties than the minimum 2; reading VdLettersBox", "");
    expect_valid(std::vector<VdTaggedCasters>{VdLettersBox{{1}}});

    // the keys of the object of a caster: a closed type takes only the key of a tag, and yyjson::property_names
    // checks that key too
    const auto closed = R"(VdClosedKeys has the key "a", which yyjson::additional_properties(false) rejects)";
    expect_valid(VdClosedKeys{0});
    expect_invalid(VdClosedKeys{1},
                   "the object has a key that is not allowed at /a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/a");
    expect_invalid(std::vector<VdClosedKeys>{{0}, {2}},
                   "the object has a key that is not allowed at /1/a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/1/a");
    expect_valid(VdTaggedKeys{VdClosedKeys{0}});
    expect_invalid(VdTaggedKeys{VdClosedKeys{1}},
                   "the object has a key that is not allowed at /a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/a");
    expect_valid(VdNamedKey{"ab"});
    expect_invalid(VdNamedKey{"abc"},
                   "the key is longer than the maximum length 2 at /abc; reading a key of VdNamedKey", "/abc");
    expect_invalid(std::vector<VdNamedKey>{{"ab"}, {"abc"}},
                   "the key is longer than the maximum length 2 at /1/abc; reading a key of VdNamedKey", "/1/abc");
    expect_invalid(VdTaggedKeys{VdNamedKey{}},
                   "the key is longer than the maximum length 2 at /type; reading a key of VdNamedKey", "/type");
    // a closed type that write reflects takes the keys of its fields
    expect_valid(VdReadClosed{1});
    expect_valid(VdReadClosedHolder{{1}});
    EXPECT_EQ(validation_error([] { (void)yyjson::read<VdReadClosed>(R"({"x":1,"y":2})"); }).path(), "/y");
    const auto nested = validation_error([] { (void)yyjson::read<VdReadClosedHolder>(R"({"s":{"x":1,"y":2}})"); });
    EXPECT_EQ(std::string_view(nested.what()),
              "the object has a key that is not allowed at /s/y (line 1, column 17, pos 16); reading VdReadClosed, "
              "which yyjson::additional_properties(false) closes")
        << nested.what();
    EXPECT_EQ(nested.path(), "/s/y");

    // translators to an integer and to a bool, also in an optional
    expect_valid(VdPanel{VdLevel{9}, VdSwitch{true}});
    expect_valid(VdPanel{std::nullopt, VdSwitch{true}});
    expect_invalid(VdPanel{VdLevel{10}, VdSwitch{true}},
                   R"(the number is greater than the maximum 9 at /level; reading the key "level" of VdPanel)",
                   "/level");
    expect_invalid(VdPanel{VdLevel{-1}, VdSwitch{true}},
                   R"(the number is less than the minimum 0 at /level; reading the key "level" of VdPanel)", "/level");
    expect_invalid(VdPanel{VdLevel{1}, VdSwitch{false}},
                   R"(the value is not equal to the constant true at /power; reading the key "power" of VdPanel)",
                   "/power");
}

// translators to a struct whose keywords check the value that `to_json` returns
struct VdWire
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWire> = yyjson::annotate(yyjson::field("n", yyjson::minimum(1)));
struct VdWrap
{
    int n = 0;
};
template <>
struct yyjson::caster<VdWrap>
{
    static VdWrap from_json(const VdWire& w) { return {w.n}; }
    static VdWire to_json(const VdWrap& w) { return {w.n}; }
};
struct VdWraps
{
    VdWrap w;
    std::vector<VdWrap> ws;
};
struct VdWireTwo
{
    int n = 0;
    int m = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWireTwo> = yyjson::annotate(yyjson::max_properties(2));
struct VdWrapTwo
{
    int n = 0;
};
template <>
struct yyjson::caster<VdWrapTwo>
{
    static VdWrapTwo from_json(const VdWireTwo& w) { return {w.n}; }
    static VdWireTwo to_json(const VdWrapTwo& w) { return {w.n, 0}; }
};
struct VdOtherK
{
    int k = 0;
};
using VdTaggedWrap = std::variant<VdWrapTwo, VdOtherK>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedWrap> = yyjson::annotate(yyjson::internal_tag("type", {"a", "b"}));
struct VdWireReal
{
    double d = 0;
    int k = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWireReal> = yyjson::annotate(yyjson::field("k", yyjson::minimum(0)));
struct VdWrapReal
{
    double d = 0;
    int k = 0;
};
template <>
struct yyjson::caster<VdWrapReal>
{
    static VdWrapReal from_json(const VdWireReal& w) { return {w.d, w.k}; }
    static VdWireReal to_json(const VdWrapReal& w) { return {w.d, w.k}; }
};
#if YYJSON_VERSION_HEX >= 0x000C00
// reads any JSON, also null for `d`
struct VdWrapLoose
{
    double d = 0;
    int k = 0;
};
template <>
struct yyjson::caster<VdWrapLoose>
{
    static VdWrapLoose from_json(yyjson::json_view) { return {}; }
    static VdWireReal to_json(const VdWrapLoose& w) { return {w.d, w.k}; }
};
#endif
struct VdScalarReal
{
    double d = 0;
};
template <>
struct yyjson::caster<VdScalarReal>
{
    static VdScalarReal from_json(double d) { return {d}; }
    static double to_json(const VdScalarReal& s) { return s.d; }
};
// reads only, so the value that `with_caster` holds is written
struct VdReadWire
{
    static VdWire from_json(const VdWire& w) { return w; }
};

TEST(Validate, TranslatedValue)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    expect_valid(VdWraps{{1}, {{1}, {2}}});
    expect_invalid(VdWraps{{0}, {}}, R"(the number is less than the minimum 1 at /w/n; reading the key "n" of VdWire)",
                   "/w/n");
    expect_invalid(VdWraps{{1}, {{1}, {0}}},
                   R"(the number is less than the minimum 1 at /ws/1/n; reading the key "n" of VdWire)", "/ws/1/n");
    expect_invalid(VdWrap{0}, R"(the number is less than the minimum 1 at /n; reading the key "n" of VdWire)", "/n");
    // the object of the translation holds the key of the tag
    expect_invalid(VdTaggedWrap{VdWrapTwo{1}}, "the object has more properties than the maximum 2; reading VdWireTwo",
                   "");
    expect_valid(VdTaggedWrap{VdOtherK{1}});

    // a non-finite number written as null is checked only if `from_json` reads the translation
    const auto as_null = WriteFlag::InfAndNanAsNull;
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdWrapReal{std::nan(""), 0}, as_null, with_validation);
                          }).what()),
              "expected a number, got null at /d; reading double");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(std::string(yyjson::write(VdWrapLoose{std::nan(""), 0}, as_null, with_validation)),
              R"({"d":null,"k":0})");
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdWrapLoose{std::nan(""), -1}, as_null, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /k; reading the key "k" of VdWireReal)");
#endif
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdScalarReal{std::nan("")}, as_null, with_validation);
                          }).what()),
              "expected a number, got null; reading double");
    expect_invalid(yyjson::with_caster<VdWire, VdReadWire>{VdWire{0}},
                   R"(the number is less than the minimum 1 at /n; reading the key "n" of VdWire)", "/n");
}

// a type without keywords is checked by nothing
struct VdPlain
{
    int a = 0;
    std::string b;
    std::vector<std::map<std::string, double>> c;
};

TEST(Validate, Plain)
{
    static_assert(!yyjson::detail::needs_validation<VdPlain>);
    static_assert(yyjson::detail::needs_validation<VdNested>);
    expect_valid(VdPlain{1, "x", {{{"k", 1.5}}}});
    expect_valid(std::vector<int>{1, 2});
    expect_valid(42);
}

// the keywords check the JSON that `write` gives at their position, and those of another JSON kind check nothing
enum class VdKjColor
{
    red,
    green
};
struct VdKjRect
{
    int w = 0;
    int h = 0;
};
struct VdKjCircle
{
    double r = 0;
};
using VdKjShape = std::variant<VdKjCircle, VdKjRect>;
template <>
inline constexpr auto yyjson::annotations<VdKjShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
// read from a string and written as one
class VdKjName
{
    std::string s_;

public:
    VdKjName() = default;
    explicit VdKjName(std::string s) : s_(std::move(s)) {}
    const std::string& str() const { return s_; }
};
template <>
struct yyjson::caster<VdKjName>
{
    static VdKjName from_json(std::string s) { return VdKjName(std::move(s)); }
    static std::string to_json(const VdKjName& n) { return n.str(); }
};
// written as a string, and read as an object by its field
struct VdKjText
{
    std::string s;
};
template <>
struct yyjson::caster<VdKjText>
{
    static std::string to_json(const VdKjText& t) { return t.s; }
};
struct VdKjValues
{
    VdKjRect r;
    VdKjShape s;
    std::variant<VdKjColor, std::string> v;
    VdKjColor c{};
    std::variant<int, std::string> w = std::string();
    std::tuple<int, double, std::string> t = {0, 1, ""};
    std::vector<VdKjName> names;
    std::map<std::string, VdKjName> map;
};
template <>
inline constexpr auto yyjson::annotations<VdKjValues> =
    yyjson::annotate(yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::max_properties(2)), yyjson::field("v", yyjson::max_length(3)),
                     yyjson::field("c", yyjson::max_length(3)), yyjson::field("w", yyjson::not_(yyjson::min_length(1))),
                     yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
                     yyjson::field("names", yyjson::items(yyjson::max_length(2))),
                     yyjson::field("map", yyjson::additional_properties(yyjson::max_length(2))));
struct VdKjText1
{
    VdKjText text;
};
template <>
inline constexpr auto yyjson::annotations<VdKjText1> = yyjson::annotate(yyjson::field("text", yyjson::max_length(1)));
struct VdKjExtra
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdKjExtra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
struct VdKjEvent
{
    int id = 0;
    std::variant<VdKjRect, int> data;
};
template <>
inline constexpr auto yyjson::annotations<VdKjEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));

TEST(Validate, JsonKinds)
{
    expect_valid(VdKjValues{});
    expect_valid(VdKjValues{{1, 2},
                            VdKjCircle{1},
                            VdKjColor::red,
                            VdKjColor::red,
                            std::string(),
                            {1, 2.0, "x"},
                            {VdKjName("ab")},
                            {{"k", VdKjName("ab")}}});
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.r.h = -1; }),
        R"(the object has a value that does not satisfy additionalProperties at /r; reading the key "r" of VdKjValues)",
        "/r");
    expect_invalid(with<VdKjValues>([](auto& t) { t.s = VdKjRect{1, 2}; }),
                   R"(the object has more properties than the maximum 2 at /s; reading the key "s" of VdKjValues)",
                   "/s");
    expect_invalid(with<VdKjValues>([](auto& t) { t.v = VdKjColor::green; }),
                   R"(the string is longer than the maximum length 3 at /v; reading the key "v" of VdKjValues)", "/v");
    expect_invalid(with<VdKjValues>([](auto& t) { t.c = VdKjColor::green; }),
                   R"(the string is longer than the maximum length 3 at /c; reading the key "c" of VdKjValues)", "/c");
    expect_invalid(with<VdKjValues>([](auto& t) { t.w = 1; }),
                   R"(the value satisfies the subschema of not at /w; reading the key "w" of VdKjValues)", "/w");
    expect_invalid(
        with<VdKjValues>([](auto& t) { std::get<0>(t.t) = -1; }),
        R"(the number is less than the minimum 0 at /t/0; reading an element of the value at the key "t" of VdKjValues)",
        "/t/0");
    expect_invalid(
        with<VdKjValues>([](auto& t) { std::get<0>(t.t) = 1; }),
        R"(the item is equal to an earlier item at /t/1; reading an element of the value at the key "t" of VdKjValues)",
        "/t/1");
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.names = {VdKjName("a"), VdKjName("abc")}; }),
        R"(the string is longer than the maximum length 2 at /names/1; reading an element of the value at the key "names" of VdKjValues)",
        "/names/1");
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.map = {{"k", VdKjName("abc")}}; }),
        R"(the string is longer than the maximum length 2 at /map/k; reading an element of the value at the key "map" of VdKjValues)",
        "/map/k");

    // the read takes the object of the field, and the check the string that `to_json` returns
    EXPECT_NO_THROW(yyjson::validate(VdKjText1{{"a"}}));
    expect_invalid(VdKjText1{{"ab"}},
                   R"(the string is longer than the maximum length 1 at /text; reading the key "text" of VdKjText1)",
                   "/text", read_leg::skipped);

    // the struct writes any key with the map of `yyjson::extra_keys`, and the key of an adjacent tag
    expect_valid(VdKjExtra{1, {{"b", 0}}});
    expect_invalid(VdKjExtra{1, {{"b", -1}}},
                   R"(the number is less than the minimum 0 at /b; reading the key "b" of VdKjExtra)", "/b");
    expect_valid(VdKjEvent{1, 5});
    expect_invalid(VdKjEvent{1, VdKjRect{}},
                   R"(the value is not equal to the constant "count" at /kind; reading the key "kind" of VdKjEvent)",
                   "/kind");
}

// a cast inside a callback of validate reads its JSON as read does, though write gave that JSON
struct VdCastLetter
{
    std::string s;
    auto operator<=>(const VdCastLetter&) const = default;
};
template <>
struct yyjson::caster<VdCastLetter>
{
    static VdCastLetter from_json(const std::string& s) { return {s}; }
    static std::string to_json(const VdCastLetter& c) { return c.s; }
};
inline bool vd_cast_is_a(const VdCastLetter& c) { return c.s == "a"; }
struct VdCastLetters
{
    std::set<VdCastLetter> v;
};
template <>
inline constexpr auto yyjson::annotations<VdCastLetters> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::max_length(1), yyjson::satisfies<vd_cast_is_a>))));
inline bool vd_casts_letters(const yyjson::reader::const_value_ref& json)
{
    try
    {
        (void)yyjson::cast<VdCastLetters>(json);
        return true;
    }
    catch (const yyjson::bad_cast&)
    {
        return false;
    }
}
struct VdCastHolder
{
};
using VdCastJson = std::map<std::string, std::map<std::string, std::vector<std::string>>>;
template <>
struct yyjson::caster<VdCastHolder>
{
    static VdCastHolder from_json(const VdCastJson&) { return {}; }
    static VdCastJson to_json(const VdCastHolder&) { return {{"data", {{"v", {"long", "a"}}}}}; }
};
template <>
inline constexpr auto yyjson::annotations<VdCastHolder> =
    yyjson::annotate(yyjson::property("data", yyjson::satisfies<vd_casts_letters>));

TEST(Validate, CastInsideCallback)
{
    EXPECT_THROW((void)yyjson::read<VdCastHolder>(R"({"data":{"v":["long","a"]}})"), yyjson::bad_cast);
    expect_invalid(VdCastHolder{}, "the object does not satisfy properties; reading VdCastHolder", "");
}

// NOLINTEND
