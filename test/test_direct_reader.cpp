#include "test_common.hpp"

// NOLINTBEGIN

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(DirectReader, PowersOfFive)
{
    // the table of the reader, computed again as its comment describes
    using yyjson::reader::detail::pow5_128_max;
    using yyjson::reader::detail::pow5_128_min;
    constexpr int words = 28;  // 2^1728, which divided by 5^342 leaves more than the bits needed
    using big = std::array<std::uint64_t, words>;
    const auto bit_length = [](const big& a, int used) {
        for (int i = used - 1; i >= 0; --i)
        {
            if (a[static_cast<std::size_t>(i)] != 0)
                return i * 64 + 64 - std::countl_zero(a[static_cast<std::size_t>(i)]);
        }
        return 0;
    };
    // the 64 bits from bit `pos`, which may be negative
    const auto bits_at = [](const big& a, int pos) {
        const auto word = [&](int i) { return i >= 0 && i < words ? a[static_cast<std::size_t>(i)] : 0; };
        const int i = pos >= 0 ? pos / 64 : -((-pos + 63) / 64);
        const int b = pos - i * 64;
        return word(i) >> b | (b != 0 ? word(i + 1) << (64 - b) : 0);
    };
    const auto mul5 = [](big& a, int& used) {
        auto carry = std::uint64_t{0};
        for (int i = 0; i < used; ++i)
        {
            auto& w = a[static_cast<std::size_t>(i)];
            const auto lo = (w & 0xFFFFFFFFu) * 5 + carry;
            const auto hi = (w >> 32) * 5 + (lo >> 32);
            w = (hi << 32) | (lo & 0xFFFFFFFFu);
            carry = hi >> 32;
        }
        if (carry != 0) a[static_cast<std::size_t>(used++)] = carry;
    };
    const auto div5 = [](big& a, int& used) {
        auto rem = std::uint64_t{0};
        for (int i = used - 1; i >= 0; --i)
        {
            auto& w = a[static_cast<std::size_t>(i)];
            const auto hi = (rem << 32) | (w >> 32);
            const auto lo = ((hi % 5) << 32) | (w & 0xFFFFFFFFu);
            w = ((hi / 5) << 32) | (lo / 5);
            rem = lo % 5;
        }
        if (a[static_cast<std::size_t>(used - 1)] == 0) --used;
    };
    auto table = std::vector<std::uint64_t>(2 * (pow5_128_max - pow5_128_min + 1));
    const auto store = [&](int q, std::uint64_t hi, std::uint64_t lo) {
        table[static_cast<std::size_t>(2 * (q - pow5_128_min))] = hi;
        table[static_cast<std::size_t>(2 * (q - pow5_128_min) + 1)] = lo;
    };
    auto p = big();
    p[0] = 1;
    auto p_used = 1;
    for (int q = 0; q <= pow5_128_max; ++q, mul5(p, p_used))
    {
        const int top = bit_length(p, p_used) - 128;
        store(q, bits_at(p, top + 64), bits_at(p, top));
    }
    // floor(2^b / 5^n) + 1 with b = z + 127 for n <= 27 and b = 2z + 128 beyond, where z is the bit
    // length of 5^n, truncated to 128 bits: floor(2^b / 5^n) is the quotient of 2^1728 shifted right
    constexpr int k = (words - 1) * 64;
    auto f = big();
    f[words - 1] = 1;
    auto f_used = words;
    p = big();
    p[0] = 1;
    p_used = 1;
    for (int n = 1; n <= -pow5_128_min; ++n)
    {
        div5(f, f_used);
        mul5(p, p_used);
        const int z = bit_length(p, p_used);
        const int low = k - (n <= 27 ? z + 127 : 2 * z + 128);  // the lowest bit of the quotient
        const int top = bit_length(f, f_used) - 128;            // the lowest bit kept of it
        auto hi = bits_at(f, top + 64);
        auto lo = bits_at(f, top);
        // adding 1 changes the kept bits only if all the bits below them are ones
        auto carry = true;
        for (int i = low; carry && i < top; i += 64)
        {
            const int n_bits = std::min(64, top - i);
            const auto mask = n_bits == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << n_bits) - 1;
            carry = (bits_at(f, i) & mask) == mask;
        }
        if (carry && ++lo == 0 && ++hi == 0) hi = std::uint64_t{1} << 63;  // 2^128 truncated
        store(-n, hi, lo);
    }
    EXPECT_TRUE(std::ranges::equal(table, yyjson::reader::detail::pow5_128_table));
}
#endif

namespace
{
    // whether every real of a JSON array can be converted to I; converting one out of range is undefined
    template <std::integral I>
    bool reals_fit(const std::string& json)
    {
        try
        {
            const auto upper = std::ldexp(1.0, std::numeric_limits<I>::digits);
            const auto lower = std::is_signed_v<I> ? -upper : -1.0;
            return std::ranges::all_of(*yyjson::read(json).as_array(), [&](const auto& v) {
                if (!v.is_real()) return true;
                const auto x = *v.as_real();
                return x > lower && x < upper;
            });
        }
        catch (const yyjson::read_error&)
        {
            return true;
        }
    }
}  // namespace

TEST(DirectReader, Numbers)
{
    using namespace yyjson;

    const auto numbers = std::vector<std::string>{
        "0", "-0", "1", "-1", "9", "10", "01", "-01", "00", "1.", "1e", "1e+", "-", ".5", "+1", "1.5", "-1.5", "0.0",
        "-0.0", "1.5e3", "1.5E-3", "1e400", "-1e400", "1e-400", "4.9e-324", "2.2250738585072014e-308",
        "1.7976931348623157e308", "0.1", "0.30000000000000004", "9007199254740993", "9007199254740993.0",
        "1.00000000000000011102230246251565404236316680908203125", "123456789012345678901234567890",
        "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967295", "4294967296",
        "9223372036854775807", "9223372036854775808", "-9223372036854775808", "-9223372036854775809",
        "18446744073709551615", "18446744073709551616", "-18446744073709551616", "true", "null", "\"1\"", "1.5.3",
        "1.5-2", "1..5", "0.5.", "-.5", "1.2345678901234567", "12345678.12345678901", "0.0000000000000001"};
    for (const auto& number : numbers)
    {
        // as the last value, and followed by enough characters for the vector loads of the reader
        const auto padding = std::string(40, ' ');
        for (const auto& json : {"[" + number + "]", "[" + number + padding + ",1" + padding + "]"})
        {
            expect_direct_read<std::vector<double>>(json);
            expect_direct_read<std::vector<float>>(json);
            if (reals_fit<std::int64_t>(json)) expect_direct_read<std::vector<std::int64_t>>(json);
            if (reals_fit<std::uint64_t>(json)) expect_direct_read<std::vector<std::uint64_t>>(json);
            if (reals_fit<int>(json)) expect_direct_read<std::vector<int>>(json);
            if (reals_fit<std::int8_t>(json)) expect_direct_read<std::vector<std::int8_t>>(json);
            expect_direct_read<std::vector<bool>>(json);
            expect_direct_read<std::vector<std::optional<double>>>(json);
        }
    }

    // every length of the integer part and of the fraction around the bounds of the short form of reals
    for (auto n1 = 1; n1 <= 10; ++n1)
    {
        for (auto n2 = 0; n2 <= 20; ++n2)
        {
            for (const auto* sign : {"", "-"})
            {
                for (const auto* suffix : {"", "e5", "E-2", "x", ".7"})
                {
                    auto number = std::string(sign);
                    for (auto i = 0; i < n1; ++i) number += static_cast<char>('1' + (i * 7 + n2) % 9);
                    if (n2 > 0) number += '.';
                    for (auto i = 0; i < n2; ++i) number += static_cast<char>('0' + (i * 3 + n1) % 10);
                    number += suffix;
                    const auto padded = "[" + number + "," + std::string(40, ' ') + "1]";
                    const auto direct = expect_direct_read<std::vector<double>>(padded);
                    if (*suffix != 'x' && *suffix != '.')
                    {
                        EXPECT_TRUE(direct) << padded;
                    }
                    expect_direct_read<std::vector<double>>("[" + number + "]");
                    expect_direct_read<std::vector<std::array<double, 2>>>("[[" + number + "," + number + "]," +
                                                                           std::string(40, ' ') + "[1,2]]");
                }
            }
        }
    }
    // integers of every length as elements of an array, which read one or two digits apart from longer ones, followed
    // by each character that ends or continues them
    for (auto n = 1; n <= 21; ++n)
    {
        for (const auto* sign : {"", "-"})
        {
            for (const auto* next : {",", "]", " ,", "x,", ".,", ".5,", "e1,", "0,"})
            {
                auto number = std::string(sign);
                for (auto i = 0; i < n; ++i) number += static_cast<char>('1' + (i * 5 + n) % 9);
                for (const auto& json : {"[" + number + next + "1," + std::string(40, ' ') + "2]",
                                         "[" + number + next + "1]"})
                {
                    expect_direct_read<std::vector<double>>(json);
                    if (reals_fit<std::int64_t>(json))
                    {
                        expect_direct_read<std::vector<std::int64_t>>(json);
                        expect_direct_read<std::array<std::int64_t, 3>>(json);
                    }
                    if (reals_fit<std::uint64_t>(json)) expect_direct_read<std::vector<std::uint64_t>>(json);
                    if (reals_fit<std::int8_t>(json)) expect_direct_read<std::vector<std::int8_t>>(json);
                }
            }
        }
    }
    // an exponent beyond the range of doubles that leading zeros of the fraction bring back into it, or not
    for (const auto zeros : {100, 99990, 100000, 999990})
    {
        for (const auto* exponent : {"e100", "e99990", "e100000", "e1000000", "e1000300", "e-1000000"})
        {
            const auto json = "[0." + std::string(static_cast<std::size_t>(zeros), '0') + "1" + exponent + "]";
            expect_direct_read<std::vector<double>>(json);
            expect_direct_read<KeyLengths>(R"({"a":1,"x":)" + json + "}");
        }
    }
}

TEST(DirectReader, Strings)
{
    using namespace yyjson;

    // every kind of escape and of invalid text, at every position of a vector load
    auto pieces = std::vector<std::string>{
        "a", "\\\"", "\\\\", "\\/", "\\b", "\\f", "\\n", "\\r", "\\t", "\\u0041", "\\u00e9", "\\u20AC",
        "\\ud83d\\ude00", "\\ud83d", "\\ude00", "\\uD800\\u0041", "\\u0000", "\\uZZZZ", "\\x", "\\", "\xc3\xa9",
        "\xe3\x81\x82", "\xf0\x9f\x98\x80", "\xc3", "\xff", "\xed\xa0\x80", "\x01", "\x1f", "\x7f", "\t",
        "\\u\"", "\\u00\"1", "\\\x01", "\\\xc3\xa9", "\\u3042\\u3044\\u3046\\u3048\\u304a\\u304b\\u304d\\u304f"};
    // a quote after a run of backslashes ends the string when the run is even, across the end of a vector as well
    for (const auto run : {1, 2, 3, 4, 15, 16, 17, 31, 32, 33, 63, 64})
    {
        pieces.push_back(std::string(static_cast<std::size_t>(run), '\\') + "\"");
        pieces.push_back(std::string(static_cast<std::size_t>(run), '\\') + "\\u0041");
    }
    for (const auto& piece : pieces)
    {
        for (auto before = std::size_t{0}; before < 40; ++before)
        {
            const auto text = std::string(before, 'x') + piece + std::string(before % 7, 'y');
            const auto arr = "[\"" + text + "\",\"" + text + "z\"" + std::string(before % 5 * 10, ' ') + "]";
            const auto obj = "{\"" + text + "\":\"v\",\"k\":\"" + text + "\"}";
            for (const auto flag : {ReadFlag::NoFlag, ReadFlag::AllowInvalidUnicode})
            {
                expect_direct_read<std::vector<std::string>>(arr, flag);
                expect_direct_read<std::map<std::string, std::string>>(obj, flag);
            }
        }
    }
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>(
        R"(["a\"b\\c\/\b\f\n\r\t\u0041\u00e9\u20ac\ud83d\ude00"])")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>(R"(["", "a", ""])")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>("[\"" + std::string(1000, 'z') + "\"]")));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, int>>(R"({"a\nb":1,"\u0041":2,"a":3,"a":4})")));
    // text of every length between escapes, and a backslash before every character
    for (auto gap = std::size_t{0}; gap <= 70; ++gap)
    {
        EXPECT_TRUE((expect_direct_read<std::vector<std::string>>(
            "[\"\\n" + std::string(gap, 'x') + "\\t" + std::string(gap, 'y') + "\\u00e9" + std::string(gap, 'z') +
            "\"]")));
    }
    for (auto c = 0; c < 256; ++c)
    {
        expect_direct_read<std::vector<std::string>>("[\"" + std::string(40, 'x') + "\\" +
                                                     std::string(1, static_cast<char>(c)) + "1234\"]");
    }
    expect_direct_read<std::vector<std::string>>("[\"a\xff\"]");
    expect_direct_read<std::vector<std::string>>("[\"a\xff\"]", ReadFlag::AllowInvalidUnicode);

    // the escapes of a value that a struct skips are checked as well
    for (const auto& piece : pieces)
    {
        for (const auto& json : {R"({"a":1,"x":")" + piece + R"("})", R"({"a":1,"x":{")" + piece + R"(":1}})",
                                 R"({"a":1,"x":[1,")" + piece + R"("]})"})
        {
            expect_direct_read<KeyLengths>(json);
        }
    }
}

TEST(DirectReader, Structs)
{
    using namespace yyjson;

    for (const auto* json : {
             R"({"i":1,"j":2.5,"k":"s"})", R"({"k":"s","j":2.5,"i":1})", R"({"i":1})", R"({})",
             R"({"x":{"y":[1,{"z":null}],"w":"\"}"},"i":1,"q":[[],{}]})", R"({"\u0069":3,"k":"\u0041"})",
             R"({"i":1,"i":2})", R"( { "i" : 1 , "j" : 2 } )", "{\n  \"i\": 1,\n  \"k\": \"s\"\n}",
             R"({"i":1,"j":2.5,"k":"s","x":0,"i":2})", R"({"i":1,"k":"s","j":2.5})"})
    {
        EXPECT_TRUE((expect_direct_read<ReflectionRecord>(json))) << json;
        EXPECT_TRUE((expect_direct_read<VisitableRecord>(json))) << json;
        EXPECT_TRUE((expect_direct_read<std::vector<ReflectionRecord>>(std::format("[{},{}]", json, json)))) << json;
    }
    for (const auto* json : {R"({"i":"s"})", R"({"i":1.5})", R"({"k":1})", R"({"i":null})", R"({"i":[1]})", R"([1])",
                             R"({"i":1,})", R"({"i" 1})", R"({"i":1}})", R"({i:1})", R"({"i":1)", R"({"i":tru})"})
    {
        expect_direct_read<ReflectionRecord>(json);
        expect_direct_read<VisitableRecord>(json);
    }

    auto many = std::string("{");
    for (auto i = 1; i <= 30; ++i) many += std::format("{}\"f{:02}\":{}", i == 1 ? "" : ",", i, i);
    EXPECT_TRUE((expect_direct_read<ManyFields>(many + "}")));
    EXPECT_TRUE((expect_direct_read<ManyFields>(R"({"f30":1,"f01":2,"f15":3,"f16":4})")));

    // the names that the reader compares with the text in one or two loads, and their neighbours
    for (const auto* key : {"a", "b", "", "bb", "bbb", "fifteen_chars_x", "fifteen_chars_", "fifteen_chars_xy",
                            "sixteen_chars_xx", "sixteen_chars_x", "sixteen_chars_xxx", "seventeen_chars_x",
                            "seventeen_chars_", "seventeen_chars_xy", "fifteen_chars_y",
                            "thirty_one_chars_xxxxxxxxxxxxxx", "thirty_one_chars_xxxxxxxxxxxxx",
                            "thirty_one_chars_xxxxxxxxxxxxxxx", "thirty_one_chars_xxxxxxxxxxxxxy",
                            "thirty_one_charsxxxxxxxxxxxxxxx", "thirty_two_chars_xxxxxxxxxxxxxxx",
                            "thirty_two_chars_xxxxxxxxxxxxxx", "thirty_two_chars_xxxxxxxxxxxxxxxx",
                            "thirty_two_chars_xxxxxxxxxxxxxxy", "\\u0061", "v"})
    {
        // the members before the key make the reader expect the field after them, which it compares the key with
        for (const auto* before : {"", R"("a":1,"bb":2,)",
                                   R"("a":1,"bb":2,"fifteen_chars_x":3,"sixteen_chars_xx":4,"seventeen_chars_x":5,)",
                                   R"("a":1,"bb":2,"fifteen_chars_x":3,"sixteen_chars_xx":4,"seventeen_chars_x":5,)"
                                   R"("thirty_one_chars_xxxxxxxxxxxxxx":6,)"})
        {
            for (const auto* after : {"", R"(,"a":7)", R"(,"bb":8,"fifteen_chars_x":9)",
                                      R"(,"thirty_one_chars_xxxxxxxxxxxxxx":10,"thirty_two_chars_xxxxxxxxxxxxxxx":11)"})
            {
                for (const auto padding : {0, 40})
                {
                    const auto json = std::format(R"({{{}"{}":1{}}}{})", before, key, after,
                                                  std::string(static_cast<std::size_t>(padding), ' '));
                    EXPECT_TRUE((expect_direct_read<KeyLengths>(json) || std::string_view(key) == "v")) << json;
                }
            }
        }
    }

    EXPECT_TRUE((expect_direct_read<X>(R"({"a":1,"b":null,"c":"s"})")));
    EXPECT_TRUE((expect_direct_read<X>(R"({"a":1,"b":2.5})")));
    EXPECT_TRUE((expect_direct_read<Z>(R"({"c":"z","a":1})")));
    EXPECT_TRUE((expect_direct_read<std::vector<Z>>(R"([{"a":1},{"a":2,"b":3}])")));
    EXPECT_TRUE(
        (expect_direct_read<NullableField>(R"({"optional_value":null,"nullable_value":null,"tri_state":null})")));
    EXPECT_TRUE((expect_direct_read<NullableField>(R"({"optional_value":1,"nullable_value":2,"tri_state":3})")));
    EXPECT_TRUE((expect_direct_read<Shape>(R"({"name":"s","origin":{"x":1,"y":2,"valid":true}})")));
    EXPECT_TRUE((expect_direct_read<CounterRecord>(R"({"name":"c","hits":3})")));
}

TEST(DirectReader, Containers)
{
    using namespace yyjson;

    EXPECT_TRUE((expect_direct_read<std::vector<int>>("[]")));
    EXPECT_TRUE((expect_direct_read<std::vector<int>>(" [ 1 , 2 ,3 ] ")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::vector<int>>>("[[],[1],[2,3]]")));
    EXPECT_TRUE((expect_direct_read<std::list<std::string>>(R"(["a","b"])")));
    EXPECT_TRUE((expect_direct_read<std::array<int, 2>>("[1,2]")));
    expect_direct_read<std::array<int, 2>>("[1]");
    expect_direct_read<std::array<int, 2>>("[1,2,3]");
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<double, 2>>>("[[1.5,2],[3,4.25]]")));
    expect_direct_read<std::vector<std::array<double, 2>>>("[[1.5,2],[3]]");
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<double>>>(R"({"a":[1.5],"b":[]})")));
    EXPECT_TRUE((expect_direct_read<std::unordered_map<std::string, int>>(R"({"a":1,"b":2})")));
    EXPECT_TRUE((expect_direct_read<std::tuple<int, std::string, double>>(R"([1,"a",2.5])")));
    expect_direct_read<std::tuple<int, std::string>>(R"([1,"a",2.5])");
    expect_direct_read<std::vector<std::optional<std::vector<int>>>>("[null,[1]]");
    EXPECT_TRUE((expect_direct_read<std::vector<std::optional<int>>>("[null,1]")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::shared_ptr<int>>>("[null,1]")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::variant<std::monostate, int, std::string>>>(R"([null,1,"s"])")));
    EXPECT_TRUE((expect_direct_read<std::vector<ReadPoint>>("[[1,2],[3,4]]")));
    expect_direct_read<std::vector<ReadPoint>>("[[1,2],[3]]");
    EXPECT_TRUE((expect_direct_read<ReadRecord>(
        R"({"name":"r","points":[[1,2]],"tag":"t","series":{"s":[0.5]},"keys":{"a":1,"v":[2]},"note":"n",)"
        R"("pair":[1,"p"]})")));
    EXPECT_TRUE((expect_direct_read<ReadRecord>(R"({"tag":1,"keys":null,"note":null})")));
    expect_direct_read<ReadRecord>(R"({"tag":1.5})");

    // the text that the reader leaves to the document, and invalid text
    for (const auto* json : {"1", "\"s\"", "null", "", " ", "[", "]", "[1,]", "[1 2]", "[1]x", "[1] [2]", "[1]]",
                             "[tru]", "[nul]", "[1,2", "{\"a\":1,}", "[1/*c*/]", "[\"a\" \"b\"]"})
    {
        expect_direct_read<std::vector<int>>(json);
        expect_direct_read<std::map<std::string, int>>(json);
        expect_direct_read<std::vector<int>>(json, ReadFlag::AllowComments);
        expect_direct_read<std::vector<int>>(json, ReadFlag::AllowTrailingCommas);
    }
}

TEST(DirectReader, EmptyArrayFields)
{
    using namespace yyjson;

    // fields whose default values are not empty
    for (const auto* json : {R"({"v":[],"s":[],"l":[],"nested":[]})", "{\"v\":[ ],\"s\":[\n],\"nested\":[[]]}",
                             R"({"v":[1],"s":[2,3],"m":{"a":1},"nested":[[],[6]]})", R"({"v":[] })"})
    {
        EXPECT_TRUE((expect_direct_read<EmptyArrays>(json))) << json;
        EXPECT_TRUE((expect_direct_read<std::vector<EmptyArrays>>("[" + std::string(json) + "]"))) << json;
    }
    for (const auto* json : {R"({"v":[]x})", R"({"v":[])", R"({"v":[)", R"({"v":[})", R"({"v":[],})", R"({"v":{}})",
                             R"({"v":null})", R"({"s":[1,]})", R"({"m":[]})"})
    {
        expect_direct_read<EmptyArrays>(json);
        expect_direct_read<std::vector<EmptyArrays>>("[" + std::string(json) + "]");
    }
}

TEST(DirectReader, BoolFields)
{
    using namespace yyjson;

    for (const auto* json : {R"({"a":true,"b":false,"c":true})", R"({"a":false,"b":true,"c":false})",
                             R"({"c":null,"b":false })", R"({"b" : false})"})
    {
        EXPECT_TRUE((expect_direct_read<BoolFields>(json))) << json;
        EXPECT_TRUE((expect_direct_read<std::vector<BoolFields>>("[" + std::string(json) + "]"))) << json;
    }
    for (const auto* json : {R"({"a":tru})", R"({"a":fals})", R"({"a":true)", R"({"a":false)", R"({"a":t)",
                             R"({"a":f)", R"({"a":truex})", R"({"a":falsee})", R"({"a":True})", R"({"a":null})",
                             R"({"a":1})", R"({"a":"true"})", R"({"b":tRue})", R"({"b":fAlse})"})
    {
        expect_direct_read<BoolFields>(json);
        expect_direct_read<std::vector<BoolFields>>("[" + std::string(json) + "]");
    }
}

TEST(DirectReader, Indentation)
{
    using namespace yyjson;

    // the white space before the keys and the elements, which the reader predicts to be as long as the one before
    // the previous key or element: the same run, runs of other lengths around the size of a vector register, other
    // white space characters, and a character that is not white space within the predicted run
    auto state = std::uint32_t{12345};
    const auto next = [&](std::uint32_t n) {
        state = state * 1103515245u + 12345u;
        return (state >> 16) % n;
    };
    const auto make_run = [&](std::size_t length) {
        auto run = std::string();
        for (auto i = std::size_t{0}; i < length; ++i) run += i == 0 ? '\n' : " \t\r "[next(4)];
        return run;
    };
    for (auto round = 0; round < 400; ++round)
    {
        auto run = make_run(next(40));
        // whether each of the object, the strings and the records is valid
        bool valid[3] = {true, true, true};
        const auto ws = [&](int text) {
            const auto choice = next(20);
            if (choice < 12) return run;
            if (choice < 19) run = make_run(choice < 16 ? next(40) : run.size() + next(3) - (run.empty() ? 0 : 1));
            else if (!run.empty())
            {
                auto broken = run;
                broken[next(static_cast<std::uint32_t>(broken.size()))] = 'x';
                valid[text] = false;
                return broken;
            }
            return run;
        };
        const auto count = 1 + next(6);
        auto object = std::string("{");
        auto strings = std::string("[");
        auto records = std::string("[");
        for (auto i = 0u; i < count; ++i)
        {
            const auto* comma = i == 0 ? "" : ",";
            object += std::format("{}{}\"{}\":{}", comma, ws(0), "iju"[next(3)], i);
            strings += std::format("{}{}\"s{}\"", comma, ws(1), i);
            records += std::format("{}{}{{{}\"i\":{},", comma, ws(2), ws(2), i);
            records += std::format("{}\"k\":\"t\"{}}}", ws(2), ws(2));
        }
        object += ws(0) + "}";
        strings += ws(1) + "]";
        records += ws(2) + "]";
        EXPECT_EQ((expect_direct_read<std::map<std::string, int>>(object)), valid[0] || !reads_directly) << object;
        EXPECT_EQ((expect_direct_read<ReflectionRecord>(object)), valid[0] || !reads_directly) << object;
        EXPECT_EQ((expect_direct_read<std::vector<std::string>>(strings)), valid[1] || !reads_directly) << strings;
        EXPECT_EQ((expect_direct_read<std::vector<ReflectionRecord>>(records)), valid[2] || !reads_directly)
            << records;
    }
}

TEST(DirectReader, MapMemberOrder)
{
    using namespace yyjson;

    // members in the order of the map, the reverse, a mix, and duplicate keys, whose first value is kept
    constexpr auto n = 150;
    auto orders = std::vector<std::vector<std::pair<std::string, int>>>(5);
    for (auto i = 0; i < n; ++i)
    {
        orders[0].emplace_back(std::to_string(i), i);
        orders[1].emplace_back(std::to_string(n - 1 - i), i);
        orders[2].emplace_back(std::to_string(i * 37 % n), i);
        orders[3].emplace_back(std::format("k{:03}", i), i);
        orders[4].emplace_back(std::to_string(i % 7 * 3 % 10), i);
    }
    auto run = [&]<typename T>(std::type_identity<T>) {
        for (const auto& members : orders)
        {
            auto json = std::string("{");
            auto expected = T();
            for (const auto& [key, value] : members)
            {
                json += std::format(R"({}"{}":{})", json.size() == 1 ? "" : ",", key, value);
                expected.emplace(key, value);
            }
            json += "}";
            EXPECT_EQ(read(json).cast<T>(), expected) << json;
            EXPECT_EQ(read<T>(json), expected) << json;
        }
    };
    run(std::type_identity<std::map<std::string, int>>());
    run(std::type_identity<std::map<std::string, int, std::greater<>>>());
    run(std::type_identity<std::multimap<std::string, int>>());
    run(std::type_identity<std::unordered_map<std::string, int>>());
    static_assert(detail::hinted_map<std::map<std::string, int>>);
    static_assert(!detail::hinted_map<std::multimap<std::string, int>>);
    static_assert(!detail::hinted_map<std::unordered_map<std::string, int>>);
}

TEST(DirectReader, DocumentOfWholeText)
{
    using namespace yyjson;
#if YYJSON_VERSION_HEX >= 0x000C00
    using reader::detail::direct_reader;

    // the types that the reader would read through a document for each element of a range or a map are read
    // through a document of the whole text
    static_assert(!direct_reader::reads_documents_in_loop<ReflectionRecord>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<ReflectionRecord>>);
    static_assert(!direct_reader::reads_documents_in_loop<ReadPoint>);
    static_assert(!direct_reader::reads_documents_in_loop<CounterRecord>);
    static_assert(!direct_reader::reads_documents_in_loop<std::shared_ptr<ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::map<std::string, ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<CounterRecord>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::shared_ptr<ReadPoint>>>);
    static_assert(direct_reader::reads_documents_in_loop<ReadRecord>);
    // a recursive type is visited once
    static_assert(!direct_reader::reads_documents_in_loop<ReadChain>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<ReadChain>>);
    // and so is a type that many paths reach
    static_assert(!direct_reader::reads_documents_in_loop<DiamondChain<diamond_depth>>);
#endif

    expect_direct_read<std::vector<CounterRecord>>(R"([{"name":"a","hits":1},{"hits":2}])");
    expect_direct_read<std::vector<CounterRecord>>(R"([{"name":"a","hits":"1"}])");
    expect_direct_read<std::map<std::string, ReadPoint>>(R"({"a":[1,2],"b":[3,4]})");
    expect_direct_read<std::map<std::string, ReadPoint>>(R"({"a":[1,2],"b":[3]})");
    expect_direct_read<std::vector<std::shared_ptr<ReadPoint>>>(R"([null,[1,2]])");

    // a recursive type
#if YYJSON_VERSION_HEX >= 0x000C00
    for (const auto* json : {R"({"point":[1,2],"next":{"point":[3,4],"next":null}})", R"({"point":[1,2]})"})
    {
        const auto expected = read(json).cast<ReadChain>();
        EXPECT_EQ(read<ReadChain>(json), expected) << json;
        const auto direct =
            reader::detail::direct_reader(json, json + std::strlen(json) - 1, ReadFlag::NoFlag).read_root<ReadChain>();
        EXPECT_EQ(direct, expected) << json;
        EXPECT_EQ(read<std::vector<ReadChain>>(std::format("[{},{}]", json, json)),
                  (std::vector<ReadChain>{expected, expected}))
            << json;
    }
    EXPECT_THROW((void)read<std::vector<ReadChain>>(R"([{"point":[1,2],"next":{"point":[3]}}])"), bad_cast);

    expect_direct_read<DiamondChain<diamond_depth>>(R"({"a":[{"b":[{"a":[]}]}],"b":[]})");
#endif
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(DirectReader, LocalDocuments)
{
    using namespace yyjson;
    using reader::detail::direct_reader;

    // the reader reads each value that a caster takes on a document from a copy of the text that it makes a chunk
    // at a time, and the values that end beyond the chunk again from a longer one
    // from a buffer that ends at the closing bracket, for the address sanitizer to see a read beyond it
    const auto direct = []<typename T>(std::type_identity<T>, const std::string& json) {
        const auto text = std::make_unique_for_overwrite<char[]>(json.size());
        std::memcpy(text.get(), json.data(), json.size());
        return direct_reader(text.get(), text.get() + json.size() - 1, ReadFlag::NoFlag).read_root<T>();
    };
    const auto check = [&]<typename T>(std::type_identity<T> type, const std::string& json) {
        const auto expected = read(json).cast<T>();
        EXPECT_EQ(direct(type, json), expected) << json.substr(0, 80);
        EXPECT_EQ(read<T>(json), expected) << json.substr(0, 80);
    };

    // numbers that a chunk may cut short, from every offset in the text, and a number longer than a chunk
    check(std::type_identity<std::vector<DocumentOf<double>>>{},
          std::format("[1,1.{}e-5,2]", std::string(10000, '0')));
    for (auto pad = 0; pad < 24; ++pad)
    {
        auto json = std::string("[") + std::string(static_cast<std::size_t>(pad), ' ');
        for (auto i = 0; i < 2000; ++i) json += std::format("{}{},", i % 7 == 0 ? "-" : "", 1234567890123LL + i);
        json += "-1.5e300]";
        check(std::type_identity<std::vector<DocumentOf<double>>>{}, json);
        check(std::type_identity<std::vector<DocumentOf<std::int64_t>>>{},
              json.substr(0, json.size() - 9) + "0]");
    }

    // strings with escapes, which yyjson unescapes in its copy, before and after values longer than the chunk
    {
        auto json = std::string("[");
        for (auto i = 0; i < 300; ++i)
        {
            json += i % 50 == 25 ? std::format(R"("{}\u00e9\n",)", std::string(static_cast<std::size_t>(i) * 300, 'x'))
                                 : std::format(R"("a\"b\\{}\u3042",)", i);
        }
        json += R"("end")" "]";
        check(std::type_identity<std::vector<DocumentOf<std::string>>>{}, json);
    }
    // and in the part of a value longer than the chunk that yyjson reads before it finds the chunk too short
    {
        auto json = std::string("[");
        for (auto i = 0; i < 40; ++i)
        {
            json += "[";
            for (auto j = 0; j < (i % 8 + 1) * 150; ++j) json += std::format(R"("a\"b\\{}\u3042",)", j);
            json += R"("end"],)";
        }
        json += "[]]";
        check(std::type_identity<std::vector<DocumentOf<std::vector<std::string>>>>{}, json);
    }

    // containers, in a map, the last of which ends at the closing bracket of the root
    {
        auto json = std::string("{");
        for (auto i = 0; i < 200; ++i)
            json += std::format(R"("k{}":[{},{}{}] ,)", i, i, std::string(static_cast<std::size_t>(i % 9) * 40, ' '), i);
        json += R"("last":[1,2]})";
        check(std::type_identity<std::map<std::string, DocumentOf<std::vector<int>>>>{}, json);
    }

    // the root itself, and a value that is the whole of the root
    check(std::type_identity<DocumentOf<std::vector<int>>>{}, "[1,2]");
    check(std::type_identity<std::vector<DocumentOf<std::vector<int>>>>{}, "[[1,2]]");
    check(std::type_identity<std::vector<DocumentOf<std::vector<int>>>>{}, "[]");

    // a value that yyjson rejects fails the reader, and `read<T>` throws as reading a document does
    for (const auto* json : {R"([[1,2],[1,2})", R"([[1,2],[1,2])", R"([[1,2],[1,)", R"([[1,2],tru])", R"([[1,2],"a])",
                             R"([[1,2],[1,2]x])", R"([[1,2],01])"})
    {
        const auto text = std::string(json);
        EXPECT_THROW(direct(std::type_identity<std::vector<DocumentOf<std::vector<int>>>>{}, text),
                     reader::detail::direct_read_failure)
            << json;
        EXPECT_THROW((void)read<std::vector<DocumentOf<std::vector<int>>>>(text), read_error) << json;
    }
}

// an element that a constructor for any type would take in place of the value that its caster reads
struct TakesAny
{
    int value = 0;
    TakesAny() = default;
    explicit TakesAny(int v) : value(v) {}
    template <typename U>
        requires(!std::same_as<std::remove_cvref_t<U>, TakesAny> && !std::same_as<std::remove_cvref_t<U>, int>)
    TakesAny(U&&) : value(-1)
    {
    }
    bool operator==(const TakesAny&) const = default;
};

template <>
struct yyjson::caster<TakesAny>
{
    template <typename Json>
    static TakesAny from_json(const Json& json)
    {
        return TakesAny(cast<int>(json));
    }
};

TEST(DirectReader, ElementsInPlace)
{
    // the elements that casters read are constructed in place, but not by a constructor that takes any type
    const auto direct = []<typename T>(std::type_identity<T>, std::string_view json) {
        return yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1,
                                                     yyjson::ReadFlag::NoFlag)
            .read_root<T>();
    };
    EXPECT_EQ(direct(std::type_identity<std::vector<TakesAny>>{}, "[1,2,3]"),
              (std::vector<TakesAny>{TakesAny(1), TakesAny(2), TakesAny(3)}));
    EXPECT_EQ(direct(std::type_identity<std::vector<std::vector<TakesAny>>>{}, "[[1],[2,3]]"),
              (std::vector<std::vector<TakesAny>>{{TakesAny(1)}, {TakesAny(2), TakesAny(3)}}));
    EXPECT_EQ(yyjson::read<std::vector<TakesAny>>("[1,2,3]"),
              (std::vector<TakesAny>{TakesAny(1), TakesAny(2), TakesAny(3)}));
}

#endif

// NOLINTEND
