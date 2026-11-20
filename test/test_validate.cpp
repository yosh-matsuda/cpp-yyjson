#include "test_validate_common.hpp"

// NOLINTBEGIN

// `yyjson::validate(t)` and `yyjson::write(t, ..., yyjson::with_validation)`, which check a value against the
// keywords of its type before it is written

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

// NOLINTEND
