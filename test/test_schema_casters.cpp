#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, Casters)
{
    namespace casters = schema_test::casters;
    // a translator reads an integer and writes a string, and a field keyword checks either JSON
    check<casters::Digits>({"5", "-1", "1.0", R"("5")", "null"});
    check<casters::Counted>({R"({"d":5})", R"({"d":-1})", R"({"d":10})", R"({"d":"5"})", "{}"});
    const auto counted = compile(checked_schema(yyjson::json_schema<casters::Counted>(yyjson::write_direction)));
    EXPECT_FALSE(meets_constraints(casters::Counted{{100}}));
    EXPECT_FALSE(counted.is_valid(json::parse(std::string(yyjson::write(casters::Counted{{100}})))));
    EXPECT_EQ(schema<casters::Digits>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema",)"
                                         R"("type":"integer","minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(schema<casters::Digits>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    check<casters::Spot>({R"({"x":1,"y":2})", R"({"x":1})", R"({"x":"a","y":2})", "[1,2]"});
    EXPECT_EQ(definition<casters::Spot>(false), definition<schema_test::Point>(false));
    check<casters::Grade>({R"({"n":1})", R"({"n":0})", "{}"});
    const auto grade = compile(checked_schema(yyjson::json_schema<casters::Grade>(yyjson::write_direction)));
    EXPECT_FALSE(meets_constraints(casters::Grade{0}));
    EXPECT_FALSE(grade.is_valid(json::parse(std::string(yyjson::write(casters::Grade{0})))));
    // a field reads in place and any other value by the form that returns
    check_read<casters::Both>({"1", R"("ab")", "true"});
    check_read<casters::HoldsBoth>({R"({"b":"ab"})", R"({"b":1})", R"({"b":true})"});
    check_read<casters::NestsBoth>({R"({"v":[1],"o":2})", R"({"v":["ab"]})", R"({"v":[],"o":"ab"})"});
    check_read<casters::NotBoth>({R"({"n":"ab"})", R"({"n":1})", R"({"n":true})"});
    // the form that returns reads any JSON, and the field reads a string in place
    EXPECT_EQ(schema<casters::Dom>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema"})");
    check_read<casters::Dom>({"1", R"("ab")", "{}"});
    check_read<casters::HoldsDom>({R"({"d":"ab"})", R"({"d":1})"});
    check_read<casters::NotDom>({R"({"n":"ab"})", R"({"n":1})"});
    // the field reads its default 3 in place, which takes only a string, and the element by the form that returns
    EXPECT_EQ(definition<casters::BothDefaults>(false),
              json::parse(R"({"type":"object","properties":{"b":{"type":"string"},"v":{"type":"array","items":)"
                          R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"default":[4]}}})"));
#if YYJSON_VERSION_HEX >= 0x000C00
    // a caster that reads a `json_view` reads any JSON, which the keywords of its field check
    check<casters::Post>({R"({"tags":["a","b","c"]})", R"({"tags":"cpp20"})", R"({"tags":["cpp20"]})",
                          R"({"tags":"cpp"})", R"({"tags":["ab","cd"]})", R"({"tags":null})", R"({"tags":{}})"});
#endif
}

TEST(Schema, CasterKeywords)
{
    namespace casters = schema_test::casters;
    // an emitter writes the kind of its JSON, and the keywords of the type check the object in both directions
    const auto bags = {doc("{}"), doc(R"({"a":1})"),  doc(R"({"abc":1})"), doc(R"({"a":1,"b":2})"),
                       doc("[]"), doc(R"({"a":"x"})")};
    check<casters::Bag>(bags);
    check<casters::ClosedBag>(bags);
    check<casters::Seq>({"[]", "[1,2]", R"(["a"])", "{}"});
    EXPECT_EQ(schema<casters::Bag>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","additionalProperties":)"
              R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"propertyNames":{"maxLength":2},)"
              R"("minProperties":1})");
    EXPECT_EQ(schema<casters::Bag>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",)"
              R"("propertyNames":{"maxLength":2},"minProperties":1})");
    // without `to_json`, the write reflects the type and its keys pass the closed read
    check<casters::ReadOnly>({"{}", R"({"v":1})", R"({"w":1})", R"({"v":1,"w":1})", R"({"v":"x"})"});
    // without `from_json`, the read checks the keywords of the type on the reflected object, and `validate` on the
    // JSON of `to_json`
    check<casters::WriteOnly>({R"({"v":1})", R"({"v":1,"w":1})", "{}"});
    EXPECT_THROW(yyjson::validate(casters::WriteOnly{2}), yyjson::bad_cast);
    EXPECT_EQ(schema<casters::WriteOnly>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","additionalProperties":)"
              R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"maxProperties":1})");
    // the source of a caster that an internal tag selects reads the object with the tag
    check<casters::Plane>({R"({"type":"disk","r":1})", R"({"type":"disk","r":1,"x":1})", R"({"type":"square","a":1})",
                           R"({"r":1})", R"({"type":"disk"})"});
    // a caster type that holds itself has no name in `$defs`
    EXPECT_THROW((void)yyjson::json_schema<casters::Recurse>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<casters::Recurse>(yyjson::write_direction), std::invalid_argument);
}

TEST(Schema, CasterExamples)
{
    namespace casters = schema_test::casters;
    check<std::vector<casters::Color>>(
        {R"(["#ff8000","#000000"])", {R"(["#zz8000"])", unchecked}, R"([1])", "[]", R"("#ff8000")"});
    check<casters::Counter>({R"({"name":"b","hits":9})", R"({"name":"b","hits":"9"})", R"({"hits":1.5})"});
    check<casters::Shape>({R"({"kind":"circle","radius":1.5,"tags":["a","b"]})", R"({"kind":1})", "{}"});
    check<casters::Request>({R"({"params":{"timeout":500},"terms":[[1.5,0,1],[2.0,1]]})", R"({"params":1})"});
    check<casters::Config>({"{}", R"({"port":1})", R"({"port":"1"})"});
    EXPECT_EQ(required_keys(definition<casters::Config>(true)), std::set<std::string>{"port"});
    check<casters::Account>({R"({"id":"1f","password":"secret","keys":["c"]})",
                             {R"({"id":"zz","password":"","keys":[],"level":0})", unchecked},
                             R"({"id":31})",
                             R"({"keys":[1]})",
                             R"({"password":1})"});
    check<casters::HexChoice>({R"({"v":"ff"})",
                               R"({"v":true})",
                               R"({"v":1})",
                               R"({"v":"ff","o":"a"})",
                               {R"({"v":true,"o":null})", lenient_null},
                               R"({"v":true,"o":1})"});
}

TEST(Schema, Unconverted)
{
    namespace casters = schema_test::casters;
    check_read<int*>({"null", "0", "{}"});
    check_read<casters::Ctor>({"1", "1.5", "-1", R"("1")", "null", "true"});
    check_read<casters::Number>({"1", "1.5", "null", R"("1")"});
    check_read<std::chrono::seconds>({"5",
                                      "-5",
                                      {"5.0", integer_only},
                                      "18446744073709551615",
                                      "9223372036854775807",
                                      "-9223372036854775808",
                                      "1e308",
                                      R"("5")"});
    check_read<std::chrono::duration<std::uint8_t>>({"255", "256", "-1"});
    check_read<std::chrono::duration<double>>({"5", "1.5", R"("5")"});
    check_read<casters::Legacy>({R"({"p":null,"c":1,"n":1.5,"s":3})", R"({"p":1,"c":1,"n":1,"s":3})",
                                 R"({"p":null,"c":"x","n":1,"s":3})", R"({"p":null,"c":1,"n":1,"s":0.5})"});
    EXPECT_EQ(schema<int*>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"null"})");
    // the generator visits no field that the read and the write leave out
    check<casters::Skipping>({R"({"a":1})", R"({"a":1,"f":1})"});
    check<casters::Unlisted>({R"({"a":1})", R"({"a":1,"p":1})"});
    EXPECT_FALSE(definition<casters::Skipping>(true)["properties"].contains("f"));
    EXPECT_FALSE(definition<casters::Unlisted>(true)["properties"].contains("p"));
}

// NOLINTEND
