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
        EXPECT_STREQ(e.what(), "overflow detected: unsigned char is not constructible from JSON number 300");
    }
    try
    {
        (void)read<int>("1.5");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        EXPECT_STREQ(e.what(), "int is not constructible from JSON number 1.5, which is not an integer");
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

// The index and the JSON text of a variant cast from `json`, or nothing when the cast fails.
template <typename V, typename Json>
std::optional<std::pair<std::size_t, std::string>> cast_variant(const Json& json)
{
    try
    {
        const auto v = yyjson::cast<V>(json);
        return std::pair(v.index(), std::string(yyjson::value(v).write(yyjson::WriteFlag::AllowInfAndNan)));
    }
    catch (const yyjson::bad_cast&)
    {
        return std::nullopt;
    }
}

// The same as `cast_variant`, by trying every alternative in order as the variant caster did before
// it skipped the alternatives that do not accept the kind of the JSON value.
template <typename V, typename Json>
std::optional<std::pair<std::size_t, std::string>> cast_variant_trying_all(const Json& json)
{
    auto result = std::optional<V>();
    [&]<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
        static_cast<void>(((result = [&]() -> std::optional<V> {
                               try
                               {
                                   return V(yyjson::cast<Ts>(json));
                               }
                               catch (...)
                               {
                                   return std::nullopt;
                               }
                           }())
                               .has_value() ||
                           ...));
    }(std::type_identity<V>{});
    if (!result.has_value()) return std::nullopt;
    return std::pair(result->index(), std::string(yyjson::value(*result).write(yyjson::WriteFlag::AllowInfAndNan)));
}

template <typename... Vs>
void expect_variants_as_trying_all(std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
{
    const auto doc = yyjson::read(json, flag);
    const auto arr = *doc.as_array();
    for (const auto& e : arr)
    {
        const auto copy = yyjson::value(e);
        const auto element = "[" + std::string(e.write(yyjson::WriteFlag::AllowInfAndNan)) + "]";
        (
            [&] {
                EXPECT_EQ(cast_variant<Vs>(e), cast_variant_trying_all<Vs>(e))
                    << yyjson::detail::type_name<Vs>() << " from " << e.write();
                EXPECT_EQ(cast_variant<Vs>(copy), cast_variant_trying_all<Vs>(copy))
                    << yyjson::detail::type_name<Vs>() << " from a copy of " << e.write();
                // the direct reader reads text without the flags that change the JSON values
                if constexpr (!yyjson::detail::reads_borrowed_strings<Vs>)
                    if (flag == yyjson::ReadFlag::NoFlag) expect_direct_read<std::vector<Vs>>(element);
            }(),
            ...);
    }
}

TEST(Reader, VariantSkipsAlternatives)
{
    using namespace yyjson;
    using yyjson::detail::accepted_json_kinds;
    using yyjson::reader::const_value_ref;
    using namespace yyjson::detail;

    // the kinds of JSON values from which a type may be cast
    // an integer reads a real only if it is an integer, and neither reads true or false
    static_assert(accepted_json_kinds<int, const_value_ref>() ==
                  (json_kind_uint | json_kind_sint | json_kind_integral_real));
    static_assert(accepted_json_kinds<const double, const_value_ref>() == json_kind_number);
    static_assert(accepted_json_kinds<std::nullptr_t, const_value_ref>() == json_kind_null);
    static_assert(accepted_json_kinds<std::array<int, 2>, const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::map<std::string, int>, const_value_ref>() == json_kind_object);
    static_assert(accepted_json_kinds<std::shared_ptr<std::array<int, 2>>, const_value_ref>() ==
                  (json_kind_null | json_kind_array));
    static_assert(accepted_json_kinds<std::variant<std::array<int, 2>, std::map<std::string, int>>,
                                      const_value_ref>() == (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<std::pair<int, int>, const_value_ref>() == (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<ReadPoint, const_value_ref>() == json_kind_all);
    static_assert(accepted_json_kinds<std::array<int, 2>, writer::const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::array<int, 2>, reader::const_array_ref>() == json_kind_all);

    // the alternative that a variant holds is the same as when every alternative is tried in order
    constexpr auto values = std::string_view(
        R"([null,true,false,0,1,-1,300,-300,1.5,-2.5,2.0,-3e2,1e300,18446744073709551615,-9223372036854775808,)"
        R"("","s","1",[],[1],[1,2],[1.5,"a"],["a","b"],[[1,2]],{},{"a":1},{"x":1,"y":2},{"a":1,"b":2.0,"c":"x"}])");
    expect_variants_as_trying_all<
        std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
        std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
        std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
        std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
        std::variant<std::shared_ptr<int>, std::vector<std::string>>, std::variant<ReadRecord, int>,
        std::variant<ReadPoint, int, std::vector<double>>,
        std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
        std::variant<std::nullptr_t, std::string_view, float>,
        std::variant<std::array<int, 2>, std::map<std::string, int>, std::string_view>,
        std::variant<std::int8_t, std::uint64_t, double>, std::variant<int, std::optional<double>>>(values);
    expect_variants_as_trying_all<std::variant<std::vector<int>, std::string, double>,
                                  std::variant<std::nullptr_t, bool, std::string>>(values, ReadFlag::NumberAsRaw);

    using int_or_double = std::variant<std::array<int, 2>, int, double>;
    EXPECT_EQ(read(R"([1.5, 2.0])").cast<std::vector<int_or_double>>(), (std::vector<int_or_double>{1.5, 2}));
    using array_or_map = std::variant<std::array<int, 2>, std::map<std::string, int>>;
    EXPECT_THROW((void)read("1").cast<array_or_map>(), bad_cast);
}

TEST(Reader, FixedSizeShorterJson)
{
    using namespace yyjson;

    // a JSON array or object with fewer values than a tuple, a pair or a `std::array` has elements throws, as one
    // with more does, instead of leaving the rest value-initialized
    const auto doc = read(R"({"t":[1],"u":[],"v":{"a":1},"w":{},"y":5})");
    const auto obj = *doc.as_object();
    using pairs = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    EXPECT_THROW((void)(cast<std::pair<int, int>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<int, int, std::string>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<double, std::string>>(obj["u"])), bad_cast);
    EXPECT_THROW((void)(cast<pairs>(obj["v"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<std::pair<std::string, int>>>(obj["w"])), bad_cast);
    EXPECT_THROW((void)(cast<std::array<int, 2>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::array<int, 1>>(obj["u"])), bad_cast);
    EXPECT_EQ((cast<std::tuple<>>(obj["u"])), std::tuple<>());
    EXPECT_EQ((cast<std::array<int, 0>>(obj["u"])), (std::array<int, 0>{}));
    EXPECT_EQ((cast<std::tuple<>>(obj["w"])), std::tuple<>());
    EXPECT_EQ((cast<std::tuple<int>>(obj["t"])), std::tuple{1});

    struct Fields
    {
        std::pair<int, int> t;
        int y;
    };
    for (const auto* json : {R"({"t":[1],"y":5})", R"({"t":[],"y":5})", R"({"t":[1,2,3],"y":5})"})
    {
        EXPECT_THROW((void)read(json).cast<Fields>(), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read<Fields>(json), bad_cast) << json;
        expect_direct_read<Fields>(json);
    }
    for (const auto* json : {"[1]", "[]", R"({"a":1})", "{}"})
    {
        EXPECT_THROW((void)(yyjson::read<std::pair<int, int>>(json)), bad_cast) << json;
        EXPECT_THROW((void)(yyjson::read<std::array<int, 2>>(json)), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read<pairs>(json), bad_cast) << json;
        expect_direct_read<std::pair<int, int>>(json);
        expect_direct_read<std::array<int, 2>>(json);
        expect_direct_read<pairs>(json);
    }
    EXPECT_TRUE(expect_direct_read<Fields>(R"({"t":[1,2],"y":5})"));
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 2>>>("[[1,2],[3,4]]")));
}

namespace
{
    // The direct reader reads a variant in a range to the same alternative as the cast from a document, and
    // whenever the cast succeeds unless the value is one that the reader leaves to the document.
    template <typename... Vs>
    void expect_direct_variants(const std::vector<std::string>& values, bool supported = true)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
            {
                (
                    [&] {
                        const auto direct = expect_direct_read<std::vector<Vs>>(json);
                        if (supported && reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Vs>>{}))
                                << json << " as " << yyjson::detail::type_name<Vs>();
                        }
                    }(),
                    ...);
            }
        }
    }
}  // namespace

TEST(DirectReader, Variants)
{
    using namespace yyjson;
#if YYJSON_VERSION_HEX >= 0x000C00
    using reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::variant<double, std::vector<double>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::variant<int, ReadPoint>>>);
#endif

    const auto values = std::vector<std::string>{
        "null", "true", "false", "0", "1", "-1", "300", "-300", "1.5", "-2.5", "2.0", "-3e2", "1e300",
        "18446744073709551615", "-9223372036854775808", R"("")", R"("s")", R"("1")", "[]", "[1]", "[1,2]",
        R"([1.5,"a"])", R"(["a","b"])", "[[1,2]]", "[1,2,", "{}", R"({"a":1})", R"({"x":1,"y":2})",
        R"({"a":1,"b":2.0,"c":"x"})", R"({"i":"s"})", R"({"i":1,"k":"s"})", R"({"k":1})", "tru", R"("a)"};
    expect_direct_variants<
        std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
        std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
        std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
        std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
        std::variant<std::shared_ptr<int>, std::vector<std::string>>, std::variant<ReadPoint, int, std::vector<double>>,
        std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
        std::variant<std::nullptr_t, float, std::string>,
        std::variant<std::array<int, 2>, std::map<std::string, int>, std::string>,
        std::variant<std::vector<int>, std::vector<std::string>>>(values);
    // a struct is constructed from a number by aggregate initialization, which does not check 1e300 against its
    // first field
    auto fitting = values;
    std::erase(fitting, "1e300");
    expect_direct_variants<std::variant<ReflectionRecord, std::map<std::string, std::string>>>(fitting);
    expect_direct_variants<std::variant<int, double>, std::variant<std::string, double>,
                           std::variant<std::vector<int>, std::vector<double>>,
                           std::variant<ReflectionRecord, std::map<std::string, double>>>(
        {"0e100000", "[1,0e100000]", R"({"j":0e100000})", R"({"x":0e100000,"i":1})"}, false);

#if YYJSON_VERSION_HEX >= 0x000C00
    // an alternative that the reader cannot tell from the text does not pass to the next one, which the cast
    // from a document would not take
    using record_or_chain = std::variant<ReflectionRecord, ReadChain>;
    auto json = std::string(R"({"i":1,"next":)");
    for (auto i = 0; i < 70; ++i) json += R"({"next":)";
    json += "null" + std::string(71, '}');
    EXPECT_EQ(read(json).cast<record_or_chain>().index(), 0);
    EXPECT_EQ(read<record_or_chain>(json).index(), 0);
    EXPECT_THROW((void)direct_reader(json.data(), json.data() + json.size() - 1, ReadFlag::NoFlag)
                     .read_root<record_or_chain>(),
                 reader::detail::direct_read_unsupported);
#endif
}

struct IntList
{
    std::vector<int> v;
    bool operator==(const IntList&) const = default;
};

template <>
struct yyjson::caster<IntList>
{
    static IntList from_json(std::vector<int> v) { return IntList{std::move(v)}; }
    static const std::vector<int>& to_json(const IntList& x) { return x.v; }
};

TEST(DirectReader, VariantChecksNestedValues)
{
    using namespace yyjson::detail;

    // the values that a variant checks before it tries an alternative that another may follow
    static_assert(may_reject_nested<std::vector<int>>());
    static_assert(may_reject_nested<std::optional<std::vector<std::string>>>());
    static_assert(may_reject_nested<std::map<std::string, int>>());
    static_assert(may_reject_nested<std::vector<std::vector<int>>>());
    static_assert(may_reject_nested<IntList>());
    static_assert(!may_reject_nested<std::vector<ReadPoint>>());
    static_assert(!may_reject_nested<std::optional<int>>());
    static_assert(!may_reject_nested<ReadRecord>());
    static_assert(!may_reject_nested<std::string>());

    // only the first element or value is checked, so that one after it that does not fit still fails the
    // alternative
    const auto values = std::vector<std::string>{
        "null", "[]", "[1]", R"(["a"])", R"([1,"a"])", R"(["a",1])", "[1.5]", "[-1]", "[null]", "[[]]", "[[1]]",
        R"([["a"]])", R"([[1],["a"]])", R"([["a"],[1]])", "{}", R"({"a":1})", R"({"a":"x"})", R"({"a":1,"b":"x"})",
        R"({"a":[1]})", R"({"a":["x"]})", R"([1,2)", R"(["a)"};
    using vector_or_strings = std::variant<std::vector<int>, std::vector<std::string>>;
    using strings_or_vector = std::variant<std::vector<std::string>, std::vector<int>>;
    using nested = std::variant<std::vector<std::vector<int>>, std::vector<std::vector<std::string>>>;
    using maps = std::variant<std::map<std::string, int>, std::map<std::string, std::string>>;
    using maps_of_vectors =
        std::variant<std::map<std::string, std::vector<int>>, std::map<std::string, std::vector<std::string>>>;
    using optional_or_strings = std::variant<std::optional<std::vector<int>>, std::vector<std::string>>;
    using pairs_or_strings = std::variant<std::vector<std::pair<std::string, int>>, std::vector<std::string>>;
    using optionals_or_strings = std::variant<std::vector<std::optional<int>>, std::vector<std::string>>;
    using bytes_or_ints = std::variant<std::vector<std::uint8_t>, std::vector<int>>;
    using translated_or_strings = std::variant<IntList, std::vector<std::string>>;
    auto doc_values = std::string("[");
    for (const auto& value : values)
    {
        if (value.starts_with("[1,2") || value.starts_with("[\"a")) continue;
        doc_values += value + ",";
    }
    doc_values.back() = ']';
    expect_variants_as_trying_all<vector_or_strings, strings_or_vector, nested, maps, maps_of_vectors,
                                  optional_or_strings, pairs_or_strings, optionals_or_strings, bytes_or_ints,
                                  translated_or_strings,
                                  std::variant<std::shared_ptr<std::vector<int>>, std::vector<std::string>>>(
        doc_values);
    expect_direct_variants<vector_or_strings, strings_or_vector, nested, maps, maps_of_vectors, optional_or_strings,
                           pairs_or_strings, optionals_or_strings, bytes_or_ints, translated_or_strings>(values);
    EXPECT_EQ(yyjson::read<vector_or_strings>(R"(["a","b"])"), vector_or_strings(std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(yyjson::read<maps>(R"({"a":"x"})"), maps(std::map<std::string, std::string>{{"a", "x"}}));
}

struct AllocationFails
{
    int v = 0;
    bool operator==(const AllocationFails&) const = default;
};

template <>
struct yyjson::caster<AllocationFails>
{
    static inline auto calls = 0;
    static AllocationFails from_json(int)
    {
        ++calls;
        throw std::bad_alloc();
    }
    static int to_json(const AllocationFails& x) { return x.v; }
};

struct AllocationFailsField
{
    AllocationFails a;
};

TEST(DirectReader, BadAllocIsNotRetried)
{
    // `std::bad_alloc` from the reader without a document is thrown as it is, instead of reading the text again
    // into a document
    using yyjson::caster;
    caster<AllocationFails>::calls = 0;
    EXPECT_THROW((void)yyjson::read<std::vector<AllocationFails>>("[1]"), std::bad_alloc);
    auto v = std::vector<AllocationFails>();
    EXPECT_THROW(yyjson::read_into("[1]", v), std::bad_alloc);
    auto field = AllocationFailsField();
    EXPECT_THROW(yyjson::merge_into(R"({"a":1})", field), std::bad_alloc);
    EXPECT_EQ(caster<AllocationFails>::calls, 3);
}

struct NoDefaultValue
{
    explicit NoDefaultValue(int v) : v(v) {}
    int v;
};

template <>
struct yyjson::caster<NoDefaultValue>
{
    template <typename Json>
    static NoDefaultValue from_json(const Json& json)
    {
        return NoDefaultValue(cast<int>(json));
    }
    static int to_json(const NoDefaultValue& x) { return x.v; }
};

struct VariantFields
{
    std::variant<int, std::string> a;
    std::optional<std::variant<double, std::vector<int>>> b;
    std::variant<std::monostate, ReflectionRecord, std::vector<std::string>> c;
};

TEST(DirectReader, VariantFields)
{
    for (const auto* json : {R"({"a":1,"b":2.5,"c":{"i":1,"j":2,"k":"s"}})", R"({"a":"s","b":[1,2],"c":["x","y"]})",
                             R"({"a":"s","b":null,"c":null})", R"({"c":["x"],"a":"\u0041\n","b":[]})",
                             R"({"c":{"i":1},"c":["a"],"b":1,"b":null,"a":"x","a":2})", R"({})"})
    {
        EXPECT_TRUE(expect_direct_read<VariantFields>(json));
        EXPECT_TRUE(expect_direct_read<std::vector<VariantFields>>(std::format("[{},{}]", json, json)));
    }
    for (const auto* json : {R"({"b":[1,"x"]})", R"({"a":[1]})", R"({"c":{"i":"x"}})", R"({"c":1})",
                             R"({"b":{"a":1}})"})
    {
        expect_direct_read<VariantFields>(json);
        expect_direct_read<std::vector<VariantFields>>(std::format("[{}]", json));
    }

    // a variant whose first alternative has no default constructor
    using no_default = std::variant<NoDefaultValue, std::string>;
    static_assert(!std::is_default_constructible_v<no_default>);
    for (const auto* json : {"1", R"("s")", "[1]", "null"})
    {
        expect_direct_read<no_default>(json);
        expect_direct_read<std::vector<no_default>>(std::format("[{},{}]", json, json));
        expect_direct_read<std::map<std::string, no_default>>(std::format(R"({{"k":{}}})", json));
    }
    EXPECT_EQ(std::get<0>(yyjson::read<no_default>("2")).v, 2);
    EXPECT_EQ(std::get<1>(yyjson::read<no_default>(R"("s")")), "s");
}

namespace
{
    // The direct reader reads a tuple as the cast from a document does, and whenever the cast succeeds.
    template <typename... Ts>
    void expect_direct_tuples(const std::vector<std::string>& values)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            (
                [&] {
                    for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
                    {
                        const auto direct = expect_direct_read<std::vector<Ts>>(json);
                        if (reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Ts>>{}))
                                << json << " as " << yyjson::detail::type_name<Ts>();
                        }
                    }
                    const auto json = R"({"k":)" + value + "}";
                    const auto direct = expect_direct_read<std::map<std::string, Ts>>(json);
                    if (reads_directly)
                    {
                        EXPECT_EQ(direct, (succeeds(json, std::type_identity<std::map<std::string, Ts>>{})))
                            << json << " as " << yyjson::detail::type_name<Ts>();
                    }
                }(),
                ...);
        }
    }
}  // namespace

struct WithTuples
{
    std::tuple<int, std::string> a;
    std::vector<std::pair<std::string, double>> b;
    std::optional<std::tuple<std::pair<std::string, int>>> c;
};

TEST(DirectReader, Tuples)
{
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::tuple<int, std::string>>>);
    static_assert(
        !direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string, ReflectionRecord>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<ReadPoint, int>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string_view, int>>>>);
#endif

    const auto values = std::vector<std::string>{
        "[]", "[1]", R"([1,"a"])", R"([1, "a", 2])", R"([1, 2.5, "x"])", R"([1,2.5,"x",4])", R"({})",
        R"({"a":1})", R"({"a":1,"b":[1,2]})", R"({"a":1,"b":[1,2],"c":3})", R"({"a
":1, "A":[]})",
        R"({"a":"s"})", "1", "null", R"("s")", "[[1,2]]", R"([{"i":1,"k":"x"}])", "[null,null]", R"(["a",1])",
        R"([["a",1],["b",[2]]])", "[1,", R"({"a":1,})", R"([1, 2.5, "x\"y\u0041"])",
        R"({"a\u0041":1, "a key longer than the small string buffer":[1,2]})",
        R"([1, 2.5, "a string longer than the small string buffer"])", R"({"\\":1,"k":"v"})"};
    expect_direct_tuples<std::pair<int, std::string>, std::tuple<int, double, std::string>, std::tuple<int>,
                         std::tuple<>, std::tuple<std::pair<std::string, int>, std::pair<std::string, std::vector<int>>>,
                         std::tuple<ReflectionRecord, std::optional<int>>,
                         std::tuple<std::variant<int, std::string>, std::shared_ptr<int>>,
                         std::pair<std::tuple<int, int>, std::vector<std::pair<int, int>>>>(values);

    for (const auto* json : {R"({"a":[1,"x"],"b":[["p",1.5],["q",2]],"c":{"k":3}})", R"({"a":[2,""],"b":[],"c":null})",
                             R"({"c":{"k":1},"c":{"j":2},"a":[0,""],"a":[3,"y"]})"})
    {
        EXPECT_TRUE(expect_direct_read<WithTuples>(json)) << json;
    }
    const auto with_tuples = yyjson::read<WithTuples>(R"({"a":[1,"x"],"c":{"k\t":3}})");
    EXPECT_EQ(with_tuples.a, (std::tuple{1, std::string("x")}));
    EXPECT_EQ(with_tuples.c, (std::tuple<std::pair<std::string, int>>{{"k\t", 3}}));
    using string_pair = std::pair<std::string, std::string>;
    const auto escaped = yyjson::read<std::vector<std::tuple<string_pair, string_pair>>>(
        R"([{"\u0041 key longer than the small string buffer":"x\"y","k":"z"}, [["k","\\"],["\n",""]]])");
    ASSERT_EQ(escaped.size(), 2);
    EXPECT_EQ(std::get<0>(escaped[0]), string_pair("A key longer than the small string buffer", "x\"y"));
    EXPECT_EQ(std::get<1>(escaped[0]), string_pair("k", "z"));
    EXPECT_EQ(std::get<0>(escaped[1]), string_pair("k", "\\"));
    EXPECT_EQ(std::get<1>(escaped[1]), string_pair("\n", ""));
}

struct ReservedStrings
{
    std::vector<std::string> v;
    std::vector<std::string> w;
    int n = 0;
};

struct TrivialRecord
{
    int a;
    double b;
    auto operator<=>(const TrivialRecord&) const = default;
};
static_assert(std::is_trivially_copyable_v<TrivialRecord>);

TEST(DirectReader, ReservesRest)
{
    // strings that hide brackets, commas, quotes and backslashes at every offset of the blocks that the count
    // compares at once
    auto seed = std::uint32_t{1};
    const auto next = [&seed](std::uint32_t n) {
        seed = seed * 1664525 + 1013904223;
        return (seed >> 8) % n;
    };
    const auto pieces = std::array<std::string_view, 12>{
        ",", "]", "[", "{", "}", R"(\")", R"(\\)", R"(\n)", "a", R"(\u0041)", " ", R"(\\\")"};
    auto json = std::string("[");
    for (auto i = 0; i < 1500; ++i)
    {
        if (i != 0) json += next(3) == 0 ? " , " : ",";
        json += '"';
        for (auto k = next(24); k != 0; --k) json += pieces[next(pieces.size())];
        json += '"';
    }
    json += "]";

    const auto expected = yyjson::read(json).cast<std::vector<std::string>>();
    ASSERT_EQ(expected.size(), 1500);
#if YYJSON_VERSION_HEX >= 0x000C00
    const auto strings =
        yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, yyjson::ReadFlag::NoFlag)
            .read_root<std::vector<std::string>>();
    EXPECT_EQ(strings, expected);
#endif
    EXPECT_EQ(yyjson::read<std::vector<std::string>>(json), expected);

    const auto object = std::format(R"({{"v":{},"w":["a"],"n":1}})", json);
    const auto fields = yyjson::read<ReservedStrings>(object);
    EXPECT_EQ(fields.v, expected);
    EXPECT_EQ(fields.w, std::vector<std::string>{"a"});
    EXPECT_EQ(fields.n, 1);
    EXPECT_TRUE(expect_direct_read<ReservedStrings>(object));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    // the root array takes the exact count after its first element, and the others double up to 512 elements
    // before they take it
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(strings.capacity(), strings.size());
#endif
    EXPECT_EQ(fields.v.capacity(), fields.v.size());
#endif

    // the elements of an array with containers in it are not counted, but estimated in the root array
    using string_or_ints = std::variant<std::string, std::vector<int>>;
    auto mixed = json.substr(0, json.size() - 1) + ",[1,2]," + json.substr(1);
    EXPECT_TRUE(expect_direct_read<std::vector<string_or_ints>>(mixed));
    EXPECT_EQ(yyjson::read<std::vector<string_or_ints>>(mixed).size(), 3001);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<std::string>>>("[" + json + "," + json + "]"));
    auto nested = std::string("[");
    for (auto i = 0; i < 2000; ++i) nested += i == 0 ? "[" + std::to_string(i) + "]" : ",[" + std::to_string(i) + "]";
    nested += "]";
    expect_direct_read<std::vector<std::vector<std::string>>>(nested);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested + " "));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<std::vector<int>>>>(
        std::format(R"({{"a":{},"b":{}}})", nested, nested))));
    auto records = std::string("[");
    for (auto i = 0; i < 1000; ++i) records += std::format(R"({}{{"i":{},"k":"{}"}})", i == 0 ? "" : ",", i, i);
    records += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<ReflectionRecord>>(records));

    // a long element after short ones takes no more room than the bytes left allow
    using int_or_string = std::variant<int, std::string, std::vector<int>>;
    auto skewed = std::string("[");
    for (auto i = 0; i < 600; ++i) skewed += "1,";
    skewed += "[1],\"" + std::string(100000, 'x') + "\"]";
    EXPECT_TRUE(expect_direct_read<std::vector<int_or_string>>(skewed));
    const auto skewed_read = yyjson::read<std::vector<int_or_string>>(skewed);
    EXPECT_EQ(skewed_read.size(), 602);
    EXPECT_LE(skewed_read.capacity() * sizeof(int_or_string), 4 * skewed.size() + 1024 * sizeof(int_or_string));

    // trivially copyable elements are not counted, but estimated in the root array only
    auto trivial = std::string("[");
    for (auto i = 0; i < 3000; ++i) trivial += std::format(R"({}{{"a":{},"b":{}.25}})", i == 0 ? "" : ",", i, i);
    trivial += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial));
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial + " "));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<TrivialRecord>>>("[" + trivial + "," + trivial + "]"));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<TrivialRecord>>>(
        std::format(R"({{"a":{},"b":{}}})", trivial, trivial))));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    const auto trivial_read = yyjson::read<std::vector<TrivialRecord>>(trivial);
    EXPECT_GE(trivial_read.capacity(), 3000);
    EXPECT_LT(trivial_read.capacity(), 4096);
#endif
    // the estimate is too small when the elements get longer, and too large when they get shorter
    auto growing = std::string("[");
    for (auto i = 0; i < 600; ++i) growing += "1,";
    for (auto i = 0; i < 3000; ++i) growing += "1000000000,";
    growing += "2]";
    EXPECT_TRUE(expect_direct_read<std::vector<int>>(growing));
    EXPECT_EQ(yyjson::read<std::vector<int>>(growing).size(), 3601);
    auto shrinking = std::string("[");
    for (auto i = 0; i < 600; ++i) shrinking += "[1000000000,1000000000],";
    for (auto i = 0; i < 3000; ++i) shrinking += "[1,2],";
    shrinking += "[3,4]]";
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 2>>>(shrinking)));
    const auto shrinking_read = yyjson::read<std::vector<std::array<int, 2>>>(shrinking);
    EXPECT_EQ(shrinking_read.size(), 3601);
    EXPECT_LE(shrinking_read.capacity() * sizeof(std::array<int, 2>),
              4 * shrinking.size() + 1024 * sizeof(std::array<int, 2>));
}

// NOLINTEND
