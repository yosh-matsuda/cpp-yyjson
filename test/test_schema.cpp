#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, Primitives)
{
    check<bool>({"true", "false", "0", "null", R"("true")"});
    check<int>({"0", "-2147483648", "2147483647", "2147483648", "-2147483649", "1.0", "1e2", "1.5", R"("1")", "null"});
    check<std::int8_t>({"-128", "127", "128", "-129"});
    check<std::uint8_t>({"0", "255", "256", "-1"});
    check<std::uint64_t>({"0", "18446744073709551615", "18446744073709551616", "-1"});
    check<std::int64_t>({"-9223372036854775808", "9223372036854775807", "9223372036854775808"});
    check<char>({"65", R"("A")", "128"});
    check<float>({"0", "1.5", "3.4028235e38", "-3.4028235e38", "3.4028235677973366e38", "3.5e38", "1e39", R"("1")"});
    check<double>({"0", "-1.5", "1e308", "1.7976931348623157e308", "-1.7976931348623157e308", "true"});
    check<long double>({"1.5", "-1e300"});
    check<std::string>({R"("")", R"("abc")", "1", "null"});
    check<std::nullptr_t>({"null", "0", "{}"});
    check<std::monostate>({"null", "false"});
    EXPECT_EQ(schema<std::string_view>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    EXPECT_EQ(schema<int>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"integer",)"
                             R"("minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(schema<float>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"number",)"
              R"("minimum":-3.4028235e38,"maximum":3.4028235e38})");
    EXPECT_EQ(schema<double>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"number",)"
                                R"("minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308})");
    // `read<T>` cannot give a `const char*`, but `cast` can, from a document that the program keeps
    const auto pointer = to_jsoncons(yyjson::json_schema<const char*>(yyjson::write_direction));
    EXPECT_TRUE(compile(pointer).is_valid(json::parse(std::string(yyjson::write("a")))));
    EXPECT_EQ(schema<const char*>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    // `write` gives a C array of `char` at the root as a string
    const auto letters = to_jsoncons(yyjson::json_schema<char[4]>(yyjson::write_direction));
    char text[4] = "ab";
    EXPECT_TRUE(compile(letters).is_valid(json::parse(std::string(yyjson::write(text)))));
    EXPECT_EQ(letters["maxLength"].as<int>(), 4);
}

TEST(Schema, DomTypes)
{
    constexpr auto head = R"({"$schema":"https://json-schema.org/draft/2020-12/schema")";
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(schema<yyjson::json_view>(), std::string(head) + "}");
#endif
    EXPECT_EQ(schema<yyjson::value>(), std::string(head) + "}");
    EXPECT_EQ(schema<yyjson::object>(yyjson::write_direction), std::string(head) + R"(,"type":"object"})");
    EXPECT_EQ(schema<yyjson::array>(), std::string(head) + R"(,"type":"array"})");
    check<yyjson::value>({"null", "1", "[]", "{}"});
    check<yyjson::object>({"{}", R"({"a":[1]})", "[]", "1"});
    check<yyjson::array>({"[]", "[1,{}]", "{}", "null"});
}

TEST(Schema, Enums)
{
    check<Hue>({R"("red")", R"("blue")", R"("Red")", "0", "null"});
    check<Level>({"-1", "1", "0", "127", "-128", "128", "1.0", R"("low")"});
    check<Flags>({R"("none")", R"("write")", "0", "2", "5", "65535", "65536", "-1", R"("5")", R"("all")"});
    EXPECT_EQ(definition<Hue>(false).to_string(), R"({"enum":["red","green","blue"],"type":"string"})");
    EXPECT_EQ(definition<Level>(true).to_string(), R"({"maximum":127,"minimum":-128,"type":"integer"})");
    EXPECT_EQ(definition<Flags>(true), definition<Flags>(false));
    EXPECT_TRUE(definition<Flags>(true).contains("anyOf"));
    check<Switch>({"0", "1", "2", "-1", "true"});
}

TEST(Schema, Sequences)
{
    check<std::vector<int>>({"[]", "[1,2,2]", R"([1,"a"])", "{}", "null", "[2147483648]"});
    check<std::list<std::string>>({"[]", R"(["a"])", "[1]"});
    check<std::deque<double>>({"[1.5]", "[null]"});
    check<std::set<int>>({"[]", "[1,2]", {"[2,2]", lenient_repeat}, "[1.5]"});
    check<std::unordered_set<std::string>>({R"(["a","b"])", {R"(["a","a"])", lenient_repeat}});
    check<std::multiset<int>>({"[2,2,1]", "[]"});
    check<std::set<std::shared_ptr<int>>>({"[1,1]", "[]"});
    check<std::set<int, std::greater<>>>({"[1,2]", {"[2,2]", lenient_repeat}});
    check<std::array<int, 2>>({"[1,2]", "[1]", "[1,2,3]", "[]"});
    check<std::vector<bool>>({"[true,false]", "[1]", "[]"});
    check<std::vector<std::vector<Hue>>>({R"([["red"],[]])", R"([["pink"]])", "[[0]]"});
    const auto set = to_jsoncons(yyjson::json_schema<std::set<int>>());
    EXPECT_TRUE(set["uniqueItems"].as<bool>());
    EXPECT_FALSE(to_jsoncons(yyjson::json_schema<std::multiset<int>>()).contains("uniqueItems"));
    // pointers that differ can point to equal values, and an order of the user can take unequal values as equal
    EXPECT_FALSE(to_jsoncons(yyjson::json_schema<std::set<std::shared_ptr<int>>>(yyjson::write_direction))
                     .contains("uniqueItems"));
    EXPECT_TRUE(to_jsoncons(yyjson::json_schema<std::unordered_set<std::string>>()).contains("uniqueItems"));
    if constexpr (std::numeric_limits<long double>::digits > std::numeric_limits<double>::digits)
    {
        EXPECT_FALSE(
            to_jsoncons(yyjson::json_schema<std::set<long double>>(yyjson::write_direction)).contains("uniqueItems"));
    }
    const auto array = to_jsoncons(yyjson::json_schema<std::array<int, 3>>(yyjson::write_direction));
    EXPECT_EQ(array["minItems"].as<int>(), 3);
    EXPECT_EQ(array["maxItems"].as<int>(), 3);
}

TEST(Schema, Maps)
{
    check<std::map<std::string, int>>(
        {"{}", R"({"a":1,"b":2})", R"({"a":"x"})", "[]", R"([["a",1]])", {R"({"a":1,"a":2})", not_expressible}});
    check<std::unordered_map<std::string, std::vector<int>>>({R"({"a":[1]})", R"({"a":1})"});
    check<std::map<Hue, int>>({"{}", R"({"red":1})", R"({"pink":1})", R"({"0":1})", R"([["red",1]])"});
    check<std::map<Level, int>>({R"({"-1":1,"1":2})", R"({"0":1})", R"({"low":1})", R"({"01":1})", R"({"+1":1})",
                                 R"({"-0":1})", R"({"-128":1})", R"({"127":1})", R"({"128":1})", R"({"-129":1})"});
    check<std::map<Switch, int>>({R"({"0":1,"1":2})", R"({"2":1})", R"({"01":1})", R"({"true":1})"});
    check<std::map<Flags, int>>({R"({"read":1})", R"({"3":1})", R"({"65535":1})", R"({"65536":1})", R"({"-1":1})",
                                 R"({"all":1})", R"({"03":1})"});
    check<std::map<int, std::string>>(
        {"[]", R"([[1,"a"],[2,"b"]])", R"({"1":"a"})", R"([[1]])", {R"([[1,"a"],[1,"b"]])", not_expressible}});
    check<std::vector<std::pair<std::string, int>>>({"{}", R"({"a":1})", R"([["a",1]])", R"({"a":"x"})"});
    check<std::array<std::pair<std::string, int>, 2>>({R"({"a":1,"b":2})", R"([["a",1],["b",2]])", R"({"a":1})",
                                                       R"({"a":1,"b":2,"c":3})", R"([["a",1]])", R"({"a":"x","b":1})"});
    check<std::array<std::pair<Hue, int>, 1>>({R"({"red":1})", R"({"pink":1})", R"([["red",1]])", "{}"});
    EXPECT_EQ((schema<std::array<std::pair<std::string, int>, 2>>(yyjson::write_direction)),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","minProperties":2,)"
              R"("maxProperties":2,"additionalProperties":{"type":"integer","minimum":-2147483648,)"
              R"("maximum":2147483647}})");
    check<PairArrays>({R"({"c":{"a":1,"b":2}})", R"({"c":[["a",1],["b",2]]})", R"({"c":{"a":1}})",
                       R"({"c":{"a":1,"b":2,"c":3}})", R"({"c":[["a",1]]})", R"({"c":{"a":"x","b":1}})"});
    check<Grid>({R"({"g":[[1,2],[3,4]],"p":[{"a":1},[["b",2]]]})", R"({"g":[[1,2],[3]]})", R"({"g":[[1,2]]})",
                 R"({"p":[{"a":1},{"b":2,"c":3}]})", R"({"p":[{"a":1}]})", R"({"g":[[1,2],[3,"x"]]})"});
    check<std::map<std::string, Point>>({R"({"p":{"x":1,"y":2}})", R"({"p":{"x":1}})", R"({"p":1})"});
    constexpr auto level_keys =
        "^(0|[1-9][0-9]{0,1}|1[0-1][0-9]|12[0-6]|127|-([1-9][0-9]{0,1}|1[0-1][0-9]|12[0-7]|128))$";
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Level, int>>())["propertyNames"]["pattern"].as<std::string>(),
              level_keys);
    EXPECT_EQ(
        to_jsoncons(yyjson::json_schema<std::map<Level, int>>(yyjson::write_direction))["propertyNames"]["pattern"]
            .as<std::string>(),
        level_keys);
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Flags, int>>(
                  yyjson::write_direction))["propertyNames"]["anyOf"][1]["pattern"]
                  .as<std::string>(),
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-4]|65535)$");
}

TEST(Schema, Tuples)
{
    check<std::tuple<int, std::string>>({R"([1,"a"])", "[1]", R"([1,"a",2])", R"(["a",1])", "{}"});
    check<std::pair<std::string, bool>>({R"(["a",true])", R"({"a":true})"});
    check<std::tuple<>>({"[]", "{}", "[1]", R"({"a":1})", "null"});
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>>(
        {{R"({"a":1,"b":"x"})", lenient},
         R"([["a",1],["b","x"]])",
         R"({"a":1})",
         R"({"a":1,"b":2})",
         R"({"a":"x","b":1})"});
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>>(
        {R"({"a":1,"b":2})", R"({"a":1,"b":"x"})", R"([["a",1],["b",2]])", R"({"a":1,"b":2,"c":3})"});
    check<std::tuple<std::pair<Hue, int>>>({R"([["red",1]])", R"({"red":1})"});
    EXPECT_EQ(schema<std::tuple<>>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","maxProperties":0})");
    const auto pairs =
        to_jsoncons(yyjson::json_schema<std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>>(
            yyjson::write_direction));
    EXPECT_EQ(pairs["type"].as<std::string>(), "object");
}

TEST(Schema, Nullable)
{
    check<std::optional<int>>({"null", "1", R"("1")", "1.5"});
    check<std::optional<Point>>({"null", R"({"x":1,"y":2})", "1"});
    check<std::unique_ptr<std::string>>({"null", R"("a")", "1"});
    check<std::shared_ptr<Hue>>({"null", R"("red")", "0"});
    check<yyjson::nullable<double>>({"null", "1.5", "true"});
    check<yyjson::indirect<Point>>({R"({"x":1,"y":2})", "null"});
    EXPECT_EQ(schema<std::optional<int>>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["integer","null"],)"
              R"("minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::optional<std::vector<int>>>())["anyOf"][1].to_string(),
              R"({"type":"null"})");
}

// NOLINTEND
