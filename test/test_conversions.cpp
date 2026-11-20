#include "test_common.hpp"

// NOLINTBEGIN

TEST(Reader, RealToIntegers)
{
    using namespace yyjson;

    // a real number is read into an integer only if it is an integer that fits, which is checked before the
    // conversion
    for (const auto* json : {"1.0", "1e2", "-0.0", "2.5e1"})
    {
        EXPECT_EQ(read(json).cast<int>(), static_cast<int>(std::strtod(json, nullptr))) << json;
        EXPECT_EQ(read<int>(json), static_cast<int>(std::strtod(json, nullptr))) << json;
    }
    EXPECT_EQ(read("255.0").cast<std::uint8_t>(), 255);
    EXPECT_EQ(read("-128.0").cast<std::int8_t>(), -128);
    EXPECT_EQ(read("-9223372036854775808.0").cast<std::int64_t>(), std::numeric_limits<std::int64_t>::min());
    EXPECT_EQ(read("1.8446744073709550e19").cast<std::uint64_t>(), 18446744073709549568ULL);
    EXPECT_EQ(read<std::uint64_t>("1e19"), 10000000000000000000ULL);
    EXPECT_EQ(read("-1.0").cast<std::optional<int>>(), -1);
    EXPECT_EQ(read<std::vector<int>>("[1.0,-2e0,3]"), (std::vector<int>{1, -2, 3}));
    for (const auto* json : {"1.5", "-1.5", "-0.5", "255.9", "1e-1", "1e300", "-1e300"})
    {
        EXPECT_THROW((void)read(json).cast<int>(), bad_cast) << json;
        EXPECT_THROW((void)read<int>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<std::optional<long>>(), bad_cast) << json;
        EXPECT_THROW((void)read<std::optional<long>>(json), bad_cast) << json;
    }
    EXPECT_THROW((void)read("256.0").cast<std::uint8_t>(), bad_cast);
    EXPECT_THROW((void)read("-1.0").cast<std::uint8_t>(), bad_cast);
    EXPECT_THROW((void)read("-129.0").cast<std::int8_t>(), bad_cast);
    EXPECT_THROW((void)read("9223372036854775808.0").cast<std::int64_t>(), bad_cast);
    EXPECT_THROW((void)read("18446744073709551616.0").cast<std::uint64_t>(), bad_cast);
    EXPECT_THROW((void)read<std::uint64_t>("2e19"), bad_cast);
    EXPECT_THROW((void)read("NaN", ReadFlag::AllowInfAndNan).cast<int>(), bad_cast);
    EXPECT_THROW((void)read("-Infinity", ReadFlag::AllowInfAndNan).cast<long>(), bad_cast);
    EXPECT_THROW((void)read<std::vector<int>>("[1.0,-2.5]"), bad_cast);
    EXPECT_THROW((void)read<std::uint16_t>("1e300"), bad_cast);

    // `read<T>` hands an integer part with a fraction of zeros over without parsing the fraction, which must read
    // as the DOM does, beyond 2^53 as well
    for (const auto* json : {"-65.0", "0.000", "-0.0", "7.00e1", "1.000001", "9007199254740993.0",
                             "-9007199254740992.00", "18014398509481985.0", "300.0", "-1.0"})
    {
        for (const auto& text : {std::string(json), std::format("[ {} ,{}]", json, json)})
        {
            using V = std::vector<std::int64_t>;
            const auto expect_same = [&]<typename T>() {
                std::optional<T> dom, direct;
                try { dom = read(text).cast<T>(); } catch (const bad_cast&) {}
                try { direct = read<T>(text); } catch (const bad_cast&) {}
                EXPECT_EQ(direct, dom) << text;
            };
            if (text.front() == '[')
            {
                expect_same.template operator()<V>();
                expect_same.template operator()<std::vector<std::uint8_t>>();
            }
            else
            {
                expect_same.template operator()<std::int64_t>();
                expect_same.template operator()<std::uint8_t>();
            }
        }
    }
    EXPECT_EQ(read<std::int64_t>("9007199254740993.0"), 9007199254740992);
    for (const auto* json : {"65.0e", "65.0.0", "65.0-"}) EXPECT_THROW((void)read<std::int64_t>(json), std::exception);
    try
    {
        (void)read<std::uint8_t>("300.0");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_STREQ(e.what(), "the number is out of range, got 300 (line 1, column 1, pos 0); reading unsigned char");
    }
    try
    {
        (void)read<int>("1.5");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_STREQ(e.what(), "expected an integer, got a number (line 1, column 1, pos 0); reading int");
    }

    // a number is not the size of a container
    EXPECT_THROW((void)read("2").cast<std::vector<int>>(), bad_cast);
    EXPECT_THROW((void)read("-2.5").cast<std::vector<std::string>>(), bad_cast);
    EXPECT_THROW((void)read("true").cast<std::list<int>>(), bad_cast);
    EXPECT_THROW((void)read("1e300").cast<std::optional<std::vector<int>>>(), bad_cast);
    EXPECT_THROW((void)read<std::vector<std::vector<int>>>("[2]"), bad_cast);
}

TEST(Reader, RealToFloat)
{
    using namespace yyjson;

    // a finite number that rounds to infinity as a `float` throws, while one that rounds to the largest `float`,
    // an underflow to zero and an infinity of the text are read
    constexpr auto max = std::numeric_limits<float>::max();
    for (const auto* json : {"3.4028235e38", "3.4028235677973362e38"})
    {
        EXPECT_EQ(read(json).cast<float>(), max) << json;
        EXPECT_EQ(read<float>(json), max) << json;
    }
    EXPECT_EQ(read<float>("-3.4028235e38"), -max);
    EXPECT_EQ(read<float>("1e-50"), 0.0F);
    EXPECT_EQ(read<float>("Infinity", ReadFlag::AllowInfAndNan), std::numeric_limits<float>::infinity());
    EXPECT_EQ(read("-Infinity", ReadFlag::AllowInfAndNan).cast<float>(), -std::numeric_limits<float>::infinity());
    EXPECT_TRUE(std::isnan(read<float>("NaN", ReadFlag::AllowInfAndNan)));
    for (const auto* json : {"3.4028235677973366e38", "1e39", "-1e39", "1e300"})
    {
        EXPECT_THROW((void)read(json).cast<float>(), bad_cast) << json;
        EXPECT_THROW((void)read<float>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<std::optional<float>>(), bad_cast) << json;
    }
    EXPECT_EQ(read<double>("1e39"), 1e39);
    EXPECT_THROW((void)read<std::vector<float>>("[1.5,1e39]"), bad_cast);
    EXPECT_THROW((void)read("[1.5,1e39]").cast<std::vector<float>>(), bad_cast);
    for (const auto* json : {"[1.5,1e39]", "[1.5,2.5]", R"({"f":1e39})", R"({"f":-1.5})"})
    {
        expect_direct_read<std::vector<float>>(json);
        expect_direct_read<std::map<std::string, float>>(json);
    }

    // a variant goes on to the next alternative
    using float_or_double = std::variant<float, double>;
    EXPECT_EQ(read("1e39").cast<float_or_double>(), float_or_double(1e39));
    EXPECT_EQ(read<float_or_double>("1e39"), float_or_double(1e39));
    EXPECT_EQ(read<float_or_double>("1.5"), float_or_double(1.5F));
}

TEST(Reader, NullToStrings)
{
    using namespace yyjson;

    // a JSON null is not a C string, even before C++23 where a string converts from `nullptr`
    const auto null = read("null");
    EXPECT_THROW((void)null.cast<std::string_view>(), bad_cast);
    EXPECT_THROW((void)null.cast<std::string>(), bad_cast);
    EXPECT_EQ(null.cast<std::optional<std::string>>(), std::nullopt);
    EXPECT_EQ(null.cast<std::optional<std::string_view>>(), std::nullopt);
    EXPECT_EQ(null.cast<const char*>(), nullptr);
    EXPECT_EQ(null.cast<std::shared_ptr<std::string>>(), nullptr);
    EXPECT_THROW((void)read<std::string>("null"), bad_cast);
    EXPECT_EQ(read<std::optional<std::string>>("null"), std::nullopt);
    EXPECT_EQ((read<std::vector<std::optional<std::string>>>(R"([null,"a"])")),
              (std::vector<std::optional<std::string>>{std::nullopt, "a"}));
    EXPECT_EQ((null.cast<std::variant<std::string_view, std::optional<int>>>()),
              (std::variant<std::string_view, std::optional<int>>(std::nullopt)));
}

namespace
{
    struct FromString
    {
        std::string s;
        FromString() = default;
        FromString(int) {}
        FromString(std::string str) : s(std::move(str)) {}
    };
    struct FromStringOrNullopt
    {
        bool empty = false;
        FromStringOrNullopt() = default;
        FromStringOrNullopt(std::string) {}
        FromStringOrNullopt(std::nullopt_t) : empty(true) {}
    };
    struct FromPointer
    {
        const int* p = &p_default;
        static constexpr int p_default = 0;
        FromPointer(const int* q) : p(q) {}
        FromPointer(std::string) {}
    };
    struct FromBoolOrString
    {
        FromBoolOrString() = default;
        FromBoolOrString(bool) {}
        FromBoolOrString(std::string) {}
    };
    struct FromNullOnly
    {
        bool null = false;
        FromNullOnly() = default;
        template <std::same_as<std::nullptr_t> N>
        FromNullOnly(N) : null(true)
        {
        }
    };
    struct WithFromString
    {
        FromBoolOrString f;
    };
}  // namespace

TEST(Reader, NullToClassesFromStrings)
{
    using namespace yyjson;

    // `nullptr` converts to a class through `std::string` only as a C string, which JSON null is not
    const auto null = read("null");
    EXPECT_THROW((void)null.cast<FromString>(), bad_cast);
    EXPECT_THROW((void)read<FromString>("null"), bad_cast);
    EXPECT_EQ(read<FromString>(R"("a")").s, "a");
    EXPECT_TRUE(null.cast<FromStringOrNullopt>().empty);
    EXPECT_TRUE(read<FromStringOrNullopt>("null").empty);
    EXPECT_EQ(null.cast<FromPointer>().p, nullptr);
    EXPECT_EQ(read<FromPointer>("null").p, nullptr);
    EXPECT_THROW((void)null.cast<FromBoolOrString>(), bad_cast);
    EXPECT_THROW((void)read<FromBoolOrString>("null"), bad_cast);
    EXPECT_THROW((void)read(R"({"f":null})").cast<WithFromString>(), bad_cast);
    EXPECT_THROW((void)read<WithFromString>(R"({"f":null})"), bad_cast);
    EXPECT_TRUE(null.cast<FromNullOnly>().null);
    EXPECT_TRUE(read<FromNullOnly>("null").null);
}

namespace
{
    struct SingleField
    {
        int a = 0;
    };
    struct StringFirst
    {
        std::string s;
        int n = 0;
    };
}  // namespace

TEST(Reader, ValueKinds)
{
    using namespace yyjson;

    // a struct is read from a JSON object only, although C++20 initializes its first field by `T(v)`
    for (const auto* json : {"5", "2.5", "1e300", "-1", "\"x\"", "null", "true"})
    {
        EXPECT_THROW((void)read(json).cast<SingleField>(), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<StringFirst>(), bad_cast) << json;
    }
    EXPECT_EQ(read(R"({"a":5})").cast<SingleField>().a, 5);
    for (const auto* json : {"[5]", "[2.5]", "[1e300]", R"(["x"])", "[null]"})
    {
        expect_direct_read<std::vector<SingleField>>(json);
        expect_direct_read<std::vector<StringFirst>>(json);
    }
    EXPECT_TRUE(expect_direct_read<std::vector<SingleField>>(R"([{"a":5}])"));
    EXPECT_EQ((read("5").cast<std::variant<SingleField, int>>().index()), 1U);
    EXPECT_EQ((read(R"("x")").cast<std::variant<StringFirst, std::string>>().index()), 1U);

    // bool is read from true and false only, although C++ converts numbers, pointers and null to it
    for (const auto* json : {"0", "1", "2.5", "\"\"", "\"false\"", "null"})
        EXPECT_THROW((void)read(json).cast<bool>(), bad_cast) << json;
    EXPECT_TRUE(read("true").cast<bool>());
    EXPECT_FALSE(read("false").cast<bool>());
    EXPECT_TRUE(expect_direct_read<std::list<bool>>("[true,false]"));
    // `std::vector<bool>`, whose proxy reference is not assignable through a const reference in C++20
    EXPECT_EQ(read("[true,false,true]").cast<std::vector<bool>>(), (std::vector<bool>{true, false, true}));
    EXPECT_TRUE(expect_direct_read<std::vector<bool>>("[true,false,true]") || YYJSON_VERSION_HEX < 0x000C00);
    EXPECT_TRUE(expect_direct_read<std::vector<bool>>("[]") || YYJSON_VERSION_HEX < 0x000C00);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<bool>>>("[[true],[],[false,true]]") ||
                YYJSON_VERSION_HEX < 0x000C00);
    for (const auto* json : {"[0]", "[1]", "[0.5]", R"(["false"])", "[null]"})
    {
        expect_direct_read<std::list<bool>>(json);
        expect_direct_read<std::vector<bool>>(json);
    }
    EXPECT_EQ((read("1").cast<std::variant<bool, int>>().index()), 1U);
    EXPECT_EQ(read("null").cast<std::optional<bool>>(), std::nullopt);

    // nor is a number read from true and false, nor a class that is constructed from a number through `bool`, or
    // from `bool` through a number
    struct Flag
    {
        bool b;
        Flag(bool v) : b(v) {}
    };
    struct Count
    {
        int n;
        Count(int v) : n(v) {}
    };
    // `std::complex` would do in C++20, but C++26 makes it tuple-like, read from `[re, im]`
    struct Scale
    {
        double x;
        Scale(double v) : x(v) {}
    };
    for (const auto* json : {"true", "false"})
    {
        EXPECT_THROW((void)read(json).cast<int>(), bad_cast) << json;
        EXPECT_THROW((void)read<double>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<std::optional<float>>(), bad_cast) << json;
        EXPECT_THROW((void)read<Count>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<std::chrono::seconds>(), bad_cast) << json;
        EXPECT_THROW((void)read<Scale>(json), bad_cast) << json;
        expect_direct_read<std::vector<std::int64_t>>("[" + std::string(json) + "]");
    }
    EXPECT_TRUE(read<Flag>("true").b);
    EXPECT_FALSE(read("false").cast<Flag>().b);
    EXPECT_THROW((void)read<Flag>("1"), bad_cast);
    EXPECT_THROW((void)read("0.5").cast<Flag>(), bad_cast);
    EXPECT_EQ(read<Count>("2").n, 2);
    EXPECT_EQ(read("3").cast<std::chrono::seconds>(), std::chrono::seconds(3));
    // a duration reads only the integers that its count holds
    try
    {
        (void)read<std::chrono::seconds>("18446744073709551615");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_EQ(e.kind(), error_kind::out_of_range);
    }
    EXPECT_THROW((void)read("-1").cast<std::chrono::duration<unsigned>>(), bad_cast);
    EXPECT_THROW((void)read<std::vector<std::chrono::duration<std::int8_t>>>("[1,128]"), bad_cast);
    EXPECT_EQ(read<std::chrono::duration<std::int8_t>>("-128").count(), -128);
    EXPECT_EQ(read<Scale>("1.5").x, 1.5);
    EXPECT_EQ((read<std::variant<int, bool>>("true")), (std::variant<int, bool>(true)));
    EXPECT_EQ((read("true").cast<std::variant<double, bool>>().index()), 1U);

    // a string is read from a JSON string only, while another range of characters is read from an array
    EXPECT_THROW((void)read("[104,105]").cast<std::string>(), bad_cast);
    expect_direct_read<std::vector<std::string>>("[[104,105]]");
    EXPECT_EQ(read("[104,105]").cast<std::vector<char>>(), (std::vector<char>{'h', 'i'}));

    // `std::optional<T>` is read from null or from what T is read from, also outside of a struct
    EXPECT_THROW((void)read("4294967297").cast<std::optional<int>>(), bad_cast);
    EXPECT_THROW((void)read("-1").cast<std::optional<unsigned>>(), bad_cast);
    EXPECT_EQ(read("[1,2]").cast<std::optional<std::vector<int>>>(), (std::vector<int>{1, 2}));
    EXPECT_EQ(read("null").cast<std::optional<std::vector<int>>>(), std::nullopt);
    EXPECT_EQ(read(R"({"a":3})").cast<std::optional<SingleField>>()->a, 3);
    const auto pointer = read("null").cast<std::optional<std::shared_ptr<int>>>();
    ASSERT_TRUE(pointer.has_value());
    EXPECT_EQ(*pointer, nullptr);
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<std::vector<int>>>>("[null,[1,2],[]]"));
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<SingleField>>>(R"([null,{"a":3}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<std::shared_ptr<int>>>>("[null,1]"));
    expect_direct_read<std::vector<std::optional<std::vector<int>>>>(R"([[1],"x"])");
    expect_direct_read<std::vector<std::optional<int>>>("[4294967297]");
    EXPECT_EQ((read("[1]").cast<std::variant<int, std::optional<std::vector<int>>>>().index()), 1U);
    EXPECT_EQ((read("null").cast<std::variant<int, std::optional<std::vector<int>>>>().index()), 1U);
}

template <>
struct yyjson::caster<std::array<std::int8_t, 2>>
{
    template <typename Json>
    static std::array<std::int8_t, 2> from_json(const Json& json)
    {
        const auto arr = json.as_array();
        if (!arr || arr->size() != 2) throw yyjson::bad_cast("not a pair");
        return {cast<std::int8_t>((*arr)[1]), cast<std::int8_t>((*arr)[0])};
    }
};

TEST(Reader, RangesOfFixedSizeRanges)
{
    using namespace yyjson;
    using points = std::vector<std::array<double, 2>>;

    // the elements are filled where they are stored, and a JSON array of another length throws
    EXPECT_EQ(read("[[1,2.5],[3,4]]").cast<points>(), (points{{1.0, 2.5}, {3.0, 4.0}}));
    for (const auto* json : {"[[1,2,3]]", "[[1],[]]", "[[]]", "[1]", "[null]", R"([[1,"x"]])", R"([{"a":1}])"})
        EXPECT_THROW((void)read(json).cast<points>(), bad_cast) << json;
    for (const auto* json : {"[]", "[[1,2.5],[3,4]]", "[[1],[]]", "[[1,2,3]]", "[1]", "[null]", R"([[1,"x"]])"})
    {
        expect_direct_read<points>(json);
        expect_direct_read<std::list<std::array<int, 3>>>(json);
        expect_direct_read<std::vector<std::array<std::array<int, 2>, 2>>>(json);
    }
    EXPECT_TRUE(expect_direct_read<points>("[[1,2.5],[3,4]]"));
    EXPECT_TRUE((expect_direct_read<std::list<std::array<int, 3>>>("[[1,2,3],[4,5,6]]")));
    using nested = std::vector<std::array<std::array<int, 2>, 2>>;
    EXPECT_EQ(read("[[[1,2],[3,4]]]").cast<nested>(), (nested{{{{1, 2}, {3, 4}}}}));
    EXPECT_THROW((void)read("[[[1,2],[3]]]").cast<nested>(), bad_cast);
    EXPECT_THROW((void)yyjson::read<nested>("[[[1,2],[3]]]"), bad_cast);
    EXPECT_THROW((void)yyjson::read<nested>("[[[1,2]]]"), bad_cast);

    // the size of the type decides how the elements are read, not the length of the JSON array
    auto long_json = std::string("[[");
    for (auto i = 0; i < 20; ++i) long_json += std::format("{}{}", i == 0 ? "" : ",", i);
    long_json += "]]";
    const auto long_arrays = read(long_json).cast<std::vector<std::array<int, 20>>>();
    ASSERT_EQ(long_arrays.size(), 1U);
    EXPECT_EQ(long_arrays[0][19], 19);
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 20>>>(long_json)));
    expect_direct_read<std::vector<std::array<int, 17>>>(long_json);

    // a caster of the element type is still used
    using pairs = std::vector<std::array<std::int8_t, 2>>;
    EXPECT_EQ(read("[[1,2],[3,4]]").cast<pairs>(), (pairs{{2, 1}, {4, 3}}));
    EXPECT_THROW((void)read("[[1]]").cast<pairs>(), bad_cast);
    expect_direct_read<pairs>("[[1,2],[3,4]]");

    // a `std::array` of two strings is a key-value pair, written as a member of a JSON object, and is read back
    // from one as well as from an array
    using string_pairs = std::vector<std::array<std::string, 2>>;
    const auto abcd = string_pairs{{"a", "b"}, {"c", "d"}};
    EXPECT_EQ(std::string_view(value(abcd).write()), R"({"a":"b","c":"d"})");
    EXPECT_EQ(read(R"({"a":"b","c":"d"})").cast<string_pairs>(), abcd);
    EXPECT_EQ(read(R"([["a","b"],["c","d"]])").cast<string_pairs>(), abcd);
    EXPECT_EQ((read(R"({"c":"d","a":"b"})").cast<std::set<std::array<std::string, 2>>>()),
              (std::set<std::array<std::string, 2>>(abcd.begin(), abcd.end())));
    EXPECT_THROW((void)read(R"({"a":1})").cast<string_pairs>(), bad_cast);
    for (const auto* json : {R"({"a":"b","c":"d"})", R"([["a","b"],["c","d"]])", R"({"a":1})", R"({"a\n":"\u00e9"})"})
        expect_direct_read<string_pairs>(json);
    EXPECT_TRUE(expect_direct_read<string_pairs>(R"({"a\n":"\u00e9","c":"d"})"));
    EXPECT_TRUE(expect_direct_read<string_pairs>(R"([["a","b"],["c","d"]])"));
    EXPECT_THROW((void)yyjson::read<string_pairs>(R"([["a","b"],["c"]])"), bad_cast);
    expect_direct_read<string_pairs>(R"([["a","b"],["c"]])");
    EXPECT_TRUE((expect_direct_read<std::set<std::array<std::string, 2>>>(R"({"c":"d","a":"b"})")));
}

enum class PairSide
{
    left,
    right
};
struct FixedPairs
{
    std::array<std::pair<std::string, int>, 2> kv{};
    std::array<std::pair<std::string, std::set<int>>, 2> sets{};
    bool operator==(const FixedPairs&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FixedPairs> =
    yyjson::annotate(yyjson::field("kv", yyjson::property_names(yyjson::max_length(1))),
                     yyjson::field("sets", yyjson::additional_properties(yyjson::min_items(2))));

TEST(Reader, FixedSizeRangesOfPairs)
{
    using namespace yyjson;
    using pairs = std::array<std::pair<std::string, int>, 2>;
    const auto ab = pairs{{{"a", 1}, {"b", 2}}};

    // written as a JSON object, and read back from one in its order as well as from an array of pairs
    EXPECT_EQ(yyjson::write(ab), R"({"a":1,"b":2})");
    EXPECT_EQ(yyjson::read<pairs>(yyjson::write(ab)), ab);
    EXPECT_EQ(read(R"({"a":1,"b":2})").cast<pairs>(), ab);
    EXPECT_EQ(yyjson::read<pairs>(R"([["a",1],["b",2]])"), ab);
    EXPECT_EQ(yyjson::read<pairs>(R"({"b":2,"a":1})"), (pairs{{{"b", 2}, {"a", 1}}}));
    for (const auto* json : {R"({"a":1,"b":2})", R"({"a\n":1,"b":2})", R"([["a",1],["b",2]])"})
        EXPECT_TRUE(expect_direct_read<pairs>(json)) << json;

    // an object of another number of members throws, as an array of another length does
    for (const auto* json : {R"({"a":1})", "{}", R"({"a":1,"b":2,"c":3})", R"({"a":"x","b":1})", R"([["a",1]])"})
    {
        EXPECT_THROW((void)yyjson::read<pairs>(json), bad_cast) << json;
        expect_direct_read<pairs>(json);
    }
    try
    {
        (void)read(R"({"a":1})").cast<pairs>();
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_EQ(std::string_view(e.what()),
                  std::format("expected an object of 2 members; reading {}", yyjson::detail::type_name<pairs>()));
    }

    // a repeated key is kept, as in a `std::vector` of pairs, unless `yyjson::reject_duplicates` is given
    EXPECT_EQ(yyjson::read<pairs>(R"({"a":1,"a":2})"), (pairs{{{"a", 1}, {"a", 2}}}));
    EXPECT_THROW((void)yyjson::read<pairs>(R"({"a":1,"a":2})", reject_duplicates), bad_cast);
    EXPECT_THROW((void)read(R"({"a":1,"a":2})").cast<pairs>(reject_duplicates), bad_cast);

    // enumeration keys, and nested ranges
    using sides = std::array<std::pair<PairSide, int>, 2>;
    EXPECT_EQ(yyjson::read<sides>(R"({"right":1,"left":2})"), (sides{{{PairSide::right, 1}, {PairSide::left, 2}}}));
    EXPECT_THROW((void)yyjson::read<sides>(R"({"up":1,"left":2})"), bad_cast);
    EXPECT_TRUE(expect_direct_read<sides>(R"({"right":1,"left":2})"));
    using nested = std::array<std::pair<std::string, std::array<int, 2>>, 2>;
    EXPECT_EQ(yyjson::read<nested>(R"({"a":[1,2],"b":[3,4]})"), (nested{{{"a", {1, 2}}, {"b", {3, 4}}}}));
    EXPECT_THROW((void)yyjson::read<nested>(R"({"a":[1,2],"b":[3]})"), bad_cast);
    EXPECT_TRUE(expect_direct_read<nested>(R"({"a":[1,2],"b":[3,4]})"));
    using ranges = std::vector<pairs>;
    const auto json = R"([{"a":1,"b":2},[["a",1],["b",2]]])";
    EXPECT_EQ(yyjson::read<ranges>(json), (ranges{ab, ab}));
    EXPECT_TRUE(expect_direct_read<ranges>(json));
    EXPECT_TRUE(expect_read_into(ranges(3), json));
    EXPECT_TRUE(expect_read_into(ab, R"({"c":3,"d":4})"));
    using string_pairs = std::array<std::array<std::string, 2>, 1>;
    EXPECT_EQ(yyjson::read<string_pairs>(R"({"a":"b"})"), (string_pairs{{{"a", "b"}}}));

    // a key that is not a string is written as an array of pairs, and is not read from an object
    using int_pairs = std::array<std::pair<int, int>, 1>;
    EXPECT_EQ(yyjson::write(int_pairs{{{1, 2}}}), "[[1,2]]");
    EXPECT_THROW((void)yyjson::read<int_pairs>(R"({"1":2})"), bad_cast);

    // the keywords of objects constrain the members that are read
    EXPECT_EQ(yyjson::read<FixedPairs>(R"({"kv":{"a":1,"b":2},"sets":{"a":[1,1],"b":[2,3]}})").sets[0].second,
              (std::set<int>{1}));
    for (const auto* text : {R"({"kv":{"a":1,"b":2},"sets":{"a":[1,1],"b":[2,3]}})", R"({"kv":{"ab":1,"b":2}})",
                             R"({"sets":{"a":[1],"b":[2,3]}})", R"({"sets":{"a":[1,2]}})"})
        expect_direct_read<FixedPairs>(text);
    EXPECT_THROW((void)yyjson::read<FixedPairs>(R"({"kv":{"ab":1,"b":2}})"), bad_cast);
    EXPECT_THROW((void)yyjson::read<FixedPairs>(R"({"sets":{"a":[1],"b":[2,3]}})"), bad_cast);
    auto fixed = FixedPairs{{{{"a", 1}, {"b", 2}}}, {{{"a", {1, 2}}, {"b", {3, 4}}}}};
    EXPECT_NO_THROW(yyjson::validate(fixed));
    fixed.kv[0].first = "ab";
    EXPECT_THROW(yyjson::validate(fixed), bad_cast);
}

struct PairArrays
{
    std::pair<std::string, int> kv[2]{};
    std::pair<std::string, std::set<int>> sets[2]{};
    std::pair<std::string, int> grid[2][2]{};
    bool operator==(const PairArrays&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PairArrays> =
    yyjson::annotate(yyjson::field("kv", yyjson::property_names(yyjson::max_length(1))),
                     yyjson::field("sets", yyjson::additional_properties(yyjson::min_items(2))));
struct PairArrayList
{
    std::vector<PairArrays> items;
    bool operator==(const PairArrayList&) const = default;
};

TEST(Reader, CArraysOfPairs)
{
    using namespace yyjson;
    auto ab =
        PairArrays{{{"a", 1}, {"b", 2}}, {{"a", {1, 2}}, {"b", {3, 4}}}, {{{"a", 1}, {"b", 2}}, {{"c", 3}, {"d", 4}}}};

    // a C array of pairs is written as a JSON object, and read from one as a `std::array` of pairs is
    const auto json = std::string(yyjson::write(ab));
    EXPECT_EQ(json, R"({"kv":{"a":1,"b":2},"sets":{"a":[1,2],"b":[3,4]},"grid":[{"a":1,"b":2},{"c":3,"d":4}]})");
    EXPECT_EQ(yyjson::read<PairArrays>(json), ab);
    EXPECT_EQ(read(json).cast<PairArrays>(), ab);
    EXPECT_EQ(yyjson::read<PairArrays>(R"({"kv":[["a",1],["b",2]],"sets":[["a",[1,2]],["b",[3,4]]],)"
                                       R"("grid":[[["a",1],["b",2]],{"c":3,"d":4}]})"),
              ab);
    EXPECT_TRUE(expect_direct_read<PairArrays>(json));
    EXPECT_TRUE(expect_read_into(PairArrays(), json));
    EXPECT_TRUE(expect_read_into(ab, R"({"kv":{"c":3,"d":4}})"));
    const auto list = PairArrayList{{ab, ab}};
    EXPECT_EQ(yyjson::read<PairArrayList>(yyjson::write(list)), list);
    EXPECT_TRUE(expect_direct_read<PairArrayList>(std::string(yyjson::write(list))));

    // an object of another number of members throws, as an array of another length does
    for (const auto* text :
         {R"({"kv":{"a":1}})", R"({"kv":{}})", R"({"kv":{"a":1,"b":2,"c":3}})", R"({"grid":[{"a":1,"b":2},{"c":3}]})"})
    {
        EXPECT_THROW((void)yyjson::read<PairArrays>(text), bad_cast) << text;
        EXPECT_THROW((void)yyjson::read<PairArrayList>(std::string(R"({"items":[)") + text + "]}"), bad_cast) << text;
        expect_direct_read<PairArrays>(text);
    }
    try
    {
        (void)read(R"({"kv":{"a":1}})").cast<PairArrays>();
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_EQ(std::string_view(e.what()), std::format("expected an object of 2 members at /kv; reading {}",
                                                          yyjson::detail::type_name<std::pair<std::string, int>[2]>()));
    }

    // a repeated key is kept unless `yyjson::reject_duplicates` is given, which validate gives as well
    EXPECT_EQ(yyjson::read<PairArrays>(R"({"kv":{"a":1,"a":2}})").kv[1], (std::pair<std::string, int>{"a", 2}));
    EXPECT_THROW((void)yyjson::read<PairArrays>(R"({"kv":{"a":1,"a":2}})", reject_duplicates), bad_cast);
    EXPECT_THROW((void)read(R"({"kv":{"a":1,"a":2}})").cast<PairArrays>(reject_duplicates), bad_cast);

    // the keywords of objects constrain the members that are read, and the positions are the keys
    for (const auto* text : {R"({"kv":{"ab":1,"b":2}})", R"({"sets":{"a":[1],"b":[2,3]}})"})
    {
        EXPECT_THROW((void)yyjson::read<PairArrays>(text), bad_cast) << text;
        expect_direct_read<PairArrays>(text);
    }
    EXPECT_EQ(yyjson::read<PairArrays>(R"({"sets":{"a":[1,1],"b":[2,3]}})").sets[0].second, (std::set<int>{1}));
    EXPECT_NO_THROW(yyjson::validate(ab));
    ab.kv[0].first = "ab";
    try
    {
        yyjson::validate(ab);
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_TRUE(std::string_view(e.what()).find(" at /kv/ab") != std::string_view::npos) << e.what();
    }
    ab.kv[0].first = "b";
    EXPECT_THROW(yyjson::validate(ab), bad_cast);

    // a key that is not a string is written as an array of pairs, and is not read from an object
    struct IntPairs
    {
        std::pair<int, int> c[1]{};
    };
    EXPECT_EQ(yyjson::write(IntPairs{{{1, 2}}}), R"({"c":[[1,2]]})");
    EXPECT_THROW((void)yyjson::read<IntPairs>(R"({"c":{"1":2}})"), bad_cast);
}

struct SetFields
{
    std::set<int> s;
    std::unordered_set<std::string> u;
    std::optional<std::multiset<int>> m;
    bool operator==(const SetFields&) const = default;
};

TEST(Reader, Sets)
{
    using namespace yyjson;

    // a set is written as a JSON array and read back from one, keeping the duplicates only in a multiset
    EXPECT_EQ(std::string_view(value(std::set<int>{3, 1, 2}).write()), "[1,2,3]");
    EXPECT_EQ(read("[3,1,2,1]").cast<std::set<int>>(), (std::set<int>{1, 2, 3}));
    EXPECT_EQ(read("[3,1,2,1]").cast<std::multiset<int>>(), (std::multiset<int>{1, 1, 2, 3}));
    EXPECT_EQ(read("[3,1,2,1]").cast<std::unordered_set<int>>(), (std::unordered_set<int>{1, 2, 3}));
    EXPECT_EQ(read(R"(["b","a"])").cast<std::set<std::string>>(), (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(read("[]").cast<std::set<int>>(), std::set<int>());
    for (const auto* json : {R"([1,"x"])", R"({"a":1})", "1", "null", "[[1]]"})
        EXPECT_THROW((void)read(json).cast<std::set<int>>(), bad_cast) << json;

    // the reader without a document gives the same, and reads every valid text itself
    for (const auto* json : {"[]", "[3,1,2,1]", "[1,2,3]"})
    {
        EXPECT_TRUE(expect_direct_read<std::set<int>>(json));
        EXPECT_TRUE(expect_direct_read<std::multiset<int>>(json));
        EXPECT_TRUE(expect_direct_read<std::unordered_multiset<int>>(json));
    }
    for (const auto* json : {R"([1,"x"])", R"({"a":1})", "1", "null"})
    {
        expect_direct_read<std::set<int>>(json);
        expect_direct_read<std::multiset<int>>(json);
        expect_direct_read<std::unordered_multiset<int>>(json);
    }
    EXPECT_TRUE(expect_direct_read<std::unordered_set<std::string>>(R"(["b","a","b\n"])"));
    EXPECT_TRUE(expect_direct_read<std::set<double>>("[2.5,1e300,-0.0]"));

    // nested in ranges, maps, optionals, structs and variants
    using nested = std::vector<std::set<int>>;
    EXPECT_EQ(read("[[2,1],[],[3,3]]").cast<nested>(), (nested{{1, 2}, {}, {3}}));
    EXPECT_TRUE(expect_direct_read<nested>("[[2,1],[],[3,3]]"));
    EXPECT_TRUE(expect_direct_read<std::set<std::vector<int>>>("[[2,1],[],[3,3]]"));
    EXPECT_TRUE(expect_direct_read<std::set<std::set<int>>>("[[2,1],[],[3,3]]"));
    for (const auto* json : {"[[1],2]", "[[1],null]"})
    {
        expect_direct_read<nested>(json);
        expect_direct_read<std::set<std::vector<int>>>(json);
        expect_direct_read<std::set<std::set<int>>>(json);
    }
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::set<int>>>(R"({"a":[2,1],"b":[]})")));
    EXPECT_TRUE(expect_direct_read<std::optional<std::set<int>>>("[2,1]"));
    expect_direct_read<std::optional<std::set<int>>>("null");

    const auto fields = SetFields{{1, 2}, {"a"}, std::multiset<int>{1, 1}};
    const auto fields_json = std::string(value(fields).write());
    EXPECT_EQ(read(fields_json).cast<SetFields>(), fields);
    EXPECT_TRUE(expect_direct_read<SetFields>(fields_json));
    EXPECT_TRUE(expect_direct_read<SetFields>(R"({"s":[2,1,2],"u":[],"m":null})"));
    expect_direct_read<SetFields>(R"({"s":{}})");

    using set_or_int = std::variant<int, std::set<int>>;
    EXPECT_EQ(read("[2,1]").cast<set_or_int>(), set_or_int(std::set<int>{1, 2}));
    EXPECT_EQ(read("2").cast<set_or_int>(), set_or_int(2));
    EXPECT_TRUE(expect_direct_read<set_or_int>("[2,1]"));
    expect_direct_read<set_or_int>("2");
    expect_direct_read<set_or_int>(R"(["x"])");
}

// NOLINTEND
