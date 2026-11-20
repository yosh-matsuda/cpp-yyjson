#include "test_constraints_common.hpp"

// NOLINTBEGIN

// const_ and enum_ reject JSON null unless they list nullptr, as JSON Schema's const and enum
struct NullableListed
{
    yyjson::nullable<std::string> unit;
    yyjson::nullable<int> level;
    std::shared_ptr<int> count;
    std::optional<std::string> note;
    std::optional<yyjson::nullable<int>> mode;
    std::shared_ptr<std::vector<int>> none;
    bool operator==(const NullableListed& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<NullableListed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm")), yyjson::field("level", yyjson::enum_(1, 2, nullptr)),
    yyjson::field("count", yyjson::const_(1)), yyjson::field("note", yyjson::const_("x")),
    yyjson::field("mode", yyjson::const_(nullptr)), yyjson::field("none", yyjson::const_(nullptr)));

// the string settings written as string literals, which hold their characters before C++26
struct Literals
{
    std::string unit;
    std::string day = "2024-01-01";
    bool operator==(const Literals&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Literals> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm")), yyjson::field("day", yyjson::format_assertion("date")));
// they are template arguments
template <auto... Settings>
struct setting_arguments
{
};
template struct setting_arguments<yyjson::pattern("^a"), yyjson::format("date"), yyjson::format_assertion("date"),
                                  yyjson::const_("c"), yyjson::enum_("e", 1), yyjson::const_(yyjson::json_text("[1]"))>;

// const_ and enum_ compare the value read as a value of the type of the field
struct Listed
{
    std::string unit = "m";
    int version = 2;
    float ratio = 0.1F;
    bool flag = true;
    std::uint64_t big = 0;
    std::optional<int> level;
    int odd = 1;
    bool operator==(const Listed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Listed> = yyjson::annotate(
    yyjson::field("unit", yyjson::enum_("mm", "cm", "m")), yyjson::field("version", yyjson::const_(2)),
    yyjson::field("ratio", yyjson::enum_(0.1, 0.5)), yyjson::field("flag", yyjson::const_(true)),
    yyjson::field("big", yyjson::enum_(-1, 0, 18446744073709551615ULL)), yyjson::field("level", yyjson::enum_(1, 2, 3)),
    yyjson::field("odd", yyjson::const_(1), yyjson::enum_(1, 3)));

// const_ and enum_ of an enumeration written by name take the JSON names of its enumerators
enum class Tone
{
    low,
    mid,
    high
};
template <>
inline constexpr auto yyjson::annotations<Tone> = yyjson::annotate(yyjson::enumerator("high", yyjson::rename("HIGH")));
struct ListedEnums
{
    Tone first = Tone::low;
    Tone loud = Tone::high;
    std::optional<Tone> maybe;
    bool operator==(const ListedEnums&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedEnums> =
    yyjson::annotate(yyjson::field("first", yyjson::const_("low")), yyjson::field("loud", yyjson::enum_("mid", "HIGH")),
                     yyjson::field("maybe", yyjson::const_("mid")));

// numbers that a document keeps as raw text with ReadFlag::NumberAsRaw
struct RawNumbers
{
    yyjson::value n;
    yyjson::value big;
    yyjson::value half;
    yyjson::value flag;
};
template <>
inline constexpr auto yyjson::annotations<RawNumbers> =
    yyjson::annotate(yyjson::field("n", yyjson::minimum(0), yyjson::maximum(100)),
                     yyjson::field("big", yyjson::minimum(9007199254740993LL)),
                     yyjson::field("half", yyjson::multiple_of(0.5)), yyjson::field("flag", yyjson::matches<bool>));

// raw numbers in the extended syntax of JSON5
struct RawExtended
{
    yyjson::value n;
    yyjson::value c;
    std::vector<yyjson::value> u = {};
    yyjson::value a;
};
template <>
inline constexpr auto yyjson::annotations<RawExtended> =
    yyjson::annotate(yyjson::field("n", yyjson::maximum(5)), yyjson::field("c", yyjson::const_(16)),
                     yyjson::field("u", yyjson::unique_items), yyjson::field("a", yyjson::unique_items));

// equality keywords on values that a document may keep as raw numbers
struct RawHolder
{
    yyjson::value v;
    bool operator==(const RawHolder&) const = default;
};
struct RawEquals
{
    yyjson::value one;
    yyjson::value listed;
    yyjson::value deep;
    RawHolder held;
    yyjson::value unique;
    yyjson::array items;
};
template <>
inline constexpr auto yyjson::annotations<RawEquals> =
    yyjson::annotate(yyjson::field("one", yyjson::const_(1)), yyjson::field("listed", yyjson::enum_("a", 2.5)),
                     yyjson::field("deep", yyjson::const_(yyjson::json_text(R"({"a":[1,2.5]})"))),
                     yyjson::field("held", yyjson::const_(yyjson::json_text(R"({"v":3})"))),
                     yyjson::field("unique", yyjson::unique_items), yyjson::field("items", yyjson::unique_items));

// a constant with a key that Spot does not have, which yyjson::strict alone accepts
struct WideText
{
    Spot at{};
};
template <>
inline constexpr auto yyjson::annotations<WideText> =
    yyjson::annotate(yyjson::field("at", yyjson::const_(yyjson::json_text(R"({"x":1,"y":2,"z":3})"))));

TEST(Constraints, Nullables)
{
    auto expected = Nullables();
    expect_accepted<Nullables>(R"({"code":null,"ratio":null,"name":null})", expected);
    expected.code = "abc";
    expected.ratio = 1.0;
    expected.name = std::make_shared<std::string>("abc");
    *expected.boxed = 3;
    expect_accepted<Nullables>(R"({"code":"abc","ratio":1.0,"name":"abc","boxed":3})", expected);
    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of Nullables )", key);
    };
    expect_rejected<Nullables>(
        R"({"code":"abcd"})",
        R"(the string is longer than the maximum length 3 at /code; reading the key "code" of Nullables)", "/code");
    expect_rejected<Nullables>(
        R"({"ratio":1.5})",
        R"(the number is greater than the maximum 1 at /ratio; reading the key "ratio" of Nullables)", "/ratio");
    expect_rejected<Nullables>(
        R"({"name":"abcd"})",
        R"(the value does not satisfy the predicate at /name; reading the key "name" of Nullables)", "/name");
    expect_rejected<Nullables>(
        R"({"boxed":-1})", R"(the number is less than the minimum 0 at /boxed; reading the key "boxed" of Nullables)",
        "/boxed");

    // std::unique_ptr, in a field and in a std::optional
    const auto owned = yyjson::read<Owned>(R"({"count":null,"level":null})");
    EXPECT_EQ(owned.count, nullptr);
    EXPECT_EQ(written(yyjson::read<Owned>(R"({"count":0,"level":5})")), R"({"count":0,"level":5})");
    const auto expect_owned_rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<Owned>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<Owned>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = Owned();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<Owned>(); }));
    };
    expect_owned_rejected(R"({"count":-1})",
                          R"(the number is less than the minimum 0 at /count; reading the key "count" of Owned)",
                          "/count");
    expect_owned_rejected(R"({"level":6})",
                          R"(the number is greater than the maximum 5 at /level; reading the key "level" of Owned)",
                          "/level");
    expect_owned_rejected(R"({"level":3})",
                          R"(the value does not satisfy the predicate at /level; reading the key "level" of Owned)",
                          "/level");
}

TEST(Constraints, NullableListed)
{
    // a key that the object leaves out is not checked
    expect_accepted<NullableListed>("{}", {});
    auto expected = NullableListed();
    expected.unit = "mm";
    expected.count = std::make_shared<int>(1);
    expected.note = "x";
    expect_accepted<NullableListed>(R"({"unit":"mm","level":null,"count":1,"note":"x","none":null})", expected);
    // null keeps a std::optional of yyjson::nullable, except in merge_into, where null resets the field
    expected = NullableListed();
    expected.mode = yyjson::nullable<int>();
    EXPECT_EQ(yyjson::read<NullableListed>(R"({"mode":null})"), expected);
    EXPECT_EQ(yyjson::read(R"({"mode":null})").cast<NullableListed>(), expected);
    expected = NullableListed();
    expected.level = 2;
    expect_accepted<NullableListed>(R"({"level":2})", expected);

    const auto field = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of NullableListed )", key);
    };
    expect_rejected<NullableListed>(
        R"({"level":3})", R"(the value is not one of 1, 2, null at /level; reading the key "level" of NullableListed)",
        "/level");
    expect_rejected<NullableListed>(
        R"({"mode":1})",
        R"(the value is not equal to the constant null at /mode; reading the key "mode" of NullableListed)", "/mode");
    expect_rejected<NullableListed>(
        R"({"none":[]})",
        R"(the value is not equal to the constant null at /none; reading the key "none" of NullableListed)", "/none");
    // null that a field holds is checked, as JSON Schema checks null, but merge_into resets the field with null
    expect_rejected<NullableListed>(
        R"({"unit":null})", R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of NullableListed)",
        "/unit", false);
    expect_rejected<NullableListed>(
        R"({"count":null})",
        R"(the value is not equal to the constant 1 at /count; reading the key "count" of NullableListed)", "/count",
        false);
    // null of a std::optional is a missing key
    expect_accepted<NullableListed>(R"({"note":null})", {});
}

TEST(Constraints, StringLiterals)
{
    expect_accepted<Literals>(R"({"unit":"cm","day":"2024-01-31"})", {"cm", "2024-01-31"});
    expect_rejected<Literals>(R"({"unit":"km"})",
                              R"(the value is not one of "mm", "cm" at /unit; reading the key "unit" of Literals)",
                              "/unit");
    expect_rejected<Literals>(
        R"({"day":"2024-01-32"})",
        R"(the string does not have the format "date" at /day; reading the key "day" of Literals)", "/day");
}

TEST(Constraints, ConstAndEnum)
{
    // a float with a listed value reads through a document, which keeps the JSON number to compare
    expect_accepted<Listed>(R"({"unit":"mm","version":2,"ratio":0.5,"flag":true,"big":18446744073709551615,"level":2})",
                            {.unit = "mm", .ratio = 0.5F, .big = 18446744073709551615ULL, .level = 2}, false);
    expect_accepted<Listed>(R"({"ratio":0.1,"level":null})", {}, false);
    // the JSON number compares, and not the float that it rounds to
    expect_rejected<Listed>(R"({"ratio":0.100000001})",
                            R"(the value is not one of 0.1, 0.5 at /ratio; reading the key "ratio" of Listed)",
                            "/ratio");
    const auto value = [](std::string_view key) { return std::format(R"(the value of the key "{}" of Listed )", key); };
    expect_rejected<Listed>(R"({"unit":"km"})",
                            R"(the value is not one of "mm", "cm", "m" at /unit; reading the key "unit" of Listed)",
                            "/unit");
    expect_rejected<Listed>(
        R"({"version":3})",
        R"(the value is not equal to the constant 2 at /version; reading the key "version" of Listed)", "/version");
    expect_rejected<Listed>(R"({"ratio":0.3})",
                            R"(the value is not one of 0.1, 0.5 at /ratio; reading the key "ratio" of Listed)",
                            "/ratio");
    expect_rejected<Listed>(R"({"flag":false})",
                            R"(the value is not equal to the constant true at /flag; reading the key "flag" of Listed)",
                            "/flag");
    expect_rejected<Listed>(
        R"({"big":18446744073709551614})",
        R"(the value is not one of -1, 0, 18446744073709551615 at /big; reading the key "big" of Listed)", "/big");
    expect_rejected<Listed>(
        R"({"level":4})", R"(the value is not one of 1, 2, 3 at /level; reading the key "level" of Listed)", "/level");
    // the constant is checked first; keywords that leave no value, as with enum_(2, 3), do not compile
    expect_rejected<Listed>(
        R"({"odd":3})", R"(the value is not equal to the constant 1 at /odd; reading the key "odd" of Listed)", "/odd");
    expect_accepted<Listed>(R"({"odd":1})", {}, false);
}

TEST(Constraints, ConstAndEnumNames)
{
    expect_accepted<ListedEnums>(R"({"first":"low","loud":"mid","maybe":"mid"})",
                                 {.loud = Tone::mid, .maybe = Tone::mid});
    expect_rejected<ListedEnums>(
        R"({"first":"mid"})",
        R"(the value is not equal to the constant "low" at /first; reading the key "first" of ListedEnums)", "/first");
    expect_rejected<ListedEnums>(
        R"({"loud":"low"})", R"(the value is not one of "mid", "HIGH" at /loud; reading the key "loud" of ListedEnums)",
        "/loud");
    expect_rejected<ListedEnums>(
        R"({"maybe":"HIGH"})",
        R"(the value is not equal to the constant "mid" at /maybe; reading the key "maybe" of ListedEnums)", "/maybe");
}

TEST(Constraints, RawNumbers)
{
    const auto raw = [](std::string_view json) {
        return yyjson::read<RawNumbers>(json, yyjson::ReadFlag::NumberAsRaw);
    };
    const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    EXPECT_EQ(*raw(R"({"n":5,"big":9007199254740993,"half":1.5,"flag":true})").n.as_raw(), "5");
    expect_raw_rejected(R"({"n":-1})",
                        R"(the number is less than the minimum 0 at /n; reading the key "n" of RawNumbers)", "/n");
    expect_raw_rejected(R"({"n":1e400})",
                        R"(the number is greater than the maximum 100 at /n; reading the key "n" of RawNumbers)", "/n");
    // an integer that fits 64 bits compares exactly, which a double does not
    expect_raw_rejected(
        R"({"big":9007199254740992})",
        R"(the number is less than the minimum 9007199254740993 at /big; reading the key "big" of RawNumbers)", "/big");
    expect_raw_rejected(R"({"half":1.25})",
                        R"(the number is not a multiple of 0.5 at /half; reading the key "half" of RawNumbers)",
                        "/half");
    // a raw number is a number, which yyjson::matches<bool> does not read
    expect_raw_rejected(R"({"flag":1})",
                        R"(the value does not satisfy matches at /flag; reading the key "flag" of RawNumbers)",
                        "/flag");
}

TEST(Constraints, RawEqualities)
{
    const auto raw = [](std::string_view json) { return yyjson::read<RawEquals>(json, yyjson::ReadFlag::NumberAsRaw); };
    const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] { (void)raw(json); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    // a raw number equals the number that its text is without the flag
    EXPECT_NO_THROW((void)raw(R"({"one":1,"listed":2.5,"deep":{"a":[1.0,2.50]},"held":{"v":3.0}})"));
    EXPECT_NO_THROW((void)raw(R"({"one":1.0,"listed":"a","unique":[1,"1",2],"items":[[1],[2]]})"));
    // a number that overflows a double is an infinity
    expect_raw_rejected(
        R"({"unique":[1e400,2e400]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/1");
    expect_raw_rejected(R"({"one":2})",
                        R"(the value is not equal to the constant 1 at /one; reading the key "one" of RawEquals)",
                        "/one");
    expect_raw_rejected(R"({"listed":2})",
                        R"(the value is not one of "a", 2.5 at /listed; reading the key "listed" of RawEquals)",
                        "/listed");
    expect_raw_rejected(
        R"({"deep":{"a":[1,2.4]}})",
        R"(the value is not equal to the constant {"a":[1,2.5]} at /deep; reading the key "deep" of RawEquals)",
        "/deep");
    expect_raw_rejected(
        R"({"held":{"v":4}})",
        R"(the value is not equal to the constant {"v":3} at /held; reading the key "held" of RawEquals)", "/held");
    // a duplicate is reported at the later element, as for a std::vector
    expect_raw_rejected(
        R"({"unique":[1,1.0]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/1");
    expect_raw_rejected(
        R"({"items":[[2],[1],[1e0]]})",
        R"(the item is equal to an earlier item at /items/2; reading an element of the value at the key "items" of RawEquals)",
        "/items/2");
    expect_rejected<RawEquals>(
        R"({"unique":[0,{"a":1},{"a":1.0}]})",
        R"(the item is equal to an earlier item at /unique/2; reading an element of the value at the key "unique" of RawEquals)",
        "/unique/2");

    // a raw number equals a number that is not raw, and the public operator== compares the raw text
    const auto doc = yyjson::read(R"([1,1.0])", yyjson::ReadFlag::NumberAsRaw);
    EXPECT_NE((*doc.as_array())[0], (*doc.as_array())[1]);
    EXPECT_NE((*doc.as_array())[0], yyjson::read("1"));
    auto mixed = yyjson::object(yyjson::read(R"({"unique":[1]})", yyjson::ReadFlag::NumberAsRaw));
    (*mixed["unique"].as_array()).emplace_back(1.0);
    const auto e = bad_cast_of([&] { (void)yyjson::value(mixed).cast<RawEquals>(); });
    EXPECT_EQ(
        message_of(e),
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of RawEquals)")
        << e.what();
    EXPECT_EQ(e.path(), "/unique/1");
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, RawExtendedNumbers)
{
    // a raw number in the syntax of JSON5 compares as the number that JSON5 reads
    for (const auto flag : {yyjson::ReadFlag::Json5, yyjson::ReadFlag::Json5 | yyjson::ReadFlag::NumberAsRaw})
    {
        const auto read = [flag](std::string_view json) { return yyjson::read<RawExtended>(json, flag); };
        const auto expect_raw_rejected = [&](std::string_view json, const expected_text& message,
                                             std::string_view path) {
            const auto e = bad_cast_of([&] { (void)read(json); });
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        EXPECT_NO_THROW((void)read(R"({"n":+5,"c":0x10,"u":[0x10,15],"a":[16.,+15]})"));
        EXPECT_NO_THROW((void)read(R"({"c":16.})"));
        expect_raw_rejected(R"({"n":0x10})",
                            R"(the number is greater than the maximum 5 at /n; reading the key "n" of RawExtended)",
                            "/n");
        expect_raw_rejected(R"({"n":+10})",
                            R"(the number is greater than the maximum 5 at /n; reading the key "n" of RawExtended)",
                            "/n");
        expect_raw_rejected(R"({"c":0x11})",
                            R"(the value is not equal to the constant 16 at /c; reading the key "c" of RawExtended)",
                            "/c");
        expect_raw_rejected(
            R"({"u":[0x10,16]})",
            R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of RawExtended)",
            "/u/1");
        expect_raw_rejected(
            R"({"a":[0x10,16.]})",
            R"(the item is equal to an earlier item at /a/1; reading an element of the value at the key "a" of RawExtended)",
            "/a/1");
    }
}
#endif

TEST(Constraints, TextUnderStrict)
{
    // the constant compares the JSON object, with the key that Spot does not have
    EXPECT_EQ(yyjson::read<WideText>(R"({"at":{"x":1,"y":2,"z":3}})").at, (Spot{1, 2}));
    EXPECT_THROW((void)yyjson::read<WideText>(R"({"at":{"x":1,"y":2,"z":3}})", yyjson::reject_unknown_keys),
                 yyjson::bad_cast);
    for (const auto* json : {R"({"at":{"x":1,"y":2}})", R"({"at":{"x":1,"y":3,"z":3}})"})
        expect_rejected<WideText>(
            json,
            R"(the value is not equal to the constant {"x":1,"y":2,"z":3} at /at; reading the key "at" of WideText)",
            "/at");
}

TEST(Constraints, ConstAndEnumTexts)
{
    // the JSON values compare, which only a document keeps
    expect_accepted<ListedTexts>(R"({"at":{"y":1,"x":1},"pair":[1,2],"scale":[2.5,2.5],"mixed":[0]})",
                                 {.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}}, false);
    // the number 1.0 equals 1, and an integer is a number of the variant, which reads it into its alternative
    expect_accepted<ListedTexts>(R"({"scale":[1.0],"mixed":0})", {.scale = std::vector{1.0}}, false);
    expect_accepted<ListedTexts>("{}", {}, false);
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of ListedTexts )", key);
    };
    // the members of an object have no order
    expect_accepted<ListedTexts>(R"({"order":{"b":1,"a":2}})", {.order = {{"b", 1}, {"a", 2}}}, false);
    expect_rejected<ListedTexts>(
        R"({"order":{"a":2}})",
        R"(the value is not equal to the constant {"a":2,"b":1} at /order; reading the key "order" of ListedTexts)",
        "/order");
    expect_rejected<ListedTexts>(
        R"({"at":{"x":0,"y":1}})",
        R"(the value is not one of { "y": 0, "x": 0 }, {"x":1,"y":1} at /at; reading the key "at" of ListedTexts)",
        "/at");
    expect_rejected<ListedTexts>(
        R"({"pair":[2,1]})",
        R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of ListedTexts)", "/pair");
    expect_rejected<ListedTexts>(
        R"({"scale":[2.5]})",
        R"(the value is not one of [1], [2.5, 2.5] at /scale; reading the key "scale" of ListedTexts)", "/scale");
    // an object is no value of the variant: it reads no constant
    expect_rejected<ListedTexts>(
        R"({"mixed":1})", R"(the value is not one of 0, [0], {} at /mixed; reading the key "mixed" of ListedTexts)",
        "/mixed");

    // validate and the checked write compare the text that the write gives
    expect_valid(ListedTexts());
    expect_valid(ListedTexts{.at = {1, 1}, .scale = std::vector{2.5, 2.5}, .mixed = std::vector{0}});
    expect_valid(ListedTexts{.order = {{"b", 1}, {"a", 2}}});
    expect_invalid(
        ListedTexts{.at = {2, 2}},
        R"(the value is not one of { "y": 0, "x": 0 }, {"x":1,"y":1} at /at; reading the key "at" of ListedTexts)",
        "/at");
    expect_invalid(ListedTexts{.pair = {}},
                   R"(the value is not equal to the constant [1, 2] at /pair; reading the key "pair" of ListedTexts)",
                   "/pair");
    expect_invalid(ListedTexts{.scale = std::vector{2.5}},
                   R"(the value is not one of [1], [2.5, 2.5] at /scale; reading the key "scale" of ListedTexts)",
                   "/scale");
    expect_invalid(ListedTexts{.mixed = 3},
                   R"(the value is not one of 0, [0], {} at /mixed; reading the key "mixed" of ListedTexts)", "/mixed");
    expect_invalid(
        ListedTexts{.order = {{"a", 2}}},
        R"(the value is not equal to the constant {"a":2,"b":1} at /order; reading the key "order" of ListedTexts)",
        "/order");

    EXPECT_NO_THROW((void)yyjson::read<ListedDom>(R"({"any":{"b":true,"a":[1.0,2]}})"));
    EXPECT_NO_THROW((void)yyjson::read<ListedDom>(R"({"any":null})"));
    const auto dom = bad_cast_of([] { (void)yyjson::read<ListedDom>(R"({"any":{"a":[1,2],"b":1}})"); });
    EXPECT_EQ(dom.path(), "/any");

    // the text is read when a value is checked first
    try
    {
        (void)yyjson::read<UnreadableText>(R"({"n":1})");
        ADD_FAILURE();
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_TRUE(std::string_view(e.what()).starts_with(
            R"(the JSON text "one" of yyjson::const_ or yyjson::enum_ cannot be read as int: )"))
            << e.what();
    }
}

TEST(Constraints, Format)
{
    expect_accepted<Formatted>(R"({"when":"1985-04-12T23:20:50.52+01:00","mail":"a.b@[IPv6:::1]","code":"AB"})",
                               {"1985-04-12T23:20:50.52+01:00", "a.b@[IPv6:::1]", "AB", std::nullopt});
    expect_accepted<Formatted>(R"({"mail":null})", {});
    // yyjson::format checks nothing
    expect_accepted<Formatted>(R"({"day":"tomorrow"})", {.day = "tomorrow"});
    expect_accepted<Described>(R"({"id":"anything"})", {"anything"});
    expect_rejected<Formatted>(
        R"({"when":"2023-02-29T00:00:00Z"})",
        R"(the string does not have the format "date-time" at /when; reading the key "when" of Formatted)", "/when");
    expect_rejected<Formatted>(
        R"({"when":"2024-01-01T23:59:60+01:00"})",
        R"(the string does not have the format "date-time" at /when; reading the key "when" of Formatted)", "/when");
    expect_rejected<Formatted>(
        R"({"mail":"a..b@example.com"})",
        R"(the string does not have the format "email" at /mail; reading the key "mail" of Formatted)", "/mail");
    expect_rejected<Formatted>(
        R"({"code":"Ab"})",
        R"(the string does not have the format "upper" at /code; reading the key "code" of Formatted)", "/code");
    expect_accepted<Formatted>(R"({"re":"([abc])+\\s+$"})", {.re = R"(([abc])+\s+$)"});
    expect_rejected<Formatted>(
        R"({"re":"^(abc]"})",
        R"(the string does not have the format "regex" at /re; reading the key "re" of Formatted)", "/re");
}

TEST(Constraints, FormatRegexOutOfMemory)
{
    constexpr auto json = R"({"re":"<out of memory>"})";
    EXPECT_THROW((void)yyjson::read<Formatted>(json), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(json).cast<Formatted>(), std::bad_alloc);
    // not passed on to the next alternative of a std::variant
    using variant = std::variant<Formatted, yyjson::value>;
    EXPECT_THROW((void)yyjson::read<variant>(json), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read(json).cast<variant>(), std::bad_alloc);
}

TEST(Constraints, Formats)
{
    using namespace yyjson::detail;
    const auto check = [](std::string_view format, std::string_view s) {
        const auto index = builtin_format_index(format);
        EXPECT_LT(index, std::size(builtin_formats)) << format;
        return builtin_formats[index].second(s);
    };
    // the cases of the JSON-Schema-Test-Suite that the grammars of the formats decide on boundaries
    EXPECT_TRUE(check("date", "2020-02-29") && !check("date", "2021-02-29") && !check("date", "2000-13-01"));
    EXPECT_TRUE(check("time", "23:59:60Z") && check("time", "15:59:60-08:00") && !check("time", "22:59:60Z"));
    EXPECT_TRUE(!check("time", "08:30:06") && !check("time", "08:30:06.Z") && check("time", "08:30:06.123z"));
    EXPECT_TRUE(check("duration", "P4DT12H30M5S") && check("duration", "P2W") && check("duration", "PT1M"));
    EXPECT_TRUE(!check("duration", "P") && !check("duration", "PT") && !check("duration", "P1YT") &&
                !check("duration", "P2D1Y") && !check("duration", "P1Y2W") && !check("duration", "P1Y2D"));
    EXPECT_TRUE(check("ipv4", "192.168.0.1") && !check("ipv4", "087.10.0.1") && !check("ipv4", "256.0.0.0"));
    EXPECT_TRUE(check("ipv6", "::") && check("ipv6", "::ffff:192.168.0.1") && check("ipv6", "1:2:3:4:5:6:7:8"));
    EXPECT_TRUE(!check("ipv6", "1:2:3:4:5:6:7:8:9") && !check("ipv6", "1::2::3") && !check("ipv6", "12345::") &&
                !check("ipv6", "fe80::a%eth1"));
    EXPECT_TRUE(check("hostname", "a--b.com") && !check("hostname", "ab--c.com") && !check("hostname", "xn--X") &&
                !check("hostname", "example.com."));
    EXPECT_TRUE(check("email", "\"joe bloggs\"@example.com") && check("email", "joe@[127.0.0.1]") &&
                !check("email", "joe@[300.0.0.1]") && !check("email", ".joe@example.com"));
    EXPECT_TRUE(check("uri", "urn:oasis:names:specification:docbook:dtd:xml:4.1.2") &&
                check("uri", "http://[2001:db8::7]/c=GB?objectClass?one") && !check("uri", "//foo.bar/?baz=qux") &&
                !check("uri", "http:// shouldfail.com") && !check("uri", "http://a/%zz"));
    EXPECT_TRUE(check("uri-reference", "#fragment") && check("uri-reference", "../a") &&
                !check("uri-reference", "a:b:\\c") && !check("uri-reference", "#frag\\ment"));
    EXPECT_TRUE(check("uri-template", "http://example.com/dictionary/{term:1}/{term}") && check("uri-template", "") &&
                !check("uri-template", "http://example.com/dictionary/{term:1}/{term"));
    EXPECT_TRUE(check("uuid", "2EB8AA08-AA98-11EA-B4AA-73B441D16380") &&
                !check("uuid", "2eb8aa08-aa98-11ea-b4aa73b441d16380"));
    EXPECT_TRUE(check("json-pointer", "") && check("json-pointer", "/~0~1") && !check("json-pointer", "/~2") &&
                !check("json-pointer", "a"));
    EXPECT_TRUE(check("relative-json-pointer", "0#") && check("relative-json-pointer", "1/a") &&
                !check("relative-json-pointer", "01") && !check("relative-json-pointer", "/a"));
}

TEST(Constraints, Strict)
{
    // yyjson::strict checks the keywords as a read without it does
    const auto rejected = [](auto read, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of(read);
        expect_text(message_of(e), message);
        EXPECT_EQ(e.path(), path);
    };
    rejected([] { (void)yyjson::read<Bounded>(R"({"i":11,"d":0.5,"u":3,"o":1})", yyjson::strict); },
             R"(the number is greater than the maximum 10 at /i; reading the key "i" of Bounded)", "/i");
    rejected(
        [] {
            auto t = Texts();
            yyjson::read_into(R"({"name":"a","code":"ABC"})", t, yyjson::strict);
        },
        R"(the string is shorter than the minimum length 2 at /name; reading the key "name" of Texts)", "/name");
    rejected(
        [] {
            auto t = Listed();
            yyjson::merge_into(R"({"version":1})", t, yyjson::strict);
        },
        R"(the value is not equal to the constant 2 at /version; reading the key "version" of Listed)", "/version");
    rejected([] { (void)yyjson::read(R"({"done":-1,"steps":[]})").cast<Progress>(yyjson::strict); },
             "the number is less than the minimum 0 at /done; reading Percent", "/done");
}

// the reader without a document and the cast of a document give the same value or the same error, of which the
// reader without a document leaves an error to the document
TEST(Constraints, Differential)
{
    const auto same = [](auto read, bool accepted, const char* json) {
        const bool direct = read(json);
        if (accepted)
        {
            EXPECT_TRUE(direct) << json;
        }
    };
    const auto bounded = [](const char* json) { return expect_direct_read<Bounded>(json); };
    same(bounded, true, R"({"i":3,"d":0.5,"u":6,"o":2})");
    for (const auto* json : {R"({"i":-1})", R"({"u":7})", R"({"o":-9})", R"({"d":2})", R"({"i":null})"})
        same(bounded, false, json);
    const auto texts = [](const char* json) { return expect_direct_read<Texts>(json); };
    same(texts, true, R"({"name":"ab","code":"QRS"})");
    same(texts, false, R"({"name":"abcd"})");
    same(texts, false, R"({"code":"Q"})");
    // a float with a listed value leaves Listed to the document
    const auto listed = [](const char* json) { return expect_direct_read<Listed>(json); };
    same(listed, false, R"({"unit":"cm","version":2})");
    same(listed, false, R"({"unit":"x"})");
    same(listed, false, R"({"ratio":0.2})");
    const auto progress = [](const char* json) { return expect_direct_read<Progress>(json); };
    same(progress, true, R"({"done":3,"steps":[1,2]})");
    same(progress, false, R"({"done":3,"steps":[1,-2]})");
    const auto outer = [](const char* json) {
        const bool read = expect_direct_read<ConstrainedOuter>(json);
        return expect_read_into(ConstrainedOuter{{{1, 1, 3, {}}}, {}}, json) && read;
    };
    same(outer, true, R"({"items":[{"i":1},{"i":2}]})");
    same(outer, false, R"({"items":[{"i":1},{"u":2}]})");
}

// NOLINTEND
